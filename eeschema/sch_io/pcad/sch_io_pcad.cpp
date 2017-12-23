/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2017 Eldar Khayrullin <eldar.khayrullin@mail.ru>
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#include <sch_io/pcad/sch_io_pcad.h>
#include <sch_io/pcad/pcad_sch_parser.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <page_info.h>
#include <pin_type.h>
#include <sch_bus_entry.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <stroke_params.h>
#include <string_utils.h>
#include <title_block.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>
#include <wx/regex.h>

#include <cmath>


namespace
{

using namespace PCAD_SCH;

// P-CAD stroke and TrueType fonts are specified by cell height; the KiCad text
// size is the glyph height.  These ratios come from measuring P-CAD output.
constexpr double STROKE_HEIGHT_TO_SIZE = 0.656;
constexpr double TRUETYPE_HEIGHT_TO_SIZE = 0.585;

constexpr double BUS_ENTRY_SIZE_MILS = 100.0;


// ---------------------------------------------------------------------------
// Coordinate helpers.  P-CAD uses Y-up; KiCad uses Y-down.
// ---------------------------------------------------------------------------

int toIU( double aMils )
{
    return schIUScale.MilsToIU( aMils );
}


int schX( double aPcadX )
{
    return toIU( aPcadX );
}


int schY( double aPcadY, double aPageHeightMils )
{
    return toIU( aPageHeightMils - aPcadY );
}


int symX( double aPcadX )
{
    return toIU( aPcadX );
}


int symY( double aPcadY )
{
    return -toIU( aPcadY );
}


int normalizeDeg( double aRotDeg )
{
    int rot = static_cast<int>( std::round( aRotDeg ) ) % 360;

    if( rot < 0 )
        rot += 360;

    return rot;
}


// P-CAD auto-generated net names are either purely numeric or NETnnnn;
// anything else is a user-assigned name worth a label.
bool isAutoNetName( const wxString& aName )
{
    if( aName.IsEmpty() )
        return true;

    bool allDigits = true;

    for( wxUniChar c : aName )
    {
        if( !wxIsdigit( c ) )
        {
            allDigits = false;
            break;
        }
    }

    if( allDigits )
        return true;

    static wxRegEx autoNet( wxT( "^NET[0-9]+$" ), wxRE_ICASE );

    return autoNet.Matches( aName );
}


bool isAutoBusName( const wxString& aName )
{
    if( aName.IsEmpty() )
        return true;

    static wxRegEx autoBus( wxT( "^BUS[0-9]+$" ), wxRE_ICASE );

    return autoBus.Matches( aName );
}


// ---------------------------------------------------------------------------
// Text formatting
// ---------------------------------------------------------------------------

void applyJustify( EDA_TEXT* aText, JUSTIFY aJustify, bool aFlipped )
{
    GR_TEXT_H_ALIGN_T hAlign;
    GR_TEXT_V_ALIGN_T vAlign;

    switch( aJustify )
    {
    case JUSTIFY::LOWER_LEFT:
    case JUSTIFY::UPPER_LEFT:
    case JUSTIFY::LEFT:
        hAlign = GR_TEXT_H_ALIGN_LEFT;
        break;
    case JUSTIFY::LOWER_RIGHT:
    case JUSTIFY::UPPER_RIGHT:
    case JUSTIFY::RIGHT:
        hAlign = GR_TEXT_H_ALIGN_RIGHT;
        break;
    default:
        hAlign = GR_TEXT_H_ALIGN_CENTER;
        break;
    }

    switch( aJustify )
    {
    case JUSTIFY::LOWER_LEFT:
    case JUSTIFY::LOWER_CENTER:
    case JUSTIFY::LOWER_RIGHT:
        vAlign = GR_TEXT_V_ALIGN_BOTTOM;
        break;
    case JUSTIFY::UPPER_LEFT:
    case JUSTIFY::UPPER_CENTER:
    case JUSTIFY::UPPER_RIGHT:
        vAlign = GR_TEXT_V_ALIGN_TOP;
        break;
    default:
        vAlign = GR_TEXT_V_ALIGN_CENTER;
        break;
    }

    if( aFlipped )
    {
        if( hAlign == GR_TEXT_H_ALIGN_LEFT )
            hAlign = GR_TEXT_H_ALIGN_RIGHT;
        else if( hAlign == GR_TEXT_H_ALIGN_RIGHT )
            hAlign = GR_TEXT_H_ALIGN_LEFT;
    }

    aText->SetHorizJustify( hAlign );
    aText->SetVertJustify( vAlign );
}


// Apply a P-CAD text style (font size, weight, slant) and the per-item
// justification and rotation to a KiCad text object.
void applyTextStyle( EDA_TEXT* aText, const PCAD_SCH::TEXT_ITEM& aItem,
                     const PCAD_SCH::SCHEMATIC& aPcad )
{
    const PCAD_SCH::TEXT_STYLE* style = aPcad.FindTextStyle( aItem.styleRef );

    double heightMils = 100.0;
    double widthMils = 10.0;
    bool   isBold = false;
    bool   isItalic = false;

    if( style )
    {
        const PCAD_SCH::FONT& font = style->EffectiveFont();
        double ratio = font.isTrueType ? TRUETYPE_HEIGHT_TO_SIZE : STROKE_HEIGHT_TO_SIZE;

        heightMils = font.height * ratio;
        widthMils = font.strokeWidth;
        isBold = font.isBold;
        isItalic = font.isItalic;
    }

    int size = toIU( heightMils );

    if( size <= 0 )
        size = toIU( 50.0 );

    aText->SetTextSize( VECTOR2I( size, size ) );

    if( !isBold && widthMils > 0 )
        aText->SetTextThickness( toIU( widthMils ) );

    aText->SetBold( isBold );
    aText->SetItalic( isItalic );
    aText->SetVisible( aItem.isVisible );

    int rot = normalizeDeg( aItem.rotation );

    // KiCad schematic text is horizontal or vertical; 180 and 270 fold onto
    // the base angle with mirrored justification.
    if( rot == 90 || rot == 270 )
        aText->SetTextAngle( EDA_ANGLE( 90.0, DEGREES_T ) );
    else
        aText->SetTextAngle( EDA_ANGLE( 0.0, DEGREES_T ) );

    applyJustify( aText, aItem.justify, aItem.isFlipped );
}


// ---------------------------------------------------------------------------
// Pin mapping
// ---------------------------------------------------------------------------

// Map P-CAD pin rotation to KiCad PIN_ORIENTATION.  P-Cad rotation is the
// direction the pin exits the body toward the connection point (CCW, Y-up).
PIN_ORIENTATION pinOrientation( double aRotDeg )
{
    switch( normalizeDeg( aRotDeg ) )
    {
    case 90:  return PIN_ORIENTATION::PIN_DOWN;
    case 180: return PIN_ORIENTATION::PIN_RIGHT;
    case 270: return PIN_ORIENTATION::PIN_UP;
    default:  return PIN_ORIENTATION::PIN_LEFT;   // 0 degrees
    }
}


ELECTRICAL_PINTYPE mapPinType( const wxString& aPcadType )
{
    // "Pasive" is the historical P-CAD spelling; Altium exports write "Passive".
    if( aPcadType == wxT( "Passive" ) || aPcadType == wxT( "Pasive" ) )
        return ELECTRICAL_PINTYPE::PT_PASSIVE;
    if( aPcadType == wxT( "Input" ) )
        return ELECTRICAL_PINTYPE::PT_INPUT;
    if( aPcadType == wxT( "Output" ) )
        return ELECTRICAL_PINTYPE::PT_OUTPUT;
    if( aPcadType == wxT( "Bidirectional" ) )
        return ELECTRICAL_PINTYPE::PT_BIDI;
    if( aPcadType == wxT( "Power" ) )
        return ELECTRICAL_PINTYPE::PT_POWER_IN;
    if( aPcadType == wxT( "ThreeState" ) )
        return ELECTRICAL_PINTYPE::PT_TRISTATE;
    if( aPcadType == wxT( "OpenH" ) )
        return ELECTRICAL_PINTYPE::PT_OPENEMITTER;
    if( aPcadType == wxT( "OpenL" ) )
        return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;

    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}


GRAPHIC_PINSHAPE mapPinShape( const PIN& aPin )
{
    bool inverted = ( aPin.outsideEdgeStyle == wxT( "Dot" ) );
    bool clock = ( aPin.outsideEdgeStyle == wxT( "Clock" )
                   || aPin.insideEdgeStyle == wxT( "Clock" ) );

    if( inverted && clock )
        return GRAPHIC_PINSHAPE::INVERTED_CLOCK;

    if( inverted )
        return GRAPHIC_PINSHAPE::INVERTED;

    if( clock )
        return GRAPHIC_PINSHAPE::CLOCK;

    return GRAPHIC_PINSHAPE::LINE;
}


// ---------------------------------------------------------------------------
// IEEE symbol synthesis.  P-CAD qualifier symbols have no KiCad primitive;
// draw them as polylines scaled by the symbol height.
// ---------------------------------------------------------------------------

const VECTOR2D IEEE_BREAK( -1e9, -1e9 );

std::vector<VECTOR2D> ieeePoints( const IEEE_SYMBOL& aSym )
{
    const double h = aSym.height;
    const double half = h / 2.0;
    const double quarter = h / 4.0;
    const double third = h / 3.0;

    switch( aSym.kind )
    {
    case IEEE_KIND::ADDER:
        return { { h, h - quarter }, { h, h }, { 0, h }, { half, half }, { 0, 0 }, { h, 0 },
                 { h, quarter } };

    case IEEE_KIND::AMPLIFIER:
        return { { 0, 0 }, { h, half }, { 0, h }, { 0, 0 } };

    case IEEE_KIND::ASTABLE:
        return { { 0, 0 }, { half, 0 }, { half, h }, { h, h }, { h, 0 }, { h + half, 0 },
                 { h + half, h }, { 2 * h, h }, { 2 * h, 0 }, { 2 * h + half, 0 } };

    case IEEE_KIND::COMPLEX:
        return { { 0, 0 }, { h, 0 }, IEEE_BREAK, { half, 0 }, { half, h }, IEEE_BREAK,
                 { 0, h }, { h, h } };

    case IEEE_KIND::GENERATOR:
        return { { 0, 0 }, { half, 0 }, { half, h }, { h, h }, { h, 0 }, { h + half, 0 } };

    case IEEE_KIND::HYSTERESIS:
        return { { 0, 0 }, { half + h, 0 }, { half + h, h }, { half, h }, { half, 0 },
                 IEEE_BREAK, { half + h, h }, { 2 * h, h } };

    case IEEE_KIND::MULTIPLIER:
        return { { 0, h }, { h, h }, IEEE_BREAK, { third, 0 }, { third, h }, IEEE_BREAK,
                 { 2 * third, 0 }, { 2 * third, h } };

    default:
        return {};
    }
}


VECTOR2D ieeeTransform( const VECTOR2D& aPoint, const IEEE_SYMBOL& aSym )
{
    double rad = aSym.rotation * M_PI / 180.0;
    double x = aPoint.x * std::cos( rad ) - aPoint.y * std::sin( rad );
    double y = aPoint.x * std::sin( rad ) + aPoint.y * std::cos( rad );

    if( aSym.isFlipped )
        x = -x;

    return { x + aSym.x, y + aSym.y };
}


// Emit an IEEE symbol as polyline shapes through a caller-provided
// point transform (symbol space or sheet space).
template <typename XFORM>
std::vector<SCH_SHAPE*> buildIeeeShapes( const IEEE_SYMBOL& aSym, int aWidthIU, SCH_LAYER_ID aLayer,
                                         XFORM aXform )
{
    std::vector<SCH_SHAPE*> shapes;
    std::vector<VECTOR2D>   points = ieeePoints( aSym );

    if( points.empty() )
        return shapes;

    SCH_SHAPE* poly = nullptr;

    for( const VECTOR2D& pt : points )
    {
        if( pt == IEEE_BREAK )
        {
            poly = nullptr;
            continue;
        }

        VECTOR2D world = ieeeTransform( pt, aSym );

        if( !poly )
        {
            poly = new SCH_SHAPE( SHAPE_T::POLY, aLayer );
            poly->SetStroke( STROKE_PARAMS( aWidthIU, LINE_STYLE::SOLID ) );
            poly->SetFillMode( FILL_T::NO_FILL );
            shapes.push_back( poly );
        }

        poly->AddPoint( aXform( world.x, world.y ) );
    }

    // A Complex qualifier also carries a small circle at its centre.
    if( aSym.kind == IEEE_KIND::COMPLEX )
    {
        IEEE_SYMBOL centre = aSym;
        VECTOR2D    c = ieeeTransform( { aSym.height / 2.0, aSym.height / 2.0 }, centre );

        auto* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, aLayer );
        circle->SetStroke( STROKE_PARAMS( aWidthIU, LINE_STYLE::SOLID ) );
        circle->SetFillMode( FILL_T::NO_FILL );
        circle->SetCenter( aXform( c.x, c.y ) );
        circle->SetEnd( aXform( c.x + aSym.height / 8.0, c.y ) );
        shapes.push_back( circle );
    }

