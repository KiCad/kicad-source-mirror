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

    def object(self, key: int) -> allegro_brd.AllegroBrd.BoardObject:
        """
        Get an object from the Allegro board file by key.
        """
        if key in self.keys:
            return self.keys[key]
        else:
            raise KeyError(f"Key {key:#01x} not found in Allegro board file.")


class IntIsh:

    def __init__(self, value):

        if isinstance(value, str) and value.startswith("0x"):
            value = int(value, 16)
        else:
            value = int(value)

        self.value = value

    def __int__(self):
        return self.value


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Allegro file CLI explorer")
    parser.add_argument("brd", type=Path,
                        help="Allegro board file")

    parser.add_argument("-S", "--summary", action="store_true",
                        help="Print a summary of the Allegro board file")

    parser.add_argument("-k", "--key", type=IntIsh,
                        help="Print the object with the given key")

    parser.add_argument("--oc", "--object-count", action="store_true",
                        help="Print the object counts of the Allegro board file")

    parser.add_argument("--walk-list", "--wl", type=str,
                        help="Walk a list of objects in the Allegro board file")

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

        print(
            tabulate.tabulate(
                table,
                headers=["Type", "Count"],
            )
        )

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

    if args.walk_list is not None:
        # Walk a list of objects in the Allegro board file

        lists = {
            "x24_x28": kt_brd_struct.ll_x24_x28,
            "x04": kt_brd_struct.ll_x04,
            "x06": kt_brd_struct.ll_x06,
            "x0c_2": kt_brd_struct.ll_x0c_2,
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

        while(node_key and node_key != ll.tail):
            print(f"Entry {index}, Key: {node_key:#01x}")

            obj = brd.object(node_key)
            print(f"  Object: {obj.type:#04x}")

            node_key = obj.data.next
