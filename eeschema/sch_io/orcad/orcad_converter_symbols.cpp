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
 * @file orcad_converter_symbols.cpp
 *
 * ORCAD_CONVERTER symbol half. Cache and in-memory LIB_SYMBOL space both Y-down;
 * .kicad_sch writer flips into file Y-up, so graphics/pins pass through unchanged.
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <wx/string.h>
#include <wx/translation.h>

#include <base_units.h>
#include <layer_ids.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <math/util.h>
#include <pin_type.h>
#include <sch_field.h>
#include <sch_no_connect.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <stroke_params.h>
#include <symbol.h>
#include <template_fieldnames.h>

#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_stream.h>


namespace
{

int schMm( double aMm )
{
    return schIUScale.mmToIU( aMm );
}


int dbuIu( double aDbu )
{
    return KiROUND( aDbu * ORCAD_IU_PER_DBU );
}


ELECTRICAL_PINTYPE pinTypeFor( ORCAD_PORT_TYPE aType )
{
    switch( aType )
    {
    case ORCAD_PORT_TYPE::INPUT:          return ELECTRICAL_PINTYPE::PT_INPUT;
    case ORCAD_PORT_TYPE::BIDIRECTIONAL:  return ELECTRICAL_PINTYPE::PT_BIDI;
    case ORCAD_PORT_TYPE::OUTPUT:         return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case ORCAD_PORT_TYPE::OPEN_COLLECTOR: return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case ORCAD_PORT_TYPE::PASSIVE:        return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case ORCAD_PORT_TYPE::TRI_STATE:      return ELECTRICAL_PINTYPE::PT_TRISTATE;
    case ORCAD_PORT_TYPE::OPEN_EMITTER:   return ELECTRICAL_PINTYPE::PT_OPENEMITTER;
    case ORCAD_PORT_TYPE::POWER_IN:       return ELECTRICAL_PINTYPE::PT_POWER_IN;
    }

    return ELECTRICAL_PINTYPE::PT_PASSIVE;
}


// OrCAD bookkeeping props kept out of user fields; useful ones map to dedicated
// KiCad fields, rest describe library linkage import re-establishes itself.
bool isBookkeepingProp( const std::string& aName )
{
    static const char* const skipped[] = {
        "Part Reference",
        "Reference",
        "Name",
        "Graphic",
        "Implementation",
        "Implementation Path",
        "Implementation Type",
        "Source Library",
        "Source Package",
        "Source Part",
    };

    for( const char* name : skipped )
    {
        if( aName == name )
            return true;
    }

    return false;
}

} // namespace


void ORCAD_CONVERTER::prepareSymbols()
{
    // Page symbols absent from design cache get synthesized placeholder so T0x10
    // connection points stay electrically intact.
    std::vector<std::string>                                          missingOrder;
    std::map<std::string, std::vector<const ORCAD_PLACED_INSTANCE*>>  missing;

    // Scan root and child-folder pages; child-folder parts live in childFolderPages,
    // not root page list.
    std::vector<const ORCAD_RAW_PAGE*> allPages;

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
        allPages.push_back( &page );

    for( const auto& [folder, pages] : m_design.childFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            allPages.push_back( &page );
    }

    for( const ORCAD_RAW_PAGE* page : allPages )
    {
        for( const ORCAD_PLACED_INSTANCE& inst : page->instances )
        {
            if( inst.pkgName.empty() || m_design.symbols.count( inst.pkgName ) )
                continue;

            auto [it, isNew] = missing.try_emplace( inst.pkgName );

            if( isNew )
                missingOrder.push_back( inst.pkgName );

            it->second.push_back( &inst );
        }
    }

    for( const std::string& name : missingOrder )
    {
        const std::vector<const ORCAD_PLACED_INSTANCE*>& insts = missing[name];

        note( wxString::Format( _( "Symbol '%s' is absent from the design cache; synthesized "
                                   "placeholder from %d instance(s)." ),
                                FromOrcadString( name ), (int) insts.size() ) );

        m_design.symbols[name] = synthesizeSymbol( name, insts );
    }

    for( const ORCAD_RAW_PAGE* page : allPages )
    {
        for( const ORCAD_PLACED_INSTANCE& inst : page->instances )
            libForInstance( inst );
    }

    computeFontBaseline();
}


