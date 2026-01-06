# Cadence Allegro Binary .brd Format

Reverse-engineered documentation for the Allegro PCB binary file format
as implemented in this importer. This is not an official specification.

## File Layout

```
Offset 0x0000: File Header (~4KB)
  - Magic number (4 bytes)
  - Object count
  - 22 linked list heads (block type indices)
  - Allegro version string (60 bytes)
  - Board units (1 byte: 0x01=Imperial, 0x02=Metric)
  - Units divisor (4 bytes)
  - String count
  - Layer map (256 entries)

Offset 0x1200: String Table
  - Repeated: [4-byte ID] [null-terminated string]

After strings: Object Blocks
  - Repeated: [1-byte type tag] [type-specific data]
  - Each block type has a fixed layout with version-conditional fields
```

## Version Detection

The first 4 bytes are the file magic. The lower byte is masked
to determine the format version:

| Magic         | Version | Allegro Release |
|---------------|---------|-----------------|
| `0x0013_0000` | V_160   | 16.0            |
| `0x0013_0400` | V_162   | 16.2            |
| `0x0013_0C00` | V_164   | 16.4            |
| `0x0013_1000` | V_165   | 16.5            |
| `0x0013_1500` | V_166   | 16.6            |
| `0x0014_0400` | V_172   | 17.2            |
| `0x0014_0900` | V_174   | 17.4            |
| `0x0014_1500` | V_175   | 17.5            |

Version 17.2 is the most significant layout change. Many struct fields
are conditionally present using `COND_GE<FMT_VER::V_172, T>` (present
in >= 17.2) or `COND_LT<FMT_VER::V_172, T>` (present in < 17.2).

## Coordinate System

All integer coordinates are stored in mils divided by the units divisor
(`m_UnitsDivisor` from the header). To convert to nanometers:

    scale = 25400.0 / divisor
    nm = coordinate * scale

The Y axis is negated during conversion to KiCad coordinates.

Arc center coordinates and radii use IEEE 754 doubles stored as two
big-endian 32-bit words (`ReadAllegroFloat`).

### Rotation

Rotation values are stored as millidegrees (divide by 1000 to get
degrees). The rotation is NOT negated despite the Y-axis flip (same
convention as Altium).

Pad rotation in 0x0D blocks is board-absolute, meaning it includes
the parent footprint's rotation. To get the footprint-local pad
rotation for KiCad, subtract the footprint's rotation.

## Linked Lists

Most block types form singly-linked lists via a `m_Next` field containing
the key of the next block. The header contains 22 linked list descriptors
with head and tail keys. Walking a linked list starts from the head key,
looks up each block by key, and follows `m_Next` until 0 or the tail key.

The `m_Key` field serves as a global block identifier. All blocks are
indexed by key in a flat hash map (`m_ObjectKeyMap`).

## Block Types

