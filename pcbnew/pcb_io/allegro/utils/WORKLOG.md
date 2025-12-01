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

* Decoding all the lines in 0x14 pointer of R16 in PreAmp (0x2B = 0xF790220 -> 0x2D 0xf790550)
  * I think 0x17 is a vertical line, 0x15 is horizontal. but they both have full coords.
  * They all have layer family = 0x09, but, these are the ASCII file layer names:
    * 0xF2 -> DISPLAY_TOP (w = 0, slightly outside ASSEMBLY_TOP)
    * 0xFB -> PLACE_BOUND_TOP (central bullseye)
    * 0xFD -> ASSEMBLY_TOP (looks like fab outline: 80x50 mil for 0805 resistor)
    * 0xF7 -> SILKSCREEN_TOP (silkscreen line)

* Looking at https://www.artwork.com/all2dxf/alleggeo.htm, it then seems that family 9 is PACKAGE_GEOMETRY, so the layer info "family" is the CLASS, and the ordinal might be the subclass.

* The PLACE_BOUND_TOP and DFA_BOUND_TOP polys are not in the 0x14 list, they're in the 'ptr_4' one as 0x28s
  * List is two nodes long:
    * 0x28 0xf7ceae8 - Layer 9:0xfb -> PLACE_BOUND_TOP
    * 0x28 0xf7ceae8 - Layer 9:0xef -> DFA_BOUND_TOP (presumably)

* Presumably, then 0x6 is class = ETCH and the subclass is the copper layer index.

* It's still not quite clear how to get to the layer name - presumably there's a fixed mapping into the header layer map somehow - the same entries largely have the same functions between all the files.

* It looks like 0x15 is H line, 0x17 is V, and 0x16 is neither, so we can just merge the handling.

* Arcs can now be imported, following some logic from the Fabmaster importer. For start = end, we can just use a circle anyway.

## 2025-05-11

* Broke out 0x14 builder function
* Fixed Kinoma import (some wrong versions for V_164 boards)

* Text looks like there's a reference to something for the font

```
S!LINEAR_DIMENSION!DRAFTING!!!BOARD GEOMETRY!DIMENSION!461 1!TEXT!260!-278!752!0.000!NO!LEFT!9 0 35 30 0.000 25 100 2!1.60!!!!!!!!!!!!!!!!

looks like the entry in the 0x36 object with c = 0x08:

  - Item 8
    char_height : 35
    char_width  : 30
    a           : 0
    b           : 0
    xs          : [25, 100, 0, 2]

in the 0x30, we see

  txt props key: 0x9
  txt props flags: 0x0
  txt props align: TextAligmnent.left (= LEFT above?)
  txt props rev: TextReversal.straight (= NO above?)

S!RES2012X50N_0805!PACKAGE!RESISTOR_RES2012X50N_0805_1K!R16!REF DES!ASSEMBLY_TOP!490 1!TEXT!260!1260!520!0.000!NO!CENTER!17 0 24 18 0.000 4 8 3!R16!!!!!!!!!!!!!!!!

looks like:

  - Item 16
    char_height : 24
    char_width  : 18
    a           : 0
    b           : 0
    xs          : [4, 8, 0, 3]
```
* So the first number (9, 17) is the 0x36 index + 1?

## 2025-05-12

* A bit more work on decoding texts

* Looks like the SUBCLASS uint8 isn't the same for all CLASSes: PACKAGE_GEOMETRY:SILKSCREEN_TOP and REFDES:SILKSCREEN_TOP are 0x09:0xF7 and 0x0d:0xFB. Annoying.

