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

#include <sch_io/orcad/orcad_cache.h>

#include <algorithm>
#include <utility>
#include <set>

#include <ki_exception.h>


namespace
{

bool isPrimType( int aType )
{
    switch( aType )
    {
    case ORCAD_PRIM_RECT:
    case ORCAD_PRIM_LINE:
    case ORCAD_PRIM_ARC:
    case ORCAD_PRIM_ELLIPSE:
    case ORCAD_PRIM_POLYGON:
    case ORCAD_PRIM_POLYLINE:
    case ORCAD_PRIM_COMMENT_TEXT:
    case ORCAD_PRIM_BITMAP:
    case ORCAD_PRIM_SYMBOL_VECTOR:
    case ORCAD_PRIM_BEZIER:
    case ORCAD_PRIM_OLE_IMAGE: return true;
    default: return false;
    }
}


bool isSymbolType( int aTypeId )
{
    switch( aTypeId )
    {
    case ORCAD_ST_LIBRARY_PART:
    case ORCAD_ST_GLOBAL_SYMBOL:
    case ORCAD_ST_PORT_SYMBOL:
    case ORCAD_ST_OFFPAGE_SYMBOL:
    case ORCAD_ST_TITLEBLOCK_SYMBOL:
    case ORCAD_ST_ERC_SYMBOL:
    case ORCAD_ST_BOOKMARK_SYMBOL: return true;

    // ORCAD_ST_PIN_SHAPE_SYMBOL is absent on purpose. It has no registered prefix depth, so
    // claiming it here would promise a decode we cannot frame. No corpus file contains one.
    default: return false;
    }
}


std::optional<ORCAD_PRIMITIVE> readPrimitiveBody( ORCAD_STREAM& aStream, int aType );


// Nested prefix-framed vector graphic inside library parts.  Entered on the second of the two
// type bytes, which is where the vector's own one-long-prefix chain begins.
std::optional<ORCAD_PRIMITIVE> readSymbolVector( ORCAD_STREAM& aStream )
{
    ORCAD_STREAM::NEST_GUARD guard( aStream, wxS( "symbol vector" ) );

    ORCAD_STRUCT_READER       reader( aStream );
    ORCAD_PREFIXES            pfx = reader.ReadPrefixes( ORCAD_ST_SYMBOL_VECTOR, ORCAD_STREAM::npos, 1 );
    ORCAD_STREAM::LIMIT_GUARD limit( aStream, pfx.end );

    ORCAD_PRIMITIVE group;
    group.kind = ORCAD_PRIM_KIND::GROUP_PRIM;
    group.x1 = aStream.ReadI16();
    group.y1 = aStream.ReadI16();

    uint16_t count = aStream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        // Nested prims use 3-byte prefix: type, 0x00, type.
        int t = aStream.ReadU8();
        aStream.ExpectByte( 0x00, wxS( "symbol vector prim pad" ) );

        if( !isPrimType( t ) )
            THROW_IO_ERROR( wxS( "symbol vector prim prefix mismatch" ) );

        std::optional<ORCAD_PRIMITIVE> child;

        if( t == ORCAD_PRIM_SYMBOL_VECTOR )
        {
            child = readSymbolVector( aStream );
        }
        else
        {
            int t2 = aStream.ReadU8();

            if( t != t2 )
                THROW_IO_ERROR( wxS( "symbol vector prim prefix mismatch" ) );

            child = readPrimitiveBody( aStream, t );
        }

        if( child )
            group.children.push_back( std::move( *child ) );
    }

    aStream.ReadLzt(); // vector name

    if( pfx.end != 0 )
        aStream.Seek( std::max( aStream.GetOffset(), pfx.end ) );

    return group;
}

