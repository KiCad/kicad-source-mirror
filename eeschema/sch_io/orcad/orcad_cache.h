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
std::optional<ORCAD_PRIMITIVE> OrcadReadPrimitive( ORCAD_STREAM& aStream );

/** A zero byte marks an empty pin slot and returns nullopt. */
std::optional<ORCAD_SYMBOL_PIN> OrcadReadSymbolPin( ORCAD_STRUCT_READER& aReader );

/** Set aWithPins to read the trailing pin and property lists. */
ORCAD_SYMBOL_DEF OrcadReadSymbolDef( ORCAD_STRUCT_READER& aReader,
                                     const ORCAD_PREFIXES& aPrefixes, bool aWithPins );


ORCAD_SYMBOL_DEF OrcadReadSthInPages0( ORCAD_STRUCT_READER& aReader,
                                       const ORCAD_PREFIXES& aPrefixes );


ORCAD_DRAWN_INSTANCE OrcadReadDrawnInstance( ORCAD_STRUCT_READER& aReader,
                                             const ORCAD_PREFIXES& aPrefixes );


ORCAD_DEVICE OrcadReadDevice( ORCAD_STRUCT_READER& aReader );


ORCAD_PACKAGE OrcadReadPackage( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** The first entry is the default; later entries become variants.
 * A body error preserves earlier entries and resumes at the declared structure end. */
void OrcadParseCache( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                      const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                      std::map<std::string, ORCAD_PACKAGE>& aPackages );

/** Throws IO_ERROR for invalid framing or trailing bytes. */
void OrcadParseSymbolStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                             std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols );

/** Package streams contain counted PartCells and LibraryParts, followed by one Package. */
void OrcadParsePackageStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                              std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                              std::map<std::string, ORCAD_PACKAGE>& aPackages );

/** Existing symbols gain variants. Existing packages retain their cache definition. */
void OrcadMergeCacheStreams( std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                             std::map<std::string, ORCAD_PACKAGE>& aPackages,
                             std::map<std::string, ORCAD_SYMBOL_DEF>&& aExtraSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&& aExtraPackages );

void OrcadMergeSymbolGeneralProperties( std::map<std::string, ORCAD_SYMBOL_DEF>&       aSymbols,
                                        const std::map<std::string, ORCAD_SYMBOL_DEF>& aMetadataSymbols );

#endif // ORCAD_CACHE_H_
