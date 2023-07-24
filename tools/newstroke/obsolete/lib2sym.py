#!/usr/bin/python

"""
Converts 5.1 font .lib files to .kicad_sym.
Deals with "rescaled" libraries that have non-integer coordinates,
such as CKJ_wide.lib.

This is not a general purpose converter - it's only meant to
deal with font files.

Usage: lib2sym.py
"""

import re
from dataclasses import dataclass, field
from typing import Any, ClassVar, TextIO

ROTATIONS = {'R': 0, 'U': 90, 'L': 180, 'D': 270}
VISIBILITIES = {'V': True, 'I': False}
ORIENTATIONS = {'H': 0, 'V': 90}
FILLTYPES = {'N': 'none'}
PINETYPES = {'I': 'input', 'O': 'output'}
PINSHAPES = {
    '': 'line',
    'I': 'inverted',
    'C': 'clock',
    'CI': 'inverted_clock',
    'L': 'input_low',
    'CL': 'clock_low',
    'F': 'falling_clock',
    'X': 'non_logic'}
PINHIDDEN = 'N'


def mil_to_mm(mil: float):
    return round(mil * 0.0254, 6)


def npairwise(iterable):
    args = (iter(iterable),) * 2
    return list(zip(*args))


class SexprWriter:
    def __init__(self, stream: TextIO):
        self.indent = 0
        self.stream = stream
        self.space = ""
        self.dedents: list[bool] = []

    def _indent(self) -> None:
        self.stream.write(" " * 2 * self.indent)

    def _newline(self) -> None:
        self.stream.write("\n")

    def _space(self) -> None:
        self.stream.write(self.space)
        self.space = ""

    def group(self, key: Any, /, items=None, newline=False, indent=False):
        self.startgroup(key, items, newline, indent)
        self.endgroup(newline=False)

    def startgroup(self, key: Any, /, items=None, newline=False, indent=False):
        self.dedents.append(indent)
        if indent:
            self.indent += 1
        if newline:
            self._newline()
            self._indent()
        else:
            self._space()
        self.stream.write("(")
        if key:
            self.stream.write(str(key))
            self.space = " "
        if items:
            for item in items:
                self.additem(item)

    def endgroup(self, newline=True):
        dedent = self.dedents.pop()
        if newline:
            self._newline()
            self._indent()
            self.space = ""
        else:
            self.space = " "
        if dedent:
            if self.indent > 0:
                self.indent -= 1
        self.stream.write(")")

    def additem(self, item: Any):
        self._space()
        if item == 0:
            self.stream.write("0")
        else:
            self.stream.write(str(item))
        self.space = " "


@dataclass
class Point:
    x: float
    y: float

    @classmethod
    def new_mil(cls, x: float, y: float) -> 'Point':
        return cls(mil_to_mm(x), mil_to_mm(y))

    def write(self, to: SexprWriter):
        to.group("xy", [self.x, self.y], newline=True, indent=True)


@dataclass
class TextEffect:
    size: float
    is_hidden: bool = False

    @classmethod
    def new_mil(cls, size: float) -> 'TextEffect':
        return cls(mil_to_mm(size))

    def write(self, to: SexprWriter, newline=True):
        if self:
            to.startgroup("effects", newline=newline, indent=True)
            to.startgroup("font")
            to.group("size", [self.size, self.size])
            to.endgroup(newline=False)
            if self.is_hidden:
                to.additem("hide")
            to.endgroup(newline=False)


