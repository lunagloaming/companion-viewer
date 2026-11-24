#!/usr/bin/env python3
"""\
@file fs_greeting_lookalikes.py
@brief Generates indra/newview/fsgreetinglookalikes.inc: the plain Latin letters that other letters
       and styled characters look like, for matching names in greetings (fsgreetingnames.cpp).

$LicenseInfo:firstyear=2026&license=viewerlgpl$
Phoenix Firestorm Viewer Source Code
Copyright (C) 2026 Luna Witch

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
http://www.firestormviewer.org
$/LicenseInfo$

Usage:
    fs_greeting_lookalikes.py confusables.txt > indra/newview/fsgreetinglookalikes.inc

confusables.txt is Unicode's list of characters that look alike (UTS #39), from
https://www.unicode.org/Public/security/<version>/confusables.txt, for the Unicode version of
the Python running this script (python3 -c "import unicodedata; print(unicodedata.unidata_version)").

Each character in the blocks below is mapped, by the first rule that gives plain letters:
  1. OVERRIDES, for decorations that fancy text generators use and Unicode does not count as
     lookalikes (Cyrillic и for N, CJK 尺 for R), or where the rules below choose wrongly.
  2. Its name: LATIN ... LETTER X, LATIN LETTER SMALL CAPITAL X, MODIFIER LETTER SMALL X,
     MATHEMATICAL BOLD CAPITAL X, CIRCLED LATIN SMALL LETTER X, REGIONAL INDICATOR SYMBOL LETTER X...
  3. Compatibility decomposition (NFKD) without accents: fullwidth, ligatures, accented letters.
  4. confusables.txt, after the decomposition: Greek, Cyrillic, Armenian, Cherokee... lookalikes.
Uppercase and lowercase map to the same lowercase letters; comparisons ignore case.
"""

import re
import sys
import unicodedata

# Blocks whose letters are used to decorate names: Latin, Greek, Cyrillic and other alphabets with
# letters shaped like Latin ones, styled and enclosed letters. Scripts written as whole words
# (Arabic, Hebrew, CJK, Thai...) are left alone, apart from OVERRIDES.
BLOCKS = [
    (0x0080, 0x024F),   # Latin-1 Supplement, Latin Extended-A and -B
    (0x0250, 0x02AF),   # IPA Extensions
    (0x02B0, 0x02FF),   # Spacing Modifier Letters
    (0x0370, 0x03FF),   # Greek and Coptic
    (0x0400, 0x052F),   # Cyrillic, Cyrillic Supplement
    (0x0530, 0x058F),   # Armenian
    (0x13A0, 0x13FF),   # Cherokee
    (0x1400, 0x167F),   # Unified Canadian Aboriginal Syllabics
    (0x16A0, 0x16FF),   # Runic
    (0x1C80, 0x1C8F),   # Cyrillic Extended-C
    (0x1D00, 0x1DBF),   # Phonetic Extensions and Supplement
    (0x1E00, 0x1EFF),   # Latin Extended Additional
    (0x1F00, 0x1FFF),   # Greek Extended
    (0x2070, 0x209F),   # Superscripts and Subscripts
    (0x2100, 0x214F),   # Letterlike Symbols
    (0x2460, 0x24FF),   # Enclosed Alphanumerics
    (0x2C60, 0x2C7F),   # Latin Extended-C
    (0x2C80, 0x2CFF),   # Coptic
    (0xA640, 0xA69F),   # Cyrillic Extended-B
    (0xA720, 0xA7FF),   # Latin Extended-D
    (0xAB30, 0xAB6F),   # Latin Extended-E
    (0xAB70, 0xABBF),   # Cherokee Supplement
    (0xFB00, 0xFB06),   # Latin ligatures
    (0xFF00, 0xFFEF),   # Halfwidth and Fullwidth Forms
    (0x10780, 0x107BF), # Latin Extended-F
    (0x1D400, 0x1D7FF), # Mathematical Alphanumeric Symbols
    (0x1DF00, 0x1DFFF), # Latin Extended-G
    (0x1F100, 0x1F1FF), # Enclosed Alphanumeric Supplement, regional indicators
]

