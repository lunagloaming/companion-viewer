/**
 * @file fsmissingassetlog.h
 * @brief Logs which objects use a texture or material the viewer could not load.
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

#ifndef FS_MISSINGASSETLOG_H
#define FS_MISSINGASSETLOG_H

#include "stdtypes.h"

#include <string>

class LLTextureEntry;
class LLViewerFetchedTexture;
class LLViewerObject;

// Log lines for assets the viewer gives up on, detailed enough to find the object in world:
// object and linkset IDs, face, region and position, attachment and avatar.
namespace FSMissingAssetLog
{
    // One object face, for the log.
    std::string describeFace(LLViewerObject* object, S32 face);

    // A texture the viewer gave up on (it now shows the missing texture placeholder),
    // with every face and object that uses it.
    void logTextureGivenUp(LLViewerFetchedTexture* texture, const std::string& reason);

    // A face with a PBR material override but no render material made from it
    // (LLTextureEntry::getGLTFRenderMaterial()). Logged once per face.
    void logRenderMaterialMissing(const LLTextureEntry* entry);
}

#endif // FS_MISSINGASSETLOG_H