@dataclass
class Field:
    name: str
    value: str
    posx: float
    posy: float
    rotation: float
    effects: TextEffect

    _pattern: ClassVar[re.Pattern] = re.compile(
        r'^\s*F(?P<n>\d+)\s+"(?P<value>[^"]*)"\s+(?P<rest>.+)$')

    @staticmethod
    def valid(line: str):
        return Field._pattern.match(line) is not None

    @classmethod
    def new_v5(cls, line: str) -> 'Field':
        match = Field._pattern.match(line)
        assert match is not None

        n = int(match.group("n"))
        name = ['Reference', 'Value', 'Footprint', 'Datasheet'][n]
        value = match.group("value")
        rest = match.group("rest").split()

        x = mil_to_mm(float(rest[0]))
        y = mil_to_mm(float(rest[1]))
        dimension = int(rest[2])
        rotation = ORIENTATIONS[rest[3]]
        visibility = VISIBILITIES[rest[4]]

        effects = TextEffect.new_mil(dimension)
        effects.is_hidden = not visibility
        return cls(name, value, x, y, rotation, effects)

    def write(self, to: SexprWriter):
        to.startgroup(
            "property", [
                f'"{self.name}"', f'"{self.value}"'], newline=True, indent=True)
        to.group("at", [self.posx, self.posy, self.rotation])
        self.effects.write(to)
        to.endgroup()


@dataclass
class Polyline:
    points: list[Point]
    stroke_width: float
    fill_type: str

    @staticmethod
    def valid(line: str):
        return line.startswith('P ')

    @classmethod
    def new_v5(cls, line: str) -> 'Polyline':
        tokens = line.split()

        points: list[Point] = []
        _1, _2, _3, stroke_width = tokens[1:5]
        fill_type = tokens[-1]

        for x, y in npairwise(tokens[5:]):
            points.append(Point.new_mil(float(x), float(y)))

        return cls(points,
                   mil_to_mm(int(stroke_width)),
                   FILLTYPES[fill_type]
                   )

    def write(self, to: SexprWriter):
        to.startgroup("polyline", newline=True, indent=True)

        to.startgroup("pts", newline=True, indent=True)
        for point in self.points:
            point.write(to)
        to.endgroup()

        to.startgroup("stroke", newline=True, indent=True)
        to.group("width", [self.stroke_width])
        to.group("type", ["solid"])
        to.endgroup(newline=False)

        to.startgroup("fill", newline=True, indent=True)
        to.group("type", [self.fill_type])
        to.endgroup(newline=False)

        to.endgroup()


@dataclass
class Pin:
    name: str
    number: str
    etype: str
    name_effect: TextEffect
    number_effect: TextEffect
    posx: float = 0.0
    posy: float = 0.0
    rotation: int = 0
    shape: str = "line"
    length: float = 2.54
    is_hidden: bool = False

    @staticmethod
    def valid(line: str):
        return line.startswith('X ')

    @classmethod
    def new_v5(cls, line: str) -> 'Pin':
        tokens = line.split()

        name, number, x, y, length, orientation, numsize, namesize, _1, _2, etype = tokens[
            1:12]
        shape = tokens[12] if len(tokens) > 12 else ''
        rotation = ROTATIONS[orientation]
        etype = PINETYPES[etype]
        if is_hidden := shape.startswith(PINHIDDEN):
            shape = shape[len(PINHIDDEN):]

        pin = cls(name, number, etype,
                  name_effect=TextEffect.new_mil(int(namesize)),
                  number_effect=TextEffect.new_mil(int(numsize))
                  )
        pin.posx = mil_to_mm(float(x))
        pin.posy = mil_to_mm(float(y))
        pin.shape = PINSHAPES[shape]
        pin.length = mil_to_mm(float(length))
        pin.is_hidden = is_hidden
        pin.rotation = rotation
        return pin

    def write(self, to: SexprWriter):
        to.startgroup("pin", [self.etype, self.shape],
                      newline=True, indent=True)

        to.group("at", [self.posx, self.posy, self.rotation])
        to.group("length", [self.length])

        to.startgroup("name", [f'"{self.name}"'], newline=True, indent=True)
        self.name_effect.write(to, newline=False)
        to.endgroup(newline=False)

        to.startgroup("number", [f'"{self.number}"'],
                      newline=True, indent=True)
        self.number_effect.write(to, newline=False)
        to.endgroup(newline=False)

        to.endgroup(newline=True)


