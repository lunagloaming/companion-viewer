/**
 * @file fsgreetingnames_test.cpp
 * @brief Tests for matching names said in chat with avatar names (fsgreetingnames.h).
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

#include "../test/lltut.h"

#include "../fsgreetingnames.h"

#include <algorithm>

// All names here are invented.
namespace
{
    // A plain word folded as a word said in chat would be
    LLWString folded(const std::string& word)
    {
        const std::vector<LLWString> words = FSGreetingNames::chatWords(word);
        return words.empty() ? LLWString() : words.front();
    }

    bool has_key(const std::vector<LLWString>& keys, const std::string& word)
    {
        return std::find(keys.begin(), keys.end(), folded(word)) != keys.end();
    }

    std::string joined(const std::vector<LLWString>& words)
    {
        std::string text;
        for (const LLWString& word : words)
        {
            text += (text.empty() ? "" : " ") + wstring_to_utf8str(word);
        }
        return text;
    }
}

namespace tut
{
    struct greetingnames_data
    {
    };
    typedef test_group<greetingnames_data> greetingnames_group;
    typedef greetingnames_group::object greetingnames_object;
    tut::greetingnames_group greetingnames_testgroup("FSGreetingNames");

    // Words of chat: case, punctuation, apostrophes, digits
    template<> template<>
    void greetingnames_object::test<1>()
    {
        ensure_equals("words, lowercase, punctuation dropped", joined(FSGreetingNames::chatWords("Welcome, Quorra!")),
                      joined(FSGreetingNames::chatWords("welcome quorra")));
        ensure_equals("an apostrophe joins", joined(FSGreetingNames::chatWords("D'Ambrin")), joined(FSGreetingNames::chatWords("dambrin")));
        ensure_equals("digits separate", FSGreetingNames::chatWords("Kea77").size(), (size_t)1);
        ensure("l and i are the same letter", folded("Lulla") == folded("Iuiia"));
    }

    // Lookalike letters fold to the letters they imitate
    template<> template<>
    void greetingnames_object::test<2>()
    {
        ensure("small capitals", has_key(FSGreetingNames::nameKeys("ᴘᴇʙʙʟᴇ", "pebble.resident"), "pebble"));
        ensure("Greek capitals look like Latin capitals", has_key(FSGreetingNames::nameKeys("ΤΗΙΚΑ", "x1"), "thika"));
        ensure("Greek capital mu and nu", has_key(FSGreetingNames::nameKeys("ΝΙΜΑ", "x1"), "nima"));
        ensure("Greek capital upsilon", has_key(FSGreetingNames::nameKeys("ΥΑΚΙ", "x1"), "yaki"));
        ensure("Greek lowercase by shape", has_key(FSGreetingNames::nameKeys("Nεℓℓιε", "x1"), "nellie"));
        ensure("Cyrillic и reads as n", has_key(FSGreetingNames::nameKeys("Nαиcу", "x1"), "nancy"));
        ensure("Cyrillic capital У is Y", has_key(FSGreetingNames::nameKeys("УАRA", "x1"), "yara"));
        ensure("mathematical script", has_key(FSGreetingNames::nameKeys("𝓚𝓮𝓪", "x1"), "kea"));
        ensure("fullwidth", has_key(FSGreetingNames::nameKeys("Ｋｅａ", "x1"), "kea"));
        ensure("circled", has_key(FSGreetingNames::nameKeys("ⓚⓔⓐ", "x1"), "kea"));
        ensure("negative squared", has_key(FSGreetingNames::nameKeys("🅺🅴🅰", "x1"), "kea"));
        ensure("regional indicators", has_key(FSGreetingNames::nameKeys("🇰🇪🇦", "x1"), "kea"));
        ensure("accents dropped", has_key(FSGreetingNames::nameKeys("Ilvë", "x1"), "ilve"));
        ensure("CJK decoration letters", has_key(FSGreetingNames::nameKeys("尺ﾑ卄", "x1"), "rah"));
        ensure("Thai fancy alphabet", has_key(FSGreetingNames::nameKeys("ค๒๔", "x1"), "abd"));
        ensure("decoration around a name", has_key(FSGreetingNames::nameKeys("亗Wren亗", "x1"), "wren"));
        ensure("leet digits in names", has_key(FSGreetingNames::nameKeys("Qu1ll", "x1"), "quill"));
    }

    // Name keys: digits, CamelCase, single letters, usernames, Resident
    template<> template<>
    void greetingnames_object::test<3>()
    {
        const std::vector<LLWString> keys = FSGreetingNames::nameKeys("OrielVantwick", "oriel.vantwick");
        ensure("whole display name", has_key(keys, "orielvantwick"));
        ensure("CamelCase first part", has_key(keys, "oriel"));
        ensure("CamelCase second part", has_key(keys, "vantwick"));
        ensure("digits as separators", has_key(FSGreetingNames::nameKeys("Kea77", "x1"), "kea"));
        ensure("single letters joined", has_key(FSGreetingNames::nameKeys("K T", "x1"), "kt"));
        ensure("username words", has_key(FSGreetingNames::nameKeys("Somebody", "wren.ashdown"), "ashdown"));
        ensure("Resident is never a name", !has_key(FSGreetingNames::nameKeys("Wren", "wren.resident"), "resident"));

        std::vector<LLWString> sorted = keys;
        std::sort(sorted.begin(), sorted.end());
        ensure("keys sorted and unique", sorted == keys && std::adjacent_find(keys.begin(), keys.end()) == keys.end());
    }

    // Welcomes, and replies to thanks that aren't welcomes
    template<> template<>
    void greetingnames_object::test<4>()
    {
        using FSGreetingNames::chatWords;
        using FSGreetingNames::isWelcome;
        ensure("welcome with a name", isWelcome(chatWords("welcome Quorra!")));
        ensure("welcome back", isWelcome(chatWords("Welcome back, everyone :)")));
        ensure("welcome later in the line", isWelcome(chatWords("hi Juniper, welcome to the club")));
        ensure("you're welcome", !isWelcome(chatWords("you're welcome")));
        ensure("youre welcome", !isWelcome(chatWords("youre welcome")));
        ensure("your welcome", !isWelcome(chatWords("your welcome")));
        ensure("ur welcome", !isWelcome(chatWords("ur welcome")));
        ensure("you are welcome", !isWelcome(chatWords("you are welcome")));
        ensure("not part of a word", !isWelcome(chatWords("an unwelcome guest")));
        ensure("no welcome", !isWelcome(chatWords("hello Quorra")));
    }

    // Filler words are never taken for names
    template<> template<>
    void greetingnames_object::test<5>()
    {
        for (const char* word : { "welcome", "everyone", "back", "to", "the", "hun", "Hi" })
        {
            ensure(std::string("filler: ") + word, FSGreetingNames::isFillerWord(folded(word)));
        }
        for (const char* word : { "quorra", "juniper", "vantwick" })
        {
            ensure(std::string("not filler: ") + word, !FSGreetingNames::isFillerWord(folded(word)));
        }
    }
}
