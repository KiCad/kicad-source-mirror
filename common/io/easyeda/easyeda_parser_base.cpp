/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "easyeda_parser_base.h"

#include <bezier_curves.h>
#include <ki_exception.h>
#include <wx/translation.h>
#include <eda_text.h>


double EASYEDA_PARSER_BASE::Convert( const wxString& aValue )
{
    double value = 0;

    if( !aValue.ToCDouble( &value ) )
        THROW_IO_ERROR( wxString::Format( _( "Failed to parse number from '%s'" ), aValue ) );

    return value;
}


double EASYEDA_PARSER_BASE::RelPosX( double aValue )
{
    double value = aValue - m_relOrigin.x;
    return ScaleSize( value );
}


double EASYEDA_PARSER_BASE::RelPosY( double aValue )
{
    double value = aValue - m_relOrigin.y;
    return ScaleSize( value );
}


double EASYEDA_PARSER_BASE::RelPosX( const wxString& aValue )
{
    return RelPosX( Convert( aValue ) );
}


double EASYEDA_PARSER_BASE::RelPosY( const wxString& aValue )
{
    return RelPosY( Convert( aValue ) );
}


void EASYEDA_PARSER_BASE::TransformTextToBaseline( EDA_TEXT*       textItem,
                                                   const wxString& baselineAlign )
{
    int upOffset = 0;

    if( baselineAlign == wxS( "" ) || baselineAlign == wxS( "auto" )
        || baselineAlign == wxS( "use-script" ) || baselineAlign == wxS( "no-change" )
        || baselineAlign == wxS( "reset-size" ) || baselineAlign == wxS( "alphabetic" )
        || baselineAlign == wxS( "inherit" ) )
    {
        upOffset = textItem->GetTextSize().y;
    }
    else if( baselineAlign == wxS( "ideographic" ) || baselineAlign == wxS( "text-after-edge" ) )
    {
        upOffset = textItem->GetTextSize().y * 1.2;
    }
    else if( baselineAlign == wxS( "central" ) )
    {
        upOffset = textItem->GetTextSize().y * 0.5;
    }
    else if( baselineAlign == wxS( "middle" ) )
    {
        upOffset = textItem->GetTextSize().y * 0.6;
    }
    else if( baselineAlign == wxS( "mathematical" ) )
    {
        upOffset = textItem->GetTextSize().y * 0.1;
    }
    else if( baselineAlign == wxS( "hanging" ) || baselineAlign == wxS( "text-before-edge" ) )
    {
        upOffset = 0;
    }

    VECTOR2I offset( 0, -upOffset );
    RotatePoint( offset, textItem->GetTextAngle() );

    textItem->SetTextPos( textItem->GetTextPos() + offset );
}


