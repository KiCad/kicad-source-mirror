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
                                                         const ORCAD_WARN_FN& aWarn,
                                                         std::map<uint32_t, std::string>*
                                                                 aOccurrenceRefs )
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

            // First lzt = child folder (hier block link), second lzt = occurrence refdes; either
            // may be empty.
            std::string child = stream.ReadLzt();
            std::string occurrenceRef = stream.ReadLzt();

            if( !child.empty() )
                links[instDbId] = child;

            if( aOccurrenceRefs && !occurrenceRef.empty() )
                ( *aOccurrenceRefs )[instDbId] = occurrenceRef;

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


// Framed occ header = long prefix (u8 type, u32 bodyLen, u32 0) + short prefix (u8 type, i16
// propCount, propCount x u32 pairs) + FF E4 5C 39 preamble; throw on mismatch to scan fallback.
static uint8_t readOccHeader( ORCAD_STREAM& aStream, int aExpectType )
{
    uint8_t type = aStream.ReadU8();
    aStream.ReadU32();                          // bodyLen, unused

    if( aStream.ReadU32() != 0 )
        THROW_IO_ERROR( wxS( "occurrence record: long-prefix pad not zero" ) );

    if( aStream.ReadU8() != type )
        THROW_IO_ERROR( wxS( "occurrence record: short-prefix type mismatch" ) );

    int16_t propCount = aStream.ReadI16();

    for( int i = 0; i < std::max<int>( propCount, 0 ); i++ )
    {
        aStream.ReadU32();
        aStream.ReadU32();
    }

    aStream.ExpectPreamble( wxS( "occurrence record preamble" ) );
    aStream.Skip( aStream.ReadU32() );          // trailer

    if( aExpectType >= 0 && type != aExpectType )
        THROW_IO_ERROR( wxString::Format( wxS( "occurrence record type %d != expected %d" ),
                                          (int) type, aExpectType ) );

    return type;
}


static ORCAD_OCC_SCOPE readOccScope( ORCAD_STREAM& aStream );


static void readOccurrence( ORCAD_STREAM& aStream, ORCAD_OCC_SCOPE& aScope )
{
    readOccHeader( aStream, 0x42 );

    aStream.ReadU32();                          // occurrence's own db id
    uint32_t targetDbId = aStream.ReadU32();    // placed-instance (t13) / block (t12) db id

    aStream.ExpectByte( 0x42, wxS( "occurrence inner marker" ) );

    aStream.ReadU32();                          // C (symbol-complexity correlated)
    aStream.ReadU32();                          // D (zero observed)

    std::string child = aStream.ReadLzt();      // child folder name; empty for parts
    std::string ref = aStream.ReadLzt();        // occurrence refdes; empty when none

    aStream.ReadU32();                          // F (zero except rare multi-unit)

    uint16_t pinCount = aStream.ReadU16();

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        readOccHeader( aStream, -1 );           // 0x44 scalar / 0x45 bus pin occurrence
        aStream.ReadU32();                      // pin occurrence db id
        aStream.ReadU16();                      // pin index
    }

    ORCAD_OCC_SCOPE nested = readOccScope( aStream );

    if( !child.empty() )
    {
        ORCAD_OCC_BLOCK block;
        block.targetDbId = targetDbId;
        block.childFolder = child;
        block.scope = std::move( nested );
        aScope.blocks.push_back( std::move( block ) );
    }
    else if( !ref.empty() )
    {
        aScope.partRefs[targetDbId] = ref;
    }
}


// Scope = net occ (0x43), title-block occ (0x52), global/off-page occ (0x5b, u32-counted),
// optional separator preamble, then part/block occ; only part/block kept, rest read to stay aligned.
static ORCAD_OCC_SCOPE readOccScope( ORCAD_STREAM& aStream )
{
    ORCAD_OCC_SCOPE scope;

    uint16_t netCount = aStream.ReadU16();

    for( uint16_t i = 0; i < netCount; i++ )
    {
        readOccHeader( aStream, 0x43 );
        aStream.ReadU32();
        aStream.ReadLzt();
    }

    uint16_t titleBlockCount = aStream.ReadU16();

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        readOccHeader( aStream, 0x52 );
        aStream.ReadU32();
        aStream.ReadU32();
    }

    uint32_t globalCount = aStream.ReadU32();

    for( uint32_t i = 0; i < globalCount; i++ )
    {
        readOccHeader( aStream, 0x5b );
        aStream.ReadU32();
        aStream.ReadU32();
    }

    if( aStream.AtPreamble() )
    {
        aStream.Skip( 4 );
        aStream.Skip( aStream.ReadU32() );
    }

    uint16_t occCount = aStream.ReadU16();

    for( uint16_t i = 0; i < occCount; i++ )
        readOccurrence( aStream, scope );

    return scope;
}