| Type | Struct                  | Purpose                              |
|------|-------------------------|--------------------------------------|
| 0x01 | BLK_0x01_ARC            | Arc segment (track, shape, zone)     |
| 0x03 | BLK_0x03                | Field/property reference             |
| 0x04 | BLK_0x04_NET_ASSIGNMENT | Net assignment                       |
| 0x05 | BLK_0x05_TRACK          | Track segment collection             |
| 0x06 | BLK_0x06                | Component/symbol definition          |
| 0x07 | BLK_0x07                | Footprint instance reference data    |
| 0x08 | BLK_0x08                | Pin number                           |
| 0x09 | BLK_0x09                | Intermediate (fill-to-shape link)    |
| 0x0A | BLK_0x0A_DRC            | DRC error marker                     |
| 0x0C | BLK_0x0C                | Pin definition                       |
| 0x0D | BLK_0x0D_PAD            | Pad geometry and placement           |
| 0x0E | BLK_0x0E                | Shape/fill segment                   |
| 0x0F | BLK_0x0F                | Function slot reference              |
| 0x10 | BLK_0x10                | Function instance reference          |
| 0x11 | BLK_0x11                | Pin name                             |
| 0x12 | BLK_0x12                | Unknown                              |
| 0x14 | BLK_0x14                | Graphics container (lines, arcs)     |
| 0x15 | BLK_0x15_16_17_SEGMENT  | Line segment (tracks, outlines)      |
| 0x16 | (same as 0x15)          | Line segment variant                 |
| 0x17 | (same as 0x15)          | Line segment variant                 |
| 0x1B | BLK_0x1B_NET            | Net definition                       |
| 0x1C | BLK_0x1C_PADSTACK       | Padstack definition                  |
| 0x1D | BLK_0x1D                | Design rule                          |
| 0x1E | BLK_0x1E                | Design rule variant                  |
| 0x1F | BLK_0x1F                | Design rule variant                  |
| 0x21 | BLK_0x21                | Headered block (rules, stackup)      |
| 0x22 | BLK_0x22                | Unknown                              |
| 0x23 | BLK_0x23_RATLINE        | Ratsnest line                        |
| 0x24 | BLK_0x24_RECT           | Rectangle shape                      |
| 0x26 | BLK_0x26                | Unknown                              |
| 0x27 | BLK_0x27                | Unknown                              |
| 0x28 | BLK_0x28_SHAPE          | Polygon shape (zones, fills, pads)   |
| 0x29 | BLK_0x29_PIN            | Pin instance in .dra files           |
| 0x2A | BLK_0x2A_LAYER_LIST     | Layer list entry                     |
| 0x2B | BLK_0x2B                | Component/symbol reference           |
| 0x2C | BLK_0x2C_TABLE          | Table/lookup structure               |
| 0x2D | BLK_0x2D                | Footprint instance (placed part)     |
| 0x2E | BLK_0x2E                | Connection/ratsnest                  |
| 0x2F | BLK_0x2F                | Unknown                              |
| 0x30 | BLK_0x30_STR_WRAPPER    | Text object (wrapped string)         |
| 0x31 | BLK_0x31_SGRAPHIC       | String graphic content               |
| 0x32 | BLK_0x32_PLACED_PAD     | Placed pad instance                  |
| 0x33 | BLK_0x33_VIA            | Via instance                         |
| 0x34 | BLK_0x34_KEEPOUT        | Keepout area                         |
| 0x35 | BLK_0x35                | Design constraints                   |
| 0x36 | BLK_0x36                | Font/misc definitions (substructs)   |
| 0x37 | BLK_0x37                | Pointer array (net resolution, etc.) |
| 0x38 | BLK_0x38_FILM           | Film definition                      |
| 0x39 | BLK_0x39_FILM_LAYER_LIST| Film layer list                      |
| 0x3B | BLK_0x3B                | Unknown                              |
| 0x3C | BLK_0x3C                | Unknown                              |

## Arc Direction (0x01)

The `m_SubType` field encodes arc sweep direction:
- Bit 6 clear (value 0): counter-clockwise sweep
- Bit 6 set (value 0x40): clockwise sweep

Only values 0 and 64 (0x40) have been observed.

## Padstack Layout (0x1C)

A padstack contains fixed-slot components followed by per-layer
components (`m_LayerCount * m_NumCompsPerLayer`).

### Drill Diameter

The drill field location is version-dependent:

- Pre-V172: `m_Drill` field directly in the padstack header, in
  internal coordinate units (mils * divisor).
- V172+: `m_DrillArr[4]` holds the drill width, `m_DrillArr[7]`
  holds the drill height. For round drills, height is 0 (treated as
  equal to width). The old `m_Drill` field still exists in V172+ but
  does NOT contain the drill diameter.

### Fixed Slots (Technical Layers)

The number of fixed slots varies by version: 10 for pre-V172, 21 for
V172+. Verified slot assignments:

Pre-V172 (10 fixed):

| Slot | Layer                    |
|------|--------------------------|
| 0    | Top solder mask (~TSM)   |
| 5    | Top paste mask (~TPM)    |
| 7    | Top film mask (~TFM)     |

V172+ (21 fixed):

| Slot | Layer                    |
|------|--------------------------|
| 14   | Top solder mask (~TSM)   |
| 15   | Bottom solder mask       |

### Per-Layer Components

Per-layer component order (`m_NumCompsPerLayer` entries per layer):
- Index 0: Antipad (clearance)
- Index 1: Thermal relief
- Index 2: Pad shape

### Thermal Relief

The thermal relief component (per-layer index 1) stores the outer
extent of the thermal pattern in m_W/m_H, not the spoke width. The
thermal gap is derived from the antipad-to-pad size difference:

    thermal_gap = (antipad.W - pad.W) / 2

### Pad Shape Types

Each PADSTACK_COMPONENT has a `m_Type` field selecting the shape:

