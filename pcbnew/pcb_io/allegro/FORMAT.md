# Cadence Allegro Binary .brd Format

Reverse-engineered documentation for the Allegro PCB binary file format
as implemented in this importer. This is not an official specification.

## File Layout

```
Offset 0x0000: File Header (~4KB)
  - Magic number (4 bytes)
  - Object count
  - Linked lists: 22 for pre-V180, 28 for V180+
  - Allegro version string (60 bytes)
  - Board units (1 byte: 0x01=Imperial, 0x02=Metric)
  - Units divisor (4 bytes)
  - String count (explicit field pre-V180, embedded in Unknown2 for V180)
  - Layer map (256 entries)

Offset 0x1200: String Table
  - Repeated: [4-byte ID] [null-terminated string]

After strings: Object Blocks
  - Repeated: [1-byte type tag] [type-specific data]
  - Each block type has a fixed layout with version-conditional fields
  - V180 files may have zero-padded gaps between block groups
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
| `0x0015_0000` | V_180   | 18.0+           |

Version 17.2 is the most significant layout change for structs. Many
fields are conditionally present using `COND_GE<FMT_VER::V_172, T>`
(present in >= 17.2) or `COND_LT<FMT_VER::V_172, T>` (present in
< 17.2).

Version 18.0 is the most significant layout change for the file header.
See the V18 Header Changes section for details.

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

## Header

Every file as a header block which is around 4kB in size. The header contains
global file metadata and a set of linked list descriptors that point to the
top-level blocks for each major category of data.

Versions 16 and 17 have the same (or at least very similar) header layout,
while version 18 introduces significant changes.

* `m_Magic` - file magic number used for version detection.
* `m_Unknown1a` - this always seems to be 0x03
* `m_FileRole` - this seems to be:
  * `0x01`: .brd files
  * `0x02`: .dra footprint files
* `m_Unknown1b` - this always seems to be 0x03
* `m_writerProgram`
  * `0x00000009`: Allegro editor
  * `0x00130000`: DB Doctor utility

The next block of 7 fields seems to have different meanings in v18 compared to
earlier versions.

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
| 0x1D | BLK_0x1D                | Physical constraint set (see below)  |
| 0x1E | BLK_0x1E                | Signal integrity model (IBIS data)   |
| 0x1F | BLK_0x1F                | Linked list connector (empty)        |
| 0x20 | BLK_0x20_UNKNOWN        | Unknown                              |
| 0x21 | BLK_0x21                | Headered block (rules, stackup)      |
| 0x22 | BLK_0x22                | Unknown                              |
| 0x23 | BLK_0x23_RATLINE        | Ratsnest line                        |
| 0x24 | BLK_0x24_RECT           | Rectangle shape                      |
| 0x26 | BLK_0x26                | Match group indirection (diff pair)  |
| 0x27 | BLK_0x27                | Constraint manager cross-ref table   |
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
| 0x35 | BLK_0x35                | File references (log paths)          |
| 0x36 | BLK_0x36                | Font/misc definitions (substructs)   |
| 0x37 | BLK_0x37                | Pointer array (net resolution, etc.) |
| 0x38 | BLK_0x38_FILM           | Film definition                      |
| 0x39 | BLK_0x39_FILM_LAYER_LIST| Film layer list                      |
| 0x3B | BLK_0x3B                | Unknown                              |
| 0x3C | BLK_0x3C                | Unknown                              |

## Arcs (0x01)

## Direction Encoding

The `m_SubType` field encodes arc sweep direction:
- Bit 6 clear (value 0): counter-clockwise sweep
- Bit 6 set (value 0x40): clockwise sweep

Only values 0 and 64 (0x40) have been observed.

## Segments (0x15/0x16/0x17)

The 0x15/0x16/0x17 block types all share the same layout and represent
line segments. The only difference is the 0x15/0x16/0x17 are
horizontally/diagonally/vertically oriented segments respectively.

The parent field can be:
* 0x05: Track
* 0x14: Graphics container
* 0x20: Unknown
* 0x28: Polygon shape
* 0x34: Keepout area

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

1. On the `m_LL_Shapes` header linked list with BOUNDARY class (0x15):
   these are zone outlines and are imported as ZONE objects.

2. On the net assignment chain (reachable from 0x1B NET via 0x04/0x05):
   these have ETCH class and include both zone outlines repeated as
   copper shapes and actual computed fills. applyZoneFills() matches
   fills to zones by net code, layer, and bounding box overlap, then
   applies the genuine fills via SetFilledPolysList(). Unmatched fills
   also serve as a fallback for zone net resolution when
   resolveShapeNet() returns UNCONNECTED.

3. Also on the net assignment chain but with NO corresponding BOUNDARY
   zone: these are standalone copper polygons (Allegro "shapes" drawn
   directly on ETCH layers). They are imported as filled PCB_SHAPE
   objects with the appropriate net assignment.

   On V172+ boards, the m_Unknown2 field in 0x28 SHAPE blocks is a
   discriminator for dynamic copper. Bit 12 (0x1000) marks shapes that
   Allegro auto-generated (teardrops, via fillets, pad connection
   polygons). Observed values:

   - 0x3001: classic teardrop shapes (5 vertices, asymmetric bbox)
   - 0x1001: dynamic copper fillets at via/pad junctions
   - 0x0001: genuine standalone copper pours
   - 0x0000: pre-V172 boards (field absent, value_or(0) returns 0)

   Shapes with bit 12 set are imported as KiCad teardrop ZONE objects
   (TEARDROP_TYPE::TD_VIAPAD). Shapes without this flag are imported
   as filled PCB_SHAPE objects (standalone copper).

   After teardrop zones are created, enablePadTeardrops() finds the
   anchoring pad/via for each teardrop by checking which pad/via
   position on the same net and layer is contained within the teardrop
   outline, and calls SetTeardropsEnabled(true) on matching items.

   BeagleBone Black: 1063 teardrops (20 pads + 499 vias enabled),
   1 genuine copper pour. Pre-V172 boards: 0 teardrops.

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

## Drill Slot Orientation

Allegro stores slot drill dimensions as (primary, secondary) rather
than (X, Y). The primary dimension is always the larger value
regardless of pad orientation. For OBLONG_X pads this works naturally
(slot width > height), but for OBLONG_Y pads the raw dimensions
produce a horizontal slot instead of the intended vertical slot.

The importer corrects this by comparing the first copper layer's pad
aspect ratio (m_W vs m_H) with the drill aspect ratio (drillW vs
drillH). If they disagree on orientation, the drill dimensions are
swapped. Round drills (equal W and H) skip the check.

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

## Physical Constraint Sets (0x1D)

0x1D blocks define Allegro Physical Constraint Sets containing trace
width, clearance, and routing rules. They reside on the
`m_LL_0x1D_0x1E_0x1F` linked list in the file header.

### Record Structure

Each 0x1D block contains two record arrays:

- `m_DataA`: `m_SizeA` records of 256 bytes each. ASCII padstack/via
  name strings (null-terminated at offset 4) and file path references.
- `m_DataB`: `m_SizeB` records of 56 bytes each. Per-copper-layer
  dimension values. The count equals the board's copper layer count.

### Header Fields

| Field | Content |
|-------|---------|
| m_Key | Unique block key |
| m_Next | Linked list next pointer (used by LL_WALKER) |
| m_NameStrKey | String table key for constraint set name (e.g., "DEFAULT", "PWR", "DDR") |
| m_FieldPtr | Pointer to 0x03 FIELD block with CS name as schematic cross-reference (fallback when m_NameStrKey fails) |
| m_Unknown4 | V172+ only; usually 0 |

### 56-Byte Record Layout (V172+)

All values are in internal units (mils * divisor). Interpreted as 14
consecutive `int32_t` fields:

| Field | Offset | Content                              | Example (BB Black) |
|-------|--------|--------------------------------------|--------------------|
| f[0]  | 0      | Reserved (always 0)                  | 0                  |
| f[1]  | 4      | Line width (min trace width)         | 15.0 mil           |
| f[2]  | 8      | Spacing (min spacing)                | 8.0 mil            |
| f[3]  | 12     | Max length (0 = unlimited)           | 200 mil            |
| f[4]  | 16     | **Clearance**                        | 4.0 mil            |
| f[5]  | 20     | Max value or 0                       | 0                  |
| f[6]  | 24     | Flag (usually 1; packed in complex)  | 1                  |
| f[7]  | 28     | **Diff pair gap**                    | 6.5 mil            |
| f[8]  | 32     | Neck width                           | 3.0 mil            |
| f[9]  | 36     | Diff pair neck gap                   | 3.5 mil            |
| f[10] | 40     | Dimension or 0                       | 4.0 mil            |
| f[11] | 44     | Reserved (usually 0)                 | 0                  |
| f[12] | 48     | Reserved (always 0)                  | 0                  |
| f[13] | 52     | Reserved (always 0)                  | 0                  |

BB Black: f[4] = 4.0 mil (all 5 sets). BB-AI: f[4] = 5.0 mil.
EVK boards: f[4] = 5.0 mil.

### 56-Byte Record Layout (Pre-V172)

The field layout is shifted compared to V172+. Line width moved from
f[1] to f[0], and there is no dedicated clearance field:

| Field | Offset | Content                              | Example (TRS80)    |
|-------|--------|--------------------------------------|--------------------|
| f[0]  | 0      | Line width                           | 15.0 mil           |
| f[1]  | 4      | Spacing                              | 7.0 mil            |
| f[2]  | 8      | Usually 0                            | 0                  |
| f[3]  | 12     | Max length or dimension              | 47.2 mil           |
| f[4]  | 16     | Always 0 (no clearance field)        | 0                  |
| f[5]  | 20     | Flag (1 or 0x10001)                  | 1                  |
| f[6]  | 24     | Dimension or 0                       | 15.0 mil           |
| f[7]  | 28     | **Diff pair gap**                    | 4.8 mil            |
| f[8]  | 32     | Neck width                           | 0                  |
| f[9]  | 36     | Diff pair neck gap                   | 7.0 mil            |
| f[10-13]| 40-52| Reserved (usually 0)                 | 0                  |

Pre-V172 boards have no dedicated clearance field. The importer uses
f[1] (spacing) as the clearance fallback since spacing is typically
the closest proxy for pad-to-trace clearance in Allegro's rule model.

### Net-to-Constraint-Set Linkage

Nets reference their Physical Constraint Set by name through NET
field code 0x1a0:

1. Each 0x1D block's `m_NameStrKey` is a string table key resolving to
   the constraint set name (e.g., "DEFAULT", "PWR", "DDR", "BGA")
2. Each NET may have a field with code 0x1a0 whose value is the SAME
   string table key. On most boards this field is an integer (subtype
   0x6a), but on EVK boards it appears as a direct string.
3. Nets WITHOUT field 0x1a0 implicitly use the "DEFAULT" constraint set

Example (BeagleBone Black, 5 constraint sets):

| Constraint Set | Nets Matched |
|----------------|-------------|
| DEFAULT        | ~483 (implicit, no 0x1a0 field) |
| PWR            | 23 |
| BGA            | 0 |
| 90_OHM_DIFF    | 0 |
| 100OHM_DIFF    | 0 |

Additional NET field codes discovered:

| Code  | Purpose |
|-------|---------|
| 0x1a0 | Physical Constraint Set name |
| 0x1a1 | Spacing Constraint Set name |
| 0x3d  | Match group name |
| 0x77  | Net class/schedule name |

### Import Algorithm

The constraint import runs in three passes during BuildBoard():

1. **applyConstraintSets()** walks `m_LL_0x1D_0x1E_0x1F`, creates one
   NETCLASS per 0x1D block with clearance, trace width, and diff pair
   gap. Nets are assigned via field 0x1a0 (default set for nets without
   the field). When diff pair gap is non-zero, also sets diff pair width
   equal to the constraint set's trace width.
2. **applyNetConstraints()** creates per-net trace width netclasses
   (`Allegro_W<N>mil`) from MIN_LINE_WIDTH field 0x55. These override
   the constraint set trace width for nets that have explicit widths.
3. **applyMatchGroups()** creates diff pair and match group netclasses
   from m_MatchGroupPtr pointer chains. Inherits clearance, track width,
   diff pair gap, and diff pair width from the underlying constraint set
   netclass. Uses direct SetNetClass().

### Constraint Set Name Resolution

The constraint set name is resolved from 0x1D blocks in two ways:

1. **m_NameStrKey** (primary): String table key resolving to the name.
   Works for all tested boards except BB-AI which uses 0x0300XXXX keys
   that don't resolve (producing synthetic names like "CS_0").
2. **m_FieldPtr** (fallback): Points to a 0x03 FIELD block containing
   a schematic cross-reference string in the format
   `@lib.xxx(view):\CONSTRAINT_SET_NAME\`. The name is extracted from
   after the last `:\` separator.

Some constraint sets (e.g., BGA, 90_OHM_DIFF, 100OHM_DIFF in
BeagleBone Black) resolve correctly but have zero net assignments.
These are unused design templates, not orphaned references. Only field
0x1a0 assigns nets to constraint sets; there is no component-based,
region-based, or class-based assignment mechanism in the binary format.

## Blocks 0x1E and 0x1F

0x1E and 0x1F blocks share the `m_LL_0x1D_0x1E_0x1F` linked list
with 0x1D constraint set blocks.

**0x1E blocks** contain IBIS and simulation netlists. The m_DataA
records hold ASCII netlist strings (IBIS model references, SKIDL
simulation data). Not currently imported.

**0x1F blocks** contain per-padstack dimension records. Each has a
name (`m_NameStrKey`) and dimension value. Not currently imported.

## Block 0x27 (Constraint Manager Cross-Reference Table)

The 0x27 block extends to an offset defined in the header (`m_0x27_End`).
Size scales with board complexity (10KB for simple boards to 4MB for
VCU118).

### Binary Layout

The blob starts with 3 bytes of zero padding, then a flat array of
uint32 LE values. The first non-zero byte always appears at an offset
congruent to 3 mod 4 (verified across all 11 test boards).

    [3 zero bytes] [uint32 val0] [uint32 val1] ... [uint32 valN]

Non-zero values are block key references (V172+) or runtime heap
pointers (pre-V172). Zero values separate sections, which are groups
of related object references forming a display list entry.

### V172+ Format

Values are compact block keys (small integers matching keys from
blocks of types 0x01, 0x05, 0x0a, 0x0c, 0x15, 0x16, 0x17, 0x28,
0x2d, 0x31, 0x32, etc). Key match rate is 99.6-100% across all V172+
test boards (verified: led_youtube 100%, BB-AI 99.6%, BB-Black 99.7%,
EVK_BaseBoard 99.7%, EVK_SOM 99.9%).

Sections typically contain keys referencing a consistent set of block
types. For example, sections of 0x15 BOUNDARY + 0x17 LINE keys
(constraint region shapes), or mixed sections with 0x28 SHAPE + 0x31
+ 0x32 placed pad keys (component display lists).

### Pre-V172 Format

Values are Allegro runtime heap addresses (large values like
0x0f4487b0). These are block keys from the Allegro address space,
NOT file offsets. Key match rate against the board's block key table
is 92-100% (verified: TRS80 100%, mainBoard 100%, mainBoard2 100%,
ProiectBoard 99.8%, CutiePi 92.7%, VCU118 97.7%).

Section structure is identical to V172+: zero-separated groups of
related pointer values.

### Purpose

This is a constraint manager display and navigation structure. It
maps display list entries to board objects for Allegro's interactive
constraint editor. It does NOT contain clearance, trace width, or
other design rule values directly; those are in 0x1D blocks.

Not imported for PCB conversion since the data is purely
navigational. Parsed into a vector of uint32 references for
validation and potential future use.

## Block 0x35 (File References)

Contains ASCII file path references to Allegro log and report files
(e.g., `terminator.log`, `eclrpt.txt`). Structure: 1 byte T2 +
2 bytes T3 + 120 bytes content with null-terminated strings. Not
imported.

## Per-Net Trace Widths

NET FIELD_KEYS contain trace width constraints (MIN_LINE_WIDTH field
key 0x55, MAX_LINE_WIDTH field key 0x173). These are imported as
KiCad netclasses (e.g., `Allegro_W20mil` with 508000nm track width).
Spacing and clearance values are not present in NET fields.

## NET Pointer Fields

| Field | Usage | Target Type |
|-------|-------|-------------|
| m_MatchGroupPtr | Differential pair / match group | 0x26 (V172+) or 0x2C (pre-V172) |
| m_ModelPtr | IBIS signal integrity model | 0x1E (compiled binary, not imported) |
| m_UnknownPtr4 | Unused (always zero) | - |
| m_UnknownPtr5 | Unused (always zero) | - |
| m_UnknownPtr6 | Unused (always zero) | - |

## Differential Pairs and Match Groups

NET.m_MatchGroupPtr chains to a 0x2C TABLE naming the match group.
The pointer path is version-dependent:

    V172+:      NET -> 0x26 -> m_GroupPtr -> 0x2C TABLE -> string
    Pre-V172:   NET -> 0x2C TABLE -> string

On V172+ boards, some 0x26 blocks have m_GroupPtr pointing to another
0x26 instead of a 0x2C. The importer handles this gracefully by
returning an empty name. VCU118 has 142 match groups total (117 diff
pairs + 25 match groups); BB Black has 21 (17 + 4).

Groups with exactly 2 nets are imported as differential pair netclasses
(`Allegro_DP_<name>`). Groups with more nets are imported as match
group netclasses (`Allegro_MG_<name>`). Assignment uses direct
`SetNetClass()` on the NETINFO_ITEM.

## Keepout Areas

Keepout shapes appear on the `m_LL_0x24_0x28` header linked list
with ROUTE_KEEPOUT (0x0F) or VIA_KEEPOUT (0x13) class. They share
the 0x28 shape structure and are converted to KiCad ZONE objects
with appropriate keepout flags.

## Board-Level Text (0x30)

0x30 STR_WRAPPER blocks contain text objects. Each wraps a 0x31
SGRAPHIC block (via `m_StrGraphicPtr`) that holds the actual string
content, position, and font reference.

Board-level text blocks are NOT on the `m_LL_0x03_0x30` linked list.
That list only contains 0x03 FIELD blocks. Board-level 0x30 blocks
exist in the flat block database (`m_Blocks`) and must be found by
scanning all blocks for type 0x30.

To distinguish board-level text from footprint text, collect the keys
of all 0x30 blocks reachable through footprint instance text chains
(BLK_0x2D_FOOTPRINT_INST `m_TextPtr`), then exclude those keys when
scanning for board-level text.

Board-level text with ETCH class (0x06) is placed on copper layers.
The subclass byte indexes the copper layer in stackup order.

## Empty Net Names

Allegro zones (BOUNDARY shapes) can have unnamed nets. When the
resolveShapeNet() pointer chain yields a 0x1B NET block whose string
table name is empty, the importer assigns a synthetic net name of
the form `Net_<code>` to prevent the net from collapsing into
KiCad's UNCONNECTED net (code 0). Without this, zone fill matching
in applyZoneFills() fails because the zone and its fills end up on
different nets.

## V18 Header Changes (0x00150000)

V18 restructures the file header with significant differences from
V16/V17.

### Linked Lists

V18 has 28 linked lists compared to 22 in pre-V18 files.

| V18 Position | Pre-V18 Position | Field            |
|-------------|-----------------|------------------|
| 0-4         | (new)           | m_LL_V18_1..5    |
| 5-22        | 0-17            | Same 18 shared   |
| 23          | 19              | m_LL_Unknown5    |
| 24          | 18              | m_LL_0x36        |
| 25-26       | 20-21           | Same 2 shared    |
| 27          | (new)           | m_LL_V18_6       |

Note positions 23-24: m_LL_0x36 and m_LL_Unknown5 are swapped
relative to their pre-V18 order.

### Head/Tail Word Order

Each linked list descriptor is two 32-bit words. Pre-V18 reads
word1=tail, word2=head. V18 reverses this to word1=head, word2=tail.

### String Count

Pre-V18 stores the string count as an explicit field in the header
after the layer map. V18 stores it in `m_Unknown2[5]` (read during
early header parsing) and has no separate field. Similarly,
`m_0x27_End` is relocated to `m_Unknown2[2]`.

### 0x35 File References

Pre-V18 stores 0x35 extents (start/end) between the first 18 LLs
and the last 4 LLs. V18 stores them after all 28 linked lists as
two scalar uint32 values.

### Object Stream Gaps

V18 files can have zero-padded gaps between groups of object blocks.
After encountering the end-of-objects marker (0x00 type byte), the
parser skips consecutive zero bytes, then checks if a valid block
type tag (0x01-0x3C) follows. If so, parsing continues from the
4-byte-aligned position before that tag.

### Padstack Trailer

0x1C PADSTACK blocks in V18 have 8 additional uint32 values
(`m_V180Trailer`) between the fixed arrays and the component table.
These are read via `COND_GE<FMT_VER::V_180>` and their purpose is
not yet determined.
