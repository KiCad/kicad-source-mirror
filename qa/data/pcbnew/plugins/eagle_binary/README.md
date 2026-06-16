# Eagle binary (.brd) test boards

Real pre-v6 binary Eagle board files used to regression-test the binary branch
of `PCB_IO_EAGLE`.

| File              | Eagle binary magic | Format era             | Exercises                          |
|-------------------|--------------------|------------------------|------------------------------------|
| `blink1_b1a.brd`  | `0x10 0x00`        | v4/v5 (with DRC+notes) | baseline v4/v5 load                |
| `blink1_v1a.brd`  | `0x10 0x80`        | v3 (no DRC/notes)      | baseline v3 load                   |
| `rocketgps.brd`   | `0x10 0x80`        | v3                     | custom element attributes (no name)|
| `boomchak.brd`    | `0x10 0x80`        | v3                     | unnamed signals (auto-named nets)  |
| `turnemoff.brd`   | `0x10 0x00`        | v4/v5                  | vertex-less package/signal polygons|

The last three boards each reproduced a distinct crash or load failure in the
binary importer before the fixes accompanying these files:

- `rocketgps.brd` carries custom element attributes whose binary record has no
  name field, producing schema-invalid `<attribute>` nodes that aborted the
  shared XML reader.
- `boomchak.brd` contains auto-generated (unnamed) signals whose empty net name
  collided with the reserved unconnected net, orphaning the net code of every
  item routed on them.
- `turnemoff.brd` contains degenerate polygons with no vertices, which
  dereferenced an empty vertex list in `packagePolygon`.

## Source and license

`blink1_b1a.brd` and `blink1_v1a.brd` are from the blink(1) open-source hardware
project by ThingM (https://github.com/todbot/blink1). blink(1) hardware is
released under the Creative Commons Attribution-ShareAlike 3.0 license
(CC BY-SA 3.0, https://creativecommons.org/licenses/by-sa/3.0/), OSHW US000051.

The remaining boards are unmodified copies harvested from public GitHub
repositories, retained here solely as small importer regression fixtures:

- `rocketgps.brd` from https://github.com/KevWal/RocketGPS
- `boomchak.brd` from https://github.com/mengstr/BoomChak
- `turnemoff.brd` from https://github.com/mengstr/TurnEmOff

These three repositories do not declare an explicit license.
