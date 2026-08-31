/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sch_io/pads/pads_sch_symbol_builder.h>

#include <lib_symbol.h>
#include <sch_shape.h>
#include <sch_pin.h>
#include <sch_text.h>
#include <pin_type.h>
#include <layer_ids.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <stroke_params.h>

#include <advanced_config.h>
#include <io/pads/pads_common.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <tuple>


namespace PADS_SCH
{

PADS_SCH_SYMBOL_BUILDER::PADS_SCH_SYMBOL_BUILDER( const PARAMETERS& aParams ) :
        m_params( aParams )
{
}


PADS_SCH_SYMBOL_BUILDER::~PADS_SCH_SYMBOL_BUILDER()
{
}


int PADS_SCH_SYMBOL_BUILDER::toKiCadUnits( double aPadsValue ) const
{
    // PADS Logic ASCII schematics always store geometry in mils. The UNITS field selects only
    // the design-rules unit and must not scale the schematic coordinates.
    return schIUScale.MilsToIU( aPadsValue );
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildSymbol( const SYMBOL_DEF& aSymbolDef )
{
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    // Add graphics
    for( const SYMBOL_GRAPHIC& graphic : aSymbolDef.graphics )
    {
        std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

        for( SCH_SHAPE* shape : shapes )
            libSymbol->AddDrawItem( shape );
    }

    // Add pins
    bool mapDiodeAK = isDiodeAKPinSet( aSymbolDef.pins );

    for( const SYMBOL_PIN& pin : aSymbolDef.pins )
    {
        SCH_PIN* schPin = createPin( pin, libSymbol, mapDiodeAK );

        if( schPin )
            libSymbol->AddDrawItem( schPin );
    }

    // Add embedded text labels
    for( const SYMBOL_TEXT& text : aSymbolDef.texts )
    {
        if( SCH_TEXT* schText = createSymbolText( text ) )
            libSymbol->AddDrawItem( schText );
    }

    libSymbol->SetShowPinNumbers( false );
    libSymbol->SetShowPinNames( false );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateSymbol( const SYMBOL_DEF& aSymbolDef )
{
    auto it = m_symbolCache.find( aSymbolDef.name );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* newSymbol = BuildSymbol( aSymbolDef );
    m_symbolCache[aSymbolDef.name] = std::unique_ptr<LIB_SYMBOL>( newSymbol );

    return newSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildMultiUnitSymbol( const PARTTYPE_DEF&            aPartType,
                                                           const std::vector<SYMBOL_DEF>& aSymbolDefs )
{
    // Build a lookup from CAEDECAL name to definition
    std::map<std::string, const SYMBOL_DEF*> symDefByName;

    for( const SYMBOL_DEF& sd : aSymbolDefs )
        symDefByName[sd.name] = &sd;

    int         gateCount = static_cast<int>( aPartType.gates.size() );
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aPartType.name ) );
    libSymbol->SetUnitCount( gateCount, false );
    libSymbol->LockUnits( true );

    for( int gi = 0; gi < gateCount; gi++ )
    {
        const GATE_DEF& gate = aPartType.gates[gi];
        int             unit = gi + 1;

        // Resolve the CAEDECAL for this gate
        std::string decalName;

        if( !gate.decal_names.empty() )
            decalName = gate.decal_names[0];

        auto sdIt = symDefByName.find( decalName );

        if( sdIt == symDefByName.end() )
            continue;

        const SYMBOL_DEF& symDef = *sdIt->second;

        // Add graphics for this unit
        for( const SYMBOL_GRAPHIC& graphic : symDef.graphics )
        {
            std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

            for( SCH_SHAPE* shape : shapes )
            {
                shape->SetUnit( unit );
                libSymbol->AddDrawItem( shape );
            }
        }

        // Add pins with PARTTYPE overrides
        std::vector<SYMBOL_PIN> pins = applyGateOverrides( symDef.pins, gate );
        bool                    mapDiodeAK = isDiodeAKPinSet( pins );

        for( const SYMBOL_PIN& pin : pins )
        {
            SCH_PIN* schPin = createPin( pin, libSymbol, mapDiodeAK );

            if( schPin )
            {
                schPin->SetUnit( unit );
                libSymbol->AddDrawItem( schPin );
            }
        }

        // Add embedded text labels for this unit
        for( const SYMBOL_TEXT& text : symDef.texts )
        {
            if( SCH_TEXT* schText = createSymbolText( text, unit ) )
                libSymbol->AddDrawItem( schText );
        }
    }

    libSymbol->SetShowPinNumbers( true );
    libSymbol->SetShowPinNames( true );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateMultiUnitSymbol( const PARTTYPE_DEF&            aPartType,
                                                                 const std::vector<SYMBOL_DEF>& aSymbolDefs )
{
    // Use a prefixed key to avoid collision with CAEDECAL symbols that may
    // share the same name as the PARTTYPE (e.g. both named "TL082").
    std::string cacheKey = "parttype:" + aPartType.name;
    auto        it = m_symbolCache.find( cacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* newSymbol = BuildMultiUnitSymbol( aPartType, aSymbolDefs );
    m_symbolCache[cacheKey] = std::unique_ptr<LIB_SYMBOL>( newSymbol );

    return newSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreatePartTypeSymbol( const PARTTYPE_DEF& aPartType,
                                                                const SYMBOL_DEF&   aSymbolDef )
{
    // Cache by PARTTYPE + CAEDECAL pair. A single-gate PARTTYPE with multiple decal
    // variants (e.g. horizontal vs vertical resistor) needs a separate LIB_SYMBOL per
    // variant because the graphics and pin positions differ.
    std::string cacheKey = aPartType.name + ":" + aSymbolDef.name;
    auto        it = m_symbolCache.find( cacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    if( aPartType.gates.empty() )
        return nullptr;

    // Build from the CAEDECAL then apply pin overrides from the PARTTYPE gate
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    for( const SYMBOL_GRAPHIC& graphic : aSymbolDef.graphics )
    {
        std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

        for( SCH_SHAPE* shape : shapes )
            libSymbol->AddDrawItem( shape );
    }

    const GATE_DEF&         gate = aPartType.gates[0];
    std::vector<SYMBOL_PIN> pins = applyGateOverrides( aSymbolDef.pins, gate );
    bool                    mapDiodeAK = isDiodeAKPinSet( pins );

    for( const SYMBOL_PIN& pin : pins )
    {
        SCH_PIN* schPin = createPin( pin, libSymbol, mapDiodeAK );

        if( schPin )
            libSymbol->AddDrawItem( schPin );
    }

    for( const SYMBOL_TEXT& text : aSymbolDef.texts )
    {
        if( SCH_TEXT* schText = createSymbolText( text ) )
            libSymbol->AddDrawItem( schText );
    }

    // Show pin names/numbers if any gate pin has an explicit name
    bool hasPinNames = false;

    for( const PARTTYPE_PIN& pin : gate.pins )
    {
        if( !pin.pin_name.empty() )
        {
            hasPinNames = true;
            break;
        }
    }

    // Connectors number their pins even though they carry no pin names. A single
    // multi-pin connector placement (no per-pin reference suffix) still routes here,
    // so force pin numbers on for connector part types.
    libSymbol->SetShowPinNumbers( hasPinNames || aPartType.is_connector );
    libSymbol->SetShowPinNames( hasPinNames );

    m_symbolCache[cacheKey] = std::unique_ptr<LIB_SYMBOL>( libSymbol );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateConnectorPinSymbol( const PARTTYPE_DEF& aPartType,
                                                                    const SYMBOL_DEF&   aSymbolDef,
                                                                    const std::string&  aPinNumber )
{
    std::string cacheKey = aPartType.name + ":" + aSymbolDef.name + ":" + aPinNumber;
    auto        it = m_symbolCache.find( cacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    for( const SYMBOL_GRAPHIC& graphic : aSymbolDef.graphics )
    {
        std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

        for( SCH_SHAPE* shape : shapes )
            libSymbol->AddDrawItem( shape );
    }

    // Create pin(s) from the CAEDECAL but override the pin number
    for( size_t p = 0; p < aSymbolDef.pins.size(); p++ )
    {
        SYMBOL_PIN pin = aSymbolDef.pins[p];
        pin.number = aPinNumber;

        if( !aPartType.gates.empty() && p < aPartType.gates[0].pins.size() )
        {
            pin.name = aPartType.gates[0].pins[p].pin_name;

            if( aPartType.gates[0].pins[p].pin_type != 0 )
                pin.type = PADS_SCH_PARSER::ParsePinTypeChar( aPartType.gates[0].pins[p].pin_type );
        }

        SCH_PIN* schPin = createPin( pin, libSymbol );

        if( schPin )
            libSymbol->AddDrawItem( schPin );
    }

    for( const SYMBOL_TEXT& text : aSymbolDef.texts )
    {
        if( SCH_TEXT* schText = createSymbolText( text ) )
            libSymbol->AddDrawItem( schText );
    }

    libSymbol->SetShowPinNumbers( false );
    libSymbol->SetShowPinNames( false );

    m_symbolCache[cacheKey] = std::unique_ptr<LIB_SYMBOL>( libSymbol );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildMultiUnitConnectorSymbol( const PARTTYPE_DEF&             aPartType,
                                                                    const SYMBOL_DEF&               aSymbolDef,
                                                                    const std::vector<std::string>& aPinNumbers )
{
    int         unitCount = static_cast<int>( aPinNumbers.size() );
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aPartType.name ) );
    libSymbol->SetUnitCount( unitCount, false );
    libSymbol->LockUnits( true );

    // Build a lookup from pin ID to PARTTYPE pin definition
    std::map<std::string, const PARTTYPE_PIN*> ptPinById;

    if( !aPartType.gates.empty() )
    {
        for( const PARTTYPE_PIN& ptPin : aPartType.gates[0].pins )
            ptPinById[ptPin.pin_id] = &ptPin;
    }

    for( int u = 0; u < unitCount; u++ )
    {
        int unit = u + 1;

        for( const SYMBOL_GRAPHIC& graphic : aSymbolDef.graphics )
        {
            std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

            for( SCH_SHAPE* shape : shapes )
            {
                shape->SetUnit( unit );
                libSymbol->AddDrawItem( shape );
            }
        }

        // One pin per unit with the correct pin number
        if( !aSymbolDef.pins.empty() )
        {
            SYMBOL_PIN pin = aSymbolDef.pins[0];
            pin.number = aPinNumbers[u];

            auto ptPinIt = ptPinById.find( aPinNumbers[u] );

            if( ptPinIt != ptPinById.end() )
            {
                if( ptPinIt->second->pin_type != 0 )
                    pin.type = PADS_SCH_PARSER::ParsePinTypeChar( ptPinIt->second->pin_type );

                if( !ptPinIt->second->pin_name.empty() )
                    pin.name = ptPinIt->second->pin_name;
            }

            SCH_PIN* schPin = createPin( pin, libSymbol );

            if( schPin )
            {
                schPin->SetUnit( unit );
                libSymbol->AddDrawItem( schPin );
            }
        }

        for( const SYMBOL_TEXT& text : aSymbolDef.texts )
        {
            if( SCH_TEXT* schText = createSymbolText( text, unit ) )
                libSymbol->AddDrawItem( schText );
        }
    }

    libSymbol->SetShowPinNumbers( true );
    libSymbol->SetShowPinNames( false );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateMultiUnitConnectorSymbol( const PARTTYPE_DEF&             aPartType,
                                                                          const SYMBOL_DEF&               aSymbolDef,
                                                                          const std::vector<std::string>& aPinNumbers,
                                                                          const std::string&              aCacheKey )
{
    auto it = m_symbolCache.find( aCacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* newSymbol = BuildMultiUnitConnectorSymbol( aPartType, aSymbolDef, aPinNumbers );
    m_symbolCache[aCacheKey] = std::unique_ptr<LIB_SYMBOL>( newSymbol );

    return newSymbol;
}


bool PADS_SCH_SYMBOL_BUILDER::HasSymbol( const std::string& aName ) const
{
    return m_symbolCache.find( aName ) != m_symbolCache.end();
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetSymbol( const std::string& aName ) const
{
    auto it = m_symbolCache.find( aName );

    if( it != m_symbolCache.end() )
        return it->second.get();

    return nullptr;
}


SCH_SHAPE* PADS_SCH_SYMBOL_BUILDER::createShape( const SYMBOL_GRAPHIC& aGraphic )
{
    SCH_SHAPE* shape = nullptr;

    switch( aGraphic.type )
    {
    case GRAPHIC_TYPE::LINE:
    case GRAPHIC_TYPE::POLYLINE:
    {
        bool hasArcs = false;

        for( const GRAPHIC_POINT& pt : aGraphic.points )
        {
            if( pt.arc.has_value() )
            {
                hasArcs = true;
                break;
            }
        }

        if( !hasArcs )
        {
            shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            for( const auto& pt : aGraphic.points )
                shape->AddPoint( VECTOR2I( toKiCadUnits( pt.coord.x ), -toKiCadUnits( pt.coord.y ) ) );
        }
        else
        {
            // Mixed line/arc path requires multiple shapes. Return nullptr here and let
            // BuildSymbol handle this via createShapes() instead.
            return nullptr;
        }

        break;
    }

    case GRAPHIC_TYPE::RECTANGLE:
    {
        shape = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

        if( aGraphic.points.size() >= 2 )
        {
            VECTOR2I start( toKiCadUnits( aGraphic.points[0].coord.x ), -toKiCadUnits( aGraphic.points[0].coord.y ) );
            VECTOR2I end( toKiCadUnits( aGraphic.points[1].coord.x ), -toKiCadUnits( aGraphic.points[1].coord.y ) );

            shape->SetStart( start );
            shape->SetEnd( end );
        }

        break;
    }

    case GRAPHIC_TYPE::CIRCLE:
    {
        shape = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );

        VECTOR2I center( toKiCadUnits( aGraphic.center.x ), -toKiCadUnits( aGraphic.center.y ) );
        int      radius = toKiCadUnits( aGraphic.radius );

        shape->SetStart( center );
        shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );

        break;
    }

    case GRAPHIC_TYPE::ARC:
    {
        shape = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );

        VECTOR2I center( toKiCadUnits( aGraphic.center.x ), -toKiCadUnits( aGraphic.center.y ) );
        int      radius = toKiCadUnits( aGraphic.radius );

        // Convert angles from PADS format to KiCad
        // PADS uses degrees, KiCad uses tenths of degrees for arc definition
        double startAngle = aGraphic.start_angle * M_PI / 180.0;
        double endAngle = aGraphic.end_angle * M_PI / 180.0;

        VECTOR2I startPt( center.x + radius * cos( startAngle ), center.y - radius * sin( startAngle ) );
        VECTOR2I endPt( center.x + radius * cos( endAngle ), center.y - radius * sin( endAngle ) );

        shape->SetStart( startPt );
        shape->SetEnd( endPt );
        shape->SetCenter( center );

        break;
    }
    }

    if( shape )
    {
        int lineWidth = toKiCadUnits( aGraphic.line_width );

        if( lineWidth == 0 )
            lineWidth = toKiCadUnits( m_params.line_width );

        shape->SetStroke( STROKE_PARAMS( lineWidth, PADS_COMMON::PadsLineStyleToKiCad( aGraphic.line_style ) ) );

        if( aGraphic.filled )
            shape->SetFillMode( FILL_T::FILLED_SHAPE );
    }

    return shape;
}


std::vector<SCH_SHAPE*> PADS_SCH_SYMBOL_BUILDER::createShapes( const SYMBOL_GRAPHIC& aGraphic )
{
    std::vector<SCH_SHAPE*> result;

    // Try the simple single-shape path first
    SCH_SHAPE* single = createShape( aGraphic );

    if( single )
    {
        result.push_back( single );
        return result;
    }

    // Mixed line/arc path: emit individual segments
    int lineWidth = toKiCadUnits( aGraphic.line_width );

    if( lineWidth == 0 )
        lineWidth = toKiCadUnits( m_params.line_width );

    LINE_STYLE lineStyle = PADS_COMMON::PadsLineStyleToKiCad( aGraphic.line_style );

    for( size_t i = 0; i + 1 < aGraphic.points.size(); i++ )
    {
        const GRAPHIC_POINT& cur = aGraphic.points[i];
        const GRAPHIC_POINT& next = aGraphic.points[i + 1];

        VECTOR2I startPt( toKiCadUnits( cur.coord.x ), -toKiCadUnits( cur.coord.y ) );
        VECTOR2I endPt( toKiCadUnits( next.coord.x ), -toKiCadUnits( next.coord.y ) );

        if( cur.arc.has_value() )
        {
            const ARC_DATA& ad = *cur.arc;
            double          cx = ( ad.bbox_x1 + ad.bbox_x2 ) / 2.0;
            double          cy = ( ad.bbox_y1 + ad.bbox_y2 ) / 2.0;
            VECTOR2I        center( toKiCadUnits( cx ), -toKiCadUnits( cy ) );

            VECTOR2I midPt = padsSchArcMidpoint( startPt, endPt, center );

            // The initial midpoint is always on the minor arc side (between start
            // and end radii). Flip to the major arc side when the sweep exceeds
            // 180 degrees. The sign of the angle encodes CW/CCW direction in PADS
            // but does not affect which semicircle the arc occupies.
            if( std::abs( ad.angle ) > 1800 )
            {
                midPt.x = 2 * center.x - midPt.x;
                midPt.y = 2 * center.y - midPt.y;
            }

            SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
            arc->SetArcGeometry( startPt, midPt, endPt );
            arc->SetStroke( STROKE_PARAMS( lineWidth, lineStyle ) );

            if( aGraphic.filled )
                arc->SetFillMode( FILL_T::FILLED_SHAPE );

            result.push_back( arc );
        }
        else
        {
            if( startPt == endPt )
                continue;

            SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line->AddPoint( startPt );
            line->AddPoint( endPt );
            line->SetStroke( STROKE_PARAMS( lineWidth, lineStyle ) );

            if( aGraphic.filled )
                line->SetFillMode( FILL_T::FILLED_SHAPE );

            result.push_back( line );
        }
    }

    return result;
}


SCH_TEXT* PADS_SCH_SYMBOL_BUILDER::createSymbolText( const SYMBOL_TEXT& aText, int aUnit )
{
    if( aText.content.empty() )
        return nullptr;

    SCH_TEXT* schText = new SCH_TEXT( VECTOR2I( toKiCadUnits( aText.position.x ), -toKiCadUnits( aText.position.y ) ),
                                      wxString::FromUTF8( aText.content ), LAYER_DEVICE );

    if( aText.size > 0.0 )
    {
        int scaledSize = toKiCadUnits( aText.size );
        int charHeight = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
        int charWidth = static_cast<int>( scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
        schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
    }

    if( aText.rotation != 0.0 )
        schText->SetTextAngleDegrees( aText.rotation );

    if( aUnit != 0 )
        schText->SetUnit( aUnit );

    return schText;
}


std::vector<SYMBOL_PIN> PADS_SCH_SYMBOL_BUILDER::applyGateOverrides( const std::vector<SYMBOL_PIN>& aDecalPins,
                                                                     const GATE_DEF&               aGate )
{
    std::vector<SYMBOL_PIN> pins = aDecalPins;

    for( size_t p = 0; p < pins.size() && p < aGate.pins.size(); p++ )
    {
        pins[p].name = aGate.pins[p].pin_name;
        pins[p].number = aGate.pins[p].pin_id;

        if( aGate.pins[p].pin_type != 0 )
            pins[p].type = PADS_SCH_PARSER::ParsePinTypeChar( aGate.pins[p].pin_type );
    }

    return pins;
}


bool PADS_SCH_SYMBOL_BUILDER::isDiodeAKPinSet( const std::vector<SYMBOL_PIN>& aPins )
{
    if( aPins.size() != 2 )
        return false;

    bool haveA = false;
    bool haveK = false;

    for( const SYMBOL_PIN& pin : aPins )
    {
        if( !pin.name.empty() )
            return false;

        if( pin.number == "A" )
            haveA = true;
        else if( pin.number == "K" )
            haveK = true;
        else
            return false;
    }

    return haveA && haveK;
}


SCH_PIN* PADS_SCH_SYMBOL_BUILDER::createPin( const SYMBOL_PIN& aPin, LIB_SYMBOL* aParent, bool aMapDiodeAK )
{
    SCH_PIN* pin = new SCH_PIN( aParent );

    // Set pin name and number
    if( aMapDiodeAK && aPin.number == "A" )
    {
        pin->SetName( wxString::FromUTF8( aPin.number ) );
        pin->SetNumber( wxT( "2" ) );
    }
    else if( aMapDiodeAK && aPin.number == "K" )
    {
        pin->SetName( wxString::FromUTF8( aPin.number ) );
        pin->SetNumber( wxT( "1" ) );
    }
    else
    {
        pin->SetName( wxString::FromUTF8( aPin.name ) );
        pin->SetNumber( wxString::FromUTF8( aPin.number ) );
    }

    // Set pin position (end point where wire connects)
    VECTOR2I pos( toKiCadUnits( aPin.position.x ), -toKiCadUnits( aPin.position.y ) );
    pin->SetPosition( pos );

    // Set pin length
    int length = toKiCadUnits( aPin.length );
    pin->SetLength( length );

    // Determine pin orientation from the T-line angle and side fields.
    // The angle indicates pin text rotation (0=horizontal, 90=vertical) while
    // the side field indicates which edge of the symbol body the pin is on.
    // Pin decal names containing "VRT" indicate perpendicular pins.
    PIN_ORIENTATION orientation = PIN_ORIENTATION::PIN_RIGHT;
    bool            isVerticalDecal = ( aPin.pin_decal_name.find( "VRT" ) != std::string::npos );
    int             angle = static_cast<int>( aPin.rotation ) % 360;

    if( isVerticalDecal )
    {
        orientation = ( aPin.side == 2 ) ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;
    }
    else if( angle >= 45 && angle < 135 )
    {
        // Sides 0,1 (horizontal edges) point up; sides 2,3 (vertical edges) point down
        orientation = ( aPin.side >= 2 ) ? PIN_ORIENTATION::PIN_DOWN : PIN_ORIENTATION::PIN_UP;
    }
    else if( angle >= 225 && angle < 315 )
    {
        orientation = ( aPin.side >= 2 ) ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;
    }
    else if( angle >= 135 && angle < 225 )
    {
        orientation = ( aPin.side & 1 ) ? PIN_ORIENTATION::PIN_RIGHT : PIN_ORIENTATION::PIN_LEFT;
    }
    else
    {
        orientation = ( aPin.side & 1 ) ? PIN_ORIENTATION::PIN_LEFT : PIN_ORIENTATION::PIN_RIGHT;
    }

    pin->SetOrientation( orientation );

    // Set electrical type
    ELECTRICAL_PINTYPE pinType = static_cast<ELECTRICAL_PINTYPE>( mapPinType( aPin.type ) );
    pin->SetType( pinType );

    // Set graphic style
    GRAPHIC_PINSHAPE pinShape = GRAPHIC_PINSHAPE::LINE;

    if( aPin.inverted )
        pinShape = GRAPHIC_PINSHAPE::INVERTED;
    else if( aPin.clock )
        pinShape = GRAPHIC_PINSHAPE::CLOCK;

    pin->SetShape( pinShape );

    int pinTextSize = schIUScale.MilsToIU( 50 );
    pin->SetNumberTextSize( pinTextSize );
    pin->SetNameTextSize( pinTextSize );

    return pin;
}


/// The distinct body shapes KiCad power symbols are drawn with. Several PADS power names share
/// a shape, so the name maps to a style and the style alone decides the geometry.
enum class POWER_STYLE
{
    GROUND_BARS,   // three descending horizontal bars, body below the pin
    FILLED_BAR,    // one thick filled bar, body below the pin
    FILLED_ARROW,  // filled triangle, body above the pin
    OPEN_ARROW,    // two open arrow strokes, body below the pin
    OPEN_CIRCLE    // open circle, body above the pin
};


static POWER_STYLE powerStyleFromName( const std::string& aUpperName )
{
    if( aUpperName == "GND" || aUpperName == "GNDA" || aUpperName == "GNDPWR" || aUpperName == "EARTH"
        || aUpperName == "CHASSIS" )
    {
        return POWER_STYLE::GROUND_BARS;
    }

    if( aUpperName == "GNDD" || aUpperName == "PWR_BAR" )
        return POWER_STYLE::FILLED_BAR;

    if( aUpperName == "PWR_TRIANGLE" )
        return POWER_STYLE::FILLED_ARROW;

    if( aUpperName == "VEE" || aUpperName == "VSS" )
        return POWER_STYLE::OPEN_ARROW;

    return POWER_STYLE::OPEN_CIRCLE;
}


static SCH_SHAPE* addPolyline( LIB_SYMBOL* aSymbol, const std::vector<VECTOR2I>& aPoints, int aWidth = 0 )
{
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    for( const VECTOR2I& point : aPoints )
        shape->AddPoint( point );

    shape->SetStroke( STROKE_PARAMS( aWidth, LINE_STYLE::SOLID ) );
    aSymbol->AddDrawItem( shape );

    return shape;
}


static void addPowerPin( LIB_SYMBOL* aSymbol, const std::string& aKiCadName, PIN_ORIENTATION aOrientation )
{
    SCH_PIN* pin = new SCH_PIN( aSymbol );
    pin->SetNumber( wxT( "1" ) );
    pin->SetName( wxString::FromUTF8( aKiCadName ) );
    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    pin->SetVisible( false );
    pin->SetLength( 0 );
    pin->SetPosition( VECTOR2I( 0, 0 ) );
    pin->SetOrientation( aOrientation );
    aSymbol->AddDrawItem( pin );
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildKiCadPowerSymbol( const std::string& aKiCadName )
{
    // Convert mm coordinates from KiCad power symbol library to internal units
    auto mm = [&]( double v )
    {
        return schIUScale.mmToIU( v );
    };

    LIB_SYMBOL* sym = new LIB_SYMBOL( wxString::FromUTF8( aKiCadName ) );
    sym->SetGlobalPower();
    sym->SetShowPinNumbers( false );
    sym->SetShowPinNames( false );

    // Determine which visual style to use based on the KiCad symbol name
    std::string upper = aKiCadName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c )
                    {
                        return std::toupper( c );
                    } );

    switch( powerStyleFromName( upper ) )
    {
    case POWER_STYLE::GROUND_BARS:
    {
        const std::array<std::tuple<double, double, double>, 3> bars{ std::tuple{ -1.27, 1.27, -1.27 },
                                                                      std::tuple{ -0.762, 0.762, -1.778 },
                                                                      std::tuple{ -0.254, 0.254, -2.286 } };

        for( const auto& [x1, x2, y] : bars )
            addPolyline( sym, { VECTOR2I( mm( x1 ), mm( y ) ), VECTOR2I( mm( x2 ), mm( y ) ) } );

        addPolyline( sym, { VECTOR2I( 0, 0 ), VECTOR2I( 0, mm( -1.27 ) ) } );
        addPowerPin( sym, aKiCadName, PIN_ORIENTATION::PIN_DOWN );
        break;
    }

    case POWER_STYLE::FILLED_BAR:
    {
        // Placed with 180 degree rotation for positive supplies (+V1) so the bar points up on
        // the schematic. Negative supplies (-V1) use it unrotated.
        SCH_SHAPE* bar = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        bar->SetStart( VECTOR2I( mm( -1.27 ), mm( -1.524 ) ) );
        bar->SetEnd( VECTOR2I( mm( 1.27 ), mm( -2.032 ) ) );
        bar->SetStroke( STROKE_PARAMS( mm( 0.254 ), LINE_STYLE::SOLID ) );
        bar->SetFillMode( FILL_T::FILLED_SHAPE );
        sym->AddDrawItem( bar );

        addPolyline( sym, { VECTOR2I( mm( 0 ), mm( 0 ) ), VECTOR2I( mm( 0 ), mm( -1.524 ) ) } );
        addPowerPin( sym, aKiCadName, PIN_ORIENTATION::PIN_DOWN );
        break;
    }

    case POWER_STYLE::FILLED_ARROW:
    {
        SCH_SHAPE* tri = addPolyline( sym, { VECTOR2I( mm( 0.762 ), mm( 1.27 ) ),
                                             VECTOR2I( mm( -0.762 ), mm( 1.27 ) ),
                                             VECTOR2I( mm( 0 ), mm( 2.54 ) ),
                                             VECTOR2I( mm( 0.762 ), mm( 1.27 ) ) } );
        tri->SetFillMode( FILL_T::FILLED_SHAPE );

        addPolyline( sym, { VECTOR2I( mm( 0 ), mm( 0 ) ), VECTOR2I( mm( 0 ), mm( 1.27 ) ) } );
        addPowerPin( sym, aKiCadName, PIN_ORIENTATION::PIN_UP );
        break;
    }

    case POWER_STYLE::OPEN_ARROW:
        addPolyline( sym, { VECTOR2I( mm( -0.762 ), mm( -1.27 ) ), VECTOR2I( mm( 0 ), mm( -2.54 ) ) } );
        addPolyline( sym, { VECTOR2I( mm( 0 ), mm( -2.54 ) ), VECTOR2I( mm( 0.762 ), mm( -1.27 ) ) } );
        addPolyline( sym, { VECTOR2I( mm( 0 ), mm( 0 ) ), VECTOR2I( mm( 0 ), mm( -2.54 ) ) } );
        addPowerPin( sym, aKiCadName, PIN_ORIENTATION::PIN_DOWN );
        break;

    case POWER_STYLE::OPEN_CIRCLE:
    {
        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
        circle->SetCenter( VECTOR2I( 0, mm( 2.032 ) ) );
        circle->SetEnd( VECTOR2I( mm( 0.635 ), mm( 2.032 ) ) );
        circle->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        circle->SetFillMode( FILL_T::NO_FILL );
        sym->AddDrawItem( circle );

        addPolyline( sym, { VECTOR2I( 0, 0 ), VECTOR2I( 0, mm( 1.397 ) ) } );
        addPowerPin( sym, aKiCadName, PIN_ORIENTATION::PIN_UP );
        break;
    }
    }

    sym->GetReferenceField().SetText( wxT( "#PWR" ) );
    sym->GetReferenceField().SetVisible( false );

    return sym;
}


std::string PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( const std::string& aDecalName,
                                                               const std::string& aPinType )
{
    std::string upper = aDecalName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c )
                    {
                        return std::toupper( c );
                    } );

    bool isPositive = !upper.empty() && upper[0] == '+';
    bool isGround = ( aPinType == "G" );

    if( upper.find( "RAIL" ) != std::string::npos )
        return isPositive ? "PWR_BAR" : "GNDD";

    if( upper.find( "ARROW" ) != std::string::npos )
        return isPositive ? "PWR_TRIANGLE" : "VEE";

    if( upper.find( "BUBBLE" ) != std::string::npos )
        return isPositive ? "VCC" : "VEE";

    if( isGround )
    {
        if( upper.find( "CH" ) != std::string::npos )
            return "Chassis";

        return "GND";
    }

    if( isPositive )
        return "VCC";

    return "VEE";
}


int PADS_SCH_SYMBOL_BUILDER::NextFreePowerOrdinal( SCH_SHEET* aSheet )
{
    int next = 1;

    if( !aSheet )
        return next;

    // Walk the destination directly; SCHEMATIC::Hierarchy() is a cache the importer has
    // no reason to have refreshed mid-load
    for( const SCH_SHEET_PATH& path : SCH_SHEET_LIST( aSheet ) )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            wxString digits;
            long     ordinal = 0;

            if( static_cast<SCH_SYMBOL*>( item )->GetRef( &path ).StartsWith( wxS( "#PWR" ), &digits )
                && digits.ToLong( &ordinal ) )
            {
                next = std::max( next, static_cast<int>( ordinal ) + 1 );
            }
        }
    }

    return next;
}


void PADS_SCH_SYMBOL_BUILDER::AddHiddenPowerPins( LIB_SYMBOL*                              aSymbol,
                                                  const std::vector<PARTTYPE_DEF::SIGPIN>& aSigpins )
{
    if( !aSymbol )
        return;

    // Collect existing pin numbers to avoid duplicates
    std::set<wxString> existingPins;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            existingPins.insert( static_cast<const SCH_PIN&>( item ).GetNumber() );
    }

    for( const PARTTYPE_DEF::SIGPIN& sp : aSigpins )
    {
        wxString pinNum = wxString::FromUTF8( sp.pin_number );

        if( existingPins.count( pinNum ) )
            continue;

        SCH_PIN* pin = new SCH_PIN( aSymbol );
        pin->SetNumber( pinNum );
        pin->SetName( wxString::FromUTF8( sp.net_name ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );

        aSymbol->AddDrawItem( pin );
        existingPins.insert( pinNum );
    }
}


int PADS_SCH_SYMBOL_BUILDER::mapPinType( PIN_TYPE aPadsType )
{
    switch( aPadsType )
    {
    case PIN_TYPE::INPUT: return static_cast<int>( ELECTRICAL_PINTYPE::PT_INPUT );
    case PIN_TYPE::OUTPUT: return static_cast<int>( ELECTRICAL_PINTYPE::PT_OUTPUT );
    case PIN_TYPE::BIDIRECTIONAL: return static_cast<int>( ELECTRICAL_PINTYPE::PT_BIDI );
    case PIN_TYPE::TRISTATE: return static_cast<int>( ELECTRICAL_PINTYPE::PT_TRISTATE );
    case PIN_TYPE::OPEN_COLLECTOR: return static_cast<int>( ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR );
    case PIN_TYPE::OPEN_EMITTER: return static_cast<int>( ELECTRICAL_PINTYPE::PT_OPENEMITTER );
    case PIN_TYPE::POWER: return static_cast<int>( ELECTRICAL_PINTYPE::PT_POWER_IN );
    case PIN_TYPE::PASSIVE: return static_cast<int>( ELECTRICAL_PINTYPE::PT_PASSIVE );
    case PIN_TYPE::UNSPECIFIED:
    default: return static_cast<int>( ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
    }
}


bool PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( const std::string& aName )
{
    // Convert to uppercase for case-insensitive comparison
    std::string upper = aName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c )
                    {
                        return std::toupper( c );
                    } );

    // Check for ground variants
    if( upper == "GND" || upper == "AGND" || upper == "DGND" || upper == "PGND" || upper == "EARTH"
        || upper == "CHASSIS" || upper == "VSS" || upper == "0V" )
    {
        return true;
    }

    // Check for power supply variants
    if( upper == "VCC" || upper == "VDD" || upper == "VEE" || upper == "VPP" || upper == "VBAT" || upper == "VBUS"
        || upper == "V+" || upper == "V-" )
    {
        return true;
    }

    // Check for voltage patterns like +3V3, +5V, -12V, +V1, -V2, etc.
    if( upper.length() >= 2 && ( upper[0] == '+' || upper[0] == '-' ) )
        return true;

    return false;
}


std::optional<LIB_ID> PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( const std::string& aPadsName )
{
    // Convert to uppercase for case-insensitive comparison
    std::string upper = aPadsName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c )
                    {
                        return std::toupper( c );
                    } );

