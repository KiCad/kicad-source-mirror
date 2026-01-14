#!/usr/bin/python
"""
Generates newstroke_font.cpp from .kicad_sym font libraries.

Usage: fontconv.py
"""

from io import TextIOBase
from typing import Any, NamedTuple
import re
import sys

try:
    import fontforge
except ImportError:
    print("** fontforge is not installed. woff output will fail")
    print("** see https://fontforge.org for installation instructions")
    class fontforge:
        class glyphPen:
            pass
        class font:
            pass

global_duplicate_point_removal = True

input_fonts = ['symbol', 'font', 'hiragana',
               'katakana', 'half_full', 'CJK_symbol',
               'CJK_wide_U+4E00',
               'CJK_wide_U+5AE6',
               'CJK_wide_U+66B9',
               'CJK_wide_U+7212',
               'CJK_wide_U+7D2A',
               'CJK_wide_U+8814',
               'CJK_wide_U+92B4',
               'CJK_wide_U+9C60']

input_charlist = 'charlist.txt'
input_header = 'font_header.cpp'
output_cpp = '../../common/newstroke_font.cpp'


FONT_BASE = 9
FONT_SCALE = 50
FONT_WOFF_THICKNESS = 2.53
FONT_WOFF_XOFFS = 0
FONT_WOFF_YOFFS = FONT_WOFF_THICKNESS/2
FONT_WOFF_SCALE = 29.7
FONT_C_BIAS = ord("R")
FONT_NEWSTROKE = " R"
C_ESC_TRANS = str.maketrans({'"': '\\"', '\\': '\\\\'})

REMOVE_REDUNDANT_STROKES = re.compile(r"(?P<point>\S\S) R(?P=point)")
REMOVE_POINT_PAIRS = re.compile(r"^(?P<prefix>(..)*)(?P<point>\S\S)(?P=point)")


def mm_to_mil_scaled(mm: float) -> int:
    return round(mm / 0.0254 - 0.1) // FONT_SCALE


def c_encode(a: int, b: int) -> str:
    return chr(a + FONT_C_BIAS) + chr(b + FONT_C_BIAS)


def remove_duplicate_points(data: str) -> str:
    data = REMOVE_REDUNDANT_STROKES.sub(r"\g<point>", data)
    return REMOVE_POINT_PAIRS.sub(r"\g<prefix>\g<point>", data)


def cesc(s: str):
    return s.translate(C_ESC_TRANS)


###
# S-Expressions
###

# Sexpr code extracted from: http://rosettacode.org/wiki/S-Expressions

term_regex = r"""(?mx)
    \s*(?:
        (\()|
        (\))|
        ([+-]?\d+\.\d+(?=[\ \)\n]))|
        (\-?\d+(?=[\ \)\n]))|
        "((?:[^"]|(?<=\\)")*)"|
        ([^(^)\s]+)
       )"""


class SexprError(ValueError):
    pass


def parse_sexp(sexp: str) -> Any:
    re_iter = re.finditer(term_regex, sexp)
    rv = list(_parse_sexp_internal(re_iter))

    for leftover in re_iter:
        lparen, rparen, *rest = leftover.groups()
        if lparen or any(rest):
            raise SexprError(f'Leftover garbage after end of expression at position {leftover.start()}')  # noqa: E501

        elif rparen:
            raise SexprError(
                f'Unbalanced closing parenthesis at position {leftover.start()}')

    if len(rv) == 0:
        raise SexprError('No or empty expression')

    if len(rv) > 1:
        raise SexprError('Missing initial opening parenthesis')

    return rv[0]


def _parse_sexp_internal(re_iter) -> Any:
    for match in re_iter:
        lparen, rparen, float_num, integer_num, quoted_str, bare_str = match.groups()

        if lparen:
            yield list(_parse_sexp_internal(re_iter))
        elif rparen:
            break
        elif bare_str is not None:
            yield bare_str
        elif quoted_str is not None:
            yield quoted_str.replace('\\"', '"')
        elif float_num:
            yield float(float_num)
        elif integer_num:
            yield int(integer_num)

###
# Primitives
###


class Transform(NamedTuple):
    SX: int = +1
    SY: int = +1
    OY: int = 0