# Decorations Unicode doesn't list as lookalikes, and corrections. Lowercase Greek and Cyrillic letters
# are read by their shape (η is n, и is n), as fancy text generators use them; their capitals by theirs
# (Η is h, И is n).
OVERRIDES = {
    # Greek: lowercase shapes, and capitals that look like Latin capitals or are used as them
    'α': 'a', 'β': 'b', 'γ': 'y', 'δ': 'd', 'ε': 'e', 'ζ': 'z', 'η': 'n', 'θ': 'o', 'ι': 'i', 'κ': 'k',
    'λ': 'a', 'μ': 'u', 'ν': 'v', 'ξ': 'e', 'π': 'n', 'ρ': 'p', 'ς': 's', 'σ': 'o', 'τ': 't', 'υ': 'u',
    'φ': 'o', 'χ': 'x', 'ψ': 'w', 'ω': 'w', 'ϑ': 'o', 'ϐ': 'b', 'ϰ': 'k', 'ϱ': 'p', 'ϲ': 'c', 'ϳ': 'j',
    'Γ': 'r', 'Δ': 'a', 'Θ': 'o', 'Λ': 'a', 'Ξ': 'e', 'Π': 'n', 'Σ': 'e', 'Φ': 'o', 'Ψ': 'w', 'Ω': 'o',
    'Ϲ': 'c', 'Ͻ': 'c', 'Ͼ': 'c', 'Ͽ': 'c', 'ϻ': 'm', 'Ϻ': 'm',
    # Cyrillic: lowercase shapes, and capitals used for the Latin letters they resemble
    'а': 'a', 'б': 'b', 'в': 'b', 'г': 'r', 'д': 'd', 'е': 'e', 'ж': 'x', 'з': 'e', 'и': 'n', 'й': 'n',
    'к': 'k', 'л': 'n', 'м': 'm', 'н': 'h', 'о': 'o', 'п': 'n', 'р': 'p', 'с': 'c', 'т': 't', 'у': 'y',
    'ф': 'o', 'х': 'x', 'ц': 'u', 'ч': 'y', 'ш': 'w', 'щ': 'w', 'ъ': 'b', 'ы': 'bi', 'ь': 'b', 'э': 'e',
    'ю': 'io', 'я': 'r', 'ѐ': 'e', 'ё': 'e', 'ђ': 'h', 'ѓ': 'r', 'є': 'e', 'ѕ': 's', 'і': 'i', 'ї': 'i',
    'ј': 'j', 'ћ': 'h', 'ќ': 'k', 'ѝ': 'n', 'ў': 'y', 'џ': 'u',
    'Б': 'b', 'Г': 'r', 'Д': 'd', 'Ж': 'x', 'З': 'e', 'И': 'n', 'Й': 'n', 'Л': 'n', 'П': 'n', 'Ф': 'o',
    'Ц': 'u', 'Ч': 'y', 'Ш': 'w', 'Щ': 'w', 'Ъ': 'b', 'Ы': 'bi', 'Ь': 'b', 'Э': 'e', 'Ю': 'io', 'Я': 'r',
    'Ѓ': 'r', 'Є': 'e', 'Ѝ': 'n', 'Ў': 'y', 'Џ': 'u', 'Ђ': 'h', 'Ћ': 'h',
    # Latin letters whose names don't give their shape
    'ı': 'i', 'ȷ': 'j', 'ſ': 's', 'ß': 'ss', 'ẞ': 'ss', 'Þ': 'p', 'þ': 'p', 'ð': 'd', 'Ð': 'd', 'ŋ': 'n',
    'Ŋ': 'n', 'ɐ': 'a', 'ɑ': 'a', 'ɒ': 'a', 'ɔ': 'o', 'Ɔ': 'o', 'ə': 'e', 'Ə': 'e', 'ɛ': 'e', 'Ɛ': 'e',
    'ɣ': 'y', 'ɩ': 'i', 'ɯ': 'm', 'ʊ': 'u', 'ʋ': 'v', 'ʌ': 'a', 'ʍ': 'm', 'ʎ': 'y', 'ʃ': 's', 'ʒ': 'z',
    'Ʒ': 'z', 'Ʀ': 'r', 'ƨ': 's', 'Ƨ': 's', 'Ʊ': 'u', 'ǝ': 'e', 'Ǝ': 'e', 'ɹ': 'r', 'ɾ': 'r', 'ʇ': 't',
    'ʞ': 'k', 'ɥ': 'h', 'Ɐ': 'a', 'Ʌ': 'a', 'Ɯ': 'm', 'Ɣ': 'y', 'ꞵ': 'b', 'Ꞵ': 'b',
    'ꭓ': 'x', 'ɸ': 'o', 'ʉ': 'u', 'ɤ': 'y', 'Ɤ': 'y', 'ƛ': 'a', 'Ƛ': 'a', 'ƿ': 'p', 'Ƿ': 'p', 'ℇ': 'e',
    # Small capital sets use these for F and Q
    'ғ': 'f', 'Ғ': 'f', 'ǫ': 'q',
    # Cyrillic and Greek letters with no Latin lookalike in Unicode's data
    'ә': 'e', 'Ә': 'e', 'һ': 'h', 'Һ': 'h', 'ѡ': 'w', 'Ѡ': 'w', 'ѧ': 'a', 'Ѧ': 'a', 'ѱ': 'w', 'Ѱ': 'w',
    'ҁ': 'c', 'Ҁ': 'c', 'ꙩ': 'o', 'Ꙩ': 'o', 'ϙ': 'q', 'Ϙ': 'q', 'ϝ': 'f', 'ϸ': 'p', 'Ϸ': 'p',
    'Ӻ': 'f', 'ӻ': 'f', 'Ҡ': 'k', 'ҡ': 'k', 'Ӡ': 'z', 'ӡ': 'z', 'Ѻ': 'o', 'ѻ': 'o', 'Ѿ': 'w', 'ѿ': 'w',
    'Ӌ': 'y', 'ӌ': 'y', 'Ҥ': 'h', 'ҥ': 'h', 'Ҽ': 'e', 'Ҿ': 'e', 'ͻ': 'c', 'ͼ': 'c', 'ͽ': 'c', 'Ʃ': 'e',
    # Reversed and turned letters used in mirrored and upside-down names, read as the letter they turn
    'ɘ': 'e', 'ᴎ': 'n', 'ᴙ': 'r', 'ᴚ': 'r', 'ʁ': 'r', 'ʶ': 'r', 'ᴉ': 'i', 'Ɥ': 'h', 'Ʞ': 'k', 'Ʇ': 't',
    'Ƞ': 'n', 'Ⱶ': 'h', 'ⱶ': 'h', 'Ɡ': 'g', 'ꝷ': 't',
    # Armenian and runic letters used in decorated names
    'Ա': 'w', 'Զ': 'q', 'Հ': 'h', 'Ո': 'n', 'Ռ': 'n', 'Ց': 'g', 'ᚠ': 'f', 'ᚱ': 'r', 'ᚺ': 'h', 'ᛏ': 't',
    'ᛒ': 'b', 'ᛚ': 'l', 'ᛟ': 'o',
    # Other scripts and symbols fancy text generators use as letters
    'ภ': 'n', 'เ': 'i', 'ץ': 'y', 'ひ': 'u', '丂': 's', '刀': 'n', '尺': 'r', 'ﾑ': 'a', '乃': 'b', '匚': 'c',
    '乇': 'e', '千': 'f', '卄': 'h', '工': 'i', 'ﾌ': 'j', '长': 'k', '爪': 'm', '卩': 'p', '乙': 'z',
    'ㄖ': 'o', 'ㄒ': 't', 'ㄩ': 'u', 'ㄚ': 'y', '乂': 'x', '山': 'w', 'ᗩ': 'a', 'ᗷ': 'b', 'ᑕ': 'c', 'ᗪ': 'd',
    'ᗴ': 'e', 'ᖴ': 'f', 'ᕼ': 'h', 'ᒍ': 'j', 'ᒪ': 'l', 'ᗰ': 'm', 'ᑎ': 'n', 'ᑭ': 'p', 'ᑫ': 'q', 'ᖇ': 'r',
    'ᔕ': 's', 'ᑌ': 'u', 'ᐯ': 'v', 'ᗯ': 'w', '᙭': 'x', 'ᘔ': 'z', 'ᕮ': 'e', 'ᖻ': 'y', 'ᘜ': 'g',
    # The fancy text alphabet built from Thai and other letters: ค๒ς๔єŦﻮђเןкɭ๓ภ๏קợгรՇยשฬאץչ
    'ค': 'a', '๒': 'b', '๔': 'd', 'Ŧ': 't', 'ﻮ': 'g', 'ן': 'i', 'ɭ': 'l', '๓': 'm', '๏': 'o', 'ק': 'p',
    'ợ': 'q', 'ร': 's', 'Շ': 't', 'ย': 'u', 'ש': 'v', 'ฬ': 'w', 'א': 'x', 'չ': 'z',
    # The one built from Japanese: ﾑ乃ᄃり乇ｷムんﾉﾌズﾚﾶ刀のｱゐ尺丂ｲひ√Wﾒﾘ乙
    'ᄃ': 'c', 'り': 'd', 'ｷ': 'f', 'ム': 'g', 'ん': 'h', 'ﾉ': 'i', 'ズ': 'k', 'ﾚ': 'l', 'ﾶ': 'm', 'の': 'o',
    'ｱ': 'p', 'ゐ': 'q', 'ｲ': 't', 'ﾒ': 'x', 'ﾘ': 'y',
}

