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
  * `ll_x2b` - a list of footprints (confusingly, Allegro calls these "symbols")


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
