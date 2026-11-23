/**
 * @file fstexturebudget.h
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

#ifndef FS_TEXTUREBUDGET_H
#define FS_TEXTUREBUDGET_H

class LLViewerFetchedTexture;

// The discard bias makes every texture coarser than its faces ask for, to fit a memory budget.
// It used to creep up while video memory was over budget and down again once there was room, so it
// overshot on teleports and in crowds and kept swinging: textures in view blurred and sharpened again.
//
// Here each texture works out, whenever the texture list visits it, how much it would take on the GPU
// at each candidate bias. Their running totals say which bias fits the budget before anything loads:
// the smallest that fits. The bias only falls again when the textures then fit in 90% of the budget,
// so it does not swing back and forth at the edge.
namespace FSTextureBudget
{
    constexpr int BIAS_COUNT = 7;
    // Candidate biases, finest first. 1.5 only affects textures off screen, 2 and up those on screen too
    // (LLViewerTextureList::updateImageDecodePriority).
    constexpr float BIASES[BIAS_COUNT] = { 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f };

    // Keeps the running totals: called when the texture list visits a texture, and when one is destroyed.
    void update(LLViewerFetchedTexture* texture);
    void forget(LLViewerFetchedTexture* texture);

    struct Inputs
    {
        float target_mb = 0.f;        // what textures and vertex buffers may use
        float used_mb = 0.f;          // what they use now (render targets count as textures)
        float texture_mb = 0.f;
        float vertex_mb = 0.f;
        float render_target_mb = 0.f;
        int card_free_mb = -1;        // free on the card as the driver says, -1 if unknown
        int evictions = -1;           // NVIDIA eviction count, -1 if unknown
        float system_cap_mb = 0.f;    // cap while system memory is low, 0 if none
    };

    // Once a second from LLViewerTexture::updateClass: returns the bias to use. Logs changes, and a
    // summary once a minute.
    float chooseBias(float current_bias, const Inputs& inputs);

    // Over the budget at the last check: textures one level finer than wanted are shrunk too.
    bool isOverBudget();

    // A texture with a face on screen was shrunk (counted for the log).
    void noteShrunkInView();
}

#endif // FS_TEXTUREBUDGET_H
