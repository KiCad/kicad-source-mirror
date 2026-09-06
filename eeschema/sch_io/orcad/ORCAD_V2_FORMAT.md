# OrCAD Capture legacy (pre-2003) design format

Read when changing the OrCAD stream parsers or investigating import failures.

`orcad_dsn.ksy` records the modern and legacy layouts. This document records the
container rules and historical corpus measurements. Those measurements predate
the expanded decoder and are not a current coverage claim.

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

## Cache framing

Both families use a zero u16 marker followed by four counted sections: loose
symbols, LibraryParts, PartCells, and Packages. Each section contains a u16 group
count. A group contains an LZT name, a u16 variant count, and that many entries.
Each entry contains an LZT source library, two u32 dates, a type byte, a zero pad,
and a body in the selected framing.

The first definition is the default. Later definitions with the same name become
variants. Modern records have prefix bounds for recovery. A legacy framing error
ends the stream and preserves definitions already read; it does not scan for a
later header. Cache PartCells end after their view lists, while Package PartCells
also contain inline symbol definitions.

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

## Netlist agreement

The Capture Viewer never resolves pin-to-net — `FLAT_NET` is empty in every extraction and
only 83 of 109,551 `PIN` records carry a net — so the oracle's authority is the net name
OrCAD stores on each wire, plus the net on each power symbol and off-page connector.  Those
are compared against a KiCad netlist exported from a project-level import.

Across 358 designs with oracle data:

| | designs | all named nets present | coverage |
|---|---|---|---|
| legacy | 159 | 121 | 78.2% |
| legacy, excluding the 3 hierarchical-block failures | 156 | 121 | 88.7% |
| modern | 199 | 146 | 89.0% |

The reported net coverage is similar for both format families after exclusion of the
three block failures. This comparison does not prove pin-to-net connectivity.

Compare local net names without their sheet path. KiCad can export `CB1` as
`/TILE_TPS65400 (2)/CB1`. Exclude bus aliases such as `clk_n[11:8]` from scalar net
comparisons.

## Initial cache scan

A scanner that recognises symbol definitions by their decoded fields, rather than by any
byte pattern, was run over all 188 legacy designs.  Every file yielded definitions, and
together they hold **10,934 symbol definitions carrying 120,861 graphic primitives** — all
of which the importer discarded before the cache fix.

The initial scanner decoded a median of 56% of each Cache stream. The sequential
parser described above replaced that scanner.

## Hierarchy and stream selection

Type 12 describes a hierarchical block. Its inline LibraryPart supplies the pin
interface; placed pin records supply page positions. Both format families now
have block and occurrence readers. Repeated child folders receive separate
occurrence references.

DSN import reads Library, Cache, local Packages, Views Directory, page-order,
page, hierarchy, and CIS streams. The Views Directory supplies visible folders;
an unlisted folder is imported only when a hierarchy occurrence reaches it.
Standalone OLB import also reads Symbols streams and skips names beginning with `$`.

The Library version selects the framing. A modern design can contain a legacy
Package stream; its header selects that stream's parser. Legacy files always use
the legacy Package grammar.

CIS selection uses the requested variant or the first name in byte order. Its
property overrides are applied to occurrences before schematic conversion.

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
| `/Symbols/<name>` | One symbol (OLB import only) |
| `/Packages/<name>` | Part cells, library parts, package |
| `/Views/<folder>/Schematic` | Page display order |
| `/Views/<folder>/Pages/<page>` | One page |
| `/Views/<root>/Hierarchy/Hierarchy` | Occurrence tree |

A Capture schematic folder can contain several pages. A hierarchical block names
that folder. KiCad uses a wrapper sheet when a block refers to several pages.
