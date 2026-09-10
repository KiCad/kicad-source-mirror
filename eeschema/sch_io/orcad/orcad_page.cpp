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
#include <set>

#include <ki_exception.h>
#include <wx/translation.h>

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


ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn );
    ORCAD_RAW_PAGE      page;

    ORCAD_PREFIXES prefixes = reader.ReadPrefixes( ORCAD_ST_PAGE );

    page.props = reader.PropsDict( prefixes );
    page.name = stream.ReadLzt();
    page.pageSize = stream.ReadLzt();

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    page.width = settings.width;
    page.height = settings.height;
    page.isMetric = settings.isMetric;
    page.horizontalCount = settings.horizontalCount;
    page.verticalCount = settings.verticalCount;
    page.horizontalWidth = settings.horizontalWidth;
    page.verticalWidth = settings.verticalWidth;
    page.horizontalChar = settings.horizontalChar;
    page.horizontalAscending = settings.horizontalAscending;
    page.verticalChar = settings.verticalChar;
    page.verticalAscending = settings.verticalAscending;
    page.borderPrinted = settings.borderPrinted;
    page.gridRefPrinted = settings.gridRefPrinted;
    page.createTimestamp = settings.createTimestamp;
    page.modifyTimestamp = settings.modifyTimestamp;

    readStructureList( reader, page.titleBlocks );

    // T0x34/T0x35 raw (not prefix-framed); read only to stay aligned.
    uint16_t count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
        page.netGroups.push_back( OrcadReadT0x34Raw( stream ) );

    count = stream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
        page.netGroups.push_back( OrcadReadT0x35Raw( stream ) );

    // Net table = lzt net name + u32 net db id; authoritative for names/junctions, wires key it.
    count = stream.ReadU16();

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

    if( stream.GetOffset() > prefixes.end )
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


std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream );

    reader.ReadPrefixes( ORCAD_ST_SCH_LIB );

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
static uint8_t readOccHeader( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings, int aExpectType,
                              std::map<std::string, std::string>* aProps = nullptr )
{
    ORCAD_STRUCT_READER reader( aStream, &aStrings );
    ORCAD_PREFIXES      prefixes = reader.ReadPrefixes( aExpectType, ORCAD_STREAM::npos, 1 );

    if( aProps )
        *aProps = reader.PropsDict( prefixes );

    return static_cast<uint8_t>( prefixes.typeId );
}


static ORCAD_OCC_SCOPE readOccScope( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings );


static void readOccurrence( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings, ORCAD_OCC_SCOPE& aScope )
{
    std::map<std::string, std::string> props;
    readOccHeader( aStream, aStrings, 0x42, &props );

    aStream.ReadU32();
    uint32_t blockObjectId = aStream.ReadU32();

    aStream.ExpectByte( 0x42, wxS( "occurrence inner marker" ) );

    aStream.ReadU32(); // C (symbol-complexity correlated)
    aStream.ReadU32(); // D (zero observed)

    std::string child = aStream.ReadLzt(); // child folder name; empty for parts
    std::string ref = aStream.ReadLzt();   // occurrence refdes; empty when none

    uint32_t unitRefIdx = aStream.ReadU32();

    uint16_t pinCount = aStream.ReadU16();

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        readOccHeader( aStream, aStrings, -1 ); // 0x44 scalar / 0x45 bus pin occurrence
        aStream.ReadU32();                      // pin occurrence db id
        aStream.ReadU16();                      // pin index
    }

    ORCAD_OCC_SCOPE nested = readOccScope( aStream, aStrings );

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

        if( unitRefIdx < aStrings.size() && !aStrings[unitRefIdx].empty() )
        {
            aScope.partUnitRefs[blockObjectId] = aStrings[unitRefIdx];
        }
    }
}


// Scope = net occ (0x43), title-block occ (0x52), global/off-page occ (0x5b, u32-counted),
// optional separator preamble, then part/block occ; only part/block kept, rest read to stay aligned.
static ORCAD_OCC_SCOPE readOccScope( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    ORCAD_STREAM::NEST_GUARD guard( aStream, wxS( "occurrence scope" ) );

    ORCAD_OCC_SCOPE scope;

    uint16_t netCount = aStream.ReadU16();

    for( uint16_t i = 0; i < netCount; i++ )
    {
        readOccHeader( aStream, aStrings, 0x43 );
        uint32_t    occurrenceId = aStream.ReadU32();
        std::string name = aStream.ReadLzt();
        scope.netNames[occurrenceId] = std::move( name );
    }

    uint16_t titleBlockCount = aStream.ReadU16();

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        readOccHeader( aStream, aStrings, 0x52 );
        aStream.ReadU32();
        aStream.ReadU32();
    }

    uint32_t globalCount = aStream.ReadU32();

    for( uint32_t i = 0; i < globalCount; i++ )
    {
        readOccHeader( aStream, aStrings, 0x5b );
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
        readOccurrence( aStream, aStrings, scope );

    return scope;
}


// Legacy structures use short prefixes and u16 string indices. See orcad_dsn.ksy for layouts.