LETTERS = re.compile(r'^[a-z]{1,3}$')

NAME_PATTERNS = [
    # LATIN CAPITAL LETTER A WITH GRAVE, CIRCLED LATIN SMALL LETTER A, LATIN SMALL LETTER AE...
    re.compile(r'^(?:[A-Z-]+ )*LATIN (?:SMALL |CAPITAL )?LETTER (?:SMALL CAPITAL |INSULAR )?([A-Z]{1,2})(?: WITH .*)?$'),
    # MODIFIER LETTER SMALL A, LATIN SUBSCRIPT SMALL LETTER A, SUPERSCRIPT LATIN SMALL LETTER I
    re.compile(r'^MODIFIER LETTER (?:SMALL |CAPITAL )(?:CAPITAL )?([A-Z])(?: WITH .*)?$'),
    re.compile(r'^(?:LATIN SUBSCRIPT|SUPERSCRIPT LATIN) (?:SMALL |CAPITAL )LETTER ([A-Z])$'),
    # MATHEMATICAL BOLD ITALIC CAPITAL A, DOUBLE-STRUCK CAPITAL N, SCRIPT SMALL L, BLACK-LETTER CAPITAL Z
    re.compile(r'^(?:MATHEMATICAL|SCRIPT|BLACK-LETTER|DOUBLE-STRUCK|ITALIC)(?: [A-Z-]+)* (?:CAPITAL|SMALL) ([A-Z])$'),
    re.compile(r'^REGIONAL INDICATOR SYMBOL LETTER ([A-Z])$'),
]
# Name words that mean the letter doesn't look like the plain one
NOT_THE_SHAPE = re.compile(r'\b(TURNED|REVERSED|INVERTED|SIDEWAYS|OPEN|CLOSED|BARRED|HALF|TAILLESS|BROKEN|STRETCHED|LONG|REVERSED-SCHWA|ESH|EZH)\b'
                           r'|WITH SMALL LETTER')  # ǅ: both letters, from the decomposition
