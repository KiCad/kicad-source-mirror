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

#include <sch_io/orcad/orcad_structures.h>

#include <limits>
#include <memory>
#include <utility>
#include <variant>

#include <ki_exception.h>

#include <sch_io/orcad/orcad_cache.h>


ORCAD_STRUCT_READER::ORCAD_STRUCT_READER( ORCAD_STREAM& aStream, const std::vector<std::string>* aStrings,
                                          ORCAD_WARN_FN aWarn ) :
        m_stream( aStream ),
        m_strings( aStrings ),
        m_warn( std::move( aWarn ) )
{
}


void ORCAD_STRUCT_READER::Warn( const wxString& aMsg ) const
{
    if( m_warn )
        m_warn( aMsg );
}


std::optional<size_t> OrcadLongPrefixCount( int aTypeId )
{
    switch( aTypeId )
    {
    case 9:
    case 66:
    case 67:
    case 68:
    case 69:
    case 82:
    case 91: return 1;

    case 10:
    case 16:
    case 17:
    case 20:
    case 21:
    case 26:
    case 27:
    case 29:
    case 32:
    case 37:
    case 38:
    case 39:
    case 48:
    case 49:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 88:
    case 89: return 2;

    case 2:
    case 6:
    case 12:
    case 23:
    case 31:
    case 65:
    case 77: return 3;

    case 13:
    case 33:
    case 34:
    case 35:
    case 64:
    case 75:
    case 76: return 4;

    case 24: return 5;

    default: return std::nullopt;
    }
}


ORCAD_PREFIXES ORCAD_STRUCT_READER::ReadPrefixes( int aExpectedType, size_t aEnclosingEnd, size_t aLongPrefixCount )
{
    ORCAD_PREFIXES pfx;
    pfx.start = m_stream.GetOffset();

    int typeId = m_stream.PeekU8();

    if( typeId < 0 )
        THROW_IO_ERROR( wxS( "OrCAD structure: missing structure type" ) );

    if( aExpectedType >= 0 && typeId != aExpectedType )
        THROW_IO_ERRORF( wxS( "OrCAD structure: expected type %d, got %d" ), aExpectedType, typeId );

    std::optional<size_t> longCount = aLongPrefixCount == ORCAD_STREAM::npos
                                              ? OrcadLongPrefixCount( typeId )
                                              : std::optional<size_t>( aLongPrefixCount );

    if( !longCount )
        THROW_IO_ERRORF( wxS( "OrCAD structure: unregistered structure type %d" ), typeId );

    if( *longCount == 0 )
        THROW_IO_ERROR( wxS( "OrCAD structure: long prefix count is zero" ) );

    size_t bound = aEnclosingEnd == ORCAD_STREAM::npos ? m_stream.Size() : aEnclosingEnd;

    if( bound > m_stream.Size() )
        THROW_IO_ERROR( wxS( "OrCAD structure: enclosing bound exceeds stream" ) );

    for( size_t i = 0; i < *longCount; ++i )
    {
        int prefixType = m_stream.ReadU8();

        if( prefixType != typeId )
            THROW_IO_ERRORF( wxS( "OrCAD structure: prefix type mismatch %d != %d" ), prefixType, typeId );

        pfx.bodyLens.push_back( m_stream.ReadU32() );

        if( m_stream.ReadU32() != 0 )
            THROW_IO_ERROR( wxS( "OrCAD structure: long prefix pad not zero" ) );
    }

    if( m_stream.ReadU8() != typeId )
        THROW_IO_ERROR( wxS( "OrCAD structure: short prefix type mismatch" ) );

    // Short prefix i16 pair count, then (u32 nameIdx, u32 valueIdx) pairs. Negative count = no pairs.
    int16_t propertyCount = m_stream.ReadI16();

    for( int i = 0; i < propertyCount; ++i )
    {
        uint32_t nameIdx = m_stream.ReadU32();
        uint32_t valueIdx = m_stream.ReadU32();
        pfx.props.emplace_back( nameIdx, valueIdx );
    }

    m_stream.ExpectPreamble( wxS( "structure preamble" ) );
    uint32_t trail = m_stream.ReadU32();
    m_stream.Skip( trail );
    pfx.bodyStart = m_stream.GetOffset();
    pfx.typeId = typeId;

    for( size_t i = 0; i < pfx.bodyLens.size(); ++i )
    {
        // Body length counts bytes after own 9-byte record; outermost bounds whole structure.
        size_t headerEnd = pfx.start + 9 * i + 9;

        if( pfx.bodyLens[i] > std::numeric_limits<size_t>::max() - headerEnd )
            THROW_IO_ERROR( wxS( "OrCAD structure: structure stop overflow" ) );

        size_t stop = headerEnd + pfx.bodyLens[i];

        if( stop < pfx.bodyStart || stop > bound )
            THROW_IO_ERRORF( wxS( "OrCAD structure: stop 0x%zx outside the enclosing bound" ), stop );

        pfx.stops.push_back( stop );
    }

    pfx.end = pfx.stops.front();

    return pfx;
}


