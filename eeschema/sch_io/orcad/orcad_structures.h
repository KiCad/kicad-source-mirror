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

/** Prefix lengths supply structure bounds. See orcad_dsn.ksy for the framing layout. */

#ifndef ORCAD_STRUCTURES_H_
#define ORCAD_STRUCTURES_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <wx/string.h>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_stream.h>


struct ORCAD_PREFIXES
{
    int                                        typeId = 0;    ///< ORCAD_ST value (u8 in the stream)
    std::vector<uint32_t>                      bodyLens;      ///< one per long prefix, outermost first
    std::vector<std::pair<uint32_t, uint32_t>> props;         ///< short prefix (nameIdx, valueIdx) pairs
    size_t                                     start = 0;     ///< stream offset where the chain began
    size_t                                     bodyStart = 0; ///< offset right after the preamble trail
    size_t                                     end = 0;       ///< offset right after the whole structure
                                                              /** < (from the outermost long prefix);
                                                               * < 0 when unknown */
    std::vector<size_t> stops;                                ///< checkpoint offsets, one per long prefix:
                                                              /** < start + 9 * i + 9 + bodyLens[i] */
};


/** A skipped or unreadable body produces monostate. */
using ORCAD_RECORD_VARIANT =
        std::variant<std::monostate, ORCAD_DISPLAY_PROP, ORCAD_ALIAS, ORCAD_WIRE, ORCAD_PIN_INST, ORCAD_BUS_ENTRY,
                     ORCAD_PLACED_INSTANCE, ORCAD_GRAPHIC_INST, ORCAD_DRAWN_INSTANCE, ORCAD_SYMBOL_DEF>;

struct ORCAD_READ_RESULT
{
    int                  typeId = 0;
    ORCAD_RECORD_VARIANT record;
};


/** Registered number of long prefixes for a modern framed structure type. */
std::optional<size_t> OrcadLongPrefixCount( int aTypeId );


/** ReadStructure can recover from a body error when the prefix supplies a valid end offset. */
class ORCAD_STRUCT_READER
{
public:
    ORCAD_STRUCT_READER( ORCAD_STREAM& aStream, const std::vector<std::string>* aStrings = nullptr,
                         ORCAD_WARN_FN aWarn = nullptr );

    ORCAD_STREAM& Stream() { return m_stream; }

    /** aLongPrefixCount overrides the type depth; aEnclosingEnd bounds all stops.
     * Invalid or inconsistent prefixes throw IO_ERROR. */
    ORCAD_PREFIXES ReadPrefixes( int aExpectedType = -1, size_t aEnclosingEnd = ORCAD_STREAM::npos,
                                 size_t aLongPrefixCount = ORCAD_STREAM::npos );

    /** String-table lookup; returns "" for out-of-range indices. */
    std::string Resolve( uint32_t aIndex ) const;

    /** Resolve the short-prefix (nameIdx, valueIdx) pairs; empty names are dropped. */
    std::map<std::string, std::string> PropsDict( const ORCAD_PREFIXES& aPrefixes ) const;

    /** Throws IO_ERROR if the end is unknown or behind the cursor. */
    void SkipStructure( const ORCAD_PREFIXES& aPrefixes, const wxString& aWhat );

    /** Skip unknown bodies. After a body error, resume at the prefix end when possible. */
    ORCAD_READ_RESULT ReadStructure();

    /** Report a recoverable problem to the warning sink (no-op when none was given). */
    void Warn( const wxString& aMsg ) const;

private:
    ORCAD_STREAM&                   m_stream;
    const std::vector<std::string>* m_strings;
    ORCAD_WARN_FN                   m_warn;
};


/** Body readers start after the prefixes. ReadStructure handles recovery from IO_ERROR. */

ORCAD_DISPLAY_PROP OrcadReadDisplayProp( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_ALIAS OrcadReadAlias( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** Types 20 (wire) and 21 (bus); isBus is set from the prefix type. */
ORCAD_WIRE OrcadReadWire( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_PLACED_INSTANCE OrcadReadPlacedInstance( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** Common body of Port/Global/OffPageConnector/TitleBlock/ERCObject/Graphic*Inst. */
ORCAD_GRAPHIC_INST OrcadReadGraphicInst( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** GraphicInst body followed by a 9-byte trailer. */
ORCAD_GRAPHIC_INST OrcadReadPort( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** GraphicInst body followed by a 12-byte trailer. */
ORCAD_GRAPHIC_INST OrcadReadTitleBlock( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** GraphicInst body followed by 3 lzt strings. */
ORCAD_GRAPHIC_INST OrcadReadErcObject( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_PIN_INST OrcadReadPinInst( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

ORCAD_BUS_ENTRY OrcadReadBusEntryBody( ORCAD_STREAM& aStream );

ORCAD_BUS_ENTRY OrcadReadBusEntry( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes );

/** Keep only display properties that ReadStructure decodes. */
std::vector<ORCAD_DISPLAY_PROP> OrcadReadDisplayPropList( ORCAD_STRUCT_READER& aReader );

/** T0x34 records have no prefixes; consume their fixed layout to preserve alignment. */
ORCAD_NET_GROUP OrcadReadT0x34Raw( ORCAD_STREAM& aStream );

/** T0x35 = the T0x34 raw layout followed by u16 n and 4 * n bytes. */
ORCAD_NET_GROUP OrcadReadT0x35Raw( ORCAD_STREAM& aStream );

#endif // ORCAD_STRUCTURES_H_