std::optional<ORCAD_PRIMITIVE> readPrimitiveBody( ORCAD_STREAM& aStream, int t1 )
{
    size_t start = aStream.GetOffset();
    size_t size = aStream.ReadU32();
    size_t available = aStream.Size() - start;

    if( size > available )
        THROW_IO_ERRORF( wxS( "primitive type %d at 0x%zx exceeds its enclosing structure" ), t1, start );

    // Two byteLength conventions are mixed even within one file: the modern one counts the u32
    // size and its 4-byte pad, the legacy one excludes them. The bound below admits both.
    size_t recordSize = size;

    if( t1 != ORCAD_PRIM_OLE_IMAGE && available - size >= 8 )
        recordSize += 8;

    std::optional<ORCAD_PRIMITIVE> prim;

    {
        ORCAD_STREAM::LIMIT_GUARD limit( aStream, start + recordSize );

        static const uint8_t pad[4] = { 0x00, 0x00, 0x00, 0x00 };
        aStream.Expect( pad, 4, wxS( "primitive pad" ) );

        if( t1 == ORCAD_PRIM_RECT || t1 == ORCAD_PRIM_ELLIPSE )
        {
            ORCAD_PRIMITIVE p;
            p.kind = t1 == ORCAD_PRIM_RECT ? ORCAD_PRIM_KIND::RECTANGLE : ORCAD_PRIM_KIND::ELLIPSE;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
            p.fillStyle = aStream.ReadU32();
            p.hatchStyle = aStream.ReadU32();
            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_LINE )
        {
            ORCAD_PRIMITIVE p;
            p.kind = ORCAD_PRIM_KIND::LINE;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_ARC )
        {
            ORCAD_PRIMITIVE p;
            p.kind = ORCAD_PRIM_KIND::ARC;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();

            ORCAD_POINT arcStart;
            arcStart.x = aStream.ReadI32();
            arcStart.y = aStream.ReadI32();

            ORCAD_POINT arcEnd;
            arcEnd.x = aStream.ReadI32();
            arcEnd.y = aStream.ReadI32();

            p.start = arcStart;
            p.end = arcEnd;
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_POLYGON || t1 == ORCAD_PRIM_POLYLINE || t1 == ORCAD_PRIM_BEZIER )
        {
            ORCAD_PRIMITIVE p;
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();

            if( t1 == ORCAD_PRIM_POLYGON )
            {
                p.kind = ORCAD_PRIM_KIND::POLYGON;
                p.fillStyle = aStream.ReadU32();
                p.hatchStyle = aStream.ReadU32();
            }
            else if( t1 == ORCAD_PRIM_POLYLINE )
            {
                p.kind = ORCAD_PRIM_KIND::POLYLINE;
            }
            else
            {
                p.kind = ORCAD_PRIM_KIND::BEZIER;
            }

            uint16_t pointCount = aStream.ReadU16();

            for( uint16_t i = 0; i < pointCount; i++ )
            {
                ORCAD_POINT pt;
                pt.y = aStream.ReadI16();
                pt.x = aStream.ReadI16();
                p.points.push_back( pt );
            }

            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_COMMENT_TEXT )
        {
            ORCAD_PRIMITIVE p;
            p.kind = ORCAD_PRIM_KIND::TEXT;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();
            p.textBoundsStart = ORCAD_POINT{ aStream.ReadI32(), aStream.ReadI32() };
            p.fontIdx = aStream.ReadU16();
            aStream.Skip( 2 );
            p.text = aStream.ReadLzt();
            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_BITMAP )
        {
            ORCAD_PRIMITIVE p;
            p.kind = ORCAD_PRIM_KIND::IMAGE;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();
            aStream.Skip( 8 ); // x1, y1 duplicate corner
            aStream.Skip( 8 ); // pixel width/height

            uint32_t dataSize = aStream.ReadU32();
            p.data = aStream.ReadBytes( dataSize );
            prim = std::move( p );
        }
        else if( t1 == ORCAD_PRIM_OLE_IMAGE )
        {
            ORCAD_PRIMITIVE p;
            p.kind = ORCAD_PRIM_KIND::IMAGE;
            p.x1 = aStream.ReadI32();
            p.y1 = aStream.ReadI32();
            p.x2 = aStream.ReadI32();
            p.y2 = aStream.ReadI32();
            aStream.Skip( 16 ); // crop/original-extent values

            // OLE compound-document payload fills the rest of the record.
            size_t from = aStream.GetOffset();
            size_t to = start + size;

            if( to < from )
                THROW_IO_ERRORF( wxS( "OLE primitive at 0x%zx is shorter than its header" ), start );

            if( to > from )
                p.data.assign( aStream.Data() + from, aStream.Data() + to );

            aStream.Seek( to );
            prim = std::move( p );
        }

        // CommentText keeps undecoded padding after the string, so its length owns the extent.
        // Every other type is decoded in full, so its length must agree with one of the conventions.
        if( t1 == ORCAD_PRIM_COMMENT_TEXT )
        {
            aStream.Seek( start + recordSize );
        }
        else
        {
            size_t physicalSize = aStream.GetOffset() - start;
            bool   validSize = t1 == ORCAD_PRIM_OLE_IMAGE ? size == physicalSize
                                                          : size == physicalSize || size + 8 == physicalSize;

            if( !validSize )
            {
                THROW_IO_ERRORF( wxS( "primitive type %d stores %zu bytes but consumes %zu" ), t1, size, physicalSize );
            }
        }
    }

    aStream.SkipOptionalPreambleBlock();
    return prim;
}

} // namespace