std::string ORCAD_STRUCT_READER::Resolve( uint32_t aIndex ) const
{
    if( m_strings && aIndex < m_strings->size() )
        return ( *m_strings )[aIndex];

    return std::string();
}


std::map<std::string, std::string> ORCAD_STRUCT_READER::PropsDict( const ORCAD_PREFIXES& aPrefixes ) const
{
    std::map<std::string, std::string> out;

    for( const std::pair<uint32_t, uint32_t>& prop : aPrefixes.props )
    {
        std::string name = Resolve( prop.first );

        if( !name.empty() )
            out[name] = Resolve( prop.second );
    }

    return out;
}


void ORCAD_STRUCT_READER::SkipStructure( const ORCAD_PREFIXES& aPrefixes, const wxString& aWhat )
{
    if( aPrefixes.end != 0 && aPrefixes.end >= m_stream.GetOffset() )
    {
        m_stream.Seek( aPrefixes.end );
    }
    else
    {
        THROW_IO_ERRORF( wxS( "OrCAD structure: cannot skip structure %s" ), aWhat );
    }
}


ORCAD_READ_RESULT ORCAD_STRUCT_READER::ReadStructure()
{
    ORCAD_STREAM::NEST_GUARD guard( m_stream, wxS( "structure" ) );

    size_t         start = m_stream.GetOffset();
    ORCAD_PREFIXES pfx = ReadPrefixes();

    ORCAD_READ_RESULT result;
    result.typeId = pfx.typeId;

    try
    {
        ORCAD_STREAM::LIMIT_GUARD limit( m_stream, pfx.end );

        switch( pfx.typeId )
        {
        case ORCAD_ST_WIRE_SCALAR:
        case ORCAD_ST_WIRE_BUS: result.record = OrcadReadWire( *this, pfx ); break;

        case ORCAD_ST_ALIAS: result.record = OrcadReadAlias( *this, pfx ); break;

        case ORCAD_ST_SYMBOL_DISPLAY_PROP: result.record = OrcadReadDisplayProp( *this, pfx ); break;

        case ORCAD_ST_PLACED_INSTANCE: result.record = OrcadReadPlacedInstance( *this, pfx ); break;

        case ORCAD_ST_PORT: result.record = OrcadReadPort( *this, pfx ); break;

        case ORCAD_ST_GLOBAL:
        case ORCAD_ST_OFFPAGE_CONNECTOR:
        case ORCAD_ST_GRAPHIC_BOX_INST:
        case ORCAD_ST_GRAPHIC_LINE_INST:
        case ORCAD_ST_GRAPHIC_ARC_INST:
        case ORCAD_ST_GRAPHIC_ELLIPSE_INST:
        case ORCAD_ST_GRAPHIC_POLYGON_INST:
        case ORCAD_ST_GRAPHIC_POLYLINE_INST:
        case ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST:
        case ORCAD_ST_GRAPHIC_BITMAP_INST:
        case ORCAD_ST_GRAPHIC_BEZIER_INST:
        case ORCAD_ST_GRAPHIC_OLE_INST: result.record = OrcadReadGraphicInst( *this, pfx ); break;

        case ORCAD_ST_TITLEBLOCK: result.record = OrcadReadTitleBlock( *this, pfx ); break;

        case ORCAD_ST_ERC_OBJECT: result.record = OrcadReadErcObject( *this, pfx ); break;

        case ORCAD_ST_BUS_ENTRY: result.record = OrcadReadBusEntry( *this, pfx ); break;

        case ORCAD_ST_T0X10:
        case ORCAD_ST_T0X11: result.record = OrcadReadPinInst( *this, pfx ); break;

        case ORCAD_ST_STH_IN_PAGES0: result.record = OrcadReadSthInPages0( *this, pfx ); break;

        case ORCAD_ST_DRAWN_INSTANCE: result.record = OrcadReadDrawnInstance( *this, pfx ); break;

        default: SkipStructure( pfx, wxString::Format( wxS( "type %d" ), pfx.typeId ) ); break;
        }
    }
    catch( const IO_ERROR& e )
    {
        // Recover via prefix offsets on body-parse failure; one bad record must not abort page/cache parse.
        if( pfx.end != 0 && pfx.end > start )
        {
            Warn( wxString::Format( wxS( "OrCAD structure type %d at 0x%zx: %s; skipped" ), pfx.typeId, start,
                                    e.What() ) );
            m_stream.Seek( pfx.end );
            result.record = std::monostate();
            return result;
        }

        throw;
    }

    return result;
}