    return shapes;
}


LINE_STYLE mapLineStyle( LINE_KIND aKind )
{
    switch( aKind )
    {
    case LINE_KIND::DASHED: return LINE_STYLE::DASH;
    case LINE_KIND::DOTTED: return LINE_STYLE::DOT;
    default:                return LINE_STYLE::SOLID;
    }
}


// ---------------------------------------------------------------------------
// LIB_SYMBOL construction
// ---------------------------------------------------------------------------

const PCAD_SCH::SYMBOL_DEF* findSymbolDef( const PCAD_SCH::SCHEMATIC& aPcad,
                                           const wxString& aName )
{
    auto it = aPcad.symbolDefsByName.find( aName );

    if( it != aPcad.symbolDefsByName.end() )
        return it->second;

    for( const SYMBOL_DEF& sd : aPcad.symbolDefs )
    {
        if( sd.originalName == aName )
            return &sd;
    }

    return nullptr;
}


const PCAD_SCH::COMP_PIN* findCompPin( const PCAD_SCH::COMP_DEF& aDef, int aPartNum,
                                       const wxString& aSymPinNum )
{
    for( const COMP_PIN& cp : aDef.compPins )
    {
        if( cp.partNum == aPartNum && cp.symPinNum == aSymPinNum )
            return &cp;
    }

    return nullptr;
}


