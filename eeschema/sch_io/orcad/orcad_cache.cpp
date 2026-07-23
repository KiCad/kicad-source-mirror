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
#include <array>
#include <utility>

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
    case ORCAD_PRIM_OLE_IMAGE:
        return true;
    default:
        return false;
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
    case ORCAD_ST_BOOKMARK_SYMBOL:
    case ORCAD_ST_PIN_SHAPE_SYMBOL:
        return true;
    default:
        return false;
    }
}


// Struct type bytes seen in cache streams; validates backtracked prefix-chain candidates.
bool isKnownStructType( uint8_t aType )
{
    static const std::array<bool, 256> known = []()
    {
        std::array<bool, 256> table{};

        for( int t : { 2, 4, 6, 9, 10, 11, 12, 13, 16, 20, 21, 23, 24, 26, 27, 29, 31, 32, 33,
                       34, 35, 37, 38, 39, 48, 49, 52, 53, 55, 56, 57, 58, 59, 60, 61, 62, 64,
                       65, 66, 67, 68, 69, 75, 76, 77, 78, 82, 88, 89, 91, 98, 103 } )
        {
            table[t] = true;
        }

        return table;
    }();

    return known[aType];
}


std::optional<ORCAD_PRIMITIVE> readPrimitiveBody( ORCAD_STREAM& aStream, int aType );


// Nested prefix-framed vector graphic inside library parts.
std::optional<ORCAD_PRIMITIVE> readSymbolVector( ORCAD_STREAM& aStream )
{
    // Type bytes (48,48) consumed; own prefix chain starts at second, so rewind one byte.
    aStream.Seek( aStream.GetOffset() - 1 );

    ORCAD_STRUCT_READER reader( aStream );
    ORCAD_PREFIXES      pfx = reader.ReadPrefixes();

    ORCAD_PRIMITIVE group;
    group.kind = ORCAD_PRIM_KIND::GROUP;
    group.x1 = aStream.ReadI16();
    group.y1 = aStream.ReadI16();

    uint16_t count = aStream.ReadU16();

    for( uint16_t i = 0; i < count; i++ )
    {
        // Nested prims use 3-byte prefix: type, 0x00, type.
        int t = aStream.ReadU8();
        aStream.ExpectByte( 0x00, wxS( "symbol vector prim pad" ) );
        int t2 = aStream.ReadU8();

        if( t != t2 || !isPrimType( t ) )
            THROW_IO_ERROR( wxS( "symbol vector prim prefix mismatch" ) );

        std::optional<ORCAD_PRIMITIVE> child;

        if( t == ORCAD_PRIM_SYMBOL_VECTOR )
        {
            aStream.Seek( aStream.GetOffset() - 1 );
            child = readSymbolVector( aStream );
        }
        else
        {
            child = readPrimitiveBody( aStream, t );
        }

        if( child )
            group.children.push_back( std::move( *child ) );
    }

    aStream.ReadLzt();    // vector name

    if( pfx.end != 0 )
        aStream.Seek( std::max( aStream.GetOffset(), pfx.end ) );

    return group;
}