std::optional<ORCAD_PRIMITIVE> OrcadReadPrimitive( ORCAD_STREAM& aStream )
{
    int t1 = aStream.ReadU8();

    if( !isPrimType( t1 ) )
        THROW_IO_ERRORF( wxS( "bad primitive type %d at 0x%zx" ), t1, aStream.GetOffset() - 1 );

    if( t1 == ORCAD_PRIM_SYMBOL_VECTOR )
        return readSymbolVector( aStream );

    int t2 = aStream.ReadU8();

    if( t1 != t2 )
        THROW_IO_ERRORF( wxS( "bad primitive prefix %d/%d at 0x%zx" ), t1, t2, aStream.GetOffset() - 2 );

    return readPrimitiveBody( aStream, t1 );
}


std::optional<ORCAD_SYMBOL_PIN> OrcadReadSymbolPin( ORCAD_STRUCT_READER& aReader )
{
    ORCAD_STREAM& stream = aReader.Stream();

    // Single 0x00 instead of prefix chain = skipped pin slot.
    if( stream.PeekU8() == 0x00 )
    {
        stream.Skip( 1 );
        return std::nullopt;
    }

    ORCAD_PREFIXES pfx = aReader.ReadPrefixes();

    if( pfx.typeId != ORCAD_ST_SYMBOL_PIN_SCALAR && pfx.typeId != ORCAD_ST_SYMBOL_PIN_BUS )
        THROW_IO_ERRORF( wxS( "expected symbol pin, got type %d" ), pfx.typeId );

    ORCAD_SYMBOL_PIN pin;
    pin.name = stream.ReadLzt();
    pin.startX = stream.ReadI32();
    pin.startY = stream.ReadI32();
    pin.hotptX = stream.ReadI32();
    pin.hotptY = stream.ReadI32();
    pin.shapeBits = stream.ReadU16();
    stream.Skip( 2 ); // uninitialized junk

    uint32_t portType = stream.ReadU32();
    pin.portType = portType <= 7 ? static_cast<ORCAD_PORT_TYPE>( portType ) : ORCAD_PORT_TYPE::PASSIVE;

    if( pfx.end != 0 && pfx.end >= stream.GetOffset() + 6 && stream.PeekU8() == pfx.typeId )
    {
        stream.ExpectByte( static_cast<uint8_t>( pfx.typeId ), wxS( "symbol pin type echo" ) );
        stream.Skip( 3 );
        pin.displayProps = OrcadReadDisplayPropList( aReader );
    }

    if( pfx.end != 0 && pfx.end >= stream.GetOffset() )
        stream.Seek( pfx.end );

    return pin;
}