// Add one symbolDef's geometry and pins to a LIB_SYMBOL as the given unit.
// aCompDef may be null when building directly from a bare symbolDef.
void addSymbolDefToLibSymbol( LIB_SYMBOL* aSymbol, const PCAD_SCH::SYMBOL_DEF& aDef,
                              const PCAD_SCH::COMP_DEF* aCompDef, int aUnit,
                              const PCAD_SCH::SCHEMATIC& aPcad,
                              bool* aShowPinDes, bool* aShowPinName )
{
    const int defaultWidth = toIU( 10.0 );

    for( const LINE& ln : aDef.lines )
    {
        if( ln.pts.size() < 2 )
            continue;

        auto* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        for( const auto& [x, y] : ln.pts )
            shape->AddPoint( VECTOR2I( symX( x ), symY( y ) ) );

        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( ln.width > 0 ? toIU( ln.width ) : defaultWidth,
                                         mapLineStyle( ln.style ) ) );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape );
    }

    for( const ARC& arc : aDef.arcs )
    {
        auto* shape = new SCH_SHAPE( arc.sweepAngle >= 360.0 ? SHAPE_T::CIRCLE : SHAPE_T::ARC,
                                     LAYER_DEVICE );
        int      radius = toIU( arc.radius );
        VECTOR2I center( symX( arc.x ), symY( arc.y ) );

        if( shape->GetShape() == SHAPE_T::CIRCLE )
        {
            shape->SetCenter( center );
            shape->SetEnd( center + VECTOR2I( radius, 0 ) );
        }
        else
        {
            // Y-flip mirrors angles
            double startRad = -arc.startAngle * M_PI / 180.0;
            VECTOR2I start( center.x + KiROUND( radius * std::cos( startRad ) ),
                            center.y + KiROUND( radius * std::sin( startRad ) ) );
            shape->SetCenter( center );
            shape->SetStart( start );
            shape->SetArcAngleAndEnd( EDA_ANGLE( -arc.sweepAngle, DEGREES_T ) );
        }

        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( arc.width > 0 ? toIU( arc.width ) : defaultWidth,
                                         LINE_STYLE::SOLID ) );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape );
    }

    for( const POLY& poly : aDef.polys )
    {
        if( poly.pts.size() < 3 )
            continue;

        auto* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

        for( const auto& [x, y] : poly.pts )
            shape->AddPoint( VECTOR2I( symX( x ), symY( y ) ) );

        // close the outline
        shape->AddPoint( VECTOR2I( symX( poly.pts[0].first ), symY( poly.pts[0].second ) ) );
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
        shape->SetStroke( STROKE_PARAMS( defaultWidth, LINE_STYLE::SOLID ) );
        shape->SetUnit( aUnit );
        aSymbol->AddDrawItem( shape );
    }

    for( const TEXT_ITEM& text : aDef.texts )
    {
        if( text.text.IsEmpty() )
            continue;

        auto* schText = new SCH_TEXT( VECTOR2I( symX( text.x ), symY( text.y ) ), text.text,
                                      LAYER_DEVICE );
        applyTextStyle( schText, text, aPcad );
        schText->SetUnit( aUnit );
        aSymbol->AddDrawItem( schText );
    }

    for( const IEEE_SYMBOL& ieee : aDef.ieeeSymbols )
    {
        for( SCH_SHAPE* shape : buildIeeeShapes( ieee, defaultWidth, LAYER_DEVICE,
                                                 []( double x, double y )
                                                 {
                                                     return VECTOR2I( symX( x ), symY( y ) );
                                                 } ) )
        {
            shape->SetUnit( aUnit );
            aSymbol->AddDrawItem( shape );
        }
    }

    const int pinTextSize = toIU( 50.0 );

    for( const PIN& pin : aDef.pins )
    {
        // The wire connection point is the body attachment displaced by the
        // pin length along the exit direction.
        int    rot = normalizeDeg( pin.rotation );
        double extX = pin.x;
        double extY = pin.y;

        switch( rot )
        {
        case 0:   extX += pin.pinLength; break;
        case 90:  extY += pin.pinLength; break;
        case 180: extX -= pin.pinLength; break;
        case 270: extY -= pin.pinLength; break;
        default:
            extX += pin.pinLength * std::cos( rot * M_PI / 180.0 );
            extY += pin.pinLength * std::sin( rot * M_PI / 180.0 );
            break;
        }

        auto* schPin = new SCH_PIN( aSymbol );

        const COMP_PIN* compPin = aCompDef ? findCompPin( *aCompDef, aUnit, pin.pinNum )
                                           : nullptr;

        wxString number;
        wxString name;

        if( compPin )
        {
            number = compPin->padDes;
            name = compPin->pinName;
        }

        if( number.IsEmpty() )
            number = !pin.defaultPinDes.IsEmpty() ? pin.defaultPinDes : pin.pinDesText.text;

        if( number.IsEmpty() )
            number = pin.pinNum;

        if( name.IsEmpty() )
            name = pin.pinNameText.text;

        schPin->SetNumber( number );
        schPin->SetName( name );
        schPin->SetPosition( VECTOR2I( symX( extX ), symY( extY ) ) );
        schPin->SetLength( toIU( pin.pinLength ) );
        schPin->SetOrientation( pinOrientation( pin.rotation ) );
        schPin->SetShape( mapPinShape( pin ) );
        schPin->SetUnit( aUnit );

        if( aCompDef && aCompDef->isPower )
        {
            schPin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            schPin->SetVisible( false );
        }
        else
        {
            schPin->SetType( compPin ? mapPinType( compPin->pinType )
                                     : ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
        }

        const PCAD_SCH::TEXT_STYLE* desStyle = aPcad.FindTextStyle( pin.pinDesText.styleRef );
        const PCAD_SCH::TEXT_STYLE* nameStyle = aPcad.FindTextStyle( pin.pinNameText.styleRef );

        auto styleSize =
                []( const PCAD_SCH::TEXT_STYLE* aStyle, int aDefault )
                {
                    if( !aStyle )
                        return aDefault;

                    const PCAD_SCH::FONT& font = aStyle->EffectiveFont();
                    double ratio = font.isTrueType ? TRUETYPE_HEIGHT_TO_SIZE
                                                   : STROKE_HEIGHT_TO_SIZE;
                    int    size = toIU( font.height * ratio );

                    return size > 0 ? size : aDefault;
                };

        schPin->SetNumberTextSize( styleSize( desStyle, pinTextSize ) );
        schPin->SetNameTextSize( styleSize( nameStyle, pinTextSize ) );

        if( aShowPinDes )
            *aShowPinDes |= pin.showPinDes;

        if( aShowPinName )
            *aShowPinName |= pin.showPinName;

        aSymbol->AddDrawItem( schPin );
    }

    // Reference and value placement from (attr "RefDes" ...) and (attr "Type" ...).
    // Only the first unit positions the shared fields.
    if( aUnit == 1 )
    {
        for( const ATTR& attr : aDef.attrs )
        {
            SCH_FIELD* field = nullptr;

            if( attr.name == wxT( "RefDes" ) )
                field = &aSymbol->GetReferenceField();
            else if( attr.name == wxT( "Type" ) || attr.name == wxT( "Value" ) )
                field = &aSymbol->GetValueField();

            if( field )
            {
                field->SetPosition( VECTOR2I( symX( attr.placement.x ),
                                              symY( attr.placement.y ) ) );
                applyTextStyle( field, attr.placement, aPcad );
                field->SetVisible( attr.placement.isVisible );
            }
        }
    }
}