// v2.0 (pre-2003) page parsing. Short-prefix-only framing: every structure is u8 typeId, i16
// propCount, propCount x (u16 nameIdx, u16 valueIdx), no long prefixes, no FF E4 5C 39 preambles.
// String-table indices u16 (modern u32); primitive bodies drop doubled type/byteLength envelope.
// Record vocabulary/field order otherwise modern. Modern path (version >= 3) untouched.

static std::string v2Resolve( const std::vector<std::string>& aStrings, uint16_t aIdx )
{
    return ( aIdx != 0xFFFF && aIdx < aStrings.size() ) ? aStrings[aIdx] : std::string();
}


// Some ancient v2 OLB streams use 10-byte display-prop body (u8 dispMode, no u16, no terminator);
// selected per stream by retry. thread_local so parallel library loads do not race on flag.
static thread_local bool g_v2ShortDisplayProp = false;


// Reject repeat count that cannot fit remaining bytes or exceeds ceiling, so mis-parsed count
// throws (stream skipped) not runaway allocation.
static void v2CheckCount( ORCAD_STREAM& aStream, uint32_t aCount )
{
    if( aCount > 30000 || aCount > aStream.Remaining() )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "v2 repeat count %u exceeds the sane bound "
                                               "(%zu bytes remain)" ),
                                          aCount, aStream.Remaining() ) );
    }
}


// v2 short prefix; fills aProps w/ resolved name/value pairs (empty value when index 0xFFFF).
static uint8_t v2Prefix( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings,
                  std::map<std::string, std::string>* aProps = nullptr )
{
    uint8_t type = aStream.ReadU8();
    int16_t count = aStream.ReadI16();

    if( count > 1000 )
        THROW_IO_ERROR( wxString::Format( wxS( "v2 prefix: absurd property count %d" ), count ) );

    for( int i = 0; i < count; i++ )
    {
        uint16_t nameIdx = aStream.ReadU16();
        uint16_t valueIdx = aStream.ReadU16();

        if( aProps )
            ( *aProps )[v2Resolve( aStrings, nameIdx )] = v2Resolve( aStrings, valueIdx );
    }

    return type;
}


static ORCAD_DISPLAY_PROP v2DisplayProp( ORCAD_STREAM& aStream,
                                         const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_DISPLAY_PROP prop;
    prop.nameIdx = aStream.ReadU16();
    prop.name = v2Resolve( aStrings, static_cast<uint16_t>( prop.nameIdx ) );
    prop.x = aStream.ReadI16();
    prop.y = aStream.ReadI16();

    uint16_t rotFont = aStream.ReadU16();
    prop.fontIdx = rotFont & 0x3FFF;
    prop.rotation = ( rotFont >> 14 ) & 0x3;
    prop.color = aStream.ReadU8();

    if( g_v2ShortDisplayProp )
    {
        prop.dispMode = aStream.ReadU8();
    }
    else
    {
        prop.dispMode = aStream.ReadU16();
        aStream.ExpectByte( 0x00, wxS( "v2 display prop terminator" ) );
    }

    return prop;
}


static std::vector<ORCAD_DISPLAY_PROP> v2DisplayPropList( ORCAD_STREAM& aStream,
                                                   const std::vector<std::string>& aStrings )
{
    std::vector<ORCAD_DISPLAY_PROP> props;
    uint16_t                        count = aStream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
        props.push_back( v2DisplayProp( aStream, aStrings ) );

    return props;
}