ORCAD_SYMBOL_DEF OrcadReadSymbolDef( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes, bool aWithPins )
{
    ORCAD_STREAM& stream = aReader.Stream();

    std::vector<size_t> stops = aPrefixes.stops;
    std::sort( stops.begin(), stops.end() );

    ORCAD_SYMBOL_DEF sym;
    sym.typeId = aPrefixes.typeId;
    sym.name = stream.ReadLzt();
    sym.sourceLib = stream.ReadLzt();
    sym.props = aReader.PropsDict( aPrefixes );

    sym.color = static_cast<int>( stream.ReadU32() );

    uint16_t primCount = stream.ReadU16();

    for( uint16_t i = 0; i < primCount; i++ )
    {
        std::optional<ORCAD_PRIMITIVE> prim = OrcadReadPrimitive( stream );

        if( prim )
            sym.primitives.push_back( std::move( *prim ) );

        // Occasional 8 zero bytes between prims.
        static const uint8_t zeroTrailer[8] = {};

        if( i + 1 < primCount && stream.PeekMatches( zeroTrailer, sizeof( zeroTrailer ) ) )
            stream.Skip( 8 );
    }

    // Bbox = last 8 bytes before next checkpoint (gap 8, or 16 w/ 8 legacy-trailer bytes), as 4x i16.
    auto nextStopIt = std::upper_bound( stops.begin(), stops.end(), stream.GetOffset() );

    if( nextStopIt != stops.end() )
    {
        size_t nextStop = *nextStopIt;
        size_t gap = nextStop - stream.GetOffset();

        if( gap >= 8 )
        {
            stream.Seek( nextStop - 8 );

            int x1 = stream.ReadI16();
            int y1 = stream.ReadI16();
            int x2 = stream.ReadI16();
            int y2 = stream.ReadI16();

            if( x1 <= x2 && y1 <= y2 && x2 - x1 <= 4000 && y2 - y1 <= 4000 )
            {
                ORCAD_BBOX box;
                box.x1 = x1;
                box.y1 = y1;
                box.x2 = x2;
                box.y2 = y2;
                sym.bbox = box;
            }
        }
        else if( gap > 0 )
        {
            stream.Seek( nextStop );
        }
    }

    if( aWithPins )
    {
        uint16_t pinCount = stream.ReadU16();

        for( uint16_t i = 0; i < pinCount; i++ )
        {
            std::optional<ORCAD_SYMBOL_PIN> pin = OrcadReadSymbolPin( aReader );

            if( pin )
            {
                pin->position = i;
                sym.pins.push_back( std::move( *pin ) );
            }
        }

        uint16_t propCount = stream.ReadU16();

        for( uint16_t i = 0; i < propCount; i++ )
            aReader.ReadStructure();

        // The fifth prefix bounds LibraryPart metadata. Reject data outside those stops.
        if( sym.typeId == ORCAD_ST_LIBRARY_PART && aPrefixes.stops.size() >= 2 )
        {
            size_t save = stream.GetOffset();
            size_t tailStart = aPrefixes.stops[1];

            if( stream.GetOffset() == tailStart && tailStart < aPrefixes.end )
            {
                std::string implementationPath = stream.ReadLzt();
                std::string implementation = stream.ReadLzt();
                stream.ReadLzt(); // reference prefix
                stream.ReadLzt(); // part value

                int flags = stream.ReadU16();

                // Landing anywhere but the outer stop means the strings were not the tail.
                if( stream.GetOffset() == aPrefixes.end )
                {
                    if( !implementationPath.empty() )
                        sym.props["Implementation Path"] = std::move( implementationPath );

                    if( !implementation.empty() )
                        sym.props["Implementation"] = std::move( implementation );

                    sym.generalFlags = flags;
                }
            }

            stream.Seek( save );
        }
    }

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return sym;
}


ORCAD_SYMBOL_DEF OrcadReadSthInPages0( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    return OrcadReadSymbolDef( aReader, aPrefixes, false );
}


