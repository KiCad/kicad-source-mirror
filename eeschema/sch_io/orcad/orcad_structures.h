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
 * @file orcad_structures.h
 *
 * Prefix framing machinery and readers for the prefix-framed OrCAD structures
 * found in Page/Cache/Schematic/Hierarchy streams.  Every framed structure is
 *
 *     N x long prefix   u8 typeId, u32 bodyLen, u32 (zeros)
 *     1 x short prefix  u8 typeId, i16 nProps, nProps x ( u32 nameIdx, u32 valIdx )
 *     preamble          FF E4 5C 39, u32 trailLen, trailLen bytes
 *     body...
 *
 * The number of long prefixes is structure-type dependent and discovered by trial
 * (longest chain first).  Each long-prefix bodyLen counts the bytes from the end
 * of its own 9-byte record to the end of the structure, which yields reliable skip
 * offsets ("stops") for structures whose body cannot be fully interpreted.
 *
 * Implemented in orcad_structures.cpp (except the readers declared in
 * orcad_cache.h, which orcad_structures.cpp's dispatcher calls for nested
 * SthInPages0 bodies and DrawnInstance blocks).
 */

#ifndef ORCAD_STRUCTURES_H_
#define ORCAD_STRUCTURES_H_

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <wx/string.h>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_stream.h>


/**
 * The decoded prefix chain of one framed structure.
 */
struct ORCAD_PREFIXES
{
    int                                        typeId = 0;   ///< ORCAD_ST value (u8 in the stream)
    std::vector<uint32_t>                      bodyLens;     ///< one per long prefix, outermost first
    std::vector<std::pair<uint32_t, uint32_t>> props;        ///< short prefix (nameIdx, valueIdx) pairs
    size_t                                     start = 0;    ///< stream offset where the chain began
    size_t                                     bodyStart = 0; ///< offset right after the preamble trail
    size_t                                     end = 0;      ///< offset right after the whole structure
                                                             ///< (from the outermost long prefix);
                                                             ///< 0 when unknown
    std::vector<size_t>                        stops;        ///< checkpoint offsets, one per long prefix:
                                                             ///< start + 9 * i + 9 + bodyLens[i]
};


/**
 * Result of reading one structure of any type.  record holds std::monostate when
 * the structure was skipped (no reader for its type) or its body parse failed and
 * was recovered by skipping to the prefix end.
 */
using ORCAD_RECORD_VARIANT =
        std::variant<std::monostate, ORCAD_DISPLAY_PROP, ORCAD_ALIAS, ORCAD_WIRE, ORCAD_PIN_INST,
                     ORCAD_BUS_ENTRY, ORCAD_PLACED_INSTANCE, ORCAD_GRAPHIC_INST,
                     ORCAD_DRAWN_INSTANCE, ORCAD_SYMBOL_DEF>;

struct ORCAD_READ_RESULT
{
    int                  typeId = 0;
    ORCAD_RECORD_VARIANT record;
};


/**
 * Stateful reader shared by all structure parsers.  Holds the stream cursor, the
 * Library string table used to resolve property indices, and the warning sink.
 *
 * Error model: all readers throw IO_ERROR on malformed data.  ReadStructure()
 * additionally implements the standard per-structure recovery: when a body parser
 * throws and the prefix end offset is known, the structure is skipped with a
 * warning and parsing continues (a single bad record never aborts a page).
 */
class ORCAD_STRUCT_READER
{
public:
    ORCAD_STRUCT_READER( ORCAD_STREAM& aStream, const std::vector<std::string>* aStrings = nullptr,
                         ORCAD_WARN_FN aWarn = nullptr );

    ORCAD_STREAM& Stream() { return m_stream; }

    /**
     * Attempt to read exactly aCount prefixes (aCount - 1 long + 1 short) at the
     * current position and verify that the preamble magic follows.  All prefix
     * type bytes must match; every long-prefix pad u32 must be zero; a negative
     * short-prefix pair count is accepted as zero pairs.  On success
     * the cursor rests ON the preamble (not consumed) and the returned
     * ORCAD_PREFIXES has typeId/bodyLens/props/start filled (not bodyStart/end/
     * stops).  Throws IO_ERROR when the shape does not fit; the caller must
     * restore the cursor itself.
     */
    ORCAD_PREFIXES TryReadPrefixes( int aCount );

    /**
     * Discover the prefix count by trial from 10 down to 1 (longest chain first),
     * then consume the prefixes, the preamble and its u32-length trailing block.
     * Fills bodyStart, and — when long prefixes exist — stops[] and end.
     * Throws IO_ERROR (with the cursor restored to the start) when no chain fits.
     */
    ORCAD_PREFIXES ReadPrefixes();