static ORCAD_ALIAS v2Alias( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_ALIAS alias;
    alias.x = aStream.ReadI32();
    alias.y = aStream.ReadI32();
    alias.color = aStream.ReadU32();
    alias.rotation = aStream.ReadU32() & 0x3;
    alias.fontIdx = aStream.ReadU32();
    alias.name = aStream.ReadLzt();

    return alias;
}


static void v2Structure( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings );


static ORCAD_WIRE v2Wire( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    uint8_t type = v2Prefix( aStream, aStrings );

    ORCAD_WIRE wire;
    aStream.ReadU32();                          // wire db id
    wire.id = aStream.ReadU32();                // net db id, keys page net table
    aStream.ReadU32();                          // color
    wire.x1 = aStream.ReadI32();
    wire.y1 = aStream.ReadI32();
    wire.x2 = aStream.ReadI32();
    wire.y2 = aStream.ReadI32();
    wire.isBus = type == ORCAD_ST_WIRE_BUS;
    aStream.Skip( 1 );

    uint16_t aliasCount = aStream.ReadU16();

    for( uint16_t i = 0; i < aliasCount; i++ )
        wire.aliases.push_back( v2Alias( aStream, aStrings ) );

    uint16_t propCount = aStream.ReadU16();

    for( uint16_t i = 0; i < propCount; i++ )
        v2Structure( aStream, aStrings );       // framed wire properties, consumed in full

    return wire;
}


static ORCAD_PIN_INST v2PinInst( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_PIN_INST pin;
    aStream.ReadU16();
    pin.x = aStream.ReadI16();
    pin.y = aStream.ReadI16();
    pin.wordA = aStream.ReadU32();
    pin.wordB = aStream.ReadU32();
    pin.displayProps = v2DisplayPropList( aStream, aStrings );

    return pin;
}


static ORCAD_PLACED_INSTANCE v2PlacedInstance( ORCAD_STREAM& aStream,
                                               const std::vector<std::string>& aStrings )
{
    ORCAD_PLACED_INSTANCE inst;
    v2Prefix( aStream, aStrings, &inst.props );

    aStream.Skip( 8 );                          // uninitialized header bytes
    inst.pkgName = aStream.ReadLzt();
    inst.dbId = aStream.ReadU32();

    int by1 = aStream.ReadI16();
    int bx1 = aStream.ReadI16();
    int by2 = aStream.ReadI16();
    int bx2 = aStream.ReadI16();
    inst.bbox.x1 = bx1;
    inst.bbox.y1 = by1;
    inst.bbox.x2 = bx2;
    inst.bbox.y2 = by2;

    inst.x = aStream.ReadI16();
    inst.y = aStream.ReadI16();
    inst.color = aStream.ReadU8();

    uint8_t orientation = aStream.ReadU8();
    inst.rotation = orientation & 0x3;
    inst.mirror = ( orientation & 0x4 ) != 0;
    aStream.Skip( 2 );

    inst.displayProps = v2DisplayPropList( aStream, aStrings );
    aStream.Skip( 1 );
    inst.reference = aStream.ReadLzt();
    inst.value = v2Resolve( aStrings, aStream.ReadU16() );  // v2 u16 value index
    aStream.Skip( 6 );                                      // v2 6 bytes (modern 10)

    uint16_t pinCount = aStream.ReadU16();

    for( uint16_t i = 0; i < pinCount; i++ )
        inst.pins.push_back( v2PinInst( aStream, aStrings ) );

    inst.sourcePackage = aStream.ReadLzt();
    aStream.Skip( 2 );

    return inst;
}


static ORCAD_PRIMITIVE v2PrimBody( ORCAD_STREAM& aStream, uint8_t aType, int aDepth = 0 );


static ORCAD_PRIMITIVE v2Primitive( ORCAD_STREAM& aStream )
{
    return v2PrimBody( aStream, aStream.ReadU8() );
}


static ORCAD_SYMBOL_DEF v2SymbolDef( ORCAD_STREAM& aStream,
                                     const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_SYMBOL_DEF def;
    def.typeId = ORCAD_ST_STH_IN_PAGES0;
    def.name = aStream.ReadLzt();
    def.sourceLib = aStream.ReadLzt();
    aStream.ReadU32();                          // color

    uint16_t primCount = aStream.ReadU16();

    for( uint16_t i = 0; i < primCount; i++ )
        def.primitives.push_back( v2Primitive( aStream ) );

    ORCAD_BBOX bbox;
    bbox.x1 = aStream.ReadI16();
    bbox.y1 = aStream.ReadI16();
    bbox.x2 = aStream.ReadI16();
    bbox.y2 = aStream.ReadI16();
    def.bbox = bbox;

    return def;
}