wxString libSymbolName( const PCAD_SCH::COMP_DEF& aDef )
{
    wxString name = !aDef.originalName.IsEmpty() ? aDef.originalName : aDef.name;
    name.Replace( wxT( " " ), wxT( "_" ) );
    return name;
}


// Build a complete LIB_SYMBOL (all units) from a component definition.
LIB_SYMBOL* buildLibSymbolFromCompDef( const PCAD_SCH::COMP_DEF& aDef,
                                       const PCAD_SCH::SCHEMATIC& aPcad,
                                       const wxString& aLibName )
{
    auto* sym = new LIB_SYMBOL( libSymbolName( aDef ) );

    sym->SetLibId( LIB_ID( aLibName, libSymbolName( aDef ) ) );
    sym->SetUnitCount( aDef.numParts > 0 ? aDef.numParts : 1, false );

    bool showPinDes = false;
    bool showPinName = false;

    for( int unit = 1; unit <= aDef.numParts; unit++ )
    {
        wxString symName;

        if( unit < static_cast<int>( aDef.attachedSymbols.size() ) )
            symName = aDef.attachedSymbols[unit];

        const SYMBOL_DEF* symDef = symName.IsEmpty() ? nullptr
                                                     : findSymbolDef( aPcad, symName );

        if( symDef )
            addSymbolDefToLibSymbol( sym, *symDef, &aDef, unit, aPcad, &showPinDes,
                                     &showPinName );
    }

    sym->SetShowPinNumbers( showPinDes );
    sym->SetShowPinNames( showPinName );

    if( !aDef.refDesPrefix.IsEmpty() )
        sym->GetReferenceField().SetText( aDef.refDesPrefix );

    if( !aDef.description.IsEmpty() )
        sym->SetDescription( aDef.description );

    if( !aDef.attachedPattern.IsEmpty() )
        sym->GetFootprintField().SetText( aDef.attachedPattern );

    if( aDef.isPower )
    {
        sym->SetGlobalPower();
        sym->GetReferenceField().SetText( wxT( "#PWR" ) );
        sym->GetReferenceField().SetVisible( false );
    }

    return sym;
}


// Fallback for files that carry symbolDefs without component definitions.
LIB_SYMBOL* buildLibSymbolFromSymbolDef( const PCAD_SCH::SYMBOL_DEF& aDef,
                                         const PCAD_SCH::SCHEMATIC& aPcad,
                                         const wxString& aLibName )
{
    wxString name = !aDef.originalName.IsEmpty() ? aDef.originalName : aDef.name;
    name.Replace( wxT( " " ), wxT( "_" ) );

    auto* sym = new LIB_SYMBOL( name );
    sym->SetLibId( LIB_ID( aLibName, name ) );

    bool showPinDes = false;
    bool showPinName = false;

    addSymbolDefToLibSymbol( sym, aDef, nullptr, 1, aPcad, &showPinDes, &showPinName );

    sym->SetShowPinNumbers( showPinDes );
    sym->SetShowPinNames( showPinName );

    return sym;
}