| Value | Shape               | KiCad Mapping        |
|-------|---------------------|----------------------|
| 0x00  | Null (no pad)       | (skipped)            |
| 0x02  | Circle              | PAD_SHAPE::CIRCLE    |
| 0x03  | Octagon             | CHAMFERED_RECT       |
| 0x04  | Cross               | (unhandled)          |
| 0x05  | Square              | PAD_SHAPE::RECTANGLE |
| 0x06  | Rectangle           | PAD_SHAPE::RECTANGLE |
| 0x07  | Diamond             | (unhandled)          |
| 0x0B  | Oblong X            | PAD_SHAPE::OVAL      |
| 0x0C  | Oblong Y            | PAD_SHAPE::OVAL      |
| 0x0F  | Hexagon X           | (unhandled)          |
| 0x10  | Hexagon Y           | (unhandled)          |
| 0x12  | Triangle            | (unhandled)          |
| 0x16  | Shape Symbol        | PAD_SHAPE::CUSTOM    |
| 0x17  | Flash               | (unhandled)          |
| 0x19  | Donut               | (unhandled)          |
| 0x1B  | Rounded Rectangle   | PAD_SHAPE::ROUNDRECT |
| 0x1C  | Chamfered Rectangle | CHAMFERED_RECT       |
| 0x1E  | N-Sided Polygon     | (unhandled)          |

For shape symbols (0x16), the `m_StrPtr` field points to a 0x28
polygon block whose segment list defines the custom pad outline.

## Polygon Shapes (0x28)

A 0x28 shape has a `m_FirstSegmentPtr` which begins a linked list of
segment blocks (0x01 arcs and 0x15/0x16/0x17 line segments). These
form the polygon outline.

Used for: zone outlines, copper fills, board outline, keepout areas,
and custom pad shapes.

### Zone Outlines vs Copper Fills

0x28 shapes appear in two distinct contexts:

1. On the net assignment chain (reachable from 0x1B NET via 0x04/0x05):
   these are computed copper fills. KiCad recomputes fills from zone
   outlines, so these are not imported.

2. On the `m_LL_Shapes` header linked list with BOUNDARY class (0x15):
   these are zone outlines and are imported as ZONE objects.

### Zone Net Resolution

To find the net for a BOUNDARY shape, follow this pointer chain:

```
BOUNDARY 0x28 shape
  └─ m_Ptr7 (V172+) or m_Ptr7_16x (pre-V172)
       └─ 0x2C TABLE
            └─ m_Ptr1
                 └─ 0x37 pointer array
                      └─ m_Ptrs[0]
                           └─ 0x1B NET block
```

The `m_Ptr7` field is version-conditional: `m_Ptr7` exists in V172+,
`m_Ptr7_16x` exists in pre-V172. One or the other will be populated.

The 0x37 pointer array's `m_Ptrs[0]` holds the key of the 0x1B NET
block that owns this zone. The `m_Count` field indicates how many
entries are valid in the array.

## Board Outline

The board outline is stored as BOUNDARY class or BOARD_GEOMETRY /
DRAWING_FORMAT class shapes with specific subclass codes:

| Subclass Code | Name          | Usage                          |
|---------------|---------------|--------------------------------|
| 0xFD          | DFMT_OUTLINE  | Most common outline identifier |
| 0xEA          | BGEOM_OUTLINE | Alternative outline identifier |

Both codes appear under either BOARD_GEOMETRY (0x01) or
DRAWING_FORMAT (0x04) classes. The `IsOutlineLayer()` check must
handle both.

The outline geometry is a linked list of segments (0x15/0x16/0x17
lines and 0x01 arcs) forming a closed contour on the Edge.Cuts layer.

## Layer Encoding

Each block has a 2-byte `LAYER_INFO` consisting of a class code and
subclass code.

### Layer Classes

| Code | Class            | Purpose                        |
|------|------------------|--------------------------------|
| 0x01 | BOARD_GEOMETRY   | Board-level features           |
| 0x02 | COMPONENT_VALUE  | Component value text           |
| 0x03 | DEVICE_TYPE      | Device type text               |
| 0x04 | DRAWING_FORMAT   | Drawing annotations            |
| 0x05 | DRC_ERROR        | DRC error markers              |
| 0x06 | ETCH             | Copper layers                  |
| 0x07 | MANUFACTURING    | Manufacturing features         |
| 0x08 | ANALYSIS         | Analysis features              |
| 0x09 | PACKAGE_GEOMETRY | Footprint-level geometry       |
| 0x0A | PACKAGE_KEEPIN   | Package keepin                 |
| 0x0B | PACKAGE_KEEPOUT  | Package keepout                |
| 0x0C | PIN              | Pin features                   |
| 0x0D | REF_DES          | Reference designator text      |
| 0x0E | ROUTE_KEEPIN     | Route keepin                   |
| 0x0F | ROUTE_KEEPOUT    | Route keepout region           |
| 0x10 | TOLERANCE        | Tolerance text                 |
| 0x11 | USER_PART_NUMBER | User part number text          |
| 0x12 | VIA_CLASS        | Via class features             |
| 0x13 | VIA_KEEPOUT      | Via keepout region             |
| 0x14 | ANTI_ETCH        | Anti-etch (negative copper)    |
| 0x15 | BOUNDARY         | Zone boundary outlines         |

### Subclass Codes

