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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/pads/pads_sch_symbol_builder.h>

#include <lib_symbol.h>
#include <sch_shape.h>
#include <sch_pin.h>
#include <sch_text.h>
#include <pin_type.h>
#include <layer_ids.h>
#include <sch_screen.h>
#include <stroke_params.h>

#include <advanced_config.h>
#include <io/pads/pads_common.h>

#include <algorithm>
#include <cctype>
#include <cmath>


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
    // Convert from PADS units to KiCad internal units (nanometers)
    // PADS uses mils by default, KiCad schematic uses schIUScale.MilsToIU()

    double milsValue = aPadsValue;

    switch( m_params.units )
    {
    case UNIT_TYPE::MILS:
        milsValue = aPadsValue;
        break;

    case UNIT_TYPE::METRIC:
        // mm to mils: 1 mm = 39.37 mils
        milsValue = aPadsValue * 39.3701;
        break;

    case UNIT_TYPE::INCHES:
        // inches to mils
        milsValue = aPadsValue * 1000.0;
        break;
    }

    return schIUScale.MilsToIU( milsValue );
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildSymbol( const SYMBOL_DEF& aSymbolDef )
{
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    // Add graphics
    for( const auto& graphic : aSymbolDef.graphics )
    {
        std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

        for( SCH_SHAPE* shape : shapes )
            libSymbol->AddDrawItem( shape );
    }

    // Add pins
    for( const auto& pin : aSymbolDef.pins )
    {
        SCH_PIN* schPin = createPin( pin, libSymbol );

        if( schPin )
            libSymbol->AddDrawItem( schPin );
    }

    // Add embedded text labels
    for( const auto& text : aSymbolDef.texts )
    {
        if( text.content.empty() )
            continue;

        SCH_TEXT* schText = new SCH_TEXT(
                VECTOR2I( toKiCadUnits( text.position.x ), -toKiCadUnits( text.position.y ) ),
                wxString::FromUTF8( text.content ), LAYER_DEVICE );

        if( text.size > 0.0 )
        {
            int scaledSize = toKiCadUnits( text.size );
            int charHeight = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
            int charWidth = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
            schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
        }

        if( text.rotation != 0.0 )
            schText->SetTextAngleDegrees( text.rotation );

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


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildMultiUnitSymbol(
        const PARTTYPE_DEF& aPartType, const std::vector<SYMBOL_DEF>& aSymbolDefs )
{
    // Build a lookup from CAEDECAL name to definition
    std::map<std::string, const SYMBOL_DEF*> symDefByName;

    for( const auto& sd : aSymbolDefs )
        symDefByName[sd.name] = &sd;

    int gateCount = static_cast<int>( aPartType.gates.size() );
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aPartType.name ) );
    libSymbol->SetUnitCount( gateCount, false );
    libSymbol->LockUnits( true );

    for( int gi = 0; gi < gateCount; gi++ )
    {
        const GATE_DEF& gate = aPartType.gates[gi];
        int unit = gi + 1;

        // Resolve the CAEDECAL for this gate
        std::string decalName;

        if( !gate.decal_names.empty() )
            decalName = gate.decal_names[0];

        auto sdIt = symDefByName.find( decalName );

        if( sdIt == symDefByName.end() )
            continue;

        const SYMBOL_DEF& symDef = *sdIt->second;

        // Add graphics for this unit
        for( const auto& graphic : symDef.graphics )
        {
            std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

            for( SCH_SHAPE* shape : shapes )
            {
                shape->SetUnit( unit );
                libSymbol->AddDrawItem( shape );
            }
        }

        // Add pins with PARTTYPE overrides
        for( size_t p = 0; p < symDef.pins.size(); p++ )
        {
            SYMBOL_PIN pin = symDef.pins[p];

            if( p < gate.pins.size() )
            {
                pin.name = gate.pins[p].pin_name;
                pin.number = gate.pins[p].pin_id;

                if( gate.pins[p].pin_type != 0 )
                    pin.type = PADS_SCH_PARSER::ParsePinTypeChar( gate.pins[p].pin_type );
            }

            SCH_PIN* schPin = createPin( pin, libSymbol );

            if( schPin )
            {
                schPin->SetUnit( unit );
                libSymbol->AddDrawItem( schPin );
            }
        }

        // Add embedded text labels for this unit
        for( const auto& text : symDef.texts )
        {
            if( text.content.empty() )
                continue;

            SCH_TEXT* schText = new SCH_TEXT(
                    VECTOR2I( toKiCadUnits( text.position.x ),
                              -toKiCadUnits( text.position.y ) ),
                    wxString::FromUTF8( text.content ), LAYER_DEVICE );

            if( text.size > 0.0 )
            {
                int scaledSize = toKiCadUnits( text.size );
                int charHeight = static_cast<int>(
                            scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
                int charWidth = static_cast<int>(
                            scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
                schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
            }

            if( text.rotation != 0.0 )
                schText->SetTextAngleDegrees( text.rotation );

            schText->SetUnit( unit );
            libSymbol->AddDrawItem( schText );
        }
    }

    libSymbol->SetShowPinNumbers( true );
    libSymbol->SetShowPinNames( true );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateMultiUnitSymbol(
        const PARTTYPE_DEF& aPartType, const std::vector<SYMBOL_DEF>& aSymbolDefs )
{
    auto it = m_symbolCache.find( aPartType.name );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* newSymbol = BuildMultiUnitSymbol( aPartType, aSymbolDefs );
    m_symbolCache[aPartType.name] = std::unique_ptr<LIB_SYMBOL>( newSymbol );

    return newSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreatePartTypeSymbol(
        const PARTTYPE_DEF& aPartType, const SYMBOL_DEF& aSymbolDef )
{
    // Cache by PARTTYPE + CAEDECAL pair. A single-gate PARTTYPE with multiple decal
    // variants (e.g. horizontal vs vertical resistor) needs a separate LIB_SYMBOL per
    // variant because the graphics and pin positions differ.
    std::string cacheKey = aPartType.name + ":" + aSymbolDef.name;
    auto it = m_symbolCache.find( cacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    if( aPartType.gates.empty() )
        return nullptr;

    // Build from the CAEDECAL then apply pin overrides from the PARTTYPE gate
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    for( const auto& graphic : aSymbolDef.graphics )
    {
        std::vector<SCH_SHAPE*> shapes = createShapes( graphic );

        for( SCH_SHAPE* shape : shapes )
            libSymbol->AddDrawItem( shape );
    }

    const GATE_DEF& gate = aPartType.gates[0];

    for( size_t p = 0; p < aSymbolDef.pins.size(); p++ )
    {
        SYMBOL_PIN pin = aSymbolDef.pins[p];

        if( p < gate.pins.size() )
        {
            pin.name = gate.pins[p].pin_name;
            pin.number = gate.pins[p].pin_id;

            if( gate.pins[p].pin_type != 0 )
                pin.type = PADS_SCH_PARSER::ParsePinTypeChar( gate.pins[p].pin_type );
        }

        SCH_PIN* schPin = createPin( pin, libSymbol );

        if( schPin )
            libSymbol->AddDrawItem( schPin );
    }

    for( const auto& text : aSymbolDef.texts )
    {
        if( text.content.empty() )
            continue;

        SCH_TEXT* schText = new SCH_TEXT(
                VECTOR2I( toKiCadUnits( text.position.x ),
                          -toKiCadUnits( text.position.y ) ),
                wxString::FromUTF8( text.content ), LAYER_DEVICE );

        if( text.size > 0.0 )
        {
            int scaledSize = toKiCadUnits( text.size );
            int charHeight = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
            int charWidth = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
            schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
        }

        if( text.rotation != 0.0 )
            schText->SetTextAngleDegrees( text.rotation );

        libSymbol->AddDrawItem( schText );
    }

    // Show pin names/numbers if any gate pin has an explicit name
    bool hasPinNames = false;

    for( const auto& pin : gate.pins )
    {
        if( !pin.pin_name.empty() )
        {
            hasPinNames = true;
            break;
        }
    }

    libSymbol->SetShowPinNumbers( hasPinNames );
    libSymbol->SetShowPinNames( hasPinNames );

    m_symbolCache[cacheKey] = std::unique_ptr<LIB_SYMBOL>( libSymbol );

    return libSymbol;
}


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::GetOrCreateConnectorPinSymbol(
        const PARTTYPE_DEF& aPartType, const SYMBOL_DEF& aSymbolDef,
        const std::string& aPinNumber )
{
    std::string cacheKey = aPartType.name + ":" + aSymbolDef.name + ":" + aPinNumber;
    auto it = m_symbolCache.find( cacheKey );

    if( it != m_symbolCache.end() )
        return it->second.get();

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxString::FromUTF8( aSymbolDef.name ) );

    for( const auto& graphic : aSymbolDef.graphics )
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

    for( const auto& text : aSymbolDef.texts )
    {
        if( text.content.empty() )
            continue;

        SCH_TEXT* schText = new SCH_TEXT(
                VECTOR2I( toKiCadUnits( text.position.x ),
                          -toKiCadUnits( text.position.y ) ),
                wxString::FromUTF8( text.content ), LAYER_DEVICE );

        if( text.size > 0.0 )
        {
            int scaledSize = toKiCadUnits( text.size );
            int charHeight = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextHeightScale );
            int charWidth = static_cast<int>(
                        scaledSize * ADVANCED_CFG::GetCfg().m_PadsSchTextWidthScale );
            schText->SetTextSize( VECTOR2I( charWidth, charHeight ) );
        }

        if( text.rotation != 0.0 )
            schText->SetTextAngleDegrees( text.rotation );

        libSymbol->AddDrawItem( schText );
    }

    libSymbol->SetShowPinNumbers( false );
    libSymbol->SetShowPinNames( false );

    m_symbolCache[cacheKey] = std::unique_ptr<LIB_SYMBOL>( libSymbol );

    return libSymbol;
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

        for( const auto& pt : aGraphic.points )
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
                shape->AddPoint( VECTOR2I( toKiCadUnits( pt.coord.x ),
                                           -toKiCadUnits( pt.coord.y ) ) );
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
            VECTOR2I start( toKiCadUnits( aGraphic.points[0].coord.x ),
                           -toKiCadUnits( aGraphic.points[0].coord.y ) );
            VECTOR2I end( toKiCadUnits( aGraphic.points[1].coord.x ),
                         -toKiCadUnits( aGraphic.points[1].coord.y ) );

            shape->SetStart( start );
            shape->SetEnd( end );
        }

        break;
    }

    case GRAPHIC_TYPE::CIRCLE:
    {
        shape = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );

        VECTOR2I center( toKiCadUnits( aGraphic.center.x ), -toKiCadUnits( aGraphic.center.y ) );
        int radius = toKiCadUnits( aGraphic.radius );

        shape->SetStart( center );
        shape->SetEnd( VECTOR2I( center.x + radius, center.y ) );

        break;
    }

    case GRAPHIC_TYPE::ARC:
    {
        shape = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );

        VECTOR2I center( toKiCadUnits( aGraphic.center.x ), -toKiCadUnits( aGraphic.center.y ) );
        int radius = toKiCadUnits( aGraphic.radius );

        // Convert angles from PADS format to KiCad
        // PADS uses degrees, KiCad uses tenths of degrees for arc definition
        double startAngle = aGraphic.start_angle * M_PI / 180.0;
        double endAngle = aGraphic.end_angle * M_PI / 180.0;

        VECTOR2I startPt( center.x + radius * cos( startAngle ),
                          center.y - radius * sin( startAngle ) );
        VECTOR2I endPt( center.x + radius * cos( endAngle ),
                        center.y - radius * sin( endAngle ) );

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
            double cx = ( ad.bbox_x1 + ad.bbox_x2 ) / 2.0;
            double cy = ( ad.bbox_y1 + ad.bbox_y2 ) / 2.0;
            VECTOR2I center( toKiCadUnits( cx ), -toKiCadUnits( cy ) );

            // Compute the arc midpoint on the circle between start and end.
            // Use vector math: midpoint of arc = center + R * normalize(midvector)
            // where midvector = (start - center) + (end - center)
            double sx = startPt.x - center.x;
            double sy = startPt.y - center.y;
            double ex = endPt.x - center.x;
            double ey = endPt.y - center.y;
            double radius = std::sqrt( sx * sx + sy * sy );

            double mx = sx + ex;
            double my = sy + ey;
            double mlen = std::sqrt( mx * mx + my * my );

            VECTOR2I midPt;

            if( mlen > 0.001 )
            {
                midPt.x = center.x + static_cast<int>( radius * mx / mlen );
                midPt.y = center.y + static_cast<int>( radius * my / mlen );
            }
            else
            {
                // Start and end are diametrically opposite, pick perpendicular direction
                midPt.x = center.x + static_cast<int>( -sy * radius / std::max( radius, 1.0 ) );
                midPt.y = center.y + static_cast<int>( sx * radius / std::max( radius, 1.0 ) );
            }

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


SCH_PIN* PADS_SCH_SYMBOL_BUILDER::createPin( const SYMBOL_PIN& aPin, LIB_SYMBOL* aParent )
{
    SCH_PIN* pin = new SCH_PIN( aParent );

    // Set pin name and number
    pin->SetName( wxString::FromUTF8( aPin.name ) );
    pin->SetNumber( wxString::FromUTF8( aPin.number ) );

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
    bool isVerticalDecal = ( aPin.pin_decal_name.find( "VRT" ) != std::string::npos );
    int angle = static_cast<int>( aPin.rotation ) % 360;

    if( isVerticalDecal )
    {
        orientation = ( aPin.side == 2 ) ? PIN_ORIENTATION::PIN_UP
                                         : PIN_ORIENTATION::PIN_DOWN;
    }
    else if( angle >= 45 && angle < 135 )
    {
        // Sides 0,1 (horizontal edges) point up; sides 2,3 (vertical edges) point down
        orientation = ( aPin.side >= 2 ) ? PIN_ORIENTATION::PIN_DOWN
                                         : PIN_ORIENTATION::PIN_UP;
    }
    else if( angle >= 225 && angle < 315 )
    {
        orientation = ( aPin.side >= 2 ) ? PIN_ORIENTATION::PIN_UP
                                         : PIN_ORIENTATION::PIN_DOWN;
    }
    else if( angle >= 135 && angle < 225 )
    {
        orientation = ( aPin.side & 1 ) ? PIN_ORIENTATION::PIN_RIGHT
                                        : PIN_ORIENTATION::PIN_LEFT;
    }
    else
    {
        orientation = ( aPin.side & 1 ) ? PIN_ORIENTATION::PIN_LEFT
                                        : PIN_ORIENTATION::PIN_RIGHT;
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


LIB_SYMBOL* PADS_SCH_SYMBOL_BUILDER::BuildKiCadPowerSymbol( const std::string& aKiCadName )
{
    // Convert mm coordinates from KiCad power symbol library to internal units
    auto mm = [&]( double v ) { return schIUScale.mmToIU( v ); };

    LIB_SYMBOL* sym = new LIB_SYMBOL( wxString::FromUTF8( aKiCadName ) );
    sym->SetGlobalPower();
    sym->SetShowPinNumbers( false );
    sym->SetShowPinNames( false );

    // Determine which visual style to use based on the KiCad symbol name
    std::string upper = aKiCadName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c ) { return std::toupper( c ); } );

    bool isGround = ( upper == "GND" || upper == "GNDA" || upper == "GNDPWR" );
    bool isGNDD = ( upper == "GNDD" );
    bool isPwrBar = ( upper == "PWR_BAR" );
    bool isPwrTriangle = ( upper == "PWR_TRIANGLE" );
    bool isVEE = ( upper == "VEE" || upper == "VSS" );
    bool isEarth = ( upper == "EARTH" || upper == "CHASSIS" );

    // Default to VCC style (open arrow up) for anything not matched above
    bool isVCC = !isGround && !isGNDD && !isPwrBar && !isPwrTriangle
                 && !isVEE && !isEarth;

    if( isGround )
    {
        // Standard GND chevron: polyline (0,0)→(0,-1.27)→(1.27,-1.27)→(0,-2.54)→(-1.27,-1.27)→(0,-1.27)
        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        shape->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        shape->AddPoint( VECTOR2I( mm( 0 ), mm( -1.27 ) ) );
        shape->AddPoint( VECTOR2I( mm( 1.27 ), mm( -1.27 ) ) );
        shape->AddPoint( VECTOR2I( mm( 0 ), mm( -2.54 ) ) );
        shape->AddPoint( VECTOR2I( mm( -1.27 ), mm( -1.27 ) ) );
        shape->AddPoint( VECTOR2I( mm( 0 ), mm( -1.27 ) ) );
        shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( shape );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        sym->AddDrawItem( pin );
    }
    else if( isGNDD )
    {
        // GNDD: thick filled bar + vertical stem
        SCH_SHAPE* bar = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        bar->SetStart( VECTOR2I( mm( -1.27 ), mm( -1.524 ) ) );
        bar->SetEnd( VECTOR2I( mm( 1.27 ), mm( -2.032 ) ) );
        bar->SetStroke( STROKE_PARAMS( mm( 0.254 ), LINE_STYLE::SOLID ) );
        bar->SetFillMode( FILL_T::FILLED_SHAPE );
        sym->AddDrawItem( bar );

        SCH_SHAPE* stem = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( -1.524 ) ) );
        stem->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stem );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        sym->AddDrawItem( pin );
    }
    else if( isPwrBar )
    {
        // PWR_BAR: same bar-down shape as GNDD. Placed with 180° rotation for
        // positive supplies (+V1) so the bar points up on the schematic.
        // Negative supplies (-V1) use GNDD directly without rotation.
        SCH_SHAPE* bar = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        bar->SetStart( VECTOR2I( mm( -1.27 ), mm( -1.524 ) ) );
        bar->SetEnd( VECTOR2I( mm( 1.27 ), mm( -2.032 ) ) );
        bar->SetStroke( STROKE_PARAMS( mm( 0.254 ), LINE_STYLE::SOLID ) );
        bar->SetFillMode( FILL_T::FILLED_SHAPE );
        sym->AddDrawItem( bar );

        SCH_SHAPE* stem = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( -1.524 ) ) );
        stem->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stem );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        sym->AddDrawItem( pin );
    }
    else if( isPwrTriangle )
    {
        // PWR_TRIANGLE: filled triangle pointing UP (like -9V style)
        SCH_SHAPE* tri = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        tri->AddPoint( VECTOR2I( mm( 0.762 ), mm( 1.27 ) ) );
        tri->AddPoint( VECTOR2I( mm( -0.762 ), mm( 1.27 ) ) );
        tri->AddPoint( VECTOR2I( mm( 0 ), mm( 2.54 ) ) );
        tri->AddPoint( VECTOR2I( mm( 0.762 ), mm( 1.27 ) ) );
        tri->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        tri->SetFillMode( FILL_T::FILLED_SHAPE );
        sym->AddDrawItem( tri );

        SCH_SHAPE* stem = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stem->AddPoint( VECTOR2I( mm( 0 ), mm( 1.27 ) ) );
        stem->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stem );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_UP );
        sym->AddDrawItem( pin );
    }
    else if( isVEE )
    {
        // VEE: inverted arrow (pointing down), pin at bottom
        SCH_SHAPE* arrow1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        arrow1->AddPoint( VECTOR2I( mm( -0.762 ), mm( -1.27 ) ) );
        arrow1->AddPoint( VECTOR2I( mm( 0 ), mm( -2.54 ) ) );
        arrow1->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( arrow1 );

        SCH_SHAPE* arrow2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        arrow2->AddPoint( VECTOR2I( mm( 0 ), mm( -2.54 ) ) );
        arrow2->AddPoint( VECTOR2I( mm( 0.762 ), mm( -1.27 ) ) );
        arrow2->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( arrow2 );

        SCH_SHAPE* stemLine = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( -2.54 ) ) );
        stemLine->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stemLine );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        sym->AddDrawItem( pin );
    }
    else if( isEarth )
    {
        // Earth: horizontal bars descending in width + vertical stem
        SCH_SHAPE* bar1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        bar1->AddPoint( VECTOR2I( mm( -1.27 ), mm( -1.27 ) ) );
        bar1->AddPoint( VECTOR2I( mm( 1.27 ), mm( -1.27 ) ) );
        bar1->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( bar1 );

        SCH_SHAPE* bar2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        bar2->AddPoint( VECTOR2I( mm( -0.762 ), mm( -1.778 ) ) );
        bar2->AddPoint( VECTOR2I( mm( 0.762 ), mm( -1.778 ) ) );
        bar2->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( bar2 );

        SCH_SHAPE* bar3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        bar3->AddPoint( VECTOR2I( mm( -0.254 ), mm( -2.286 ) ) );
        bar3->AddPoint( VECTOR2I( mm( 0.254 ), mm( -2.286 ) ) );
        bar3->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( bar3 );

        SCH_SHAPE* stemLine = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( -1.27 ) ) );
        stemLine->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stemLine );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        sym->AddDrawItem( pin );
    }
    else if( isVCC )
    {
        // VCC style: open arrow pointing up + vertical stem
        SCH_SHAPE* arrow1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        arrow1->AddPoint( VECTOR2I( mm( -0.762 ), mm( 1.27 ) ) );
        arrow1->AddPoint( VECTOR2I( mm( 0 ), mm( 2.54 ) ) );
        arrow1->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( arrow1 );

        SCH_SHAPE* arrow2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        arrow2->AddPoint( VECTOR2I( mm( 0 ), mm( 2.54 ) ) );
        arrow2->AddPoint( VECTOR2I( mm( 0.762 ), mm( 1.27 ) ) );
        arrow2->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( arrow2 );

        SCH_SHAPE* stemLine = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( 0 ) ) );
        stemLine->AddPoint( VECTOR2I( mm( 0 ), mm( 2.54 ) ) );
        stemLine->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
        sym->AddDrawItem( stemLine );

        SCH_PIN* pin = new SCH_PIN( sym );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxString::FromUTF8( aKiCadName ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( VECTOR2I( 0, 0 ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_UP );
        sym->AddDrawItem( pin );
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
                    []( unsigned char c ) { return std::toupper( c ); } );

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


void PADS_SCH_SYMBOL_BUILDER::AddHiddenPowerPins(
        LIB_SYMBOL* aSymbol, const std::vector<PARTTYPE_DEF::SIGPIN>& aSigpins )
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

    for( const auto& sp : aSigpins )
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
    case PIN_TYPE::INPUT:           return static_cast<int>( ELECTRICAL_PINTYPE::PT_INPUT );
    case PIN_TYPE::OUTPUT:          return static_cast<int>( ELECTRICAL_PINTYPE::PT_OUTPUT );
    case PIN_TYPE::BIDIRECTIONAL:   return static_cast<int>( ELECTRICAL_PINTYPE::PT_BIDI );
    case PIN_TYPE::TRISTATE:        return static_cast<int>( ELECTRICAL_PINTYPE::PT_TRISTATE );
    case PIN_TYPE::OPEN_COLLECTOR:  return static_cast<int>( ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR );
    case PIN_TYPE::OPEN_EMITTER:    return static_cast<int>( ELECTRICAL_PINTYPE::PT_OPENEMITTER );
    case PIN_TYPE::POWER:           return static_cast<int>( ELECTRICAL_PINTYPE::PT_POWER_IN );
    case PIN_TYPE::PASSIVE:         return static_cast<int>( ELECTRICAL_PINTYPE::PT_PASSIVE );
    case PIN_TYPE::UNSPECIFIED:
    default:                        return static_cast<int>( ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
    }
}


