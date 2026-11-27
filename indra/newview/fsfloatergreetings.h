/**
 * @file fsfloatergreetings.h
 * @brief The Greetings floater: the patterns that tell welcomes in chat, with a line to try them on.
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

#ifndef FS_FLOATERGREETINGS_H
#define FS_FLOATERGREETINGS_H

#include "llfloater.h"

#include <functional>

class LLLineEditor;
class LLTextBox;
class LLTextEditor;

// Edits the pattern lists of FSGreetingPatterns, one tab per kind of line. Each list is a text
// editor with one regular expression per line, saved as it is typed; patterns that aren't valid
// are listed under it, and a line typed in the tab's test box shows what would come of it.
class FSFloaterGreetings : public LLFloater
{
public:
    FSFloaterGreetings(const LLSD& key);

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    struct PatternList
    {
        LLTextEditor*   mEditor = nullptr;
        std::string     mSetting;
    };
    struct Tab
    {
        std::vector<PatternList>                        mLists;
        LLTextBox*                                      mErrors = nullptr;
        LLLineEditor*                                   mTest = nullptr;
        LLTextBox*                                      mResult = nullptr;
        std::function<std::string(const std::string&)>  mTry;
    };

    // editors: control name and setting for each list on the tab; prefix names its errors, test and result controls
    void addTab(const std::string& prefix, const std::vector<std::pair<std::string, std::string>>& editors,
                std::function<std::string(const std::string&)> try_line);
    void onEdited(size_t tab);
    void refreshTab(size_t tab);

    std::string tryWelcome(const std::string& line) const;
    // The words of a line that could be names, once filler words are left out
    std::string nameWords(const std::string& text) const;

    std::vector<Tab> mTabs;
};

#endif // FS_FLOATERGREETINGS_H