ORCAD_SYMBOL_DEF
ORCAD_CONVERTER::synthesizeSymbol( const std::string& aPkgName,
                                   const std::vector<const ORCAD_PLACED_INSTANCE*>& aInstances ) const
{
    // Prefer reference-orientation instance: t-term in T = I + t(w,h) + M*p cancels
    // there, making reconstructed connection points exact.
    const ORCAD_PLACED_INSTANCE* ref = aInstances.front();

    for( const ORCAD_PLACED_INSTANCE* inst : aInstances )
    {
        if( OrcadOrientOf( inst->rotation, inst->mirror ) == 0 )
        {
            ref = inst;
            break;
        }
    }

    int                       ori = OrcadOrientOf( ref->rotation, ref->mirror );
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[ori];
    int                       det = e.a * e.d - e.b * e.c;    // always +/-1

    auto inv = [&]( int aX, int aY ) -> ORCAD_POINT
    {
        return { ( e.d * aX - e.b * aY ) / det, ( -e.c * aX + e.a * aY ) / det };
    };

    // Instance-local pin positions from T0x10 records.
    std::vector<ORCAD_POINT> r;

    for( const ORCAD_PIN_INST& t : ref->pins )
        r.push_back( inv( t.x - ref->x, t.y - ref->y ) );

    std::vector<char> sides;    // 'L', 'R', 'T', 'B' or 0 per pin
    int               bx1 = 0;
    int               by1 = 0;
    int               bx2 = 0;
    int               by2 = 0;

    if( !r.empty() )
    {
        int x0 = r.front().x;
        int x1 = r.front().x;
        int y0 = r.front().y;
        int y1 = r.front().y;

        for( const ORCAD_POINT& p : r )
        {
            x0 = std::min( x0, p.x );
            x1 = std::max( x1, p.x );
            y0 = std::min( y0, p.y );
            y1 = std::max( y1, p.y );
        }

        bool horizFirst = ( x1 - x0 ) >= ( y1 - y0 );

        for( const ORCAD_POINT& p : r )
        {
            std::vector<char> cand;

            if( p.x == x0 )
                cand.push_back( 'L' );

            if( p.x == x1 )
                cand.push_back( 'R' );

            if( p.y == y0 )
                cand.push_back( 'T' );

            if( p.y == y1 )
                cand.push_back( 'B' );

            if( cand.empty() )
            {
                sides.push_back( 0 );
                continue;
            }

            // Prefer L/R sides when pin field wider than tall, T/B otherwise; ties
            // keep L,R,T,B order.
            char pick = cand.front();

            for( char c : cand )
            {
                bool isTB = c == 'T' || c == 'B';

                if( isTB != horizFirst )
                {
                    pick = c;
                    break;
                }
            }

            sides.push_back( pick );
        }

        auto hasSide = [&]( char aSide )
        {
            return std::find( sides.begin(), sides.end(), aSide ) != sides.end();
        };

        bx1 = hasSide( 'L' ) ? x0 + PIN_LEN_DBU : x0;
        bx2 = hasSide( 'R' ) ? x1 - PIN_LEN_DBU : x1;
        by1 = hasSide( 'T' ) ? y0 + PIN_LEN_DBU : y0;
        by2 = hasSide( 'B' ) ? y1 - PIN_LEN_DBU : y1;

        if( bx2 <= bx1 )
        {
            int m = (int) std::floor( ( bx1 + bx2 ) / 2.0 );
            bx1 = m - PIN_LEN_DBU;
            bx2 = m + PIN_LEN_DBU;
        }

        if( by2 <= by1 )
        {
            int m = (int) std::floor( ( by1 + by2 ) / 2.0 );
            by1 = m - PIN_LEN_DBU;
            by2 = m + PIN_LEN_DBU;
        }
    }
    else
    {
        // No pin records; derive body from inverse-transformed placed box.
        ORCAD_POINT c1 = inv( ref->bbox.x1 - ref->x, ref->bbox.y1 - ref->y );
        ORCAD_POINT c2 = inv( ref->bbox.x2 - ref->x, ref->bbox.y2 - ref->y );

        bx1 = std::min( c1.x, c2.x );
        bx2 = std::max( c1.x, c2.x );
        by1 = std::min( c1.y, c2.y );
        by2 = std::max( c1.y, c2.y );
    }

    // Remove orientation re-anchoring offset so definition anchors like real cache
    // symbol.
    int         w = bx2 - bx1;
    int         h = by2 - by1;
    VECTOR2I    t = OrcadOrientOffset( ori, w, h );
    ORCAD_POINT s = inv( t.x, t.y );

    ORCAD_SYMBOL_DEF sym;
    sym.typeId = ORCAD_ST_LIBRARY_PART;
    sym.name = aPkgName;
    sym.synthesized = true;
    sym.bbox = ORCAD_BBOX{ bx1 - s.x, by1 - s.y, bx2 - s.x, by2 - s.y };

    ORCAD_PRIMITIVE rect;
    rect.kind = ORCAD_PRIM_KIND::RECT;
    rect.x1 = bx1 - s.x;
    rect.y1 = by1 - s.y;
    rect.x2 = bx2 - s.x;
    rect.y2 = by2 - s.y;
    sym.primitives.push_back( rect );

    for( size_t i = 0; i < r.size(); ++i )
    {
        int px = r[i].x - s.x;
        int py = r[i].y - s.y;
        int ddx = 0;
        int ddy = 0;

        switch( sides[i] )
        {
        case 'L': ddx = PIN_LEN_DBU;  break;
        case 'R': ddx = -PIN_LEN_DBU; break;
        case 'T': ddy = PIN_LEN_DBU;  break;
        case 'B': ddy = -PIN_LEN_DBU; break;
        default:                      break;
        }

        ORCAD_SYMBOL_PIN pin;
        pin.startX = px + ddx;
        pin.startY = py + ddy;
        pin.hotptX = px;
        pin.hotptY = py;
        pin.portType = ORCAD_PORT_TYPE::PASSIVE;
        pin.shapeBits = 0;
        sym.pins.push_back( pin );
    }

    return sym;
}


std::string ORCAD_CONVERTER::unitLetter( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    // Cache names look like "<base><unit>.<view>", e.g. "74LS00A.Normal" (unit A of
    // 74LS00) or "C.Normal" (single-unit capacitor).
    const std::string& base = aInst.sourcePackage;
    const std::string& v = aInst.pkgName;

    std::string tail = ( !base.empty() && v.compare( 0, base.size(), base ) == 0 )
                               ? v.substr( base.size() )
                               : v;

    size_t      dot = tail.find( '.' );
    std::string unit = tail.substr( 0, dot );
    std::string view = dot == std::string::npos ? std::string() : tail.substr( dot + 1 );

    if( !view.empty() )
    {
        std::string lower = view;

        std::transform( lower.begin(), lower.end(), lower.begin(),
                        []( unsigned char c )
                        {
                            return (char) std::tolower( c );
                        } );

        // Non-Normal views are DeMorgan alternates with own body graphics, so view
        // name stays part of unit discriminator.
        if( lower != "normal" )
            unit += ":" + view;
    }

    return unit;
}


std::pair<const ORCAD_SYMBOL_DEF*, int>
ORCAD_CONVERTER::pickVariant( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    auto it = m_design.symbols.find( aInst.pkgName );

    if( it == m_design.symbols.end() )
        return { nullptr, 0 };

    const ORCAD_SYMBOL_DEF* prime = &it->second;

    std::vector<const ORCAD_SYMBOL_DEF*> variants;
    variants.push_back( prime );

    for( const ORCAD_SYMBOL_DEF& v : prime->variants )
        variants.push_back( &v );

    if( aInst.pins.empty() || variants.size() == 1 )
        return { prime, 0 };

    int ori = OrcadOrientOf( aInst.rotation, aInst.mirror );

    for( size_t vi = 0; vi < variants.size(); ++vi )
    {
        const ORCAD_SYMBOL_DEF* sym = variants[vi];

        if( sym->pins.size() != aInst.pins.size() )
            continue;

        ORCAD_BBOX bb = sym->bbox.value_or( ORCAD_BBOX() );
        bool       match = true;

        for( size_t i = 0; i < sym->pins.size(); ++i )
        {
            VECTOR2I calc = OrcadTransformPoint( ori, bb.x2 - bb.x1, bb.y2 - bb.y1, aInst.x,
                                                 aInst.y, sym->pins[i].hotptX,
                                                 sym->pins[i].hotptY );

            if( calc.x != aInst.pins[i].x || calc.y != aInst.pins[i].y )
            {
                match = false;
                break;
            }
        }

        if( match )
            return { sym, (int) vi };
    }

    return { prime, 0 };
}