std::vector<SHAPE_LINE_CHAIN>
EASYEDA_PARSER_BASE::ParseLineChains( const wxString& data, int aMaxError, bool aForceClosed )
{
    std::vector<SHAPE_LINE_CHAIN> result;

    VECTOR2D         prevPt;
    SHAPE_LINE_CHAIN chain;

    size_t pos = 0;
    auto   readNumber = [&]( wxString& aOut )
    {
        wxUniChar ch = data[pos];

        while( ch == ' ' || ch == ',' )
            ch = data[++pos];

        while( isdigit( ch ) || ch == '.' || ch == '-' )
        {
            aOut += ch;
            pos++;

            if( pos == data.size() )
                break;

            ch = data[pos];
        }
    };

    do
    {
        wxUniChar sym = data[pos++];

        if( sym == ' ' )
            continue;

        if( sym == 'M' )
        {
            wxString xStr, yStr;

            readNumber( xStr );
            readNumber( yStr );

            if( chain.PointCount() > 1 )
            {
                if( aForceClosed )
                    chain.SetClosed( true );

                result.emplace_back( chain );
            }

            chain.Clear();

            VECTOR2D pt( Convert( xStr ), Convert( yStr ) );
            chain.Append( RelPos( pt ) );

            prevPt = pt;
        }
        else if( sym == 'Z' )
        {
            if( chain.PointCount() > 2 )
            {
                chain.SetClosed( true );
                result.emplace_back( chain );
            }
            chain.Clear();
        }
        else if( sym == 'L' || isdigit( sym ) || sym == '-' )
        {
            // We may not have a command, just coordinates:
            // M 4108.8 3364.1 3982.598 3295.6914
            if( isdigit( sym ) || sym == '-' )
                pos--;

            while( true )
            {
                if( pos >= data.size() )
                    break;

                wxUniChar ch = data[pos];

                while( ch == ' ' || ch == ',' )
                {
                    if( ++pos >= data.size() )
                        break;

                    ch = data[pos];
                }

                if( !isdigit( ch ) && ch != '-' )
                    break;

                wxString xStr, yStr;

                readNumber( xStr );
                readNumber( yStr );

                VECTOR2D pt( Convert( xStr ), Convert( yStr ) );
                chain.Append( RelPos( pt ) );

                prevPt = pt;
            };
        }
        else if( sym == 'A' )
        {
            // Arc command can have multiple consecutive arc parameter sets
            while( true )
            {
                if( pos >= data.size() )
                    break;

                wxUniChar ch = data[pos];

                while( ch == ' ' || ch == ',' )
                {
                    if( ++pos >= data.size() )
                        break;

                    ch = data[pos];
                }

                if( !isdigit( ch ) && ch != '-' )
                    break;

                wxString radX, radY, unknown, farFlag, cwFlag, endX, endY;

                readNumber( radX );
                readNumber( radY );
                readNumber( unknown );
                readNumber( farFlag );
                readNumber( cwFlag );
                readNumber( endX );
                readNumber( endY );

                bool     isFar = farFlag == wxS( "1" );
                bool     cw = cwFlag == wxS( "1" );
                VECTOR2D rad( Convert( radX ), Convert( radY ) );
                VECTOR2D end( Convert( endX ), Convert( endY ) );

                VECTOR2D start = prevPt;
                VECTOR2D delta = end - start;

                double d = delta.EuclideanNorm();
                double h = sqrt( std::max( 0.0, rad.x * rad.x - d * d / 4 ) );

                //( !far && cw ) => h
                //( far && cw ) => -h
                //( !far && !cw ) => -h
                //( far && !cw ) => h
                VECTOR2D arcCenter = start + delta / 2 + delta.Perpendicular().Resize( ( isFar ^ cw ) ? h : -h );

                SHAPE_ARC arc;
                arc.ConstructFromStartEndCenter( RelPos( start ), RelPos( end ), RelPos( arcCenter ), !cw );

                chain.Append( arc, aMaxError );

                prevPt = end;
            }
        }
        else if( sym == 'C' )
        {
            wxString p1_xStr, p1_yStr, p2_xStr, p2_yStr, p3_xStr, p3_yStr;
            readNumber( p1_xStr );
            readNumber( p1_yStr );
            readNumber( p2_xStr );
            readNumber( p2_yStr );
            readNumber( p3_xStr );
            readNumber( p3_yStr );

            VECTOR2D pt1( Convert( p1_xStr ), Convert( p1_yStr ) );
            VECTOR2D pt2( Convert( p2_xStr ), Convert( p2_yStr ) );
            VECTOR2D pt3( Convert( p3_xStr ), Convert( p3_yStr ) );

            std::vector<VECTOR2I> ctrlPoints = { RelPos( prevPt ), RelPos( pt1 ), RelPos( pt2 ),
                                                 RelPos( pt3 ) };
            BEZIER_POLY           converter( ctrlPoints );

            std::vector<VECTOR2I> bezierPoints;
            converter.GetPoly( bezierPoints, aMaxError );

            chain.Append( bezierPoints );

            prevPt = pt3;
        }
    } while( pos < data.size() );

    if( chain.PointCount() > 1 )
    {
        if( aForceClosed )
            chain.SetClosed( true );

        result.emplace_back( chain );
    }

    return result;
}