@dataclass
class Symbol:
    name: str
    pin_names_offset: float
    properties: list[Field] = field(default_factory=list)
    pins: list[Pin] = field(default_factory=list)
    polylines: list[Polyline] = field(default_factory=list)

    @staticmethod
    def valid(line: str):
        return line.startswith("DEF ")

    @staticmethod
    def final(line: str):
        return line.startswith("ENDDEF")

    @classmethod
    def new_v5(cls, line: str) -> 'Symbol':
        tokens = line.split()
        name = tokens[1]
        offset = mil_to_mm(int(tokens[4]))
        return cls(name, offset)

    @property
    def has_contents(self):
        return bool(self.pins) | bool(self.polylines)

    def write(self, to: SexprWriter):
        to.startgroup("symbol", [f'"{self.name}"'], newline=True, indent=True)

        to.startgroup("pin_names")
        to.group("offset", [self.pin_names_offset])
        to.endgroup(newline=False)

        to.group("in_bom", ["yes"])
        to.group("on_board", ["yes"])

        for prop in self.properties:
            prop.write(to)

        if self.has_contents:
            to.startgroup(
                "symbol", [f'"{self.name}_0_1"'], newline=True, indent=True)
            for poly in self.polylines:
                poly.write(to)
            for pin in self.pins:
                pin.write(to)
            to.endgroup()

        to.endgroup()

    def getheft(self):
        """A good heuristic for how much space this symbol's sexprs take"""
        heft = 1 + 2*len(self.properties) + 3*len(self.pins)
        for poly in self.polylines:
            heft += 1 + len(poly.points)
        return heft


def translate(libfilename: str, symfilebase: str, symfileext: str, split=False):

    fo: Optional[TextIO] = None
    to: Optional[SexprWriter] = None
    part = ""

    def closeOutput():
        nonlocal fo, to
        if fo:
            to.endgroup()
            fo.write('\n')
            fo.close()
            fo = None

    def newOutput():
        nonlocal fo, to, part

        fo = open(f"{symfilebase}{part}{symfileext}", 'w', encoding="utf-8")
        to = SexprWriter(fo)
        to.startgroup("kicad_symbol_lib")
        to.group("version", ["20220914"])
        to.group("generator", ["font_lib2sym"])

    with open(libfilename, encoding="utf-8") as fi:

        linecount = 0
        totalheft = 0
        heftperfile = 175_000  # just shy of 10MB
        sym: Symbol

        while line := fi.readline():
            linecount += 1

            if Polyline.valid(line):
                polyline = Polyline.new_v5(line)
                sym.polylines.append(polyline)

            elif Pin.valid(line):
                pin = Pin.new_v5(line)
                sym.pins.append(pin)

            elif Field.valid(line):
                prop = Field.new_v5(line)
                sym.properties.append(prop)

            elif line.startswith('#'):
                pass

            elif Symbol.valid(line):
                sym = Symbol.new_v5(line)

            elif Symbol.final(line):
                if not fo or (split and totalheft > heftperfile):
                    if split:
                        part = f'_{sym.name}'
                    closeOutput()
                    newOutput()
                    totalheft = 0

                sym.write(to)
                totalheft += sym.getheft()
                if linecount & 127 == 1:
                    print(sym.name)

            elif line.startswith("DRAW") or line.startswith("ENDDRAW") or line.startswith("EESchema-LIBRARY"):
                pass

            elif line.strip():
                raise RuntimeError(f"Unknown line contents: {line}")

        closeOutput()


if __name__ == "__main__":
    fonts = ['symbol', 'hiragana', 'katakana',
             'half_full', 'font', 'CJK_symbol', 'CKJ_wide:split']

    for font in fonts:
        split = False
        out = font.replace("CKJ", "CJK")
        if font.endswith(":split"):
            font = font[:-6]
            out = out[:-6]
            split = True

        translate(f'{font}.lib', out, '.kicad_sym', split)