class Point(NamedTuple):
    x: int
    y: int

    @staticmethod
    def from_mm(sx: str, sy: str) -> 'Point':
        x = mm_to_mil_scaled(float(sx))
        y = mm_to_mil_scaled(-float(sy))
        return Point(x, y)

    def transformed(self, tr: Transform, ofs: 'Point') -> 'Point':
        return Point(self.x * tr.SX + ofs.x, self.y * tr.SY + tr.OY + ofs.y)

    def as_data(self) -> str:
        return c_encode(self.x, self.y + FONT_BASE)

    def __add__(self, other):
        return Point(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        return Point(self.x - other.x, self.y - other.y)


POINT0 = Point(0, 0)
DEFAULT_TRANSFORM = Transform()


class Metrics(NamedTuple):
    l: int
    r: int

    def transformed(self, tr: Transform, ofs: Point) -> 'Metrics':
        a = self.l * tr.SX + ofs.x
        b = self.r * tr.SX + ofs.x
        return Metrics(min(a, b), max(a, b))

    def as_data(self):
        return c_encode(self.l, self.r)

    def __and__(self, other):
        if self.l == self.r:
            return other
        elif other.l == other.r:
            return self
        left = min(self.l, self.r, other.l, other.r)
        right = max(self.l, self.r, other.l, other.r)
        return Metrics(left, right)


###
# Glyph Input
###

class KicadSymError(ValueError):
    pass


class Glyph(NamedTuple):
    """The immutable shape and metrics of a single glyph.

    Carries the shape of a single glyph.
    Can parse the data from the .kicad_sym symbol sexp.

    The `data` of the glyph eventually ends up in the glyph array
    newstroke_font.cpp`

    Attributes:
        name:     The name of the symbol library component this glyph came from
        metrics:  Left & right extents
        anchors:  Named anchor points
        strokes:  Strokes in this glyph
        width:    Computed width of the glyph
    """

    name: str
    metrics: Metrics
    anchors: dict[str, Point]
    strokes: list[tuple[Point, ...]]

    def as_data(self, tr: Transform, ofs: Point) -> str:
        def stroke_gen(s):
            return "".join(map(lambda p: p.transformed(tr, ofs).as_data(), s))
        data = FONT_NEWSTROKE.join(map(stroke_gen, self.strokes))

        if global_duplicate_point_removal:
            return data
        else:
            return remove_duplicate_points(data)

    def render(self, tr: Transform, ofs: Point, pen: fontforge.glyphPen):
        woff_tr = Transform(SX=FONT_WOFF_SCALE, SY=-FONT_WOFF_SCALE, OY=0)
        woff_ofs = Point(
                x=FONT_WOFF_XOFFS*FONT_WOFF_SCALE,
                y=FONT_WOFF_YOFFS*FONT_WOFF_SCALE
                )
        for stroke in self.strokes:
            if not stroke:
                continue
            transformed = (
                    p.transformed(tr, ofs).transformed(woff_tr, woff_ofs)
                    for p in stroke
                    )
            pen.moveTo(next(transformed))
            for t in transformed:
                pen.lineTo(t)
            pen.endPath()

    @property
    def width(self):
        return self.metrics.r - self.metrics.l

    @classmethod
    def from_sexpr(cls, sexp: Any) -> 'Glyph':
        if sexp[0] != "symbol":
            raise KicadSymError(f"Expected a symbol sexpr: {sexp}")

        # If the name ends with a double underscore suffix, it's a suffix with the
        # codepoint of the glyph as a readable glyph, and we don't need it here.
        name = re.sub(r"__.*$", "", sexp[1])

        if name[0] in Compositions.transforms:
            raise KicadSymError(f"Invalid glyph name {name}")

        anchors = {'-': POINT0}
        strokes: list[tuple[Point, ...]] = []
        for s1 in sexp[2:]:
            if s1[0] == "symbol":
                for s2 in s1[1:]:
                    if s2[0] == "polyline":
                        strokes.append(Glyph._parse_polyline(s2))
                    elif s2[0] == "pin":
                        anchor, point = Glyph._parse_pin(s2)
                        anchors[anchor] = point

        P, S = anchors.get("P", POINT0), anchors.get("S", POINT0)
        if P.x > S.x:
            raise KicadSymError(
                f"P/S anchors are right-to-left: P={P.x}, S={S.x}")
        if P is POINT0:
            print(f"   Warning: missing P anchor in glyph {name}")
        if S is POINT0:
            print(f"   Warning: missing S anchor in glyph {name}")

        return Glyph(name, Metrics(P.x, S.x), anchors, strokes)

    @staticmethod
    def _parse_polyline(sexp: Any) -> tuple[Point, ...]:
        if sexp[0] != "polyline":
            raise KicadSymError(f"Expected a polyline sexpr: {sexp}")
        for s in sexp[1:]:
            if s[0] == "pts":
                points = s[1:]
                break

        stroke: list[Point] = []
        for key, x, y in points:
            if key != "xy":
                raise KicadSymError(f"Expected a point sexpr: {points}")
            stroke.append(Point.from_mm(x, y))

        return tuple(stroke)

    @staticmethod
    def _parse_pin(sexp: Any) -> tuple[str, Point]:
        # pins are used for metrics and anchors
        if sexp[0] != "pin":
            raise KicadSymError(f"Expected a pin sexpr: {sexp}")
        for s in sexp[1:]:
            if s[0] == "at":
                point = Point.from_mm(s[1], s[2])
            elif s[0] == "name":
                pname = s[1]

        if pname == "~":
            pname = "P" if point.x <= 0 else "S"
        return pname, point


def parse_symfile(filename: str) -> dict[str, Glyph]:
    with open(filename, 'r', encoding='utf-8') as f:
        sexp = parse_sexp(f.read())

    glyphs: dict[str, Glyph] = {}
    for s in sexp:
        if s[0] == 'symbol':
            glyph = Glyph.from_sexpr(s)
            glyphs[glyph.name] = glyph
    return glyphs

###
# Compositor
###


def _make_transforms() -> dict[str, Transform]:
    cap_height = -21
    x_height = -14
    sym_height = -16
    sup_offset = -13
    sub_offset = 6
    # transformation prefixes used in charlist.txt
    #                  SX  SY  OY
    return {
        "!": Transform(-1, +1, 0),           # revert
        "-": Transform(+1, -1, x_height),    # invert small
        "=": Transform(+1, -1, cap_height),  # invert cap
        "~": Transform(+1, -1, sym_height),  # invert symbol
        "+": Transform(-1, -1, x_height),    # rotate small
        "%": Transform(-1, -1, cap_height),  # rotate cap
        "*": Transform(-1, -1, sym_height),  # rotate symbol
        "^": Transform(+1, +1, sup_offset),  # superscript
        "`": Transform(-1, +1, sup_offset),  # superscript reversed
        ".": Transform(+1, +1, sub_offset),  # subscript
        ",": Transform(-1, +1, sub_offset),  # subscript reversed
    }


class SubGlyph(NamedTuple):
    glyph: Glyph
    tname: str
    transform: Transform = DEFAULT_TRANSFORM
    offset: Point = POINT0

    def as_data(self):
        return self.glyph.as_data(self.transform, self.offset)

    def render(self, pen: fontforge.glyphPen, ofs: Point):
        offset = self.offset.transformed(Transform(), ofs)
        self.glyph.render(self.transform, offset, pen)


class Composition(list[SubGlyph]):
    __slots__ = ["metrics"]

    def __init__(self, *args):
        super().__init__(self, *args)
        self.metrics = Metrics(0, 0)

    def append(self, sg: SubGlyph, in_metrics=True):
        if in_metrics:
            self.metrics &= sg.glyph.metrics.transformed(
                sg.transform, sg.offset)
        super().append(sg)

    def as_data(self) -> str:
        mdata = self.metrics.as_data()
        gdata = FONT_NEWSTROKE.join(map(SubGlyph.as_data, self))
        if global_duplicate_point_removal:
            return mdata + remove_duplicate_points(gdata)
        else:
            return mdata + gdata

    def create_char(self, cp: int, font: fontforge.font):
        name = ' '.join(c.tname for c in self if c.tname)
        if name == "0":  # 0 has special meaning in fonts
            name = "ZERO"
        glyph = font.createChar(cp, name)
        offset = Point(x=-self.metrics.l, y=0)
        pen = glyph.glyphPen()
        for g in self:
            g.render(pen, offset)
        pen = None
        glyph.stroke("circular", font.strokewidth)
        glyph.width = round((self.metrics.r - self.metrics.l) * FONT_WOFF_SCALE)


class Compositions:
    """Compositions of glyphs for every code point described in the character list file"""

    transforms = _make_transforms()

    def __init__(self, glyphs: dict[str, Glyph]):
        self.glyphs = glyphs
        self.default_subglyph = SubGlyph(glyphs["DEL"], "")
        self.empty_subglyph = SubGlyph(glyphs["0"], "")
        self.missed: set[str] = set()
        self.used: set[str] = set()

        self._skip = POINT0

        self.font_name = "default_font"
        self.codepoints: list[Composition] = []
        self.comments: dict[int, str] = {}

    @classmethod
    def from_read(cls, sin: TextIOBase, glyphs: dict[str, Glyph]) -> 'Compositions':
        charlist = Compositions(glyphs)
        try:
            for line in sin:
                line = line[:line.find("#")]  # remove comments
                charlist._parse_command(line)
        except BaseException:
            print(f"Error parsing line '{line}'", file=sys.stderr)
            raise
        return charlist

    @property
    def _codepoint(self) -> int:
        return len(self.codepoints)-1

    def _new_composition(self) -> None:
        self.codepoints.append(Composition())
        self._skip = POINT0

    def _gl_tr(self, glyphname: str) -> tuple[Glyph, Transform]:
        transform = Compositions.transforms.get(
            glyphname[0], DEFAULT_TRANSFORM)
        if transform is not DEFAULT_TRANSFORM:
            glyphname = glyphname[1:]

        glyph = self.glyphs.get(glyphname, None)
        if glyph:
            self.used.add(glyphname)
        else:
            self.missed.add(glyphname)
            glyph = self.default_subglyph.glyph

        return glyph, transform

    def _parse_command(self, line) -> None:
        tokens = line.split()
        tokens.reverse()
        if not tokens:
            return
        cmd = tokens.pop()
        if cmd in {"+", "+w", "+p", "+(", "+|", "+)"}:
            if cmd != "+|" and cmd != "+)":
                self._new_composition()
            self._parse_entry(tokens, cmd == "+w" or cmd == "+p")
        elif cmd.startswith("//"):
            self.comments[self._codepoint] = line
        elif cmd == "skipcodes":
            numskip = int(tokens.pop())
            subglyph = self.default_subglyph if self._codepoint < 0x9000 else self.empty_subglyph
            for _ in range(numskip):
                self._new_composition()
                self.codepoints[-1].append(subglyph)
        elif cmd == "startchar":
            codepoint = int(tokens.pop())
            self.codepoints = [Composition()] * codepoint
        elif cmd == "font":
            self.font_name = tokens.pop()
        else:
            raise ValueError(f"Invalid charlist command '{cmd}'")

    def _parse_entry(self, tokens, sub_metrics) -> None:
        composition = self.codepoints[-1]

        bname = tokens.pop()
        base, btr = self._gl_tr(bname)
        if self._skip is not POINT0:
            self._skip += Point(-base.metrics.l, 0)
        composition.append(SubGlyph(base, bname, btr, self._skip))

        while tokens:
            name = tokens.pop()
            sub, tr = self._gl_tr(name)
            offset = self._skip

            if tokens:
                parts = tokens.pop().split("=")
                if len(parts) == 2:
                    n_from, n_to = parts
                    a_from = base.anchors[n_from].transformed(btr, POINT0)
                    a_to = sub.anchors[n_to].transformed(tr, POINT0)
                    offset += (a_from - a_to)

            composition.append(SubGlyph(sub, name, tr, offset), sub_metrics)

        self._skip += Point(base.metrics.r, 0)


class CFontWriter:
    """Processes the compositions to generate a C file with the font."""

    def __init__(self, comps: Compositions):
        self.comps = comps

    def print_stats(self, sout: TextIOBase):
        glyphs = set(self.comps.glyphs.keys())
        unused_glyphs = list(glyphs - self.comps.used)

        if self.comps.missed:
            print(
                f"/* --- {len(self.comps.missed)} missed glyphs --- */", file=sout)
            for m in self.comps.missed:
                print(f"/*  {m}  */", file=sout)

        if unused_glyphs:
            print("/* --- unused glyphs --- */", file=sout)
            unused_glyphs.sort()
            for u in unused_glyphs:
                print(f"/*  {u}  */", file=sout)

    def generate_c_output(self, start_cp: int, sout: TextIOBase) -> None:
        fname = self.comps.font_name
        print(f"\n\n\nconst char* const {fname}[] =\n{{", file=sout)

        cmt_iter = iter(self.comps.comments)
        while (cmt_point := next(cmt_iter, start_cp)) < start_cp:
            self._print_comment(cmt_point, sout)

        for cp, composition in enumerate(self.comps.codepoints[start_cp:], start_cp):
            CFontWriter._print_row(cp, composition, sout)
            if cmt_point == cp:
                self._print_comment(cmt_point, sout)
                cmt_point = next(cmt_iter, 0)

        end = f"}};\nconst int {fname}_bufsize = sizeof({fname})/sizeof({fname}[0]);\n"
        print(end, file=sout)

    def _print_comment(self, codepoint, sout):
        print(f"    /* {self.comps.comments[codepoint]} */", file=sout)

    @staticmethod
    def _print_row(codepoint, composition: Composition, sout: TextIOBase):
        data = cesc(composition.as_data())
        if codepoint % 16:
            print(f'    "{data}",', file=sout)
        else:
            name1 = composition[0].tname
            name2 = composition[1].tname if len(composition) > 1 else ""
            if name1:
                print(
                    f'    "{data}", /* U+{codepoint:X} {name1} {name2} */', file=sout)
            else:
                print(f'    "{data}", /* U+{codepoint:X} */', file=sout)


class WoffFontWriter:
    """Processes the compositions to fill in a font's glyphs."""

    def __init__(self, comps: Compositions):
        self.comps = comps

    def generate_glyphs(self, start_cp: int, end_cp: int, font: fontforge.font):
        fname = self.comps.font_name.replace("_font", "")
        font.familyname = fname
        font.fontname = f"kicad-{fname}"
        font.fullname = f"KiCad {fname.capitalize()}"
        font.copyright = 'CC0-1.0'
        font.strokedfont = True
        font.strokewidth = FONT_WOFF_THICKNESS * FONT_WOFF_SCALE
        for cp, composition in enumerate(self.comps.codepoints[start_cp:end_cp+1], start_cp):
            # Don't output tofu/blanks into the file
            if chr(cp).isspace() or any(c.glyph.strokes and c.glyph != self.comps.default_subglyph.glyph for c in composition):
                composition.create_char(cp, font)


if __name__ == "__main__":
    cp_min = 0x20
    cp_max = 0xFFFF
    woff = None
    for arg in sys.argv[1:]:
        param = arg.partition("=")
        if param[0] == "--woff":
            woff = param[2]
        elif param[0] == "--cp_min":
            cp_min = int(param[2], 0)
        elif param[0] == "--cp_max":
            cp_max = int(param[2], 0)

    print('** Reading glyphs from fonts:')
    all_glyphs: dict[str, Glyph] = {}
    for basename in input_fonts:
        print(f' - Reading {basename}.kicad_sym...')
        all_glyphs |= parse_symfile(f'{basename}.kicad_sym')

    print(f"** Reading {input_header}")
    header = open(input_header, encoding='utf-8').read()

    print(f"** Reading {input_charlist}")
    with open(input_charlist, encoding='utf-8') as src:
        compositions = Compositions.from_read(src, all_glyphs)

    if woff:
        print(f"** Writing {woff}")
        font = fontforge.font()
        WoffFontWriter(compositions).generate_glyphs(cp_min, cp_max, font)
        font.generate(woff, flags=("no-FFTM-table",))
    else:
        print(f"** Writing {output_cpp}")
        font = CFontWriter(compositions)
        with open(output_cpp, "w", encoding='utf-8') as dst:
            dst.write(header)
            font.generate_c_output(cp_min, dst)
            font.print_stats(dst)

    print("** Done")
