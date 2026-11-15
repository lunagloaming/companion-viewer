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

    // Letters from Latin, Greek, Cyrillic, Armenian, Cherokee, Canadian
    // syllabics, runes and a few other scripts, small capitals and letterlike
    // symbols, with the plain letters they look like. Sorted by code point. Letters with accents are listed
    // without them; uppercase letters map to lowercase.
    constexpr Lookalike LOOKALIKES[] =
    {
    {0x00C0, "a"}, {0x00C1, "a"}, {0x00C2, "a"}, {0x00C3, "a"}, {0x00C4, "a"}, {0x00C5, "a"},
    {0x00C6, "ae"}, {0x00C7, "c"}, {0x00C8, "e"}, {0x00C9, "e"}, {0x00CA, "e"}, {0x00CB, "e"},
    {0x00CC, "i"}, {0x00CD, "i"}, {0x00CE, "i"}, {0x00CF, "i"}, {0x00D0, "d"}, {0x00D1, "n"},
    {0x00D2, "o"}, {0x00D3, "o"}, {0x00D4, "o"}, {0x00D5, "o"}, {0x00D6, "o"}, {0x00D8, "o"},
    {0x00D9, "u"}, {0x00DA, "u"}, {0x00DB, "u"}, {0x00DC, "u"}, {0x00DD, "y"}, {0x00DE, "p"},
    {0x00DF, "ss"}, {0x00E0, "a"}, {0x00E1, "a"}, {0x00E2, "a"}, {0x00E3, "a"}, {0x00E4, "a"},
    {0x00E5, "a"}, {0x00E6, "ae"}, {0x00E7, "c"}, {0x00E8, "e"}, {0x00E9, "e"}, {0x00EA, "e"},
    {0x00EB, "e"}, {0x00EC, "i"}, {0x00ED, "i"}, {0x00EE, "i"}, {0x00EF, "i"}, {0x00F0, "d"},
    {0x00F1, "n"}, {0x00F2, "o"}, {0x00F3, "o"}, {0x00F4, "o"}, {0x00F5, "o"}, {0x00F6, "o"},
    {0x00F8, "o"}, {0x00F9, "u"}, {0x00FA, "u"}, {0x00FB, "u"}, {0x00FC, "u"}, {0x00FD, "y"},
    {0x00FE, "p"}, {0x00FF, "y"}, {0x0100, "a"}, {0x0101, "a"}, {0x0102, "a"}, {0x0103, "a"},
    {0x0104, "a"}, {0x0105, "a"}, {0x0106, "c"}, {0x0107, "c"}, {0x0108, "c"}, {0x0109, "c"},
    {0x010A, "c"}, {0x010B, "c"}, {0x010C, "c"}, {0x010D, "c"}, {0x010E, "d"}, {0x010F, "d"},
    {0x0110, "d"}, {0x0111, "d"}, {0x0112, "e"}, {0x0113, "e"}, {0x0114, "e"}, {0x0115, "e"},
    {0x0116, "e"}, {0x0117, "e"}, {0x0118, "e"}, {0x0119, "e"}, {0x011A, "e"}, {0x011B, "e"},
    {0x011C, "g"}, {0x011D, "g"}, {0x011E, "g"}, {0x011F, "g"}, {0x0120, "g"}, {0x0121, "g"},
    {0x0122, "g"}, {0x0123, "g"}, {0x0124, "h"}, {0x0125, "h"}, {0x0126, "h"}, {0x0127, "h"},
    {0x0128, "i"}, {0x0129, "i"}, {0x012A, "i"}, {0x012B, "i"}, {0x012C, "i"}, {0x012D, "i"},
    {0x012E, "i"}, {0x012F, "i"}, {0x0130, "i"}, {0x0131, "i"}, {0x0132, "ij"}, {0x0133, "ij"},
    {0x0134, "j"}, {0x0135, "j"}, {0x0136, "k"}, {0x0137, "k"}, {0x0139, "l"}, {0x013A, "l"},
    {0x013B, "l"}, {0x013C, "l"}, {0x013D, "l"}, {0x013E, "l"}, {0x0141, "l"}, {0x0142, "l"},
    {0x0143, "n"}, {0x0144, "n"}, {0x0145, "n"}, {0x0146, "n"}, {0x0147, "n"}, {0x0148, "n"},
    {0x014A, "n"}, {0x014B, "n"}, {0x014C, "o"}, {0x014D, "o"}, {0x014E, "o"}, {0x014F, "o"},
    {0x0150, "o"}, {0x0151, "o"}, {0x0152, "oe"}, {0x0153, "oe"}, {0x0154, "r"}, {0x0155, "r"},
    {0x0156, "r"}, {0x0157, "r"}, {0x0158, "r"}, {0x0159, "r"}, {0x015A, "s"}, {0x015B, "s"},
    {0x015C, "s"}, {0x015D, "s"}, {0x015E, "s"}, {0x015F, "s"}, {0x0160, "s"}, {0x0161, "s"},
    {0x0162, "t"}, {0x0163, "t"}, {0x0164, "t"}, {0x0165, "t"}, {0x0166, "t"}, {0x0167, "t"},
    {0x0168, "u"}, {0x0169, "u"}, {0x016A, "u"}, {0x016B, "u"}, {0x016C, "u"}, {0x016D, "u"},
    {0x016E, "u"}, {0x016F, "u"}, {0x0170, "u"}, {0x0171, "u"}, {0x0172, "u"}, {0x0173, "u"},
    {0x0174, "w"}, {0x0175, "w"}, {0x0176, "y"}, {0x0177, "y"}, {0x0178, "y"}, {0x0179, "z"},
    {0x017A, "z"}, {0x017B, "z"}, {0x017C, "z"}, {0x017D, "z"}, {0x017E, "z"}, {0x017F, "s"},
    {0x0180, "b"}, {0x0181, "b"}, {0x0182, "b"}, {0x0183, "b"}, {0x0184, "b"}, {0x0185, "b"},
    {0x0186, "o"}, {0x0187, "c"}, {0x0188, "c"}, {0x0189, "d"}, {0x018A, "d"}, {0x018B, "d"},
    {0x018C, "d"}, {0x018E, "e"}, {0x018F, "e"}, {0x0190, "e"}, {0x0191, "f"}, {0x0192, "f"},
    {0x0193, "g"}, {0x0194, "y"}, {0x0196, "i"}, {0x0197, "i"}, {0x0198, "k"}, {0x0199, "k"},
    {0x019A, "l"}, {0x019B, "a"}, {0x019C, "m"}, {0x019D, "n"}, {0x019E, "n"}, {0x019F, "o"},
    {0x01A0, "o"}, {0x01A1, "o"}, {0x01A4, "p"}, {0x01A5, "p"}, {0x01A6, "r"}, {0x01A7, "s"},
    {0x01A8, "s"}, {0x01A9, "s"}, {0x01AB, "t"}, {0x01AC, "t"}, {0x01AD, "t"}, {0x01AE, "t"},
    {0x01AF, "u"}, {0x01B0, "u"}, {0x01B1, "u"}, {0x01B2, "v"}, {0x01B3, "y"}, {0x01B4, "y"},
    {0x01B5, "z"}, {0x01B6, "z"}, {0x01B7, "z"}, {0x01BC, "s"}, {0x01BD, "s"}, {0x01BF, "p"},
    {0x01C0, "i"}, {0x01C4, "dz"}, {0x01C5, "dz"}, {0x01C6, "dz"}, {0x01C7, "lj"}, {0x01C8, "lj"},
    {0x01C9, "lj"}, {0x01CA, "nj"}, {0x01CB, "nj"}, {0x01CC, "nj"}, {0x01CD, "a"}, {0x01CE, "a"},
    {0x01CF, "i"}, {0x01D0, "i"}, {0x01D1, "o"}, {0x01D2, "o"}, {0x01D3, "u"}, {0x01D4, "u"},
    {0x01D5, "u"}, {0x01D6, "u"}, {0x01D7, "u"}, {0x01D8, "u"}, {0x01D9, "u"}, {0x01DA, "u"},
    {0x01DB, "u"}, {0x01DC, "u"}, {0x01DD, "e"}, {0x01DE, "a"}, {0x01DF, "a"}, {0x01E0, "a"},
    {0x01E1, "a"}, {0x01E2, "ae"}, {0x01E3, "ae"}, {0x01E4, "g"}, {0x01E5, "g"}, {0x01E6, "g"},
    {0x01E7, "g"}, {0x01E8, "k"}, {0x01E9, "k"}, {0x01EA, "o"}, {0x01EB, "o"}, {0x01EC, "o"},
    {0x01ED, "o"}, {0x01EE, "z"}, {0x01EF, "z"}, {0x01F0, "j"}, {0x01F1, "dz"}, {0x01F2, "dz"},
    {0x01F3, "dz"}, {0x01F4, "g"}, {0x01F5, "g"}, {0x01F7, "p"}, {0x01F8, "n"}, {0x01F9, "n"},
    {0x01FA, "a"}, {0x01FB, "a"}, {0x01FC, "ae"}, {0x01FD, "ae"}, {0x01FE, "o"}, {0x01FF, "o"},
    {0x0200, "a"}, {0x0201, "a"}, {0x0202, "a"}, {0x0203, "a"}, {0x0204, "e"}, {0x0205, "e"},
    {0x0206, "e"}, {0x0207, "e"}, {0x0208, "i"}, {0x0209, "i"}, {0x020A, "i"}, {0x020B, "i"},
    {0x020C, "o"}, {0x020D, "o"}, {0x020E, "o"}, {0x020F, "o"}, {0x0210, "r"}, {0x0211, "r"},
    {0x0212, "r"}, {0x0213, "r"}, {0x0214, "u"}, {0x0215, "u"}, {0x0216, "u"}, {0x0217, "u"},
    {0x0218, "s"}, {0x0219, "s"}, {0x021A, "t"}, {0x021B, "t"}, {0x021C, "z"}, {0x021D, "z"},
    {0x021E, "h"}, {0x021F, "h"}, {0x0220, "n"}, {0x0221, "d"}, {0x0224, "z"}, {0x0225, "z"},
    {0x0226, "a"}, {0x0227, "a"}, {0x0228, "e"}, {0x0229, "e"}, {0x022A, "o"}, {0x022B, "o"},
    {0x022C, "o"}, {0x022D, "o"}, {0x022E, "o"}, {0x022F, "o"}, {0x0230, "o"}, {0x0231, "o"},
    {0x0232, "y"}, {0x0233, "y"}, {0x0234, "l"}, {0x0235, "n"}, {0x0236, "t"}, {0x0237, "j"},
    {0x023A, "a"}, {0x023B, "c"}, {0x023C, "c"}, {0x023D, "l"}, {0x023E, "t"}, {0x023F, "s"},
    {0x0240, "z"}, {0x0243, "b"}, {0x0244, "u"}, {0x0245, "a"}, {0x0246, "e"}, {0x0247, "e"},
    {0x0248, "j"}, {0x0249, "j"}, {0x024A, "q"}, {0x024B, "q"}, {0x024C, "r"}, {0x024D, "r"},
    {0x024E, "y"}, {0x024F, "y"}, {0x0250, "a"}, {0x0251, "a"}, {0x0252, "a"}, {0x0253, "b"},
    {0x0254, "o"}, {0x0255, "c"}, {0x0256, "d"}, {0x0257, "d"}, {0x0258, "e"}, {0x0259, "e"},
    {0x025A, "e"}, {0x025B, "e"}, {0x025C, "e"}, {0x025D, "e"}, {0x025F, "j"}, {0x0260, "g"},
    {0x0261, "g"}, {0x0262, "g"}, {0x0263, "y"}, {0x0264, "y"}, {0x0265, "h"}, {0x0266, "h"},
    {0x0267, "h"}, {0x0268, "i"}, {0x0269, "i"}, {0x026A, "i"}, {0x026B, "l"}, {0x026C, "l"},
    {0x026D, "l"}, {0x026E, "l"}, {0x026F, "m"}, {0x0270, "w"}, {0x0271, "m"}, {0x0272, "n"},
    {0x0273, "n"}, {0x0274, "n"}, {0x0275, "o"}, {0x0276, "oe"}, {0x0277, "w"}, {0x0278, "o"},
    {0x0279, "r"}, {0x027A, "r"}, {0x027B, "r"}, {0x027C, "r"}, {0x027D, "r"}, {0x027E, "r"},
    {0x027F, "r"}, {0x0280, "r"}, {0x0281, "r"}, {0x0282, "s"}, {0x0283, "s"}, {0x0284, "j"},
    {0x0287, "t"}, {0x0288, "t"}, {0x0289, "u"}, {0x028A, "u"}, {0x028B, "v"}, {0x028C, "a"},
    {0x028D, "m"}, {0x028E, "y"}, {0x028F, "y"}, {0x0290, "z"}, {0x0291, "z"}, {0x0292, "z"},
    {0x0293, "z"}, {0x0299, "b"}, {0x029A, "e"}, {0x029B, "g"}, {0x029C, "h"}, {0x029D, "j"},
    {0x029E, "k"}, {0x029F, "l"}, {0x02A0, "q"}, {0x02A4, "dz"}, {0x02A5, "dz"}, {0x02A6, "ts"},
    {0x02A7, "ts"}, {0x02B0, "h"}, {0x02B1, "h"}, {0x02B2, "j"}, {0x02B3, "r"}, {0x02B4, "r"},
    {0x02B5, "r"}, {0x02B6, "r"}, {0x02B7, "w"}, {0x02B8, "y"}, {0x02E0, "y"}, {0x02E1, "l"},
    {0x02E2, "s"}, {0x02E3, "x"}, {0x0372, "t"}, {0x0373, "t"}, {0x037B, "c"}, {0x037C, "c"},
    {0x037D, "c"}, {0x037F, "j"}, {0x0386, "a"}, {0x0388, "e"}, {0x0389, "n"}, {0x038A, "i"},
    {0x038C, "o"}, {0x038E, "u"}, {0x038F, "w"}, {0x0390, "i"}, {0x0391, "a"}, {0x0392, "b"},
    {0x0393, "y"}, {0x0394, "d"}, {0x0395, "e"}, {0x0396, "z"}, {0x0397, "n"}, {0x0398, "o"},
    {0x0399, "i"}, {0x039A, "k"}, {0x039B, "a"}, {0x039C, "u"}, {0x039D, "v"}, {0x039E, "e"},
    {0x039F, "o"}, {0x03A0, "n"}, {0x03A1, "p"}, {0x03A3, "e"}, {0x03A4, "t"}, {0x03A5, "u"},
    {0x03A6, "o"}, {0x03A7, "x"}, {0x03A8, "w"}, {0x03A9, "w"}, {0x03AA, "i"}, {0x03AB, "u"},
    {0x03AC, "a"}, {0x03AD, "e"}, {0x03AE, "n"}, {0x03AF, "i"}, {0x03B0, "u"}, {0x03B1, "a"},
    {0x03B2, "b"}, {0x03B3, "y"}, {0x03B4, "d"}, {0x03B5, "e"}, {0x03B6, "z"}, {0x03B7, "n"},
    {0x03B8, "o"}, {0x03B9, "i"}, {0x03BA, "k"}, {0x03BB, "a"}, {0x03BC, "u"}, {0x03BD, "v"},
    {0x03BE, "e"}, {0x03BF, "o"}, {0x03C0, "n"}, {0x03C1, "p"}, {0x03C2, "s"}, {0x03C3, "o"},
    {0x03C4, "t"}, {0x03C5, "u"}, {0x03C6, "o"}, {0x03C7, "x"}, {0x03C8, "w"}, {0x03C9, "w"},
    {0x03CA, "i"}, {0x03CB, "u"}, {0x03CC, "o"}, {0x03CD, "u"}, {0x03CE, "w"}, {0x03D0, "b"},
    {0x03D1, "o"}, {0x03D2, "u"}, {0x03D3, "u"}, {0x03D4, "u"}, {0x03D5, "o"}, {0x03D6, "n"},
    {0x03D8, "q"}, {0x03D9, "q"}, {0x03DC, "f"}, {0x03DD, "f"}, {0x03EA, "x"}, {0x03EB, "x"},
    {0x03EE, "t"}, {0x03EF, "t"}, {0x03F0, "k"}, {0x03F1, "p"}, {0x03F2, "c"}, {0x03F3, "j"},
    {0x03F4, "o"}, {0x03F5, "e"}, {0x03F7, "p"}, {0x03F8, "p"}, {0x03F9, "c"}, {0x03FA, "m"},
    {0x03FB, "m"}, {0x03FC, "p"}, {0x03FD, "c"}, {0x03FE, "c"}, {0x03FF, "c"}, {0x0400, "e"},
    {0x0401, "e"}, {0x0402, "h"}, {0x0403, "r"}, {0x0404, "e"}, {0x0405, "s"}, {0x0406, "i"},
    {0x0407, "i"}, {0x0408, "j"}, {0x040B, "h"}, {0x040C, "k"}, {0x040D, "u"}, {0x040E, "y"},
    {0x040F, "u"}, {0x0410, "a"}, {0x0411, "b"}, {0x0412, "b"}, {0x0413, "r"}, {0x0414, "d"},
    {0x0415, "e"}, {0x0416, "x"}, {0x0418, "u"}, {0x0419, "u"}, {0x041A, "k"}, {0x041B, "n"},
    {0x041C, "m"}, {0x041D, "h"}, {0x041E, "o"}, {0x041F, "n"}, {0x0420, "p"}, {0x0421, "c"},
    {0x0422, "t"}, {0x0423, "y"}, {0x0424, "o"}, {0x0425, "x"}, {0x0426, "u"}, {0x0427, "y"},
    {0x0428, "w"}, {0x0429, "w"}, {0x042A, "b"}, {0x042B, "bi"}, {0x042C, "b"}, {0x042D, "e"},
    {0x042E, "io"}, {0x042F, "r"}, {0x0430, "a"}, {0x0431, "b"}, {0x0432, "b"}, {0x0433, "r"},
    {0x0434, "d"}, {0x0435, "e"}, {0x0436, "x"}, {0x0438, "u"}, {0x0439, "u"}, {0x043A, "k"},
    {0x043B, "n"}, {0x043C, "m"}, {0x043D, "h"}, {0x043E, "o"}, {0x043F, "n"}, {0x0440, "p"},
    {0x0441, "c"}, {0x0442, "t"}, {0x0443, "y"}, {0x0444, "o"}, {0x0445, "x"}, {0x0446, "u"},
    {0x0447, "y"}, {0x0448, "w"}, {0x0449, "w"}, {0x044A, "b"}, {0x044B, "bi"}, {0x044C, "b"},
    {0x044D, "e"}, {0x044E, "io"}, {0x044F, "r"}, {0x0450, "e"}, {0x0451, "e"}, {0x0452, "h"},
    {0x0453, "r"}, {0x0454, "e"}, {0x0455, "s"}, {0x0456, "i"}, {0x0457, "i"}, {0x0458, "j"},
    {0x045B, "h"}, {0x045C, "k"}, {0x045D, "u"}, {0x045E, "y"}, {0x045F, "u"}, {0x0460, "w"},
    {0x0461, "w"}, {0x0462, "b"}, {0x0463, "b"}, {0x0466, "a"}, {0x0467, "a"}, {0x0470, "w"},
    {0x0471, "w"}, {0x0472, "o"}, {0x0473, "o"}, {0x0474, "v"}, {0x0475, "v"}, {0x0476, "v"},
    {0x0477, "v"}, {0x047A, "o"}, {0x047B, "o"}, {0x047C, "w"}, {0x047D, "w"}, {0x047E, "w"},
    {0x047F, "w"}, {0x0480, "c"}, {0x0481, "c"}, {0x048A, "u"}, {0x048B, "u"}, {0x048C, "b"},
    {0x048D, "b"}, {0x048E, "p"}, {0x048F, "p"}, {0x0490, "r"}, {0x0491, "r"}, {0x0492, "f"},
    {0x0493, "f"}, {0x0496, "x"}, {0x0497, "x"}, {0x049A, "k"}, {0x049B, "k"}, {0x049C, "k"},
    {0x049D, "k"}, {0x049E, "k"}, {0x049F, "k"}, {0x04A0, "k"}, {0x04A1, "k"}, {0x04A2, "h"},
    {0x04A3, "h"}, {0x04A4, "h"}, {0x04A5, "h"}, {0x04AA, "c"}, {0x04AB, "c"}, {0x04AC, "t"},
    {0x04AD, "t"}, {0x04AE, "y"}, {0x04AF, "y"}, {0x04B0, "y"}, {0x04B1, "y"}, {0x04B2, "x"},
    {0x04B3, "x"}, {0x04B6, "y"}, {0x04B7, "y"}, {0x04B8, "y"}, {0x04B9, "y"}, {0x04BA, "h"},
    {0x04BB, "h"}, {0x04BC, "e"}, {0x04BD, "e"}, {0x04BE, "e"}, {0x04BF, "e"}, {0x04C0, "i"},
    {0x04C1, "x"}, {0x04C2, "x"}, {0x04C3, "k"}, {0x04C4, "k"}, {0x04C5, "n"}, {0x04C6, "n"},
    {0x04C7, "h"}, {0x04C8, "h"}, {0x04C9, "h"}, {0x04CA, "h"}, {0x04CB, "y"}, {0x04CC, "y"},
    {0x04CD, "m"}, {0x04CE, "m"}, {0x04CF, "i"}, {0x04D0, "a"}, {0x04D1, "a"}, {0x04D2, "a"},
    {0x04D3, "a"}, {0x04D4, "ae"}, {0x04D5, "ae"}, {0x04D6, "e"}, {0x04D7, "e"}, {0x04D8, "e"},
    {0x04D9, "e"}, {0x04DA, "e"}, {0x04DB, "e"}, {0x04DC, "x"}, {0x04DD, "x"}, {0x04E0, "z"},
    {0x04E1, "z"}, {0x04E2, "u"}, {0x04E3, "u"}, {0x04E4, "u"}, {0x04E5, "u"}, {0x04E6, "o"},
    {0x04E7, "o"}, {0x04E8, "o"}, {0x04E9, "o"}, {0x04EA, "o"}, {0x04EB, "o"}, {0x04EC, "e"},
    {0x04ED, "e"}, {0x04EE, "y"}, {0x04EF, "y"}, {0x04F0, "y"}, {0x04F1, "y"}, {0x04F2, "y"},
    {0x04F3, "y"}, {0x04F4, "y"}, {0x04F5, "y"}, {0x04F6, "r"}, {0x04F7, "r"}, {0x04F8, "bi"},
    {0x04F9, "bi"}, {0x04FA, "f"}, {0x04FB, "f"}, {0x04FC, "x"}, {0x04FD, "x"}, {0x04FE, "x"},
    {0x04FF, "x"}, {0x0531, "w"}, {0x0536, "q"}, {0x0540, "h"}, {0x0548, "n"}, {0x054C, "n"},
    {0x054D, "u"}, {0x0551, "g"}, {0x0555, "o"}, {0x0561, "w"}, {0x0566, "q"}, {0x0570, "h"},
    {0x0578, "n"}, {0x057C, "n"}, {0x057D, "u"}, {0x0581, "g"}, {0x0585, "o"}, {0x05E5, "y"},
    {0x0E20, "n"}, {0x0E40, "i"}, {0x13A0, "d"}, {0x13A1, "r"}, {0x13A2, "t"}, {0x13A5, "i"},
    {0x13A9, "y"}, {0x13AA, "a"}, {0x13AB, "j"}, {0x13AC, "e"}, {0x13B3, "w"}, {0x13B7, "m"},
    {0x13BB, "h"}, {0x13BE, "o"}, {0x13C0, "g"}, {0x13C3, "z"}, {0x13D9, "v"}, {0x13DA, "s"},
    {0x13DE, "l"}, {0x13DF, "c"}, {0x13E2, "p"}, {0x13E6, "k"}, {0x13F4, "b"}, {0x142F, "v"},
    {0x144C, "u"}, {0x144E, "n"}, {0x1455, "c"}, {0x146B, "q"}, {0x146D, "p"}, {0x148D, "j"},
    {0x14AA, "l"}, {0x1515, "s"}, {0x156E, "e"}, {0x157C, "h"}, {0x1587, "r"}, {0x15B4, "f"},
    {0x15BB, "y"}, {0x15E9, "a"}, {0x15EA, "d"}, {0x15EF, "w"}, {0x15F0, "m"}, {0x15F4, "e"},
    {0x15F7, "b"}, {0x1614, "z"}, {0x161C, "g"}, {0x166D, "x"}, {0x16A0, "f"}, {0x16B1, "r"},
    {0x16BA, "h"}, {0x16C1, "i"}, {0x16CF, "t"}, {0x16D2, "b"}, {0x16D5, "k"}, {0x16D6, "m"},
    {0x16DA, "l"}, {0x16DF, "o"}, {0x1D00, "a"}, {0x1D01, "ae"}, {0x1D03, "b"}, {0x1D04, "c"},
    {0x1D05, "d"}, {0x1D06, "d"}, {0x1D07, "e"}, {0x1D08, "e"}, {0x1D09, "i"}, {0x1D0A, "j"},
    {0x1D0B, "k"}, {0x1D0C, "l"}, {0x1D0D, "m"}, {0x1D0E, "n"}, {0x1D0F, "o"}, {0x1D10, "o"},
    {0x1D11, "o"}, {0x1D12, "o"}, {0x1D13, "o"}, {0x1D14, "oe"}, {0x1D18, "p"}, {0x1D19, "r"},
    {0x1D1A, "r"}, {0x1D1B, "t"}, {0x1D1C, "u"}, {0x1D1D, "u"}, {0x1D1E, "u"}, {0x1D1F, "m"},
    {0x1D20, "v"}, {0x1D21, "w"}, {0x1D22, "z"}, {0x1D23, "z"}, {0x1D26, "r"}, {0x1D27, "a"},
    {0x1D28, "n"}, {0x1D29, "p"}, {0x1D2A, "w"}, {0x1D2B, "n"}, {0x1D2C, "a"}, {0x1D2D, "ae"},
    {0x1D2E, "b"}, {0x1D30, "d"}, {0x1D31, "e"}, {0x1D32, "e"}, {0x1D33, "g"}, {0x1D34, "h"},
    {0x1D35, "i"}, {0x1D36, "j"}, {0x1D37, "k"}, {0x1D38, "l"}, {0x1D39, "m"}, {0x1D3A, "n"},
    {0x1D3C, "o"}, {0x1D3E, "p"}, {0x1D3F, "r"}, {0x1D40, "t"}, {0x1D41, "u"}, {0x1D42, "w"},
    {0x1D43, "a"}, {0x1D44, "a"}, {0x1D45, "a"}, {0x1D46, "ae"}, {0x1D47, "b"}, {0x1D48, "d"},
    {0x1D49, "e"}, {0x1D4A, "e"}, {0x1D4B, "e"}, {0x1D4C, "e"}, {0x1D4D, "g"}, {0x1D4E, "i"},
    {0x1D4F, "k"}, {0x1D50, "m"}, {0x1D51, "n"}, {0x1D52, "o"}, {0x1D53, "o"}, {0x1D54, "o"},
    {0x1D55, "u"}, {0x1D56, "p"}, {0x1D57, "t"}, {0x1D58, "u"}, {0x1D59, "u"}, {0x1D5A, "m"},
    {0x1D5B, "v"}, {0x1D5D, "b"}, {0x1D5E, "y"}, {0x1D5F, "d"}, {0x1D60, "o"}, {0x1D61, "x"},
    {0x1D62, "i"}, {0x1D63, "r"}, {0x1D64, "u"}, {0x1D65, "v"}, {0x1D66, "b"}, {0x1D67, "y"},
    {0x1D68, "p"}, {0x1D69, "o"}, {0x1D6A, "x"}, {0x1D6B, "ue"}, {0x1D6C, "b"}, {0x1D6D, "d"},
    {0x1D6E, "f"}, {0x1D6F, "m"}, {0x1D70, "n"}, {0x1D71, "p"}, {0x1D72, "r"}, {0x1D73, "r"},
    {0x1D74, "s"}, {0x1D75, "t"}, {0x1D76, "z"}, {0x1D77, "g"}, {0x1D78, "h"}, {0x1D79, "g"},
    {0x1D7B, "i"}, {0x1D7C, "i"}, {0x1D7D, "p"}, {0x1D7E, "u"}, {0x1D7F, "u"}, {0x1D9B, "a"},
    {0x1D9C, "c"}, {0x1D9D, "c"}, {0x1D9E, "d"}, {0x1D9F, "e"}, {0x1DA0, "f"}, {0x1DA1, "j"},
    {0x1DA2, "g"}, {0x1DA3, "h"}, {0x1DA4, "i"}, {0x1DA5, "i"}, {0x1DA6, "i"}, {0x1DA7, "i"},
    {0x1DA8, "j"}, {0x1DA9, "l"}, {0x1DAB, "l"}, {0x1DAC, "m"}, {0x1DAD, "w"}, {0x1DAE, "n"},
    {0x1DAF, "n"}, {0x1DB0, "n"}, {0x1DB1, "o"}, {0x1DB2, "o"}, {0x1DB3, "s"}, {0x1DB4, "s"},
    {0x1DB5, "t"}, {0x1DB6, "u"}, {0x1DB7, "u"}, {0x1DB8, "u"}, {0x1DB9, "v"}, {0x1DBA, "a"},
    {0x1DBB, "z"}, {0x1DBC, "z"}, {0x1DBD, "z"}, {0x1DBE, "z"}, {0x1DBF, "o"}, {0x1E00, "a"},
    {0x1E01, "a"}, {0x1E02, "b"}, {0x1E03, "b"}, {0x1E04, "b"}, {0x1E05, "b"}, {0x1E06, "b"},
    {0x1E07, "b"}, {0x1E08, "c"}, {0x1E09, "c"}, {0x1E0A, "d"}, {0x1E0B, "d"}, {0x1E0C, "d"},
    {0x1E0D, "d"}, {0x1E0E, "d"}, {0x1E0F, "d"}, {0x1E10, "d"}, {0x1E11, "d"}, {0x1E12, "d"},
    {0x1E13, "d"}, {0x1E14, "e"}, {0x1E15, "e"}, {0x1E16, "e"}, {0x1E17, "e"}, {0x1E18, "e"},
    {0x1E19, "e"}, {0x1E1A, "e"}, {0x1E1B, "e"}, {0x1E1C, "e"}, {0x1E1D, "e"}, {0x1E1E, "f"},
    {0x1E1F, "f"}, {0x1E20, "g"}, {0x1E21, "g"}, {0x1E22, "h"}, {0x1E23, "h"}, {0x1E24, "h"},
    {0x1E25, "h"}, {0x1E26, "h"}, {0x1E27, "h"}, {0x1E28, "h"}, {0x1E29, "h"}, {0x1E2A, "h"},
    {0x1E2B, "h"}, {0x1E2C, "i"}, {0x1E2D, "i"}, {0x1E2E, "i"}, {0x1E2F, "i"}, {0x1E30, "k"},
    {0x1E31, "k"}, {0x1E32, "k"}, {0x1E33, "k"}, {0x1E34, "k"}, {0x1E35, "k"}, {0x1E36, "l"},
    {0x1E37, "l"}, {0x1E38, "l"}, {0x1E39, "l"}, {0x1E3A, "l"}, {0x1E3B, "l"}, {0x1E3C, "l"},
    {0x1E3D, "l"}, {0x1E3E, "m"}, {0x1E3F, "m"}, {0x1E40, "m"}, {0x1E41, "m"}, {0x1E42, "m"},
    {0x1E43, "m"}, {0x1E44, "n"}, {0x1E45, "n"}, {0x1E46, "n"}, {0x1E47, "n"}, {0x1E48, "n"},
    {0x1E49, "n"}, {0x1E4A, "n"}, {0x1E4B, "n"}, {0x1E4C, "o"}, {0x1E4D, "o"}, {0x1E4E, "o"},
    {0x1E4F, "o"}, {0x1E50, "o"}, {0x1E51, "o"}, {0x1E52, "o"}, {0x1E53, "o"}, {0x1E54, "p"},
    {0x1E55, "p"}, {0x1E56, "p"}, {0x1E57, "p"}, {0x1E58, "r"}, {0x1E59, "r"}, {0x1E5A, "r"},
    {0x1E5B, "r"}, {0x1E5C, "r"}, {0x1E5D, "r"}, {0x1E5E, "r"}, {0x1E5F, "r"}, {0x1E60, "s"},
    {0x1E61, "s"}, {0x1E62, "s"}, {0x1E63, "s"}, {0x1E64, "s"}, {0x1E65, "s"}, {0x1E66, "s"},
    {0x1E67, "s"}, {0x1E68, "s"}, {0x1E69, "s"}, {0x1E6A, "t"}, {0x1E6B, "t"}, {0x1E6C, "t"},
    {0x1E6D, "t"}, {0x1E6E, "t"}, {0x1E6F, "t"}, {0x1E70, "t"}, {0x1E71, "t"}, {0x1E72, "u"},
    {0x1E73, "u"}, {0x1E74, "u"}, {0x1E75, "u"}, {0x1E76, "u"}, {0x1E77, "u"}, {0x1E78, "u"},
    {0x1E79, "u"}, {0x1E7A, "u"}, {0x1E7B, "u"}, {0x1E7C, "v"}, {0x1E7D, "v"}, {0x1E7E, "v"},
    {0x1E7F, "v"}, {0x1E80, "w"}, {0x1E81, "w"}, {0x1E82, "w"}, {0x1E83, "w"}, {0x1E84, "w"},
    {0x1E85, "w"}, {0x1E86, "w"}, {0x1E87, "w"}, {0x1E88, "w"}, {0x1E89, "w"}, {0x1E8A, "x"},
    {0x1E8B, "x"}, {0x1E8C, "x"}, {0x1E8D, "x"}, {0x1E8E, "y"}, {0x1E8F, "y"}, {0x1E90, "z"},
    {0x1E91, "z"}, {0x1E92, "z"}, {0x1E93, "z"}, {0x1E94, "z"}, {0x1E95, "z"}, {0x1E96, "h"},
    {0x1E97, "t"}, {0x1E98, "w"}, {0x1E99, "y"}, {0x1E9B, "s"}, {0x1E9E, "ss"}, {0x1EA0, "a"},
    {0x1EA1, "a"}, {0x1EA2, "a"}, {0x1EA3, "a"}, {0x1EA4, "a"}, {0x1EA5, "a"}, {0x1EA6, "a"},
    {0x1EA7, "a"}, {0x1EA8, "a"}, {0x1EA9, "a"}, {0x1EAA, "a"}, {0x1EAB, "a"}, {0x1EAC, "a"},
    {0x1EAD, "a"}, {0x1EAE, "a"}, {0x1EAF, "a"}, {0x1EB0, "a"}, {0x1EB1, "a"}, {0x1EB2, "a"},
    {0x1EB3, "a"}, {0x1EB4, "a"}, {0x1EB5, "a"}, {0x1EB6, "a"}, {0x1EB7, "a"}, {0x1EB8, "e"},
    {0x1EB9, "e"}, {0x1EBA, "e"}, {0x1EBB, "e"}, {0x1EBC, "e"}, {0x1EBD, "e"}, {0x1EBE, "e"},
    {0x1EBF, "e"}, {0x1EC0, "e"}, {0x1EC1, "e"}, {0x1EC2, "e"}, {0x1EC3, "e"}, {0x1EC4, "e"},
    {0x1EC5, "e"}, {0x1EC6, "e"}, {0x1EC7, "e"}, {0x1EC8, "i"}, {0x1EC9, "i"}, {0x1ECA, "i"},
    {0x1ECB, "i"}, {0x1ECC, "o"}, {0x1ECD, "o"}, {0x1ECE, "o"}, {0x1ECF, "o"}, {0x1ED0, "o"},
    {0x1ED1, "o"}, {0x1ED2, "o"}, {0x1ED3, "o"}, {0x1ED4, "o"}, {0x1ED5, "o"}, {0x1ED6, "o"},
    {0x1ED7, "o"}, {0x1ED8, "o"}, {0x1ED9, "o"}, {0x1EDA, "o"}, {0x1EDB, "o"}, {0x1EDC, "o"},
    {0x1EDD, "o"}, {0x1EDE, "o"}, {0x1EDF, "o"}, {0x1EE0, "o"}, {0x1EE1, "o"}, {0x1EE2, "o"},
    {0x1EE3, "o"}, {0x1EE4, "u"}, {0x1EE5, "u"}, {0x1EE6, "u"}, {0x1EE7, "u"}, {0x1EE8, "u"},
    {0x1EE9, "u"}, {0x1EEA, "u"}, {0x1EEB, "u"}, {0x1EEC, "u"}, {0x1EED, "u"}, {0x1EEE, "u"},
    {0x1EEF, "u"}, {0x1EF0, "u"}, {0x1EF1, "u"}, {0x1EF2, "y"}, {0x1EF3, "y"}, {0x1EF4, "y"},
    {0x1EF5, "y"}, {0x1EF6, "y"}, {0x1EF7, "y"}, {0x1EF8, "y"}, {0x1EF9, "y"}, {0x1F00, "a"},
    {0x1F01, "a"}, {0x1F02, "a"}, {0x1F03, "a"}, {0x1F04, "a"}, {0x1F05, "a"}, {0x1F06, "a"},
    {0x1F07, "a"}, {0x1F08, "a"}, {0x1F09, "a"}, {0x1F0A, "a"}, {0x1F0B, "a"}, {0x1F0C, "a"},
    {0x1F0D, "a"}, {0x1F0E, "a"}, {0x1F0F, "a"}, {0x1F10, "e"}, {0x1F11, "e"}, {0x1F12, "e"},
    {0x1F13, "e"}, {0x1F14, "e"}, {0x1F15, "e"}, {0x1F18, "e"}, {0x1F19, "e"}, {0x1F1A, "e"},
    {0x1F1B, "e"}, {0x1F1C, "e"}, {0x1F1D, "e"}, {0x1F20, "n"}, {0x1F21, "n"}, {0x1F22, "n"},
    {0x1F23, "n"}, {0x1F24, "n"}, {0x1F25, "n"}, {0x1F26, "n"}, {0x1F27, "n"}, {0x1F28, "n"},
    {0x1F29, "n"}, {0x1F2A, "n"}, {0x1F2B, "n"}, {0x1F2C, "n"}, {0x1F2D, "n"}, {0x1F2E, "n"},
    {0x1F2F, "n"}, {0x1F30, "i"}, {0x1F31, "i"}, {0x1F32, "i"}, {0x1F33, "i"}, {0x1F34, "i"},
    {0x1F35, "i"}, {0x1F36, "i"}, {0x1F37, "i"}, {0x1F38, "i"}, {0x1F39, "i"}, {0x1F3A, "i"},
    {0x1F3B, "i"}, {0x1F3C, "i"}, {0x1F3D, "i"}, {0x1F3E, "i"}, {0x1F3F, "i"}, {0x1F40, "o"},
    {0x1F41, "o"}, {0x1F42, "o"}, {0x1F43, "o"}, {0x1F44, "o"}, {0x1F45, "o"}, {0x1F48, "o"},
    {0x1F49, "o"}, {0x1F4A, "o"}, {0x1F4B, "o"}, {0x1F4C, "o"}, {0x1F4D, "o"}, {0x1F50, "u"},
    {0x1F51, "u"}, {0x1F52, "u"}, {0x1F53, "u"}, {0x1F54, "u"}, {0x1F55, "u"}, {0x1F56, "u"},
    {0x1F57, "u"}, {0x1F59, "u"}, {0x1F5B, "u"}, {0x1F5D, "u"}, {0x1F5F, "u"}, {0x1F60, "w"},
    {0x1F61, "w"}, {0x1F62, "w"}, {0x1F63, "w"}, {0x1F64, "w"}, {0x1F65, "w"}, {0x1F66, "w"},
    {0x1F67, "w"}, {0x1F68, "w"}, {0x1F69, "w"}, {0x1F6A, "w"}, {0x1F6B, "w"}, {0x1F6C, "w"},
    {0x1F6D, "w"}, {0x1F6E, "w"}, {0x1F6F, "w"}, {0x1F70, "a"}, {0x1F71, "a"}, {0x1F72, "e"},
    {0x1F73, "e"}, {0x1F74, "n"}, {0x1F75, "n"}, {0x1F76, "i"}, {0x1F77, "i"}, {0x1F78, "o"},
    {0x1F79, "o"}, {0x1F7A, "u"}, {0x1F7B, "u"}, {0x1F7C, "w"}, {0x1F7D, "w"}, {0x1F80, "a"},
    {0x1F81, "a"}, {0x1F82, "a"}, {0x1F83, "a"}, {0x1F84, "a"}, {0x1F85, "a"}, {0x1F86, "a"},
    {0x1F87, "a"}, {0x1F88, "a"}, {0x1F89, "a"}, {0x1F8A, "a"}, {0x1F8B, "a"}, {0x1F8C, "a"},
    {0x1F8D, "a"}, {0x1F8E, "a"}, {0x1F8F, "a"}, {0x1F90, "n"}, {0x1F91, "n"}, {0x1F92, "n"},
    {0x1F93, "n"}, {0x1F94, "n"}, {0x1F95, "n"}, {0x1F96, "n"}, {0x1F97, "n"}, {0x1F98, "n"},
    {0x1F99, "n"}, {0x1F9A, "n"}, {0x1F9B, "n"}, {0x1F9C, "n"}, {0x1F9D, "n"}, {0x1F9E, "n"},
    {0x1F9F, "n"}, {0x1FA0, "w"}, {0x1FA1, "w"}, {0x1FA2, "w"}, {0x1FA3, "w"}, {0x1FA4, "w"},
    {0x1FA5, "w"}, {0x1FA6, "w"}, {0x1FA7, "w"}, {0x1FA8, "w"}, {0x1FA9, "w"}, {0x1FAA, "w"},
    {0x1FAB, "w"}, {0x1FAC, "w"}, {0x1FAD, "w"}, {0x1FAE, "w"}, {0x1FAF, "w"}, {0x1FB0, "a"},
    {0x1FB1, "a"}, {0x1FB2, "a"}, {0x1FB3, "a"}, {0x1FB4, "a"}, {0x1FB6, "a"}, {0x1FB7, "a"},
    {0x1FB8, "a"}, {0x1FB9, "a"}, {0x1FBA, "a"}, {0x1FBB, "a"}, {0x1FBC, "a"}, {0x1FBE, "i"},
    {0x1FC2, "n"}, {0x1FC3, "n"}, {0x1FC4, "n"}, {0x1FC6, "n"}, {0x1FC7, "n"}, {0x1FC8, "e"},
    {0x1FC9, "e"}, {0x1FCA, "n"}, {0x1FCB, "n"}, {0x1FCC, "n"}, {0x1FD0, "i"}, {0x1FD1, "i"},
    {0x1FD2, "i"}, {0x1FD3, "i"}, {0x1FD6, "i"}, {0x1FD7, "i"}, {0x1FD8, "i"}, {0x1FD9, "i"},
    {0x1FDA, "i"}, {0x1FDB, "i"}, {0x1FE0, "u"}, {0x1FE1, "u"}, {0x1FE2, "u"}, {0x1FE3, "u"},
    {0x1FE4, "p"}, {0x1FE5, "p"}, {0x1FE6, "u"}, {0x1FE7, "u"}, {0x1FE8, "u"}, {0x1FE9, "u"},
    {0x1FEA, "u"}, {0x1FEB, "u"}, {0x1FEC, "p"}, {0x1FF2, "w"}, {0x1FF3, "w"}, {0x1FF4, "w"},
    {0x1FF6, "w"}, {0x1FF7, "w"}, {0x1FF8, "o"}, {0x1FF9, "o"}, {0x1FFA, "w"}, {0x1FFB, "w"},
    {0x1FFC, "w"}, {0x2071, "i"}, {0x207F, "n"}, {0x2090, "a"}, {0x2091, "e"}, {0x2092, "o"},
    {0x2093, "x"}, {0x2094, "e"}, {0x2095, "h"}, {0x2096, "k"}, {0x2097, "l"}, {0x2098, "m"},
    {0x2099, "n"}, {0x209A, "p"}, {0x209B, "s"}, {0x209C, "t"}, {0x2102, "c"}, {0x2107, "e"},
    {0x210A, "g"}, {0x210B, "h"}, {0x210C, "h"}, {0x210D, "h"}, {0x210E, "h"}, {0x210F, "h"},
    {0x2110, "i"}, {0x2111, "i"}, {0x2112, "l"}, {0x2113, "l"}, {0x2115, "n"}, {0x2119, "p"},
    {0x211A, "q"}, {0x211B, "r"}, {0x211C, "r"}, {0x211D, "r"}, {0x2124, "z"}, {0x2126, "w"},
    {0x2128, "z"}, {0x212A, "k"}, {0x212B, "a"}, {0x212C, "b"}, {0x212D, "c"}, {0x212F, "e"},
    {0x2130, "e"}, {0x2131, "f"}, {0x2133, "m"}, {0x2134, "o"}, {0x2139, "i"}, {0x213C, "n"},
    {0x213D, "y"}, {0x213E, "y"}, {0x213F, "n"}, {0x2145, "d"}, {0x2146, "d"}, {0x2147, "e"},
    {0x2148, "i"}, {0x2149, "j"}, {0x2C60, "l"}, {0x2C61, "l"}, {0x2C62, "l"}, {0x2C63, "p"},
    {0x2C64, "r"}, {0x2C65, "a"}, {0x2C66, "t"}, {0x2C67, "h"}, {0x2C68, "h"}, {0x2C69, "k"},
    {0x2C6A, "k"}, {0x2C6B, "z"}, {0x2C6C, "z"}, {0x2C6D, "a"}, {0x2C6E, "m"}, {0x2C6F, "a"},
    {0x2C70, "a"}, {0x2C71, "v"}, {0x2C72, "w"}, {0x2C73, "w"}, {0x2C74, "v"}, {0x2C75, "h"},
    {0x2C76, "h"}, {0x2C78, "e"}, {0x2C79, "r"}, {0x2C7A, "o"}, {0x2C7B, "e"}, {0x2C7C, "j"},
    {0x2C7D, "v"}, {0x2C7E, "s"}, {0x2C7F, "z"}, {0x3072, "u"}, {0x4E02, "s"}, {0x5200, "n"},
    {0x5C3A, "r"}, {0xA668, "o"}, {0xA669, "o"}, {0xA69C, "b"}, {0xA69D, "b"}, {0xA728, "tz"},
    {0xA729, "tz"}, {0xA730, "f"}, {0xA731, "s"}, {0xA732, "aa"}, {0xA733, "aa"}, {0xA740, "k"},
    {0xA741, "k"}, {0xA742, "k"}, {0xA743, "k"}, {0xA744, "k"}, {0xA745, "k"}, {0xA746, "l"},
    {0xA747, "l"}, {0xA748, "l"}, {0xA749, "l"}, {0xA74A, "o"}, {0xA74B, "o"}, {0xA74C, "o"},
    {0xA74D, "o"}, {0xA74E, "oo"}, {0xA74F, "oo"}, {0xA750, "p"}, {0xA751, "p"}, {0xA752, "p"},
    {0xA753, "p"}, {0xA754, "p"}, {0xA755, "p"}, {0xA756, "q"}, {0xA757, "q"}, {0xA758, "q"},
    {0xA759, "q"}, {0xA75A, "r"}, {0xA75B, "r"}, {0xA75C, "r"}, {0xA75D, "r"}, {0xA75E, "v"},
    {0xA75F, "v"}, {0xA771, "d"}, {0xA772, "l"}, {0xA773, "m"}, {0xA774, "n"}, {0xA775, "r"},
    {0xA776, "r"}, {0xA777, "t"}, {0xA779, "d"}, {0xA77A, "d"}, {0xA77B, "f"}, {0xA77C, "f"},
    {0xA77D, "g"}, {0xA77E, "g"}, {0xA77F, "g"}, {0xA780, "l"}, {0xA781, "l"}, {0xA782, "r"},
    {0xA783, "r"}, {0xA784, "s"}, {0xA785, "s"}, {0xA786, "t"}, {0xA787, "t"}, {0xA78D, "h"},
    {0xA790, "n"}, {0xA791, "n"}, {0xA792, "c"}, {0xA793, "c"}, {0xA794, "c"}, {0xA795, "h"},
    {0xA796, "b"}, {0xA797, "b"}, {0xA798, "f"}, {0xA799, "f"}, {0xA7A0, "g"}, {0xA7A1, "g"},
    {0xA7A2, "k"}, {0xA7A3, "k"}, {0xA7A4, "n"}, {0xA7A5, "n"}, {0xA7A6, "r"}, {0xA7A7, "r"},
    {0xA7A8, "s"}, {0xA7A9, "s"}, {0xA7AA, "h"}, {0xA7AB, "e"}, {0xA7AC, "g"}, {0xA7AD, "l"},
    {0xA7AE, "i"}, {0xA7B0, "k"}, {0xA7B1, "t"}, {0xA7B2, "j"}, {0xA7B3, "x"}, {0xA7B4, "b"},
    {0xA7B5, "b"}, {0xA7B6, "w"}, {0xA7B7, "w"}, {0xA7C4, "c"}, {0xA7C5, "s"}, {0xA7CB, "y"},
    {0xA7DC, "a"}, {0xA7F2, "c"}, {0xA7F3, "f"}, {0xA7F4, "q"}, {0xA7F8, "h"}, {0xA7F9, "oe"},
    {0xAB53, "x"}, {0xAB5E, "l"}, {0xAB69, "m"}, {0xFF91, "a"},
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

    // A letter from one of the blocks of styled alphabets that keep the order
    // of A to Z: fullwidth, mathematical, circled, squared, regional indicators.
    // Returns the plain lowercase letter, or 0.
    llwchar styledLetter(llwchar c)
    {
        struct Block { U32 mFirst; U32 mLast; U32 mPeriod; };
        constexpr Block BLOCKS[] =
        {
            { 0xFF21, 0xFF3A, 26 },   // fullwidth A-Z
            { 0xFF41, 0xFF5A, 26 },   // fullwidth a-z
            { 0x1D400, 0x1D6A3, 52 }, // mathematical bold, italic, script, fraktur... A-Z a-z
            { 0x249C, 0x24B5, 26 },   // parenthesized a-z
            { 0x24B6, 0x24CF, 26 },   // circled A-Z
            { 0x24D0, 0x24E9, 26 },   // circled a-z
            { 0x1F130, 0x1F149, 26 }, // squared A-Z
            { 0x1F150, 0x1F169, 26 }, // negative circled A-Z
            { 0x1F170, 0x1F189, 26 }, // negative squared A-Z
            { 0x1F1E6, 0x1F1FF, 26 }, // regional indicators A-Z
        };
        for (const auto& block : BLOCKS)
        {
            if (c >= block.mFirst && c <= block.mLast)
            {
                return 'a' + (llwchar)(((c - block.mFirst) % block.mPeriod) % 26);
            }
        }
        return 0;
    }

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

        if (llwchar letter = styledLetter(c))
        {
            appendLatin(out, letter);
            return CharKind::Latin;
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
