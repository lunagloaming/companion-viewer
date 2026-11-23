/**
 * @file fsimagebc7.cpp
 * @brief BC7 compression of decoded textures, checked block by block.
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

#include "linden_common.h"

#include "fsimagebc7.h"

#include "llimage.h"

// bc7enc.h has no include guard and declares a global color_rgba: include it here only.
#include "bc7enc/bc7enc.h"
#include "bc7enc/bc7decomp.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <mutex>

namespace
{
    // A block is off when a channel of a texel is more than this far from its source...
    constexpr S32 MAX_CHANNEL_ERROR = 32;
    // ...or when the block's squared error is above that of 8 levels on every channel of every texel.
    constexpr U32 MAX_BLOCK_SQUARED_ERROR = 8 * 8 * 16 * 4;
    // A texture stays uncompressed when more than this share of its blocks (all levels) are off.
    constexpr F32 MAX_BAD_BLOCK_SHARE = 0.01f;

    std::atomic<U32> sCompressed{ 0 };
    std::atomic<U32> sRejected{ 0 };
    std::atomic<U32> sRejectedAlpha{ 0 };
    std::atomic<U64> sMicroseconds{ 0 };
    std::atomic<U64> sBytesSaved{ 0 };
    std::atomic<U64> sDecodeMicroseconds{ 0 };
    std::once_flag sEncoderInit;

    // Halves an RGBA8 image with a 2x2 box filter; an odd edge repeats its last texel.
    void half_size(const std::vector<U8>& src, U32 width, U32 height, std::vector<U8>& dst, U32& dst_width, U32& dst_height)
    {
        dst_width = llmax(width / 2, 1U);
        dst_height = llmax(height / 2, 1U);
        dst.resize((size_t)dst_width * dst_height * 4);
        for (U32 y = 0; y < dst_height; ++y)
        {
            const U32 y0 = llmin(y * 2, height - 1);
            const U32 y1 = llmin(y * 2 + 1, height - 1);
            for (U32 x = 0; x < dst_width; ++x)
            {
                const U32 x0 = llmin(x * 2, width - 1);
                const U32 x1 = llmin(x * 2 + 1, width - 1);
                const U8* a = &src[((size_t)y0 * width + x0) * 4];
                const U8* b = &src[((size_t)y0 * width + x1) * 4];
                const U8* c = &src[((size_t)y1 * width + x0) * 4];
                const U8* d = &src[((size_t)y1 * width + x1) * 4];
                U8* out = &dst[((size_t)y * dst_width + x) * 4];
                for (S32 i = 0; i < 4; ++i)
                {
                    out[i] = (U8)((a[i] + b[i] + c[i] + d[i] + 2) / 4);
                }
            }
        }
    }

    U64 blocks_in(U32 width, U32 height)
    {
        return (U64)((width + 3) / 4) * ((height + 3) / 4);
    }
}

U64 FSImageBC7::getBytes() const
{
    U64 bytes = 0;
    for (const Level& level : mLevels)
    {
        bytes += level.blocks.size();
    }
    return bytes;
}

FSImageBC7::Stats FSImageBC7::takeStats()
{
    Stats stats;
    stats.compressed = sCompressed.exchange(0);
    stats.rejected = sRejected.exchange(0);
    stats.rejected_alpha = sRejectedAlpha.exchange(0);
    stats.microseconds = sMicroseconds.exchange(0);
    stats.bytes_saved = sBytesSaved.exchange(0);
    stats.decode_microseconds = sDecodeMicroseconds.exchange(0);
    return stats;
}

void FSImageBC7::addDecodeTime(U64 microseconds)
{
    sDecodeMicroseconds += microseconds;
}

LLPointer<FSImageBC7> FSImageBC7::compress(const LLImageRaw* raw)
{
    if (!raw || !raw->getData())
    {
        return nullptr;
    }
    const S32 components = raw->getComponents();
    U32 width = raw->getWidth();
    U32 height = raw->getHeight();
    if ((components != 3 && components != 4) || !width || !height)
    {
        return nullptr;
    }

    std::call_once(sEncoderInit, bc7enc_compress_block_init);
    const auto start = std::chrono::steady_clock::now();

    // Linear weights: these textures hold colours, normals and masks alike, and the check below
    // measures each channel on its own.
    bc7enc_compress_block_params params;
    bc7enc_compress_block_params_init(&params);
    bc7enc_compress_block_params_init_linear_weights(&params);

    // Level 0 as RGBA8; RGB gets an opaque alpha, which the check then requires exactly.
    std::vector<U8> pixels((size_t)width * height * 4);
    const U8* src = raw->getData();
    for (size_t i = 0, n = (size_t)width * height; i < n; ++i)
    {
        pixels[i * 4 + 0] = src[i * components + 0];
        pixels[i * 4 + 1] = src[i * components + 1];
        pixels[i * 4 + 2] = src[i * components + 2];
        pixels[i * 4 + 3] = components == 4 ? src[i * components + 3] : 255;
    }

    U64 total_blocks = 0;
    for (U32 w = width, h = height;; w = llmax(w / 2, 1U), h = llmax(h / 2, 1U))
    {
        total_blocks += blocks_in(w, h);
        if (w == 1 && h == 1)
        {
            break;
        }
    }
    const U64 allowed_bad_blocks = (U64)(total_blocks * MAX_BAD_BLOCK_SHARE);

    LLPointer<FSImageBC7> image = new FSImageBC7;
    image->mSourceComponents = components;
    U64 source_bytes = 0;
    U64 bad_blocks = 0;
    bool alpha_harmed = false;
    std::vector<U8> next;

    while (!alpha_harmed && bad_blocks <= allowed_bad_blocks)
    {
        Level level;
        level.width = width;
        level.height = height;
        const U32 blocks_x = (width + 3) / 4;
        const U32 blocks_y = (height + 3) / 4;
        level.blocks.resize((size_t)blocks_x * blocks_y * 16);
        source_bytes += (U64)width * height * components;

        for (U32 by = 0; by < blocks_y && !alpha_harmed && bad_blocks <= allowed_bad_blocks; ++by)
        {
            for (U32 bx = 0; bx < blocks_x; ++bx)
            {
                // The block's texels; past an edge of a small level, the last row or column repeats
                U8 block_pixels[16 * 4];
                for (U32 py = 0; py < 4; ++py)
                {
                    const U32 y = llmin(by * 4 + py, height - 1);
                    for (U32 px = 0; px < 4; ++px)
                    {
                        const U32 x = llmin(bx * 4 + px, width - 1);
                        memcpy(&block_pixels[(py * 4 + px) * 4], &pixels[((size_t)y * width + x) * 4], 4);
                    }
                }

                U8* block = &level.blocks[((size_t)by * blocks_x + bx) * 16];
                bc7enc_compress_block(block, block_pixels, &params);

                bc7decomp::color_rgba decoded[16];
                bc7decomp::unpack_bc7(block, decoded);

                S32 max_error = 0;
                U32 squared_error = 0;
                for (U32 py = 0; py < 4; ++py)
                {
                    for (U32 px = 0; px < 4; ++px)
                    {
                        if (bx * 4 + px >= width || by * 4 + py >= height)
                        {
                            continue; // padding
                        }
                        const U32 i = py * 4 + px;
                        for (S32 c = 0; c < 4; ++c)
                        {
                            const S32 error = std::abs((S32)decoded[i].m_comps[c] - (S32)block_pixels[i * 4 + c]);
                            max_error = llmax(max_error, error);
                            squared_error += (U32)(error * error);
                        }
                        // A mask cutoff turns the smallest error at an alpha end into holes or specks
                        const U8 source_alpha = block_pixels[i * 4 + 3];
                        if ((source_alpha == 0 || source_alpha == 255) && decoded[i].m_comps[3] != source_alpha)
                        {
                            alpha_harmed = true;
                        }
                    }
                }
                if (max_error > MAX_CHANNEL_ERROR || squared_error > MAX_BLOCK_SQUARED_ERROR)
                {
                    ++bad_blocks;
                }
                if (alpha_harmed)
                {
                    break;
                }
            }
        }

        image->mLevels.push_back(std::move(level));
        if (width == 1 && height == 1)
        {
            break;
        }
        U32 next_width, next_height;
        half_size(pixels, width, height, next, next_width, next_height);
        pixels.swap(next);
        width = next_width;
        height = next_height;
    }

    const U64 microseconds = (U64)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
    sMicroseconds += microseconds;

    if (alpha_harmed || bad_blocks > allowed_bad_blocks)
    {
        ++sRejected;
        if (alpha_harmed)
        {
            ++sRejectedAlpha;
        }
        return nullptr;
    }

    ++sCompressed;
    const U64 bytes = image->getBytes();
    if (source_bytes > bytes)
    {
        sBytesSaved += source_bytes - bytes;
    }
    return image;
}
