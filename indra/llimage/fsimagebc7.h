/**
 * @file fsimagebc7.h
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

#ifndef FS_IMAGEBC7_H
#define FS_IMAGEBC7_H

#include "stdtypes.h"
#include "llpointer.h"
#include "llrefcount.h"

#include <vector>

class LLImageRaw;

// A decoded texture and its mipmaps compressed to BC7: 4x4 texels in 16 bytes, a quarter of RGBA8.
// Made on the decode threads; LLImageGL uploads the levels as they are. A texture BC7 would visibly
// harm (fine-grained normal maps, thin alpha lattices) is left uncompressed: every block is decoded
// back and compared with its source.
class FSImageBC7 : public LLThreadSafeRefCount
{
public:
    struct Level
    {
        U32 width = 0;
        U32 height = 0;
        std::vector<U8> blocks;
    };

    // Compresses an RGB or RGBA image with its whole mipmap chain, down to 1x1. Returns null when the
    // image has another number of channels, or when BC7 would harm it.
    static LLPointer<FSImageBC7> compress(const LLImageRaw* raw);

    const std::vector<Level>& getLevels() const { return mLevels; }
    U32 getWidth() const { return mLevels.empty() ? 0 : mLevels[0].width; }
    U32 getHeight() const { return mLevels.empty() ? 0 : mLevels[0].height; }
    S32 getSourceComponents() const { return mSourceComponents; }
    U64 getBytes() const;

    // Counted since the last call: textures compressed and left alone (and of those, how many for their
    // alpha ends), microseconds spent on both (encoding and checking), and bytes saved against RGBA8 or
    // RGB8 with mipmaps.
    struct Stats
    {
        U32 compressed = 0;
        U32 rejected = 0;
        U32 rejected_alpha = 0;
        U64 microseconds = 0;
        U64 bytes_saved = 0;
        U64 decode_microseconds = 0; // JPEG 2000 decoding on the same threads, to compare
    };
    static Stats takeStats();
    static void addDecodeTime(U64 microseconds);

private:
    std::vector<Level> mLevels;
    S32 mSourceComponents = 0;
};

#endif // FS_IMAGEBC7_H