* 0x2D:
  * -inst_ref -> 0x07:
    * Has refdes str
    * 0x10 (ptr_2)
      * 0x07 (same as 0x2D's)
      * 0x12
        * 0x12 (next)
        * 0x11
        * 0x32 (same as 0x2D's)
      * 0x0F
        * Str: 'G1;
        * C_CAPC2008X126N_0805_2.2U
        * 0x06
        * 0x11 (not the same as the 0x12)
      * 0x03
        * subtype: 0x68
        * str:  @preampl_schem.schematic1(sch_1):page1_ins13791@discrete.\c.normal\(chips)
      * Str: 'F31'
  * 0x32 (pads)


* Looking at 0x14s, we find some more layer mappings:
  * 7:1 - MANUFACTURING:NCLEGEND-1-2

## 2025-05-13

* Net decoding and importing
* Still not quite clear why 0x04s are in single-entry lists - don't nets have many member objects?

* 0x05s seem like lists of objects. Maybe they're just tracks and we can get the pads/zones elsewhere?

* Layer map seems to be indexed directly by CLASS, so then we know where to pull out layer names for the copper layers.

* Stackup seems to be in one of the 0x21 objects:
```
Hex View  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F

00009EE0                           21 00 18 05 B4 00 00 00          !.......
00009EF0  70 2E 79 0F FD 00 00 00  00 00 00 00 41 49 52 00  p.y.........AIR.
00009F00  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
00009F10  30 20 4D 49 4C 00 30 30  30 30 30 30 30 30 65 2B  0 MIL.00000000e+
00009F20  30 30 30 00 30 20 77 2F  63 6D 2D 64 65 67 43 00  000.0 w/cm-degC.
00009F30  30 30 65 2D 30 30 32 00  30 20 6D 68 6F 2F 63 6D  00e-002.0 mho/cm
00009F40  00 30 30 30 30 30 65 2B  30 30 30 00 31 00 30 30  .00000e+000.1.00
00009F50  30 30 30 30 30 30 30 30  30 30 65 2B 30 30 30 00  0000000000e+000.
00009F60  53 55 52 46 41 43 45 00  00 00 00 00 00 00 00 00  SURFACE.........

A!LAYER_SORT!LAYER_SUBCLASS!LAYER_ARTWORK!LAYER_USE!LAYER_CONDUCTOR!LAYER_DIELECTRIC_CONSTANT!LAYER_ELECTRICAL_CONDUCTIVITY!LAYER_MATERIAL!LAYER_THERMAL_CONDUCTIVITY!LAYER_THICKNESS!
J!C:/All_Preamp.brd!Fri May 02 14:59:47 2025!-560!-410!10440!8090!1!mils!PREAMPL_SCHEM!59.940157 mil!2!OUT OF DATE!
S!000000!!!!NO!1.000000!0 mho/cm!AIR!!0 mil!
S!000001!TOP!POSITIVE!!YES!4.500000!595900 mho/cm!COPPER!!0.72 mil!
S!000002!!!!NO!4.500000!0 mho/cm!FR-4!!58.500000 mil!
S!000003!BOTTOM!POSITIVE!!YES!4.500000!595900 mho/cm!COPPER!!0.72 mil!
S!000004!!!!NO!1.000000!0 mho/cm!AIR!!0 mil!
```

* Figureed out several layer maps by comparing unmapped layers with cooridinates.
  * DRAWING FORMAT is class 4, as it have the largest numbers and seems to work
* https://www.artwork.com/all2dxf/alleggeo.htm seems to be in order. 0xFD looks like the highest in each class, and then count backwards. SO ASSEMBLY_TOP in DEVICE_TYPE is 0xFD, and ASSEMBLY_BOTTOM is 0xFC and so on.


## 2025-05-14

* More work on layers on KiCad side
* Iterate 0x1B -> 0x04 -> 0x05/0x33/0x32/0x2E lists for connected things
* 0x32 - next was wrong - there is a next for connection iteration, but another one for when iterating the footprint from 0x2B/0x2E
* 0x2E - first unknown is next, found some more pointers, but unsure what it is:
```
  Object type : 0x2e
  Key         : 0xf7cffb0
  next        : 0x0f7cf270 (0x05)
  net_ptr     : 0x0f7d0560 (0x04)
  connection  : 0x0f7cfbb8 (0x05)
  coords      : (959, 550)
  unknown_1   : 0x20
  unknown_4   : 0x0

but only these coords match:

S!ETCH!TOP!305 1!LINE!257!959!540!959!550!20!!!!!CONNECT!!!!!!!!N13829!!!!!
S!ETCH!TOP!306 1!LINE!257!959!550!959!609!20!!!!!CONNECT!!!!!!!!N13829!!!!!
S!ETCH!TOP!307 4!LINE!257!928!581!959!550!20!!!!!CONNECT!!!!!!!!N13829!!!!!
```

* One 0x2E seems to be referred to by several 0x05s (one as next and ptr2a and 3a of two directly linked 0x2Es, so 2a, 3a might be forward/backlinks)
* PreAmp only has 4 of them

* Padstacks 0x1C:
  * Comp count: 10 + 3 * layer count (this is from a field)
  * `R110_95` - count = 1 -> comps = 13:
    * SOLDERMASK_TOP, PASTEMASK_TOP, FILMMASKTOP = (47, 45) => comps 0, 5, 7
    * TOP => comp 12
  * `R345_165` - count = 1 -> comps = 13:
    * SOLDERMASK_TOP, PASTEMASK_TOP = (47, 45) => comps 0, 5
    * TOP => comp 12
  * So FILMMASKTOP = comp 7?
  * `60X100` =  SOLDERMASK_TOP is 66x106, PASTEMASK_TOP is 60x100
  * So SOLDERMASK_TOP = comp 0
    * PASTEMASK_TOP = 5

  * Looking at the ASCII headings, the three/layer is PAD, thermal relief, anti-pad (in <V172)
    * The order in the comps seems to be AP/TR, TR/AP, PAD, X (x is unknown, seems always 0x0 >=V172)
  * S62D38 in BB-AI tells use it is in order: AP (72.000), TP (null), PAD (62.000), X (unknown function)
 * TSM is index 14 in BB-AI

## 2025-09-05

* Start to consider a multi-step approach, parsing as:
   * Binary data -> Kaitai-like field structs, then
   * To a DB object, which mirrors data (presumably) in the BRD DB file
   * Then resolve inter-object links
   * Then to KiCad objects, or to an ASCII dump format

* 0x14 (graphic?) can also point to 0x01 (arc), as well as segs

* GRAPHIC_DATA_NAME/GRAPHIC_DATA_NUMBER are the same thing:
    * ARC = 256
    * LINE = 257
    * RECTANGLE = 259


## 2025-11-29

* Figured out a few more areas:
* 0x0C is something to do with FIGUREs, for example the origin cross and drill symbols
* Get a PEGTL based parser working for the ASCII export format, which can use to cross-check the board reader

* Preamp: looking at the schematic name (BOARD_SCHEMATIC_NAME), we see it occurs twice:
    *  Offset 0x0a568 - an 0x21 object with data size 1043, contains 00 00 00 00 00 00 01 01, the string and the rest 00s
    *  Offset 0x43168 - an 0x03 object subtype 0x68 (string-y) ID 0xf7d1d10
       * This is only pointed as "next" by 0x03 subtype 0x68 0xf7d28a8, which contains null string
       * So it seems to be an orphaned object, unless the other fields in the 0x03 give it special meaning

* A lot of 0x30/68 objects have content like  @preampl_schem.schematic1(sch_1):page1_ins20635@discrete.\r.normal\(chips)
  * These are presumably schematic references to instances
  * They all have 0x4 in the hdr2 field
  * hdr1 is the same for similar-ish strings

* On 0x03.68  k=0xf7d10d0 'raddu' (author?) is also an orphaned object, unless special meaning to hdr1 = 9x2db

## 2025-11-30

* Looking at COMPONENT_PIN view
* The iteratiohn doesn't seem to be in the same order as SYMBOL. Probably not important to emulate the order exactly,
  but would be nice to know exactly what is being iterated here.
  * SYMBOL seems to iterate by 0x2B then each 0x2D chain
  * ALso not ordered by NET
* In PreAmp for COMPONENT_PIN view, first 3 lines:

  S!N13116!J4!1!1!!!
  S!N13105!C1!1!1!!!
  S!N13116!C1!2!2!!!

* N13116 is in the string map, key 0x09a34ddd
* This is NOT the first in the 0x1B linked list
  * Only pointed to by 0x1B NET key 0xf790de8
    * Which has a 0x04 ptr to 0xf790de8
      * Which has next = net ptr back to 0x1B
      * And conn_item 0x05 0x0f7cac38 TRACK, which them points to 0x32 PLACED_PAD...


* BeagleBone U4: 0x0000136f

* 0x08 and 0x11 seems to be pin number and pin name respectively
* 0x10 is FUNCTION and 0x0F is SLOT

* 0x24 ptr1 is actually the parent pointer