std::optional<ORCAD_PRIMITIVE> readPrimitiveBody( ORCAD_STREAM& aStream, int t1 )
{
    size_t start = aStream.GetOffset();
    size_t size = aStream.ReadU32();

    // CommentText uses exclusive byteLength in all eras.
    if( t1 == ORCAD_PRIM_COMMENT_TEXT )
        size += 8;

    size_t end = start + size;

    // Two byteLength conventions, sometimes mixed in one file: modern counts u32 size + 4-byte pad
    // and is followed by a preamble; legacy excludes those 8 bytes. Detect per record via preamble magic.
    if( t1 != ORCAD_PRIM_COMMENT_TEXT && !aStream.HasPreambleAt( end ) )
        end += 8;    // legacy exclusive-length record

    static const uint8_t pad[4] = { 0x00, 0x00, 0x00, 0x00 };
    aStream.Expect( pad, 4, wxS( "primitive pad" ) );

    size_t clen = end - start;

    std::optional<ORCAD_PRIMITIVE> prim;

    if( t1 == ORCAD_PRIM_RECT || t1 == ORCAD_PRIM_ELLIPSE )
    {
        ORCAD_PRIMITIVE p;
        p.kind = t1 == ORCAD_PRIM_RECT ? ORCAD_PRIM_KIND::RECT : ORCAD_PRIM_KIND::ELLIPSE;
        p.x1 = aStream.ReadI32();
        p.y1 = aStream.ReadI32();
        p.x2 = aStream.ReadI32();
        p.y2 = aStream.ReadI32();

        if( clen >= 32 )
        {
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
        }

        if( clen >= 40 )
        {
            p.fillStyle = aStream.ReadU32();
            p.hatchStyle = aStream.ReadU32();
        }
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

        if( clen >= 32 )
        {
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
        }

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

        if( clen >= 48 )
        {
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();
        }

        prim = std::move( p );
    }
    else if( t1 == ORCAD_PRIM_POLYGON || t1 == ORCAD_PRIM_POLYLINE || t1 == ORCAD_PRIM_BEZIER )
    {
        // Point count found deterministically: must reconcile with stored byteLength under one convention.
        size_t         body = start + 8;
        const uint8_t* data = aStream.Data();

        std::vector<size_t> candidates;

        if( t1 == ORCAD_PRIM_POLYGON )
            candidates = { 16, 8, 0 };
        else
            candidates = { 8, 0 };

        bool   found = false;
        size_t off = 0;

        for( size_t candidate : candidates )
        {
            if( body + candidate + 2 > aStream.Size() )
                continue;

            size_t n = static_cast<size_t>( data[body + candidate] )
                       | static_cast<size_t>( data[body + candidate + 1] ) << 8;
            size_t need = 8 + candidate + 2 + 4 * n;

            if( size == need )
            {
                end = start + size;
                off = candidate;
                found = true;
                break;
            }

            if( size == need - 8 )
            {
                end = start + size + 8;
                off = candidate;
                found = true;
                break;
            }
        }

        if( !found )
        {
            THROW_IO_ERROR( wxString::Format( wxS( "poly primitive at 0x%zx: no consistent "
                                                   "point count for size %zu" ),
                                              start, size ) );
        }

        ORCAD_PRIMITIVE p;

        if( off >= 8 )
        {
            aStream.Seek( body );
            p.lineStyle = aStream.ReadU32();
            p.lineWidth = aStream.ReadU32();

            if( t1 == ORCAD_PRIM_POLYGON && off >= 16 )
            {
                p.fillStyle = aStream.ReadU32();
                p.hatchStyle = aStream.ReadU32();
            }
        }

        aStream.Seek( body + off );

        uint16_t pointCount = aStream.ReadU16();

        if( t1 == ORCAD_PRIM_POLYGON )
            p.kind = ORCAD_PRIM_KIND::POLYGON;
        else if( t1 == ORCAD_PRIM_POLYLINE )
            p.kind = ORCAD_PRIM_KIND::POLYLINE;
        else
            p.kind = ORCAD_PRIM_KIND::BEZIER;

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
        aStream.ReadI32();    // duplicate corner x
        aStream.ReadI32();    // duplicate corner y
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
        aStream.Skip( 8 );    // x1, y1 duplicate corner
        aStream.Skip( 8 );    // pixel width/height

        uint32_t dataSize = aStream.ReadU32();

        // Payload past record end = malformed bitmap; skip whole record.
        if( aStream.GetOffset() + static_cast<size_t>( dataSize ) <= end )
        {
            p.data = aStream.ReadBytes( dataSize );
            prim = std::move( p );
        }
    }
    else if( t1 == ORCAD_PRIM_OLE_IMAGE )
    {
        ORCAD_PRIMITIVE p;
        p.kind = ORCAD_PRIM_KIND::IMAGE;
        p.x1 = aStream.ReadI32();
        p.y1 = aStream.ReadI32();
        p.x2 = aStream.ReadI32();
        p.y2 = aStream.ReadI32();
        aStream.Skip( 16 );    // crop/original-extent values

        // OLE compound-document payload fills rest of record.
        size_t from = std::min( aStream.GetOffset(), aStream.Size() );
        size_t to = std::min( end, aStream.Size() );

        if( to > from )
            p.data.assign( aStream.Data() + from, aStream.Data() + to );

        prim = std::move( p );
    }

    if( aStream.GetOffset() > end )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "primitive type %d overran (0x%zx > 0x%zx)" ), t1,
                                          aStream.GetOffset(), end ) );
    }

    aStream.Seek( end );
    aStream.SkipOptionalPreambleBlock();
    return prim;
}

} // namespace


