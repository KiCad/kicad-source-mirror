/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <base_units.h>
#include <macros.h>
#include <schematic_lexer.h>
#include "sch_io_kicad_sexpr_common.h"
#include <string_utils.h>


using namespace TSCHEMATIC_T;


static const char* emptyString = "";


void formatFill( OUTPUTFORMATTER* aFormatter, FILL_T aFillMode, const COLOR4D& aFillColor )
{
    const char* fillType;

    switch( aFillMode )
    {
    default:
    case FILL_T::NO_FILL:                  fillType = "none";          break;
    case FILL_T::FILLED_SHAPE:             fillType = "outline";       break;
    case FILL_T::FILLED_WITH_BG_BODYCOLOR: fillType = "background";    break;
    case FILL_T::FILLED_WITH_COLOR:        fillType = "color";         break;
    case FILL_T::HATCH:                    fillType = "hatch";         break;
    case FILL_T::REVERSE_HATCH:            fillType = "reverse_hatch"; break;
    case FILL_T::CROSS_HATCH:              fillType = "cross_hatch";   break;
    }

    if( aFillMode == FILL_T::FILLED_WITH_COLOR
            || aFillMode == FILL_T::HATCH
            || aFillMode == FILL_T::REVERSE_HATCH
            || aFillMode == FILL_T::CROSS_HATCH )
    {
        aFormatter->Print( "(fill (type %s) (color %d %d %d %s))",
                           fillType,
                           KiROUND( aFillColor.r * 255.0 ),
                           KiROUND( aFillColor.g * 255.0 ),
                           KiROUND( aFillColor.b * 255.0 ),
                           FormatDouble2Str( aFillColor.a ).c_str() );
    }
    else
    {
        aFormatter->Print( "(fill (type %s))",
                           fillType );
    }
}


const char* getPinElectricalTypeToken( ELECTRICAL_PINTYPE aType )
{
    switch( aType )
    {
    case ELECTRICAL_PINTYPE::PT_INPUT:
        return SCHEMATIC_LEXER::TokenName( T_input );

    case ELECTRICAL_PINTYPE::PT_OUTPUT:
        return SCHEMATIC_LEXER::TokenName( T_output );

    case ELECTRICAL_PINTYPE::PT_BIDI:
        return SCHEMATIC_LEXER::TokenName( T_bidirectional );

    case ELECTRICAL_PINTYPE::PT_TRISTATE:
        return SCHEMATIC_LEXER::TokenName( T_tri_state );

    case ELECTRICAL_PINTYPE::PT_PASSIVE:
        return SCHEMATIC_LEXER::TokenName( T_passive );

    case ELECTRICAL_PINTYPE::PT_NIC:
        return SCHEMATIC_LEXER::TokenName( T_free );

    case ELECTRICAL_PINTYPE::PT_UNSPECIFIED:
        return SCHEMATIC_LEXER::TokenName( T_unspecified );

    case ELECTRICAL_PINTYPE::PT_POWER_IN:
        return SCHEMATIC_LEXER::TokenName( T_power_in );

    case ELECTRICAL_PINTYPE::PT_POWER_OUT:
        return SCHEMATIC_LEXER::TokenName( T_power_out );

    case ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR:
        return SCHEMATIC_LEXER::TokenName( T_open_collector );

    case ELECTRICAL_PINTYPE::PT_OPENEMITTER:
        return SCHEMATIC_LEXER::TokenName( T_open_emitter );

    case ELECTRICAL_PINTYPE::PT_NC:
        return SCHEMATIC_LEXER::TokenName( T_no_connect );

    default:
        wxFAIL_MSG( "Missing symbol library pin connection type" );
    }

    return emptyString;
}