ORCAD_DRAWN_INSTANCE OrcadReadDrawnInstance( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM& stream = aReader.Stream();

    uint32_t nameIdx = stream.ReadU32();
    stream.ReadU32(); // source library string index
    stream.ReadLzt(); // name

    uint32_t dbId = stream.ReadU32();

    stream.ReadI16(); // anchor y
    stream.ReadI16(); // anchor x
    stream.ReadI16(); // bbox y2
    stream.ReadI16(); // bbox x2

    int x1 = stream.ReadI16();
    int y1 = stream.ReadI16();

    stream.ReadU8(); // color
    uint8_t orientation = stream.ReadU8();
    stream.Skip( 2 ); // structId, unknown

    std::vector<ORCAD_DISPLAY_PROP> displayProps = OrcadReadDisplayPropList( aReader );

    // Inline LibraryPart carries block's pin interface.
    uint8_t flag = stream.ReadU8();

    if( flag != ORCAD_ST_LIBRARY_PART )
    {
        THROW_IO_ERRORF( wxS( "drawn instance nested flag %d at 0x%zx" ), static_cast<int>( flag ),
                         stream.GetOffset() - 1 );
    }

    ORCAD_PREFIXES   nestedPfx = aReader.ReadPrefixes( ORCAD_ST_LIBRARY_PART, aPrefixes.end );
    ORCAD_SYMBOL_DEF nested = OrcadReadSymbolDef( aReader, nestedPfx, true );

    ORCAD_BBOX bbox = nested.bbox.value_or( ORCAD_BBOX() );

    ORCAD_DRAWN_INSTANCE block;
    block.dbId = dbId;
    block.name = aReader.Resolve( nameIdx );
    block.props = aReader.PropsDict( aPrefixes );
    block.x1 = x1;
    block.y1 = y1;
    bool quarterTurn = ( orientation & 1 ) != 0;
    block.w = quarterTurn ? bbox.y2 - bbox.y1 : bbox.x2 - bbox.x1;
    block.h = quarterTurn ? bbox.x2 - bbox.x1 : bbox.y2 - bbox.y1;
    block.displayProps = std::move( displayProps );

    // Block reference starts at second-to-last prefix checkpoint.
    std::vector<size_t> stops = aPrefixes.stops;
    std::sort( stops.begin(), stops.end() );

    if( stops.size() >= 2 && stops[stops.size() - 2] >= stream.GetOffset() )
        stream.Seek( stops[stops.size() - 2] );

    block.reference = stream.ReadLzt();
    stream.Skip( 14 );

    // Framed T0x10 structs carry absolute pin page positions, in inline LibraryPart pin order.
    uint16_t pinCount = stream.ReadU16();

    std::vector<ORCAD_PIN_INST> pinInsts;

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        ORCAD_READ_RESULT result = aReader.ReadStructure();

        if( ORCAD_PIN_INST* pin = std::get_if<ORCAD_PIN_INST>( &result.record ) )
            pinInsts.push_back( std::move( *pin ) );
    }

    std::set<std::pair<int, int>> placedPoints;
    std::set<std::pair<int, int>> definitionPoints;

    for( const ORCAD_PIN_INST& pin : pinInsts )
        placedPoints.emplace( pin.x, pin.y );

    for( const ORCAD_SYMBOL_PIN& pin : nested.pins )
        definitionPoints.emplace( pin.hotptX, pin.hotptY );

    bool useDefinitionGeometry = placedPoints.size() <= 1 && definitionPoints.size() > 1;

    for( size_t i = 0; i < pinInsts.size() && i < nested.pins.size(); i++ )
    {
        const ORCAD_SYMBOL_PIN& pin = nested.pins[i];

        ORCAD_BLOCK_PIN blockPin;
        blockPin.name = pin.name;
        blockPin.portType = pin.portType;
        blockPin.x = useDefinitionGeometry ? x1 + pin.hotptX - bbox.x1 : pinInsts[i].x;
        blockPin.y = useDefinitionGeometry ? y1 + pin.hotptY - bbox.y1 : pinInsts[i].y;
        blockPin.noConnect = pinInsts[i].IsNoConnect();
        block.pins.push_back( std::move( blockPin ) );
    }

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return block;
}


