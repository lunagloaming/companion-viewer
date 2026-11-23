/**
 * @file fstexturebudget.cpp
 * @brief Chooses the texture discard bias from what the textures in view would need.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2026 Luna Witch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fstexturebudget.h"

#include "llframetimer.h"
#include "fsimagebc7.h"
#include "llviewertexture.h"

namespace
{
    // The bias falls only when the textures then fit in this share of the target.
    constexpr F32 BIAS_FALL_SHARE = 0.9f;
    // Predictions miss textures not visited yet and decodes in flight. If measured use stays this far
    // over the target for this long, the bias steps up anyway.
    constexpr F32 MEASURED_OVER_SHARE = 1.05f;
    constexpr F32 MEASURED_OVER_SECS = 5.f;
    constexpr F32 SUMMARY_SECS = 60.f;
    // No fall this soon after a rise. Right after textures shrink, what they would need at a lower bias reads
    // low (sizes they asked for before the rise are forgotten), and the bias swung 1, 2.5, 1, 2, 2.5 within
    // seconds.
    constexpr F32 FALL_HOLD_SECS = 10.f;

    U64 sPredictedKB[FSTextureBudget::BIAS_COUNT] = {};
    U64 sCurrentKB = 0;
    bool sOverBudget = false;
    U32 sShrunkInView = 0;
    U32 sBiasChanges = 0;

    S32 index_of(F32 bias)
    {
        S32 index = 0;
        for (S32 i = 0; i < FSTextureBudget::BIAS_COUNT; ++i)
        {
            if (FSTextureBudget::BIASES[i] <= bias + 0.01f)
            {
                index = i;
            }
        }
        return index;
    }
}

void FSTextureBudget::update(LLViewerFetchedTexture* texture)
{
    U64 bytes[BIAS_COUNT];
    texture->predictGPUBytes(BIASES, BIAS_COUNT, bytes);
    const U32 current_kb = (U32)(texture->currentGPUBytes() >> 10);

    if (texture->mInBudgetTotals)
    {
        for (S32 i = 0; i < BIAS_COUNT; ++i)
        {
            sPredictedKB[i] -= texture->mBudgetPredictedKB[i];
        }
        sCurrentKB -= texture->mBudgetCurrentKB;
    }

    for (S32 i = 0; i < BIAS_COUNT; ++i)
    {
        texture->mBudgetPredictedKB[i] = (U32)(bytes[i] >> 10);
        sPredictedKB[i] += texture->mBudgetPredictedKB[i];
    }
    texture->mBudgetCurrentKB = current_kb;
    sCurrentKB += current_kb;
    texture->mInBudgetTotals = true;
}

void FSTextureBudget::forget(LLViewerFetchedTexture* texture)
{
    if (!texture->mInBudgetTotals)
    {
        return;
    }
    for (S32 i = 0; i < BIAS_COUNT; ++i)
    {
        sPredictedKB[i] -= texture->mBudgetPredictedKB[i];
    }
    sCurrentKB -= texture->mBudgetCurrentKB;
    texture->mInBudgetTotals = false;
}

F32 FSTextureBudget::chooseBias(F32 current_bias, const Inputs& in)
{
    // Render targets, media and textures not visited yet: the same at every bias
    const F32 other_mb = llmax(in.used_mb - (F32)(sCurrentKB / 1024.0), 0.f);
    F32 predicted_mb[BIAS_COUNT];
    for (S32 i = 0; i < BIAS_COUNT; ++i)
    {
        predicted_mb[i] = (F32)(sPredictedKB[i] / 1024.0) + other_mb;
    }

    auto fits = [&](F32 limit)
    {
        for (S32 i = 0; i < BIAS_COUNT; ++i)
        {
            if (predicted_mb[i] <= limit)
            {
                return i;
            }
        }
        return BIAS_COUNT - 1;
    };

    const S32 current = index_of(current_bias);
    const S32 needed = fits(in.target_mb);
    S32 chosen = needed;
    if (needed < current)
    {
        chosen = llmin(fits(in.target_mb * BIAS_FALL_SHARE), current);
    }

    // What is measured wins over what is predicted: never fall while over the target, and step up
    // when it stays over. Never fall soon after a rise either.
    static LLFrameTimer over_timer;
    static LLFrameTimer rise_timer;
    static bool risen = false;
    const bool over = in.used_mb > in.target_mb;
    if (over || (risen && rise_timer.getElapsedTimeF32() < FALL_HOLD_SECS))
    {
        chosen = llmax(chosen, current);
    }
    const char* cause = "the textures wanted";
    if (in.used_mb > in.target_mb * MEASURED_OVER_SHARE)
    {
        if (over_timer.getElapsedTimeF32() > MEASURED_OVER_SECS && chosen == current && current < BIAS_COUNT - 1)
        {
            chosen = current + 1;
            cause = "measured use staying over";
            over_timer.reset();
        }
    }
    else
    {
        over_timer.reset();
    }
    sOverBudget = over;

    if (chosen > current)
    {
        risen = true;
        rise_timer.reset();
    }

    if (chosen != current)
    {
        ++sBiasChanges;
        LL_INFOS("TextureMemory") << "Discard bias " << BIASES[current] << " -> " << BIASES[chosen] << " (" << cause
                                  << "): would need " << (S32)predicted_mb[current] << " MB at " << BIASES[current]
                                  << ", " << (S32)predicted_mb[chosen] << " MB at " << BIASES[chosen]
                                  << "; target " << (S32)in.target_mb << " MB, using " << (S32)in.used_mb << " MB"
                                  << (in.system_cap_mb > 0.f ? " (capped for low system memory)" : "") << LL_ENDL;
    }

    static LLFrameTimer summary_timer;
    if (summary_timer.getElapsedTimeF32() > SUMMARY_SECS)
    {
        summary_timer.reset();
        LL_INFOS("TextureMemory") << "Textures " << (S32)in.texture_mb << " MB (render targets " << (S32)in.render_target_mb
                                  << "), vertex buffers " << (S32)in.vertex_mb << " MB; target " << (S32)in.target_mb
                                  << " MB; bias " << BIASES[chosen] << ", predicted " << (S32)predicted_mb[chosen]
                                  << " MB; card free " << in.card_free_mb << " MB, evictions " << in.evictions
                                  << "; system memory cap " << (S32)in.system_cap_mb << " MB; last minute: "
                                  << sBiasChanges << " bias changes, " << sShrunkInView
                                  << " textures shrunk while on screen" << LL_ENDL;
        const FSImageBC7::Stats bc7 = FSImageBC7::takeStats();
        if (bc7.compressed || bc7.rejected)
        {
            LL_INFOS("TextureMemory") << "BC7 last minute: " << bc7.compressed << " textures compressed, " << bc7.rejected
                                      << " left uncompressed (" << bc7.rejected_alpha << " of them for holes or specks in alpha), "
                                      << (S32)(bc7.bytes_saved >> 20) << " MB saved; compressing took "
                                      << (S32)(bc7.microseconds / 1000) << " ms of decode thread time, decoding "
                                      << (S32)(bc7.decode_microseconds / 1000) << " ms" << LL_ENDL;
        }
        sBiasChanges = 0;
        sShrunkInView = 0;
    }

    return BIASES[chosen];
}

bool FSTextureBudget::isOverBudget()
{
    return sOverBudget;
}

void FSTextureBudget::noteShrunkInView()
{
    ++sShrunkInView;
}
