# OrCAD Capture legacy (pre-2003) design format

Companion to `orcad_dsn.ksy`, which documents the modern (version 3.x) framing.  This
file records what is known about the version 1.x/2.x framing that `SCH_IO_ORCAD` currently
gates off with `design.library.versionMajor < 3`.

## Version detection

The root `Library` stream begins with a NUL-terminated introduction inside a fixed 32-byte
buffer, then `u16 versionMajor`, `u16 versionMinor`.  The introduction is
`OrCAD Windows Design` for a `.DSN` and `OrCAD Windows Library` for an `.OLB`.

Versions seen in the wild:

| version | era | notes |
|---------|-----|-------|
| 1.1 | OrCAD Capture 7.x | has a root `Hierarchy` storage; no `DsnStream` |
| 2.0 | Capture 9.x / 10.x | the bulk of legacy files |
| 2.1 | Capture 10.5 | still produced by users today |
| 3.2 | Capture 16.x/17.x | what the importer decodes |
| 3.3 | recent 17.4 | seen in the wild, not yet exercised |

`versionMajor < 3` selects a different framing everywhere, not just in one stream.  The
two families differ in three independent ways:

1. **Prefix framing.**  Version 3 structures carry N long prefixes (`u8 type`, `u32 body
   length`, `u32 zero`) followed by a short prefix and a `FF E4 5C 39` preamble.  Version 2
   structures have the short prefix only, and no preamble.
2. **String indices.**  Version 3 property pairs are `u32`/`u32` into the Library string
   table; version 2 pairs are `u16`/`u16`, with `0xFFFF` meaning "empty".
3. **String table length.**  Version 3 counts strings with a `u32`; version 2 uses a `u16`.

Items 1-3 are already handled.  What follows is the part that is not.

## The `Cache` stream

In a version 3 design the root `Cache` stream is preamble-framed and `OrcadParseCache`
reads it.  In a version 2 design it uses an entirely different container, so the importer
skips it and synthesizes a placeholder symbol for every part.  That is the single largest
visible defect in legacy import: the netlist and the sheet come through, but no part has
its real graphics.

The container has been recovered from the corpus:

```
cache_stream:
    u16     unknown0                    observed 0
    u16     unknown1                    observed 2
    entry*                              until the stream is consumed

entry:
    lzt     name                        "C.Normal", "TitleBlock0", "MC33063.Normal"
    u16     flag                        observed 1
    lzt     sourceLib                   absolute path of the .OLB it came from
    u32     dateCreated                 time_t
    u32     dateModified                time_t
    u8      typeId                      structure type of the body
    u8      0x00                        mandatory pad; the type byte is doubled
    body                                type-specific; its short prefix repeats typeId
```

The doubled type byte is the same convention `ORCAD_PRIM_SYMBOL_VECTOR` already uses for
its nested graphic, so it is a general version 2 idiom rather than a quirk of this stream.

### Entry body types

The `typeId` values observed as cache entries are all existing `ORCAD_STRUCTURE_TYPE`
members:

| typeId | name | body | files |
|--------|------|------|-------|
| 24 | `ORCAD_ST_LIBRARY_PART` | symbol definition; a part's drawn body | 187 |
| 31 | `ORCAD_ST_PACKAGE` | pin map, no graphics | 182 |
| 33 | `ORCAD_ST_GLOBAL_SYMBOL` | power symbol; tail carries the net name | 174 |
| 64 | `ORCAD_ST_TITLEBLOCK_SYMBOL` | title block | 159 |
| 26 | `ORCAD_ST_SYMBOL_PIN_SCALAR` | pin shape | 121 |
| 75 | `ORCAD_ST_ERC_SYMBOL` | ERC marker shape | 83 |
| 35 | `ORCAD_ST_OFFPAGE_SYMBOL` | off-page connector shape | 55 |
| 49 | `ORCAD_ST_ALIAS` | net alias | 23 |
| 48 | `ORCAD_ST_SYMBOL_VECTOR` | nested vector graphic | 18 |
| 76 | `ORCAD_ST_BOOKMARK_SYMBOL` | bookmark shape | 12 |
| 34 | `ORCAD_ST_PORT_SYMBOL` | hierarchical port shape | 10 |

The "files" column counts how many of the 188 legacy designs in the corpus contain at
least one entry of that type.  Every value that appears is an existing
`ORCAD_STRUCTURE_TYPE` member, which is the strongest evidence that the structure
numbering did not change between the two format families.

For every symbol-bearing type the body is **byte-identical to what `v2SymbolDef` already
reads** for `.OLB` `Symbols/<name>` streams:

```
symbol_def:
    short prefix                        u8 typeId, i16 propCount, pairs of u16/u16
    lzt     name
    lzt     sourceLib
    u32     colour
    u16     primitiveCount
    primitive*                          u8 type then a body from the table below
    i16     bbox x1, y1, x2, y2
```

Primitive bodies are the ordinary `ORCAD_PRIM_*` set (40 rect, 41 line, 42 arc, 43 ellipse,
44 polygon, 45 polyline, 46 text, 48 symbol vector).  Nothing about them is version
specific.  This is the important result: the version 2 symbol grammar was already correct
and already implemented; only the container that locates each definition was missing.

### Entry tail