ORCAD_DEVICE OrcadReadDevice( ORCAD_STRUCT_READER& aReader )
{
    ORCAD_STREAM&  stream = aReader.Stream();
    ORCAD_PREFIXES pfx = aReader.ReadPrefixes( ORCAD_ST_DEVICE );

    ORCAD_DEVICE device;
    device.unitRef = stream.ReadLzt();
    device.refDes = stream.ReadLzt();

    uint16_t pinCount = stream.ReadU16();

    for( uint16_t i = 0; i < pinCount; i++ )
    {
        // FF FF = empty pin slot (no number, ignored).
        static const uint8_t emptyMarker[2] = { 0xFF, 0xFF };

        if( stream.PeekMatches( emptyMarker, 2 ) )
        {
            stream.Skip( 2 );
            device.pinNumbers.emplace_back();
            device.pinIgnore.push_back( true );
            continue;
        }

        device.pinNumbers.push_back( stream.ReadLzt() );

        uint8_t config = stream.ReadU8();
        device.pinIgnore.push_back( ( config & 0x80 ) != 0 );
    }

    if( pfx.end != 0 && pfx.end > stream.GetOffset() )
        stream.Seek( pfx.end );

    return device;
}


ORCAD_PACKAGE OrcadReadPackage( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM& stream = aReader.Stream();

    ORCAD_PACKAGE pkg;
    pkg.name = stream.ReadLzt();
    pkg.sourceLib = stream.ReadLzt();
    pkg.refDes = stream.ReadLzt();
    stream.ReadLzt(); // unknown
    pkg.pcbFootprint = stream.ReadLzt();

    uint16_t deviceCount = stream.ReadU16();

    for( uint16_t i = 0; i < deviceCount; i++ )
        pkg.devices.push_back( OrcadReadDevice( aReader ) );

    pkg.props = aReader.PropsDict( aPrefixes );

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return pkg;
}


// Store one framed Cache/Packages record.  The Cache may hold several stale library versions of
// one name; the first entry wins as the default and later ones become variants.
static void storeFramedRecord( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes,
                               std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                               std::map<std::string, ORCAD_PACKAGE>&    aPackages )
{
    ORCAD_STREAM::LIMIT_GUARD limit( aReader.Stream(), aPrefixes.end );

    if( isSymbolType( aPrefixes.typeId ) )
    {
        ORCAD_SYMBOL_DEF symbol = OrcadReadSymbolDef( aReader, aPrefixes, true );
        auto             existing = aSymbols.find( symbol.name );

        if( existing == aSymbols.end() )
        {
            std::string key = symbol.name;
            aSymbols.emplace( std::move( key ), std::move( symbol ) );
        }
        else
        {
            existing->second.variants.push_back( std::move( symbol ) );
        }
    }
    else if( aPrefixes.typeId == ORCAD_ST_PACKAGE )
    {
        ORCAD_PACKAGE package = OrcadReadPackage( aReader, aPrefixes );
        auto          existing = aPackages.find( package.name );

        if( existing == aPackages.end() )
        {
            std::string key = package.name;
            aPackages.emplace( std::move( key ), std::move( package ) );
        }
        else
        {
            existing->second.variants.push_back( std::move( package ) );
        }
    }
    else
    {
        aReader.SkipStructure( aPrefixes, wxString::Format( wxS( "type %d" ), aPrefixes.typeId ) );
    }
}


// The type each Cache section is allowed to hold. Section membership is what proves the walk
// is still aligned, so a section carrying the wrong type is a framing error, not a bad record.
static bool cacheSectionAcceptsType( int aSection, int aTypeId )
{
    switch( aSection )
    {
    // Type 24 lives in section 1 and carries a deeper prefix chain. Accepting it here would let
    // a walk that has lost alignment land on a LibraryPart and be waved through.
    case 0: return aTypeId != ORCAD_ST_LIBRARY_PART && isSymbolType( aTypeId );
    case 1: return aTypeId == ORCAD_ST_LIBRARY_PART;
    case 2: return aTypeId == ORCAD_ST_PART_CELL;
    default: return aTypeId == ORCAD_ST_PACKAGE;
    }
}