std::optional<ORCAD_PRIMITIVE> OrcadReadPrimitive( ORCAD_STREAM& aStream )
{
    int t1 = aStream.ReadU8();
    int t2 = aStream.ReadU8();

    if( t1 != t2 || !isPrimType( t1 ) )
    {
        THROW_IO_ERROR(
                wxString::Format( wxS( "bad primitive prefix %d/%d at 0x%zx" ), t1, t2, aStream.GetOffset() - 2 ) );
    }

    if( t1 == ORCAD_PRIM_SYMBOL_VECTOR )
        return readSymbolVector( aStream );

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
        THROW_IO_ERROR( wxString::Format( wxS( "expected symbol pin, got type %d" ), pfx.typeId ) );

    ORCAD_SYMBOL_PIN pin;
    pin.name = stream.ReadLzt();
    pin.startX = stream.ReadI32();
    pin.startY = stream.ReadI32();
    pin.hotptX = stream.ReadI32();
    pin.hotptY = stream.ReadI32();
    pin.shapeBits = stream.ReadU16();
    stream.Skip( 2 );    // uninitialized junk

    uint32_t portType = stream.ReadU32();
    pin.portType = portType <= 7 ? static_cast<ORCAD_PORT_TYPE>( portType )
                                 : ORCAD_PORT_TYPE::PASSIVE;

    // Remaining body is junk/zeros; pin ends at outer stop.
    if( pfx.end != 0 && pfx.end >= stream.GetOffset() )
        stream.Seek( pfx.end );

    return pin;
}


ORCAD_SYMBOL_DEF OrcadReadSymbolDef( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes,
                                     bool aWithPins )
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

        // Occasional 8 zero bytes between prims; detected when next 2 bytes aren't a valid type pair.
        if( i + 1 < primCount )
        {
            int b0 = stream.PeekU8( 0 );
            int b1 = stream.PeekU8( 1 );

            if( b0 >= 0 && b1 >= 0 && !( b0 == b1 && isPrimType( b0 ) ) )
                stream.Skip( 8 );
        }
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

        // LibraryPart GeneralProperties tail ends w/ u16 flags for pin number/name visibility; last before stop
        if( sym.typeId == ORCAD_ST_LIBRARY_PART && aPrefixes.end >= 2
            && aPrefixes.end - 2 >= stream.GetOffset() )
        {
            size_t save = stream.GetOffset();
            stream.Seek( aPrefixes.end - 2 );
            sym.generalFlags = stream.ReadU16();
            stream.Seek( save );
        }
    }

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return sym;
}


ORCAD_SYMBOL_DEF OrcadReadSthInPages0( ORCAD_STRUCT_READER& aReader,
                                       const ORCAD_PREFIXES& aPrefixes )
{
    return OrcadReadSymbolDef( aReader, aPrefixes, false );
}


