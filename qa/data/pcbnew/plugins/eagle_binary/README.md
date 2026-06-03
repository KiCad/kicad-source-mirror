# Eagle binary (.brd) test boards

Real pre-v6 binary Eagle board files used to regression-test the binary branch
of `PCB_IO_EAGLE`.

| File              | Eagle binary magic | Format era            |
|-------------------|--------------------|-----------------------|
| `blink1_b1a.brd`  | `0x10 0x00`        | v4/v5 (with DRC+notes)|
| `blink1_v1a.brd`  | `0x10 0x80`        | v3 (no DRC/notes)     |

## Source and license

Both boards are from the blink(1) open-source hardware project by ThingM
(https://github.com/todbot/blink1). blink(1) hardware is released under the
Creative Commons Attribution-ShareAlike 3.0 license (CC BY-SA 3.0,
https://creativecommons.org/licenses/by-sa/3.0/), OSHW US000051.