static std::string v2Resolve( const std::vector<std::string>& aStrings, uint16_t aIdx )
{
    return ( aIdx != 0xFFFF && aIdx < aStrings.size() ) ? aStrings[aIdx] : std::string();
}


// Version 1 display properties use a 10-byte body. Keep the mode local to each loading thread.
static thread_local bool g_v2ShortDisplayProp = false;


class V2_DISPLAY_PROP_SCOPE
{
public:
    explicit V2_DISPLAY_PROP_SCOPE( bool aShort ) :
            m_previous( g_v2ShortDisplayProp )
    {
        g_v2ShortDisplayProp = aShort;
    }

    ~V2_DISPLAY_PROP_SCOPE() { g_v2ShortDisplayProp = m_previous; }

private:
    bool m_previous;
};


// Reject repeat count that cannot fit remaining bytes or exceeds ceiling, so mis-parsed count
// throws (stream skipped) not runaway allocation.
static void v2CheckCount( ORCAD_STREAM& aStream, uint32_t aCount )
{
    if( aCount > 30000 || aCount > aStream.Remaining() )
    {
        THROW_IO_ERRORF( wxS( "v2 repeat count %u exceeds the sane bound (%zu bytes remain)" ), aCount,
                         aStream.Remaining() );
    }
}


// v2 short prefix; fills aProps w/ resolved name/value pairs (empty value when index 0xFFFF).
static uint8_t v2Prefix( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings,
                         std::map<std::string, std::string>* aProps = nullptr )
{
    uint8_t type = aStream.ReadU8();
    int16_t count = aStream.ReadI16();

    if( count > 1000 )
        THROW_IO_ERRORF( wxS( "v2 prefix: absurd property count %d" ), count );

    for( int i = 0; i < count; i++ )
    {
        uint16_t nameIdx = aStream.ReadU16();
        uint16_t valueIdx = aStream.ReadU16();

        if( aProps )
            ( *aProps )[v2Resolve( aStrings, nameIdx )] = v2Resolve( aStrings, valueIdx );
    }

    return type;
}


static ORCAD_DISPLAY_PROP v2DisplayProp( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
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

    if( g_v2ShortDisplayProp )
    {
        aStream.ReadU8();
        prop.color = aStream.ReadU8();
        prop.dispMode = 0x100;
    }
    else
    {
        prop.color = aStream.ReadU8();
        prop.dispMode = aStream.ReadU16();
        aStream.ExpectByte( 0x00, wxS( "v2 display prop terminator" ) );
    }

    return prop;
}


static std::vector<ORCAD_DISPLAY_PROP> v2DisplayPropList( ORCAD_STREAM&                   aStream,
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
    ORCAD_STREAM::NEST_GUARD guard( aStream, wxS( "v2 wire" ) );

    uint8_t type = v2Prefix( aStream, aStrings );

    ORCAD_WIRE wire;
    wire.dbId = aStream.ReadU32();
    wire.id = aStream.ReadU32(); // net db id, keys page net table
    wire.color = static_cast<int>( aStream.ReadU32() );
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
        v2Structure( aStream, aStrings ); // framed wire properties, consumed in full

    return wire;
}


static ORCAD_PIN_INST v2PinInst( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_PIN_INST pin;
    pin.pinIndex = aStream.ReadI16();
    pin.x = aStream.ReadI16();
    pin.y = aStream.ReadI16();
    pin.wordA = aStream.ReadU32();
    pin.wordB = aStream.ReadU32();
    pin.displayProps = v2DisplayPropList( aStream, aStrings );

    return pin;
}


static ORCAD_PLACED_INSTANCE v2PlacedInstance( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    ORCAD_PLACED_INSTANCE inst;
    v2Prefix( aStream, aStrings, &inst.props );

    aStream.ReadU16();
    inst.sourceLibrary = v2Resolve( aStrings, aStream.ReadU16() );
    aStream.Skip( 4 );
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
    inst.partIndex = aStream.ReadU8();
    inst.partByte = aStream.ReadU8();

    inst.displayProps = v2DisplayPropList( aStream, aStrings );
    aStream.Skip( 1 );
    inst.reference = aStream.ReadLzt();
    inst.value = v2Resolve( aStrings, aStream.ReadU16() ); // v2 u16 value index
    aStream.Skip( 6 );                                     // v2 6 bytes (modern 10)

    uint16_t pinCount = aStream.ReadU16();

    for( uint16_t i = 0; i < pinCount; i++ )
        inst.pins.push_back( v2PinInst( aStream, aStrings ) );

    inst.sourcePackage = aStream.ReadLzt();
    inst.unitIndex = aStream.ReadU16();

    return inst;
}


static ORCAD_PRIMITIVE v2PrimBody( ORCAD_STREAM& aStream, uint8_t aType );


static ORCAD_PRIMITIVE v2Primitive( ORCAD_STREAM& aStream )
{
    return v2PrimBody( aStream, aStream.ReadU8() );
}


