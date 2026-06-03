#!/usr/bin/env python3
"""
Generate a minimal TrueType font that reproduces the legacy "symbol" font layout
from KiCad issue #24514: glyphs live in the U+F000..U+F0FF private-use range and
the font carries a (3,0) Microsoft Symbol cmap plus a (0,0) Unicode cmap that
mirrors that private-use layout. It deliberately has NO (3,1) Unicode BMP cmap,
so selecting FT_ENCODING_UNICODE leaves the Basic Latin block unmapped.

Authored for the KiCad project; released under the same license as KiCad.
"""
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

upm = 1000
glyph_order = [".notdef", "A", "B"]

def box(pen, x0, y0, x1, y1):
    pen.moveTo((x0, y0)); pen.lineTo((x1, y0)); pen.lineTo((x1, y1)); pen.lineTo((x0, y1)); pen.closePath()

pens = {}
# .notdef: an open rectangle (the classic tofu box)
p = TTGlyphPen(None); box(p, 100, 0, 500, 700); pens[".notdef"] = p.glyph()
# A: a distinct filled rectangle
p = TTGlyphPen(None); box(p, 50, 0, 600, 700); pens["A"] = p.glyph()
# B: a different, narrower rectangle so its outline differs from A
p = TTGlyphPen(None); box(p, 50, 0, 300, 700); pens["B"] = p.glyph()

fb = FontBuilder(upm, isTTF=True)
fb.setupGlyphOrder(glyph_order)
fb.setupCharacterMap({})  # we install custom cmap subtables below
fb.setupGlyf(pens)
metrics = {g: (650, 50) for g in glyph_order}
fb.setupHorizontalMetrics(metrics)
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "KiCadSymbolTest", "styleName": "Regular",
                   "psName": "KiCadSymbolTest-Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200)
fb.setupPost()

# Build cmap manually: (3,0) symbol + (0,0) unicode, both mapping U+F041/U+F042 only.
from fontTools.ttLib.tables._c_m_a_p import CmapSubtable
pua = {0xF041: "A", 0xF042: "B"}
cmap = fb.font["cmap"] if "cmap" in fb.font else None
from fontTools.ttLib import newTable
cmap = newTable("cmap"); cmap.tableVersion = 0
sub_sym = CmapSubtable.getSubtableClass(4)(4)
sub_sym.platformID, sub_sym.platEncID, sub_sym.format, sub_sym.language = 3, 0, 4, 0
sub_sym.cmap = dict(pua)
sub_uni = CmapSubtable.getSubtableClass(4)(4)
sub_uni.platformID, sub_uni.platEncID, sub_uni.format, sub_uni.language = 0, 0, 4, 0
sub_uni.cmap = dict(pua)
cmap.tables = [sub_uni, sub_sym]
fb.font["cmap"] = cmap

out = "qa/resources/fonts/symbol_pua_test.ttf"
fb.save(out)
print("wrote", out)
