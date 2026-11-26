/**
 * @file fsgreetingpatterns.h
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

#ifndef FS_GREETINGPATTERNS_H
#define FS_GREETINGPATTERNS_H

#include <boost/regex.hpp>

#include <string>
#include <utility>
#include <vector>

// Lists of regular expressions in per-account settings (LLSD arrays of strings),
// edited in the Greetings floater. Case is ignored. A pattern that isn't a valid
// regular expression is left out and reported, never thrown.
namespace FSGreetingPatterns
{
    struct Compiled
    {
        std::vector<std::pair<std::string, boost::regex>> mRegexes;  // the pattern and its regex
        std::vector<std::string> mErrors;                            // "pattern: what is wrong"
    };

    // A part of a line a pattern matched: byte offsets [first, second)
    using Span = std::pair<size_t, size_t>;

    std::vector<std::string> getPatterns(const std::string& setting);
    void setPatterns(const std::string& setting, const std::vector<std::string>& patterns);

    Compiled compile(const std::vector<std::string>& patterns);
    // The setting's patterns, compiled once until they change
    const Compiled& get(const std::string& setting);

    // Every part of the text any of the patterns matches
    std::vector<Span> findAll(const Compiled& compiled, const std::string& text);
    // The first match of any pattern, and its first capture group if it has one (else the whole match)
    bool findFirst(const Compiled& compiled, const std::string& text, std::string* capture = nullptr, std::string* pattern = nullptr);
    // The text with those parts replaced by spaces
    std::string blank(const std::string& text, const std::vector<Span>& spans);
}

#endif // FS_GREETINGPATTERNS_H