std::pair<std::string, int> ORCAD_CONVERTER::libForInstance( const ORCAD_PLACED_INSTANCE& aInst )
{
    auto [sym, vi] = pickVariant( aInst );

    std::string srcOrPkg = !aInst.sourcePackage.empty() ? aInst.sourcePackage : aInst.pkgName;
    PKG_KEY     key{ srcOrPkg, aInst.pkgName, vi };

    auto found = m_pkgToLib.find( key );

    if( found != m_pkgToLib.end() )
        return found->second;

    std::string base = !aInst.sourcePackage.empty()
                               ? aInst.sourcePackage
                               : aInst.pkgName.substr( 0, aInst.pkgName.find( '.' ) );

    std::string letter = unitLetter( aInst );

    if( !sym )
    {
        warn( wxString::Format( _( "No cached symbol for '%s' (%s); placeholder emitted." ),
                                FromOrcadString( aInst.pkgName ),
                                FromOrcadString( aInst.reference ) ) );

        // Register empty definition so unit references stable storage.
        ORCAD_SYMBOL_DEF& slot = m_design.symbols[aInst.pkgName];

        if( slot.name.empty() )
        {
            slot.typeId = 0;
            slot.name = aInst.pkgName;
        }

        sym = &slot;
    }

    const ORCAD_PACKAGE* pkg = nullptr;
    auto                 pkgIt = m_design.packages.find( base );

    if( pkgIt != m_design.packages.end() )
        pkg = &pkgIt->second;

    // Pin numbers from package device whose unitRef equals BARE unit letter; ':View'
    // suffix only selects DeMorgan graphics, views share Normal pin map.
    std::vector<std::string> pinNumbers;
    std::string              bare = letter.substr( 0, letter.find( ':' ) );

    if( pkg )
    {
        bool foundDevice = false;

        for( const ORCAD_DEVICE& d : pkg->devices )
        {
            if( d.unitRef == bare )
            {
                pinNumbers = d.pinNumbers;
                foundDevice = true;
                break;
            }
        }

        if( !foundDevice && !pkg->devices.empty() && bare.empty() )
            pinNumbers = pkg->devices.front().pinNumbers;
    }

    std::string libname = SymbolId( base );

    if( vi )
        libname += "_v" + std::to_string( vi + 1 );

    LIB_ENTRY& ls = m_libSymbols[libname];

    if( ls.name.empty() )
    {
        ls.name = libname;
        ls.refPrefix = ( pkg && !pkg->refDes.empty() ) ? pkg->refDes : "U";
        ls.footprint = pkg ? pkg->pcbFootprint : "";
    }

    int unitNo = 0;

    for( size_t i = 0; i < ls.units.size(); ++i )
    {
        if( ls.units[i].letter == letter )
        {
            unitNo = (int) i + 1;
            break;
        }
    }

    if( unitNo == 0 )
    {
        UNIT_INFO unit;
        unit.letter = letter;
        unit.symbol = sym;
        unit.pinNumbers = pinNumbers;
        ls.units.push_back( std::move( unit ) );

        std::stable_sort( ls.units.begin(), ls.units.end(),
                          []( const UNIT_INFO& a, const UNIT_INFO& b )
                          {
                              return a.letter < b.letter;
                          } );

        for( size_t i = 0; i < ls.units.size(); ++i )
        {
            if( ls.units[i].letter == letter )
            {
                unitNo = (int) i + 1;
                break;
            }
        }

        // Inserting a unit renumbers later-sorting letters, so every map entry for
        // this lib symbol is stale; rebuild them.
        for( auto mapIt = m_pkgToLib.begin(); mapIt != m_pkgToLib.end(); )
        {
            if( mapIt->second.first == libname )
                mapIt = m_pkgToLib.erase( mapIt );
            else
                ++mapIt;
        }

        for( size_t i = 0; i < ls.units.size(); ++i )
        {
            std::string pkgName = !aInst.sourcePackage.empty()
                                          ? aInst.sourcePackage + ls.units[i].letter + ".Normal"
                                          : aInst.pkgName;

            m_pkgToLib[PKG_KEY{ srcOrPkg, pkgName, vi }] = { libname, (int) i + 1 };
        }
    }

    m_pkgToLib[key] = { libname, unitNo };
    return { libname, unitNo };
}


std::string ORCAD_CONVERTER::powerLibFor( const std::string& aSymbolName,
                                          const std::string& aNetName )
{
    // Keyed by net name, not symbol name; users rename power ports and one symbol
    // shape can serve several nets.
    std::string libname = "PWR_" + SymbolId( aNetName );

    if( m_libSymbols.count( libname ) )
        return libname;

    const ORCAD_SYMBOL_DEF* sym = nullptr;
    auto                    it = m_design.symbols.find( aSymbolName );

    if( it != m_design.symbols.end() )
    {
        sym = &it->second;
    }
    else
    {
        // Uncached power symbol; register empty definition (no graphics).
        ORCAD_SYMBOL_DEF& slot = m_design.symbols[aSymbolName];

        if( slot.name.empty() )
        {
            slot.typeId = ORCAD_ST_GLOBAL_SYMBOL;
            slot.name = aSymbolName;
        }

        sym = &slot;
    }

    LIB_ENTRY& ls = m_libSymbols[libname];
    ls.name = libname;
    ls.isPower = true;
    ls.powerNet = aNetName;
    ls.refPrefix = "#PWR";

    UNIT_INFO unit;
    unit.symbol = sym;
    unit.pinNumbers = { "1" };
    ls.units.push_back( std::move( unit ) );

    return libname;
}


