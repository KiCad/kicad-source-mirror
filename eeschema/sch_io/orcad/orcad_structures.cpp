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

#include <memory>
#include <utility>
#include <variant>

#include <ki_exception.h>

#include <sch_io/orcad/orcad_cache.h>


ORCAD_STRUCT_READER::ORCAD_STRUCT_READER( ORCAD_STREAM& aStream,
                                          const std::vector<std::string>* aStrings,
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


ORCAD_PREFIXES ORCAD_STRUCT_READER::TryReadPrefixes( int aCount )
{
    ORCAD_PREFIXES pfx;
    pfx.start = m_stream.GetOffset();

    bool haveFirst = false;
    int  first = 0;

    for( int i = 0; i < aCount; i++ )
    {
        int typeId = m_stream.ReadU8();

        if( !haveFirst )
        {
            first = typeId;
            haveFirst = true;
        }
        else if( typeId != first )
        {
            THROW_IO_ERROR( wxString::Format( wxS( "OrCAD structure: prefix type mismatch "
                                                   "%d != %d" ),
                                              typeId, first ) );
        }

        if( i == aCount - 1 )
        {
            // Short prefix i16 pair count, then (u32 nameIdx, u32 valueIdx) pairs. Negative count = no pairs.
            int16_t count = m_stream.ReadI16();

            if( count >= 0 )
            {
                for( int k = 0; k < count; k++ )
                {
                    uint32_t nameIdx = m_stream.ReadU32();
                    uint32_t valueIdx = m_stream.ReadU32();
                    pfx.props.emplace_back( nameIdx, valueIdx );
                }
            }
        }
        else
        {
            // Long prefix u32 body length, then always-zero u32.
            uint32_t bodyLen = m_stream.ReadU32();
            uint32_t zeros = m_stream.ReadU32();

            if( zeros != 0 )
                THROW_IO_ERROR( wxS( "OrCAD structure: long prefix pad not zero" ) );

            pfx.bodyLens.push_back( bodyLen );
        }
    }

    // Preamble magic must follow prefix chain (not consumed here).
    if( !m_stream.AtPreamble() )
        THROW_IO_ERROR( wxS( "OrCAD structure: no preamble after prefixes" ) );

    pfx.typeId = first;
    return pfx;
}


ORCAD_PREFIXES ORCAD_STRUCT_READER::ReadPrefixes()
{
    size_t         saved = m_stream.GetOffset();
    wxString       lastErr;
    ORCAD_PREFIXES pfx;
    bool           found = false;

    // Long-prefix count type-dependent; try longest chain first so short chain never wins as prefix of longer.
    for( int n = 10; n >= 1 && !found; n-- )
    {
        m_stream.Seek( saved );

        try
        {
            pfx = TryReadPrefixes( n );
            found = true;
        }
        catch( const IO_ERROR& e )
        {
            lastErr = e.Problem();
        }
    }

    if( !found )
    {
        m_stream.Seek( saved );
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD structure: no valid prefix chain at "
                                               "0x%zx: %s" ),
                                          saved, lastErr ) );
    }

    m_stream.ExpectPreamble( wxS( "structure preamble" ) );
    uint32_t trail = m_stream.ReadU32();
    m_stream.Skip( trail );
    pfx.bodyStart = m_stream.GetOffset();

    if( !pfx.bodyLens.empty() )
    {
        // Body length counts bytes after own 9-byte record; outermost bounds whole structure.
        for( size_t i = 0; i < pfx.bodyLens.size(); i++ )
            pfx.stops.push_back( pfx.start + 9 * i + 9 + pfx.bodyLens[i] );

        pfx.end = pfx.stops[0];
    }

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
        THROW_IO_ERROR( wxString::Format( wxS( "OrCAD structure: cannot skip structure %s" ),
                                          aWhat ) );
    }
}


