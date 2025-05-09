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


    def print_obj(self, obj, indent="  "):
        """
        Print some object as best as possible.
        """

        def prnt(s):
            print(indent + s)

        def print_ptr(name, pkey):
            try:
                obj = self.object(pkey)
            except KeyError:
                prnt(f"{name:12}: {pkey:#010x} (not found)")
                return

            prnt(f"{name:12}: {pkey:#010x} ({obj.type:#04x})")

        def print_v(name, value, hex: bool=True):
            if isinstance(value, int) and hex:
                prnt(f"{name:12}: {value:#x}")
            else:
                prnt(f"{name:12}: {value}")

        def print_s(name, k : int | list):

            if isinstance(k, list):
                bs = bytearray(k)
                s = bs.decode("utf-8", errors="ignore")
                prnt(f"{name:12}: {s}")
                return

            try:
                s = self.string(k)
                prnt(f"{name:12}: {s} (key: {k:#010x})")
            except KeyError:
                prnt(f"{name:12}: {k:#010x} (not found)")
                return

        def print_coords(name, coords):
            prnt(f"{name:12}: ({coords.x}, {coords.y})")

        def print_layer(layer):
            name = "Layer"
            v = f"{layer.family} {layer.ordinal}"
            print_v(name, v)

        t = obj.type
        d = obj.data

        print_v(f"Object type", t)

        if hasattr(d, "key"):
            print_v("Key", d.key)

        if t == 0x03:
            print_v("subtype", d.subtype)
            print_v("unknown_hdr", d.unknown_hdr)
            print_v("size", d.size)

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)

            if d.subtype in [0x68, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x73, 0x78]:
                bytes = bytearray(d.data.chars)
                s = bytes.decode("utf-8", errors="ignore")
                print_v("string data", s)

        elif t == 0x06:

            print_s("String", d.str)
            print_s("Ptr 1", d.ptr_1)
            print_ptr("Ptr instance", d.ptr_instance)
            print_ptr("Ptr FP", d.ptr_fp)
            print_ptr("Ptr x08", d.ptr_x08)
            print_ptr("Ptr x03", d.ptr_x03_symbol)

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)

        elif t == 0x07:
            if hasattr(d, "ptr_1"):
                print_ptr("Ptr 1", d.ptr_1)
            print_ptr("Ptr 2", d.ptr_2)
            print_ptr("Ptr 3", d.ptr_3)
            print_ptr("Ptr 4", d.ptr_4)
            print_ptr("ptr_0x2d", d.ptr_0x2d)

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)
            if hasattr(d, "unknown_3"):
                print_v("unknown_3", d.unknown_3)

            print_s("ref_des_ref", d.ref_des_ref)


        elif t == 0x08:

            if hasattr(d, "str_ptr_16x"):
                print_s("Str ptr", d.str_ptr_16x)
            if hasattr(d, "str_ptr"):
                print_s("Str ptr", d.str_ptr)

            if hasattr(d, "prev_ptr"):
                print_ptr("Prev", d.prev_ptr)
            print_ptr("Next", d.next_ptr)
            print_ptr("Ptr 3", d.ptr3)
            print_ptr("Ptr 4", d.ptr4)

            if hasattr(d, "unknown_1"):
                print_ptr("unknown_1", d.unknown_1)

        elif t == 0x0f:
            print_s("str_unk", d.str_unk)
            print_v("s", d.s)
            print_ptr("Ptr x06", d.ptr_x06)
            print_ptr("Ptr x11", d.ptr_x11)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)
            if hasattr(d, "unknown_3"):
                print_v("unknown_3", d.unknown_3)

        elif t == 0x10:
            print_ptr("Ptr 1", d.ptr1)
            print_ptr("Ptr 2", d.ptr2)
            print_s("String", d.str)
            print_ptr("Ptr 4", d.ptr4)
            print_ptr("Path str", d.path_str)

        elif t == 0x11:
            print_s("Name", d.name)
            print_ptr("Ptr 1", d.ptr1)
            print_ptr("Ptr 2", d.ptr2)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)

        elif t == 0x14:

            print_ptr("Next", d.next)
            print_ptr("Parent", d.parent_ptr)
            print_ptr("Segment", d.segment_ptr)
            print_ptr("Ptr 0x03", d.ptr_0x03)
            print_ptr("Ptr x026", d.ptr_0x26)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)

            print_layer(d.layer)

        elif t in [0x15, 0x16, 0x17]:

            print_ptr("Next", d.next)
            print_ptr("Parent", d.parent_ptr)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)

            print_v("Width", d.width, hex=False)
            print_coords("pt0", d.coords_0)
            print_coords("pt1", d.coords_1)

        elif t == 0x1b: # Net
            print_s("Net", d.net_name)

            print_ptr("Path str", d.path_str_ptr)
            print_ptr("Model str", d.model_ptr)
            print_ptr("Ptr1", d.ptr1)
            print_ptr("Ptr2", d.ptr2)
            print_ptr("Ptr4", d.ptr4)
            print_ptr("Ptr6", d.ptr6)

            print_v("Type", d.type)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_1", d.unknown_2)

            print_v("unknown_3", d.unknown_3)
            print_v("unknown_4", d.unknown_4)

        elif t == 0x1c: # Padstack
            print_s("Pad str", d.pad_str)
            print_v("Pad path", d.pad_path)
            print_v("Layer count", d.layer_count)
            print_v("Pad type", d.pad_info.pad_type)

            for i, pc in  enumerate(d.components):
                prnt(f"- Component {i}")

                print_v(f"  type", pc.t)

                if pc.t == 0x00:
                    continue

                print_v(f"  w", pc.w, hex=False)
                print_v(f"  h", pc.h, hex=False)
                print_v(f"  x3", pc.h, hex=False)
                print_v(f"  x4", pc.h, hex=False)
                print_ptr("  str_ptr", pc.str_ptr)

        elif t == 0x24: # Rectangle
            print_coords("pt0", d.coords_0)
            print_coords("pt1", d.coords_1)
            print_ptr("ptr1", d.ptr1)

        elif t == 0x28: # Polygon?
            print_coords("pt0", d.coords_0)
            print_coords("pt1", d.coords_1)
            print_ptr("ptr1", d.ptr1)
            print_ptr("ptr2", d.ptr2)
            print_ptr("ptr3", d.ptr3)
            print_ptr("ptr4", d.ptr4)
            if hasattr(d, "ptr5"):
                print_ptr("ptr4", d.ptr5)
            print_ptr("ptr6", d.ptr6)
            if hasattr(d, "ptr7_16x"):
                print_ptr("ptr7_16x", d.ptr7_16x)

            print_ptr("first_seg", d.first_segment_ptr)

            print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)
            if hasattr(d, "unknown_3"):
                print_v("unknown_3", d.unknown_3)
            print_v("unknown_4", d.unknown_4)
            print_v("unknown_5", d.unknown_5)

        elif t == 0x2b:
            print_ptr("next", d.next)
            print_s("fp_str", d.fp_str_ref)

            print_ptr("first_inst_ptr", d.first_inst_ptr)
            print_ptr("ptr_2", d.ptr_2)
            print_ptr("ptr_3", d.ptr_3)
            print_ptr("ptr_4", d.ptr_4)
            print_ptr("str_ptr1", d.str_ptr1)
            print_ptr("ptr_5", d.ptr_5)
            print_ptr("ptr_6", d.ptr_6)
            print_ptr("ptr_7", d.ptr_7)

            print_coords("coords0", d.coords0)
            print_coords("coords1", d.coords1)

            print_v("unknown_1", d.unknown_1)

        elif t == 0x2c:
            print_ptr("next", d.next)
            print_v("flags", d.flags)
            print_ptr("ptr_1", d.ptr_1)
            print_ptr("ptr_2", d.ptr_2)
            print_ptr("ptr_3", d.ptr_3)
            print_s("subclass_str", d.subclass_str)

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)
            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)
            if hasattr(d, "unknown_3"):
                print_v("unknown_3", d.unknown_3)
            print_v("unknown_4", d.unknown_4)

        elif t == 0x2d:
            print_ptr("next", d.next)
            print_ptr("first_pad_ptr", d.first_pad_ptr)
            print_ptr("ptr_1", d.ptr_1)
            print_ptr("ptr_2", d.ptr_2)
            print_ptr("ptr_3", d.ptr_3)
            print_ptr("ptr_4", d.ptr_4)
            print_ptr("ptr_5", d.ptr_5)
            print_ptr("ptr_6", d.ptr_6)

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)

            print_v("unknown_2", d.unknown_2)
            print_v("unknown_3", d.unknown_3)
            print_v("flags", d.flags)
            print_v(f"rotation", d.rotation / 1000., hex=False)
            print_coords("coords", d.coords)

            if hasattr(d, "inst_ref_16x"):
                print_ptr("inst_ref", d.inst_ref_16x)
            if hasattr(d, "inst_ref"):
                print_ptr("inst_ref", d.inst_ref)

        elif t == 0x30:

            print_ptr("next", d.next)
            print_ptr("str_graphic_ptr", d.str_graphic_ptr)

            if hasattr(d, "ptr3"):
                print_ptr("ptr_3", d.ptr_3)

            if hasattr(d, "un4"):
                print_ptr("un4", d.un4)

        elif t == 0x31:
            print_ptr("str_graphic_wrapper_ptr", d.str_graphic_wrapper_ptr)

            print_s("value", d.value.chars)

        elif t == 0x37:

            print_ptr("ptr_1", d.ptr_1)

            print_v("capacity", d.capacity)
            print_v("count", d.count)

            print_v("unknown_1", d.unknown_1)

            if hasattr(d, "unknown_2"):
                print_v("unknown_2", d.unknown_2)
            if hasattr(d, "unknown_3"):
                print_v("unknown_3", d.unknown_3)

            for i in range(d.count):
                entry = d.ptrs[i]
                print_ptr(f"entry {i}", entry)

        elif t == 0x3c:

            if hasattr(d, "unknown_1"):
                print_v("unknown_1", d.unknown_1)

            num_entries = d.num_entries
            print_v("num_entries", num_entries)

            for i in range(num_entries):
                entry = d.entries[i]
                print_ptr(f"entry {i}", entry)

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