LIB_SYMBOL* ORCAD_CONVERTER::kicadSymbolFor( const std::string& aLibName )
{
    auto it = m_libSymbols.find( aLibName );

    if( it == m_libSymbols.end() )
        return nullptr;

    LIB_ENTRY& entry = it->second;

    if( entry.kicadSymbol )
        return entry.kicadSymbol.get();

    // OrCAD cache names can contain LIB_ID-illegal chars (e.g. backslashes from
    // library path prefixes).
    wxString name = LIB_ID::FixIllegalChars( FromOrcadString( entry.name ), false ).wx_str();

    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( name );
    symbol->SetLibId( LIB_ID( wxString::FromUTF8( LIB_NICK ), name ) );
    symbol->SetPinNameOffset( schMm( 0.254 ) );

    // Pin number/name visibility from LibraryPart flags; bit0=numbers visible,
    // bit2=names hidden.
    for( const UNIT_INFO& unit : entry.units )
    {
        if( unit.symbol && unit.symbol->generalFlags >= 0 )
        {
            int flags = unit.symbol->generalFlags;
            symbol->SetShowPinNumbers( ( flags & 0x01 ) != 0 );
            symbol->SetShowPinNames( ( flags & 0x04 ) == 0 );
            break;
        }
    }

    if( entry.isPower )
        symbol->SetGlobalPower();

    if( (int) entry.units.size() > 1 )
        symbol->SetUnitCount( (int) entry.units.size(), false );

    const ORCAD_SYMBOL_DEF* u0 = entry.units.empty() ? nullptr : entry.units.front().symbol;
    ORCAD_BBOX              bb = ( u0 && u0->bbox ) ? *u0->bbox : ORCAD_BBOX();

    // Default field spot above body; in-memory symbol space Y-down, so above = more
    // negative Y.
    int topY = bb.y1 * ORCAD_IU_PER_DBU - schMm( 2.54 );

    SCH_FIELD& refField = symbol->GetReferenceField();
    SCH_FIELD& valField = symbol->GetValueField();

    if( entry.isPower )
    {
        refField.SetText( wxS( "#PWR" ) );
        refField.SetVisible( false );
        refField.SetTextPos( VECTOR2I( 0, topY ) );

        valField.SetText( FromOrcadString( entry.powerNet ) );
        valField.SetVisible( true );
        valField.SetTextPos( VECTOR2I( 0, topY ) );
    }
    else
    {
        refField.SetText( FromOrcadString( entry.refPrefix ) );
        refField.SetVisible( true );
        refField.SetTextPos( VECTOR2I( 0, topY ) );

        valField.SetText( name );
        valField.SetVisible( true );
        valField.SetTextPos( VECTOR2I( 0, topY - schMm( 2.54 ) ) );
    }

    symbol->GetFootprintField().SetText( FromOrcadString( entry.footprint ) );
    symbol->GetFootprintField().SetVisible( false );
    symbol->GetDatasheetField().SetVisible( false );

    for( size_t ui = 0; ui < entry.units.size(); ++ui )
    {
        const UNIT_INFO& unit = entry.units[ui];
        int              unitNo = (int) ui + 1;

        if( !unit.symbol )
            continue;

        for( const ORCAD_PRIMITIVE& prim : unit.symbol->primitives )
            addSymbolPrimitive( symbol.get(), prim, unitNo );

        for( size_t pi = 0; pi < unit.symbol->pins.size(); ++pi )
        {
            wxString number = pi < unit.pinNumbers.size()
                                      ? FromOrcadString( unit.pinNumbers[pi] )
                                      : wxString::Format( wxS( "%d" ), (int) pi + 1 );

            addSymbolPin( symbol.get(), unit.symbol->pins[pi], number, unitNo, entry.isPower,
                          entry.isPower ? entry.powerNet : std::string() );
        }
    }

    entry.kicadSymbol = std::move( symbol );
    return entry.kicadSymbol.get();
}


void ORCAD_CONVERTER::addSymbolPrimitive( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim,
                                          int aUnit )
{
    // Cache defs and in-memory symbol space both Y-down; .kicad_sch writer flips to
    // file Y-up itself.
    auto toX = []( int aV )
    {
        return aV * ORCAD_IU_PER_DBU;
    };
    auto toY = []( int aV )
    {
        return aV * ORCAD_IU_PER_DBU;
    };

    switch( aPrim.kind )
    {
    case ORCAD_PRIM_KIND::RECT:
    {
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        shape->SetPosition( VECTOR2I( toX( aPrim.x1 ), toY( aPrim.y1 ) ) );
        shape->SetEnd( VECTOR2I( toX( aPrim.x2 ), toY( aPrim.y2 ) ) );
        shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
        shape->SetFillMode( aPrim.fill ? FILL_T::FILLED_WITH_BG_BODYCOLOR : FILL_T::NO_FILL );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );
        break;
    }

    case ORCAD_PRIM_KIND::LINE:
    {
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        shape->AddPoint( VECTOR2I( toX( aPrim.x1 ), toY( aPrim.y1 ) ) );
        shape->AddPoint( VECTOR2I( toX( aPrim.x2 ), toY( aPrim.y2 ) ) );
        shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );
        break;
    }

    case ORCAD_PRIM_KIND::POLYLINE:
    case ORCAD_PRIM_KIND::POLYGON:
    case ORCAD_PRIM_KIND::BEZIER:
    {
        if( aPrim.points.size() < 2 )
            return;

        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        for( const ORCAD_POINT& pt : aPrim.points )
            shape->AddPoint( VECTOR2I( toX( pt.x ), toY( pt.y ) ) );

        bool isPolygon = aPrim.kind == ORCAD_PRIM_KIND::POLYGON;

        if( isPolygon && aPrim.points.front() != aPrim.points.back() )
            shape->AddPoint( VECTOR2I( toX( aPrim.points.front().x ),
                                       toY( aPrim.points.front().y ) ) );

        shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
        shape->SetFillMode( isPolygon && aPrim.fill ? FILL_T::FILLED_WITH_BG_BODYCOLOR
                                                    : FILL_T::NO_FILL );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape, false );
        break;
    }

    case ORCAD_PRIM_KIND::ELLIPSE:
    {
        double cx = ( aPrim.x1 + aPrim.x2 ) / 2.0;
        double cy = ( aPrim.y1 + aPrim.y2 ) / 2.0;
        double rx = std::abs( aPrim.x2 - aPrim.x1 ) / 2.0;
        double ry = std::abs( aPrim.y2 - aPrim.y1 ) / 2.0;

        if( std::abs( rx - ry ) < 0.5 )
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
            VECTOR2I   center( dbuIu( cx ), dbuIu( cy ) );
            shape->SetPosition( center );
            shape->SetEnd( center + VECTOR2I( dbuIu( rx ), 0 ) );
            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
            shape->SetFillMode( aPrim.fill ? FILL_T::FILLED_WITH_BG_BODYCOLOR
                                           : FILL_T::NO_FILL );
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape, false );
        }
        else
        {
            // Unequal radii; approximate ellipse with closed polyline.
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            for( int k = 0; k <= 32; ++k )
            {
                double a = 2.0 * M_PI * k / 32.0;
                shape->AddPoint( VECTOR2I( dbuIu( cx + rx * std::cos( a ) ),
                                           dbuIu( cy + ry * std::sin( a ) ) ) );
            }

            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
            shape->SetFillMode( aPrim.fill ? FILL_T::FILLED_WITH_BG_BODYCOLOR
                                           : FILL_T::NO_FILL );
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape, false );
        }

        break;
    }

    case ORCAD_PRIM_KIND::ARC:
        addSymbolArc( aSymbol, aPrim, aUnit );
        break;

    case ORCAD_PRIM_KIND::TEXT:
    {
        SCH_TEXT* text = new SCH_TEXT( VECTOR2I( toX( aPrim.x1 ), toY( aPrim.y1 ) ),
                                       FromOrcadString( aPrim.text ), LAYER_DEVICE );
        int       size = textSizeIU( aPrim.fontIdx );
        text->SetTextSize( VECTOR2I( size, size ) );

        // OrCAD anchors comment text at top-left of bounding box.
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        text->SetUnit( aUnit );
        aSymbol->AddDrawItem( text, false );
        break;
    }

    case ORCAD_PRIM_KIND::IMAGE:
        // Embedded images only in page graphics, not symbol bodies.
        break;
    }
}


