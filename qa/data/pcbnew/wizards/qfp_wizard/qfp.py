#!/usr/bin/env python3

# Copyright The KiCad Developers
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from typing import Optional

from kipy.board_types import (
    BoardLayer,
    PadType,
    PadStackShape,
    BoardRectangle,
    BoardSegment,
    BoardText,
    FootprintInstance,
    Pad,
)
from kipy.geometry import Angle, Vector2
from kipy.util.units import from_mm, to_mm
from kipy.wizards import (
    WizardBase,
    WizardContentType,
    WizardInfo,
    WizardMetaInfo,
    WizardParameter,
    WizardParameterCategory,
    WizardParameterDataType,
)

def _parameter_definitions() -> list[WizardParameter]:
    return [
        WizardParameter.create(
            "n",
            "Pad Count",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_INTEGER,
            32,
            min_value=4,
            multiple=4,
            description="Total number of pads (must be a multiple of 4)"
        ),
        WizardParameter.create(
            "e",
            "Pad Pitch",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(0.8),
            min_value=from_mm(0.1),
            max_value=from_mm(25)
        ),
        WizardParameter.create(
            "X1",
            "Pad Width",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(0.55),
        ),
        WizardParameter.create(
            "Y1",
            "Pad Length",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(1.5),
        ),
        WizardParameter.create(
            "C1",
            "Horizontal spacing",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(8.4),
        ),
        WizardParameter.create(
            "C2",
            "Vertical spacing",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(8.4),
        ),
        WizardParameter.create(
            "oval",
            "Oval Pads",
            WizardParameterCategory.WPC_PADS,
            WizardParameterDataType.WPDT_BOOL,
            True,
        ),
        WizardParameter.create(
            "D1",
            "Overall Width",
            WizardParameterCategory.WPC_PACKAGE,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(7),
        ),
        WizardParameter.create(
            "E1",
            "Overall Height",
            WizardParameterCategory.WPC_PACKAGE,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(7),
        ),
        WizardParameter.create(
            "courtyard_margin",
            "Courtyard Margin",
            WizardParameterCategory.WPC_PACKAGE,
            WizardParameterDataType.WPDT_DISTANCE,
            from_mm(0.25),
            min_value=from_mm(0.2),
        ),
        WizardParameter.create(
            "ref_prefix",
            "Reference Prefix",
            WizardParameterCategory.WPC_METADATA,
            WizardParameterDataType.WPDT_STRING,
            "U",
            validation_regex="^[a-zA-Z]+$"
        ),
    ]


class QfpParameters:
    def __init__(self, incoming: Optional[list[WizardParameter]] = None):
        definitions = _parameter_definitions()
        self._params = {param.identifier: param for param in definitions}

        if incoming is not None:
            for param in incoming:
                target = self._params.get(param.identifier)
                if target is None:
                    continue

                value = param.value
                if value is not None:
                    target.value = value

        self._validate()

    @classmethod
    def for_info(cls) -> list[WizardParameter]:
        return _parameter_definitions()

    def _int(self, key: str) -> int:
        value = self._params[key].value
        if isinstance(value, bool) or not isinstance(value, int):
            raise TypeError(f"{key} must be int, got {type(value).__name__}")
        return value

    def _bool(self, key: str) -> bool:
        value = self._params[key].value
        if not isinstance(value, bool):
            raise TypeError(f"{key} must be bool, got {type(value).__name__}")
        return value

    def _str(self, key: str) -> str:
        value = self._params[key].value
        if not isinstance(value, str):
            raise TypeError(f"{key} must be str, got {type(value).__name__}")
        return value

    @property
    def n(self) -> int:
        return self._int("n")

    @property
    def e(self) -> int:
        return self._int("e")

    @property
    def x1(self) -> int:
        return self._int("X1")

    @property
    def y1(self) -> int:
        return self._int("Y1")

    @property
    def c1(self) -> int:
        return self._int("C1")

    @property
    def c2(self) -> int:
        return self._int("C2")

    @property
    def d1(self) -> int:
        return self._int("D1")

    @property
    def e1(self) -> int:
        return self._int("E1")

    @property
    def courtyard_margin(self) -> int:
        return self._int("courtyard_margin")

    @property
    def oval(self) -> bool:
        return self._bool("oval")

    @property
    def ref_prefix(self) -> str:
        return self._str("ref_prefix")

    def _validate(self):
        if self.n < 4 or self.n % 4 != 0:
            raise ValueError("Parameter 'n' must be >= 4 and a multiple of 4")

        if self.courtyard_margin < from_mm(0.2):
            raise ValueError("Parameter 'courtyard_margin' must be >= 0.2mm")