ORCAD_READ_RESULT ORCAD_STRUCT_READER::ReadStructure()
{
    size_t         start = m_stream.GetOffset();
    ORCAD_PREFIXES pfx = ReadPrefixes();

    ORCAD_READ_RESULT result;
    result.typeId = pfx.typeId;

    try
    {
        switch( pfx.typeId )
        {
        case ORCAD_ST_WIRE_SCALAR:
        case ORCAD_ST_WIRE_BUS:
            result.record = OrcadReadWire( *this, pfx );
            break;

        case ORCAD_ST_ALIAS:
            result.record = OrcadReadAlias( *this, pfx );
            break;

        case ORCAD_ST_SYMBOL_DISPLAY_PROP:
            result.record = OrcadReadDisplayProp( *this, pfx );
            break;

        case ORCAD_ST_PLACED_INSTANCE:
            result.record = OrcadReadPlacedInstance( *this, pfx );
            break;

        case ORCAD_ST_PORT:
            result.record = OrcadReadPort( *this, pfx );
            break;

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
        case ORCAD_ST_GRAPHIC_OLE_INST:
            result.record = OrcadReadGraphicInst( *this, pfx );
            break;

        case ORCAD_ST_TITLEBLOCK:
            result.record = OrcadReadTitleBlock( *this, pfx );
            break;

        case ORCAD_ST_ERC_OBJECT:
            result.record = OrcadReadErcObject( *this, pfx );
            break;

        case ORCAD_ST_BUS_ENTRY:
            result.record = OrcadReadBusEntry( *this, pfx );
            break;

        case ORCAD_ST_T0X10:
            result.record = OrcadReadPinInst( *this, pfx );
            break;

        case ORCAD_ST_STH_IN_PAGES0:
            result.record = OrcadReadSthInPages0( *this, pfx );
            break;

        case ORCAD_ST_DRAWN_INSTANCE:
            result.record = OrcadReadDrawnInstance( *this, pfx );
            break;

        default:
            SkipStructure( pfx, wxString::Format( wxS( "type %d" ), pfx.typeId ) );
            break;
        }
    }
    catch( const IO_ERROR& )
    {
        // Recover via prefix offsets on body-parse failure; one bad record must not abort page/cache parse.
        if( pfx.end != 0 && pfx.end > start )
        {
            Warn( wxString::Format( wxS( "OrCAD structure type %d at 0x%zx: body parse failed, "
                                         "skipped" ),
                                    pfx.typeId, start ) );
            m_stream.Seek( pfx.end );
            result.record = std::monostate();
            return result;
        }

        throw;
    }

    return result;
}


// -- per-type body readers ----------------------------------------------------------------


ORCAD_DISPLAY_PROP OrcadReadDisplayProp( ORCAD_STRUCT_READER& aReader,
                                         const ORCAD_PREFIXES& /* aPrefixes */ )
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

    ds.Skip( 4 );               // unknown
    wire.id = ds.ReadU32();
    ds.ReadU32();               // color
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

    ds.ReadU32();               // line width
    ds.ReadU32();               // line style

    wire.isBus = ( aPrefixes.typeId == ORCAD_ST_WIRE_BUS );

    return wire;
}


ORCAD_PLACED_INSTANCE OrcadReadPlacedInstance( ORCAD_STRUCT_READER& aReader,
                                               const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM&         ds = aReader.Stream();
    ORCAD_PLACED_INSTANCE inst;

    ds.Skip( 8 );               // unknown, zeros observed
    inst.pkgName = ds.ReadLzt();
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

    ds.Skip( 2 );               // structId, unknown

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
    ds.Skip( 2 );

    inst.props = aReader.PropsDict( aPrefixes );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadGraphicInst( ORCAD_STRUCT_READER& aReader,
                                         const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_STREAM&      ds = aReader.Stream();
    ORCAD_GRAPHIC_INST inst;

    uint32_t nameIdx = ds.ReadU32();    // logical net/port name string index (ports)
    ds.ReadU32();                       // source library string index
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

    ds.Skip( 2 );               // structId, unknown

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


ORCAD_GRAPHIC_INST OrcadReadTitleBlock( ORCAD_STRUCT_READER& aReader,
                                        const ORCAD_PREFIXES& aPrefixes )
{
    ORCAD_GRAPHIC_INST inst = OrcadReadGraphicInst( aReader, aPrefixes );
    aReader.Stream().Skip( 12 );

    return inst;
}


ORCAD_GRAPHIC_INST OrcadReadErcObject( ORCAD_STRUCT_READER& aReader,
                                       const ORCAD_PREFIXES& aPrefixes )
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

    ds.ReadU16();
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


ORCAD_BUS_ENTRY OrcadReadBusEntry( ORCAD_STRUCT_READER& aReader,
                                   const ORCAD_PREFIXES& /* aPrefixes */ )
{
    ORCAD_STREAM&   ds = aReader.Stream();
    ORCAD_BUS_ENTRY entry;

    entry.color = static_cast<int>( ds.ReadU32() );
    ds.Skip( 4 );               // unknown
    ds.Skip( 4 );               // unknown
    entry.x1 = ds.ReadI32();
    entry.y1 = ds.ReadI32();
    entry.x2 = ds.ReadI32();
    entry.y2 = ds.ReadI32();

    return entry;
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


void OrcadReadT0x34Raw( ORCAD_STREAM& aStream )
{
    aStream.Skip( 9 );
    aStream.ReadU32();          // id
    aStream.ReadLzt();
    aStream.ReadU32();
    aStream.ReadU32();          // color
    aStream.ReadU32();          // line style
    aStream.ReadU32();          // line width
}


void OrcadReadT0x35Raw( ORCAD_STREAM& aStream )
{
    OrcadReadT0x34Raw( aStream );

    uint16_t count = aStream.ReadU16();
    aStream.Skip( 4 * static_cast<size_t>( count ) );
}