// -- per-type body readers ----------------------------------------------------------------


ORCAD_DISPLAY_PROP OrcadReadDisplayProp( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& /* aPrefixes */ )
{
    ORCAD_STREAM&      ds = aReader.Stream();
    ORCAD_DISPLAY_PROP prop;

    prop.nameIdx = ds.ReadU32();
    prop.name = aReader.Resolve( prop.nameIdx );
    prop.x = ds.ReadI16();
    prop.y = ds.ReadI16();

    // u16 bits 0..13 = 1-based font index, bits 14..15 = quarter turns.
    uint16_t rotFont = ds.ReadU16();
    prop.fontIdx = rotFont & 0x3FFF;
    prop.rotation = ( rotFont >> 14 ) & 0x3;

    prop.color = ds.ReadU8();
    prop.dispMode = ds.ReadU16();
    ds.ExpectByte( 0x00, wxS( "display prop tail" ) );

    return prop;
}


ORCAD_ALIAS OrcadReadAlias( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& /* aPrefixes */ )
{
    ORCAD_STREAM& ds = aReader.Stream();
    ORCAD_ALIAS   alias;

    alias.x = ds.ReadI32();
    alias.y = ds.ReadI32();
    alias.color = static_cast<int>( ds.ReadU32() );
    alias.rotation = static_cast<int>( ds.ReadU32() );
    alias.fontIdx = static_cast<int>( ds.ReadU32() );
    alias.name = ds.ReadLzt();

    return alias;
}


ORCAD_WIRE OrcadReadWire( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM& ds = aReader.Stream();
    ORCAD_WIRE    wire;

    wire.dbId = ds.ReadU32();
    wire.id = ds.ReadU32();
    wire.color = static_cast<int>( ds.ReadU32() );
    wire.x1 = ds.ReadI32();
    wire.y1 = ds.ReadI32();
    wire.x2 = ds.ReadI32();
    wire.y2 = ds.ReadI32();
    ds.Skip( 1 );

    uint16_t aliasCount = ds.ReadU16();

    for( int i = 0; i < aliasCount; i++ )
    {
        ORCAD_READ_RESULT r = aReader.ReadStructure();

        if( ORCAD_ALIAS* alias = std::get_if<ORCAD_ALIAS>( &r.record ) )
            wire.aliases.push_back( std::move( *alias ) );
    }

    uint16_t propCount = ds.ReadU16();

    for( int i = 0; i < propCount; i++ )
        aReader.ReadStructure();

    wire.lineWidth = static_cast<int>( ds.ReadU32() );
    wire.lineStyle = static_cast<int>( ds.ReadU32() );

    wire.isBus = ( aPrefixes.typeId == ORCAD_ST_WIRE_BUS );

    return wire;
}


