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

#include <sch_io/orcad/orcad_page.h>

#include <algorithm>
#include <optional>
#include <utility>

#include <ki_exception.h>

#include <sch_io/orcad/orcad_cache.h>
#include <sch_io/orcad/orcad_library.h>
#include <sch_io/orcad/orcad_stream.h>
#include <sch_io/orcad/orcad_structures.h>


namespace
{

template <typename T>
bool takeRecord( ORCAD_READ_RESULT& aResult, std::vector<T>& aTarget )
{
    if( T* record = std::get_if<T>( &aResult.record ) )
    {
        aTarget.push_back( std::move( *record ) );
        return true;
    }

    return false;
}


template <typename T>
void readStructureList( ORCAD_STRUCT_READER& aReader, std::vector<T>& aTarget )
{
    uint16_t count = aReader.Stream().ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        ORCAD_READ_RESULT result = aReader.ReadStructure();
        takeRecord( result, aTarget );
    }
}

} // namespace


ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData,
                               const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn );
    ORCAD_RAW_PAGE      page;

    ORCAD_PREFIXES prefixes = reader.ReadPrefixes();

    if( prefixes.typeId != ORCAD_ST_PAGE )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD page stream: expected Page structure, "
                                               "got type %d" ),
                                          prefixes.typeId ) );
    }

    page.name = stream.ReadLzt();
    page.pageSize = stream.ReadLzt();

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    page.width = settings.width;
    page.height = settings.height;
    page.isMetric = settings.isMetric;

    readStructureList( reader, page.titleBlocks );

    // T0x34/T0x35 raw (not prefix-framed); read only to stay aligned.
    uint16_t count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
        OrcadReadT0x34Raw( stream );

    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
        OrcadReadT0x35Raw( stream );

    // Net table = lzt net name + u32 net db id; authoritative for names/junctions, wires key it.
    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        std::string netName = stream.ReadLzt();
        uint32_t    netId = stream.ReadU32();

        page.netmap[netId] = netName;
    }

    readStructureList( reader, page.wires );

    // Placed parts; type 13 = part instance, type 12 = hier block instance.
    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        ORCAD_READ_RESULT result = reader.ReadStructure();

        if( !takeRecord( result, page.instances ) )
            takeRecord( result, page.blocks );
    }

    readStructureList( reader, page.ports );

    // Each Global and OffPageConnector list entry followed by 5 bytes.
    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        ORCAD_READ_RESULT result = reader.ReadStructure();
        takeRecord( result, page.globals );
        stream.Skip( 5 );
    }

    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        ORCAD_READ_RESULT result = reader.ReadStructure();
        takeRecord( result, page.offpage );
        stream.Skip( 5 );
    }

    readStructureList( reader, page.ercObjects );
    readStructureList( reader, page.busEntries );
    readStructureList( reader, page.graphics );

    return page;
}


std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream );

    reader.ReadPrefixes();

    stream.ReadLzt();           // schematic folder name
    stream.Skip( 4 );

    uint16_t count = stream.ReadU16();

    std::vector<std::string> names;
    names.reserve( count );

    for( uint16_t i = 0; i < count; i++ )
        names.push_back( stream.ReadLzt() );

    // Page names stored last-first; reverse to display order.
    std::reverse( names.begin(), names.end() );

    return names;
}


std::map<uint32_t, std::string> OrcadReadHierarchyLinks( const std::vector<char>& aData,
                                                         const std::vector<std::string>& aStrings,
                                                         const ORCAD_WARN_FN& aWarn )
{
    std::map<uint32_t, std::string> links;

    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn );

    size_t pos = 0;

    while( true )
    {
        size_t preamblePos = stream.FindPreamble( pos );

        if( preamblePos == ORCAD_STREAM::npos )
            break;

        std::optional<size_t> start = OrcadFindStructureStart( stream, preamblePos );

        if( !start.has_value() || stream.Data()[*start] != ORCAD_ST_HIERARCHY_LINK )
        {
            pos = preamblePos + 4;
            continue;
        }

        stream.Seek( *start );

        try
        {
            ORCAD_PREFIXES prefixes = reader.ReadPrefixes();

            // Body u32, u32 inst db id, u8 inner-frame marker 0x42, u32, u32, lzt child folder name.
            stream.ReadU32();
            uint32_t instDbId = stream.ReadU32();

            stream.ExpectByte( 0x42, wxS( "hierarchy link inner frame" ) );

            stream.ReadU32();
            stream.ReadU32();

            std::string child = stream.ReadLzt();

            if( !child.empty() )
                links[instDbId] = child;

            pos = std::max( stream.GetOffset(), preamblePos + 4 );

            if( prefixes.end != 0 && prefixes.end > pos )
                pos = prefixes.end;
        }
        catch( const IO_ERROR& )
        {
            pos = preamblePos + 4;
        }
    }

    return links;
}