SCRIPTS = ('LATIN', 'GREEK', 'CYRILLIC')
NAME_NOISE = {'MODIFIER', 'LETTER', 'SMALL', 'CAPITAL', 'SUBSCRIPT', 'SUPERSCRIPT'} | set(SCRIPTS)


def strip_accents(text):
    return ''.join(c for c in unicodedata.normalize('NFKD', text) if not unicodedata.combining(c))


def read_confusables(path):
    confusables = {}
    with open(path, encoding='utf-8-sig') as f:
        for line in f:
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            source, target = [field.strip() for field in line.split(';')[:2]]
            if ' ' in source:
                continue
            confusables[chr(int(source, 16))] = ''.join(chr(int(code, 16)) for code in target.split())
    return confusables


def by_name(c):
    name = unicodedata.name(c, '')
    if NOT_THE_SHAPE.search(name):
        return None
    for pattern in NAME_PATTERNS:
        found = pattern.match(name)
        if found:
            return found.group(1).lower()
    return None


def base_letters(c):
    """Letters this one is a variant of, by name: CYRILLIC SMALL LETTER KA WITH DESCENDER is CYRILLIC SMALL
    LETTER KA, MODIFIER LETTER SMALL BETA is GREEK SMALL LETTER BETA, LATIN SMALL LETTER OMEGA is GREEK..."""
    name = unicodedata.name(c, '')
    if not name or NOT_THE_SHAPE.search(name):
        return
    words = re.sub(r' WITH .*$', '', name).split()
    case = 'CAPITAL' if 'CAPITAL' in words else 'SMALL'
    scripts = [w for w in words if w in SCRIPTS] or list(SCRIPTS)
    rest = ' '.join(w for w in words if w not in NAME_NOISE)
    if not rest:
        return
    # Latin letters named after Greek ones (LATIN SMALL LETTER OMEGA) may become those; Greek and Cyrillic
    # letters never become Latin ones by name (CYRILLIC CAPITAL LETTER U looks like Y, not U)
    others = [s for s in SCRIPTS if s not in scripts and s != 'LATIN'] if 'LATIN' in scripts else []
    for script in scripts + others:
        try:
            base = unicodedata.lookup('%s %s LETTER %s' % (script, case, rest))
        except KeyError:
            continue
        if base != c:
            yield base