static ORCAD_SYMBOL_DEF v2SymbolDef( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );

    ORCAD_SYMBOL_DEF def;
    def.typeId = ORCAD_ST_STH_IN_PAGES0;
    def.name = aStream.ReadLzt();
    def.sourceLib = aStream.ReadLzt();
    def.color = static_cast<int>( aStream.ReadU32() );

    uint16_t primCount = aStream.ReadU16();

    for( uint16_t i = 0; i < primCount; i++ )
        def.primitives.push_back( v2Primitive( aStream ) );

    if( !g_v2ShortDisplayProp )
    {
        ORCAD_BBOX bbox;
        bbox.x1 = aStream.ReadI16();
        bbox.y1 = aStream.ReadI16();
        bbox.x2 = aStream.ReadI16();
        bbox.y2 = aStream.ReadI16();
        def.bbox = bbox;
    }

    return def;
}


static ORCAD_PRIMITIVE v2PrimBody( ORCAD_STREAM& aStream, uint8_t aType )
{
    ORCAD_STREAM::NEST_GUARD guard( aStream, wxS( "v2 primitive" ) );

    uint8_t         type = aType;
    ORCAD_PRIMITIVE prim;

    switch( type )
    {
    case ORCAD_PRIM_RECT:
    case ORCAD_PRIM_ELLIPSE:
        prim.kind = type == ORCAD_PRIM_RECT ? ORCAD_PRIM_KIND::RECTANGLE : ORCAD_PRIM_KIND::ELLIPSE;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        prim.lineStyle = aStream.ReadU32();
        prim.lineWidth = aStream.ReadU32();
        prim.fillStyle = aStream.ReadU32();
        prim.hatchStyle = aStream.ReadU32();
        break;

    case ORCAD_PRIM_LINE:
        prim.kind = ORCAD_PRIM_KIND::LINE;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        prim.lineStyle = aStream.ReadU32();
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
        prim.lineStyle = aStream.ReadU32();
        prim.lineWidth = aStream.ReadU32();
        break;

    case ORCAD_PRIM_POLYGON:
    case ORCAD_PRIM_POLYLINE:
    case ORCAD_PRIM_BEZIER:
    {
        prim.kind = type == ORCAD_PRIM_POLYGON  ? ORCAD_PRIM_KIND::POLYGON
                    : type == ORCAD_PRIM_BEZIER ? ORCAD_PRIM_KIND::BEZIER
                                                : ORCAD_PRIM_KIND::POLYLINE;
        prim.lineStyle = aStream.ReadU32();
        prim.lineWidth = aStream.ReadU32();

        if( type == ORCAD_PRIM_POLYGON )
        {
            prim.fillStyle = aStream.ReadU32();
            prim.hatchStyle = aStream.ReadU32();
        }

        uint16_t count = aStream.ReadU16();
        v2CheckCount( aStream, count );

        for( uint16_t i = 0; i < count; i++ )
        {
            int y = aStream.ReadI16(); // points are stored y-first
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

        prim.kind = ORCAD_PRIM_KIND::GROUP_PRIM;
        prim.x1 = aStream.ReadI16();
        prim.y1 = aStream.ReadI16();

        uint16_t nested = aStream.ReadU16();
        v2CheckCount( aStream, nested );

        for( uint16_t i = 0; i < nested; i++ )
        {
            uint8_t nestedType = aStream.ReadU8();
            aStream.ExpectByte( 0x00, wxS( "v2 vector prim pad" ) );
            prim.children.push_back( v2PrimBody( aStream, nestedType ) );
        }

        aStream.ReadLzt(); // vector name
        break;
    }

    case ORCAD_PRIM_COMMENT_TEXT:
        prim.kind = ORCAD_PRIM_KIND::TEXT;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        prim.textBoundsStart = ORCAD_POINT{ aStream.ReadI32(), aStream.ReadI32() };
        prim.fontIdx = aStream.ReadU16();
        aStream.Skip( 2 );
        prim.text = aStream.ReadLzt();
        break;

    case ORCAD_PRIM_BITMAP:
    {
        prim.kind = ORCAD_PRIM_KIND::IMAGE;
        prim.x1 = aStream.ReadI32();
        prim.y1 = aStream.ReadI32();
        prim.x2 = aStream.ReadI32();
        prim.y2 = aStream.ReadI32();
        aStream.Skip( 8 ); // duplicate corner
        aStream.Skip( 8 ); // pixel width/height

        uint32_t dataSize = aStream.ReadU32();

        if( dataSize > aStream.Remaining() )
        {
            THROW_IO_ERRORF( wxS( "v2 bitmap: payload %u exceeds the %zu bytes left" ), dataSize, aStream.Remaining() );
        }

        prim.data = aStream.ReadBytes( dataSize );
        break;
    }

    default: THROW_IO_ERRORF( wxS( "v2 primitive: unhandled type %d at 0x%zx" ), (int) type, aStream.GetOffset() - 1 );
    }

    return prim;
}


static ORCAD_GRAPHIC_INST v2GraphicInst( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    ORCAD_GRAPHIC_INST inst;
    inst.typeId = v2Prefix( aStream, aStrings, &inst.props );

    uint16_t nameIdx = aStream.ReadU16();
    uint16_t sourceLibraryIdx = aStream.ReadU16();
    aStream.Skip( 4 ); // uninitialized junk
    inst.logicalName = v2Resolve( aStrings, nameIdx );
    inst.name = aStream.ReadLzt();

    if( std::string sourceLibrary = v2Resolve( aStrings, sourceLibraryIdx ); !sourceLibrary.empty() )
        inst.props["Source Library"] = std::move( sourceLibrary );

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
    case ORCAD_ST_STH_IN_PAGES0: v2SymbolDef( aStream, aStrings ); break;
    case ORCAD_ST_SYMBOL_DISPLAY_PROP: v2DisplayProp( aStream, aStrings ); break;
    case ORCAD_ST_ALIAS: v2Alias( aStream, aStrings ); break;
    case ORCAD_ST_T0X10:
    case ORCAD_ST_T0X11: v2PinInst( aStream, aStrings ); break;
    case ORCAD_ST_PLACED_INSTANCE: v2PlacedInstance( aStream, aStrings ); break;
    case ORCAD_ST_WIRE_SCALAR:
    case ORCAD_ST_WIRE_BUS: v2Wire( aStream, aStrings ); break;
    default: THROW_IO_ERRORF( wxS( "v2 nested structure: unhandled type %d" ), aStream.PeekU8() );
    }
}