static ORCAD_PRIMITIVE v2PrimBody( ORCAD_STREAM& aStream, uint8_t aType, int aDepth )
{
    // Symbol vectors nest recursively; bound depth so crafted chain cannot exhaust stack.
    if( aDepth > 32 )
        THROW_IO_ERROR( wxS( "v2 primitive nesting too deep" ) );

    uint8_t         type = aType;
    ORCAD_PRIMITIVE prim;

    switch( type )
    {
    case ORCAD_PRIM_RECT:
    case ORCAD_PRIM_ELLIPSE:
        prim.kind = type == ORCAD_PRIM_RECT ? ORCAD_PRIM_KIND::RECT : ORCAD_PRIM_KIND::ELLIPSE;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        aStream.ReadU32();                      // lineStyle
        prim.lineWidth = aStream.ReadU32();
        prim.fill = aStream.ReadU32() != 0;
        aStream.ReadU32();                      // hatchStyle
        break;

    case ORCAD_PRIM_LINE:
        prim.kind = ORCAD_PRIM_KIND::LINE;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        aStream.ReadU32();                      // lineStyle
        prim.lineWidth = aStream.ReadU32();
        break;

    case ORCAD_PRIM_ARC:
        prim.kind = ORCAD_PRIM_KIND::ARC;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        prim.start = ORCAD_POINT{ aStream.ReadI32(), aStream.ReadI32() };
        prim.end = ORCAD_POINT{ aStream.ReadI32(), aStream.ReadI32() };
        aStream.ReadU32();                      // lineStyle
        prim.lineWidth = aStream.ReadU32();
        break;

    case ORCAD_PRIM_POLYGON:
    case ORCAD_PRIM_POLYLINE:
    case ORCAD_PRIM_BEZIER:
    {
        prim.kind = type == ORCAD_PRIM_POLYGON ? ORCAD_PRIM_KIND::POLYGON
                    : type == ORCAD_PRIM_BEZIER ? ORCAD_PRIM_KIND::BEZIER
                                                : ORCAD_PRIM_KIND::POLYLINE;
        aStream.ReadU32();                      // lineStyle
        prim.lineWidth = aStream.ReadU32();

        if( type == ORCAD_PRIM_POLYGON )
        {
            aStream.ReadU32();                  // fillStyle
            aStream.ReadU32();                  // hatchStyle
        }

        uint16_t count = aStream.ReadU16();
        v2CheckCount( aStream, count );

        for( uint16_t i = 0; i < count; i++ )
        {
            int y = aStream.ReadI16();          // points are stored y-first
            int x = aStream.ReadI16();
            prim.points.push_back( ORCAD_POINT{ x, y } );
        }

        break;
    }

    case ORCAD_PRIM_SYMBOL_VECTOR:
    {
        // Nested graphic doubled type byte, prefix, i16 location, nested prims each padded (u8
        // type, u8 0x00); flatten not meaningful, so read to stay aligned and drop.
        aStream.ExpectByte( ORCAD_PRIM_SYMBOL_VECTOR, wxS( "v2 symbol vector pair" ) );

        int16_t nProp = aStream.ReadI16();

        for( int i = 0; i < std::max<int>( nProp, 0 ); i++ )
        {
            aStream.ReadU16();
            aStream.ReadU16();
        }

        aStream.ReadI16();                      // location x
        aStream.ReadI16();                      // location y

        uint16_t nested = aStream.ReadU16();
        v2CheckCount( aStream, nested );

        for( uint16_t i = 0; i < nested; i++ )
        {
            uint8_t nestedType = aStream.ReadU8();
            aStream.ExpectByte( 0x00, wxS( "v2 vector prim pad" ) );
            v2PrimBody( aStream, nestedType, aDepth + 1 );
        }

        aStream.ReadLzt();                      // vector name
        prim.kind = ORCAD_PRIM_KIND::LINE;      // placeholder; vector body not drawn
        break;
    }

    case ORCAD_PRIM_COMMENT_TEXT:
        prim.kind = ORCAD_PRIM_KIND::TEXT;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        aStream.ReadI32();                      // x2
        aStream.ReadI32();                      // y2
        aStream.ReadI32();                      // x1 (duplicate corner)
        aStream.ReadI32();                      // y1
        prim.fontIdx = aStream.ReadU16();
        aStream.Skip( 2 );
        prim.text = aStream.ReadLzt();
        break;

    default:
        THROW_IO_ERROR( wxString::Format( wxS( "v2 primitive: unhandled type %d" ), (int) type ) );
    }

    return prim;
}


