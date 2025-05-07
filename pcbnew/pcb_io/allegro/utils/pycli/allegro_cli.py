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

        def print_s(name, k : int):
            try:
                s = self.string(k)
                prnt(f"{name:12}: {s} (key: {k:#010x})")
            except KeyError:
                prnt(f"{name:12}: {k:#010x} (not found)")
                return

        def print_coords(name, coords):
            prnt(f"{name:12}: ({coords.x}, {coords.y})")

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


        elif t == 0x08:

            if hasattr(d, "ptr1"):
                print_ptr("Ptr 1", d.ptr1)
            if hasattr(d, "str_ptr_16x"):
                print_s("Str ptr", d.str_ptr_16x)
            if hasattr(d, "str_ptr"):
                print_s("Str ptr", d.str_ptr)

            print_ptr("Ptr 2", d.ptr2)
            print_ptr("Ptr 3", d.ptr3)
            print_ptr("Ptr 4", d.ptr4)

            if hasattr(d, "unknown_1"):
                print_ptr("unknown_1", d.unknown_1)

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

        node_key = ll.head
        index = 0

        objs = []

        while(node_key and node_key != ll.tail):
            print(f"Entry {index}, Key: {node_key:#01x}")

            obj = brd.object(node_key)
            objs.append(obj)
            print(f"  Object: {obj.type:#04x}")

            if args.dump_obj:
                print("")
                brd.print_obj(obj)
                print("")

            node_key = obj.data.next
            index += 1

        print("")
        print_obj_counts_by_type(objs)
