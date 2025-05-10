# Allegro PCB importer worklog

The purpose of this work log is to record the development process of the KiCad Allegro importer for the purposes of demonstrating that my development did not use:

* non-free copyrighted information
* An installation of Allegro
* other non-public information

The materials used for this work are:

* A Kaitai Structs format description of the file format
    * This file was produced from the C++ struct layout of a previous GPL implementation
    * This file was written by John Beard
    * As a simple description of a format, the information is not subject to copyright other than in the presentation in the file itself.
* Open source Allegro project exports done by a third party (using the Altium conversion script from https://e2e.ti.com/support/processors-group/processors/f/processors-forum/1050931/faq-sk-tda4vm-can-somebody-provide-the-pcb-schematics-and-layout-from-sprr438-in-altium-format):
    * BeagleBone AI: https://github.com/beagleboard/beaglebone-ai
    * CutiePi v2: https://github.com/cutiepi-io/cutiepi-board
    * PreAmp: https://github.com/radualexandrub/ElectronicDevices-Circuits
    * Kinoma Create: https://github.com/Kinoma/kinoma-create-pcb

## Worklog

### 2025-05-05

* Started work on Allegro importer from the Kaitai Structs format description.
* The Kaitai Structs format description is located in `pcbnew/pcb_io/allegro/allegro_pcb.ksy`.

* The general import process will be:
  * Read the Allegro file into structures that represent the raw information in the file.
  * Convert the ensemble of these structures into KiCad objects.
* The outline of the plugin layout:
  * `allegro_stream`: provides a stream interface to decode primitive types from the Allegro file.
  * `allegro_pcb_structs`: provides low-level structures (A-Structs) that represent objects found in the Allegro file. Mostly these will follow the Kaitai Structs format description.
  * `allegro_parser`: will deal with at least converting the Allegro file into the `allegro_pcb_structs` A-structs. It may or may not later also handle the conversion to KiCad objects.

* A simple CLI tool to read the Allegro file and explore the contents of the structures is created under `utils/cli/allegro_cli`.

* Generally, A-Structs will be a 1:1 representation of the Allegro file format, as even unknown fields will read in because this may help later with debugging and understanding the file format.

* Version dependent fields use class `COND_FIELD` with a runtime version check. This documents the version dependency in the struct format.

* Implemented structs against the PreAmp project (file version 16.6) as a first step:
    * 0x06, 0x07, 0x0F, 0x10, which gets to offset 0x2a98 in the file.
    * 0x2B, 0x2D, 0x1B (nets) gets to 0x3B00

### 2025-05-06

* Implemented structs:
    * 0x1C (padstacks) and substructs - gets to 0x4e40 (next is 0x21 type)
    * 0x21 (headered block) - gets to 0xa9dc (next is 0x1D type)
    * 0x39, 0x3A - brings CutiePi up to 0x28e2c (next is 0x09)
    * 0x09 - brings CutiePi up to 0x28e58 (next is 0x0D)
    * 0x36 - some kind of of multi-type graphics (PreAmp to 0x130d4, next is 0x2a)
    * 0x2A - brings both PreAmp and CutiePi to the next 0x0D (pad)
    * 0x0D - brings PreAmp to 0x000276e0 (next is 0x0C)
    * 0x0C, 0x34 - brings PreAmp to 0x000277d4 (next is 0x08)
    * 0x08 - brings PreAmp to 0x000277ec (next is 0x11)
    * 0x11 - brings PreAmp to 0x00027834 (next is 0x32)
    * 0x26 - bring CutiePi to 0x28ee4 (next is 0x31)
    * 0x32 (placed pad) - brings PreAmp to 0x278cc (next is 0x12)
    * 0x12 - brings PreAmp to 0x00027904 (next is 0x04)
    * 0x04 - brings PreAmp to 0x0002c334 (next is 0x30)
    * 0x31 - brings CutiePi 0x28f00 (next is 0x34)
    * 0x34 - brings CutiePi to 0x28f20 (next is 0x30)
    * 0x30 - brings PreAmp to 0x2c438 (next is 0x14), CutiePi to 0x0002930c (next is 0x28)
    * 0x14 - brings PreAmp to 0x0002c6b8 (next is 0x28)
    * 0x28 - brings PreAmp to 0x0002e004 (next is 0x24), CutiePi to 0x00046b38 (next is 0x3C)
    * 0x24 - brings PreAmp to 0x00030910 (next is 0x0E)
    * 0x0E, 0x3B, 0x3C - brings PreAmp to 0x0004119c (next is 0x2E)
    * And finally all the rest of the objects, and a few tidy ups of mistakes, mostly in the versioning (I mistook 172's code for 174's)
    * Fix the last byte reading.

#### 2025-05-07

* Added a CLI tool based on Kaitai and Python, as it's a little awkward to use the C++ version experimentally now we will start wiring it into a rela parser

* Start to look at the linked lists in the header. These seem to be the key to the high-level structure of the file.
  * what was `ll_unknown_4` seems to be all 0x36 (film), for example
  * ll_unknown_5: seems to have a key that doesn't exist in the file (PreAmp, CutiePi), but is 0x0A (DRC?) in the BeagleBone AI board.
  * ll_unknown_6/7: seems to be null always.

* Improve decoding of 0x1B, 0x1C, 0x24, 0x28, 0x03 in the Python CLI tool.
  * Signed co-ord format in some of these seems to be x0, y1, x1, y1

* 0x11 and 0x08 seem to be some kind of pair of concepts. They appear to be related to the PIN_NAME in the .alg output. E.g. EMITTER:

```
S!SOT23!PACKAGE!2N2221_SOT23_QBC846B!Q2!PIN!TOP!259 1!FIG_RECTANGLE!6!635!777!20!17!!!!!!!1!EMITTER!UNSPEC!SMD20_17!FIG_RECTANGLE!REGULAR!!!!N130491!635!777!
```

* 0x10 unsure, something to do with 0x07, 0x12, and 0x0F. And strings like F0, F101, etc. And an 0x03, which has strings that might be schematics (symbols?). E.g. @preampl_schem.schematic1(sch_1):page1_ins20635@discrete.\r.normal\(chips).

* 0x07 is something to do with a footprint:
  * ptr -> 0x10, which has ptr -> 0x0F
  * ptr -> 0x06 in >= V17.2 (presumably a backlik, haven't looked yet)
  * ptr -> 0x32 = placed pad (maybe a list of them, there's a next field?)
  * ptr -> 0x2D = something with a position
  * has a refdes string e.g. 'R6'

* 0x2D looks like a real footprint instance - it has a position and:
  * 0x07 (KSY calls this inst_ref)
  * 0x14 (list of segments, 0x14 has a next)
  * 0x30 (text? refdes?)
  * 0x28 (shape - courtyard?)
  * 0x2D/0x2B - a chain of other instances ending in 0x2B

* Add a decoding walker for the x2B list

* Tables are 0x2C and seem to refer to some kind of group/list in 0x37 which then refer to graphics.

* Started Allegro PCB_IO plugin as we have enough details to start an attempt at importing.

### 2025-05-08

* More work on the framework of the Allegro PCB_IO plugin.

* Started integrating the 0x2B / 0x2D info into the Allegro PCB_IO plugin as FOOTPRINTS, but need to decode text better first to get the refdes.

## 2025-05-09

* Following the 0x14 pointer from a 0x2D allows to get some graphic lines importing to Kicad.
* The layers seem to be related to 0x2A, based on the string "DIMENSIONS" in the Altium import, which is in the string table in PreAmp and only referred to by one 0x2A.

* Fixed wrong offset in the header layer map

* Fixed wrong version in 0x1E that broke Kinoma (which is ver 0x00130c00)

* So we seem to have:
```
Header map: list of:
  Some ID:  e.g. 0, 6, 4, 1, 8, 14, 19
  Pointer to 0x2A or null
  
IDs can repeat within the map, so they're not unique. Null pointers don't necessarily have ID = 0.

For example, preamp has these header maps to 0x2A. Multiple entires point to the same 0x2A:

    Index   ID    0x2A Ptr
  - 0x00:    0 -> 0
  - 0x01: 0x13 -> [NCROUTE_PATH]
  - 0x02: 0x08 -> 0
  - 0x03: 0x08 -> 0
  - 0x04: 0x06 -> 0
  - 0x05: 0x08 -> [TOP, BOTTOM]
  - 0x06: 0x01 -> [TOP, BOTTOM]
  - 0x07: 0x0e -> [NCLEGEND-1-1, NCLEGEND-1-2, NCLEGEND-BL-1-2]
  - 0x08: 0x06 -> 0
  - 0x09: 0x12 -> [DIMENSIONS]
  - 0x0a: 0x01 -> 0
  - 0x0b: 0x03 -> 0
  - 0x0c: 0x06 -> [TOP, BOTTOM]
  - 0x0d: 0x08 -> 0
  - 0x0e: 0x01 -> 0
  - 0x0f: 0x01 -> [TOP, BOTTOM]
  - 0x10: 0x08 -> 0
  - 0x11: 0x08 -> 0
  - 0x12: 0x06 -> [TOP, BOTTOM]
  - 0x13: 0x01 -> [TOP, BOTTOM]
  - 0x14: 0x01 -> [TOP, BOTTOM]
  - 0x15: 0x01 -> [TOP, BOTTOM]
  - 0x16: 0x04 -> [TOP, BOTTOM]
  - 0x17: 0x06 -> [TOP, BOTTOM]
  - 0x18:    0 -> [TOP, BOTTOM]
  
CutiePi:

  - 0x00:    0 -> 0
  - 0x01  0x13 -> [NCROUTE_PATH, DXF_TOP, DXF_BOTTOM, ENIG_TOP, ENIG_BOTTOM, PAGELINE, FP_QAMARK, DXF0228, DRILL_HOLE_SHAPE, QA_CHECK_PCB_TEMP, DXF1012, DXF1014A, DXF1014B, CUTOUT, DESIGN_CUTLINE, QT_CHECKPCB_TEMP]
  - 0x02: 0x08 -> 0
  - 0x03: 0x08 -> 0
  - 0x04: 0x06 -> [CONSTRUCTION] (not in ASCII export?)
  - 0x05: 0x08 -> [TOP, GND02, L03, L04, GND05, BOTTOM]
  - 0x06: 0x01 -> [TOP, ...]
  - 0x07: 0x0e -> [IPC_TOP, IPC_GND02, IPC_L03, IPC_L04, IPC_VCC05, IPC_BOTTOM, IMM_GLD_TOP, IMM_GOLD_BOTTOM, OSP_TOP, OSP_BOTTOM, NCLEGEND-1-6]
  - 0x08: 0x06 - 0
  - 0x09: 0x12 - [SYMDIM_TOP]
  - 0x0a: 0x01 - 0
  - 0x0b: 0x03 - 0
  - 0x0c: 0x06 - [TOP, ...]
  - 0x0d: 0x08 - 0
  - 0x0e: 0x01 - 0
  - 0x0f: 0x01 - [TOP, ...]
  - 0x10: 0x08 - 0
  - 0x11: 0x08 - 0
  - 0x12: 0x06 - [TOP, ...]
  - 0x13: 0x01 - [TOP, ...]
  - 0x14: 0x01 - [TOP, ...]
  - 0x15: 0x01 - [TOP, ...]
  - 0x16: 0x04 - [TOP, ...]
  - 0x17: 0x06 - [TOP, ...]
  - 0x18:    0 - [TOP, ...]
  
BeagleBone:

  - 0x09: 0x12 - [LEADS, MECH_DIMENSION, REWORK_PKG_BOTTOM, REWORK_PKG_TOP, MFG_KEEPOUT_TOP, SYMBOL_INFO, LIB_REV, DIMENSION, VIACAP_TOP, PART_DIM, TEST_TOP, TEST_BOTTOM]
 
0x2a only has a list of entries. In older versions and entry is just a string. In newer versions there are some flags. Presumably then the string is the only crucial thing here.

Things like 0x14 have a layer info that have a "family" and an "ordinal". The ordinal can be small (0, 10), or often large (247, 253, 253)

Family is a small number like:
 
- In 0x14 / Segment:  0, 1, 6, 7, 9.
- In 0x0a / DRC:      5
- In 0x23 / ratline:  1
- In 0x24 / rect:     1, 9
- In 0x28 / shape:    1, 6

0x28s often point to 0x06 (KSY calls this copper) with ordinals in the range of the copper layer count (0 - 5).

The 0x09 map entry (-> [DIMENSION], [SYMDIM_TOP], [LEADS, ...., DIMENSION]) seems to be for auxiliary data. But also seems to be used by silkscreen in footprints?
```

* The "Cadence FP" type seems to be a word swapped double:

```
S!RES2012X50N_0805!PACKAGE!RESISTOR_RES2012X50N_0805_1K!R16!PACKAGE GEOMETRY!PLACE_BOUND_TOP!483 1!ARC!256!1274!540!1274!540!1260!540!14!2!COUNTERCLOCKWISE!NOTCONNECT!!!!!!!!!!!!!

  x           : 0x4093b000, 0  (1260)
  y           : 0x4080e000, 0  (540)
  r           : 0x402c0000, 0  (14)
```