void OrcadParseCache( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                      const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                      std::map<std::string, ORCAD_PACKAGE>& aPackages )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn );

    // Keep decoded symbols if the cache fails. The remaining parts use placeholders.
    try
    {
        if( stream.ReadU16() != 0 )
            THROW_IO_ERROR( wxS( "OrCAD Cache: invalid marker" ) );

        // Four counted sections: loose symbols, LibraryParts, PartCells, Packages. An empty
        // cache is the marker plus four zero counts, which is the ten-byte stream in the wild.
        for( int section = 0; section < 4; ++section )
        {
            uint16_t groupCount = stream.ReadU16();

            for( uint16_t group = 0; group < groupCount; ++group )
            {
                stream.ReadLzt(); // group name
                uint16_t variantCount = stream.ReadU16();

                for( uint16_t variant = 0; variant < variantCount; ++variant )
                {
                    stream.ReadLzt();  // source library
                    stream.ReadU32();  // created
                    stream.ReadU32();  // modified

                    int typeId = stream.ReadU8();

                    stream.ExpectByte( 0, wxS( "Cache entry pad" ) );

                    if( !cacheSectionAcceptsType( section, typeId ) )
                    {
                        THROW_IO_ERRORF( wxS( "OrCAD Cache: section %d cannot hold structure type %d" ), section,
                                         typeId );
                    }

                    ORCAD_PREFIXES prefixes = reader.ReadPrefixes( typeId );

                    if( prefixes.end == 0 || prefixes.end > stream.Size() )
                        THROW_IO_ERROR( wxS( "OrCAD Cache: entry frame runs past the stream" ) );

                    try
                    {
                        storeFramedRecord( reader, prefixes, aSymbols, aPackages );
                    }
                    catch( const IO_ERROR& e )
                    {
                        // A body we cannot decode is recoverable: the entry's own frame says
                        // where the next one starts.
                        if( aWarn )
                        {
                            aWarn( wxString::Format( wxS( "cache struct type %d at 0x%zx: %s" ), typeId,
                                                     prefixes.start, e.Problem() ) );
                        }
                    }

                    stream.Seek( prefixes.end );
                }
            }
        }

        if( !stream.AtEnd() )
            THROW_IO_ERRORF( wxS( "OrCAD Cache: %zu trailing bytes" ), stream.Remaining() );
    }
    catch( const IO_ERROR& e )
    {
        if( aWarn )
        {
            aWarn( wxString::Format( wxS( "OrCAD Cache: stopped at 0x%zx (%s); symbols after this point fall "
                                          "back to synthesized placeholders" ),
                                     stream.GetOffset(), e.Problem() ) );
        }
    }
}


void OrcadParseSymbolStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                             std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings );
    ORCAD_PREFIXES      prefixes = reader.ReadPrefixes();

    if( !isSymbolType( prefixes.typeId ) )
        THROW_IO_ERRORF( wxS( "OrCAD symbol stream: expected a symbol structure, got type %d" ), prefixes.typeId );

    std::map<std::string, ORCAD_PACKAGE> unusedPackages;
    storeFramedRecord( reader, prefixes, aSymbols, unusedPackages );

    if( !stream.AtEnd() )
        THROW_IO_ERROR( wxS( "OrCAD symbol stream: trailing bytes" ) );
}