The subclass meaning depends on the class. High values (>= 0xEA) are
fixed reserved codes; low values are indices into the per-class custom
layer list from the header's layer map.

Fixed subclass codes used by multiple classes:

| Code | Used By                            | Meaning           |
|------|------------------------------------|--------------------|
| 0xEA | BOARD_GEOMETRY                     | Board outline      |
| 0xEE | PACKAGE_GEOMETRY                   | DFA bound bottom   |
| 0xEF | PACKAGE_GEOMETRY                   | DFA bound top      |
| 0xF1 | PACKAGE_GEOMETRY                   | Display bottom     |
| 0xF2 | PACKAGE_GEOMETRY                   | Display top        |
| 0xF3 | MANUFACTURING                      | Autosilk bottom    |
| 0xF4 | MANUFACTURING                      | Autosilk top       |
| 0xF5 | PACKAGE_GEOMETRY                   | Body center        |
| 0xF6 | PACKAGE_GEOMETRY                   | Silkscreen bottom  |
| 0xF7 | PACKAGE_GEOMETRY                   | Silkscreen top     |
| 0xF8 | REF_DES, COMPONENT_VALUE, etc.     | Display bottom     |
| 0xF9 | REF_DES, COMPONENT_VALUE, etc.     | Display top        |
| 0xFA | REF_DES, COMPONENT_VALUE, etc.     | Silkscreen bottom  |
| 0xFA | PACKAGE_GEOMETRY                   | Place bound bottom |
| 0xFB | REF_DES, COMPONENT_VALUE, etc.     | Silkscreen top     |
| 0xFB | PACKAGE_GEOMETRY                   | Place bound top    |
| 0xFC | REF_DES, COMPONENT_VALUE, etc.     | Assembly bottom    |
| 0xFC | PACKAGE_GEOMETRY                   | Assembly bottom    |
| 0xFD | REF_DES, COMPONENT_VALUE, etc.     | Assembly top       |
| 0xFD | DRAWING_FORMAT                     | Outline            |
| 0xFD | PACKAGE_GEOMETRY                   | Assembly top       |

For ETCH class, subclass values 0..N-1 index the copper layers in
stackup order (0 = top, N-1 = bottom).

### Custom Layer Lists

The header layer map contains a 0x2A LAYER_LIST pointer for each
class. Multiple classes can share the same list. The subclass byte
indexes into this list for low-valued subclass codes. Each entry
provides a layer name string.

## Footprints (0x2B / 0x2D)

### 0x2B: Footprint Definition

A template that multiple placed instances share.

### 0x2D: Placed Footprint Instance

An actual footprint on the board.

- `m_Layer`: 0 = top (F_Cu), 1 = bottom (B_Cu)
- `m_Rotation`: Angle in millidegrees
- `m_CoordX`, `m_CoordY`: Board position
- `m_InstRef` (V172+) or `m_InstRef16x` (pre-V172): Key to 0x07
  block containing instance data (reference designator, etc.)
- `m_GraphicPtr`: Head of 0x14 graphics linked list
- `m_FirstPadPtr`: Head of 0x32 placed pad linked list
- `m_TextPtr`: Head of 0x30 text linked list

Bottom-layer footprints (`m_Layer == 1`) must be flipped. In KiCad,
call `Flip()` AFTER adding all children (graphics, text, pads) so
that child layers and positions are mirrored correctly.

## Pad Placement (0x0D)

The 0x0D block holds per-pad geometry:

- `m_CoordsX`, `m_CoordsY`: Pad position in board coordinates
- `m_PadStack`: Key to 0x1C padstack
- `m_Rotation`: Pad rotation in millidegrees, board-absolute

The rotation stored in 0x0D includes the parent footprint's rotation.
For KiCad, which expects footprint-local pad rotation, subtract the
footprint's rotation:

    local_rotation = pad_rotation - footprint_rotation

## Font Definitions (0x36)

The 0x36 block contains a heterogeneous list of substruct variants.
The `FontDef_X08` variant provides font metrics:

- `m_CharHeight`: Character height in internal units
- `m_CharWidth`: Character width in internal units

Text objects (0x30) reference a font definition by 1-based index
into the cached list of FontDef_X08 entries.

## String Table

Located at fixed offset 0x1200. Each entry is a 4-byte ID followed
by a null-terminated string. String IDs are used throughout the file
to reference net names, pad names, refdes values, and other text.

## Keepout Areas

Keepout shapes appear on the `m_LL_0x24_0x28` header linked list
with ROUTE_KEEPOUT (0x0F) or VIA_KEEPOUT (0x13) class. They share
the 0x28 shape structure and are converted to KiCad ZONE objects
with appropriate keepout flags.
