# Allegro PCB file format

The Cadeance Allegro file format is a binary format. It is composed of:

* A fixed header structure
* A table of string data
* A sequence of `Blocks` of data

Most data elements in the file is 32-bit, with a few exception. The file is little-endian.

Block properties:

* Each block has a 1-byte tag type (which we'll call `BType`).
* Each block is made up of fields:
    * Mostly these are primitive types (ints, strings, etc)
    * Some blocks contain variable-length sublists
* Some blocks have `Keys`: 32-bit

A board file is a single file - there is no external referenced data.

There appears to be no length tagging of the blocks - the length of each block is implicit on the `BType` and the file version. This means that if you do not know the length of any block, you cannot robustly proceed to decode the file.

## Block linking

There are three main linking mechanisms within the file:

* Direct links: a field contains a key to another block. A specific field in a
  block will refer to a certain type or set of types of other blocks.
* Implicit lists of blocks: some blocks have a field (a "pointer") that points to another block of the same or similar `BType`. This chain continues until the pointer points back to where the chain started. For example, an 0x2D block may point to a list of three 0x14 objects:

```
0x2D (key A)      
   - Field = X -> 0x14
             next = Y              next = Z              next = A
0x14 (key X) -------> 0x14 (key Y) -------> 0x14 (key Z)   [END]
```

* Explicit lists. In the header structure, there are list items that contain a tail and a head. This acts just as the implicit list, but the list ends when the next pointer is the tail value (which is itself _not_ a valid key).

## Data types

* Most data is 32-bit little endian ints
* Coordinates are usually stored as pairs of signed ints: X, then Y.
* A handful of types are a double, stored as two 32-bit chunks.
* Some fields are 8 or 16 bit.
* String data is usually stored in a fixed-length buffer, where the length is stored in some prior field. Many string field are then padded to ensure 4-byte alignment of the next field.

## Header

The header contains the following fields:

* File magic: this is a 32-bit int (e.g. `0x00140400`). The top 3 bytes seem to represent a distinct file type corresponding to an Allegro version. The lower byte does not seem to imply format changes. It is monotonically increasing with version.
* A set of object linked lists - these are the primary entry points into the content of the file:
  * `ll_x1b` - a list of nets in the design 
  * `ll_x2b` - a list of footprints (confusingly, Allegro calls these "symbols")

## Layers

Layers in Allegro are a bit different to KiCad. Allegro items have a `CLASS` and a `SUBCLASS`.

Examples of `CLASS`: `PACKAGE_GEOMETRY`, `REF_DES`, `BOARD_GEOMETRY`.

Examples of `SUBCLASS`: `SILKSCREEN_TOP`, `DIMENSION`, `PLACE_BOUND_TOP`.

Not all `CLASS`es contain all `SUBCLASS`es. A list of some of the classes and subclasses can be found here: https://www.artwork.com/all2dxf/alleggeo.htm

Layers in objects are represent by two bytes, usually bytes 2 and 3 (where byte 0 is the `BType`). The first byte is the `CLASS`, which seems to be a fixed mapping:

* `ETCH`: `0x06`
* `MANUFACTURING`: `0x07`
* ...

The second byte is the subclass. The meaning of this byte depends on the class - the same subclass can have different values in different classes. For example `PACKAGE_GEOMETRY:SILKSCREEN_TOP` is `0xF7`, but the same subclass in the `REF_DES` class is `0xFB`.

High values, like `0xFB` represent fixed, common values like `SILKSCREEN_TOP`. Low values, like `0x00`, represent custom layers. For example, `ETCH` layers start from 0 and represent the `n` copper layers in the design.

The layer map in the head contains a list of custom layers for each `CLASS`. The layer map is arranged as a fixed length list of entries (appears to be 25 entries in all versions). This list is indexed by the `CLASS` value, so that the 7th entry (0-indexed) is for the `0x07` `CLASS` (which is `MANUFACTURING`).

Each entry is two fields:
* A 32-bit value of unknown use
* A pointer to the `0x2A LAYER_LIST` object.

Entries for multiple `CLASS`es can point to the same `0x2A LAYER_LIST` object.

The indexes of the layers within these lists provides the mapping for small subclass values. So, if the first entry in the `MANUFACTURING` list is `NCLEGEND-1-2`, then the class/subclass pair `0x07,0x00` means that layer. 

## Nets

### 0x1B NET

* `next` - links the list that starts in the header
* `net name` - a string ID for the net name
* `path` - an `0x03` object that points to a string that seems to refer to the net in the schematic.
  * E.g. `@preampl_schem.schematic1(sch_1):\0\` (for net `0`).
* `assignment` - a pointer to an 0x04 object. While the `0x04` object has a `next` field, these always seem to be lists of length 1.

### 0x04 NET_ASSIGNMENT

A net assignment is a very small object that adds some other object to a net.

* `next` - generally the `0x1B NET` object as these seem to be single-entry lists
* `net` - points to the `0x1B NET` object
* `conn_item` - points to some object to be assigned the net:
  * `0x05 TRACK`
  * `0x32 PLACED PAD`

## Connected items

### 0x05

This appears to be a collection of connected items:

* `layer`: the class/subclass of the items. Usually (always?) an `0x06 ETCH` class, and the subclass is then the layer number (0 = top side).
* `next`: links a list of other `0x05` block, `0x32 PLACED_PAD` and finally returns to an `0x04 NET_ASSIGNMENT`.
* `net_assignment`: back-links to the relevant `0x04 NET_ASSIGNMENT`
* `first_segment`: links to the start of a list of segments (`0x15`, `0x16`, `0x17`) representing track segments

## Footprints

### 0x2B FOOTPRINT

An 0x2B represents a footprint definition - many actual placed footprints can use one of these.

### 0x2D PLACED_FOOTPRINT

0x2D is a footprint actually on the board.

* `next` iterates to the next `0x2D` - the last instance in a list points back to the `0x2B` object.
* `0x32`: list of pads in the footprint
* `0x14`: list of graphics (lines/arcs) in the footprint
* `0x30`: list of texts in the footprint
* `0x28`: list of polygonal shapes
* `0x07`: instance information (like refdes)
* `position`: The footprint position on the board
* `rotation`: The footprint angle in milli-degrees

## Films

"Films" are related to the output gerbers. Each film is represented by an 0x38 (`FILM`) `Block`.

Probably this is not required for board import - it doesn't seem present in the ASCII exports.

### 0x38 FILM

This block contains:

* A name: literal or a string ID, depending on version
* `next` (for interation)
* Pointer to an `0x39` `Block`.

Examples of film names: `TOP, BOTTOM, BO, FAB, SSTOP, SMBOT, SMTOP, SPTOP`

### 0x39 FILM_LAYER_LIST

List of the layers in one film.

* Pointer to the parent `0x38 FILM`
* Pointer to the first `0x3a FILM_LIST_NODE`
* A list of 22 `uint16` (unknown)

### 0x3a FILM_LIST_NODE

* `next` for iteration
* The layer included in the film by this node
* `unknown_1`
