/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "kicadcurve.h"

#include <sexpr/sexpr.h>

#include <wx/log.h>

#include <cmath>
#include <iostream>
#include <sstream>


KICADCURVE::KICADCURVE()
{
    m_form = CURVE_NONE;
    m_angle = 0.0;
    m_radius = 0.0;
    m_layer = LAYER_NONE;
    m_startangle = 0.0;
    m_endangle = 0.0;

    return;
}


KICADCURVE::~KICADCURVE()
{
    return;
}

#include <sexpr/sexpr_parser.h>

bool KICADCURVE::Read( SEXPR::SEXPR* aEntry, CURVE_TYPE aCurveType )
{
    if( CURVE_LINE != aCurveType && CURVE_ARC != aCurveType
        && CURVE_CIRCLE != aCurveType && CURVE_BEZIER != aCurveType
        && CURVE_POLYGON != aCurveType )
    {
        wxLogMessage( "* Unsupported curve type: %d\n", aCurveType );
        return false;
    }

    m_form = aCurveType;

    int nchild = aEntry->GetNumberOfChildren();

    if( ( CURVE_CIRCLE == aCurveType && nchild < 5 )
        || ( CURVE_ARC == aCurveType && nchild < 6 )
        || ( CURVE_LINE == aCurveType && nchild < 5 )
        || ( CURVE_BEZIER == aCurveType && nchild < 5 )
        || ( CURVE_POLYGON == aCurveType && nchild < 5 ) )
    {
        wxLogMessage( "* bad curve data; not enough parameters\n" );
        return false;
    }

    SEXPR::SEXPR* child;
    std::string text;

    for( int i = 1; i < nchild; ++i )
    {
        child = aEntry->GetChild( i );

        if( !child->IsList() )
            continue;

        text = child->GetChild( 0 )->GetSymbol();

        if( text == "pts" )
        {
            // Parse and extract the list of xy coordinates
            SEXPR::PARSER parser;
            std::unique_ptr<SEXPR::SEXPR> prms = parser.Parse( child->AsString() );

            // We need 4 XY parameters (and "pts" that is the first parameter)
            if( ( aCurveType == CURVE_BEZIER && prms->GetNumberOfChildren() != 5 )
                    || ( aCurveType == CURVE_POLYGON && prms->GetNumberOfChildren() < 4 ) )
                return false;

            // Extract xy coordintes from pts list
            SEXPR::SEXPR_VECTOR const* list = prms->GetChildren();

            // The first parameter is "pts", so skip it.
            for( SEXPR::SEXPR_VECTOR::size_type ii = 1; ii < list->size(); ++ii  )
            {
                SEXPR::SEXPR* sub_child = ( *list )[ii];
                text = sub_child->GetChild( 0 )->GetSymbol();

                if( text == "xy" )
                {
                    DOUBLET coord;

                    if( !Get2DCoordinate( sub_child, coord ) )
                        return false;

                    if( aCurveType == CURVE_BEZIER )
                    {
                        switch( ii )
                        {
                        case 1: m_start = coord; break;
                        case 2: m_bezierctrl1 = coord; break;
                        case 3: m_bezierctrl2 = coord; break;
                        case 4:  m_end = coord; break;
                        default:
                            break;
                        }
                    }
                    else
                        m_poly.push_back( coord );
                }
            }
        }
        else if( text == "start" || text == "center" )
        {
            if( !Get2DCoordinate( child, m_start ) )
                return false;
        }
        else if( text == "end" )
        {
            if( !Get2DCoordinate( child, m_end ) )
                return false;
        }
        else if( text == "angle" )
        {
            if( child->GetNumberOfChildren() < 2
                || ( !child->GetChild( 1 )->IsDouble()
                     && !child->GetChild( 1 )->IsInteger() ) )
            {
                wxLogMessage( "* bad angle data\n" );
                return false;
            }

            if( child->GetChild( 1 )->IsDouble() )
                m_angle = child->GetChild( 1 )->GetDouble();
            else
                m_angle = child->GetChild( 1 )->GetInteger();

            m_angle = m_angle / 180.0 * M_PI;
        }
        else if( text == "layer" )
        {
            const OPT<std::string> layer = GetLayerName( *child );

            if( !layer )
            {
                std::ostringstream ostr;
                ostr << "* bad layer data: " << child->AsString();
                wxLogMessage( "%s\n", ostr.str().c_str() );
                return false;
            }

            // NOTE: for the moment we only process Edge.Cuts
            if( *layer == "Edge.Cuts" )
                m_layer = LAYER_EDGE;
        }
    }

    return true;
}


std::string KICADCURVE::Describe() const
{
    std::ostringstream desc;

    switch( m_form )
    {
        case CURVE_LINE:
            desc << "line start: " << m_start << " end: " << m_end;
            break;

        case CURVE_ARC:
            desc << "arc center: " << m_start << " radius: " << m_radius
                 << " angle: " << 180.0 * m_angle / M_PI
                 << " arc start: " << m_end << " arc end: " << m_ep;
            break;

        case CURVE_CIRCLE:
            desc << "circle center: " << m_start << " radius: " << m_radius;
            break;

        case CURVE_BEZIER:
            desc << "bezier start: " << m_start << " end: " << m_end
                 << " ctrl1: " << m_bezierctrl1 << " ctrl2: " << m_bezierctrl2 ;
            break;

        default:
            desc << "<invalid curve type>";
            break;
    }

    return desc.str();
}
