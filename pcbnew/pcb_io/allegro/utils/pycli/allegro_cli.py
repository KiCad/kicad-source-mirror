#! /usr/bin/env python3
"""
This program source code file is part of KiCad, a free EDA CAD application.

Copyright The KiCad Developers, see AUTHORS.txt for contributors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, you may find one here:
http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
or you may search the http://www.gnu.org website for the version 2 license,
or you may write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
"""

import argparse
import enum
import struct

import kaitaistruct
import tabulate
from pathlib import Path

from kaitaistruct import KaitaiStream
import allegro_brd


class Coords:
    """
    Same interface as coords data
    """
    def __init__(self, x, y):
        self.x = x
        self.y = y

class AllegroBoard:

    def __init__(self, kt_brd: allegro_brd.AllegroBrd):
        self.kt_brd = kt_brd

        self.keys = self._build_key_map()
        self.strs = self._build_string_map()

    def _build_key_map(self):
        """
        Build a key map for the Allegro board file.
        The key map is a dictionary that maps the keys to the objects in the
        Allegro board file.
        """
        keys = {}
        for obj in self.kt_brd.objects:
            if hasattr(obj.data, "key"):
                keys[obj.data.key] = obj

        return keys

    def _build_string_map(self):
        """
        Build a string map for the Allegro board file.
        The string map is a dictionary that maps the keys to the strings in the
        Allegro board file.
        """
        strings = {}
        for obj in self.kt_brd.string_map.entries:
            strings[obj.string_id] = obj.value

        return strings

    def object(self, key: int) -> allegro_brd.AllegroBrd.BoardObject:
        """
        Get an object from the Allegro board file by key.
        """
        if key in self.keys:
            return self.keys[key]
        else:
            raise KeyError(f"Key {key:#01x} not found in Allegro board file.")

    def string(self, key: int) -> str:
        """
        Get a string from the Allegro board file by key.
        """
        if key in self.strs:
            return self.strs[key]
        else:
            raise KeyError(f"Key {key:#01x} not found in Allegro board file.")

    class Printer:

        def __init__(self, board, indent: int):
            self.board = board
            self.indent = indent

        def prnt(self, string: str):
            print(" " * self.indent + string)

        @staticmethod
        def obj_type(t) -> str | None:
            ts = {
                0x01: "ARC",
                0x03: "DATA_ITEM?",
                0x04: "NET_ASSIGNMENT?",
                0x05: "TRACK",
                0x07: "COMPONENT_INST",
                0x08: "PIN_NUMBER",
                0x0C: "FIGURE",
                0x0D: "PAD",
                0x0F: "SLOT",
                0x10: "FUNCTION",
                0x11: "PIN_NAME",
                0x14: "GRAPHIC_SEGMENT",
                0x15: "LINE",
                0x16: "LINE",
                0x17: "LINE",
                0x1b: "NET",
                0x1c: "PADSTACK",
                0x23: "RATLNE",
                0x24: "RECT",
                0x2B: "FP_DEF",
                0x2C: "GROUP",
                0x2D: "FP_INST",
                0x30: "STR_GRAPHIC",
                0x32: "PLACED_PAD",
                0x37: "GROUP_ENTRIES",
                0x38: "FILM",
                0x39: "FILM_LAYER_LIST",
                0x3A: "FILM_LAYER",
                0x3C: "DIMENSION_LINK?",
            }

            if t in ts:
                return ts[t]
            return None

        @staticmethod
        def string_aligned_to_str(sa) -> str:
            bs = bytearray(sa.chars[:-1]) # remove null terminator
            value = bs.decode("utf-8", errors="ignore")
            return value

        def _value_or_by_attr(self, struct_or_v, attr_name):

            if isinstance(struct_or_v, kaitaistruct.KaitaiStruct):
                if hasattr(struct_or_v, attr_name):
                    value = getattr(struct_or_v, attr_name)
                else:
                    return
            else:
                value = struct_or_v

            return value

        def field_name(self, hdr1, hdr2):
            # if hdr2 actually needed to distinguish?
            v = (hdr1, hdr2)

            if v == (0x37, 0x04):
                return "LOGICAL_PATH"
            if v == (0x55, 0x00):
                return "NET_MIN_LINE_WIDTH"
            elif v == (0x173, 0x00):
                return "NET_MAX_LINE_WIDTH"
            elif v == (0x5c, 0x00):
                return "NET_MIN_NECK_WIDTH"
            elif v == (0x1fb, 0x00):
                # Not clear what the key is supposed to be here
                return "NET_MAX_NECK_LENGTH(?)"

            return None


        def get_obj_peek_data(self, key_val) -> str:
            """
            Get a short string representation of the object pointed to by key_val.
            """
            obj = self.board.object(key_val)

            if obj_type := self.obj_type(obj.type):
                type_str = f"{obj.type:#04x}: {obj_type}"
            else:
                type_str = f"{obj.type:#04x}"

            value_detail = None

            if obj.type == 0x03:
                if obj.data.subtype in [0x68, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x78]:
                    value_detail = f"'{self.string_aligned_to_str(obj.data.data)}'"
                elif obj.data.subtype in [0x64, 0x66, 0x67, 0x6A]:
                    # U32 data?
                    value_detail = f"{obj.data.data.val:#010x}"
                else:
                    value_detail = "<unknown>"

                type_str = f"{type_str}"

                if field_name := self.field_name(obj.data.unknown_hdr1, obj.data.unknown_hdr2):
                    type_str += f", {field_name}"

                type_str += f", subtype: {obj.data.subtype:#04x}, hdr1: {obj.data.unknown_hdr1:#04x}, hdr2: {obj.data.unknown_hdr2:#04x}"

            elif obj.type == 0x07:
                value_detail = f"'{self.board.string(obj.data.ref_des_ref)}'"
            elif obj.type == 0x08:
                str_ptr = obj.data.str_ptr_16x if hasattr(obj.data, "str_ptr_16x") else obj.data.str_ptr
                value_detail = f"'{self.board.string(str_ptr)}'"
            elif obj.type == 0x0D:
                value_detail = f"'{self.board.string(obj.data.ptr_pad_num)}'"
            elif obj.type == 0x0F:  # Slot
                value_detail = f"'{self.board.string(obj.data.slot_name)}'"
            elif obj.type == 0x11:
                value_detail = f"'{self.board.string(obj.data.name)}'"
            elif obj.type == 0x1B:
                value_detail = f"'{self.board.string(obj.data.net_name)}'"
            elif obj.type == 0x2d:
                refdes_key = obj.data.inst_ref_16x if hasattr(obj.data, "inst_ref_16x") else obj.data.inst_ref

                if refdes_key == 0:
                    value_detail = "'*'"
                else:
                    refdes = self.board.object(refdes_key)
                    value_detail = f"'{self.board.string(refdes.data.ref_des_ref)}'"

            elif obj.type == 0x30:
                str_graph = self.board.object(obj.data.str_graphic_ptr)

                value_detail = self.string_aligned_to_str(str_graph.data.value)

            val = ""
            if value_detail is not None:
                val += "=> " + value_detail + " "

            val += f"({type_str})"
            return val

        def print_x03_chain(self, start_key: int, end_key: int):
            """
            Print the 0x03 chain starting at start_key and ending at end_key (exclusive).
            """
            next_key = start_key
            while next_key != end_key:
                next_obj = self.board.object(next_key)
                self.prnt(f"  {next_key:#010x} {self.get_obj_peek_data(next_key)}")
                next_key = next_obj.data.next

        def print_ptr(self, name, struct_or_key: int | kaitaistruct.KaitaiStruct, attr_name: str | None = None):

            attr_name = attr_name or name

            key_val = self._value_or_by_attr(struct_or_key, attr_name)

            # Doesn't exist
            if key_val is None:
                return

            try:
                val = f"{key_val:#010x}"

                peek_data = self.get_obj_peek_data(key_val)
                val += " " + peek_data

                self.prnt(f"{name:16}: {val}")
            except KeyError:
                self.prnt(f"{name:16}: {key_val:#010x} (not found)")
                return

        def print_type(self, name, t: int):
            if obj_type := self.obj_type(t):
                type_str = f"{obj_type}"
            else:
                type_str = "Unknown"

            self.prnt(f"{name:16}: {t:#04x} ({type_str})")


        def print_v(self, name, block_or_value, attr_name: str | None = None, as_hex: bool = True):
            def cfp_to_double(cfp):
                high_32 = cfp.a
                low_32 = cfp.b
                combined = (high_32 << 32) | (low_32 & 0xFFFFFFFF)
                packed = struct.pack('<Q', combined)  # Little-endian
                double, = struct.unpack('<d', packed)  # Little-endian
                return double

            # if the caller didn't give an attr name, just use the name
            attr_name = attr_name or name

            if isinstance(block_or_value, allegro_brd.AllegroBrd.StringAligned):
                value = self.string_aligned_to_str(block_or_value)
            else:
                value = self._value_or_by_attr(block_or_value, attr_name)

            if value is None:
                return

            if isinstance(value, allegro_brd.AllegroBrd.CadenceFp):
                self.prnt(f"{name:16}: {cfp_to_double(value)}")
            elif isinstance(value, int) and as_hex:
                self.prnt(f"{name:16}: {value:#x}")
            else:
                self.prnt(f"{name:16}: {value}")

        def print_bytes(self, name, byte_array: bytes):
            """
            Print a byte array as hex
            """
            hex_str = " ".join(f"{b:02x}" for b in byte_array)
            self.prnt(f"{name:16}: {hex_str}")

        def print_s(self, name, struct_or_sid: int, attr_name: str | None = None):
            """
            Print a string by ID
            """

            attr_name = attr_name or name

            string_id = self._value_or_by_attr(struct_or_sid, attr_name)

            if string_id is None:
                return

            try:
                string_val = self.board.string(string_id)
                self.prnt(f"{name:16}: '{string_val}' (key: {string_id:#010x})")
            except KeyError:
                self.prnt(f"{name:16}: {string_id:#010x} (not found)")

        def print_coords(self, name, coords):
            self.prnt(f"{name:16}: ({coords.x}, {coords.y})")


        @staticmethod
        def format_layer(layer: allegro_brd.AllegroBrd.LayerInfo):
            if isinstance(layer.lclass, enum.Enum):
                lc = f"{layer.lclass} ({layer.lclass.value:#04x})"
            else:
                lc = f"Unknown ({layer.lclass:#04x})"

            if isinstance(layer.subclass, int):
                sc = f"Unknown ({layer.subclass:#04x})"
            else:
                # it's a known SC enum, but not a known value
                if isinstance(layer.subclass.sc, int):
                    sc = f"Unknown ({layer.subclass.sc:#04x})"
                else:
                    sc = f"{layer.subclass.sc} ({layer.subclass.sc.value:#04x})"

            v = f"{lc} / {sc}"
            return v

        def print_layer(self, layer: allegro_brd.AllegroBrd.LayerInfo):
            name = "Layer"
            self.print_v(name, self.format_layer(layer))

    def print_obj(self, obj, indent=2):
        """
        Print some object as best as possible.
        """
        prntr = self.Printer(self, indent)

        t = obj.type
        d = obj.data

        prntr.print_type(f"Object type", t)

        if hasattr(d, "key"):
            prntr.print_v("Key", d, "key")

        if t == 0x01:
            prntr.print_v("t1", d)
            prntr.print_v("t2", d)
            prntr.print_v("subtype", d)

            prntr.print_ptr("next", d)
            prntr.print_ptr("parent", d)
            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("width", d, as_hex=False)

            prntr.print_coords("P0", d.coords_0)
            prntr.print_coords("P1", d.coords_1)
            prntr.print_v("x", d)
            prntr.print_v("y", d)
            prntr.print_v("r", d)

        elif t == 0x03:
            prntr.print_v("subtype", d)
            prntr.print_v("unknown_hdr1", d)
            prntr.print_v("unknown_hdr2", d)
            prntr.print_v("size", d)

            prntr.print_ptr("next", d)
            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

            if d.subtype in [0x68, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x78]:
                prntr.print_v("string data", d.data)
            elif d.subtype in [0x64, 0x66, 0x67, 0x6A]:
                prntr.print_v("u32 data", d.data.val, as_hex=True)
            elif d.subtype in [0x6c]:
                prntr.print_v("num_entries", d.data)
                for( i, entry) in enumerate(d.data.entries):
                    prntr.print_v(f" - Entry {i}", entry, as_hex=True)

            elif d.subtype in [0x73]:
                # binary data?
                prntr.print_bytes("data", d.data.chars)

        elif t == 0x04:
            prntr.print_ptr("next", d)
            prntr.print_ptr("net", d)
            prntr.print_ptr("conn_item", d)
            prntr.print_ptr("unknown_1", d)

        elif t == 0x05:
            prntr.print_ptr("next", d)
            prntr.print_layer(d.layer)
            prntr.print_ptr("first_segment", d)
            prntr.print_ptr("net_assignment", d)

            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("unknown_5a", d)
            prntr.print_v("unknown_5b", d)
            prntr.print_v("unknown_6", d)

            prntr.print_ptr("ptr1", d)
            prntr.print_ptr("ptr2a", d)
            prntr.print_ptr("ptr2b", d)
            prntr.print_ptr("ptr3a", d)
            prntr.print_ptr("ptr3b", d)
            prntr.print_ptr("ptr5", d)


        elif t == 0x06:
            prntr.print_ptr("next", d)
            prntr.print_s("Comp dev type", d.comp_dev_type)
            prntr.print_s("Symbol name", d.sym_name),
            prntr.print_ptr("Ptr refdes", d.ptr_refdes)
            prntr.print_ptr("Ptr Slot", d.ptr_slot)
            prntr.print_ptr("Pin Number", d.ptr_pin_num)
            prntr.print_ptr("Fields", d.ptr_fields)

            prntr.print_v("unknown_1", d)

            # Follow the 0x03 chain back to here
            prntr.print_x03_chain(d.ptr_fields, d.key)

        elif t == 0x07:
            prntr.print_ptr("next", d)
            prntr.print_s("ref_des_ref", d)
            prntr.print_ptr("ptr_fp_inst", d)
            prntr.print_ptr("ptr_function", d)
            prntr.print_ptr("ptr_first_pad", d)
            prntr.print_ptr("ptr_1", d)
            prntr.print_ptr("ptr_3", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

        elif t == 0x08:

            prntr.print_s("Str ptr", d, "str_ptr_16x")
            prntr.print_s("Str ptr", d, "str_ptr")

            prntr.print_ptr("Prev", d, "prev_ptr")
            prntr.print_ptr("Next", d, "next_ptr")
            prntr.print_ptr("Pin name", d, "ptr_pin_name")
            prntr.print_ptr("Ptr 4", d, "ptr4")

            prntr.print_ptr("unknown_1", d)

        elif t == 0x0a:
            prntr.print_layer(d.layer)

        elif t == 0x0c:
            prntr.print_layer(d.layer)
            prntr.print_ptr("next", d)
            # prntr.print_v("rotation", d.rotation)
            prntr.print_v("shape", d)
            prntr.print_v("drill_char", d)
            prntr.print_v("shape_16x", d)
            prntr.print_v("drill_chars", d)

            prntr.print_coords("coords", d.coords)
            prntr.print_coords("size", d.size)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_ptr("unknown_4", d)
            prntr.print_ptr("unknown_5", d)
            prntr.print_v("unknown_6", d)
            prntr.print_v("unknown_7", d)
            prntr.print_v("unknown_8", d)

        elif t == 0x0d:
            prntr.print_ptr("next", d)
            prntr.print_s("ptr_pad_num", d)
            prntr.print_ptr("flags", d)
            prntr.print_ptr("padstack_ptr", d)
            prntr.print_v("rotation", d.rotation / 1000.)

            prntr.print_coords("coords", d.coords)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

        elif t == 0x0f:
            prntr.print_s("slot_name", d.slot_name)
            prntr.print_v("s", d.s)
            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Component", d.ptr_comp)
            prntr.print_ptr("Ptr x11", d.ptr_x11)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x10:
            prntr.print_ptr("Ptr refdes", d.ptr_x07)
            prntr.print_ptr("Ptr x12", d.ptr_x12)
            prntr.print_s("String", d.name)
            prntr.print_ptr("First slot", d.ptr_slots)
            prntr.print_ptr("Fields", d.path_str)

            prntr.print_x03_chain(d.path_str, d.key)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

        elif t == 0x11:
            prntr.print_s("Name", d.name)
            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Pin number", d.ptr_pin_num)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x12:
            prntr.print_ptr("next", d.next)
            prntr.print_ptr("ptr_0x11", d.ptr_0x11)
            prntr.print_ptr("pad_ptr", d.pad_ptr)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

        elif t == 0x14:

            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Parent", d.parent_ptr)
            prntr.print_ptr("Segment", d.segment_ptr)
            prntr.print_ptr("Ptr 0x03", d.ptr_0x03)
            prntr.print_ptr("Ptr x026", d.ptr_0x26)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

            prntr.print_layer(d.layer)

            seg = self.object(d.segment_ptr)

            if seg and seg.type in [0x15, 0x16, 0x17, 0x01]:
                prntr.prnt("Seg info:")
                subprntr = AllegroBoard.Printer(self, indent + 2)
                subprntr.print_coords("coords_0", seg.data.coords_0)
                subprntr.print_coords("coords_1", seg.data.coords_1)

        elif t in [0x15, 0x16, 0x17]:
            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Parent", d.parent_ptr)

            prntr.print_v("flags", d)
            prntr.print_v("Width", d.width, as_hex=False)
            prntr.print_coords("pt0", d.coords_0)
            prntr.print_coords("pt1", d.coords_1)

            prntr.print_v("unknown_1", d)

        elif t == 0x1b: # Net
            prntr.print_ptr("next", d)
            prntr.print_s("Net", d.net_name)

            prntr.print_ptr("Assignments", d.assignments)
            prntr.print_ptr("Ptr2", d.ptr2)
            prntr.print_ptr("Ptr4", d.ptr4)
            prntr.print_ptr("Ptr6", d.ptr6)

            prntr.print_v("Type", d.type)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

            prntr.print_ptr("Fields?", d.path_str_ptr)
            if( d.path_str_ptr != 0):
                prntr.print_x03_chain(d.path_str_ptr, d.key)

        elif t == 0x1c: # Padstack
            prntr.print_ptr("next", d)
            prntr.print_s("Pad str", d.pad_str)
            prntr.print_v("Pad path", d.pad_path)
            prntr.print_v("Layer count", d.layer_count)
            prntr.print_v("Pad type", d.pad_info.pad_type)

            prntr.print_v("drill", d)  # ?
            prntr.print_s("unknown_str", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("unknown_5", d)
            prntr.print_v("unknown_6", d)
            prntr.print_v("unknown_7", d)
            prntr.print_v("unknown_8", d)
            prntr.print_v("unknown_9", d)
            prntr.print_v("unknown_10", d)
            prntr.print_v("unknown_11", d)

            for i, pc in  enumerate(d.components):
                prntr.prnt(f"- Component {i}")

                prntr.print_v(f"  type", pc.t)

                if pc.t == 0x00:
                    continue

                prntr.print_v(f"  w", pc.w, as_hex=False)
                prntr.print_v(f"  h", pc.h, as_hex=False)
                prntr.print_v(f"  x3", pc.x3, as_hex=False)
                prntr.print_v(f"  x4", pc.x4, as_hex=False)
                prntr.print_ptr("  str_ptr", pc.str_ptr)

        elif t == 0x1d:
            prntr.print_ptr("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

            prntr.print_v("size_a", d)
            prntr.print_v("size_b", d)

            # A and B arrays...
            prntr.print_bytes("A array", d.array_a)
            prntr.print_bytes("B array", d.array_b)

        elif t == 0x1e:
            # prntr.print_ptr("next", d)
            prntr.print_v("size", d)
            prntr.print_s("str_ptr", d)
            prntr.print_v("str", d.str)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

        elif t == 0x1f:
            prntr.print_ptr("next", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("size", d)

            # Arrays

        elif t == 0x21:
            prntr.print_v("t", d)
            prntr.print_v("r", d)
            prntr.print_v("size", d)

        elif t == 0x23:
            prntr.print_ptr("next", d)
            prntr.print_layer(d.layer)

            prntr.print_ptr("ptr1", d)
            prntr.print_ptr("ptr2", d)
            prntr.print_ptr("ptr3", d)

            prntr.print_coords("coords_0", d.coords_0)
            prntr.print_coords("coords_1", d.coords_1)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)


        elif t == 0x24: # Rectangle
            prntr.print_layer(d.layer)
            prntr.print_coords("pt0", d.coords_0)
            prntr.print_coords("pt1", d.coords_1)
            prntr.print_coords("(size)", Coords(d.coords_1.x - d.coords_0.x, d.coords_1.y - d.coords_0.y))
            prntr.print_coords("(centre)", Coords((d.coords_0.x + d.coords_1.x) // 2, (d.coords_0.y + d.coords_1.y) // 2))
            prntr.print_ptr("ptr_parent", d.ptr_parent)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x26:
            prntr.print_ptr("member", d.member_ptr)
            prntr.print_ptr("group", d.group_ptr)
            prntr.print_ptr("const", d.const_ptr)

            prntr.print_v("un", d)
            prntr.print_v("un1", d)

        elif t == 0x28: # Polygon?
            prntr.print_layer(d.layer)
            prntr.print_coords("pt0", d.coords_0)
            prntr.print_coords("pt1", d.coords_1)
            prntr.print_ptr("next", d)
            prntr.print_ptr("parent", d)
            prntr.print_ptr("ptr2", d)
            prntr.print_ptr("ptr3", d)
            prntr.print_ptr("ptr4", d)
            prntr.print_ptr("ptr4", d)
            prntr.print_ptr("ptr6", d)
            prntr.print_ptr("ptr7_16x", d)

            prntr.print_ptr("first_seg", d.first_segment_ptr)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("unknown_5", d)

        elif t == 0x2a:
            assert isinstance(d, allegro_brd.AllegroBrd.Type2a)
            prntr.print_v("unknown_1", d)
            prntr.print_v("Entries", d, "num_entries")

            subprntr = self.Printer(self, indent=indent + 2)
            if hasattr(d, "refs"):
                for i, ref in enumerate(d.refs):
                    prntr.prnt(f"- Ref {i}")
                    subprntr.print_s("ptr", ref)
                    subprntr.print_v("properties", ref)
                    subprntr.print_v("un1", ref)
            else:
                for i, entry in enumerate(d.nonrefs):
                    prntr.prnt(f"- Entry {i}")
                    subprntr.print_v("name", entry.name)

        elif t == 0x2b:
            prntr.print_v("subtype", d.subtype)
            prntr.print_ptr("next", d.next)
            prntr.print_s("fp_str", d.fp_str_ref)

            prntr.print_ptr("first_inst_ptr", d.first_inst_ptr)

            prntr.print_coords("coords0", d.coords0)
            prntr.print_coords("coords1", d.coords1)

            prntr.print_ptr("fields", d.fields)

            if d.fields != 0:
                prntr.print_x03_chain(d.fields, d.key)

            prntr.print_ptr("ptr_2", d.ptr_2)
            prntr.print_ptr("ptr_3", d.ptr_3)
            prntr.print_ptr("ptr_4", d.ptr_4)
            prntr.print_ptr("ptr_5", d.ptr_5)
            prntr.print_ptr("ptr_6", d.ptr_6)
            prntr.print_ptr("ptr_7", d.ptr_7)

            prntr.print_v("unknown_1", d.unknown_1)

        elif t == 0x2c:
            prntr.print_ptr("next", d.next)
            prntr.print_v("flags", d.flags)
            prntr.print_ptr("ptr_1", d.ptr_1)
            prntr.print_ptr("ptr_3", d.ptr_3)
            prntr.print_s("subclass_str", d.subclass_str)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

            prntr.print_ptr("ptr_fields", d.ptr_fields)
            if d.ptr_fields != 0:
                prntr.print_x03_chain(d.ptr_fields, d.key)

        elif t == 0x2d:
            prntr.print_ptr("next", d.next)
            prntr.print_ptr("first_pad_ptr", d.first_pad_ptr)
            prntr.print_ptr("ptr_1", d)
            prntr.print_ptr("ptr_2", d)
            prntr.print_ptr("ptr_3", d)
            prntr.print_ptr("ptr_4", d)
            prntr.print_ptr("ptr_5", d)
            prntr.print_ptr("ptr_6", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("flags", d.flags)
            prntr.print_v(f"rotation", d.rotation / 1000., as_hex=False)
            prntr.print_coords("coords", d.coords)

            ref = d.inst_ref if hasattr(d, "inst_ref") else d.inst_ref_16x
            prntr.print_ptr("inst_ref", ref)

        elif t == 0x2E:
            prntr.print_ptr("next", d.next)
            prntr.print_ptr("net_ptr", d)
            prntr.print_ptr("connection", d)
            prntr.print_coords("coords", d.coords)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("unknown_5", d)
        elif t == 0x30:
            assert isinstance(d, allegro_brd.AllegroBrd.Type30StrWrapper)
            prntr.print_ptr("next", d.next)
            prntr.print_ptr("str_graphic_ptr", d.str_graphic_ptr)
            prntr.print_coords("coords", d.coords)
            prntr.print_v("rotation", d.rotation / 1000.)
            prntr.print_layer(d.layer)

            prntr.print_ptr("ptr3", d)
            prntr.print_ptr("ptr3_16x", d)
            prntr.print_ptr("ptr4", d)
            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)
            prntr.print_v("unknown_5", d)

            if hasattr(d, "font"):
                fprops = d.font
            else:
                fprops = d.font_16x

            prntr.print_v("txt props key", fprops.key)
            prntr.print_v("txt props flags", fprops.flags)
            prntr.print_v("txt props align", fprops.alignment)
            prntr.print_v("txt props rev", fprops.reversal)
        elif t == 0x31:
            prntr.print_ptr("str_graphic_wrapper_ptr", d.str_graphic_wrapper_ptr)
            prntr.print_layer(d.layer)
            prntr.print_coords("coords", d.coords)

            prntr.print_v("value", d.value)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x32:
            prntr.print_ptr("next", d)
            prntr.print_ptr("prev", d)
            prntr.print_ptr("next_in_fp", d)
            prntr.print_ptr("parent_fp", d)
            prntr.print_ptr("net_ptr", d)
            prntr.print_ptr("pad_ptr", d)
            prntr.print_ptr("next_in_comp", d)
            prntr.print_ptr("pin_num", d)
            prntr.print_ptr("pad_name_text", d)
            prntr.print_ptr("parent_fp", d)
            prntr.print_ptr("track", d)
            prntr.print_ptr("ratline", d)
            prntr.print_v("flags", d)
            prntr.print_ptr("ptr_x12", d)
            prntr.print_ptr("ptr6", d)

            prntr.print_ptr("unknown_1", d)

            prntr.print_coords("bbox_0", d.bbox_0)
            prntr.print_coords("bbox_1", d.bbox_1)

        elif t == 0x33:
            assert isinstance(d, allegro_brd.AllegroBrd.Type33Via)
            prntr.print_ptr("next", d)
            prntr.print_ptr("net_ptr", d)
            prntr.print_ptr("padstack", d)
            prntr.print_ptr("connection", d)
            prntr.print_ptr("ptr1", d)
            prntr.print_ptr("ptr2", d)
            prntr.print_ptr("ptr3", d)
            prntr.print_ptr("ptr4", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

            prntr.print_coords("pos", d.pos)



        elif t == 0x36:
            assert isinstance(d, allegro_brd.AllegroBrd.Type36)
            prntr.print_v("c", d)
            prntr.print_v("num_items", d)
            prntr.print_v("count", d)

            subprntr = AllegroBoard.Printer(self, indent + 2)
            for i, item in enumerate(d.items):
                prntr.prnt(f"- Item {i}")

                if d.c == 0x08:
                    assert isinstance(item, allegro_brd.AllegroBrd.Type36.X08)
                    subprntr.print_v("char_height", item, as_hex=False)
                    subprntr.print_v("char_width", item, as_hex=False)
                    subprntr.print_v("a", item, as_hex=False)
                    subprntr.print_v("b", item, as_hex=False)
                    subprntr.print_v("xs", item)
                    subprntr.print_v("ys", item)
                    subprntr.print_v("unknown_1", item)
                else:
                    subprntr.prnt(str(item))


        elif t == 0x37:

            prntr.print_ptr("group_ptr", d.group_ptr)

            prntr.print_v("capacity", d.capacity)
            prntr.print_v("count", d.count)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

            for i in range(d.count):
                entry = d.ptrs[i]
                prntr.print_ptr(f"entry {i}", entry)

        elif t == 0x38:
            prntr.print_ptr("next", d)
            prntr.print_ptr("layer_list", d)
            prntr.print_v("film_name", d)
            prntr.print_s("layer_name_str", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

        elif t == 0x39:
            prntr.print_ptr("parent", d)
            prntr.print_ptr("head", d)
            prntr.print_v("x", d)

        elif t == 0x3a:
            assert isinstance(d, allegro_brd.AllegroBrd.Type3aFilmListNode)
            prntr.print_ptr("next", d)
            prntr.print_layer(d.layer)
            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x3c:

            prntr.print_v("unknown_1", d)

            num_entries = d.num_entries
            prntr.print_v("num_entries", num_entries)

            for i in range(num_entries):
                entry = d.entries[i]
                prntr.print_ptr(f"entry {i}", entry)

        else:
            print(f"  Object data: {d}")

class IntIsh:

    def __init__(self, value):

        if isinstance(value, str) and value.startswith("0x"):
            value = int(value, 16)
        else:
            value = int(value)

        self.value = value

    def __int__(self):
        return self.value


def print_obj_counts_by_type(objs):
    """
    Print the object counts by type.
    """
    counts = {}

    for obj in objs:
        obj_type = obj.type
        if obj_type not in counts:
            counts[obj_type] = 0
        counts[obj_type] += 1

    # Print the counts in a table
    table = []
    for obj_type, count in counts.items():
        table.append([f"{obj_type:#04x}", count])

    # sort on type
    table.sort(key=lambda x: x[0])

    total = sum(counts.values())
    if total > 0:
        table.append(tabulate.SEPARATING_LINE)
    table.append(["Total", total])

    print(
        tabulate.tabulate(
            table,
            headers=["Type", "Count"],
        )
    )


def walk_list(brd: AllegroBoard, head: int, tail: int, next_attr="next"):
    """
    Generate a list of objects from a linked list, following the "next"
    entry on each object.

    yield index, node_key, object
    """

    node_key = head
    index = 0

    # Walk the list until we reach the end or the next is null
    while node_key and node_key != tail:
        obj = brd.object(node_key)
        yield index, node_key, obj

        node_key = getattr(obj.data, next_attr)
        index += 1


def get_layer_for_obj(obj: allegro_brd.AllegroBrd.BoardObject) -> allegro_brd.AllegroBrd.LayerInfo | None:

    if hasattr(obj.data, "layer"):
        return obj.data.layer

    return None


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Allegro file CLI explorer")
    parser.add_argument("brd", type=Path,
                        help="Allegro board file")

    parser.add_argument("-S", "--summary", action="store_true",
                        help="Print a summary of the Allegro board file")

    parser.add_argument("-s", "--string", type=IntIsh,
                        help="Print a string from the string map")

    parser.add_argument("--string-match", "--sm", type=str,
                        help="Print strings in the string map that match a substring")

    parser.add_argument("-k", "--key", type=IntIsh,
                        nargs="+",
                        help="Print the object with the given key")

    parser.add_argument("-a", "--address", type=IntIsh,
                        nargs="+",
                        help="Print the objects containing the given file address")

    parser.add_argument("--oc", "--object-count", action="store_true",
                        help="Print the object counts of the Allegro board file")

    parser.add_argument("--dump-lists", "--dll", action="store_true",
                        help="Dump all the linked lists in the Allegro board file header")

    parser.add_argument("--walk-list", "--wl", nargs="+",
                        help="Walk a list of objects in the Allegro board file. "
                             "Either the name of a header list, or a head and tail pointer")

    parser.add_argument("--dump-obj", "--do", action="store_true",
                        help="Dump the objects in detailed format when walking a list, etc")

    parser.add_argument("--dump-by-type", "--dt", type=IntIsh,
                        help="Dump all the objects of the given type.")

    parser.add_argument( "--dump-layers", "--dl", action="store_true",
                        help="Dump all the layers in the layer map structure")

    parser.add_argument( "--dump-object-layers", "--dol", action="store_true",
                         help="Dump layers for objects with layer info")

    parser.add_argument("--footprints", "--fp", nargs="*", default=None,
                        help="Dump footprint info for the given ref designators, or all if none given")

    parser.add_argument("--padstacks", "--ps", nargs="*",
                        help="Dump padstacks with the given name(s)")


    args = parser.parse_args()

    # Load the Allegro board file via Kaitai
    with open(args.brd, "rb") as f:
        ks = KaitaiStream(f)
        kt_brd_struct = allegro_brd.AllegroBrd(ks)

    # Print the parsed data
    print("Parsed Allegro board file.")

    brd = AllegroBoard(kt_brd_struct)

    prntr = AllegroBoard.Printer(brd, 4)

    lists = {
        "x04": kt_brd_struct.ll_x04,
        "x06": kt_brd_struct.ll_x06,
        "x0c_2": kt_brd_struct.ll_x0c_2,
        "x0e_x28": kt_brd_struct.ll_x0e_x28,
        "x14": kt_brd_struct.ll_x14,
        "x1b": kt_brd_struct.ll_x1b,
        "x1c": kt_brd_struct.ll_x1c,
        "x24_x28": kt_brd_struct.ll_x24_x28,
        "unused_1": kt_brd_struct.ll_unused_1,
        "x2b": kt_brd_struct.ll_x2b,
        "x03_x30": kt_brd_struct.ll_x03_x30,
        "x0a": kt_brd_struct.ll_x0a,
        "x1d_x1e_x1f": kt_brd_struct.ll_x1d_x1e_x1f,
        "unused_2": kt_brd_struct.ll_unused_2,
        "x38": kt_brd_struct.ll_x38,
        "x2c": kt_brd_struct.ll_x2c,
        "x0c": kt_brd_struct.ll_x0c,
        "unused_3": kt_brd_struct.ll_unused_3,
        "x36": kt_brd_struct.ll_x36,
        "unused_4": kt_brd_struct.ll_unused_4,
        "unused_5": kt_brd_struct.ll_unused_5,
        "x0a_2": kt_brd_struct.ll_x0a_2,
    }

    # Print a summary of the Allegro board file
    if args.summary:
        print("Summary:")
        print(f"  Magic:   {kt_brd_struct.magic:#010x}")
        print(f"  Version: {kt_brd_struct.ver:#010x}")
        print(f"  Objects: {kt_brd_struct.object_count}")

    if args.oc:

        objs = kt_brd_struct.objects

        print_obj_counts_by_type(objs)

    if args.string is not None:
        # Print a string from the string map
        key = int(args.string)
        try:
            s = brd.string(key)
            print(f"String {key:#010x}: {s}")
        except KeyError:
            print(f"String with key {key:#010x} not found")

    if args.string_match is not None:
        for k, s in brd.strs.items():
            if args.string_match in s:
                print(f"String key {k:#010x}: {s}")

    def print_obj_info(obj):
        start = obj._debug["type"]["start"]
        end = obj._debug["data"]["end"]

        # index of object in the list of objects
        index = kt_brd_struct.objects.index(obj)

        print(f"Key:           {obj.data.key:#010x}")
        print(f"Object type:   {obj.type:#04x}")
        print(f"Object extent: {start:#010x} - {end:#010x}")
        print(f"Object size:   {end - start:#010x}")
            # Good for looking up in the Kaitai Web IDE, for example
        print(f"Object index:  {index} (of {len(kt_brd_struct.objects)})")
        print("")

    if args.key:

        for key in args.key:
            obj = brd.object(int(key))

            print_obj_info(obj)
            print("")
            brd.print_obj(obj)
            print("")

    if args.address:

        for address in args.address:
            addr = int(address)

            found = False

            for obj in kt_brd_struct.objects:
                start = obj._debug["type"]["start"]
                end = obj._debug["data"]["end"]

                if start <= addr < end:
                    print_obj_info(obj)
                    print("")
                    brd.print_obj(obj)
                    print("")

                    found = True
                    break

            if not found:
                print(f"Address {addr:#010x} not found in any object")

    if args.dump_by_type is not None:
        # Dump all the objects of the given type.
        obj_type = int(args.dump_by_type)

        objs = []

        for obj in kt_brd_struct.objects:
            if obj.type == obj_type:
                objs.append(obj)

        for obj in objs:
            print("")
            brd.print_obj(obj)
            print("")

    if args.dump_layers:

        lm = kt_brd_struct.layer_map

        for i, layer in enumerate(lm.entries):
            print(f"Layer {i}:")
            prntr.print_v("A", layer.a)
            prntr.print_ptr("List:", layer.layer_list)

    if args.dump_object_layers:

        for obj in kt_brd_struct.objects:
            layer = get_layer_for_obj(obj)
            if layer is not None:
                s = f"  {obj.type:#04x} "
                if getattr(obj.data, "key"):
                    s += f"{obj.data.key:#010x} "

                s += AllegroBoard.Printer.format_layer(layer)

                print(s)

    if args.walk_list is not None:
        # Walk a list of objects in the Allegro board file

        if len(args.walk_list) == 1:
            try:
                ll = lists[args.walk_list[0]]
                head = ll.head
                tail = ll.tail
            except KeyError:
                raise ValueError(
                    f"Unknown walk list: {args.walk_list}. "
                    f"Valid options are: {', '.join(lists.keys())}"
                )
        elif len(args.walk_list) == 2:
            head = IntIsh(args.walk_list[0]).value
            tail = IntIsh(args.walk_list[1]).value
        else:
            raise ValueError("Expected 1 or 2 walk_list parameters")

        objs = []

        print(f"Walking list:")
        print(f"  Head: {head:#010x}")
        print(f"  Tail: {tail:#010x}")

        for index, key, obj in walk_list(brd, head, tail):
            prntr.print_ptr(f"    Entry {index}", key)

            objs.append(obj)

            if args.dump_obj:
                print("")
                brd.print_obj(obj)
                print("")

        print("")
        print_obj_counts_by_type(objs)

    if args.dump_lists:

        for list in lists.keys():
            ll = lists[list]
            print(f"List: {list}")
            prntr.print_ptr("  Head", ll.head)
            prntr.print_ptr("  Tail", ll.tail)

    if args.footprints is not None:

        for index, key, obj in walk_list(brd, kt_brd_struct.ll_x2b.head, kt_brd_struct.ll_x2b.tail):
            print(f"FP index: {index}, Key: {key:#01x}")

            fp_ref = brd.string(obj.data.fp_str_ref)
            print(f"  Ref: {fp_ref}")

            next = obj.data.first_inst_ptr

            if next == 0:
                print("  No instances")
                continue

            inst_num = 0
            while next and next != key:
                inst_obj = brd.object(next)

                print(f"  Instance {inst_num}: {next:#010x} ({inst_obj.type:#04x})")

                if hasattr(inst_obj.data, "inst_ref_16x"):
                    inst_ref = inst_obj.data.inst_ref_16x
                else:
                    inst_ref = inst_obj.data.inst_ref

                print(f"    Instance 0x07: {inst_ref:#010x}")
                print(f"    First pad: {inst_obj.data.first_pad_ptr:#010x}")

                next = inst_obj.data.next
                inst_num += 1

    if args.padstacks is not None:
        def filter_padstack(psobj: allegro_brd.AllegroBrd.Type1cPadStack) -> bool:
            if not args.padstacks:
                return True

            name = brd.string(psobj.pad_str)
            return name in args.padstacks

        for index, key, obj in walk_list(brd, kt_brd_struct.ll_x1c.head, kt_brd_struct.ll_x1c.tail):

            if not filter_padstack(obj.data):
                continue

            brd.print_obj(obj)