// -- v2.0 .OLB symbol-library streams -----------------------------------------------

static ORCAD_PORT_TYPE v2PortType( uint32_t aRaw )
{
    return aRaw <= 7 ? static_cast<ORCAD_PORT_TYPE>( aRaw ) : ORCAD_PORT_TYPE::PASSIVE;
}


// One symbol pin (type 26 scalar / 27 bus); lone 0x00 = skipped slot
static bool v2SymbolPin( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings, ORCAD_SYMBOL_PIN& aOut )
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
        aOut.displayProps.push_back( v2DisplayProp( aStream, aStrings ) );

    return true;
}


// Symbol-def body shared by Symbols streams and inline LibraryParts (read after prefix).
static ORCAD_SYMBOL_DEF v2LibSymbolDef( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings, int aTypeId )
{
    ORCAD_SYMBOL_DEF def;
    def.typeId = aTypeId;
    def.name = aStream.ReadLzt();
    def.sourceLib = aStream.ReadLzt();
    def.color = static_cast<int>( aStream.ReadU32() );

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


static void v2LibraryPartTail( ORCAD_STREAM& aStream, ORCAD_SYMBOL_DEF& aDef )
{
    std::string implementationPath = aStream.ReadLzt();

    if( !implementationPath.empty() )
        aDef.props["Implementation Path"] = std::move( implementationPath );

    aStream.ReadLzt(); // implementation (PSpice model)
    aStream.ReadLzt(); // reference prefix
    aStream.ReadLzt(); // part value
    aDef.generalFlags = aStream.ReadU16();
}


// One part cell (type 6); views + one inline LibraryPart per view w/ GeneralProperties tail.
struct V2_PART_CELL
{
    std::string                   name;
    std::vector<ORCAD_SYMBOL_DEF> symbols; ///< one per view, keyed positionally
    std::vector<std::string>      viewNames;
    std::string                   refDesPrefix;
    std::map<std::string, std::string> props;
};


static V2_PART_CELL v2PartCell( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    std::map<std::string, std::string> cellProps;
    v2Prefix( aStream, aStrings, &cellProps ); // type 6

    V2_PART_CELL cell;
    cell.props = cellProps;
    cell.name = aStream.ReadLzt();
    aStream.ReadLzt(); // source library

    uint16_t viewCount = aStream.ReadU16();
    v2CheckCount( aStream, viewCount );

    for( uint16_t i = 0; i < viewCount; i++ )
        cell.viewNames.push_back( aStream.ReadLzt() );

    uint16_t symbolCount = aStream.ReadU16();
    v2CheckCount( aStream, symbolCount );

    for( uint16_t i = 0; i < symbolCount; i++ )
    {
        std::map<std::string, std::string> symbolProps;
        v2Prefix( aStream, aStrings, &symbolProps ); // type 24 LibraryPart
        ORCAD_SYMBOL_DEF def = v2LibSymbolDef( aStream, aStrings, ORCAD_ST_LIBRARY_PART );
        def.props = cell.props;
        for( const auto& [name, value] : symbolProps )
            def.props[name] = value;

        std::string implementationPath = aStream.ReadLzt();

        if( !implementationPath.empty() )
            def.props["Implementation Path"] = std::move( implementationPath );

        aStream.ReadLzt(); // implementation (PSpice model)
        cell.refDesPrefix = aStream.ReadLzt();
        aStream.ReadLzt();                    // part value
        def.generalFlags = aStream.ReadU16(); // pin number/name visibility bits

        cell.symbols.push_back( std::move( def ) );
    }

    return cell;
}


static ORCAD_DEVICE v2Device( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings ); // type 32

    ORCAD_DEVICE dev;
    dev.unitRef = aStream.ReadLzt();
    dev.refDes = aStream.ReadLzt(); // part name selecting part cell

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
    ORCAD_PACKAGE pkg;
    v2Prefix( aStream, aStrings, &pkg.props ); // type 31
    pkg.name = aStream.ReadLzt();
    pkg.sourceLib = aStream.ReadLzt();
    pkg.refDes = aStream.ReadLzt();
    aStream.ReadLzt(); // unknown
    pkg.pcbFootprint = aStream.ReadLzt();

    uint16_t deviceCount = aStream.ReadU16();
    v2CheckCount( aStream, deviceCount );

    for( uint16_t i = 0; i < deviceCount; i++ )
        pkg.devices.push_back( v2Device( aStream, aStrings ) );

    return pkg;
}