// ---------------------------------------------------------------------------
// Sheet items
// ---------------------------------------------------------------------------

// Map P-CAD symbol instance rotation to KiCad orientation (both CCW once the
// Y-flip is applied).
SYMBOL_ORIENTATION_T symOrientation( double aRotDeg )
{
    switch( normalizeDeg( aRotDeg ) )
    {
    case 90:  return SYM_ORIENT_90;
    case 180: return SYM_ORIENT_180;
    case 270: return SYM_ORIENT_270;
    default:  return SYM_ORIENT_0;
    }
}


SPIN_STYLE spinFromRotation( double aRotDeg, bool aFlipped, bool aVertical )
{
    int rot = normalizeDeg( aRotDeg );

    if( rot == 0 && aVertical )
        rot = 90;

    SPIN_STYLE spin = SPIN_STYLE::RIGHT;

    switch( rot )
    {
    case 90:  spin = SPIN_STYLE::UP; break;
    case 180: spin = SPIN_STYLE::LEFT; break;
    case 270: spin = SPIN_STYLE::BOTTOM; break;
    default:  spin = SPIN_STYLE::RIGHT; break;
    }

    if( aFlipped )
    {
        if( spin == SPIN_STYLE::RIGHT )
            spin = SPIN_STYLE::LEFT;
        else if( spin == SPIN_STYLE::LEFT )
            spin = SPIN_STYLE::RIGHT;
    }

    return spin;
}


LABEL_FLAG_SHAPE mapPortShape( const wxString& aPortType )
{
    if( aPortType.Contains( wxT( "Dbl" ) ) )
        return LABEL_FLAG_SHAPE::L_BIDI;

    if( aPortType.StartsWith( wxT( "LeftAngle" ) ) || aPortType.StartsWith( wxT( "RightAngle" ) ) )
        return LABEL_FLAG_SHAPE::L_INPUT;

    return LABEL_FLAG_SHAPE::L_UNSPECIFIED;
}

} // anonymous namespace


// ---------------------------------------------------------------------------
// SCH_IO_PCAD
// ---------------------------------------------------------------------------

SCH_IO_PCAD::SCH_IO_PCAD() : SCH_IO( wxS( "P-CAD" ) )
{
}


SCH_IO_PCAD::~SCH_IO_PCAD()
{
}


bool SCH_IO_PCAD::CanReadSchematicFile( const wxString& aFileName ) const
{
    wxFileName fn( aFileName );

    if( fn.GetExt().Upper() != wxT( "SCH" ) )
        return false;

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        return false;

    char line[16];
    bool ok = ( fgets( line, sizeof( line ), fp ) != nullptr )
              && ( memcmp( line, "ACCEL_ASCII", 11 ) == 0 );
    fclose( fp );
    return ok;
}


wxString SCH_IO_PCAD::getLibName( const ::SCHEMATIC* aSchematic, const wxString& aFileName )
{
    wxString libName;

    if( aSchematic )
        libName = aSchematic->Project().GetProjectName();

    if( libName.IsEmpty() )
        libName = wxFileName( aFileName ).GetName();

    if( libName.IsEmpty() )
        libName = wxT( "noname" );

    libName += wxT( "-pcad-import" );

    return LIB_ID::FixIllegalChars( libName, true ).wx_str();
}


