/**
 * @file fsgreetingpatterns.cpp
 * @brief Regular expressions, kept per account, that tell greetings, tips and thanks in chat.
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

#include "fsgreetingpatterns.h"

#include "llsdutil.h"
#include "llviewercontrol.h"

#include <map>

namespace
{
    struct CacheEntry
    {
        LLSD mValue;
        FSGreetingPatterns::Compiled mCompiled;
    };
    std::map<std::string, CacheEntry> sCache;

    const boost::regex::flag_type REGEX_FLAGS = boost::regex::perl | boost::regex::icase;
}

std::vector<std::string> FSGreetingPatterns::getPatterns(const std::string& setting)
{
    std::vector<std::string> patterns;
    if (!gSavedPerAccountSettings.controlExists(setting))
    {
        return patterns;
    }
    const LLSD value = gSavedPerAccountSettings.getLLSD(setting);
    for (LLSD::array_const_iterator it = value.beginArray(); it != value.endArray(); ++it)
    {
        patterns.push_back(it->asString());
    }
    return patterns;
}

void FSGreetingPatterns::setPatterns(const std::string& setting, const std::vector<std::string>& patterns)
{
    if (!gSavedPerAccountSettings.controlExists(setting))
    {
        return;
    }
    LLSD value = LLSD::emptyArray();
    for (const std::string& pattern : patterns)
    {
        value.append(pattern);
    }
    gSavedPerAccountSettings.setLLSD(setting, value);
}

FSGreetingPatterns::Compiled FSGreetingPatterns::compile(const std::vector<std::string>& patterns)
{
    Compiled compiled;
    for (const std::string& pattern : patterns)
    {
        if (pattern.empty())
        {
            continue;
        }
        try
        {
            compiled.mRegexes.emplace_back(pattern, boost::regex(pattern, REGEX_FLAGS));
        }
        catch (const std::exception& e)
        {
            compiled.mErrors.push_back(pattern + ": " + e.what());
        }
    }
    return compiled;
}

const FSGreetingPatterns::Compiled& FSGreetingPatterns::get(const std::string& setting)
{
    CacheEntry& entry = sCache[setting];
    const LLSD value = gSavedPerAccountSettings.controlExists(setting) ? gSavedPerAccountSettings.getLLSD(setting) : LLSD();
    if (entry.mValue.isUndefined() || !llsd_equals(entry.mValue, value))
    {
        entry.mValue = value.isUndefined() ? LLSD::emptyArray() : value;
        entry.mCompiled = compile(getPatterns(setting));
    }
    return entry.mCompiled;
}

std::vector<FSGreetingPatterns::Span> FSGreetingPatterns::findAll(const Compiled& compiled, const std::string& text)
{
    std::vector<Span> spans;
    for (const auto& [pattern, regex] : compiled.mRegexes)
    {
        try
        {
            for (boost::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
            {
                const size_t first = (size_t)it->position();
                spans.emplace_back(first, first + (size_t)it->length());
            }
        }
        catch (const std::exception&)
        {
            // Too complex to match: as if it didn't
        }
    }
    return spans;
}

bool FSGreetingPatterns::findFirst(const Compiled& compiled, const std::string& text, std::string* capture, std::string* pattern_out)
{
    for (const auto& [pattern, regex] : compiled.mRegexes)
    {
        try
        {
            boost::smatch match;
            if (boost::regex_search(text, match, regex))
            {
                if (capture)
                {
                    *capture = match.size() > 1 && match[1].matched ? match[1].str() : match[0].str();
                }
                if (pattern_out)
                {
                    *pattern_out = pattern;
                }
                return true;
            }
        }
        catch (const std::exception&)
        {
        }
    }
    return false;
}

std::string FSGreetingPatterns::blank(const std::string& text, const std::vector<Span>& spans)
{
    std::string result = text;
    for (const auto& [first, second] : spans)
    {
        for (size_t i = first; i < second && i < result.size(); ++i)
        {
            result[i] = ' ';
        }
    }
    return result;
}