void ORCAD_CONVERTER::addSymbolArc( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit )
{
    if( !aPrim.start || !aPrim.end )
        return;

    double cx = ( aPrim.x1 + aPrim.x2 ) / 2.0;
    double cy = ( aPrim.y1 + aPrim.y2 ) / 2.0;
    double rx = std::abs( aPrim.x2 - aPrim.x1 ) / 2.0;
    double ry = std::abs( aPrim.y2 - aPrim.y1 ) / 2.0;

    if( rx == 0.0 )
        rx = 0.01;

    if( ry == 0.0 )
        ry = 0.01;

    double a0 = std::atan2( ( aPrim.start->y - cy ) / ry, ( aPrim.start->x - cx ) / rx );
    double a1 = std::atan2( ( aPrim.end->y - cy ) / ry, ( aPrim.end->x - cx ) / rx );

    // OrCAD draws arcs CCW in screen (Y-down) coords, meaning decreasing parameter
    // angle here.
    if( a1 >= a0 )
        a1 -= 2.0 * M_PI;

    int steps = std::max( 4, (int) ( std::abs( a1 - a0 ) / ( M_PI / 16.0 ) ) );

    // Polyline approximation; robust for elliptical arcs KiCad arcs cannot represent.
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    for( int k = 0; k <= steps; ++k )
    {
        double a = a0 + ( a1 - a0 ) * k / steps;
        shape->AddPoint( VECTOR2I( dbuIu( cx + rx * std::cos( a ) ),
                                   dbuIu( cy + ry * std::sin( a ) ) ) );
    }

    shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
    shape->SetFillMode( FILL_T::NO_FILL );
    shape->SetUnit( aUnit );
    aSymbol->AddDrawItem( shape, false );
}


void ORCAD_CONVERTER::addSymbolPin( LIB_SYMBOL* aSymbol, const ORCAD_SYMBOL_PIN& aPin,
                                    const wxString& aNumber, int aUnit, bool aPower,
                                    const std::string& aNameOverride )
{
    int dx = aPin.startX - aPin.hotptX;
    int dy = aPin.startY - aPin.hotptY;

    SCH_PIN* pin = new SCH_PIN( aSymbol );

    // Hot point = connection point; start point = body end.
    pin->SetPosition( VECTOR2I( aPin.hotptX * ORCAD_IU_PER_DBU,
                                aPin.hotptY * ORCAD_IU_PER_DBU ) );
    pin->SetLength( KiROUND( std::hypot( (double) dx, (double) dy ) ) * ORCAD_IU_PER_DBU );

    PIN_ORIENTATION orientation = PIN_ORIENTATION::PIN_RIGHT;

    if( dx > 0 )
        orientation = PIN_ORIENTATION::PIN_RIGHT;
    else if( dx < 0 )
        orientation = PIN_ORIENTATION::PIN_LEFT;
    else if( dy > 0 )
        orientation = PIN_ORIENTATION::PIN_DOWN;   // start below hot point (Y-down source)
    else if( dy < 0 )
        orientation = PIN_ORIENTATION::PIN_UP;

    pin->SetOrientation( orientation );

    // shapeBits bit1 = clock, bit2 = inverted dot.
    bool isClock = ( aPin.shapeBits & 0x2 ) != 0;
    bool isDot = ( aPin.shapeBits & 0x4 ) != 0;

    if( isClock && isDot )
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
    else if( isClock )
        pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
    else if( isDot )
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
    else
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );

    if( aPower )
    {
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
    }
    else
    {
        pin->SetType( pinTypeFor( aPin.portType ) );
    }

    // "$PIN"-prefixed names are OrCAD auto-generated placeholders.
    wxString name;

    if( !aNameOverride.empty() )
        name = FromOrcadString( aNameOverride );
    else if( !aPin.name.empty() && aPin.name.compare( 0, 4, "$PIN" ) != 0 )
        name = FromOrcadString( aPin.name );

    // Avoid rendering same text twice next to pin.
    if( name == aNumber )
        name = wxEmptyString;

    pin->SetName( name );
    pin->SetNumber( aNumber );
    pin->SetNameTextSize( schMm( 1.27 ) );
    pin->SetNumberTextSize( schMm( 1.27 ) );
    pin->SetUnit( aUnit );
    aSymbol->AddDrawItem( pin, false );
}