bool PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( const std::string& aName )
{
    // Convert to uppercase for case-insensitive comparison
    std::string upper = aName;
    std::transform( upper.begin(), upper.end(), upper.begin(),
                    []( unsigned char c ) { return std::toupper( c ); } );

    // Check for ground variants
    if( upper == "GND" || upper == "AGND" || upper == "DGND" || upper == "PGND" ||
        upper == "EARTH" || upper == "CHASSIS" || upper == "VSS" || upper == "0V" )
    {
        return true;
    }

    // Check for power supply variants
    if( upper == "VCC" || upper == "VDD" || upper == "VEE" || upper == "VPP" ||
        upper == "VBAT" || upper == "VBUS" || upper == "V+" || upper == "V-" )
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
                    []( unsigned char c ) { return std::toupper( c ); } );

    // Map common power symbol names to KiCad power library symbols
    struct PowerMapping
    {
        const char* padsName;
        const char* kicadSymbol;
    };

    static const PowerMapping mappings[] = {
        { "GND",     "GND" },
        { "AGND",    "GND" },
        { "DGND",    "GNDD" },
        { "PGND",    "GNDPWR" },
        { "EARTH",   "Earth" },
        { "CHASSIS", "Chassis" },
        { "VSS",     "VSS" },
        { "0V",      "GND" },
        { "VCC",     "VCC" },
        { "VDD",     "VDD" },
        { "VEE",     "VEE" },
        { "VPP",     "VPP" },
        { "VBAT",    "VBAT" },
        { "VBUS",    "VBUS" },
        { "V+",      "VCC" },
        { "V-",      "VEE" },
        { "+5V",     "+5V" },
        { "-5V",     "-5V" },
        { "+3V3",    "+3V3" },
        { "+3.3V",   "+3V3" },
        { "+12V",    "+12V" },
        { "-12V",    "-12V" },
        { "+15V",    "+15V" },
        { "-15V",    "-15V" },
        { "+1V8",    "+1V8" },
        { "+2V5",    "+2V5" },
        { "+9V",     "+9V" },
        { "+24V",    "+24V" },
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

    // Generic fallback for +/- prefixed names not in the table
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
