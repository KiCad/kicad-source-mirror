/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file orcad_cache.h
 *
 * Parsers for the DSN 'Cache' stream and the 'Packages/<name>' streams: symbol
 * definitions with graphics and pins, plus package/device pin-number maps.
 *
 * The cache stream interleaves entry metadata (names, source library, dates) with
 * prefix-framed structures.  Rather than fragile sequential parsing, the walker
 * scans for structure starts: every structure of interest begins with a chain of
 * long prefixes (u8 type, u32 len, u32 zero) repeated with the same type byte,
 * then a short prefix and the preamble magic FF E4 5C 39.  Symbol and package
 * structures embed their own name and source library, so the inter-structure
 * metadata can be skipped entirely.
 *
 * Implemented in orcad_cache.cpp.
 */

#ifndef ORCAD_CACHE_H_
#define ORCAD_CACHE_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_stream.h>
#include <sch_io/orcad/orcad_structures.h>

/**
 * Read one graphic primitive including its doubled u8 type-pair prefix, at the
 * current cursor.  Handles both byteLength conventions per record (modern records
 * have the preamble magic right at their claimed end; legacy records exclude the
 * u32 size field + 4-byte pad, so end += 8 when no preamble sits at the claimed
 * end; CommentText is exclusive in ALL eras).  For polygon/polyline/bezier the
 * point count is located deterministically by reconciling candidate offsets
 * (16, 8, 0 for polygons; 8, 0 otherwise) with the stored size under one of the
 * two conventions.  Ends by seeking to the record end and skipping an optional
 * trailing preamble block.
 *
 * @return the primitive, or std::nullopt for SymbolVector records (parsed only to
 *         stay aligned) and for bitmap records whose payload exceeds the record.
 * @throws IO_ERROR on a malformed prefix pair or an overrunning body.
 */
std::optional<ORCAD_PRIMITIVE> OrcadReadPrimitive( ORCAD_STREAM& aStream );

/**
 * Read one symbol pin.  A single 0x00 byte instead of a prefix chain marks a
 * skipped pin slot and yields std::nullopt.  Otherwise the prefixes must be of
 * type 26 or 27; layout documented on ORCAD_SYMBOL_PIN.  The pin body's trailing
 * junk bytes are consumed by seeking to the pin's outer prefix stop.
 */
std::optional<ORCAD_SYMBOL_PIN> OrcadReadSymbolPin( ORCAD_STRUCT_READER& aReader );

/**
 * Read a symbol definition body (LibraryPart / GlobalSymbol / PortSymbol /
 * OffPageSymbol / TitleBlockSymbol / ERCSymbol / BookmarkSymbol / PinShapeSymbol).
 * Layout documented on ORCAD_SYMBOL_DEF, including the bbox rule: the box is the
 * LAST 8 bytes before the next prefix stop (as 4 x i16), accepted only when
 * x1 <= x2, y1 <= y2 and both spans are <= 4000 DBU.
 *
 * @param aWithPins read the trailing u16 pin-count pin list and u16 property list
 *                  (true for cache symbols, false for nested SthInPages0 bodies).
 */
ORCAD_SYMBOL_DEF OrcadReadSymbolDef( ORCAD_STRUCT_READER& aReader,
                                     const ORCAD_PREFIXES& aPrefixes, bool aWithPins );

/**
 * Structure type 2 (SthInPages0): nested symbol body inside Graphic*Inst
 * structures on pages; carries the actual drawing primitives (e.g. the
 * CommentText of a text object).  Equals OrcadReadSymbolDef with aWithPins=false.
 * Called by the ORCAD_STRUCT_READER dispatcher.
 */
ORCAD_SYMBOL_DEF OrcadReadSthInPages0( ORCAD_STRUCT_READER& aReader,
                                       const ORCAD_PREFIXES& aPrefixes );

/**
 * Structure type 12 (DrawnInstance): hierarchical block instance.  Layout
 * documented on ORCAD_DRAWN_INSTANCE; the nested flag byte must equal 24
 * (LibraryPart) and the block reference starts at the second prefix stop.
 * Called by the ORCAD_STRUCT_READER dispatcher; used for hierarchy DETECTION
 * only in this importer.
 */
ORCAD_DRAWN_INSTANCE OrcadReadDrawnInstance( ORCAD_STRUCT_READER& aReader,
                                             const ORCAD_PREFIXES& aPrefixes );

/**
 * Read one Device structure (reads its own prefixes; type must be 32).
 * Layout documented on ORCAD_DEVICE, including the FF FF empty-pin marker and the
 * u8 config byte whose bit 7 flags an ignored pin.
 */
ORCAD_DEVICE OrcadReadDevice( ORCAD_STRUCT_READER& aReader );

/**
 * Read a Package body (type 31); layout documented on ORCAD_PACKAGE.
 */
ORCAD_PACKAGE OrcadReadPackage( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/**
 * Given the absolute position of a preamble magic, backtrack to find a valid
 * prefix chain ending right before it.  Tries short-prefix shapes with 0..39
 * property pairs (also accepting the i16 -1 marker with no pairs), requires the
 * type byte to be in the known-structure whitelist, counts same-type long
 * prefixes (u8 type, u32 len, u32 zero pad) going backwards, validates the chain
 * with TryReadPrefixes(), and rejects chains whose claimed stop offsets run past
 * the stream end.
 *
 * @return the chain start offset, or std::nullopt when no valid chain ends at
 *         this preamble.
 */
std::optional<size_t> OrcadFindStructureStart( const ORCAD_STREAM& aStream,
                                               size_t aPreamblePos );

/**
 * Scan a Cache-framed stream (the 'Cache' stream itself, or any 'Packages/<name>'
 * stream — they share the framing) and collect symbol definitions and packages.
 *
 * Walk: find each preamble, backtrack via OrcadFindStructureStart(), parse symbol
 * types (24, 33, 34, 35, 64, 75, 76, 98) with OrcadReadSymbolDef() and type 31
 * with OrcadReadPackage(); everything else is passed over.  The cache may hold
 * several stale library versions of one symbol name: the FIRST entry wins as the
 * default and later same-name entries are appended to its variants list (see
 * ORCAD_SYMBOL_DEF::variants).  Packages: a later same-name entry replaces the
 * earlier one.  A parse failure
 * inside one structure is warned and recovery continues at the structure's prefix
 * end (or past the preamble when unknown).
 *
 * @param aSymbols  filled in-place, keyed by cache symbol name.
 * @param aPackages filled in-place, keyed by package name.
 */
void OrcadParseCache( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                      const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                      std::map<std::string, ORCAD_PACKAGE>& aPackages );

/**
 * Merge the results of a 'Packages/<name>' stream (locally modified parts) into
 * the main cache maps: a symbol name already present gains the extra entries as
 * variants (the main cache definition stays the default); a new name is inserted
 * as-is.  Packages are inserted only when the name is not already present.
 */
void OrcadMergeCacheStreams( std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                             std::map<std::string, ORCAD_PACKAGE>& aPackages,
                             std::map<std::string, ORCAD_SYMBOL_DEF>&& aExtraSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&& aExtraPackages );

#endif // ORCAD_CACHE_H_
