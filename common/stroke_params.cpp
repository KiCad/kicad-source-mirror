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

#include "stroke_params.h"
#include "stroke_params_parser.h"

#include <base_units.h>
#include <charconv>
#include <string_utils.h>
#include <render_settings.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <macros.h>
#include <trigo.h>
#include <widgets/msgpanel.h>

using namespace STROKEPARAMS_T;


const std::map<LINE_STYLE, struct LINE_STYLE_DESC> lineTypeNames = {
    { LINE_STYLE::SOLID, { _( "Solid" ), BITMAPS::stroke_solid } },
    { LINE_STYLE::DASH, { _( "Dashed" ), BITMAPS::stroke_dash } },
    { LINE_STYLE::DOT, { _( "Dotted" ), BITMAPS::stroke_dot } },
    { LINE_STYLE::DASHDOT, { _( "Dash-Dot" ), BITMAPS::stroke_dashdot } },
    { LINE_STYLE::DASHDOTDOT, { _( "Dash-Dot-Dot" ), BITMAPS::stroke_dashdotdot } }
};


void STROKE_PARAMS::Stroke( const SHAPE* aShape, LINE_STYLE aLineStyle, int aWidth,
                            const KIGFX::RENDER_SETTINGS* aRenderSettings,
                            const std::function<void( const VECTOR2I& a,
                                                      const VECTOR2I& b )>& aStroker )
{
    double strokes[6] = { aWidth * 1.0, aWidth * 1.0, aWidth * 1.0, aWidth * 1.0, aWidth * 1.0,
                          aWidth * 1.0 };
    int    wrapAround = 6;

    switch( aLineStyle )
    {
    case LINE_STYLE::DASH:
        strokes[0] = aRenderSettings->GetDashLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 2;
        break;
    case LINE_STYLE::DOT:
        strokes[0] = aRenderSettings->GetDotLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 2;
        break;
    case LINE_STYLE::DASHDOT:
        strokes[0] = aRenderSettings->GetDashLength( aWidth );
        strokes[1] = aRenderSettings->GetGapLength( aWidth );
        strokes[2] = aRenderSettings->GetDotLength( aWidth );
        strokes[3] = aRenderSettings->GetGapLength( aWidth );
        wrapAround = 4;
        break;
    case LINE_STYLE::DASHDOTDOT:
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
    case SH_RECT:
    {
        SHAPE_LINE_CHAIN outline = static_cast<const SHAPE_RECT*>( aShape )->Outline();
        std::set<size_t> arcsHandled;

        for( int ii = 0; ii < outline.SegmentCount(); ++ii )
        {
            if( outline.IsArcSegment( ii ) )
            {
                size_t arcIndex = outline.ArcIndex( ii );

                if( !arcsHandled.contains( arcIndex ) )
                {
                    arcsHandled.insert( arcIndex );
                    const SHAPE_ARC& arc( outline.Arc( arcIndex ) );
                    STROKE_PARAMS::Stroke( &arc, aLineStyle, aWidth, aRenderSettings, aStroker );
                }
            }
            else
            {
                const SEG& seg = outline.GetSegment( ii );
                SHAPE_SEGMENT line( seg.A, seg.B );
                STROKE_PARAMS::Stroke( &line, aLineStyle, aWidth, aRenderSettings, aStroker );
            }
        }

        for( int jj = 0; jj < (int) outline.ArcCount(); ++jj )
        {
            const SHAPE_ARC& arc( outline.Arc( jj ) );
            STROKE_PARAMS::Stroke( &arc, aLineStyle, aWidth, aRenderSettings, aStroker );
        }

        break;
    }

    case SH_SIMPLE:
    {
        const SHAPE_SIMPLE* poly = static_cast<const SHAPE_SIMPLE*>( aShape );

        for( size_t ii = 0; ii < poly->GetSegmentCount(); ++ii )
        {
            SEG seg = poly->GetSegment( (int) ii );
            SHAPE_SEGMENT line( seg.A, seg.B );
            STROKE_PARAMS::Stroke( &line, aLineStyle, aWidth, aRenderSettings, aStroker );
        }

        break;
    }

    case SH_SEGMENT:
    {
        const SHAPE_SEGMENT* line = static_cast<const SHAPE_SEGMENT*>( aShape );

        VECTOR2D start = line->GetSeg().A;
        VECTOR2D end = line->GetSeg().B;
        BOX2I    clip( start, KiROUND( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );

        for( size_t i = 0; i < 10000; ++i )
        {
            // Calculations MUST be done in doubles to keep from accumulating rounding
            // errors as we go.
            VECTOR2D next( start.x + strokes[ i % wrapAround ] * cos( theta ),
                           start.y + strokes[ i % wrapAround ] * sin( theta ) );

            // Drawing each segment can be done rounded to ints.
            VECTOR2I a = KiROUND( start );
            VECTOR2I b = KiROUND( next );

            if( ClipLine( &clip, a.x, a.y, b.x, b.y ) )
                break;
            else if( i % 2 == 0 )
                aStroker( a, b );

            start = next;
        }

        break;
    }

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

        EDA_ANGLE angleIncrement = EDA_ANGLE( 0.5, DEGREES_T );

        for( size_t i = 0; i < 10000 && startAngle < arcEndAngle; ++i )
        {
            EDA_ANGLE theta = ANGLE_360 * strokes[ i % wrapAround ] / C;
            EDA_ANGLE endAngle = std::min( startAngle + theta, arcEndAngle );

            if( i % 2 == 0 )
            {
                if( ( ( aLineStyle == LINE_STYLE::DASHDOT || aLineStyle == LINE_STYLE::DASHDOTDOT )
                        && i % wrapAround == 0 )
                    || aLineStyle == LINE_STYLE::DASH )
                {
                    for( EDA_ANGLE currentAngle = startAngle; currentAngle < endAngle;
                         currentAngle += angleIncrement )
                    {
                        VECTOR2I a( center.x + KiROUND( r * currentAngle.Cos() ),
                                    center.y + KiROUND( r * currentAngle.Sin() ) );

                        // Calculate the next angle step, ensuring it doesn't exceed the endAngle
                        EDA_ANGLE nextAngle = currentAngle + angleIncrement;

                        if( nextAngle > endAngle )
                        {
                            nextAngle = endAngle; // Set nextAngle to endAngle if it exceeds
                        }

                        VECTOR2I b( center.x + KiROUND( r * nextAngle.Cos() ),
                                    center.y + KiROUND( r * nextAngle.Sin() ) );

                        aStroker( a, b ); // Draw the segment as an arc
                    }
                }
                else
                {
                    VECTOR2I a( center.x + KiROUND( r * startAngle.Cos() ),
                                center.y + KiROUND( r * startAngle.Sin() ) );
                    VECTOR2I b( center.x + KiROUND( r * endAngle.Cos() ),
                                center.y + KiROUND( r * endAngle.Sin() ) );

                    aStroker( a, b );
                }
            }

            startAngle = endAngle;
        }

        break;
    }

    case SH_CIRCLE:
        // A circle is always filled; a ring is represented by a 360° arc.
        KI_FALLTHROUGH;

    default:
        UNIMPLEMENTED_FOR( SHAPE_TYPE_asString( aShape->Type() ) );
    }
}