    // Map common power symbol names to KiCad power library symbols
    struct PowerMapping
    {
        const char* padsName;
        const char* kicadSymbol;
    };

    static const PowerMapping mappings[] = {
        { "GND", "GND" },         { "AGND", "GND" },  { "DGND", "GNDD" }, { "PGND", "GNDPWR" }, { "EARTH", "Earth" },
        { "CHASSIS", "Chassis" }, { "VSS", "VSS" },   { "0V", "GND" },    { "VCC", "VCC" },     { "VDD", "VDD" },
        { "VEE", "VEE" },         { "VPP", "VPP" },   { "VBAT", "VBAT" }, { "VBUS", "VBUS" },   { "V+", "VCC" },
        { "V-", "VEE" },          { "+5V", "+5V" },   { "-5V", "-5V" },   { "+3V3", "+3V3" },   { "+3.3V", "+3V3" },
        { "+12V", "+12V" },       { "-12V", "-12V" }, { "+15V", "+15V" }, { "-15V", "-15V" },   { "+1V8", "+1V8" },
        { "+2V5", "+2V5" },       { "+9V", "+9V" },   { "+24V", "+24V" },
    };

    for( const auto& mapping : mappings )
    {
        if( upper == mapping.padsName )
        {
            LIB_ID libId;
            libId.SetLibNickname( "power" );
            libId.SetLibItemName( mapping.kicadSymbol );
            return libId;
        }
    }

    // Generic handling for +/- prefixed names not in the table
    if( upper.length() >= 2 && upper[0] == '+' )
    {
        LIB_ID libId;
        libId.SetLibNickname( "power" );
        libId.SetLibItemName( "VCC" );
        return libId;
    }

    if( upper.length() >= 2 && upper[0] == '-' )
    {
        LIB_ID libId;
        libId.SetLibNickname( "power" );
        libId.SetLibItemName( "VEE" );
        return libId;
    }

    return std::nullopt;
}

} // namespace PADS_SCH
