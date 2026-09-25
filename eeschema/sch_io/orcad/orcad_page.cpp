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
#include <utility>

#include <ki_exception.h>

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


ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn, ORCAD_DIALECT aDialect )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn, aDialect );
    ORCAD_RAW_PAGE      page;

    ORCAD_PREFIXES prefixes = reader.ReadPrefixes( ORCAD_ST_PAGE );

    page.props = reader.PropsDict( prefixes );
    page.name = stream.ReadLzt();
    page.pageSize = stream.ReadLzt();

    page.settings = OrcadParsePageSettings( stream );

    readStructureList( reader, page.titleBlocks );

    if( aDialect.legacy )
    {
        // Legacy pages list every net id with an empty name, then the bus net groups
        uint16_t count = stream.ReadU16();

        for( uint16_t i = 0; i < count; i++ )
        {
            stream.ReadU32();
            stream.ReadLzt();
        }

        count = stream.ReadU16();

        for( uint16_t i = 0; i < count; i++ )
        {
            ORCAD_NET_GROUP group;
            group.id = stream.ReadU32();
            group.name = stream.ReadLzt();

            uint16_t memberCount = reader.ReadCount();

            for( uint16_t j = 0; j < memberCount; j++ )
                group.members.push_back( stream.ReadU32() );

            page.netGroups.push_back( std::move( group ) );
        }
    }
    else
    {
        // T0x34/T0x35 raw (not prefix-framed); read only to stay aligned.
        uint16_t count = stream.ReadU16();

        for( uint16_t i = 0; i < count; i++ )
            page.netGroups.push_back( OrcadReadT0x34Raw( stream ) );

        count = stream.ReadU16();

        for( uint16_t i = 0; i < count; i++ )
            page.netGroups.push_back( OrcadReadT0x35Raw( stream ) );
    }

    // Net table = lzt net name + u32 net db id; authoritative for names/junctions, wires key it.
    uint16_t count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        std::string netName = stream.ReadLzt();
        uint32_t    netId = stream.ReadU32();
        page.netAliases[netId].push_back( netName );
        page.netmap[netId] = std::move( netName );
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

    if( prefixes.end != 0 && stream.GetOffset() > prefixes.end )
        THROW_IO_ERROR( wxS( "OrCAD page stream: page body exceeds its outer stop" ) );

    return page;
}


static std::vector<std::string> pageOrderBody( ORCAD_STREAM& aStream )
{
    aStream.ReadLzt(); // schematic folder name
    aStream.Skip( 4 );

    uint16_t count = aStream.ReadU16();

    std::vector<std::string> names;
    names.reserve( count );

    for( uint16_t i = 0; i < count; i++ )
        names.push_back( aStream.ReadLzt() );

    std::reverse( names.begin(), names.end() );

    return names;
}


std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData, ORCAD_DIALECT aDialect )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, nullptr, nullptr, aDialect );

    // The legacy root type was never checked and is not known
    reader.ReadPrefixes( aDialect.legacy ? -1 : ORCAD_ST_SCH_LIB );

    return pageOrderBody( stream );
}


std::vector<std::string> OrcadParseSchematicFolderOrder( const std::vector<char>& aData )
{
    ORCAD_STREAM stream( aData );
    stream.Skip( 4 );

    uint16_t                 count = stream.ReadU16();
    std::vector<std::string> names;
    names.reserve( count );

    for( uint16_t i = 0; i < count; ++i )
    {
        names.push_back( stream.ReadLzt() );

        if( stream.ReadU16() != 9 )
            THROW_IO_ERROR( wxS( "OrCAD Views Directory: invalid folder record" ) );

        stream.Skip( 20 );
    }

    if( !stream.AtEnd() )
        THROW_IO_ERROR( wxS( "OrCAD Views Directory: trailing data" ) );

    return names;
}


// Occurrence headers always use one long prefix, regardless of the normal type depth.
static uint8_t readOccHeader( ORCAD_STRUCT_READER& aReader, int aExpectType,
                              std::map<std::string, std::string>* aProps = nullptr )
{
    ORCAD_PREFIXES prefixes = aReader.ReadPrefixes( aExpectType, ORCAD_STREAM::npos, 1 );

    if( aProps )
        *aProps = aReader.PropsDict( prefixes );

    return static_cast<uint8_t>( prefixes.typeId );
}


static ORCAD_OCC_SCOPE readOccScope( ORCAD_STRUCT_READER& aReader );


