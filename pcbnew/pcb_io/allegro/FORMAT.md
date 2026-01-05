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

Arc center coordinates and radii use IEEE 754 doubles stored as two
big-endian 32-bit words (`ReadAllegroFloat`).

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
| 0x03 | BLK_0x03                | String/name reference                |
| 0x04 | BLK_0x04                | Layer list                           |
| 0x06 | BLK_0x06                | Group/class definition               |
| 0x07 | BLK_0x07                | Footprint instance reference         |
| 0x0A | BLK_0x0A                | Property/attribute                   |
| 0x0C | BLK_0x0C_PIN            | Pin definition                       |
| 0x0E | BLK_0x0E                | Shape/fill segment                   |
| 0x0F | BLK_0x0F                | Subshape reference                   |
| 0x14 | BLK_0x14                | Graphics container (lines, arcs)     |
| 0x15 | BLK_0x15_16_17_SEGMENT  | Line segment (tracks, outlines)      |
| 0x16 | (same as 0x15)          | Line segment variant                 |
| 0x17 | (same as 0x15)          | Line segment variant                 |
| 0x1B | BLK_0x1B_NET            | Net definition                       |
| 0x1C | BLK_0x1C_PADSTACK       | Padstack definition                  |
| 0x1D | BLK_0x1D                | Design rule                          |
| 0x1E | BLK_0x1E                | Design rule variant                  |
| 0x1F | BLK_0x1F                | Design rule variant                  |
| 0x24 | BLK_0x24_RECT           | Rectangle shape                      |
| 0x28 | BLK_0x28_SHAPE          | Polygon shape (zones, fills, pads)   |
| 0x2A | BLK_0x2A                | Layer list entry                     |
| 0x2B | BLK_0x2B                | Component/symbol reference           |
| 0x2C | BLK_0x2C                | DRC marker                           |
| 0x2D | BLK_0x2D                | Footprint instance (placed part)     |
| 0x2E | BLK_0x2E                | Connection/ratsnest                  |
| 0x30 | BLK_0x30_STR_WRAPPER    | Text object (wrapped string)         |
| 0x31 | BLK_0x31_STR            | String content                       |
| 0x32 | BLK_0x32_PLACED_PAD     | Placed pad instance                  |
| 0x33 | BLK_0x33_VIA            | Via instance                         |
| 0x34 | BLK_0x34_KEEPOUT        | Keepout area                         |
| 0x35 | BLK_0x35                | Design constraints                   |
| 0x36 | BLK_0x36                | Font/misc definitions                |
| 0x37 | BLK_0x37                | Hash table / lookup array            |
| 0x38 | BLK_0x38_FILM           | Film definition                      |

## Arc Direction (0x01)

The `m_SubType` field encodes arc sweep direction:
- Bit 6 clear (value 0): counter-clockwise sweep
- Bit 6 set (value 0x40): clockwise sweep

Only values 0 and 64 (0x40) have been observed.

## Padstack Layout (0x1C)

A padstack contains `m_NumFixedCompEntries` fixed-slot components
followed by `m_LayerCount * m_NumCompsPerLayer` per-layer components.

Fixed slots (technical layers):
- Slot 0: Solder mask opening (top)
- Slot 3: Solder mask opening (additional)
- Slot 4: Paste mask opening

Per-layer component order (`m_NumCompsPerLayer` entries per layer):
- Index 0: Pad shape
- Index 1: Thermal relief
- Index 2: Antipad

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

Used for: zone fills, board outline, keepout areas, and custom pad shapes.

## Layer Encoding

Each block has a 2-byte `LAYER_INFO` consisting of a class code and
subclass code. The class identifies the layer category:

| Class  | Purpose          |
|--------|------------------|
| ETCH   | Copper layers    |
| BOARD_GEOMETRY | Board features |
| DRAWING_FORMAT | Drawing annotations |
| ROUTE_KEEPOUT  | Route keepout |
| VIA_KEEPOUT    | Via keepout   |

The subclass identifies the specific layer within the class (e.g.,
TOP, BOTTOM, DFMT_OUTLINE for board edge).

## String Table

Located at fixed offset 0x1200. Each entry is a 4-byte ID followed
by a null-terminated string. String IDs are used throughout the file
to reference net names, pad names, refdes values, and other text.
