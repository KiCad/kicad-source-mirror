# Group-placement downgrade regression

`group_placement.kicad_pcb` is a current-format regression input, not an
expected file produced by the older-version exporter. It was copied from the
native `qa/data/pcbnew/issue22983/issue22983.kicad_pcb` fixture, then opened,
annotated, and saved in the development KiCad PCB editor on 2026-10-09. The
editor generated the explanatory text item's UUID. The original board items
retain their UUIDs.

The companion `group_placement.kicad_pro` was saved by the same editor session.
The test requires it to load, and the board export also reads it from the
board's directory. The test does not assert on individual project settings.

The board contains seven zones, including four enabled group-placement rule
areas, four groups, four footprints, eight pads, and filled copper. The text on
F.SilkS explains the version difference when the board is opened in the editor.

`NativeGroupPlacementFixtureKeepsGeometryAndOwnershipForBothPolicies` exports
the board to KiCad 9 and KiCad 10 with each approximation policy, then reloads
the output. KiCad 9 must report four dropped placement links and clear only
their placement metadata. KiCad 10 must retain those links. Both versions must
retain zone outlines and fills, keepout settings, groups and their membership,
pad geometry and identity, and the explanatory text. Neither export may change
the source file or its in-memory board model.

These assertions inspect the reloaded board rather than comparing against a
golden output file. The fixture is therefore independent of the downgrade
writer's output.
