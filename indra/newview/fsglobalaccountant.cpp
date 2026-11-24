/**
 * @file fsglobalaccountant.cpp
 * @brief Starts SL Money, the app that shows the money logs.
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

#include "fsglobalaccountant.h"

#include "lldir.h"
#include "llfile.h"
#include "llnotificationsutil.h"
#include "llprocess.h"
#include "llviewercontrol.h"

#include <cstdlib>

namespace
{
    std::string env(const char* name)
    {
        const char* value = getenv(name);
        return value ? value : "";
    }

    // The copy packaged with the viewer first (fs_bundle_slmoney in fs_viewer_manifest.py),
    // then where SL Money's own installers put it, the user's own folder first.
    std::vector<std::string> candidates()
    {
        std::string custom = gSavedSettings.getString("FSGlobalAccountantPath");
        if (!custom.empty())
        {
            return { custom };
        }
        const std::string sep = gDirUtilp->getDirDelimiter();
        const std::string viewer = gDirUtilp->getAppRODataDir(); // the install folder; Contents/Resources on macOS
        std::vector<std::string> paths;
#if LL_WINDOWS
        paths.push_back(viewer + sep + "slmoney" + sep + "SL Money.exe");
        if (!env("LOCALAPPDATA").empty())
        {
            paths.push_back(env("LOCALAPPDATA") + "\\Programs\\slmoney\\SL Money.exe");
        }
#elif LL_DARWIN
        paths.push_back(viewer + sep + "SL Money.app");
        if (!env("HOME").empty())
        {
            paths.push_back(env("HOME") + "/Applications/SL Money.app");
        }
        paths.push_back("/Applications/SL Money.app");
#else
        paths.push_back(viewer + sep + "slmoney" + sep + "slmoney");
        if (!env("HOME").empty())
        {
            paths.push_back(env("HOME") + "/Applications/slmoney/slmoney");
        }
        paths.push_back("/opt/slmoney/slmoney");
#endif
        return paths;
    }

    LLProcess::Params launch_params(const std::string& path)
    {
        LLProcess::Params params;
#if LL_DARWIN
        // An app bundle is started through Launch Services, which also brings a running one forward.
        params.executable = "/usr/bin/open";
        params.args.add(path);
#elif LL_LINUX
        // The viewer's start script points LD_LIBRARY_PATH at the viewer's own libraries and keeps
        // the original in SAVED_LD_LIBRARY_PATH (as launch_url.sh does for the browser): give
        // SL Money the original, so it doesn't load the viewer's libraries.
        params.executable = "/usr/bin/env";
        if (getenv("SAVED_LD_LIBRARY_PATH"))
        {
            std::string saved = env("SAVED_LD_LIBRARY_PATH");
            if (saved.empty())
            {
                params.args.add("-u");
                params.args.add("LD_LIBRARY_PATH");
            }
            else
            {
                params.args.add("LD_LIBRARY_PATH=" + saved);
            }
        }
        params.args.add(path);
#else
        params.executable = path;
#endif
        params.autokill = false; // it keeps running after the viewer quits
        return params;
    }
}

void FSGlobalAccountant::launch()
{
    for (const std::string& path : candidates())
    {
        if (!LLFile::isfile(path) && !LLFile::isdir(path))
        {
            continue;
        }
        if (!LLProcess::create(launch_params(path)))
        {
            LL_WARNS("GlobalAccountant") << "Could not start " << path << LL_ENDL;
            LLNotificationsUtil::add("GlobalAccountantFailed", LLSD().with("PATH", path));
        }
        return;
    }
    LLNotificationsUtil::add("GlobalAccountantMissing");
}
