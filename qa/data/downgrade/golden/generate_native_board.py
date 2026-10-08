#!/usr/bin/env python3
"""Save an independently authored board seed using a genuine target-release pcbnew.

Run with that release's Python and pcbnew module, never with the downgrade exporter.
This is a maintainer tool; the automated tests only read the checked-in goldens.
"""

import argparse
import json
from pathlib import Path

import pcbnew
import wx


def board_items(board):
    items = board.GetFootprints() + board.GetTracks() + board.GetDrawings()
    items.extend(board.Zones())
    items.extend(board.Groups())
    for footprint in board.GetFootprints():
        items.extend(footprint.GetFields())
        items.extend(footprint.Pads())
        items.extend(footprint.GraphicalItems())
        items.extend(footprint.Zones())
        items.extend(footprint.Groups())
    return items


def replace_uuids(board, mapping):
    replacements = []
    for item in board_items(board):
        original = item.m_Uuid.AsString()
        if original not in mapping:
            mapping[original] = pcbnew.KIID().AsString()
        replacements.append((item, mapping[original]))

    if len(set(mapping.values())) != len(mapping):
        raise ValueError("UUID replacements must be unique")
    for item, replacement in replacements:
        item.m_Uuid.Clone(pcbnew.KIID(replacement))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--major", required=True, choices=("9", "10"))
    parser.add_argument("--uuid-map", type=Path,
                        help="Replace item UUIDs using a shared JSON map, generating missing IDs with KiCad")
    parser.add_argument("seed", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    version = pcbnew.GetBuildVersion()
    if not version.startswith(args.major + ".0."):
        parser.error(f"Expected native KiCad {args.major}.0, got {version}")
    if args.seed.resolve() == args.output.resolve():
        parser.error("The output must not overwrite the independent seed")
    if args.uuid_map and args.uuid_map.resolve() in (args.seed.resolve(), args.output.resolve()):
        parser.error("The UUID map must be separate from the board files")

    mapping = {}
    if args.uuid_map and args.uuid_map.exists():
        mapping = json.loads(args.uuid_map.read_text())
        if not isinstance(mapping, dict) or any(
                not isinstance(original, str) or not isinstance(replacement, str)
                or not pcbnew.KIID.SniffTest(original) or not pcbnew.KIID.SniffTest(replacement)
                for original, replacement in mapping.items()):
            parser.error("The UUID map must contain valid UUID-to-UUID pairs")

    _app = wx.App(False)
    board = pcbnew.LoadBoard(str(args.seed.resolve()))
    if board is None:
        raise RuntimeError("Native board load failed")
    if args.uuid_map:
        replace_uuids(board, mapping)
    expected = sorted(item.m_Uuid.AsString() for item in board_items(board))
    if not pcbnew.SaveBoard(str(args.output.resolve()), board):
        raise RuntimeError("Native board load/save failed")
    reloaded = pcbnew.LoadBoard(str(args.output.resolve()))
    if reloaded is None or sorted(item.m_Uuid.AsString() for item in board_items(reloaded)) != expected:
        raise RuntimeError("The saved board did not reload with the same item UUIDs")
    if args.uuid_map:
        args.uuid_map.write_text(json.dumps(mapping, indent=2, sort_keys=True) + "\n")
    print(f"Saved {args.output} using native KiCad {version}")


if __name__ == "__main__":
    main()