ORCAD_DRAWN_INSTANCE OrcadReadDrawnInstance( ORCAD_STRUCT_READER& aReader,
                                             const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM& stream = aReader.Stream();

    stream.ReadU32();    // name string index (empty for blocks)
    stream.ReadU32();    // source library string index
    stream.ReadLzt();    // name ("")

    uint32_t dbId = stream.ReadU32();

    stream.ReadI16();    // anchor y
    stream.ReadI16();    // anchor x
    stream.ReadI16();    // bbox y2
    stream.ReadI16();    // bbox x2

    int x1 = stream.ReadI16();
    int y1 = stream.ReadI16();

    stream.Skip( 2 );    // color, orientation
    stream.Skip( 2 );    // structId, unknown

    std::vector<ORCAD_DISPLAY_PROP> displayProps = OrcadReadDisplayPropList( aReader );

    // Inline LibraryPart carries block's pin interface.
    uint8_t flag = stream.ReadU8();

    if( flag != ORCAD_ST_LIBRARY_PART )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "drawn instance nested flag %d at 0x%zx" ),
                                          static_cast<int>( flag ), stream.GetOffset() - 1 ) );
    }

    ORCAD_PREFIXES   nestedPfx = aReader.ReadPrefixes();
    ORCAD_SYMBOL_DEF nested = OrcadReadSymbolDef( aReader, nestedPfx, true );

    ORCAD_BBOX bbox = nested.bbox.value_or( ORCAD_BBOX() );

    ORCAD_DRAWN_INSTANCE block;
    block.dbId = dbId;
    block.x1 = x1;
    block.y1 = y1;
    block.w = bbox.x2 - bbox.x1;
    block.h = bbox.y2 - bbox.y1;
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

    for( size_t i = 0; i < pinInsts.size() && i < nested.pins.size(); i++ )
    {
        const ORCAD_SYMBOL_PIN& pin = nested.pins[i];

        ORCAD_BLOCK_PIN blockPin;
        blockPin.name = pin.name;
        blockPin.portType = pin.portType;
        blockPin.x = pinInsts[i].x;
        blockPin.y = pinInsts[i].y;
        block.pins.push_back( std::move( blockPin ) );
    }

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return block;
}


ORCAD_DEVICE OrcadReadDevice( ORCAD_STRUCT_READER& aReader )
{
    ORCAD_STREAM&  stream = aReader.Stream();
    ORCAD_PREFIXES pfx = aReader.ReadPrefixes();

    if( pfx.typeId != ORCAD_ST_DEVICE )
        THROW_IO_ERROR( wxString::Format( wxS( "expected Device, got type %d" ), pfx.typeId ) );

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
    stream.ReadLzt();    // unknown
    pkg.pcbFootprint = stream.ReadLzt();

    uint16_t deviceCount = stream.ReadU16();

    for( uint16_t i = 0; i < deviceCount; i++ )
        pkg.devices.push_back( OrcadReadDevice( aReader ) );

    if( aPrefixes.end != 0 && aPrefixes.end > stream.GetOffset() )
        stream.Seek( aPrefixes.end );

    return pkg;
}


std::optional<size_t> OrcadFindStructureStart( const ORCAD_STREAM& aStream, size_t aPreamblePos )
{
    const uint8_t* data = aStream.Data();
    size_t         size = aStream.Size();

    // Short prefix = (u8 type, i16 count, count*8 bytes) or (u8 type, i16 -1). Long prefixes 9 bytes each.
    for( int propCount = 0; propCount < 40; propCount++ )
    {
        size_t shortLen = 3 + 8 * static_cast<size_t>( propCount );

        if( aPreamblePos < shortLen )
            break;

        size_t  shortPos = aPreamblePos - shortLen;
        uint8_t type = data[shortPos];

        if( !isKnownStructType( type ) )
            continue;

        int16_t count = static_cast<int16_t>( static_cast<uint16_t>( data[shortPos + 1] )
                                              | static_cast<uint16_t>( data[shortPos + 2] ) << 8 );

        if( count != propCount )
        {
            // Allow -1 marker with no pairs.
            if( !( propCount == 0 && count == -1 ) )
                continue;
        }

        // Count same-type long prefixes (u8 type, u32 len, u32 zero) backwards.
        int    longCount = 0;
        size_t p = shortPos;

        while( p >= 9 )
        {
            size_t q = p - 9;

            if( data[q] == type && data[q + 5] == 0 && data[q + 6] == 0 && data[q + 7] == 0
                && data[q + 8] == 0 )
            {
                longCount++;
                p = q;
            }
            else
            {
                break;
            }
        }

        if( longCount == 0 )
            continue;

        ORCAD_STREAM probe( data, size );
        probe.Seek( p );

        ORCAD_STRUCT_READER reader( probe );
        ORCAD_PREFIXES      pfx;

        try
        {
            pfx = reader.TryReadPrefixes( longCount + 1 );
        }
        catch( const IO_ERROR& )
        {
            continue;
        }

        // Reject chains with extents past stream end.
        bool valid = true;

        for( size_t i = 0; i < pfx.bodyLens.size(); i++ )
        {
            size_t stop = p + 9 * i + 9 + pfx.bodyLens[i];

            if( stop > size )
            {
                valid = false;
                break;
            }
        }

        if( !valid )
            continue;

        return p;
    }

    return std::nullopt;
}


