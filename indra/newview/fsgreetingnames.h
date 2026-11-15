/**
 * @file fsgreetingnames.h
 * @brief Compares names said in chat with avatar names, seeing through lookalike letters.
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

#ifndef FS_GREETINGNAMES_H
#define FS_GREETINGNAMES_H

#include "llstring.h"

// Names in chat are usually short forms ("Bel" for Bella Lark) and display
// names are often written in lookalike letters ("ᴘᴇʙʙʟᴇ", "Nєℓℓɪє", "R0s3").
// Both sides are folded to plain lowercase Latin letters before they are
// compared: lookalikes become the letter they imitate, accents are dropped,
// and l and i count as the same letter, because a 1 or a | could stand for
// either. Words in other scripts are kept as they are.
namespace FSGreetingNames
{
    // The words of something said in chat, folded. Digits and punctuation
    // separate words; apostrophes are dropped, so "D'Arcy" is one word.
    std::vector<LLWString> chatWords(std::string_view text);

    // Every folded form of an avatar's name that a word in chat can match:
    // the words of the display name and username, once with digits read as
    // letters ("R0s3" is "rose") and once with digits as separators
    // ("Mia77" is "mia"). Runs of single letters are also joined, so
    // "K T" can be matched by "KT", and CamelCase names are split, so
    // "HollyBramble" is also "holly" and "bramble".
    std::vector<LLWString> nameKeys(std::string_view display_name, std::string_view username);

    // True if the words greet someone with "welcome", but not "you're welcome".
    bool isWelcome(const std::vector<LLWString>& words);

    // Words that are never names in a greeting: "welcome", "to", "and", "everyone"...
    bool isFillerWord(const LLWString& word);
}

#endif // FS_GREETINGNAMES_H