int ORCAD_CONVERTER::toKicadOrientation( int aOrient )
{
    switch( aOrient & 7 )
    {
    default:
    case 0: return SYM_ORIENT_0;
    case 1: return SYM_ORIENT_90;
    case 2: return SYM_ORIENT_180;
    case 3: return SYM_ORIENT_270;
    case 4: return SYM_ORIENT_0 + SYM_MIRROR_Y;
    case 5: return SYM_ORIENT_90 + SYM_MIRROR_X;
    case 6: return SYM_ORIENT_0 + SYM_MIRROR_X;
    case 7: return SYM_ORIENT_270 + SYM_MIRROR_X;
    }
}


void ORCAD_CONVERTER::placeInstance( ORCAD_RAW_PAGE& aPage, const ORCAD_PLACED_INSTANCE& aInst,
                                     SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath )
{
    auto [libname, unit] = libForInstance( aInst );

    LIB_ENTRY&              entry = m_libSymbols.at( libname );
    const UNIT_INFO&        uinfo = entry.units[unit - 1];
    const ORCAD_SYMBOL_DEF& def = *uinfo.symbol;

    ORCAD_BBOX bb = def.bbox.value_or( ORCAD_BBOX() );
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;
    int        ori = OrcadOrientOf( aInst.rotation, aInst.mirror );
    VECTOR2I   offset = OrcadOrientOffset( ori, w, h );
    VECTOR2I   pos = OrcadDbuToIu( aInst.x + offset.x, aInst.y + offset.y );

    // Transformed def hot points must land exactly on T0x10 absolute pin positions;
    // mismatch means wrong variant/orientation data.
    if( !aInst.pins.empty() && aInst.pins.size() == def.pins.size() )
    {
        bool match = true;

        for( size_t i = 0; i < def.pins.size(); ++i )
        {
            VECTOR2I calc = OrcadTransformPoint( ori, w, h, aInst.x, aInst.y, def.pins[i].hotptX,
                                                 def.pins[i].hotptY );

            if( calc.x != aInst.pins[i].x || calc.y != aInst.pins[i].y )
            {
                match = false;
                break;
            }
        }

        if( !match )
        {
            if( def.synthesized )
            {
                note( wxString::Format( _( "Page %s: %s uses a synthesized placeholder at a "
                                           "different orientation; pin layout approximate." ),
                                        FromOrcadString( aPage.name ),
                                        FromOrcadString( aInst.reference ) ) );
            }
            else
            {
                warn( wxString::Format( _( "Page %s: %s pin positions mismatch (orientation %d); "
                                           "geometry may be off." ),
                                        FromOrcadString( aPage.name ),
                                        FromOrcadString( aInst.reference ), ori ) );
            }
        }
    }

    LIB_SYMBOL* libSymbol = kicadSymbolFor( libname );

    if( !libSymbol )
        return;

    LIB_ID      libId( wxString::FromUTF8( LIB_NICK ),
                       LIB_ID::FixIllegalChars( FromOrcadString( libname ), false ).wx_str() );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aSheetPath, unit, 0, pos );

    symbol->SetOrientation( toKicadOrientation( ori ) );

    std::string value = aInst.value;

    if( value.empty() )
    {
        auto vIt = aInst.props.find( "Value" );

        if( vIt != aInst.props.end() )
            value = vIt->second;
    }

    if( value.empty() )
        value = entry.name;

    std::string footprint;
    auto        fIt = aInst.props.find( "PCB Footprint" );

    if( fIt != aInst.props.end() )
        footprint = fIt->second;

    if( footprint.empty() )
        footprint = entry.footprint;

    placeSymbolFields( symbol, aInst, def, ori, value, footprint );

    wxString reference = resolveReference( aInst );

    symbol->SetRef( &aSheetPath, reference );
    symbol->SetUnitSelection( &aSheetPath, unit );

    aScreen->Append( symbol );

    // Placed pin w/ zero net word is unconnected; OrCAD draws no-connect marker,
    // reproduce it.
    for( const ORCAD_PIN_INST& pin : aInst.pins )
    {
        if( pin.wordA == 0 )
            aScreen->Append( new SCH_NO_CONNECT( OrcadDbuToIu( pin.x, pin.y ) ) );
    }
}


wxString ORCAD_CONVERTER::resolveReference( const ORCAD_PLACED_INSTANCE& aInst ) const
{
    wxString reference = FromOrcadString( aInst.reference );

    // Hierarchy stream carries authoritative per-occurrence designator; overrides
    // stale placed "C?" template since OrCAD re-annotates without rewriting record.
    if( m_currentOccRefs )
    {
        auto it = m_currentOccRefs->find( aInst.dbId );

        if( it != m_currentOccRefs->end() )
        {
            wxString occurrence = FromOrcadString( it->second );

            if( !occurrence.IsEmpty() && !occurrence.EndsWith( wxS( "?" ) ) )
                reference = occurrence;
        }
    }

    if( reference.IsEmpty() )
        reference = wxS( "?" );

    return reference;
}


