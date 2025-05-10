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
import logging
import struct

import kaitaistruct
import tabulate
from pathlib import Path

from kaitaistruct import KaitaiStream
import allegro_brd


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

        def _value_or_by_attr(self, struct_or_v, attr_name):

            if isinstance(struct_or_v, kaitaistruct.KaitaiStruct):
                if hasattr(struct_or_v, attr_name):
                    value = getattr(struct_or_v, attr_name)
                else:
                    return
            else:
                value = struct_or_v

            return value

        def print_ptr(self, name, struct_or_key: int, attr_name: str | None = None):

            attr_name = attr_name or name

            key_val = self._value_or_by_attr(struct_or_key, attr_name)

            # Doesn't exist
            if key_val is None:
                return

            try:
                obj = self.board.object(key_val)
                self.prnt(f"{name:12}: {key_val:#010x} ({obj.type:#04x})")
            except KeyError:
                self.prnt(f"{name:12}: {key_val:#010x} (not found)")
                return


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
                bs = bytearray(block_or_value.chars)
                value = bs.decode("utf-8", errors="ignore")

            else:
                value = self._value_or_by_attr(block_or_value, attr_name)

            if value is None:
                return

            if isinstance(value, allegro_brd.AllegroBrd.CadenceFp):
                self.prnt(f"{name:12}: {cfp_to_double(value)}")
            elif isinstance(value, int) and as_hex:
                self.prnt(f"{name:12}: {value:#x}")
            else:
                self.prnt(f"{name:12}: {value}")

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
                self.prnt(f"{name:12}: {string_val} (key: {string_id:#010x})")
            except KeyError:
                self.prnt(f"{name:12}: {string_id:#010x} (not found)")

        def print_coords(self, name, coords):
            self.prnt(f"{name:12}: ({coords.x}, {coords.y})")

        def print_layer(self, layer):
            name = "Layer"
            v = f"{layer.family} {layer.ordinal:#04x}"
            self.print_v(name, v)

    def print_obj(self, obj, indent=2):
        """
        Print some object as best as possible.
        """
        prntr = self.Printer(self, indent)

        t = obj.type
        d = obj.data

        prntr.print_v(f"Object type", t)

        if hasattr(d, "key"):
            prntr.print_v("Key", d, "key")

        if t == 0x01:
            prntr.print_v("un0", d)
            prntr.print_v("subtype", d)

            prntr.print_ptr("next", d)
            prntr.print_ptr("parent", d)
            prntr.print_v("un1", d)
            prntr.print_v("un6", d)
            prntr.print_v("width", d, as_hex=False)

            prntr.print_coords("P0", d.coords_0)
            prntr.print_coords("P1", d.coords_1)
            prntr.print_v("x", d)
            prntr.print_v("y", d)
            prntr.print_v("r", d)

        elif t == 0x03:
            prntr.print_v("subtype", d)
            prntr.print_v("unknown_hdr", d)
            prntr.print_v("size", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

            if d.subtype in [0x68, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x73, 0x78]:
                prntr.print_v("string data", d.data)

        elif t == 0x06:
            prntr.print_s("String", d.str)
            prntr.print_s("Ptr 1", d.str_2),
            prntr.print_ptr("Ptr instance", d.ptr_instance)
            prntr.print_ptr("Ptr FP", d.ptr_fp)
            prntr.print_ptr("Ptr x08", d.ptr_x08)
            prntr.print_ptr("Ptr x03", d.ptr_x03_symbol)

            prntr.print_v("unknown_1", d)

        elif t == 0x07:
            prntr.print_ptr("ptr_1", d)
            prntr.print_ptr("ptr_2", d)
            prntr.print_ptr("ptr_3", d)
            prntr.print_ptr("ptr_4", d)
            prntr.print_ptr("ptr_0x2d", d)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

            prntr.print_s("ref_des_ref", d)

        elif t == 0x08:

            prntr.print_s("Str ptr", d, "str_ptr_16x")
            prntr.print_s("Str ptr", d, "str_ptr")

            prntr.print_ptr("Prev", d, "prev_ptr")
            prntr.print_ptr("Next", d, "next_ptr")
            prntr.print_ptr("Ptr 3", d, "ptr3")
            prntr.print_ptr("Ptr 4", d, "ptr4")

            prntr.print_ptr("unknown_1", d)

        elif t == 0x0a:
            prntr.print_layer(d.layer)

        elif t == 0x0f:
            prntr.print_s("str_unk", d.str_unk)
            prntr.print_v("s", d.s)
            prntr.print_ptr("Ptr x06", d.ptr_x06)
            prntr.print_ptr("Ptr x11", d.ptr_x11)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

        elif t == 0x10:
            prntr.print_ptr("Ptr 1", d.ptr1)
            prntr.print_ptr("Ptr 2", d.ptr2)
            prntr.print_s("String", d.str)
            prntr.print_ptr("Ptr 4", d.ptr4)
            prntr.print_ptr("Path str", d.path_str)

        elif t == 0x11:
            prntr.print_s("Name", d.name)
            prntr.print_ptr("Ptr 1", d.ptr1)
            prntr.print_ptr("Ptr 2", d.ptr2)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

        elif t == 0x14:

            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Parent", d.parent_ptr)
            prntr.print_ptr("Segment", d.segment_ptr)
            prntr.print_ptr("Ptr 0x03", d.ptr_0x03)
            prntr.print_ptr("Ptr x026", d.ptr_0x26)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

            prntr.print_layer(d.layer)

        elif t in [0x15, 0x16, 0x17]:

            prntr.print_ptr("Next", d.next)
            prntr.print_ptr("Parent", d.parent_ptr)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

            prntr.print_v("Width", d.width, as_hex=False)
            prntr.print_coords("pt0", d.coords_0)
            prntr.print_coords("pt1", d.coords_1)

        elif t == 0x1b: # Net
            prntr.print_s("Net", d.net_name)

            prntr.print_ptr("Path str", d.path_str_ptr)
            prntr.print_ptr("Model str", d.model_ptr)
            prntr.print_ptr("Ptr1", d.ptr1)
            prntr.print_ptr("Ptr2", d.ptr2)
            prntr.print_ptr("Ptr4", d.ptr4)
            prntr.print_ptr("Ptr6", d.ptr6)

            prntr.print_v("Type", d.type)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

        elif t == 0x1c: # Padstack
            prntr.print_s("Pad str", d.pad_str)
            prntr.print_v("Pad path", d.pad_path)
            prntr.print_v("Layer count", d.layer_count)
            prntr.print_v("Pad type", d.pad_info.pad_type)

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

        elif t == 0x23:
            prntr.print_ptr("next", d)
            prntr.print_layer(d.layer)

        elif t == 0x24: # Rectangle
            prntr.print_layer(d.layer)
            prntr.print_coords("pt0", d.coords_0)
            prntr.print_coords("pt1", d.coords_1)
            prntr.print_ptr("ptr1", d.ptr1)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)

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
            prntr.print_ptr("next", d.next)
            prntr.print_s("fp_str", d.fp_str_ref)

            prntr.print_ptr("first_inst_ptr", d.first_inst_ptr)
            prntr.print_ptr("ptr_2", d.ptr_2)
            prntr.print_ptr("ptr_3", d.ptr_3)
            prntr.print_ptr("ptr_4", d.ptr_4)
            prntr.print_ptr("str_ptr1", d.str_ptr1)
            prntr.print_ptr("ptr_5", d.ptr_5)
            prntr.print_ptr("ptr_6", d.ptr_6)
            prntr.print_ptr("ptr_7", d.ptr_7)

            prntr.print_coords("coords0", d.coords0)
            prntr.print_coords("coords1", d.coords1)

            prntr.print_v("unknown_1", d.unknown_1)

        elif t == 0x2c:
            prntr.print_ptr("next", d.next)
            prntr.print_v("flags", d.flags)
            prntr.print_ptr("ptr_1", d.ptr_1)
            prntr.print_ptr("ptr_2", d.ptr_2)
            prntr.print_ptr("ptr_3", d.ptr_3)
            prntr.print_s("subclass_str", d.subclass_str)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)
            prntr.print_v("unknown_4", d)

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

            prntr.print_ptr("inst_ref", d, "inst_ref_16x")
            prntr.print_ptr("inst_ref", d, "inst_ref")

        elif t == 0x30:

            prntr.print_ptr("next", d.next)
            prntr.print_ptr("str_graphic_ptr", d.str_graphic_ptr)

            prntr.print_ptr("ptr_3", d)
            prntr.print_ptr("un4", d)

        elif t == 0x31:
            prntr.print_ptr("str_graphic_wrapper_ptr", d.str_graphic_wrapper_ptr)

            prntr.print_v("value", d.value)

        elif t == 0x37:

            prntr.print_ptr("ptr_1", d.ptr_1)

            prntr.print_v("capacity", d.capacity)
            prntr.print_v("count", d.count)

            prntr.print_v("unknown_1", d)
            prntr.print_v("unknown_2", d)
            prntr.print_v("unknown_3", d)

            for i in range(d.count):
                entry = d.ptrs[i]
                prntr.print_ptr(f"entry {i}", entry)

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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Allegro file CLI explorer")
    parser.add_argument("brd", type=Path,
                        help="Allegro board file")

    parser.add_argument("-S", "--summary", action="store_true",
                        help="Print a summary of the Allegro board file")

    parser.add_argument("-s", "--string", type=IntIsh,
                        help="Print a string from the string map")

    parser.add_argument("-k", "--key", type=IntIsh,
                        help="Print the object with the given key")

    parser.add_argument("--oc", "--object-count", action="store_true",
                        help="Print the object counts of the Allegro board file")

    parser.add_argument("--walk-list", "--wl", nargs="+",
                        help="Walk a list of objects in the Allegro board file. "
                             "Either the name of a header list, or a head and tail pointer")

    parser.add_argument("--dump-obj", "--do", action="store_true",
                        help="Dump the objects in detailed format when walking a list, etc")

    parser.add_argument("--dump-by-type", "--dt", type=IntIsh,
                        help="Dump all the objects of the given type.")

    parser.add_argument( "--dump-layers", "--dl", action="store_true",
                        help="Dump all the layers in the layer map structure")

    parser.add_argument("--footprints", "--fp", nargs="*", default=None,
                        help="Dump footprint info for the given ref designators, or all if none given")

    args = parser.parse_args()

    # Load the Allegro board file via Kaitai
    with open(args.brd, "rb") as f:
        ks = KaitaiStream(f)
        kt_brd_struct = allegro_brd.AllegroBrd(ks)

    # Print the parsed data
    print("Parsed Allegro board file.")

    brd = AllegroBoard(kt_brd_struct)

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

    if args.key is not None:
        obj = brd.object(int(args.key))

        start = obj._debug["type"]["start"]
        end = obj._debug["data"]["end"]

        # index of object in the list of objects
        index = kt_brd_struct.objects.index(obj)

        print(f"Key:           {int(args.key):#010x}")
        print(f"Object type:   {obj.type:#04x}")
        print(f"Object extent: {start:#010x} - {end:#010x}")
        print(f"Object size:   {end - start:#010x}")
        # Good for looking up in the Kaitai Web IDE, for example
        print(f"Object index:  {index} (of {len(kt_brd_struct.objects)})")
        print("")
        brd.print_obj(obj)

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
        prntr = AllegroBoard.Printer(brd, 4)

        for i, layer in enumerate(lm.entries):
            print(f"Layer {i}:")
            prntr.print_v("A", layer.a)
            prntr.print_ptr("B", layer.b)

    if args.walk_list is not None:
        # Walk a list of objects in the Allegro board file

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

        for index, key, obj in walk_list(brd, head, tail):
            print(f"Entry {index}, Key: {key:#01x}")

            objs.append(obj)

            if args.dump_obj:
                print("")
                brd.print_obj(obj)
                print("")

        print("")
        print_obj_counts_by_type(objs)

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
