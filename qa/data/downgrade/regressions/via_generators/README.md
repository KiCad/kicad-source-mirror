# Via-generator downgrade regression

`via_generators.kicad_pcb` is a current-format regression input, not an expected
file produced by the older-version exporter.

The board was created on 2026-10-08 using the native KiCad 10.99 PCB model,
`PCB_VIA_STITCH` generation algorithms and `PCB_VIA_STACK::Regenerate`. KiCad's
`KIID` generator supplied the item identifiers and the current native writer
serialized the board. It was then opened in the development PCB editor, reviewed,
edited and saved by the fixture author. The authoring build came from
`d60fd8b2d21f6e5ac230ba3109a5e82d3d0c5928`.

## What the board contains

- A GND stitching generator with 49 through vias, nested inside an ordinary group.
- A GND guarding generator with 26 through vias around a SIGNAL track.
- A genuine stacked-microvia generator with two coaxial GND microvias spanning
  F.Cu to In1.Cu and In1.Cu to In2.Cu. Their diameter is 0.30 mm, their drill is
  0.15 mm, and their filling flag is enabled.
- Three ordinary through vias and three tracks as geometry and net controls.
- Six copper layers and three filled GND zones.

## Expected export behavior

The regression loads the saved board, exports it to KiCad 9 and KiCad 10 under
both approximation policies, and reloads each exported file.

All 80 vias and all three tracks retain their item UUIDs, positions, dimensions,
layer assignments and net names. Generator objects are removed. The ordinary
outer group remains and receives the stitching generator's existing member vias.
The source file and source model remain unchanged.

Both targets report the loss of editability for two stitching/guarding generators
and one stacked-microvia generator. KiCad 9 additionally reports the unsupported
protection metadata on the two filled microvias. That metadata loss must not
remove the microvias or alter their copper geometry. KiCad 10 retains it.

`--drop-approximations` does not discard generated vias. The generator handling
reports removal of editing metadata, rather than approximating or deleting the
copper. This fixture tests that existing behavior and introduces no new export
policy.

Native KiCad 9 and KiCad 10 reader checks are a separate verification step.
Reader acceptance does not imply that the demonstration board is DRC-clean.
