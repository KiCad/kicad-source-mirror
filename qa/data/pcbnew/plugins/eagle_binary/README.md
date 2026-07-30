# Eagle binary (.brd) test boards

Real pre-v6 binary Eagle board files used to regression-test the binary branch
of `PCB_IO_EAGLE`.

| File                          | Eagle binary magic | Format era             | Exercises                          |
|-------------------------------|--------------------|------------------------|------------------------------------|
| `blink1_b1a.brd`              | `0x10 0x00`        | v4/v5 (with DRC+notes) | baseline v4/v5 load                |
| `blink1_v1a.brd`              | `0x10 0x80`        | v3 (no DRC/notes)      | baseline v3 load                   |
| `rocketgps.brd`               | `0x10 0x80`        | v3                     | custom element attributes (no name)|
| `boomchak.brd`                | `0x10 0x80`        | v3                     | unnamed signals (auto-named nets)  |
| `turnemoff.brd`               | `0x10 0x00`        | v4/v5                  | vertex-less package/signal polygons|
| `issue24827_brenner57e.brd`   | `0x10 0x80`        | v4                     | pad names, pad shapes, arc centers |

The drawing major version lives at offset 8 rather than in the magic, and
`issue24827_brenner57e.brd` is the only sample that reports 4 there; every other
board reports 5. It is therefore the only coverage of the v4 pad record layout.

The last four boards each reproduced a distinct crash or load failure in the
binary importer before the fixes accompanying these files:

- `rocketgps.brd` carries custom element attributes whose binary record has no
  name field, producing schema-invalid `<attribute>` nodes that aborted the
  shared XML reader.
- `boomchak.brd` contains auto-generated (unnamed) signals whose empty net name
  collided with the reserved unconnected net, orphaning the net code of every
  item routed on them.
- `turnemoff.brd` contains degenerate polygons with no vertices, which
  dereferenced an empty vertex list in `packagePolygon`.
- `issue24827_brenner57e.brd` is a v4 board whose pad signatures clear the same
  flag bits the Eagle 3.x short rows ignore, so every pad bound that layout and
  read an empty name, collapsing each element's contactrefs onto one signal. It
  also carries octagon and oblong pads and TO-92 silk arcs, which pinned down the
  pad shape ordinals and the curved-wire arc centre reconstruction.

## Source and license

`blink1_b1a.brd` and `blink1_v1a.brd` are from the blink(1) open-source hardware
project by ThingM (https://github.com/todbot/blink1). blink(1) hardware is
released under the Creative Commons Attribution-ShareAlike 3.0 license
(CC BY-SA 3.0, https://creativecommons.org/licenses/by-sa/3.0/), OSHW US000051.

The remaining boards are unmodified copies retained here solely as small importer
regression fixtures:

- `rocketgps.brd` from https://github.com/KevWal/RocketGPS
- `boomchak.brd` from https://github.com/mengstr/BoomChak
- `turnemoff.brd` from https://github.com/mengstr/TurnEmOff
- `issue24827_brenner57e.brd` as attached to
  https://gitlab.com/kicad/code/kicad/-/issues/24827

None of those four sources declares an explicit license.
