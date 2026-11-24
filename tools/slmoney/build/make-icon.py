#!/usr/bin/env python3
"""Makes build/icon.png: the Companion Viewer icon with a banknote emoji in the lower left.

    python3 build/make-icon.py SOURCE_ICON.png

SOURCE_ICON is the viewer's icon (source/doc/companionviewer_icon.png). Needs Pillow and
the Noto Color Emoji font. electron-builder turns icon.png into the .icns and .ico it needs.
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

SIZE = 1024
EMOJI = "\U0001F4B5"  # banknote with dollar sign
FONT = "/usr/share/fonts/noto/NotoColorEmoji.ttf"

icon = Image.open(sys.argv[1]).convert("RGBA").resize((SIZE, SIZE), Image.LANCZOS)

# Noto Color Emoji is a bitmap font that only renders at size 109; draw it there, then scale.
font = ImageFont.truetype(FONT, 109)
glyph = Image.new("RGBA", (160, 160), (0, 0, 0, 0))
ImageDraw.Draw(glyph).text((80, 80), EMOJI, font=font, embedded_color=True, anchor="mm")
glyph = glyph.crop(glyph.getbbox())
width = int(SIZE * 0.46)
glyph = glyph.resize((width, int(glyph.height * width / glyph.width)), Image.LANCZOS)

margin = int(SIZE * 0.03)
icon.alpha_composite(glyph, (margin, SIZE - glyph.height - margin))
icon.save(Path(__file__).with_name("icon.png"))
