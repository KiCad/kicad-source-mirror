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
 * Parsers for the DSN 'Cache' stream and the 'Packages/\<name\>' streams: symbol
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

/** Consumes one primitive and its optional preamble. Throws IO_ERROR for invalid framing or length.
 * Returns nullopt for a recognized body that is not supported. */
std::optional<ORCAD_PRIMITIVE> OrcadReadPrimitive( ORCAD_STRUCT_READER& aReader );

/** A zero byte marks an empty pin slot and returns nullopt. */
std::optional<ORCAD_SYMBOL_PIN> OrcadReadSymbolPin( ORCAD_STRUCT_READER& aReader );

/**
 * Read a symbol definition body (LibraryPart / GlobalSymbol / PortSymbol /
 * OffPageSymbol / TitleBlockSymbol / ERCSymbol / BookmarkSymbol / PinShapeSymbol).
 * Layout documented on ORCAD_SYMBOL_DEF, including the bbox rule: the box is the
 * LAST 8 bytes before the next prefix stop (as 4 x i16), accepted only when
 * x1 <= x2, y1 <= y2 and both spans are <= 4000 DBU.
 *
 * @param aReader it the parser object.
 * @param aPrefixes
 * @param aWithPins read the trailing u16 pin-count pin list and u16 property list
 *                  (true for cache symbols, false for nested SthInPages0 bodies).
 */
ORCAD_SYMBOL_DEF OrcadReadSymbolDef( ORCAD_STRUCT_READER& aReader,
                                     const ORCAD_PREFIXES& aPrefixes, bool aWithPins );


ORCAD_DRAWN_INSTANCE OrcadReadDrawnInstance( ORCAD_STRUCT_READER& aReader,
                                             const ORCAD_PREFIXES& aPrefixes );

/** Pair placed pin records with the inline definition by pin index. When every placed pin sits on
 * one point, the definition pins are placed through the block orientation instead. */
std::vector<ORCAD_BLOCK_PIN> OrcadResolveBlockPins( int aOrient, int aX1, int aY1, const ORCAD_SYMBOL_DEF& aDefinition,
                                                    const std::vector<ORCAD_PIN_INST>& aPlaced );


ORCAD_DEVICE OrcadReadDevice( ORCAD_STRUCT_READER& aReader );


ORCAD_PACKAGE OrcadReadPackage( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** The first entry is the default; later entries become variants. A modern body error preserves
 * earlier entries and resumes at the declared structure end; a legacy one ends the walk. */
void OrcadParseCache( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                      const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                      std::map<std::string, ORCAD_PACKAGE>& aPackages, ORCAD_DIALECT aDialect = {} );

/** Throws IO_ERROR for invalid framing or trailing bytes. */
void OrcadParseSymbolStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                             std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols, ORCAD_DIALECT aDialect = {} );

/** Package streams contain counted PartCells and LibraryParts, followed by one Package. */
void OrcadParsePackageStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                              std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                              std::map<std::string, ORCAD_PACKAGE>& aPackages, ORCAD_DIALECT aDialect = {} );

/**
 * Merge the results of a 'Packages/\<name\>' stream (locally modified parts) into
 * the main cache maps: a symbol name already present gains the extra entries as
 * variants (the main cache definition stays the default); a new name is inserted
 * as-is.  Packages are inserted only when the name is not already present.
 */
void OrcadMergeCacheStreams( std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                             std::map<std::string, ORCAD_PACKAGE>& aPackages,
                             std::map<std::string, ORCAD_SYMBOL_DEF>&& aExtraSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&& aExtraPackages );

void OrcadMergeSymbolGeneralProperties( std::map<std::string, ORCAD_SYMBOL_DEF>&       aSymbols,
                                        const std::map<std::string, ORCAD_SYMBOL_DEF>& aMetadataSymbols );

#endif // ORCAD_CACHE_H_