const char* getPinShapeToken( GRAPHIC_PINSHAPE aShape )
{
    switch( aShape )
    {
    case GRAPHIC_PINSHAPE::LINE:
        return SCHEMATIC_LEXER::TokenName( T_line );

    case GRAPHIC_PINSHAPE::INVERTED:
        return SCHEMATIC_LEXER::TokenName( T_inverted );

    case GRAPHIC_PINSHAPE::CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_clock );

    case GRAPHIC_PINSHAPE::INVERTED_CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_inverted_clock );

    case GRAPHIC_PINSHAPE::INPUT_LOW:
        return SCHEMATIC_LEXER::TokenName( T_input_low );

    case GRAPHIC_PINSHAPE::CLOCK_LOW:
        return SCHEMATIC_LEXER::TokenName( T_clock_low );

    case GRAPHIC_PINSHAPE::OUTPUT_LOW:
        return SCHEMATIC_LEXER::TokenName( T_output_low );

    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK:
        return SCHEMATIC_LEXER::TokenName( T_edge_clock_high );

    case GRAPHIC_PINSHAPE::NONLOGIC:
        return SCHEMATIC_LEXER::TokenName( T_non_logic );

    default:
        wxFAIL_MSG( "Missing symbol library pin shape type" );
    }

    return emptyString;
}


EDA_ANGLE getPinAngle( PIN_ORIENTATION aOrientation )
{
    switch( aOrientation )
    {
    case PIN_ORIENTATION::PIN_RIGHT: return ANGLE_0;
    case PIN_ORIENTATION::PIN_LEFT:  return ANGLE_180;
    case PIN_ORIENTATION::PIN_UP:    return ANGLE_90;
    case PIN_ORIENTATION::PIN_DOWN:  return ANGLE_270;
    default:        wxFAIL_MSG( "Missing symbol library pin orientation type" ); return ANGLE_0;
    }
}


const char* getSheetPinShapeToken( LABEL_FLAG_SHAPE aShape )
{
    switch( aShape )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:       return SCHEMATIC_LEXER::TokenName( T_input );
    case LABEL_FLAG_SHAPE::L_OUTPUT:      return SCHEMATIC_LEXER::TokenName( T_output );
    case LABEL_FLAG_SHAPE::L_BIDI:        return SCHEMATIC_LEXER::TokenName( T_bidirectional );
    case LABEL_FLAG_SHAPE::L_TRISTATE:    return SCHEMATIC_LEXER::TokenName( T_tri_state );
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED: return SCHEMATIC_LEXER::TokenName( T_passive );
    case LABEL_FLAG_SHAPE::F_DOT:         return SCHEMATIC_LEXER::TokenName( T_dot );
    case LABEL_FLAG_SHAPE::F_ROUND:       return SCHEMATIC_LEXER::TokenName( T_round );
    case LABEL_FLAG_SHAPE::F_DIAMOND:     return SCHEMATIC_LEXER::TokenName( T_diamond );
    case LABEL_FLAG_SHAPE::F_RECTANGLE:   return SCHEMATIC_LEXER::TokenName( T_rectangle );
    default:         wxFAIL;              return SCHEMATIC_LEXER::TokenName( T_passive );
    }
}


EDA_ANGLE getSheetPinAngle( SHEET_SIDE aSide )
{
    switch( aSide )
    {
    case SHEET_SIDE::UNDEFINED:
    case SHEET_SIDE::LEFT:     return ANGLE_180;
    case SHEET_SIDE::RIGHT:    return ANGLE_0;
    case SHEET_SIDE::TOP:      return ANGLE_90;
    case SHEET_SIDE::BOTTOM:   return ANGLE_270;
    default:   wxFAIL;         return ANGLE_0;
    }
}


const char* getTextTypeToken( KICAD_T aType )
{
    switch( aType )
    {
    case SCH_TEXT_T:            return SCHEMATIC_LEXER::TokenName( T_text );
    case SCH_LABEL_T:           return SCHEMATIC_LEXER::TokenName( T_label );
    case SCH_GLOBAL_LABEL_T:    return SCHEMATIC_LEXER::TokenName( T_global_label );
    case SCH_HIER_LABEL_T:      return SCHEMATIC_LEXER::TokenName( T_hierarchical_label );
    case SCH_DIRECTIVE_LABEL_T: return SCHEMATIC_LEXER::TokenName( T_netclass_flag );
    default:      wxFAIL;       return SCHEMATIC_LEXER::TokenName( T_text );
    }
}