void SCH_IO_PCAD::populateScreen( SCH_SCREEN* aScreen, const PCAD_SCH::SHEET& aSheet,
                                  const PCAD_SCH::SCHEMATIC& aPcad, double aPageH,
                                  const std::map<wxString, LIB_SYMBOL*>& aLibSymbols,
                                  const wxString& aLibName )
{
    auto xform =
            [aPageH]( double x, double y )
            {
                return VECTOR2I( schX( x ), schY( y, aPageH ) );
            };

    // Wires; a P-CAD wire line may be a polyline.
    for( const WIRE& w : aSheet.wires )
    {
        for( size_t i = 0; i + 1 < w.pts.size(); i++ )
        {
            auto* line = new SCH_LINE( xform( w.pts[i].first, w.pts[i].second ), LAYER_WIRE );
            line->SetEndPoint( xform( w.pts[i + 1].first, w.pts[i + 1].second ) );
            aScreen->Append( line );
        }
    }

    // Buses
    for( const BUS& b : aSheet.buses )
    {
        for( size_t i = 0; i + 1 < b.pts.size(); i++ )
        {
            auto* line = new SCH_LINE( xform( b.pts[i].first, b.pts[i].second ), LAYER_BUS );
            line->SetEndPoint( xform( b.pts[i + 1].first, b.pts[i + 1].second ) );
            aScreen->Append( line );
        }

        if( ( b.dispName || !isAutoBusName( b.name ) ) && !b.name.IsEmpty() )
        {
            VECTOR2I pos;
            const TEXT_ITEM& label = b.label;

            if( label.x != 0 || label.y != 0 )
                pos = xform( label.x, label.y );
            else
                pos = xform( b.pts[0].first, b.pts[0].second );

            auto* schLabel = new SCH_LABEL( pos, b.name );
            applyTextStyle( schLabel, label, aPcad );
            schLabel->SetSpinStyle( spinFromRotation( label.rotation, label.isFlipped, false ) );
            aScreen->Append( schLabel );
        }
    }

    // Bus entries.  The P-CAD node holds the wire-side end and the direction
    // toward the bus; the diagonal's vertical slant is chosen so the entry
    // lands on a segment of the referenced bus when one can be found.
    for( const BUS_ENTRY& e : aSheet.busEntries )
    {
        VECTOR2I pos = xform( e.x, e.y );
        int      step = toIU( BUS_ENTRY_SIZE_MILS );
        VECTOR2I size;

        if( e.orient == wxT( "Left" ) )
            size = VECTOR2I( -step, step );
        else if( e.orient == wxT( "Right" ) )
            size = VECTOR2I( step, step );
        else if( e.orient == wxT( "Up" ) )
            size = VECTOR2I( step, -step );
        else
            size = VECTOR2I( step, step );

        const BUS* bus = nullptr;

        for( const BUS& b : aSheet.buses )
        {
            if( b.name == e.busNameRef )
            {
                bus = &b;
                break;
            }
        }

        if( bus )
        {
            // Try both slants; prefer the one whose far end touches the bus.
            auto touchesBus =
                    [&]( const VECTOR2I& aEnd ) -> bool
                    {
                        for( size_t i = 0; i + 1 < bus->pts.size(); i++ )
                        {
                            VECTOR2I a = xform( bus->pts[i].first, bus->pts[i].second );
                            VECTOR2I c = xform( bus->pts[i + 1].first, bus->pts[i + 1].second );

                            if( a.x == c.x && aEnd.x == a.x
                                && aEnd.y >= std::min( a.y, c.y ) && aEnd.y <= std::max( a.y, c.y ) )
                                return true;

                            if( a.y == c.y && aEnd.y == a.y
                                && aEnd.x >= std::min( a.x, c.x ) && aEnd.x <= std::max( a.x, c.x ) )
                                return true;
                        }

                        return false;
                    };

            VECTOR2I altSize( size.x, -size.y );

            if( !touchesBus( pos + size ) && touchesBus( pos + altSize ) )
                size = altSize;
        }

        auto* entry = new SCH_BUS_WIRE_ENTRY( pos );
        entry->SetSize( size );
        aScreen->Append( entry );
    }

    // Junctions
    for( const JUNCTION& j : aSheet.junctions )
        aScreen->Append( new SCH_JUNCTION( xform( j.x, j.y ) ) );

    // Ports: P-CAD ports connect nets across sheets, which is KiCad global
    // label semantics.
    for( const PORT& p : aSheet.ports )
    {
        if( p.netNameRef.IsEmpty() )
            continue;

        auto* label = new SCH_GLOBALLABEL( xform( p.x, p.y ), p.netNameRef );
        label->SetShape( mapPortShape( p.portType ) );
        label->SetSpinStyle( spinFromRotation( p.rotation, p.isFlipped,
                                               p.portType.EndsWith( wxT( "_Vert" ) ) ) );
        label->SetTextSize( VECTOR2I( toIU( 50.0 ), toIU( 50.0 ) ) );
        aScreen->Append( label );
    }

    // Wire net-name labels where P-CAD displays them
    std::set<wxString> labelledNets;

    for( const WIRE& w : aSheet.wires )
    {
        if( !w.dispName || w.netName.IsEmpty() || w.pts.empty() )
            continue;

        VECTOR2I pos;

        if( w.label.x != 0 || w.label.y != 0 )
            pos = xform( w.label.x, w.label.y );
        else
            pos = xform( w.pts[0].first, w.pts[0].second );

        auto* label = new SCH_LABEL( pos, w.netName );
        applyTextStyle( label, w.label, aPcad );
        label->SetSpinStyle( spinFromRotation( w.label.rotation, w.label.isFlipped, false ) );
        aScreen->Append( label );

        labelledNets.insert( w.netName );
    }

    // Preserve user-assigned net names that are never displayed: place a label
    // at the first junction of the net.  Auto-generated names are dropped.
    for( const JUNCTION& j : aSheet.junctions )
    {
        if( j.netName.IsEmpty() || isAutoNetName( j.netName ) )
            continue;

        if( labelledNets.count( j.netName ) )
            continue;

        auto* label = new SCH_LABEL( xform( j.x, j.y ), j.netName );
        label->SetTextSize( VECTOR2I( toIU( 50.0 ), toIU( 50.0 ) ) );
        aScreen->Append( label );

        labelledNets.insert( j.netName );
    }

    // Free text
    for( const TEXT_ITEM& t : aSheet.texts )
    {
        if( t.text.IsEmpty() )
            continue;

        auto* text = new SCH_TEXT( xform( t.x, t.y ), t.text );
        applyTextStyle( text, t, aPcad );
        aScreen->Append( text );
    }

    // Sheet graphics
    for( const LINE& ln : aSheet.lines )
    {
        if( ln.pts.size() < 2 )
            continue;

        auto* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES );

        for( const auto& [x, y] : ln.pts )
            shape->AddPoint( xform( x, y ) );

        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( toIU( ln.width ), mapLineStyle( ln.style ) ) );
        aScreen->Append( shape );
    }

    for( const ARC& arc : aSheet.arcs )
    {
        auto* shape = new SCH_SHAPE( arc.sweepAngle >= 360.0 ? SHAPE_T::CIRCLE : SHAPE_T::ARC,
                                     LAYER_NOTES );
        int      radius = toIU( arc.radius );
        VECTOR2I center = xform( arc.x, arc.y );

        if( shape->GetShape() == SHAPE_T::CIRCLE )
        {
            shape->SetCenter( center );
            shape->SetEnd( center + VECTOR2I( radius, 0 ) );
        }
        else
        {
            double startRad = -arc.startAngle * M_PI / 180.0;
            VECTOR2I start( center.x + KiROUND( radius * std::cos( startRad ) ),
                            center.y + KiROUND( radius * std::sin( startRad ) ) );
            shape->SetCenter( center );
            shape->SetStart( start );
            shape->SetArcAngleAndEnd( EDA_ANGLE( -arc.sweepAngle, DEGREES_T ) );
        }

        shape->SetFillMode( FILL_T::NO_FILL );
        shape->SetStroke( STROKE_PARAMS( toIU( arc.width ), LINE_STYLE::SOLID ) );
        aScreen->Append( shape );
    }

    for( const POLY& poly : aSheet.polys )
    {
        auto* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES );

        for( const auto& [x, y] : poly.pts )
            shape->AddPoint( xform( x, y ) );

        shape->AddPoint( xform( poly.pts[0].first, poly.pts[0].second ) );
        shape->SetFillMode( FILL_T::FILLED_SHAPE );
        shape->SetStroke( STROKE_PARAMS( toIU( 10.0 ), LINE_STYLE::SOLID ) );
        aScreen->Append( shape );
    }

    for( const IEEE_SYMBOL& ieee : aSheet.ieeeSymbols )
    {
        for( SCH_SHAPE* shape : buildIeeeShapes( ieee, toIU( 10.0 ), LAYER_NOTES, xform ) )
            aScreen->Append( shape );
    }

    // Component instances
    for( const SYMBOL_INST& inst : aSheet.symbols )
    {
        const COMP_INST* compInst = nullptr;
        const COMP_DEF*  compDef = nullptr;

        auto ciIt = aPcad.compInstsByRef.find( inst.refDesRef );

        if( ciIt != aPcad.compInstsByRef.end() )
        {
            compInst = ciIt->second;

            wxString compRef = compInst->compRef;
            auto     aliasIt = aPcad.compAliases.find( compRef );

            if( aliasIt != aPcad.compAliases.end() )
                compRef = aliasIt->second;

            auto cdIt = aPcad.compDefsByName.find( compRef );

            if( cdIt != aPcad.compDefsByName.end() )
            {
                compDef = cdIt->second;
            }
            else
            {
                // compRef may address the definition by original name
                for( const COMP_DEF& cd : aPcad.compDefs )
                {
                    if( cd.originalName == compRef )
                    {
                        compDef = &cd;
                        break;
                    }
                }
            }
        }

        wxString libKey;

        if( compDef )
        {
            libKey = libSymbolName( *compDef );
        }
        else
        {
            // No netlist entry: fall back to the raw symbolDef
            const SYMBOL_DEF* symDef = findSymbolDef( aPcad, inst.symbolRef );

            if( symDef )
            {
                libKey = !symDef->originalName.IsEmpty() ? symDef->originalName : symDef->name;
                libKey.Replace( wxT( " " ), wxT( "_" ) );
            }
        }

        auto libIt = aLibSymbols.find( libKey );

        if( libIt == aLibSymbols.end() )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format(
                        _( "Symbol definition '%s' for '%s' not found." ),
                        inst.symbolRef, inst.refDesRef ), RPT_SEVERITY_WARNING );
            }

            continue;
        }

        const LIB_SYMBOL* libSym = libIt->second;

        auto* symbol = new SCH_SYMBOL();
        symbol->SetLibId( LIB_ID( aLibName, libKey ) );
        symbol->SetUnit( inst.partNum );
        symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );
        symbol->SetPosition( xform( inst.x, inst.y ) );
        symbol->SetOrientation( symOrientation( inst.rotation ) );

        if( inst.isFlipped )
            symbol->SetMirrorY( true );

        // Initialise field positions from the library symbol
        symbol->UpdateFields( nullptr,
                              true,  /* update style */
                              false, /* update ref text */
                              false, /* update other field text */
                              false, /* reset ref */
                              false  /* reset other fields */ );

        // Per-instance attribute placements override the library defaults
        for( const ATTR& attr : inst.attrs )
        {
            SCH_FIELD* field = nullptr;

            if( attr.name == wxT( "RefDes" ) )
                field = symbol->GetField( FIELD_T::REFERENCE );
            else if( attr.name == wxT( "Value" ) || attr.name == wxT( "Type" ) )
                field = symbol->GetField( FIELD_T::VALUE );

            if( field && ( attr.placement.x != 0 || attr.placement.y != 0 ) )
            {
                field->SetPosition( xform( attr.placement.x, attr.placement.y ) );
                applyTextStyle( field, attr.placement, aPcad );
                field->SetVisible( attr.placement.isVisible );
            }
        }

        if( compInst )
        {
            symbol->GetField( FIELD_T::REFERENCE )->SetText( compInst->refDes );

            wxString val = !compInst->value.IsEmpty() ? compInst->value
                                                      : compInst->originalName;

            if( !val.IsEmpty() )
                symbol->GetField( FIELD_T::VALUE )->SetText( val );
        }
        else
        {
            symbol->GetField( FIELD_T::REFERENCE )->SetText( inst.refDesRef );
        }

        if( compDef )
        {
            if( !compDef->attachedPattern.IsEmpty() )
                symbol->GetField( FIELD_T::FOOTPRINT )->SetText( compDef->attachedPattern );

            if( compDef->isPower )
                symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );
        }

        aScreen->Append( symbol );
    }
}