def latin_for(c, confusables, depth=0):
    if c in OVERRIDES:
        return OVERRIDES[c]
    letters = by_name(c)
    if letters:
        return letters
    plain = strip_accents(c).lower()
    if LETTERS.match(plain):
        return plain
    if depth < 3:
        # The letter without its accents (έ is ε), or the letter it is a variant of
        bases = [strip_accents(c)] if len(strip_accents(c)) == 1 and strip_accents(c) != c else []
        for base in bases + list(base_letters(c)):
            letters = latin_for(base, confusables, depth + 1)
            if letters:
                return letters
    looks_like = strip_accents(''.join(confusables.get(ch, ch) for ch in strip_accents(c))).lower()
    if LETTERS.match(looks_like):
        return looks_like
    return None


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.split('Usage:')[1].split('Each character')[0])
    confusables = read_confusables(sys.argv[1])

    candidates = sorted({code for first, last in BLOCKS for code in range(first, last + 1)}
                        | {ord(c) for c in OVERRIDES})
    entries = []
    for code in candidates:
        c = chr(code)
        if code < 0x80 or unicodedata.category(c)[0] not in 'LS' and c not in OVERRIDES:
            continue
        latin = latin_for(c, confusables)
        if latin:
            entries.append((code, latin))

    print('// Generated by scripts/code_tools/fs_greeting_lookalikes.py from Unicode %s data; do not edit.'
          % unicodedata.unidata_version)
    print('// Code point and the plain lowercase Latin letters it looks like, sorted by code point.')
    for i in range(0, len(entries), 6):
        print(' '.join('{0x%04X, "%s"},' % entry for entry in entries[i:i + 6]))


if __name__ == '__main__':
    main()