    /// String-table lookup; returns "" for out-of-range indices.
    std::string Resolve( uint32_t aIndex ) const;

    /// Resolve the short-prefix (nameIdx, valueIdx) pairs; empty names are dropped.
    std::map<std::string, std::string> PropsDict( const ORCAD_PREFIXES& aPrefixes ) const;

    /**
     * Seek to the end of a structure using its outermost prefix length.  Throws
     * IO_ERROR when end is unknown (0) or behind the cursor.
     */
    void SkipStructure( const ORCAD_PREFIXES& aPrefixes, const wxString& aWhat );

    /**
     * Read one structure of any type: prefixes, then the type-specific body via
     * the reader dispatch below.  Unknown types are skipped (monostate).  A body
     * parse failure is recovered by seeking to the prefix end with a warning when
     * possible, otherwise the IO_ERROR propagates.
     *
     * Dispatch table (type -> reader):
     *   20, 21          OrcadReadWire
     *   49              OrcadReadAlias
     *   39              OrcadReadDisplayProp
     *   13              OrcadReadPlacedInstance
     *   23              OrcadReadPort
     *   37, 38          OrcadReadGraphicInst
     *   65              OrcadReadTitleBlock
     *   77              OrcadReadErcObject
     *   29              OrcadReadBusEntry
     *   16, 17          OrcadReadPinInst
     *   55..62, 88, 89  OrcadReadGraphicInst
     *   2               OrcadReadSthInPages0    (declared in orcad_cache.h)
     *   12              OrcadReadDrawnInstance  (declared in orcad_cache.h)
     */
    ORCAD_READ_RESULT ReadStructure();

    /// Report a recoverable problem to the warning sink (no-op when none was given).
    void Warn( const wxString& aMsg ) const;

private:
    ORCAD_STREAM&                   m_stream;
    const std::vector<std::string>* m_strings;
    ORCAD_WARN_FN                   m_warn;
};


// -- per-type body readers ----------------------------------------------------------------
//
// Each reader is entered with the stream positioned at the structure body (right
// after ReadPrefixes()) and must leave the cursor at the end of everything it
// consumed.  All throw IO_ERROR on malformed data; recovery is ReadStructure()'s
// job.  Byte layouts are documented on the corresponding structs in
// orcad_records.h.

ORCAD_DISPLAY_PROP OrcadReadDisplayProp( ORCAD_STRUCT_READER& aReader,
                                         const ORCAD_PREFIXES& aPrefixes );

ORCAD_ALIAS OrcadReadAlias( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/// Types 20 (wire) and 21 (bus); isBus is set from the prefix type.
ORCAD_WIRE OrcadReadWire( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_PLACED_INSTANCE OrcadReadPlacedInstance( ORCAD_STRUCT_READER& aReader,
                                               const ORCAD_PREFIXES& aPrefixes );

/// Common body of Port/Global/OffPageConnector/TitleBlock/ERCObject/Graphic*Inst.
ORCAD_GRAPHIC_INST OrcadReadGraphicInst( ORCAD_STRUCT_READER& aReader,
                                         const ORCAD_PREFIXES& aPrefixes );

/// GraphicInst body followed by a 9-byte trailer.
ORCAD_GRAPHIC_INST OrcadReadPort( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/// GraphicInst body followed by a 12-byte trailer.
ORCAD_GRAPHIC_INST OrcadReadTitleBlock( ORCAD_STRUCT_READER& aReader,
                                        const ORCAD_PREFIXES& aPrefixes );

/// GraphicInst body followed by 3 lzt strings.
ORCAD_GRAPHIC_INST OrcadReadErcObject( ORCAD_STRUCT_READER& aReader,
                                       const ORCAD_PREFIXES& aPrefixes );

ORCAD_PIN_INST OrcadReadPinInst( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_BUS_ENTRY OrcadReadBusEntry( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/**
 * Read a u16-counted list of framed DisplayProp structures via ReadStructure(),
 * keeping only the successfully parsed ones.  Also used by the Cache readers.
 */
std::vector<ORCAD_DISPLAY_PROP> OrcadReadDisplayPropList( ORCAD_STRUCT_READER& aReader );

/**
 * T0x34 records are NOT prefix-framed.  Raw layout: 9 bytes, u32 id, lzt string,
 * u32, u32 color, u32 lineStyle, u32 lineWidth.  Content is not used; the reader
 * only keeps the stream aligned.
 */
void OrcadReadT0x34Raw( ORCAD_STREAM& aStream );

/// T0x35 = the T0x34 raw layout followed by u16 n and 4 * n bytes.
void OrcadReadT0x35Raw( ORCAD_STREAM& aStream );

#endif // ORCAD_STRUCTURES_H_