static ORCAD_DRAWN_INSTANCE v2DrawnInstance( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    ORCAD_DRAWN_INSTANCE block;
    v2Prefix( aStream, aStrings, &block.props );

    uint16_t nameIdx = aStream.ReadU16();
    aStream.ReadU16(); // source library string index
    aStream.Skip( 4 ); // uninitialized header bytes
    aStream.ReadLzt(); // name

    block.name = v2Resolve( aStrings, nameIdx );
    block.dbId = aStream.ReadU32();

    aStream.ReadI16(); // anchor y
    aStream.ReadI16(); // anchor x
    aStream.ReadI16(); // bbox y2
    aStream.ReadI16(); // bbox x2
    block.x1 = aStream.ReadI16();
    block.y1 = aStream.ReadI16();
    aStream.ReadU8(); // color
    uint8_t orientation = aStream.ReadU8();
    aStream.Skip( 2 ); // structure id

    block.displayProps = v2DisplayPropList( aStream, aStrings );
    aStream.ExpectByte( ORCAD_ST_LIBRARY_PART, wxS( "v2 drawn instance nested flag" ) );

    std::map<std::string, std::string> nestedProps;
    uint8_t nestedType = v2Prefix( aStream, aStrings, &nestedProps );

    if( nestedType != ORCAD_ST_LIBRARY_PART )
    {
        THROW_IO_ERRORF( wxS( "v2 drawn instance: unexpected nested type %d" ), static_cast<int>( nestedType ) );
    }

    ORCAD_SYMBOL_DEF nested = v2LibSymbolDef( aStream, aStrings, nestedType );
    block.props.insert( nestedProps.begin(), nestedProps.end() );
    ORCAD_BBOX       bbox = nested.bbox.value_or( ORCAD_BBOX() );
    bool             quarterTurn = ( orientation & 1 ) != 0;
    block.w = quarterTurn ? bbox.y2 - bbox.y1 : bbox.x2 - bbox.x1;
    block.h = quarterTurn ? bbox.x2 - bbox.x1 : bbox.y2 - bbox.y1;

    aStream.Skip( 14 );
    block.reference = aStream.ReadLzt();
    uint16_t valueIdx = aStream.ReadU16();
    aStream.ReadU16();
    uint16_t implementationIdx = aStream.ReadU16();
    aStream.ReadU16();
    block.props["Value"] = v2Resolve( aStrings, valueIdx );
    block.props["Implementation"] = v2Resolve( aStrings, implementationIdx );

    uint16_t pinCount = aStream.ReadU16();
    v2CheckCount( aStream, pinCount );

    std::vector<ORCAD_PIN_INST> pinInsts;

    for( uint16_t i = 0; i < pinCount; i++ )
        pinInsts.push_back( v2PinInst( aStream, aStrings ) );

    std::set<std::pair<int, int>> placedPoints;
    std::set<std::pair<int, int>> definitionPoints;

    for( const ORCAD_PIN_INST& pin : pinInsts )
        placedPoints.emplace( pin.x, pin.y );

    for( const ORCAD_SYMBOL_PIN& pin : nested.pins )
        definitionPoints.emplace( pin.hotptX, pin.hotptY );

    bool useDefinitionGeometry = placedPoints.size() <= 1 && definitionPoints.size() > 1;

    for( size_t i = 0; i < pinInsts.size() && i < nested.pins.size(); i++ )
    {
        ORCAD_BLOCK_PIN pin;
        pin.name = nested.pins[i].name;
        pin.portType = nested.pins[i].portType;
        pin.x = useDefinitionGeometry ? block.x1 + nested.pins[i].hotptX - bbox.x1 : pinInsts[i].x;
        pin.y = useDefinitionGeometry ? block.y1 + nested.pins[i].hotptY - bbox.y1 : pinInsts[i].y;
        pin.noConnect = pinInsts[i].IsNoConnect();
        block.pins.push_back( std::move( pin ) );
    }

    return block;
}