void OrcadParsePackageStream( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                              std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                              std::map<std::string, ORCAD_PACKAGE>& aPackages )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings );

    uint16_t partCellCount = stream.ReadU16();

    if( partCellCount > 1000 )
        THROW_IO_ERROR( wxS( "OrCAD package stream: implausible PartCell count" ) );

    for( uint16_t i = 0; i < partCellCount; ++i )
    {
        ORCAD_PREFIXES cellPfx = reader.ReadPrefixes( ORCAD_ST_PART_CELL );

        std::map<std::string, std::string> cellProps = reader.PropsDict( cellPfx );
        stream.ReadLzt(); // PartCell name
        stream.ReadLzt(); // source library

        uint16_t viewCount = stream.ReadU16();

        if( viewCount > 1000 )
            THROW_IO_ERROR( wxS( "OrCAD package stream: implausible view count" ) );

        for( uint16_t view = 0; view < viewCount; ++view )
            stream.ReadLzt();

        if( cellPfx.end == 0 || stream.GetOffset() > cellPfx.end )
            THROW_IO_ERROR( wxS( "OrCAD package stream: PartCell exceeds its frame" ) );

        stream.Seek( cellPfx.end );

        uint16_t symbolCount = stream.ReadU16();

        if( symbolCount > 1000 )
            THROW_IO_ERROR( wxS( "OrCAD package stream: implausible LibraryPart count" ) );

        for( uint16_t symbolIndex = 0; symbolIndex < symbolCount; ++symbolIndex )
        {
            ORCAD_PREFIXES   symbolPfx = reader.ReadPrefixes( ORCAD_ST_LIBRARY_PART );
            ORCAD_SYMBOL_DEF symbol = OrcadReadSymbolDef( reader, symbolPfx, true );

            for( const auto& [name, value] : cellProps )
                symbol.props.try_emplace( name, value );

            auto existing = aSymbols.find( symbol.name );

            if( existing == aSymbols.end() )
            {
                std::string key = symbol.name;
                aSymbols.emplace( std::move( key ), std::move( symbol ) );
            }
            else
            {
                existing->second.variants.push_back( std::move( symbol ) );
            }
        }
    }

    ORCAD_PREFIXES packagePfx = reader.ReadPrefixes( ORCAD_ST_PACKAGE );
    ORCAD_PACKAGE  package = OrcadReadPackage( reader, packagePfx );

    if( stream.Remaining() != 0 )
        THROW_IO_ERROR( wxS( "OrCAD package stream: trailing bytes" ) );

    auto existing = aPackages.find( package.name );

    if( existing == aPackages.end() )
    {
        std::string key = package.name;
        aPackages.emplace( std::move( key ), std::move( package ) );
    }
    else
    {
        existing->second.variants.push_back( std::move( package ) );
    }
}


void OrcadMergeCacheStreams( std::map<std::string, ORCAD_SYMBOL_DEF>&  aSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&     aPackages,
                             std::map<std::string, ORCAD_SYMBOL_DEF>&& aExtraSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&&    aExtraPackages )
{
    for( auto& [name, sym] : aExtraSymbols )
    {
        auto it = aSymbols.find( name );

        if( it == aSymbols.end() )
        {
            aSymbols.emplace( name, std::move( sym ) );
        }
        else
        {
            // Main cache def stays default; extra stream's default and variants appended in stream order.
            std::vector<ORCAD_SYMBOL_DEF> extraVariants = std::move( sym.variants );
            sym.variants.clear();

            if( it->second.generalFlags < 0 && sym.generalFlags >= 0 )
                it->second.generalFlags = sym.generalFlags;

            it->second.variants.push_back( std::move( sym ) );

            for( ORCAD_SYMBOL_DEF& variant : extraVariants )
                it->second.variants.push_back( std::move( variant ) );
        }
    }

    for( auto& [name, pkg] : aExtraPackages )
    {
        auto it = aPackages.find( name );

        if( it == aPackages.end() )
        {
            aPackages.emplace( name, std::move( pkg ) );
        }
        else
        {
            std::vector<ORCAD_PACKAGE> variants = std::move( pkg.variants );
            pkg.variants.clear();
            it->second.variants.push_back( std::move( pkg ) );

            for( ORCAD_PACKAGE& variant : variants )
                it->second.variants.push_back( std::move( variant ) );
        }
    }
}


void OrcadMergeSymbolGeneralProperties( std::map<std::string, ORCAD_SYMBOL_DEF>&       aSymbols,
                                        const std::map<std::string, ORCAD_SYMBOL_DEF>& aMetadataSymbols )
{
    for( const auto& [name, metadata] : aMetadataSymbols )
    {
        auto symbol = aSymbols.find( name );

        if( symbol != aSymbols.end() && symbol->second.generalFlags < 0 && metadata.generalFlags >= 0 )
            symbol->second.generalFlags = metadata.generalFlags;
    }
}