wxString STROKE_PARAMS::GetLineStyleToken( LINE_STYLE aStyle )
{
    wxString token;

    switch( aStyle )
    {
    case LINE_STYLE::DASH:       token = wxT( "dash" );         break;
    case LINE_STYLE::DOT:        token = wxT( "dot" );          break;
    case LINE_STYLE::DASHDOT:    token = wxT( "dash_dot" );     break;
    case LINE_STYLE::DASHDOTDOT: token = wxT( "dash_dot_dot" ); break;
    case LINE_STYLE::SOLID:      token = wxT( "solid" );        break;
    case LINE_STYLE::DEFAULT:    token = wxT( "default" );      break;
    }

    return token;
}


void STROKE_PARAMS::GetMsgPanelInfo( UNITS_PROVIDER* aUnitsProvider,
                                     std::vector<MSG_PANEL_ITEM>& aList,
                                     bool aIncludeStyle, bool aIncludeWidth )
{
    if( aIncludeStyle )
    {
        wxString msg = _( "Default" );

        for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        {
            if( lineStyle == GetLineStyle() )
            {
                msg = lineStyleDesc.name;
                break;
            }
        }

        aList.emplace_back( _( "Line Style" ), msg );
    }

    if( aIncludeWidth )
        aList.emplace_back( _( "Line Width" ), aUnitsProvider->MessageTextFromValue( GetWidth() ) );
}


void STROKE_PARAMS::Format( OUTPUTFORMATTER* aFormatter, const EDA_IU_SCALE& aIuScale ) const
{
    wxASSERT( aFormatter != nullptr );

    if( GetColor() == KIGFX::COLOR4D::UNSPECIFIED )
    {
        aFormatter->Print( "(stroke (width %s) (type %s))",
                           EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, GetWidth() ).c_str(),
                           TO_UTF8( GetLineStyleToken( GetLineStyle() ) ) );
    }
    else
    {
        aFormatter->Print( "(stroke (width %s) (type %s) (color %d %d %d %s))",
                           EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, GetWidth() ).c_str(),
                           TO_UTF8( GetLineStyleToken( GetLineStyle() ) ),
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
            aStroke.SetWidth( KiROUND( parseDouble( "stroke width" ) * m_iuPerMM ) );
            NeedRIGHT();
            break;

        case T_type:
        {
            token = NextTok();

            switch( token )
            {
            case T_dash:         aStroke.SetLineStyle( LINE_STYLE::DASH );       break;
            case T_dot:          aStroke.SetLineStyle( LINE_STYLE::DOT );        break;
            case T_dash_dot:     aStroke.SetLineStyle( LINE_STYLE::DASHDOT );    break;
            case T_dash_dot_dot: aStroke.SetLineStyle( LINE_STYLE::DASHDOTDOT ); break;
            case T_solid:        aStroke.SetLineStyle( LINE_STYLE::SOLID );      break;
            case T_default:      aStroke.SetLineStyle( LINE_STYLE::DEFAULT );    break;
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
            color.a = std::clamp( parseDouble( "alpha" ), 0.0, 1.0 );

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