SCH_SHEET* SCH_IO_PCAD::LoadSchematicFile( const wxString& aFileName, ::SCHEMATIC* aSchematic,
                                           SCH_SHEET*             aAppendToMe,
                                           const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic != nullptr );

    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aFileName, pcad );

    // --- Root sheet -----------------------------------------------------------

    SCH_SHEET* rootSheet = nullptr;

    wxFileName newFilename( aFileName );
    newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxT( "Can't append to a schematic with no root!" ) );
        rootSheet = aAppendToMe;
    }
    else
    {
        // The content root carries a real UUID so SetTopLevelSheets() accepts
        // it as a top-level sheet; a nil UUID marks the virtual root, which it
        // skips.
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( newFilename.GetFullPath() );

        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( newFilename.GetFullPath() );
        rootSheet->SetScreen( screen );

        aSchematic->SetTopLevelSheets( { rootSheet } );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( newFilename.GetFullPath() );
        rootSheet->SetScreen( screen );
    }

    // --- Page setup -----------------------------------------------------------

    PAGE_INFO page;
    double    wIn = pcad.workspaceWidth / 1000.0;
    double    hIn = pcad.workspaceHeight / 1000.0;

    if( std::abs( wIn - 17.0 ) < 0.5 && std::abs( hIn - 11.0 ) < 0.5 )
    {
        page.SetType( wxT( "B" ) );
    }
    else if( std::abs( wIn - 11.0 ) < 0.5 && std::abs( hIn - 8.5 ) < 0.5 )
    {
        page.SetType( wxT( "A" ) );
    }
    else
    {
        page.SetType( wxT( "User" ) );
        page.SetWidthMils( KiROUND( pcad.workspaceWidth ) );
        page.SetHeightMils( KiROUND( pcad.workspaceHeight ) );
    }

    const double pageH = pcad.workspaceHeight;

    // --- Library symbols -------------------------------------------------------

    const wxString libName = getLibName( aSchematic, aFileName );

    std::map<wxString, LIB_SYMBOL*> libSymbols;

    for( const PCAD_SCH::COMP_DEF& cd : pcad.compDefs )
    {
        wxString key = libSymbolName( cd );

        if( !libSymbols.count( key ) )
            libSymbols[key] = buildLibSymbolFromCompDef( cd, pcad, libName );
    }

    // symbolDefs never referenced by a compDef still need a library symbol so
    // netlist-less files import.
    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
    {
        wxString key = !sd.originalName.IsEmpty() ? sd.originalName : sd.name;
        key.Replace( wxT( " " ), wxT( "_" ) );

        bool referenced = false;

        for( const PCAD_SCH::COMP_DEF& cd : pcad.compDefs )
        {
            for( const wxString& attached : cd.attachedSymbols )
            {
                if( attached == sd.name || attached == sd.originalName )
                {
                    referenced = true;
                    break;
                }
            }

            if( referenced )
                break;
        }

        if( !referenced && !libSymbols.count( key ) )
            libSymbols[key] = buildLibSymbolFromSymbolDef( sd, pcad, libName );
    }

    // --- Sheets ----------------------------------------------------------------

    std::vector<SCH_SCREEN*> screens;

    if( pcad.sheets.empty() )
    {
        screens.push_back( rootSheet->GetScreen() );
    }
    else
    {
        // First P-CAD sheet lands on the root; further sheets become subsheets.
        screens.push_back( rootSheet->GetScreen() );

        if( !pcad.sheets[0].name.IsEmpty() )
            rootSheet->SetName( pcad.sheets[0].name );

        const int sheetSymbolStep = toIU( 1500.0 );
        int       sheetIndex = 1;

        for( size_t i = 1; i < pcad.sheets.size(); i++ )
        {
            const PCAD_SCH::SHEET& psheet = pcad.sheets[i];

            auto* subSheet = new SCH_SHEET( aSchematic );
            auto* subScreen = new SCH_SCREEN( aSchematic );

            wxString sheetName = psheet.name;

            if( sheetName.IsEmpty() )
                sheetName = wxString::Format( wxT( "Sheet%d" ), psheet.sheetNum );

            wxFileName subFilename( newFilename );
            wxString   fileBase = sheetName;
            fileBase.Replace( wxT( " " ), wxT( "_" ) );
            subFilename.SetName( subFilename.GetName() + wxT( "-" ) + fileBase );

            subScreen->SetFileName( subFilename.GetFullPath() );
            subSheet->SetScreen( subScreen );
            subSheet->SetName( sheetName );
            subSheet->SetFileName( subFilename.GetFullName() );

            subSheet->SetPosition( VECTOR2I( toIU( 1000.0 ),
                                             toIU( 1000.0 ) + sheetIndex * sheetSymbolStep ) );
            subSheet->SetSize( VECTOR2I( toIU( 1000.0 ), toIU( 500.0 ) ) );

            rootSheet->GetScreen()->Append( subSheet );

            screens.push_back( subScreen );
            sheetIndex++;
        }
    }

    for( size_t i = 0; i < screens.size(); i++ )
    {
        screens[i]->SetPageSettings( page );

        for( const auto& [name, sym] : libSymbols )
            screens[i]->AddLibSymbol( new LIB_SYMBOL( *sym ) );

        if( i < pcad.sheets.size() )
            populateScreen( screens[i], pcad.sheets[i], pcad, pageH, libSymbols, libName );
    }

    for( auto& [name, sym] : libSymbols )
        delete sym;

    // --- Hierarchy bookkeeping for headless and CLI consumers -------------------

    aSchematic->RefreshHierarchy();

    if( !aAppendToMe )
    {
        wxString projectName = aSchematic->Project().GetProjectName();

        if( projectName.IsEmpty() )
            projectName = wxFileName( aFileName ).GetName();

        SCH_SHEET_LIST sheets = aSchematic->BuildUnorderedSheetList();
        sheets.AddNewSymbolInstances( SCH_SHEET_PATH(), projectName );
        sheets.AddNewSheetInstances( SCH_SHEET_PATH(), 0 );

        if( sheets.AllSheetPageNumbersEmpty() )
            sheets.SetInitialPageNumbers();
    }

    return rootSheet;
}