void ORCAD_CONVERTER::placePowerSymbol( ORCAD_RAW_PAGE&, const ORCAD_GRAPHIC_INST& aInst,
                                        const std::string& aNet, SCH_SCREEN* aScreen,
                                        const SCH_SHEET_PATH& aSheetPath )
{
    std::string libname = powerLibFor( aInst.name, aNet );

    LIB_ENTRY&              entry = m_libSymbols.at( libname );
    const ORCAD_SYMBOL_DEF* def = entry.units.front().symbol;

    ORCAD_BBOX bb = ( def && def->bbox ) ? *def->bbox : ORCAD_BBOX{ 0, 0, 20, 10 };
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;
    int        ori = OrcadOrientOf( aInst.rotation, aInst.mirror );
    VECTOR2I   offset = OrcadOrientOffset( ori, w, h );

    // Orientation transform base = placed bbox min corner, NOT anchor point.
    bool hasBbox = aInst.bbox.x1 || aInst.bbox.y1 || aInst.bbox.x2 || aInst.bbox.y2;
    int  bx = hasBbox ? std::min( aInst.bbox.x1, aInst.bbox.x2 ) : aInst.x;
    int  by = hasBbox ? std::min( aInst.bbox.y1, aInst.bbox.y2 ) : aInst.y;

    VECTOR2I pos = OrcadDbuToIu( bx + offset.x, by + offset.y );

    m_powerCount++;
    wxString reference = wxString::Format( wxS( "#PWR%04d" ), m_powerCount );

    LIB_SYMBOL* libSymbol = kicadSymbolFor( libname );

    if( !libSymbol )
        return;

    LIB_ID      libId( wxString::FromUTF8( LIB_NICK ),
                       LIB_ID::FixIllegalChars( FromOrcadString( libname ), false ).wx_str() );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aSheetPath, 1, 0, pos );

    symbol->SetOrientation( toKicadOrientation( ori ) );

    SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
    refField->SetPosition( pos + VECTOR2I( 0, schMm( 2.54 ) ) );
    refField->SetTextAngle( ANGLE_HORIZONTAL );
    refField->SetVisible( false );

    // Net name against body; below ground-style symbol (pin in top quarter of box),
    // above supply one.
    int bymin = std::min( aInst.bbox.y1, aInst.bbox.y2 );
    int bymax = std::max( aInst.bbox.y1, aInst.bbox.y2 );

    VECTOR2I pinPos = graphicPinPos( aInst );

    symbol->SetValueFieldText( FromOrcadString( aNet ) );

    SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );

    // Net name snug against symbol body; OrCAD placed bbox padded w/ text, so anchor
    // near text edge for edge-to-edge gap.
    BOX2I body = symbol->GetBodyBoundingBox();

    if( pinPos.y <= bymin + ( bymax - bymin ) / 4.0 )
    {
        valField->SetPosition( VECTOR2I( body.Centre().x, body.GetBottom() + schMm( 0.3 ) ) );
        valField->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    }
    else
    {
        valField->SetPosition( VECTOR2I( body.Centre().x, body.GetTop() - schMm( 0.3 ) ) );
        valField->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }

    // Net name reads horizontally; compensate for KiCad re-rotating field w/ flipped
    // (90/270) power symbol.
    valField->SetTextAngle( symbol->GetTransform().y1 ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
    valField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    valField->SetVisible( true );

    SCH_FIELD* fpField = symbol->GetField( FIELD_T::FOOTPRINT );
    fpField->SetPosition( pos );
    fpField->SetVisible( false );

    SCH_FIELD* dsField = symbol->GetField( FIELD_T::DATASHEET );
    dsField->SetPosition( pos );
    dsField->SetVisible( false );

    symbol->SetRef( &aSheetPath, reference );
    symbol->SetUnitSelection( &aSheetPath, 1 );

    aScreen->Append( symbol );
}


void ORCAD_CONVERTER::placeSymbolFields( SCH_SYMBOL* aSymbol, const ORCAD_PLACED_INSTANCE& aInst,
                                         const ORCAD_SYMBOL_DEF& aDef, int aOrient,
                                         const std::string& aValue,
                                         const std::string& aFootprint )
{
    // Placed bbox includes OrCAD displayed text; true body box = orientation
    // transform of cache def bbox.
    ORCAD_BBOX bb = aDef.bbox.value_or( ORCAD_BBOX() );
    int        w = bb.x2 - bb.x1;
    int        h = bb.y2 - bb.y1;

    VECTOR2I c1 = OrcadTransformPoint( aOrient, w, h, aInst.x, aInst.y, bb.x1, bb.y1 );
    VECTOR2I c2 = OrcadTransformPoint( aOrient, w, h, aInst.x, aInst.y, bb.x2, bb.y2 );

    int bxmin = std::min( c1.x, c2.x );
    int bxmax = std::max( c1.x, c2.x );
    int bymin = std::min( c1.y, c2.y );
    int bymax = std::max( c1.y, c2.y );

    int cxm = ( bxmin + bxmax ) * ORCAD_IU_PER_DBU / 2;
    int cym = ( bymin + bymax ) * ORCAD_IU_PER_DBU / 2;

    // Fields always read horizontally; KiCad rotates field text w/ symbol, so
    // 90/270 placement gets compensating field angle.
    int       angle = ORCAD_ORIENT_TABLE[aOrient & 7].angle;
    bool      vertical = angle == 90 || angle == 270;
    EDA_ANGLE fieldAngle = vertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL;

    // Visibility follows source display mode; bit 0x100 is field visible flag.
    bool showRef = false;
    bool showVal = false;

    for( const ORCAD_DISPLAY_PROP& dp : aInst.displayProps )
    {
        bool visible = ( dp.dispMode & 0x100 ) != 0;

        if( dp.name == "Part Reference" )
            showRef = visible;
        else if( dp.name == "Value" )
            showVal = visible;
    }

    aSymbol->SetValueFieldText( FromOrcadString( aValue ) );

    SCH_FIELD* refField = aSymbol->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* valField = aSymbol->GetField( FIELD_T::VALUE );

    // Vertical two-pin passives stack Reference/Value right of body, centered;
    // everything else gets Reference above, Value below.
    bool side = !vertical && aDef.pins.size() == 2 && aInst.pins.size() == 2
                && std::abs( aInst.pins[0].y - aInst.pins[1].y )
                           >= std::abs( aInst.pins[0].x - aInst.pins[1].x );

    if( side )
    {
        int sx = bxmax * ORCAD_IU_PER_DBU + schMm( 1.0 );
        int ry = showVal ? cym - schMm( 0.9 ) : cym;

        refField->SetPosition( VECTOR2I( sx, ry ) );
        refField->SetTextAngle( ANGLE_HORIZONTAL );
        refField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        refField->SetVisible( showRef );

        valField->SetPosition( VECTOR2I( sx, cym + schMm( 0.9 ) ) );
        valField->SetTextAngle( ANGLE_HORIZONTAL );
        valField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        valField->SetVisible( showVal );
    }
    else
    {
        refField->SetPosition( VECTOR2I( cxm, bymin * ORCAD_IU_PER_DBU - schMm( 1.4 ) ) );
        refField->SetTextAngle( fieldAngle );
        refField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        refField->SetVisible( showRef );

        valField->SetPosition( VECTOR2I( cxm, bymax * ORCAD_IU_PER_DBU + schMm( 1.4 ) ) );
        valField->SetTextAngle( fieldAngle );
        valField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        valField->SetVisible( showVal );
    }

    aSymbol->SetFootprintFieldText( FromOrcadString( aFootprint ) );

    SCH_FIELD* fpField = aSymbol->GetField( FIELD_T::FOOTPRINT );
    fpField->SetPosition( aSymbol->GetPosition() );
    fpField->SetVisible( false );

    SCH_FIELD* dsField = aSymbol->GetField( FIELD_T::DATASHEET );
    dsField->SetPosition( aSymbol->GetPosition() );
    dsField->SetVisible( false );

    // Display-prop positions are canvas-space offsets from instance anchor; field
    // text does not rotate w/ symbol, so not run through body orientation transform.
    std::map<std::string, std::pair<VECTOR2I, const ORCAD_DISPLAY_PROP*>> shown;

    for( const ORCAD_DISPLAY_PROP& dp : aInst.displayProps )
        shown[dp.name] = { OrcadDbuToIu( aInst.x + dp.x, aInst.y + dp.y ), &dp };

    // KiCad re-rotates field text when parent transform flips X/Y (GetDrawRotation),
    // so store angle that renders property's own text angle after flip.
    bool symbolFlips = aSymbol->GetTransform().y1 != 0;

    auto storedFieldAngle = [&]( const ORCAD_DISPLAY_PROP* aDp ) -> EDA_ANGLE
    {
        bool textVertical = ( aDp->rotation & 1 ) != 0;
        return ( textVertical != symbolFlips ) ? ANGLE_VERTICAL : ANGLE_HORIZONTAL;
    };

    // OrCAD carries explicit reference/value field positions; honor them so fields
    // land where source placed them, not computed fallback.
    auto applyDisplayPos = [&]( SCH_FIELD* aField, const char* aName )
    {
        auto it = shown.find( aName );

        if( it == shown.end() )
            return;

        const ORCAD_DISPLAY_PROP* dp = it->second.second;
        int                       size = textSizeIU( dp->fontIdx );

        // OrCAD stores text top-left corner; text grows right and down, so reference
        // stacks above value clear of body.
        aField->SetPosition( it->second.first );
        aField->SetTextSize( VECTOR2I( size, size ) );
        aField->SetTextAngle( storedFieldAngle( dp ) );
        aField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aField->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    };

    applyDisplayPos( refField, "Part Reference" );
    applyDisplayPos( valField, "Value" );

    for( const auto& [propName, propValue] : aInst.props )
    {
        if( propName == "Value" || propName == "PCB Footprint" || isBookkeepingProp( propName ) )
            continue;

        SCH_FIELD field( aSymbol, FIELD_T::USER, FromOrcadString( propName ) );
        field.SetText( FromOrcadString( propValue ) );

        auto sIt = shown.find( propName );

        if( sIt != shown.end() )
        {
            const ORCAD_DISPLAY_PROP* dp = sIt->second.second;
            int                       size = textSizeIU( dp->fontIdx );

            field.SetPosition( sIt->second.first );
            field.SetTextAngle( storedFieldAngle( dp ) );
            field.SetTextSize( VECTOR2I( size, size ) );
            field.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            field.SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            field.SetVisible( ( dp->dispMode & 0x100 ) != 0 );
        }
        else
        {
            field.SetPosition( aSymbol->GetPosition() );
            field.SetTextSize( VECTOR2I( schMm( 1.27 ), schMm( 1.27 ) ) );
            field.SetVisible( false );
        }

        aSymbol->AddField( field );
    }
}