static void readOccurrence( ORCAD_STRUCT_READER& aReader, ORCAD_OCC_SCOPE& aScope )
{
    ORCAD_STREAM&                      stream = aReader.Stream();
    std::map<std::string, std::string> props;
    readOccHeader( aReader, 0x42, &props );

    uint32_t occurrenceId = stream.ReadU32();
    uint32_t blockObjectId = stream.ReadU32();

    if( !aReader.Dialect().legacy )
    {
        stream.ExpectByte( 0x42, wxS( "occurrence inner marker" ) );

        stream.ReadU32(); // C (symbol-complexity correlated)
        stream.ReadU32(); // D (zero observed)
    }

    std::string child = stream.ReadLzt(); // child folder name; empty for parts
    std::string ref = stream.ReadLzt();   // occurrence refdes; empty when none

    std::string unitRef = aReader.Resolve( aReader.ReadStrIdx() );

    uint16_t pinCount = aReader.ReadCount();

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        readOccHeader( aReader, -1 ); // 0x44 scalar / 0x45 bus pin occurrence
        stream.ReadU32();             // pin occurrence db id
        stream.ReadU16();             // pin index
    }

    ORCAD_OCC_SCOPE nested = readOccScope( aReader );

    if( !child.empty() )
    {
        ORCAD_OCC_BLOCK block;
        block.targetDbId = blockObjectId;
        block.childFolder = child;
        block.scope = std::move( nested );
        aScope.blocks.push_back( std::move( block ) );
    }
    else
    {
        if( !ref.empty() )
            aScope.partRefs[blockObjectId] = ref;

        if( !props.empty() )
            aScope.partProps[blockObjectId] = std::move( props );

        if( !unitRef.empty() )
            aScope.partUnitRefs[blockObjectId] = std::move( unitRef );

        aScope.partOccurrenceIds[blockObjectId] = occurrenceId;
    }
}


// Scope = net occ (0x43), title-block occ (0x52), global/off-page occ (0x5b, u32-counted),
// optional separator preamble, then part/block occ; only part/block kept, rest read to stay aligned.
static ORCAD_OCC_SCOPE readOccScope( ORCAD_STRUCT_READER& aReader )
{
    ORCAD_STREAM&            stream = aReader.Stream();
    ORCAD_STREAM::NEST_GUARD guard( stream, wxS( "occurrence scope" ) );

    ORCAD_OCC_SCOPE scope;

    uint16_t netCount = aReader.ReadCount();

    for( uint16_t i = 0; i < netCount; i++ )
    {
        readOccHeader( aReader, 0x43 );
        uint32_t    occurrenceId = stream.ReadU32();
        std::string name = stream.ReadLzt();
        scope.netNames[occurrenceId] = std::move( name );
    }

    uint16_t titleBlockCount = aReader.ReadCount();

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        readOccHeader( aReader, 0x52 );
        stream.ReadU32();
        stream.ReadU32();
    }

    // Legacy scopes have neither global occurrences nor the separator
    if( !aReader.Dialect().legacy )
    {
        uint32_t globalCount = stream.ReadU32();

        for( uint32_t i = 0; i < globalCount; i++ )
        {
            readOccHeader( aReader, 0x5b );
            stream.ReadU32();
            stream.ReadU32();
        }

        if( stream.AtPreamble() )
        {
            stream.Skip( 4 );
            stream.Skip( stream.ReadU32() );
        }
    }

    uint16_t occCount = aReader.ReadCount();

    for( uint16_t i = 0; i < occCount; i++ )
        readOccurrence( aReader, scope );

    return scope;
}


static ORCAD_OCC_SCOPE readOccTree( ORCAD_STRUCT_READER& aReader )
{
    ORCAD_STREAM& stream = aReader.Stream();

    stream.ReadLzt();                                // view name
    stream.Skip( aReader.Dialect().legacy ? 5 : 7 ); // zeros

    uint16_t powerCount = aReader.ReadCount();

    for( uint16_t i = 0; i < powerCount; ++i )
    {
        readOccHeader( aReader, 0x44 );
        stream.ReadU32();
        stream.ReadLzt();
    }

    return readOccScope( aReader );
}


ORCAD_OCC_SCOPE OrcadReadOccurrenceTree( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                         const ORCAD_WARN_FN& aWarn, ORCAD_DIALECT aDialect )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, nullptr, aDialect );

    // A legacy tree has no scan recovery, so a framing error fails the import
    if( aDialect.legacy )
    {
        ORCAD_OCC_SCOPE root = readOccTree( reader );

        if( stream.Remaining() != 0 )
            THROW_IO_ERRORF( wxS( "OrCAD legacy occurrence tree: %zu trailing bytes" ), stream.Remaining() );

        return root;
    }

    // Modern streams open w/ type-0x42 long prefix (u8 0x42, u32 bodyLen, u32 0); legacy pre-preamble
    // streams start at view-name lzt with only instance-annotated refs, so empty tree is correct.
    if( aData.size() < 9 || (uint8_t) aData[0] != 0x42 || aData[5] || aData[6] || aData[7] || aData[8] )
    {
        return {};
    }

    try
    {
        stream.ReadU8();  // 0x42
        stream.ReadU32(); // bodyLen
        stream.ReadU32(); // 0

        return readOccTree( reader );
    }
    catch( const IO_ERROR& e )
    {
        aWarn( wxString::Format( wxS( "The design occurrence tree could not be fully read (%s); "
                                      "reference designators fall back to the placed instances." ),
                                 e.What() ) );
        return {};
    }
}
