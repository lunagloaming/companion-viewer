/**
 * @file fsfloatergreetings.cpp
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

#include "llviewerprecompiledheaders.h"

#include "fsfloatergreetings.h"

#include "fsgreetingnames.h"
#include "fsgreetingpatterns.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltexteditor.h"

namespace
{
    std::vector<std::string> splitLines(const std::string& text)
    {
        std::vector<std::string> lines;
        std::string line;
        for (char c : text)
        {
            if (c == '\n')
            {
                lines.push_back(line);
                line.clear();
            }
            else if (c != '\r')
            {
                line += c;
            }
        }
        lines.push_back(line);
        std::vector<std::string> patterns;
        for (const std::string& l : lines)
        {
            if (!l.empty())
            {
                patterns.push_back(l);
            }
        }
        return patterns;
    }

    std::string joinLines(const std::vector<std::string>& lines)
    {
        std::string text;
        for (const std::string& line : lines)
        {
            if (!text.empty())
            {
                text += '\n';
            }
            text += line;
        }
        return text;
    }
}

FSFloaterGreetings::FSFloaterGreetings(const LLSD& key)
    : LLFloater(key)
{
}

bool FSFloaterGreetings::postBuild()
{
    mTabs.reserve(4); // the editors' callbacks hold indexes into it
    addTab("welcome", { { "welcome_patterns", "FSGreetingWelcomePatterns" }, { "not_welcome_patterns", "FSGreetingNotWelcomePatterns" } },
        [this](const std::string& line) { return tryWelcome(line); });
    return true;
}

void FSFloaterGreetings::addTab(const std::string& prefix, const std::vector<std::pair<std::string, std::string>>& editors,
                                std::function<std::string(const std::string&)> try_line)
{
    const size_t index = mTabs.size();
    Tab& tab = mTabs.emplace_back();
    for (const auto& [control, setting] : editors)
    {
        PatternList list;
        list.mEditor = getChild<LLTextEditor>(control);
        list.mSetting = setting;
        list.mEditor->setKeystrokeCallback([this, index](LLTextEditor*) { onEdited(index); });
        tab.mLists.push_back(list);
    }
    tab.mErrors = getChild<LLTextBox>(prefix + "_errors");
    tab.mTest = getChild<LLLineEditor>(prefix + "_test");
    tab.mResult = getChild<LLTextBox>(prefix + "_result");
    tab.mTry = std::move(try_line);
    tab.mTest->setKeystrokeCallback([this, index](LLLineEditor*, void*) { refresh(index); }, nullptr);
}

void FSFloaterGreetings::onOpen(const LLSD& key)
{
    for (size_t i = 0; i < mTabs.size(); ++i)
    {
        for (PatternList& list : mTabs[i].mLists)
        {
            list.mEditor->setText(joinLines(FSGreetingPatterns::getPatterns(list.mSetting)));
        }
        refresh(i);
    }
}

void FSFloaterGreetings::onEdited(size_t index)
{
    for (PatternList& list : mTabs[index].mLists)
    {
        FSGreetingPatterns::setPatterns(list.mSetting, splitLines(list.mEditor->getText()));
    }
    refresh(index);
}

void FSFloaterGreetings::refresh(size_t index)
{
    Tab& tab = mTabs[index];
    std::string errors;
    for (const PatternList& list : tab.mLists)
    {
        for (const std::string& error : FSGreetingPatterns::compile(splitLines(list.mEditor->getText())).mErrors)
        {
            errors += (errors.empty() ? "" : "\n") + error;
        }
    }
    if (!errors.empty())
    {
        LLStringUtil::format_map_t args;
        args["[ERRORS]"] = errors;
        errors = getString("bad_patterns", args);
    }
    tab.mErrors->setText(errors);

    const std::string line = tab.mTest->getText();
    tab.mResult->setText(line.empty() ? std::string() : tab.mTry(line));
}

std::string FSFloaterGreetings::nameWords(const std::string& text) const
{
    std::string names;
    for (const LLWString& word : FSGreetingNames::chatWords(text))
    {
        if (!FSGreetingNames::isFillerWord(word))
        {
            names += (names.empty() ? "" : ", ") + wstring_to_utf8str(word);
        }
    }
    return names;
}

std::string FSFloaterGreetings::tryWelcome(const std::string& line) const
{
    // As FSRadar::onOwnNearbyChat reads a line
    std::string rest = FSGreetingPatterns::blank(line,
        FSGreetingPatterns::findAll(FSGreetingPatterns::get("FSGreetingNotWelcomePatterns"), line));
    const std::vector<FSGreetingPatterns::Span> welcome = FSGreetingPatterns::findAll(FSGreetingPatterns::get("FSGreetingWelcomePatterns"), rest);
    if (welcome.empty())
    {
        return getString("not_a_welcome");
    }
    const std::string names = nameWords(FSGreetingPatterns::blank(rest, welcome));
    if (names.empty())
    {
        return getString("welcome_no_names");
    }
    LLStringUtil::format_map_t args;
    args["[NAMES]"] = names;
    return getString("welcome_names", args);
}