ORCAD_RAW_PAGE OrcadParsePageV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                 const ORCAD_WARN_FN& /* aWarn */, bool                          aShortDisplayProp )
{
    V2_DISPLAY_PROP_SCOPE displayPropScope( aShortDisplayProp );
    ORCAD_STREAM          stream( aData );
    ORCAD_RAW_PAGE        page;

    uint8_t type = v2Prefix( stream, aStrings, &page.props );

    if( type != ORCAD_ST_PAGE )
        THROW_IO_ERRORF( wxS( "v2 page: unexpected root type %d" ), (int) type );

    page.name = stream.ReadLzt();
    page.pageSize = stream.ReadLzt();

    ORCAD_PAGE_SETTINGS settings = OrcadParsePageSettings( stream );
    page.width = settings.width;
    page.height = settings.height;
    page.isMetric = settings.isMetric;
    page.horizontalCount = settings.horizontalCount;
    page.verticalCount = settings.verticalCount;
    page.horizontalWidth = settings.horizontalWidth;
    page.verticalWidth = settings.verticalWidth;
    page.horizontalChar = settings.horizontalChar;
    page.horizontalAscending = settings.horizontalAscending;
    page.verticalChar = settings.verticalChar;
    page.verticalAscending = settings.verticalAscending;
    page.borderPrinted = settings.borderPrinted;
    page.gridRefPrinted = settings.gridRefPrinted;
    page.createTimestamp = settings.createTimestamp;
    page.modifyTimestamp = settings.modifyTimestamp;

    uint16_t titleBlockCount = stream.ReadU16();

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        page.titleBlocks.push_back( v2GraphicInst( stream, aStrings ) );
        stream.Skip( 12 ); // title-block trailer
    }

    uint16_t allNetCount = stream.ReadU16(); // every net on page (name empty)

    for( uint16_t i = 0; i < allNetCount; i++ )
    {
        stream.ReadU32();
        stream.ReadLzt();
    }

    uint16_t netGroupCount = stream.ReadU16();

    for( uint16_t i = 0; i < netGroupCount; i++ )
    {
        ORCAD_NET_GROUP group;
        group.id = stream.ReadU32();
        group.name = stream.ReadLzt();

        uint16_t memberCount = stream.ReadU16();
        v2CheckCount( stream, memberCount );

        for( uint16_t j = 0; j < memberCount; j++ )
            group.members.push_back( stream.ReadU32() );

        page.netGroups.push_back( std::move( group ) );
    }

    uint16_t netmapCount = stream.ReadU16();

    for( uint16_t i = 0; i < netmapCount; i++ )
    {
        std::string netName = stream.ReadLzt();
        uint32_t    netId = stream.ReadU32();
        page.netAliases[netId].push_back( netName );
        page.netmap[netId] = std::move( netName );
    }

    uint16_t wireCount = stream.ReadU16();

    for( uint16_t i = 0; i < wireCount; i++ )
        page.wires.push_back( v2Wire( stream, aStrings ) );

    uint16_t instanceCount = stream.ReadU16();

    for( uint16_t i = 0; i < instanceCount; i++ )
    {
        if( stream.PeekU8() == ORCAD_ST_DRAWN_INSTANCE )
            page.blocks.push_back( v2DrawnInstance( stream, aStrings ) );
        else
            page.instances.push_back( v2PlacedInstance( stream, aStrings ) );
    }

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
        page.busEntries.push_back( OrcadReadBusEntryBody( stream ) );
    }

    uint16_t graphicCount = stream.ReadU16();

    for( uint16_t i = 0; i < graphicCount; i++ )
        page.graphics.push_back( v2GraphicInst( stream, aStrings ) );

    return page;
}


void OrcadParseOlbSymbolStreamV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                  std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols, bool aShortDisplayProp )
{
    V2_DISPLAY_PROP_SCOPE displayPropScope( aShortDisplayProp );
    ORCAD_STREAM          stream( aData );

    std::map<std::string, std::string> props;
    uint8_t                            type = v2Prefix( stream, aStrings, &props );
    ORCAD_SYMBOL_DEF                   def = v2LibSymbolDef( stream, aStrings, type );
    def.props = std::move( props );

    if( type == ORCAD_ST_LIBRARY_PART && stream.Remaining() != 0 )
        v2LibraryPartTail( stream, def );

    if( stream.Remaining() != 0 )
        THROW_IO_ERROR( wxS( "v2 symbol stream: trailing bytes" ) );

    if( def.name.empty() )
        return;

    auto existing = aSymbols.find( def.name );

    if( existing == aSymbols.end() )
        aSymbols.emplace( def.name, std::move( def ) );
    else
        existing->second.variants.push_back( std::move( def ) );
}


std::vector<std::string> OrcadParsePageOrderV2( const std::vector<char>&        aData,
                                                const std::vector<std::string>& aStrings )
{
    ORCAD_STREAM stream( aData );

    v2Prefix( stream, aStrings );

    return pageOrderBody( stream );
}


// Cache PartCells end after the view list. Package PartCells also contain inline symbols.
static void v2CachePartCell( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    v2Prefix( aStream, aStrings );
    aStream.ReadLzt(); // part cell name
    aStream.ReadLzt(); // source library

    uint16_t viewCount = aStream.ReadU16();

    v2CheckCount( aStream, viewCount );

    for( uint16_t i = 0; i < viewCount; ++i )
        aStream.ReadLzt();
}