static ORCAD_GRAPHIC_INST v2GraphicInst( ORCAD_STREAM& aStream,
                                         const std::vector<std::string>& aStrings )
{
    ORCAD_GRAPHIC_INST inst;
    inst.typeId = v2Prefix( aStream, aStrings, &inst.props );

    uint16_t nameIdx = aStream.ReadU16();
    aStream.ReadU16();                          // source library string index
    aStream.Skip( 4 );                          // uninitialized junk
    inst.logicalName = v2Resolve( aStrings, nameIdx );
    inst.name = aStream.ReadLzt();
    inst.dbId = aStream.ReadU32();

    inst.y = aStream.ReadI16();
    inst.x = aStream.ReadI16();
    int y2 = aStream.ReadI16();
    int x2 = aStream.ReadI16();
    int x1 = aStream.ReadI16();
    int y1 = aStream.ReadI16();
    inst.bbox.x1 = x1;
    inst.bbox.y1 = y1;
    inst.bbox.x2 = x2;
    inst.bbox.y2 = y2;

    inst.color = aStream.ReadU8();
    uint8_t orientation = aStream.ReadU8();
    inst.rotation = orientation & 0x3;
    inst.mirror = ( orientation & 0x4 ) != 0;
    aStream.Skip( 2 );

    inst.displayProps = v2DisplayPropList( aStream, aStrings );

    uint8_t flag = aStream.ReadU8();

    if( flag == 0x02 )
        inst.nested = std::make_unique<ORCAD_SYMBOL_DEF>( v2SymbolDef( aStream, aStrings ) );

    return inst;
}


static void v2Structure( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    switch( aStream.PeekU8() )
    {
    case ORCAD_ST_STH_IN_PAGES0:       v2SymbolDef( aStream, aStrings ); break;
    case ORCAD_ST_SYMBOL_DISPLAY_PROP: v2DisplayProp( aStream, aStrings ); break;
    case ORCAD_ST_ALIAS:               v2Alias( aStream, aStrings ); break;
    case ORCAD_ST_T0X10:               v2PinInst( aStream, aStrings ); break;
    case ORCAD_ST_PLACED_INSTANCE:     v2PlacedInstance( aStream, aStrings ); break;
    case ORCAD_ST_WIRE_SCALAR:
    case ORCAD_ST_WIRE_BUS:            v2Wire( aStream, aStrings ); break;
    default:
        THROW_IO_ERROR( wxString::Format( wxS( "v2 nested structure: unhandled type %d" ),
                                          aStream.PeekU8() ) );
    }
}


// -- v2.0 .OLB symbol-library streams -----------------------------------------------

static ORCAD_PORT_TYPE v2PortType( uint32_t aRaw )
{
    return aRaw <= 7 ? static_cast<ORCAD_PORT_TYPE>( aRaw ) : ORCAD_PORT_TYPE::PASSIVE;
}


// One symbol pin (type 26 scalar / 27 bus); lone 0x00 = skipped slot
static bool v2SymbolPin( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings,
                         ORCAD_SYMBOL_PIN& aOut )
{
    if( aStream.PeekU8() == 0x00 )
    {
        aStream.Skip( 1 );
        return false;
    }

    uint8_t type = v2Prefix( aStream, aStrings );

    aOut.name = aStream.ReadLzt();
    aOut.startX = aStream.ReadI32();
    aOut.startY = aStream.ReadI32();
    aOut.hotptX = aStream.ReadI32();
    aOut.hotptY = aStream.ReadI32();
    aOut.shapeBits = aStream.ReadU16();
    aStream.Skip( 2 );
    aOut.portType = v2PortType( aStream.ReadU32() );
    aStream.ExpectByte( type, wxS( "v2 pin type echo" ) );
    aStream.Skip( 3 );

    uint16_t dispCount = aStream.ReadU16();
    v2CheckCount( aStream, dispCount );

    for( uint16_t i = 0; i < dispCount; i++ )
        v2DisplayProp( aStream, aStrings );

    return true;
}


