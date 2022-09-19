/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <base_units.h>
#include <charconv>
#include <string_utils.h>
#include <render_settings.h>
#include <geometry/shape.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/geometry_utils.h>
#include <stroke_params.h>
#include <trigo.h>
#include <widgets/msgpanel.h>

using namespace STROKEPARAMS_T;


void STROKE_PARAMS::Stroke( const SHAPE* aShape, PLOT_DASH_TYPE aLineStyle, int aWidth,
                            const KIGFX::RENDER_SETTINGS* aRenderSettings,
                            std::function<void( const VECTOR2I& a, const VECTOR2I& b )> aStroker )
{
    double strokes[6] = { aWidth * 1.0, aWidth * 1.0, aWidth * 1.0, aWidth * 1.0, aWidth * 1.0,
                          aWidth * 1.0 };
    int    wrapAround = 6;

    switch( aLineStyle )
    {
    case PLOT_DASH_TYPE::DASH:
        strokes[0] = aRenderSettings->GetDashLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 2;
        break;
    case PLOT_DASH_TYPE::DOT:
        strokes[0] = aRenderSettings->GetDotLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 2;
        break;
    case PLOT_DASH_TYPE::DASHDOT:
        strokes[0] = aRenderSettings->GetDashLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        strokes[2] = aRenderSettings->GetDotLength( aWidth );
        strokes[3] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 4;
        break;
    case PLOT_DASH_TYPE::DASHDOTDOT:
        strokes[0] = aRenderSettings->GetDashLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        strokes[2] = aRenderSettings->GetDotLength( aWidth );
        strokes[3] = aRenderSettings->GetGapLength( aWidth );
        strokes[4] = aRenderSettings->GetDotLength( aWidth );
        strokes[5] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 6;
        break;
    default:
        UNIMPLEMENTED_FOR( lineTypeNames.at( aLineStyle ).name );
    }

    switch( aShape->Type() )
    {
    case SH_SIMPLE:
    {
        const SHAPE_SIMPLE* poly = static_cast<const SHAPE_SIMPLE*>( aShape );

        for( size_t ii = 0; ii < poly->GetSegmentCount(); ++ii )
        {
            SEG seg = poly->GetSegment( ii );
            SHAPE_SEGMENT line( seg.A, seg.B );
            STROKE_PARAMS::Stroke( &line, aLineStyle, aWidth, aRenderSettings, aStroker );
        }
    }
        break;

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* line = static_cast<const SHAPE_SEGMENT*>( aShape );

        VECTOR2D start = line->GetSeg().A;
        VECTOR2D end = line->GetSeg().B;
        BOX2I    clip( start, VECTOR2I( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );

        for( size_t i = 0; i < 10000; ++i )
        {
            // Calculations MUST be done in doubles to keep from accumulating rounding
            // errors as we go.
            VECTOR2D next( start.x + strokes[ i % wrapAround ] * cos( theta ),
                           start.y + strokes[ i % wrapAround ] * sin( theta ) );

            // Drawing each segment can be done rounded to ints.
            VECTOR2I a( KiROUND( start.x ), KiROUND( start.y ) );
            VECTOR2I b( KiROUND( next.x ), KiROUND( next.y ) );

            if( ClipLine( &clip, a.x, a.y, b.x, b.y ) )
                break;
            else if( i % 2 == 0 )
                aStroker( a, b );

            start = next;
        }
    }
        break;

    case SH_ARC:
    {
        const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aShape );

        double    r = arc->GetRadius();
        double    C = 2.0 * M_PI * r;
        VECTOR2I  center = arc->GetCenter();
        VECTOR2D  startRadial( arc->GetP0() - center );
        EDA_ANGLE startAngle( startRadial );
        VECTOR2D  endRadial( arc->GetP1() - center );
        EDA_ANGLE arcEndAngle( endRadial );

        if( arcEndAngle == startAngle )
            arcEndAngle = startAngle + ANGLE_360;   // ring, not null

        if( startAngle > arcEndAngle )
        {
            if( arcEndAngle < ANGLE_0 )
                arcEndAngle = arcEndAngle.Normalize();
            else
                startAngle = startAngle.Normalize() - ANGLE_360;
        }

        wxASSERT( startAngle < arcEndAngle );

        for( size_t i = 0; i < 10000 && startAngle < arcEndAngle; ++i )
        {
            EDA_ANGLE theta = ANGLE_360 * strokes[ i % wrapAround ] / C;
            EDA_ANGLE endAngle = std::min( startAngle + theta, arcEndAngle );

            if( i % 2 == 0 )
            {
                VECTOR2I a( center.x + r * startAngle.Cos(), center.y + r * startAngle.Sin() );
                VECTOR2I b( center.x + r * endAngle.Cos(),   center.y + r * endAngle.Sin() );

                aStroker( a, b );
            }

            startAngle = endAngle;
        }
    }
        break;

    case SH_CIRCLE:
        // A circle is always filled; a ring is represented by a 360° arc.
        KI_FALLTHROUGH;

    default:
        UNIMPLEMENTED_FOR( SHAPE_TYPE_asString( aShape->Type() ) );
    }
}


wxString STROKE_PARAMS::GetLineStyleToken( PLOT_DASH_TYPE aStyle )
{
    wxString token;

    switch( aStyle )
    {
    case PLOT_DASH_TYPE::DASH:       token = wxT( "dash" );         break;
    case PLOT_DASH_TYPE::DOT:        token = wxT( "dot" );          break;
    case PLOT_DASH_TYPE::DASHDOT:    token = wxT( "dash_dot" );     break;
    case PLOT_DASH_TYPE::DASHDOTDOT: token = wxT( "dash_dot_dot" ); break;
    case PLOT_DASH_TYPE::SOLID:      token = wxT( "solid" );        break;
    case PLOT_DASH_TYPE::DEFAULT:    token = wxT( "default" );      break;
    }

    return token;
}


void STROKE_PARAMS::GetMsgPanelInfo( UNITS_PROVIDER* aUnitsProvider,
                                     std::vector<MSG_PANEL_ITEM>& aList,
                                     bool aIncludeStyle, bool aIncludeWidth )
{
    if( aIncludeStyle )
    {
        wxString lineStyle = _( "Default" );

        for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        {
            if( typeEntry.first == GetPlotStyle() )
            {
                lineStyle = typeEntry.second.name;
                break;
            }
        }

        aList.emplace_back( _( "Line Style" ), lineStyle );
    }

    if( aIncludeWidth )
        aList.emplace_back( _( "Line Width" ), aUnitsProvider->MessageTextFromValue( GetWidth() ) );
}


void STROKE_PARAMS::Format( OUTPUTFORMATTER* aFormatter, const EDA_IU_SCALE& aIuScale,
                            int aNestLevel ) const
{
    wxASSERT( aFormatter != nullptr );

    if( GetColor() == KIGFX::COLOR4D::UNSPECIFIED )
    {
        aFormatter->Print( aNestLevel, "(stroke (width %s) (type %s))",
                           EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, GetWidth() ).c_str(),
                           TO_UTF8( GetLineStyleToken( GetPlotStyle() ) ) );
    }
    else
    {
        aFormatter->Print( aNestLevel, "(stroke (width %s) (type %s) (color %d %d %d %s))",
                           EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, GetWidth() ).c_str(),
                           TO_UTF8( GetLineStyleToken( GetPlotStyle() ) ),
                           KiROUND( GetColor().r * 255.0 ),
                           KiROUND( GetColor().g * 255.0 ),
                           KiROUND( GetColor().b * 255.0 ),
                           FormatDouble2Str( GetColor().a ).c_str() );
    }
}


void STROKE_PARAMS_PARSER::ParseStroke( STROKE_PARAMS& aStroke )
{
    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_width:
            aStroke.SetWidth( parseDouble( "stroke width" ) * m_iuPerMM );
            NeedRIGHT();
            break;

        case T_type:
        {
            token = NextTok();

            switch( token )
            {
            case T_dash:         aStroke.SetPlotStyle( PLOT_DASH_TYPE::DASH );       break;
            case T_dot:          aStroke.SetPlotStyle( PLOT_DASH_TYPE::DOT );        break;
            case T_dash_dot:     aStroke.SetPlotStyle( PLOT_DASH_TYPE::DASHDOT );    break;
            case T_dash_dot_dot: aStroke.SetPlotStyle( PLOT_DASH_TYPE::DASHDOTDOT ); break;
            case T_solid:        aStroke.SetPlotStyle( PLOT_DASH_TYPE::SOLID );      break;
            case T_default:      aStroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );    break;
            default:
                Expecting( "solid, dash, dash_dot, dash_dot_dot, dot or default" );
            }

            NeedRIGHT();
            break;
        }

        case T_color:
        {
            KIGFX::COLOR4D color;

            color.r = parseInt( "red" ) / 255.0;
            color.g = parseInt( "green" ) / 255.0;
            color.b = parseInt( "blue" ) / 255.0;
            color.a = Clamp( parseDouble( "alpha" ), 0.0, 1.0 );

            aStroke.SetColor( color );
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "width, type, or color" );
        }
    }
}


int STROKE_PARAMS_PARSER::parseInt( const char* aText )
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( aText );

    return atoi( CurText() );
}


double STROKE_PARAMS_PARSER::parseDouble( const char* aText )
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( aText );

    return DSNLEXER::parseDouble();
}