ORCAD_PLACED_INSTANCE OrcadReadPlacedInstance( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM&         ds = aReader.Stream();
    ORCAD_PLACED_INSTANCE inst;

    ds.ReadU32();
    uint32_t headerWordB = ds.ReadU32();
    inst.pkgName = ds.ReadLzt();
    inst.sourceLibrary = aReader.Resolve( headerWordB );
    inst.dbId = ds.ReadU32();

    // Placed bbox stored y-first; includes displayed text.
    int by1 = ds.ReadI16();
    int bx1 = ds.ReadI16();
    int by2 = ds.ReadI16();
    int bx2 = ds.ReadI16();
    inst.bbox.x1 = bx1;
    inst.bbox.y1 = by1;
    inst.bbox.x2 = bx2;
    inst.bbox.y2 = by2;

    inst.x = ds.ReadI16();
    inst.y = ds.ReadI16();
    inst.color = ds.ReadU8();

    // Orientation byte bits 0..1 = quarter turns, bit 2 = mirror.
    uint8_t orientation = ds.ReadU8();
    inst.rotation = orientation & 0x3;
    inst.mirror = ( orientation & 0x4 ) != 0;

    inst.partIndex = ds.ReadU8();
    inst.partByte = ds.ReadU8();

    inst.displayProps = OrcadReadDisplayPropList( aReader );

    ds.Skip( 1 );
    inst.reference = ds.ReadLzt();

    // Part Value = u32 string-table index after reference; next 10 bytes unknown.
    uint32_t valueIdx = ds.ReadU32();
    inst.value = aReader.Resolve( valueIdx );
    ds.Skip( 10 );

    uint16_t pinCount = ds.ReadU16();

    for( int i = 0; i < pinCount; i++ )
    {
        ORCAD_READ_RESULT r = aReader.ReadStructure();

        if( ORCAD_PIN_INST* pin = std::get_if<ORCAD_PIN_INST>( &r.record ) )
            inst.pins.push_back( std::move( *pin ) );
    }

    inst.sourcePackage = ds.ReadLzt();
    inst.unitIndex = ds.ReadU16();

    inst.props = aReader.PropsDict( aPrefixes );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadGraphicInst( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM&      ds = aReader.Stream();
    ORCAD_GRAPHIC_INST inst;

    uint32_t nameIdx = ds.ReadU32(); // logical net/port name string index (ports)
    ds.ReadU32();                    // source library string index
    inst.name = ds.ReadLzt();
    inst.dbId = ds.ReadU32();

    // Anchor and bbox stored interleaved: y, x, y2, x2, x1, y1.
    inst.y = ds.ReadI16();
    inst.x = ds.ReadI16();
    int y2 = ds.ReadI16();
    int x2 = ds.ReadI16();
    int x1 = ds.ReadI16();
    int y1 = ds.ReadI16();
    inst.bbox.x1 = x1;
    inst.bbox.y1 = y1;
    inst.bbox.x2 = x2;
    inst.bbox.y2 = y2;

    inst.color = ds.ReadU8();

    // Orientation byte bits 0..1 = quarter turns, bit 2 = mirror.
    uint8_t orientation = ds.ReadU8();
    inst.rotation = orientation & 0x3;
    inst.mirror = ( orientation & 0x4 ) != 0;

    ds.Skip( 2 ); // structId, unknown

    inst.displayProps = OrcadReadDisplayPropList( aReader );

    // Flag 0x02 = one nested structure, usually SthInPages0 symbol body with drawable primitives.
    uint8_t flag = ds.ReadU8();

    if( flag == 0x02 )
    {
        ORCAD_READ_RESULT nested = aReader.ReadStructure();

        if( ORCAD_SYMBOL_DEF* def = std::get_if<ORCAD_SYMBOL_DEF>( &nested.record ) )
            inst.nested = std::make_unique<ORCAD_SYMBOL_DEF>( std::move( *def ) );
    }

    inst.typeId = aPrefixes.typeId;
    inst.props = aReader.PropsDict( aPrefixes );
    inst.logicalName = aReader.Resolve( nameIdx );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadPort( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_GRAPHIC_INST inst = OrcadReadGraphicInst( aReader, aPrefixes );
    aReader.Stream().Skip( 9 );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadTitleBlock( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_GRAPHIC_INST inst = OrcadReadGraphicInst( aReader, aPrefixes );
    aReader.Stream().Skip( 12 );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadErcObject( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_GRAPHIC_INST inst = OrcadReadGraphicInst( aReader, aPrefixes );

    aReader.Stream().ReadLzt();
    aReader.Stream().ReadLzt();
    aReader.Stream().ReadLzt();

    return inst;
}


ORCAD_PIN_INST OrcadReadPinInst( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM&  ds = aReader.Stream();
    ORCAD_PIN_INST pin;

    pin.pinIndex = ds.ReadI16();
    pin.x = ds.ReadI16();
    pin.y = ds.ReadI16();
    pin.wordA = ds.ReadU32();
    pin.wordB = ds.ReadU32();
    pin.displayProps = OrcadReadDisplayPropList( aReader );

    // Remaining body bytes padding; record ends at outer stop.
    if( aPrefixes.end != 0 && aPrefixes.end > ds.GetOffset() )
        ds.Seek( aPrefixes.end );

    return pin;
}


ORCAD_BUS_ENTRY OrcadReadBusEntryBody( ORCAD_STREAM& aStream )
{
    ORCAD_BUS_ENTRY entry;

    entry.color = static_cast<int>( aStream.ReadU32() );
    entry.x1 = aStream.ReadI32();
    entry.y1 = aStream.ReadI32();
    entry.x2 = aStream.ReadI32();
    entry.y2 = aStream.ReadI32();
    aStream.Skip( 8 );

    return entry;
}


ORCAD_BUS_ENTRY OrcadReadBusEntry( ORCAD_STRUCT_READER& aReader, const ORCAD_PREFIXES& /* aPrefixes */ )
{
    return OrcadReadBusEntryBody( aReader.Stream() );
}


std::vector<ORCAD_DISPLAY_PROP> OrcadReadDisplayPropList( ORCAD_STRUCT_READER& aReader )
{
    std::vector<ORCAD_DISPLAY_PROP> out;

    uint16_t count = aReader.Stream().ReadU16();

    for( int i = 0; i < count; i++ )
    {
        ORCAD_READ_RESULT r = aReader.ReadStructure();

        if( ORCAD_DISPLAY_PROP* prop = std::get_if<ORCAD_DISPLAY_PROP>( &r.record ) )
            out.push_back( std::move( *prop ) );
    }

    return out;
}


ORCAD_NET_GROUP OrcadReadT0x34Raw( ORCAD_STREAM& aStream )
{
    ORCAD_NET_GROUP net;

    aStream.Skip( 9 );
    net.id = aStream.ReadU32();
    net.name = aStream.ReadLzt();
    aStream.ReadU32();
    aStream.ReadU32(); // color
    aStream.ReadU32(); // line style
    aStream.ReadU32(); // line width

    return net;
}


ORCAD_NET_GROUP OrcadReadT0x35Raw( ORCAD_STREAM& aStream )
{
    ORCAD_NET_GROUP net = OrcadReadT0x34Raw( aStream );

    uint16_t count = aStream.ReadU16();

    for( uint16_t i = 0; i < count; ++i )
        net.members.push_back( aStream.ReadU32() );

    return net;
}