void OrcadParseCacheV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                        const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                        std::map<std::string, ORCAD_PACKAGE>& aPackages )
{
    // The group count includes all variants. Each of the four sections uses legacy framing.
    ORCAD_STREAM stream( aData );

    try
    {
        if( stream.ReadU16() != 0 )
            THROW_IO_ERROR( wxS( "OrCAD legacy Cache: invalid marker" ) );

        for( int section = 0; section < 4; ++section )
        {
            uint16_t groupCount = stream.ReadU16();

            v2CheckCount( stream, groupCount );

            for( uint16_t group = 0; group < groupCount; ++group )
            {
                stream.ReadLzt(); // group name

                uint16_t variantCount = stream.ReadU16();

                v2CheckCount( stream, variantCount );

                for( uint16_t variant = 0; variant < variantCount; ++variant )
                {
                    stream.ReadLzt(); // source library
                    stream.ReadU32(); // created
                    stream.ReadU32(); // modified

                    int typeId = stream.ReadU8();

                    stream.ExpectByte( 0, wxS( "legacy Cache entry pad" ) );

                    if( typeId == ORCAD_ST_PART_CELL )
                    {
                        v2CachePartCell( stream, aStrings );
                    }
                    else if( typeId == ORCAD_ST_PACKAGE )
                    {
                        ORCAD_PACKAGE pkg = v2Package( stream, aStrings );
                        auto          existing = aPackages.find( pkg.name );

                        if( existing == aPackages.end() )
                            aPackages.emplace( pkg.name, std::move( pkg ) );
                        else
                            existing->second.variants.push_back( std::move( pkg ) );
                    }
                    else
                    {
                        std::map<std::string, std::string> props;
                        uint8_t          bodyType = v2Prefix( stream, aStrings, &props );
                        ORCAD_SYMBOL_DEF def = v2LibSymbolDef( stream, aStrings, bodyType );
                        def.props = std::move( props );

                        // Only a LibraryPart carries the implementation and reference-prefix
                        // tail; the other symbol types end at their definition.
                        if( typeId == ORCAD_ST_LIBRARY_PART )
                            v2LibraryPartTail( stream, def );

                        if( !def.name.empty() )
                        {
                            auto existing = aSymbols.find( def.name );

                            if( existing == aSymbols.end() )
                            {
                                aSymbols.emplace( def.name, std::move( def ) );
                            }
                            else
                            {
                                if( existing->second.generalFlags < 0 && def.generalFlags >= 0 )
                                    existing->second.generalFlags = def.generalFlags;

                                existing->second.variants.push_back( std::move( def ) );
                            }
                        }
                    }
                }
            }
        }

        if( !stream.AtEnd() )
            THROW_IO_ERRORF( wxS( "OrCAD legacy Cache: %zu trailing bytes" ), stream.Remaining() );
    }
    catch( const IO_ERROR& e )
    {
        // Nothing above LoadSchematicFile catches IO_ERROR, so a cache we cannot frame keeps
        // whatever was read rather than failing the whole import.
        if( aWarn )
        {
            aWarn( wxString::Format( _( "The legacy design cache could not be read past 0x%zx (%s); the "
                                        "remaining symbols fall back to placeholders." ),
                                     stream.GetOffset(), e.What() ) );
        }
    }
}


void OrcadParseOlbPackageStreamV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                   std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                                   std::map<std::string, ORCAD_PACKAGE>& aPackages, bool aShortDisplayProp )
{
    V2_DISPLAY_PROP_SCOPE displayPropScope( aShortDisplayProp );
    ORCAD_STREAM          stream( aData );

    uint16_t partCount = stream.ReadU16();
    v2CheckCount( stream, partCount );
    std::vector<V2_PART_CELL> cells;

    for( uint16_t i = 0; i < partCount; i++ )
        cells.push_back( v2PartCell( stream, aStrings ) );

    ORCAD_PACKAGE pkg = v2Package( stream, aStrings );

    if( stream.Remaining() != 0 )
        THROW_IO_ERROR( wxS( "v2 package stream: trailing bytes" ) );

    // Inline symbol keyed by view name ("7400.Normal"); view suffix later stripped to part base.
    for( const V2_PART_CELL& cell : cells )
    {
        for( const ORCAD_SYMBOL_DEF& def : cell.symbols )
        {
            if( def.name.empty() )
                continue;

            auto existing = aSymbols.find( def.name );

            if( existing == aSymbols.end() )
                aSymbols.emplace( def.name, def );
            else
                existing->second.variants.push_back( def );
        }

        if( pkg.refDes.empty() && !cell.refDesPrefix.empty() )
            pkg.refDes = cell.refDesPrefix;
    }

    if( pkg.name.empty() )
        return;

    auto it = aPackages.find( pkg.name );

    if( it == aPackages.end() )
        aPackages.emplace( pkg.name, std::move( pkg ) );
    else
        it->second.variants.push_back( std::move( pkg ) );
}


static void v2ReadOccurrence( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings,
                              ORCAD_OCC_SCOPE& aScope );