def walk_list(brd: AllegroBoard, ll):
    """
    Generate a list of objects from a linked list

    yield index, node_key, object
    """

    node_key = ll.head
    index = 0

    # Walk the list until we reach the end or the next is null
    while(node_key and node_key != ll.tail):
        obj = brd.object(node_key)
        yield index, node_key, obj

        node_key = obj.data.next
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

    parser.add_argument("--walk-list", "--wl", type=str,
                        help="Walk a list of objects in the Allegro board file")

    parser.add_argument("--dump-obj", "--do", action="store_true",
                        help="Dump the objects in detailed format when walking a list, etc")

    parser.add_argument("--dump-by-type", "--dt", type=IntIsh,
                        help="Dump all the objects of the given type.")

    parser.add_argument( "--dump-layer-map", "--dlm", action="store_true",
                        help="Dump all the layers in the layer map structure")

    parser.add_argument("--footprints", "--fp", nargs="*", default=None,
                        help="Dump footprint info for the given ref designators, or all if none given")

    parser.add_argument( "--dump-layers", "--dl", action="store_true",
                         help="Dump all the layers in the layer 0x2A structures")

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

    if args.dump_layer_map:

        lm = kt_brd_struct.layer_map

        for i, layer in enumerate(lm.entries):
            print(f"Layer {i}:")
            print(f"  A: {layer.a:#010x}")
            print(f"  B: {layer.b:#010x}")

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

        try:
            ll = lists[args.walk_list]
        except KeyError:
            raise ValueError(
                f"Unknown walk list: {args.walk_list}. "
                f"Valid options are: {', '.join(lists.keys())}"
            )

        objs = []

        for index, key, obj in walk_list(brd, ll):
            print(f"Entry {index}, Key: {key:#01x}")

            objs.append(obj)

            if args.dump_obj:
                print("")
                brd.print_obj(obj)
                print("")

        print("")
        print_obj_counts_by_type(objs)

    if args.footprints is not None:

        for index, key, obj  in walk_list(brd, kt_brd_struct.ll_x2b):
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

    # if args.dump_layers:
