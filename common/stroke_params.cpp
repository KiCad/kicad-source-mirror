/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <eda_rect.h>
#include <render_settings.h>
#include <geometry/shape.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/geometry_utils.h>
#include <stroke_params.h>
#include <trigo.h>

using namespace STROKEPARAMS_T;


void STROKE_PARAMS::Stroke( const SHAPE* aShape, PLOT_DASH_TYPE aLineStyle, int aWidth,
                            const KIGFX::RENDER_SETTINGS* aRenderSettings,
                            std::function<void( const wxPoint& a, const wxPoint& b )> aStroker )
{
    double strokes[6];
    int    wrapAround;

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

        EDA_RECT clip( (wxPoint)start, wxSize( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );

        for( size_t i = 0; i < 10000; ++i )
        {
            // Calculations MUST be done in doubles to keep from accumulating rounding
            // errors as we go.
            VECTOR2D next( start.x + strokes[ i % wrapAround ] * cos( theta ),
                           start.y + strokes[ i % wrapAround ] * sin( theta ) );

            // Drawing each segment can be done rounded to ints.
            wxPoint a( KiROUND( start.x ), KiROUND( start.y ) );
            wxPoint b( KiROUND( next.x ), KiROUND( next.y ) );

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

        double   r = arc->GetRadius();
        double   C = 2.0 * M_PI * r;
        VECTOR2I center = arc->GetCenter();
        VECTOR2D startRadial( arc->GetP0() - center );
        double   startAngle = 180.0 / M_PI * atan2( startRadial.y, startRadial.x );
        VECTOR2D endRadial( arc->GetP1() - center );
        double   arcEndAngle = 180.0 / M_PI * atan2( endRadial.y, endRadial.x );

        if( arcEndAngle == startAngle )
            arcEndAngle = startAngle + 360.0;   // ring, not null

        if( startAngle > arcEndAngle )
        {
            if( arcEndAngle < 0 )
                arcEndAngle = NormalizeAngleDegrees( arcEndAngle, 0.0, 360.0 );
            else
                startAngle = NormalizeAngleDegrees( startAngle, -360.0, 0.0 );
        }

        wxASSERT( startAngle < arcEndAngle );

        for( size_t i = 0; i < 10000 && startAngle < arcEndAngle; ++i )
        {
            double theta = 360.0 * strokes[ i % wrapAround ] / C;
            double endAngle = std::min( startAngle + theta, arcEndAngle );

            if( i % 2 == 0 )
            {
                wxPoint a( center.x + r * cos( startAngle * M_PI / 180.0 ),
                           center.y + r * sin( startAngle * M_PI / 180.0 ) );
                wxPoint b( center.x + r * cos( endAngle * M_PI / 180.0 ),
                           center.y + r * sin( endAngle * M_PI / 180.0 ) );

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


static wxString getLineStyleToken( PLOT_DASH_TYPE aStyle )
{
    wxString token;

    switch( aStyle )
    {
    case PLOT_DASH_TYPE::DASH:       token = "dash";         break;
    case PLOT_DASH_TYPE::DOT:        token = "dot";          break;
    case PLOT_DASH_TYPE::DASHDOT:    token = "dash_dot";     break;
    case PLOT_DASH_TYPE::DASHDOTDOT: token = "dash_dot_dot"; break;
    case PLOT_DASH_TYPE::SOLID:      token = "solid";        break;
    case PLOT_DASH_TYPE::DEFAULT:    token = "default";      break;
    }

    return token;
}


void STROKE_PARAMS::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel ) const
{
    wxASSERT( aFormatter != nullptr );

    if( GetColor() == KIGFX::COLOR4D::UNSPECIFIED )
    {
        aFormatter->Print( aNestLevel, "(stroke (width %s) (type %s))",
                           FormatInternalUnits(GetWidth() ).c_str(),
                           TO_UTF8( getLineStyleToken( GetPlotStyle() ) ) );
    }
    else
    {
        aFormatter->Print( aNestLevel, "(stroke (width %s) (type %s) (color %d %d %d %s))",
                           FormatInternalUnits(GetWidth() ).c_str(),
                           TO_UTF8( getLineStyleToken( GetPlotStyle() ) ),
                           KiROUND( GetColor().r * 255.0 ),
                           KiROUND( GetColor().g * 255.0 ),
                           KiROUND( GetColor().b * 255.0 ),
                           Double2Str( GetColor().a ).c_str() );
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

    double val = strtod( CurText(), NULL );

    return val;
}