After the symbol definition each entry carries a type-specific tail before the next entry
begins.  For `ORCAD_ST_TITLEBLOCK_SYMBOL` the tail is `u16 pad` then a display-property
list (`u16 count`, then `count` records of a type-39 short prefix plus a 12-byte body).
For `ORCAD_ST_GLOBAL_SYMBOL` the tail additionally carries an `lzt` net name ("GND") and
six `u32` fields before its display-property list.

The tail is the one part not yet fully pinned down, and it is the reason a walk cannot
yet run end to end.  It is a bounded problem: the tails belong to the structure types, not
to the container, and several of those types already have readers in the page path.

## Corpus state

Sweep over 856 OrCAD designs in the corpus (188 legacy, 668 modern), counting designs
that emit each diagnostic:

| diagnostic | legacy before | legacy after | modern |
|---|---|---|---|
| placeholder symbols | 130 | 1 | 6 |
| page display order unreadable | 186 | 0 | 5 |
| page skipped | 9 | 8 | 5 |
| embedded picture undecoded | 14 | 0 | 9 |
| occurrence tree unreadable | 0 | 0 | 20 |
| pin position mismatch | n/a | 9 | 0 |
| hierarchical block undecoded | n/a | 6 | 0 |

Legacy designs now carry 137,382 graphic primitives (median 504 per design) where they
previously carried a placeholder box per part.  "Pin position mismatch" only became
measurable once real symbols were read: it is the importer's own check that transformed
definition hot points land on the page's absolute pin positions.

The initial count of 107 picture failures included harness errors. An uninstalled
`kicad-cli` needs `KICAD_STOCK_DATA_HOME` set to the build directory so libwmf can
find its fonts. The table excludes those errors.

## What the corpus proves

A scanner that recognises symbol definitions by their decoded fields, rather than by any
byte pattern, was run over all 188 legacy designs.  Every file yielded definitions, and
together they hold **10,934 symbol definitions carrying 120,861 graphic primitives** — all
of which the importer discarded before the cache fix.

Median coverage is 56% of `Cache` bytes.  The unclaimed remainder is the per-entry headers
and tails, not unrecognised graphics: the primitive grammar never needed a version 2
variant.

## Hierarchical block instances (not yet decoded)

53 of the 188 legacy designs have more than one schematic folder.  Most import (flat),
but a design whose root page carries the hierarchical blocks loses that page entirely,
because the page reader parses every entry of the placed-instance list with the part
grammar and type 12 (`ORCAD_ST_DRAWN_INSTANCE`) has a different body.

Partial decode of one block, from `M523XEVB-SCH-ORCAD/SCH-20380.DSN`, page
`Hierarchical Interconnects`, offset `0x55e1`:

```
u8   typeId = 12
i16  propCount = 0
u32  ?                          0x000000c3
u32  ?                          0x0012f340
u16  ?                          0
u32  ?                          0x002ab2ed
u16  ?                          0x0013
i16  ?  x4                      819, 165, 1103, 820      bounding box, order unconfirmed
i16  ?  x2                      40, 48
u16  ?                          0x000c
u16  displayPropCount = 2       followed by 2 x 15-byte type-39 display properties
     ... 25 undecoded bytes ...
u16  portCount = 21
port*                           type 26/27 records, identical to v2SymbolPin
```

The ports are ordinary `v2SymbolPin` records (`/RSTOUT`, `ETH_CLK`, `EMDIO`, `EMDC`,
`/IRQ[7:1]`, `ECOL`), so only the 25-byte block header between the display properties and
the port count is still unknown.  Until it is, the reader stops at the block and says so
rather than desynchronising into a misleading stream error further down the page.

## Not yet examined

* The `Packages/<name>` and `Symbols/<name>` storages of a version 2 `.DSN`.  The plugin
  currently reads these only for version 3 (`!isV2 ? FindStreamSingleLevel(...)`), yet the
  storages are present in version 2 files, and the `.OLB` readers
  `OrcadParseOlbPackageStreamV2` / `OrcadParseOlbSymbolStreamV2` may apply unchanged.
* The root `Hierarchy` storage of version 1.1 files, which version 3 relocates to
  `Views/<root>/Hierarchy/Hierarchy`.
* Version 3.3, which no corpus test currently covers.

## Corpus

`~/Documents/circuits/OrCAD/_v2-corpus/` is a symlink farm over every legacy file found in
the corpus, with `MANIFEST.md` listing version, size and stream inventory.  It is built by
`AGENT/orcad-v2/build_corpus.py` and holds 206 files (2 at 1.1, 177 at 2.0, 27 at 2.1).

Two files in the corpus named `.DSN` are Proteus ISIS schematics, not OrCAD:
`OrCAD/adi-eval/DC1957A/DC1957A-1.DSN` and `OrCAD/adi-eval/DC1986A/DC1986A-1.DSN`.  They
begin with the ASCII magic `ISIS SCHEMATIC FILE` rather than an OLE2 signature.

## Stream container

The Kaitai schema describes streams extracted from the OLE compound file.
Open an extracted stream in the Kaitai IDE.

| Path | Contents |
|---|---|
| `/Library` | Version, fonts, string table |
| `/Cache` | Cached symbols and packages |
| `/Symbols/<name>` | One symbol |
| `/Packages/<name>` | Part cells, library parts, package |
| `/Views/<folder>/Schematic` | Page display order |
| `/Views/<folder>/Pages/<page>` | One page |
| `/Views/<root>/Hierarchy/Hierarchy` | Occurrence tree |

A Capture schematic folder can contain several pages. A hierarchical block names
that folder. KiCad uses a wrapper sheet when a block refers to several pages.