static ORCAD_OCC_SCOPE v2ReadOccScope( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings )
{
    ORCAD_STREAM::NEST_GUARD guard( aStream, wxS( "v2 occurrence scope" ) );
    ORCAD_OCC_SCOPE          scope;

    uint16_t netCount = aStream.ReadU16();
    v2CheckCount( aStream, netCount );

    for( uint16_t i = 0; i < netCount; i++ )
    {
        if( v2Prefix( aStream, aStrings ) != 0x43 )
            THROW_IO_ERROR( wxS( "v2 occurrence scope: unexpected net record" ) );

        uint32_t    occurrenceId = aStream.ReadU32();
        std::string name = aStream.ReadLzt();

        scope.netNames[occurrenceId] = std::move( name );
    }

    uint16_t titleBlockCount = aStream.ReadU16();
    v2CheckCount( aStream, titleBlockCount );

    for( uint16_t i = 0; i < titleBlockCount; i++ )
    {
        if( v2Prefix( aStream, aStrings ) != 0x52 )
            THROW_IO_ERROR( wxS( "v2 occurrence scope: unexpected title-block record" ) );

        aStream.ReadU32();
        aStream.ReadU32();
    }

    uint16_t occurrenceCount = aStream.ReadU16();
    v2CheckCount( aStream, occurrenceCount );

    for( uint16_t i = 0; i < occurrenceCount; i++ )
        v2ReadOccurrence( aStream, aStrings, scope );

    return scope;
}


static void v2ReadOccurrence( ORCAD_STREAM& aStream, const std::vector<std::string>& aStrings,
                              ORCAD_OCC_SCOPE& aScope )
{
    std::map<std::string, std::string> props;

    if( v2Prefix( aStream, aStrings, &props ) != 0x42 )
        THROW_IO_ERROR( wxS( "v2 occurrence: unexpected record" ) );

    aStream.ReadU32();
    uint32_t    targetDbId = aStream.ReadU32();
    std::string child = aStream.ReadLzt();
    std::string ref = aStream.ReadLzt();
    uint16_t    unitRefIdx = aStream.ReadU16();

    uint16_t pinCount = aStream.ReadU16();
    v2CheckCount( aStream, pinCount );
    for( uint16_t i = 0; i < pinCount; i++ )
    {
        uint8_t type = v2Prefix( aStream, aStrings );

        if( type != 0x44 && type != 0x45 )
            THROW_IO_ERROR( wxS( "v2 occurrence: unexpected pin record" ) );

        aStream.ReadU32();
        aStream.ReadU16();
    }

    ORCAD_OCC_SCOPE nested = v2ReadOccScope( aStream, aStrings );

    if( !child.empty() )
    {
        ORCAD_OCC_BLOCK block;
        block.targetDbId = targetDbId;
        block.childFolder = std::move( child );
        block.scope = std::move( nested );
        aScope.blocks.push_back( std::move( block ) );
    }
    else
    {
        if( !ref.empty() )
            aScope.partRefs[targetDbId] = ref;

        if( !props.empty() )
            aScope.partProps[targetDbId] = std::move( props );

        if( unitRefIdx < aStrings.size() && !aStrings[unitRefIdx].empty() )
        {
            aScope.partUnitRefs[targetDbId] = aStrings[unitRefIdx];
        }
    }
}


ORCAD_OCC_SCOPE OrcadReadOccurrenceTreeV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings )
{
    ORCAD_STREAM stream( aData );
    stream.ReadLzt();
    stream.Skip( 5 );

    uint16_t powerCount = stream.ReadU16();
    v2CheckCount( stream, powerCount );

    for( uint16_t i = 0; i < powerCount; ++i )
    {
        if( v2Prefix( stream, aStrings ) != 0x44 )
            THROW_IO_ERROR( wxS( "v2 occurrence tree: unexpected power record" ) );

        stream.ReadU32();
        stream.ReadLzt();
    }

    ORCAD_OCC_SCOPE root = v2ReadOccScope( stream, aStrings );

    if( stream.Remaining() != 0 )
        THROW_IO_ERRORF( wxS( "v2 occurrence tree: %zu trailing bytes" ), stream.Remaining() );

    return root;
}


ORCAD_OCC_SCOPE OrcadReadOccurrenceTree( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                         const ORCAD_WARN_FN& aWarn )
{
    // Modern streams open w/ type-0x42 long prefix (u8 0x42, u32 bodyLen, u32 0); legacy pre-preamble
    // streams start at view-name lzt with only instance-annotated refs, so empty tree is correct.
    if( aData.size() < 9 || (uint8_t) aData[0] != 0x42 || aData[5] || aData[6] || aData[7] || aData[8] )
    {
        return {};
    }

    ORCAD_STREAM stream( aData );

    try
    {
        stream.ReadU8();  // 0x42
        stream.ReadU32(); // bodyLen
        stream.ReadU32(); // 0
        stream.ReadLzt(); // view name
        stream.Skip( 7 ); // zeros

        uint16_t powerCount = stream.ReadU16();

        for( uint16_t i = 0; i < powerCount; ++i )
        {
            readOccHeader( stream, aStrings, 0x44 );
            stream.ReadU32();
            stream.ReadLzt();
        }

        return readOccScope( stream, aStrings );
    }
    catch( const IO_ERROR& e )
    {
        aWarn( wxString::Format( wxS( "The design occurrence tree could not be fully read (%s); "
                                      "reference designators fall back to the placed instances." ),
                                 e.What() ) );
        return {};
    }
}