void OrcadParseCache( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                      const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                      std::map<std::string, ORCAD_PACKAGE>& aPackages )
{
    ORCAD_STREAM        stream( aData );
    ORCAD_STRUCT_READER reader( stream, &aStrings, aWarn );

    size_t pos = 0;

    while( true )
    {
        size_t preamblePos = stream.FindPreamble( pos );

        if( preamblePos == ORCAD_STREAM::npos )
            break;

        std::optional<size_t> start = OrcadFindStructureStart( stream, preamblePos );

        if( !start )
        {
            pos = preamblePos + 4;
            continue;
        }

        stream.Seek( *start );

        ORCAD_PREFIXES pfx;

        try
        {
            pfx = reader.ReadPrefixes();
        }
        catch( const IO_ERROR& )
        {
            pos = preamblePos + 4;
            continue;
        }

        int  typeId = pfx.typeId;
        bool parsed = false;

        try
        {
            if( isSymbolType( typeId ) )
            {
                ORCAD_SYMBOL_DEF sym = OrcadReadSymbolDef( reader, pfx, true );
                parsed = true;

                // Cache may hold stale versions of one name; first entry = default, later same-name = variants.
                auto it = aSymbols.find( sym.name );

                if( it == aSymbols.end() )
                {
                    std::string key = sym.name;
                    aSymbols.emplace( std::move( key ), std::move( sym ) );
                }
                else
                {
                    it->second.variants.push_back( std::move( sym ) );
                }
            }
            else if( typeId == ORCAD_ST_PACKAGE )
            {
                ORCAD_PACKAGE pkg = OrcadReadPackage( reader, pfx );
                parsed = true;

                std::string key = pkg.name;
                aPackages.insert_or_assign( std::move( key ), std::move( pkg ) );
            }
        }
        catch( const IO_ERROR& e )
        {
            if( aWarn )
            {
                aWarn( wxString::Format( wxS( "cache struct type %d at 0x%zx: %s" ), typeId,
                                         *start, e.Problem() ) );
            }

            if( pfx.end != 0 && pfx.end > preamblePos )
            {
                stream.Seek( pfx.end );
            }
            else
            {
                pos = preamblePos + 4;
                continue;
            }
        }

        if( parsed || ( pfx.end != 0 && pfx.end > preamblePos ) )
            pos = std::max( stream.GetOffset(), preamblePos + 4 );
        else
            pos = preamblePos + 4;
    }
}


void OrcadMergeCacheStreams( std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                             std::map<std::string, ORCAD_PACKAGE>& aPackages,
                             std::map<std::string, ORCAD_SYMBOL_DEF>&& aExtraSymbols,
                             std::map<std::string, ORCAD_PACKAGE>&& aExtraPackages )
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

            it->second.variants.push_back( std::move( sym ) );

            for( ORCAD_SYMBOL_DEF& variant : extraVariants )
                it->second.variants.push_back( std::move( variant ) );
        }
    }

    for( auto& [name, pkg] : aExtraPackages )
        aPackages.try_emplace( name, std::move( pkg ) );
}