class QfpWizard(WizardBase):
    def __init__(self):
        super().__init__(description="QFP footprint wizard")

    def build_wizard_info(self) -> WizardInfo:
        info = WizardInfo()
        info.meta = WizardMetaInfo()
        info.meta.identifier = "org.kicad.generators.qfp.qfp"
        info.meta.name = "QFP"
        info.meta.description = "Quad Flat Package (QFP) footprint wizard"
        info.meta.types_generated = [WizardContentType.WCT_FOOTPRINT]
        info.parameters = QfpParameters.for_info()
        return info

    def build_generated_content(
        self, parameters: Optional[list[WizardParameter]] = None
    ) -> FootprintInstance:
        params = QfpParameters(parameters)

        pad_count = params.n
        pad_pitch = params.e
        pad_width = params.x1
        pad_length = params.y1
        h_pitch = params.c1
        v_pitch = params.c2
        body_width = params.d1
        body_height = params.e1
        courtyard_margin = params.courtyard_margin
        oval_pads = params.oval

        footprint = FootprintInstance()
        footprint.layer = BoardLayer.BL_F_Cu
        footprint.definition.id.name = QfpWizard._footprint_value(params)

        pads_per_row = pad_count // 4
        row_len = (pads_per_row - 1) * pad_pitch
        half_row_len = row_len // 2

        pad_positions: list[tuple[int, Vector2, bool]] = []

        for i in range(pads_per_row):
            y = -half_row_len + i * pad_pitch
            pad_positions.append((i + 1, Vector2.from_xy(-h_pitch // 2, y), True))

        for i in range(pads_per_row):
            x = -half_row_len + i * pad_pitch
            pad_positions.append((pads_per_row + i + 1, Vector2.from_xy(x, v_pitch // 2), False))

        for i in range(pads_per_row):
            y = half_row_len - i * pad_pitch
            pad_positions.append((2 * pads_per_row + i + 1, Vector2.from_xy(h_pitch // 2, y), True))

        for i in range(pads_per_row):
            x = half_row_len - i * pad_pitch
            pad_positions.append((3 * pads_per_row + i + 1, Vector2.from_xy(x, -v_pitch // 2), False))

        for number, position, rotate_90 in pad_positions:
            footprint.definition.add_item(
                QfpWizard._build_pad(number, position, pad_width, pad_length, oval_pads, rotate_90)
            )

        offset = from_mm(0.15)
        x = body_width // 2 + offset
        y = body_height // 2 + offset
        inner = half_row_len + pad_pitch

        bevel = min(from_mm(1.0), body_width // 2, body_height // 2)
        left = -body_width // 2
        right = body_width // 2
        top = -body_height // 2
        bottom = body_height // 2

        fab_outline = [
            (left + bevel, top),
            (right, top),
            (right, bottom),
            (left, bottom),
            (left, top + bevel),
            (left + bevel, top),
        ]
        QfpWizard._add_polyline(footprint, BoardLayer.BL_F_Fab, from_mm(0.1), fab_outline)

        right_edge = (h_pitch + pad_length) // 2
        left_edge = -right_edge
        bottom_edge = (v_pitch + pad_length) // 2

        QfpWizard._add_polyline(
            footprint,
            BoardLayer.BL_F_SilkS,
            from_mm(0.12),
            [(-inner, -y), (-x, -y), (-x, -inner), (left_edge, -inner)],
        )
        QfpWizard._add_polyline(
            footprint,
            BoardLayer.BL_F_SilkS,
            from_mm(0.12),
            [(inner, -y), (x, -y), (x, -inner)],
        )
        QfpWizard._add_polyline(
            footprint,
            BoardLayer.BL_F_SilkS,
            from_mm(0.12),
            [(-inner, y), (-x, y), (-x, inner)],
        )
        QfpWizard._add_polyline(
            footprint,
            BoardLayer.BL_F_SilkS,
            from_mm(0.12),
            [(inner, y), (x, y), (x, inner)],
        )

        courtyard_x = QfpWizard._round_to_grid((right_edge + courtyard_margin) * 2, from_mm(0.1))
        courtyard_y = QfpWizard._round_to_grid((bottom_edge + courtyard_margin) * 2, from_mm(0.1))

        courtyard = BoardRectangle()
        courtyard.layer = BoardLayer.BL_F_CrtYd
        courtyard.top_left = Vector2.from_xy(-courtyard_x // 2, -courtyard_y // 2)
        courtyard.bottom_right = Vector2.from_xy(courtyard_x // 2, courtyard_y // 2)
        courtyard.attributes.stroke.width = from_mm(0.05)
        footprint.definition.add_item(courtyard)

        text_size = from_mm(1.0)
        text_thickness = from_mm(0.15)
        text_offset = v_pitch // 2 + text_size + pad_length // 2
        attrs = QfpWizard._text_attributes(text_size, text_thickness)

        footprint.reference_field.text.value = f"{params.ref_prefix}?"
        footprint.reference_field.text.layer = BoardLayer.BL_F_SilkS
        footprint.reference_field.text.position = Vector2.from_xy(0, -text_offset)
        footprint.reference_field.text.attributes = attrs

        footprint.value_field.text.value = QfpWizard._footprint_value(params)
        footprint.value_field.text.layer = BoardLayer.BL_F_Fab
        footprint.value_field.text.position = Vector2.from_xy(0, text_offset)
        footprint.value_field.text.attributes = attrs

        extra_text = BoardText()
        extra_text.layer = BoardLayer.BL_F_Fab
        extra_text.position = Vector2.from_xy(0, 0)
        extra_text.value = "${REFERENCE}"
        extra_text.attributes = attrs
        footprint.definition.add_item(extra_text)

        return footprint

    @staticmethod
    def _footprint_value(params: QfpParameters) -> str:
        return "QFP-{n}_{x:g}x{y:g}_Pitch{p:g}mm".format(
            n=params.n,
            x=to_mm(params.d1),
            y=to_mm(params.e1),
            p=to_mm(params.e),
        )

    @staticmethod
    def _round_to_grid(value_nm: int, grid_nm: int) -> int:
        return ((value_nm + grid_nm // 2) // grid_nm) * grid_nm

    @staticmethod
    def _text_attributes(size_nm: int, thickness_nm: int):
        attrs = BoardText().attributes
        attrs.size = Vector2.from_xy(size_nm, size_nm)
        attrs.stroke_width = thickness_nm
        return attrs

    @staticmethod
    def _add_segment(
        footprint: FootprintInstance,
        layer: BoardLayer.ValueType,
        width_nm: int,
        start: tuple[int, int],
        end: tuple[int, int],
    ):
        segment = BoardSegment()
        segment.layer = layer
        segment.start = Vector2.from_xy(*start)
        segment.end = Vector2.from_xy(*end)
        segment.attributes.stroke.width = width_nm
        footprint.definition.add_item(segment)

    @staticmethod
    def _add_polyline(
        footprint: FootprintInstance,
        layer: BoardLayer.ValueType,
        width_nm: int,
        points: list[tuple[int, int]],
    ):
        for start, end in zip(points, points[1:]):
            QfpWizard._add_segment(footprint, layer, width_nm, start, end)

    @staticmethod
    def _build_pad(
        number: int,
        position: Vector2,
        pad_width: int,
        pad_length: int,
        is_oval: bool,
        rotate_90: bool,
    ) -> Pad:
        pad = Pad()
        pad.number = str(number)
        pad.pad_type = PadType.PT_SMD
        pad.position = position
        pad.padstack.layers = [
            BoardLayer.BL_F_Cu,
            BoardLayer.BL_F_Paste,
            BoardLayer.BL_F_Mask,
        ]

        copper = pad.padstack.copper_layer(BoardLayer.BL_F_Cu)
        assert copper is not None
        copper.shape = PadStackShape.PSS_OVAL if is_oval else PadStackShape.PSS_RECTANGLE
        copper.size = Vector2.from_xy(pad_width, pad_length)

        if rotate_90:
            pad.padstack.angle = Angle.from_degrees(90.0)

        return pad

if __name__ == "__main__":
    wizard = QfpWizard()
    wizard.run()