void SCH_IO_PCAD::EnumerateSymbolLib( wxArrayString& aSymbolNameList,
                                      const wxString& aLibraryPath,
                                      const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    if( !pcad.compDefs.empty() )
    {
        for( const PCAD_SCH::COMP_DEF& cd : pcad.compDefs )
            aSymbolNameList.Add( libSymbolName( cd ) );
    }
    else
    {
        for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
            aSymbolNameList.Add( !sd.originalName.IsEmpty() ? sd.originalName : sd.name );
    }
}


void SCH_IO_PCAD::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                      const wxString& aLibraryPath,
                                      const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    const wxString libName = getLibName( nullptr, aLibraryPath );

    if( !pcad.compDefs.empty() )
    {
        for( const PCAD_SCH::COMP_DEF& cd : pcad.compDefs )
            aSymbolList.push_back( buildLibSymbolFromCompDef( cd, pcad, libName ) );
    }
    else
    {
        for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
            aSymbolList.push_back( buildLibSymbolFromSymbolDef( sd, pcad, libName ) );
    }
}


LIB_SYMBOL* SCH_IO_PCAD::LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                                     const std::map<std::string, UTF8>* aProperties )
{
    PCAD_SCH::SCHEMATIC pcad;
    PCAD_SCH::PCAD_SCH_PARSER parser;
    parser.LoadFromFile( aLibraryPath, pcad );

    const wxString libName = getLibName( nullptr, aLibraryPath );

    for( const PCAD_SCH::COMP_DEF& cd : pcad.compDefs )
    {
        if( libSymbolName( cd ) == aPartName || cd.name == aPartName
            || cd.originalName == aPartName )
        {
            return buildLibSymbolFromCompDef( cd, pcad, libName );
        }
    }

    for( const PCAD_SCH::SYMBOL_DEF& sd : pcad.symbolDefs )
    {
        if( sd.name == aPartName || sd.originalName == aPartName )
            return buildLibSymbolFromSymbolDef( sd, pcad, libName );
    }

    return nullptr;
}