// Symbol-def body shared by Symbols streams and inline LibraryParts (read after prefix).
static ORCAD_SYMBOL_DEF v2LibSymbolDef( ORCAD_STREAM& aStream,
                                        const std::vector<std::string>& aStrings, int aTypeId )
{
    ORCAD_SYMBOL_DEF def;
    def.typeId = aTypeId;
    def.name = aStream.ReadLzt();
    def.sourceLib = aStream.ReadLzt();
    aStream.ReadU32();                          // color

    uint16_t primCount = aStream.ReadU16();
    v2CheckCount( aStream, primCount );

    for( uint16_t i = 0; i < primCount; i++ )
        def.primitives.push_back( v2Primitive( aStream ) );

    ORCAD_BBOX bbox;
    bbox.x1 = aStream.ReadI16();
    bbox.y1 = aStream.ReadI16();
    bbox.x2 = aStream.ReadI16();
    bbox.y2 = aStream.ReadI16();
    def.bbox = bbox;

    uint16_t pinCount = aStream.ReadU16();
    v2CheckCount( aStream, pinCount );

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        ORCAD_SYMBOL_PIN pin;

        if( v2SymbolPin( aStream, aStrings, pin ) )
            def.pins.push_back( std::move( pin ) );
    }

    uint16_t dispCount = aStream.ReadU16();
    v2CheckCount( aStream, dispCount );

    for( uint16_t i = 0; i < dispCount; i++ )
        v2DisplayProp( aStream, aStrings );

    return def;
}


// One part cell (type 6); views + one inline LibraryPart per view w/ GeneralProperties tail.
struct V2_PART_CELL
{
    std::string                   name;
    std::vector<ORCAD_SYMBOL_DEF> symbols;   ///< one per view, keyed positionally
    std::vector<std::string>      viewNames;
    std::string                   refDesPrefix;
};


static V2_PART_CELL v2PartCell( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );              // type 6

    V2_PART_CELL cell;
    cell.name = aStream.ReadLzt();
    aStream.ReadLzt();                          // source library

    uint16_t viewCount = aStream.ReadU16();
    v2CheckCount( aStream, viewCount );

    for( uint16_t i = 0; i < viewCount; i++ )
        cell.viewNames.push_back( aStream.ReadLzt() );

    uint16_t symbolCount = aStream.ReadU16();
    v2CheckCount( aStream, symbolCount );

    for( uint16_t i = 0; i < symbolCount; i++ )
    {
        v2Prefix( aStream, aStrings );          // type 24 LibraryPart
        ORCAD_SYMBOL_DEF def = v2LibSymbolDef( aStream, aStrings, ORCAD_ST_LIBRARY_PART );

        aStream.ReadLzt();                      // implementation path
        aStream.ReadLzt();                      // implementation (PSpice model)
        cell.refDesPrefix = aStream.ReadLzt();
        aStream.ReadLzt();                      // part value
        def.generalFlags = aStream.ReadU16();   // pin number/name visibility bits

        cell.symbols.push_back( std::move( def ) );
    }

    return cell;
}


static ORCAD_DEVICE v2Device( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );              // type 32

    ORCAD_DEVICE dev;
    dev.unitRef = aStream.ReadLzt();
    dev.refDes = aStream.ReadLzt();             // part name selecting part cell

    uint16_t pinCount = aStream.ReadU16();
    v2CheckCount( aStream, pinCount );

    static const uint8_t emptySlot[2] = { 0xFF, 0xFF };

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        if( aStream.PeekMatches( emptySlot, 2 ) )
        {
            aStream.Skip( 2 );
            dev.pinNumbers.push_back( std::string() );
            dev.pinIgnore.push_back( true );
            continue;
        }

        dev.pinNumbers.push_back( aStream.ReadLzt() );
        dev.pinIgnore.push_back( ( aStream.ReadU8() & 0x80 ) != 0 );
    }

    return dev;
}