std::string formatIU( const int& aValue )
{
    return EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aValue );
}


std::string formatIU( const VECTOR2I& aPt, bool aInvertY )
{
    VECTOR2I pt( aPt.x, aInvertY ? -aPt.y : aPt.y );
    return EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pt );
}


void formatArc( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aArc, bool aIsPrivate,
                const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(arc %s (start %s) (mid %s) (end %s)",
                       aIsPrivate ? "private" : "",
                       formatIU( aArc->GetStart(), aInvertY ).c_str(),
                       formatIU( aArc->GetArcMid(), aInvertY ).c_str(),
                       formatIU( aArc->GetEnd(), aInvertY ).c_str() );

    aStroke.Format( aFormatter, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


void formatCircle( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aCircle, bool aIsPrivate,
                   const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                   bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(circle %s (center %s) (radius %s)",
                       aIsPrivate ? "private" : "",
                       formatIU( aCircle->GetStart(), aInvertY ).c_str(),
                       formatIU( aCircle->GetRadius() ).c_str() );

    aStroke.Format( aFormatter, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


void formatRect( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aRect, bool aIsPrivate,
                 const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                 bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(rectangle %s (start %s) (end %s)",
                       aIsPrivate ? "private" : "",
                       formatIU( aRect->GetStart(), aInvertY ).c_str(),
                       formatIU( aRect->GetEnd(), aInvertY ).c_str() );
    if( aRect->GetCornerRadius() > 0 )
        aFormatter->Print( "(radius %s)",
                           formatIU( aRect->GetCornerRadius() ).c_str() );
    aStroke.Format( aFormatter, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


void formatBezier( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aBezier, bool aIsPrivate,
                   const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                   bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(bezier %s (pts ",
                       aIsPrivate ? "private" : "" );

    for( const VECTOR2I& pt : { aBezier->GetStart(), aBezier->GetBezierC1(),
                                aBezier->GetBezierC2(), aBezier->GetEnd() } )
    {
        aFormatter->Print( "(xy %s)", formatIU( pt, aInvertY ).c_str() );
    }

    aFormatter->Print( ")" );  // Closes pts token

    aStroke.Format( aFormatter, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}


void formatPoly( OUTPUTFORMATTER* aFormatter, EDA_SHAPE* aPolyLine, bool aIsPrivate,
                 const STROKE_PARAMS& aStroke, FILL_T aFillMode, const COLOR4D& aFillColor,
                 bool aInvertY, const KIID& aUuid )
{
    aFormatter->Print( "(polyline %s (pts ",
                       aIsPrivate ? "private" : "" );

    const SHAPE_POLY_SET& aPolySet = aPolyLine->GetPolyShape();

    if( aPolySet.OutlineCount() == 0 )
    {
        // If we've managed to get a polyline with no points, that's probably a bad thing,
        // but at least don't dereference it and crash.
        wxFAIL_MSG( "Polyline has no outlines" );
    }
    else
    {
        for( const VECTOR2I& pt : aPolyLine->GetPolyShape().Outline( 0 ).CPoints() )
            aFormatter->Print( "(xy %s)", formatIU( pt, aInvertY ).c_str() );
    }

    aFormatter->Print( ")" );  // Closes pts token

    aStroke.Format( aFormatter, schIUScale );
    formatFill( aFormatter, aFillMode, aFillColor );

    if( aUuid != niluuid )
        aFormatter->Print( "(uuid %s)", aFormatter->Quotew( aUuid.AsString() ).c_str() );

    aFormatter->Print( ")" );
}