void ORCAD_CONVERTER::computeFontBaseline()
{
    // Count references per font height; dominant height maps to KiCad default 1.27mm,
    // rest scale relative to it.
    std::map<int, int> counts;
    std::vector<int>   order;    // first-seen order breaks count ties

    auto tally = [&]( int aFontIdx )
    {
        int height = fontHeightDbu( aFontIdx );

        if( !height )
            return;

        auto [it, isNew] = counts.try_emplace( height, 0 );

        if( isNew )
            order.push_back( height );

        it->second++;
    };

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
    {
        for( const ORCAD_WIRE& wire : page.wires )
        {
            for( const ORCAD_ALIAS& alias : wire.aliases )
                tally( alias.fontIdx );
        }

        for( const ORCAD_PLACED_INSTANCE& inst : page.instances )
        {
            for( const ORCAD_DISPLAY_PROP& dp : inst.displayProps )
                tally( dp.fontIdx );
        }
    }

    int best = 0;
    int bestCount = 0;

    for( int height : order )
    {
        if( counts[height] > bestCount )
        {
            best = height;
            bestCount = counts[height];
        }
    }

    m_fontBaselineDbu = best;
}


int ORCAD_CONVERTER::fontHeightDbu( int aFontIdx ) const
{
    if( aFontIdx > 0 && aFontIdx <= (int) m_design.library.fonts.size() )
        return std::abs( m_design.library.fonts[aFontIdx - 1].height );

    return 0;
}


int ORCAD_CONVERTER::textSizeIU( int aFontIdx ) const
{
    int height = fontHeightDbu( aFontIdx );

    if( !height )
        return schMm( 1.27 );

    double mm;

    if( m_fontBaselineDbu )
    {
        mm = std::round( 1.27 * height / m_fontBaselineDbu * 100.0 ) / 100.0;
        mm = std::max( 0.7, std::min( 5.0, mm ) );
    }
    else
    {
        // LOGFONTA heights = device units at 96 dpi; 1 pt = 1/72 inch.
        mm = std::round( height * 25.4 * 0.75 / 96.0 * 100.0 ) / 100.0;
        mm = std::max( 0.7, mm );
    }

    return schMm( mm );
}


std::string ORCAD_CONVERTER::SymbolId( const std::string& aName )
{
    std::string out;
    bool        inRun = false;

    for( unsigned char c : aName )
    {
        // 0xA0 = CP-1252 non-breaking space.
        bool separator = c == ':' || c == '"' || c == '/' || c == ' ' || c == '\t' || c == '\n'
                         || c == '\r' || c == '\f' || c == '\v' || c == 0xA0;

        if( separator )
        {
            if( !inRun )
                out += '_';

            inRun = true;
        }
        else
        {
            out += (char) c;
            inRun = false;
        }
    }

    size_t first = out.find_first_not_of( '_' );

    if( first == std::string::npos )
        return "SYM";

    size_t last = out.find_last_not_of( '_' );
    out = out.substr( first, last - first + 1 );

    return out.empty() ? "SYM" : out;
}