static ORCAD_PACKAGE v2Package( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );              // type 31

    ORCAD_PACKAGE pkg;
    pkg.name = aStream.ReadLzt();
    pkg.sourceLib = aStream.ReadLzt();
    pkg.refDes = aStream.ReadLzt();
    aStream.ReadLzt();                          // unknown
    pkg.pcbFootprint = aStream.ReadLzt();

    uint16_t deviceCount = aStream.ReadU16();
    v2CheckCount( aStream, deviceCount );

    for( uint16_t i = 0; i < deviceCount; i++ )
        pkg.devices.push_back( v2Device( aStream, aStrings ) );

    return pkg;
}


ORCAD_RAW_PAGE OrcadParsePageV2( const std::vector<char>& aData,
                                 const std::vector<std::string>& aStrings,
                                 const ORCAD_WARN_FN& /* aWarn */ )
{
    ORCAD_STREAM   stream( aData );
    ORCAD_RAW_PAGE page;

    uint8_t type = v2Prefix( stream, aStrings );

    if( type != ORCAD_ST_PAGE )
        THROW_IO_ERROR( wxString::Format( wxS( "v2 page: unexpected root type %d" ), (int) type ) );

    page.name = stream.ReadLzt();
    page.pageSize = stream.ReadLzt();

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    page.width = settings.width;
    page.height = settings.height;
    page.isMetric = settings.isMetric;

    uint16_t titleBlockCount = stream.ReadU16();

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        page.titleBlocks.push_back( v2GraphicInst( stream, aStrings ) );
        stream.Skip( 12 );                      // title-block trailer
    }

    uint16_t allNetCount = stream.ReadU16();    // every net on page (name empty)

    for( uint16_t i = 0; i < allNetCount; i++ )
    {
        stream.ReadU32();
        stream.ReadLzt();
    }

    stream.ReadU16();                           // net table B, always zero in corpus

    uint16_t netmapCount = stream.ReadU16();

    for( uint16_t i = 0; i < netmapCount; i++ )
    {
        std::string netName = stream.ReadLzt();
        page.netmap[stream.ReadU32()] = netName;
    }

    uint16_t wireCount = stream.ReadU16();

    for( uint16_t i = 0; i < wireCount; i++ )
        page.wires.push_back( v2Wire( stream, aStrings ) );

    uint16_t instanceCount = stream.ReadU16();

    for( uint16_t i = 0; i < instanceCount; i++ )
        page.instances.push_back( v2PlacedInstance( stream, aStrings ) );

    uint16_t portCount = stream.ReadU16();

    for( uint16_t i = 0; i < portCount; i++ )
    {
        page.ports.push_back( v2GraphicInst( stream, aStrings ) );
        stream.Skip( 9 );
    }

    uint16_t globalCount = stream.ReadU16();

    for( uint16_t i = 0; i < globalCount; i++ )
    {
        page.globals.push_back( v2GraphicInst( stream, aStrings ) );
        stream.Skip( 5 );
    }

    uint16_t offPageCount = stream.ReadU16();

    for( uint16_t i = 0; i < offPageCount; i++ )
    {
        page.offpage.push_back( v2GraphicInst( stream, aStrings ) );
        stream.Skip( 5 );
    }

    uint16_t ercCount = stream.ReadU16();

    for( uint16_t i = 0; i < ercCount; i++ )
    {
        page.ercObjects.push_back( v2GraphicInst( stream, aStrings ) );
        stream.ReadLzt();
        stream.ReadLzt();
        stream.ReadLzt();
    }

    uint16_t busEntryCount = stream.ReadU16();

    for( uint16_t i = 0; i < busEntryCount; i++ )
    {
        v2Prefix( stream, aStrings );
        ORCAD_BUS_ENTRY entry;
        entry.color = stream.ReadU32();
        stream.Skip( 8 );
        entry.x1 = stream.ReadI32();
        entry.y1 = stream.ReadI32();
        entry.x2 = stream.ReadI32();
        entry.y2 = stream.ReadI32();
        page.busEntries.push_back( entry );
    }

    uint16_t graphicCount = stream.ReadU16();

    for( uint16_t i = 0; i < graphicCount; i++ )
        page.graphics.push_back( v2GraphicInst( stream, aStrings ) );

    return page;
}


