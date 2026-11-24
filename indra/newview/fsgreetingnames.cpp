/**
 * @file fsgreetingnames.cpp
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

#include "llviewerprecompiledheaders.h"

#include "fsgreetingnames.h"

#include <algorithm>
#include <array>
#include <set>

namespace
{
    struct Lookalike
    {
        U32 mCode;
        const char* mLatin;
    };

    // Letters of Latin, Greek, Cyrillic and other alphabets, small capitals, styled, enclosed and fullwidth
    // letters, with the plain lowercase letters they look like, sorted by code point. Generated from Unicode's
    // data by scripts/code_tools/fs_greeting_lookalikes.py, which says how each is chosen; edit that, not this.
    constexpr Lookalike LOOKALIKES[] =
    {
#include "fsgreetinglookalikes.inc"
    };

    bool isAsciiLetter(llwchar c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    enum class CharKind
    {
        Latin,      // folded to plain letters
        Other,      // a letter of another script, kept as it is
        Joiner,     // dropped without ending the word: accents, apostrophes, zero-width characters
        Separator   // ends the word: spaces, punctuation, symbols, emoji
    };

    enum class Digits
    {
        AsLetters,  // 0 is o, 1 is i, 3 is e, 4 is a, $ is s...
        Separate
    };

    // A digit from the fullwidth or mathematical digits, or -1.
    S32 styledDigit(llwchar c)
    {
        if (c >= 0xFF10 && c <= 0xFF19)
        {
            return c - 0xFF10;
        }
        if (c >= 0x1D7CE && c <= 0x1D7FF)
        {
            return (c - 0x1D7CE) % 10;
        }
        return -1;
    }

    bool isJoiner(llwchar c)
    {
        return c == '\'' || c == '`' || c == 0x2018 || c == 0x2019 || c == 0x02BC || c == 0x02BB
            || (c >= 0x0300 && c <= 0x036F)     // combining accents
            || (c >= 0x0483 && c <= 0x0489)     // combining Cyrillic
            || (c >= 0x0610 && c <= 0x061A) || (c >= 0x064B && c <= 0x065F) || c == 0x0670
            || (c >= 0x06D6 && c <= 0x06ED)     // Arabic marks, used as decoration
            || (c >= 0x1AB0 && c <= 0x1AFF) || (c >= 0x1DC0 && c <= 0x1DFF)
            || (c >= 0x200B && c <= 0x200F)     // zero-width characters
            || (c >= 0x20D0 && c <= 0x20FF)     // combining symbols
            || (c >= 0xFE00 && c <= 0xFE0F)     // variation selectors
            || (c >= 0xFE20 && c <= 0xFE2F);
    }

    bool isSeparator(llwchar c)
    {
        return c < 0x80
            || (c >= 0x80 && c <= 0xBF) || c == 0xD7 || c == 0xF7
            || (c >= 0x02C2 && c <= 0x02FF)     // spacing modifiers: ˙ ˚ ˜
            || (c >= 0x2000 && c <= 0x2BFF)     // punctuation, symbols, arrows, shapes, dingbats
            || (c >= 0x2E00 && c <= 0x2E7F)
            || (c >= 0x3000 && c <= 0x303F)     // CJK punctuation
            || (c >= 0x3200 && c <= 0x33FF)     // enclosed CJK, used as decoration
            || (c >= 0xE000 && c <= 0xF8FF)     // private use
            || (c >= 0xFE10 && c <= 0xFE6F)
            || (c >= 0xFF00 && c <= 0xFF0F) || (c >= 0xFF1A && c <= 0xFF20) || (c >= 0xFF3B && c <= 0xFF40) || (c >= 0xFF5B && c <= 0xFF64)
            || (c >= 0x1F000 && c <= 0x1FAFF);  // emoji and other pictographs
    }

    void appendLatin(LLWString& out, llwchar c)
    {
        out += (c == 'l') ? 'i' : c;
    }

    CharKind foldChar(llwchar c, Digits digits, LLWString& out)
    {
        if (isAsciiLetter(c))
        {
            appendLatin(out, (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
            return CharKind::Latin;
        }

        S32 digit = (c >= '0' && c <= '9') ? (S32)(c - '0') : styledDigit(c);
        if (digit >= 0)
        {
            if (digits == Digits::Separate)
            {
                return CharKind::Separator;
            }
            constexpr char LEET_DIGITS[] = "oizeasgtbg";
            appendLatin(out, LEET_DIGITS[digit]);
            return CharKind::Latin;
        }

        if (digits == Digits::AsLetters && (c == '$' || c == '@' || c == '|'))
        {
            appendLatin(out, c == '$' ? 's' : c == '@' ? 'a' : 'i');
            return CharKind::Latin;
        }

        if (isJoiner(c))
        {
            return CharKind::Joiner;
        }

        auto found = std::lower_bound(std::begin(LOOKALIKES), std::end(LOOKALIKES), (U32)c,
            [](const Lookalike& entry, U32 code) { return entry.mCode < code; });
        if (found != std::end(LOOKALIKES) && found->mCode == (U32)c)
        {
            for (const char* p = found->mLatin; *p; ++p)
            {
                appendLatin(out, (llwchar)*p);
            }
            return CharKind::Latin;
        }

        if (isSeparator(c))
        {
            return CharKind::Separator;
        }

        out += c;
        return CharKind::Other;
    }

    // Splits text into folded words. A word ends at a separator and where
    // plain letters meet letters of another script, so a decoration like
    // "亗Wren亗" still gives the word "wren".
    std::vector<LLWString> foldWords(std::string_view text, Digits digits)
    {
        std::vector<LLWString> words;
        LLWString word;
        CharKind word_kind = CharKind::Separator;
        for (llwchar c : utf8str_to_wstring(std::string(text)))
        {
            LLWString folded;
            CharKind kind = foldChar(c, digits, folded);
            if (kind == CharKind::Joiner)
            {
                continue;
            }
            if (kind != word_kind && !word.empty())
            {
                words.push_back(word);
                word.clear();
            }
            word_kind = kind;
            if (kind != CharKind::Separator)
            {
                word += folded;
            }
        }
        if (!word.empty())
        {
            words.push_back(word);
        }
        return words;
    }

    LLWString fold(std::string_view word)
    {
        auto words = foldWords(word, Digits::Separate);
        return words.empty() ? LLWString() : words.front();
    }

    void addNameWords(std::vector<LLWString>& keys, std::string_view name, Digits digits)
    {
        LLWString letters;   // a run of single letters, "K T" or "B e A"
        size_t run = 0;
        auto end_run = [&]()
        {
            if (run > 1)
            {
                keys.push_back(letters);
            }
            letters.clear();
            run = 0;
        };

        for (const auto& word : foldWords(name, digits))
        {
            keys.push_back(word);
            if (word.size() == 1)
            {
                letters += word;
                ++run;
            }
            else
            {
                end_run();
            }
        }
        end_run();
    }
}

namespace FSGreetingNames
{
    std::vector<LLWString> chatWords(std::string_view text)
    {
        return foldWords(text, Digits::Separate);
    }

    std::vector<LLWString> nameKeys(std::string_view display_name, std::string_view username)
    {
        static const LLWString resident = fold("resident");

        std::vector<LLWString> keys;
        for (std::string_view name : { display_name, username })
        {
            // "aMossyX12" is also read as "a Mossy X12".
            std::string camel_split;
            for (size_t i = 0; i < name.size(); ++i)
            {
                if (i > 0 && name[i] >= 'A' && name[i] <= 'Z' && name[i - 1] >= 'a' && name[i - 1] <= 'z')
                {
                    camel_split += ' ';
                }
                camel_split += name[i];
            }
            for (Digits digits : { Digits::AsLetters, Digits::Separate })
            {
                addNameWords(keys, name, digits);
                addNameWords(keys, camel_split, digits);
            }
        }
        std::erase(keys, resident);
        std::sort(keys.begin(), keys.end());
        keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
        return keys;
    }

    bool isWelcome(const std::vector<LLWString>& words)
    {
        static const LLWString welcome = fold("welcome");
        static const LLWString you = fold("you");
        static const LLWString are = fold("are");
        static const std::set<LLWString> thanks_reply = { fold("you're"), fold("your"), fold("ur"), fold("yer") };

        for (size_t i = 0; i < words.size(); ++i)
        {
            if (words[i] != welcome)
            {
                continue;
            }
            const bool after_youre = i >= 1 && thanks_reply.contains(words[i - 1]);
            const bool after_you_are = i >= 2 && words[i - 2] == you && words[i - 1] == are;
            if (!after_youre && !after_you_are)
            {
                return true;
            }
        }
        return false;
    }

    bool isFillerWord(const LLWString& word)
    {
        static const std::set<LLWString> filler = []()
        {
            std::set<LLWString> words;
            for (const char* w : { "welcome", "welcomes", "to", "in", "into", "and", "the", "a", "an", "i", "you", "all",
                                   "everyone", "everybody", "both", "back", "again", "here", "there", "our", "my", "at",
                                   "on", "of", "for", "too", "also", "so", "very", "much", "hi", "hey", "hello", "heya",
                                   "hiya", "oh", "aww", "awww", "omg", "yay", "good", "morning", "afternoon", "evening",
                                   "night", "miss", "mr", "mrs", "ms", "sir", "madam", "lady", "come", "in", "guys",
                                   "folks", "friends", "yall", "darling", "dear", "hun", "honey", "sweetie" })
            {
                words.insert(fold(w));
            }
            return words;
        }();
        return filler.contains(word);
    }
}
