/**
 * @file fsmissingassetlog.cpp
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

#include "llviewerprecompiledheaders.h"

#include "fsmissingassetlog.h"

#include "llagent.h"
#include "fscommon.h"
#include "llstartup.h"
#include "llface.h"
#include "llfetchedgltfmaterial.h"
#include "llselectmgr.h"
#include "lltextureentry.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llvoavatarself.h"
#include "llvovolume.h"

#include <set>

namespace
{
    const char* texture_channel_name(U32 channel)
    {
        switch (channel)
        {
            case LLRender::DIFFUSE_MAP:            return "diffuse texture";
            case LLRender::NORMAL_MAP:             return "normal map";
            case LLRender::SPECULAR_MAP:           return "specular map";
            case LLRender::BASECOLOR_MAP:          return "PBR base colour";
            case LLRender::METALLIC_ROUGHNESS_MAP: return "PBR metallic-roughness";
            case LLRender::GLTF_NORMAL_MAP:        return "PBR normal map";
            case LLRender::EMISSIVE_MAP:           return "PBR emissive";
            default:                               return "texture";
        }
    }

    const char* fetch_type_name(FTType type)
    {
        switch (type)
        {
            case FTT_DEFAULT:     return "asset";
            case FTT_SERVER_BAKE: return "server bake";
            case FTT_HOST_BAKE:   return "region bake";
            case FTT_MAP_TILE:    return "map tile";
            case FTT_LOCAL_FILE:  return "local file";
            default:              return "unknown";
        }
    }

    // The selection or hover tip may know the object's name and owner.
    void append_name_and_owner(std::ostringstream& out, LLViewerObject* root)
    {
        LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->findNode(root);
        LLSelectNode* hover = LLSelectMgr::getInstance()->getHoverNode();
        if (!node && hover && hover->getObject() == root)
        {
            node = hover;
        }
        if (node && node->mValid)
        {
            if (!node->mName.empty())
            {
                out << ", named '" << node->mName << "'";
            }
            if (node->mPermissions)
            {
                out << ", owner " << node->mPermissions->getOwner();
            }
        }
    }

    const size_t MAX_FACES_LOGGED = 25;

    // Where a face is, in a few words, for local chat.
    std::string short_place(LLViewerObject* object, S32 face)
    {
        if (!object)
        {
            return "an unknown object";
        }
        std::ostringstream out;
        LLViewerObject* root = object->getRootEdit() ? object->getRootEdit() : object;
        LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->findNode(root);
        LLSelectNode* hover = LLSelectMgr::getInstance()->getHoverNode();
        if (!node && hover && hover->getObject() == root)
        {
            node = hover;
        }
        if (node && node->mValid && !node->mName.empty())
        {
            out << "'" << node->mName << "' ";
        }
        out << "object " << object->getID();
        if (face >= 0)
        {
            out << " face " << face;
        }
        if (root->isAttachment())
        {
            LLVOAvatar* avatar = object->getAvatar();
            out << (avatar && avatar == gAgentAvatarp ? ", worn by you" : ", worn by someone");
            if (object->isHUDAttachment())
            {
                out << " (HUD)";
            }
        }
        else if (LLViewerRegion* region = object->getRegion())
        {
            const LLVector3 position = region->getPosRegionFromAgent(object->getPositionAgent());
            out << ", " << region->getName() << llformat(" %.0f, %.0f, %.0f", position.mV[VX], position.mV[VY], position.mV[VZ]);
        }
        return out.str();
    }
}

void FSMissingAssetLog::warnInChat(const std::string& text)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
    static LLCachedControl<bool> enabled(gSavedSettings, "FSMissingAssetChatWarning", true);
    if (!enabled || LLStartUp::getStartupState() < STATE_STARTED)
    {
        return;
    }
    // A broken region can fail dozens of assets at once: the log has them all.
    static U32 warnings = 0;
    const U32 MAX_WARNINGS = 25;
    if (warnings > MAX_WARNINGS)
    {
        return;
    }
    if (++warnings > MAX_WARNINGS)
    {
        FSCommon::report_to_nearby_chat("Missing asset warnings: more than " + std::to_string(MAX_WARNINGS)
            + " this session, further ones only in the log.");
        return;
    }
    FSCommon::report_to_nearby_chat(text);
#endif
}

std::string FSMissingAssetLog::describeFace(LLViewerObject* object, S32 face)
{
    std::ostringstream out;
    if (!object)
    {
        return "an unknown object";
    }

    out << "object " << object->getID() << " (local id " << object->getLocalID() << ")";
    if (face >= 0)
    {
        out << ", face " << face;
    }
    if (object->isAvatar())
    {
        out << ", an avatar's body";
    }

    LLViewerObject* root = object->getRootEdit();
    if (!root)
    {
        root = object;
    }
    if (root != object)
    {
        out << ", in the linkset of root " << root->getID() << " (" << root->getChildren().size() + 1 << " prims)";
    }
    else if (!object->isAvatar())
    {
        out << ", linkset root (" << object->getChildren().size() + 1 << " prims)";
    }
    append_name_and_owner(out, root);

    if (root->isAttachment())
    {
        LLVOAvatar* avatar = object->getAvatar();
        out << ", attached to avatar " << (avatar ? avatar->getID() : LLUUID::null);
        if (avatar && avatar == gAgentAvatarp)
        {
            out << " (you)";
        }
        out << " at attachment point " << ATTACHMENT_ID_FROM_STATE(root->getAttachmentState());
        if (object->isHUDAttachment())
        {
            out << " (HUD)";
        }
    }
    if (object->isAnimatedObject())
    {
        out << ", animated mesh";
    }

    LLViewerRegion* region = object->getRegion();
    if (region && !object->isHUDAttachment())
    {
        const LLVector3 position = region->getPosRegionFromAgent(object->getPositionAgent());
        out << ", region '" << region->getName() << "' at "
            << llformat("%.1f, %.1f, %.1f", position.mV[VX], position.mV[VY], position.mV[VZ])
            << llformat(", %.1f m from you", dist_vec(object->getPositionAgent(), gAgent.getPositionAgent()));
    }

    if (face >= 0 && face < object->getNumTEs())
    {
        const LLTextureEntry* entry = object->getTE(face);
        if (entry)
        {
            out << ", face texture " << entry->getID();
        }
        const LLUUID& material_id = object->getRenderMaterialID(face);
        if (material_id.notNull())
        {
            out << ", PBR material " << material_id;
        }
    }
    return out.str();
}

void FSMissingAssetLog::logTextureGivenUp(LLViewerFetchedTexture* texture, const std::string& reason)
{
    if (!texture)
    {
        return;
    }
    const LLUUID& id = texture->getID();
    LL_WARNS("MissingTexture") << "Texture " << id << " could not be loaded and now shows the missing texture placeholder. Reason: "
        << reason << "; type " << fetch_type_name(texture->getFTType()) << ", boost level " << texture->getBoostLevel()
        << (texture->getUrl().empty() ? std::string() : ", URL " + texture->getUrl()) << LL_ENDL;

    size_t logged = 0;
    size_t not_logged = 0;
    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = texture->getFaceList(channel);
        const S32 count = llmin(texture->getNumFaces(channel), faces ? (S32)faces->size() : 0);
        for (S32 i = 0; i < count; ++i)
        {
            LLFace* face = (*faces)[i];
            if (!face)
            {
                continue;
            }
            if (logged >= MAX_FACES_LOGGED)
            {
                ++not_logged;
                continue;
            }
            ++logged;
            LL_WARNS("MissingTexture") << "Texture " << id << " is the " << texture_channel_name(channel) << " of "
                << describeFace(face->getViewerObject(), face->getTEOffset()) << LL_ENDL;
        }
    }
    for (U32 channel = 0; channel < LLRender::NUM_VOLUME_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_volume_list_t* volumes = texture->getVolumeList(channel);
        const S32 count = llmin(texture->getNumVolumes(channel), volumes ? (S32)volumes->size() : 0);
        for (S32 i = 0; i < count; ++i)
        {
            LLVOVolume* volume = (*volumes)[i];
            if (!volume)
            {
                continue;
            }
            if (logged >= MAX_FACES_LOGGED)
            {
                ++not_logged;
                continue;
            }
            ++logged;
            LL_WARNS("MissingTexture") << "Texture " << id << " is the "
                << (channel == LLRender::SCULPT_TEX ? "sculpt map" : "projector light texture") << " of "
                << describeFace(volume, -1) << LL_ENDL;
        }
    }
    if (not_logged > 0)
    {
        LL_WARNS("MissingTexture") << "Texture " << id << " is used by " << not_logged << " more faces, not listed" << LL_ENDL;
    }
    if (logged == 0)
    {
        LL_WARNS("MissingTexture") << "Texture " << id
            << " is not on any object face right now (it may be a UI image, an avatar bake, terrain, sky,"
            << " or a material texture not yet drawn)" << LL_ENDL;
    }

    std::string where = "no object face";
    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS && where == "no object face"; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = texture->getFaceList(channel);
        if (faces && texture->getNumFaces(channel) > 0 && !faces->empty() && (*faces)[0])
        {
            where = short_place((*faces)[0]->getViewerObject(), (*faces)[0]->getTEOffset());
        }
    }
    const size_t total = logged + not_logged;
    warnInChat("Missing texture " + id.asString() + " (" + reason + ") on " + where
        + (total > 1 ? " and " + std::to_string(total - 1) + " more" : std::string())
        + ". Showing the placeholder; details in the log (MissingTexture).");
}

void FSMissingAssetLog::logMaterialGivenUp(const LLUUID& id, const LLFetchedGLTFMaterial* material, const std::string& reason)
{
    LL_WARNS("MissingMaterial") << "PBR material " << id
        << " could not be loaded and is now the plain grey placeholder material. Reason: " << reason << LL_ENDL;

    size_t logged = 0;
    size_t not_logged = 0;
    std::string first_place;
    for (S32 i = 0; i < gObjectList.getNumObjects(); ++i)
    {
        LLViewerObject* object = gObjectList.getObject(i);
        if (!object || object->isDead())
        {
            continue;
        }
        for (S32 face = 0; face < object->getNumTEs(); ++face)
        {
            const LLTextureEntry* entry = object->getTE(face);
            if (!entry || (entry->getGLTFMaterial() != material && object->getRenderMaterialID(face) != id))
            {
                continue;
            }
            if (first_place.empty())
            {
                first_place = short_place(object, face);
            }
            if (logged >= MAX_FACES_LOGGED)
            {
                ++not_logged;
                continue;
            }
            ++logged;
            LL_WARNS("MissingMaterial") << "PBR material " << id << " is used by " << describeFace(object, face) << LL_ENDL;
        }
    }
    if (not_logged > 0)
    {
        LL_WARNS("MissingMaterial") << "PBR material " << id << " is used by " << not_logged << " more faces, not listed" << LL_ENDL;
    }
    if (logged == 0)
    {
        LL_WARNS("MissingMaterial") << "PBR material " << id
            << " is not on any object face right now (it may be a terrain material or not yet drawn)" << LL_ENDL;
        first_place = "no object face";
    }
    const size_t total = logged + not_logged;
    warnInChat("Missing PBR material " + id.asString() + " (" + reason + ") on " + first_place
        + (total > 1 ? " and " + std::to_string(total - 1) + " more" : std::string())
        + ". Showing plain grey; details in the log (MissingMaterial).");
}

void FSMissingAssetLog::logRenderMaterialMissing(const LLTextureEntry* entry)
{
    // Called whenever such a face is drawn: log each one once.
    static std::set<const LLTextureEntry*> logged;
    if (!entry || logged.size() > 1000 || !logged.insert(entry).second)
    {
        return;
    }

    LLViewerObject* found = nullptr;
    S32 found_face = -1;
    for (S32 i = 0; i < gObjectList.getNumObjects() && !found; ++i)
    {
        LLViewerObject* object = gObjectList.getObject(i);
        if (!object || object->isDead())
        {
            continue;
        }
        for (S32 face = 0; face < object->getNumTEs(); ++face)
        {
            if (object->getTE(face) == entry)
            {
                found = object;
                found_face = face;
                break;
            }
        }
    }

    const LLFetchedGLTFMaterial* base = dynamic_cast<const LLFetchedGLTFMaterial*>(entry->getGLTFMaterial());
    const LLGLTFMaterial* override_material = entry->getGLTFMaterialOverride();
    LL_WARNS("MissingMaterial") << "PBR override without a render material on "
        << (found ? describeFace(found, found_face) : std::string("a face not found in the object list"))
        << "; base material " << (base ? (base->isFetching() ? "still loading" : (base->isLoaded() ? "loaded" : "failed to load")) : "none")
        << ", override " << (override_material ? override_material->asJSON() : std::string("none")) << LL_ENDL;
}

namespace
{
    // Installs the callback LLTextureEntry::getGLTFRenderMaterial() calls (llprimitive cannot see objects).
    struct InstallRenderMaterialCallback
    {
        InstallRenderMaterialCallback()
        {
            LLTextureEntry::sRenderMaterialMissingCallback = &FSMissingAssetLog::logRenderMaterialMissing;
        }
    } install_render_material_callback;
}