void OrcadParseOlbSymbolStreamV2( const std::vector<char>& aData,
                                  const std::vector<std::string>& aStrings,
                                  std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols )
{
    // Some ancient streams use 10-byte display-prop body; retry whole stream with short form when
    // normal parse does not consume it exactly.
    for( bool shortDisp : { false, true } )
    {
        g_v2ShortDisplayProp = shortDisp;
        ORCAD_STREAM stream( aData );

        try
        {
            uint8_t type = v2Prefix( stream, aStrings );
            ORCAD_SYMBOL_DEF def = v2LibSymbolDef( stream, aStrings, type );

            if( stream.Remaining() != 0 )
                THROW_IO_ERROR( wxS( "v2 symbol stream: trailing bytes" ) );

            if( !def.name.empty() )
                aSymbols.emplace( def.name, std::move( def ) );

            g_v2ShortDisplayProp = false;
            return;
        }
        catch( const IO_ERROR& )
        {
            if( shortDisp )
            {
                g_v2ShortDisplayProp = false;
                throw;
            }
        }
    }
}


void OrcadParseOlbPackageStreamV2( const std::vector<char>& aData,
                                   const std::vector<std::string>& aStrings,
                                   std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                                   std::map<std::string, ORCAD_PACKAGE>& aPackages )
{
    for( bool shortDisp : { false, true } )
    {
        g_v2ShortDisplayProp = shortDisp;
        ORCAD_STREAM stream( aData );

        try
        {
            uint16_t                  partCount = stream.ReadU16();
            v2CheckCount( stream, partCount );
            std::vector<V2_PART_CELL> cells;

            for( uint16_t i = 0; i < partCount; i++ )
                cells.push_back( v2PartCell( stream, aStrings ) );

            ORCAD_PACKAGE pkg = v2Package( stream, aStrings );

            if( stream.Remaining() != 0 )
                THROW_IO_ERROR( wxS( "v2 package stream: trailing bytes" ) );

            // Inline symbol keyed by view name ("7400.Normal"); view suffix later stripped to
            // part base.
            for( const V2_PART_CELL& cell : cells )
            {
                for( const ORCAD_SYMBOL_DEF& def : cell.symbols )
                {
                    if( !def.name.empty() )
                        aSymbols.emplace( def.name, def );
                }

                if( pkg.refDes.empty() && !cell.refDesPrefix.empty() )
                    pkg.refDes = cell.refDesPrefix;
            }

            if( !pkg.name.empty() )
                aPackages.emplace( pkg.name, std::move( pkg ) );

            g_v2ShortDisplayProp = false;
            return;
        }
        catch( const IO_ERROR& )
        {
            if( shortDisp )
            {
                g_v2ShortDisplayProp = false;
                throw;
            }
        }
    }
}


ORCAD_OCC_SCOPE OrcadReadOccurrenceTree( const std::vector<char>& aData,
                                         const ORCAD_WARN_FN& aWarn )
{
    // Modern streams open w/ type-0x42 long prefix (u8 0x42, u32 bodyLen, u32 0); legacy pre-preamble
    // streams start at view-name lzt with only instance-annotated refs, so empty tree is correct.
    if( aData.size() < 9 || (uint8_t) aData[0] != 0x42 || aData[5] || aData[6] || aData[7]
        || aData[8] )
    {
        return {};
    }

    ORCAD_STREAM stream( aData );

    try
    {
        stream.ReadU8();                        // 0x42
        stream.ReadU32();                       // bodyLen
        stream.ReadU32();                       // 0
        stream.ReadLzt();                       // view name
        stream.Skip( 9 );                       // zeros
        return readOccScope( stream );
    }
    catch( const IO_ERROR& e )
    {
        aWarn( wxString::Format( wxS( "The design occurrence tree could not be fully read (%s); "
                                      "reference designators fall back to the placed instances." ),
                                 e.What() ) );
        return {};
    }
}
