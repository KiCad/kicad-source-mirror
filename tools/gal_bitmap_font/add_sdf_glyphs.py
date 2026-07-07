#!/usr/bin/env python3
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""
Append glyphs to the OpenGL GAL bitmap (MSDF) font atlas without rerunning the
full msdf-atlas-gen pipeline.

The atlas in common/gal/opengl/ is a signed-distance-field texture indexed by
codepoint spans. Ubuntu Mono (its source) lacks many symbols, so this tool
rasterizes replacement glyphs from Noto, converts each to a single-channel SDF
matched to the atlas encoding (inside=255, outside=0, edge=0.5, ~3px range) and
stores it as R=G=B. The fragment shader takes median(rgb), so an equal-channel
SDF renders identically to a native MSDF glyph.

New glyphs are packed into a fresh shelf-band appended below the existing atlas
so the current 1024 rows stay byte-identical (img.c gains a pure append; the
pixels[] bound in gl_resources.h must track the new height).

Requires: freetype-py, numpy, scipy, and Noto fonts (fonts-noto-core).
"""
import os
import re
import sys

import freetype
import numpy as np
from scipy import ndimage

HERE = os.path.dirname(os.path.abspath(__file__))
GL_DIR = os.path.normpath(os.path.join(HERE, "../../common/gal/opengl"))
NOTO = "/usr/share/fonts/truetype/noto/"

FONTS = {
    "math": NOTO + "NotoSansMath-Regular.ttf",
    "sym2": NOTO + "NotoSansSymbols2-Regular.ttf",
}

# Target atlas codepoint -> (Noto font key, codepoint to draw from that font).
# A few pictographs are emoji-only in Unicode; a monochrome Noto stand-in with
# the same meaning is drawn instead (heavy check for U+2705, ballot-box-X for
# U+274E).
SOURCES = {
    0x2194: ("math", 0x2194), 0x2195: ("math", 0x2195), 0x21D4: ("math", 0x21D4),
    0x2208: ("math", 0x2208), 0x2220: ("math", 0x2220), 0x2225: ("math", 0x2225),
    0x229A: ("math", 0x229A), 0x229C: ("math", 0x229C), 0x22A5: ("math", 0x22A5),
    0x2316: ("sym2", 0x2316), 0x25CE: ("sym2", 0x25CE), 0x25CF: ("sym2", 0x25CF),
    0x26A0: ("sym2", 0x26A0), 0x26A1: ("sym2", 0x26A1),
    0x2705: ("sym2", 0x2714), 0x274E: ("sym2", 0x2612), 0x27F7: ("math", 0x27F7),
}

EM_UNITS = 48.0   # atlas units per em (from the Ubuntu Mono cap-height metrics)
PXRANGE = 3.0     # SDF ramp width in atlas px, matched to existing glyphs
SS = 8            # supersample factor for the distance transform
ADVANCE = 24.0    # monospace advance of the atlas

_faces = {}


def face(key):
    if key not in _faces:
        _faces[key] = freetype.Face(FONTS[key])
    return _faces[key]


def load_desc():
    src = open(GL_DIR + "/bitmap_font_desc.c", encoding="utf-8").read()
    spans, glyphs = [], []
    sm = re.search(r"font_codepoint_spans\[\]\s*=\s*\{(.*?)\};", src, re.S).group(1)
    for a, b, c in re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", sm):
        spans.append((int(a), int(b), int(c)))
    gm = re.search(r"font_codepoint_infos\[\]\s*=\s*\{(.*)\};", src, re.S).group(1)
    n = len(re.findall(r"\{[^}]*\}", gm))
    return src, spans, n


def in_atlas(spans, cp):
    return any(s <= cp < e for s, e, _ in spans)


def gen_tile(cp, smooth):
    fkey, srccp = SOURCES[cp]
    f = face(fkey)
    f.set_pixel_sizes(0, int(round(EM_UNITS * SS)))
    f.load_char(srccp, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
    g = f.glyph
    hrw, hrh = g.bitmap.width, g.bitmap.rows
    if hrw == 0 or hrh == 0:
        raise ValueError(f"empty raster for U+{cp:04X}")
    cov = np.array(g.bitmap.buffer, dtype=np.uint8).reshape(hrh, hrw)
    minx = g.bitmap_left / SS
    maxx = (g.bitmap_left + hrw) / SS
    maxy = g.bitmap_top / SS
    miny = (g.bitmap_top - hrh) / SS
    aw = int(np.ceil(maxx - minx)) + 2 * smooth
    ah = int(np.ceil(maxy - miny)) + 2 * smooth
    canvas = np.zeros((ah * SS, aw * SS), dtype=np.uint8)
    canvas[smooth * SS:smooth * SS + hrh, smooth * SS:smooth * SS + hrw] = cov
    mask = canvas >= 128
    sd = (ndimage.distance_transform_edt(mask)
          - ndimage.distance_transform_edt(~mask)) / SS
    ys = (np.arange(ah) * SS + SS // 2)
    xs = (np.arange(aw) * SS + SS // 2)
    val = np.clip(0.5 + sd[np.ix_(ys, xs)] / PXRANGE, 0.0, 1.0)
    tile = np.round(val * 255).astype(np.uint8)
    # Advance must clear the glyph's ink (drawBitmapChar advances by this alone),
    # so wide symbols do not overlap the next character. Keep the monospace cell
    # as the floor and mirror the left bearing on the right.
    advance = max(ADVANCE, round(maxx + minx, 4))
    return tile, dict(aw=aw, ah=ah, minx=round(minx, 4), maxx=round(maxx, 4),
                      miny=round(miny, 4), maxy=round(maxy, 4), advance=advance)


def main():
    desc_src, spans, nglyphs = load_desc()
    hdr = re.search(r"font_image\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{",
                    open(GL_DIR + "/bitmap_font_img.c", encoding="utf-8").read())
    W, H, _, smooth = map(int, hdr.groups())

    for cp in SOURCES:
        if in_atlas(spans, cp):
            print(f"skip U+{cp:04X}: already in atlas")

    todo = [cp for cp in sorted(SOURCES) if not in_atlas(spans, cp)]
    if not todo:
        print("nothing to add")
        return

    tiles = {cp: gen_tile(cp, smooth) for cp in todo}

    # shelf-pack into a new band below the atlas
    SP, x, y, row_h = 1, 1, H + 1, 0
    place = {}
    for cp in sorted(todo, key=lambda c: -tiles[c][1]["ah"]):
        aw, ah = tiles[cp][1]["aw"], tiles[cp][1]["ah"]
        if x + aw + SP > W:
            x, y, row_h = 1, y + row_h + SP, 0
        place[cp] = (x, y)
        x += aw + SP
        row_h = max(row_h, ah)
    new_h = y + row_h + SP

    band = np.zeros((new_h - H, W, 3), dtype=np.uint8)
    for cp, (ax, ay) in place.items():
        tile = tiles[cp][0]
        for ch in range(3):
            band[ay - H:ay - H + tile.shape[0], ax:ax + tile.shape[1], ch] = tile

    _write_desc(desc_src, tiles, place, nglyphs)
    _write_img(W, H, new_h, band)
    print(f"added {len(todo)} glyphs; atlas {W}x{H} -> {W}x{new_h}")
    print(f"** update gl_resources.h: pixels[1024 * {new_h} * 3]")


def _write_desc(src, tiles, place, nglyphs):
    span_line = {cp: f"    {{ {cp}, {cp + 1}, {nglyphs + i} }},"
                 for i, cp in enumerate(sorted(tiles))}
    out, mode, done = [], None, set()
    for line in src.splitlines():
        if "font_codepoint_spans[]" in line:
            mode = "spans"
        elif "font_codepoint_infos[]" in line:
            mode = "infos"
        elif mode == "spans":
            m = re.match(r"\s*\{\s*(\d+),", line)
            if m:
                for cp in sorted(tiles):
                    if cp not in done and cp < int(m.group(1)):
                        out.append(span_line[cp]); done.add(cp)
            elif line.strip() == "};":
                for cp in sorted(tiles):
                    if cp not in done:
                        out.append(span_line[cp]); done.add(cp)
                mode = None
        elif mode == "infos" and line.strip() == "};":
            for cp in sorted(tiles):
                g = tiles[cp][1]
                ax, ay = place[cp]
                out.append(f"    {{ {ax}, {ay}, {g['aw']}, {g['ah']}, "
                           f"{g['minx']:.4f}f, {g['maxx']:.4f}f, "
                           f"{g['miny']:.4f}f, {g['maxy']:.4f}f, {g['advance']:.4f}f }},")
            mode = None
        out.append(line)
    open(GL_DIR + "/bitmap_font_desc.c", "w").write("\n".join(out) + "\n")


def _write_img(W, H, new_h, band):
    path = GL_DIR + "/bitmap_font_img.c"
    orig = open(path, encoding="utf-8").read()
    hdr = re.search(r"(font_image\s*=\s*\{\s*)(\d+)(,\s*)(\d+)(,)", orig)
    head = orig[:hdr.start()] + hdr.group(1) + str(W) + hdr.group(3) + str(new_h) + hdr.group(5)
    rest = orig[hdr.end():]
    close = rest.rfind("}", 0, rest.rfind("}"))
    pixels = rest[:close].rstrip()
    if not pixels.endswith(","):
        pixels += ","
    band_bytes = ",".join(map(str, band.reshape(-1).tolist()))
    open(path, "w").write(head + pixels + "\n" + band_bytes + "\n" + rest[close:])


if __name__ == "__main__":
    sys.exit(main())
