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

#include <wx/log.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include "sexpr/sexpr.h"
#include "kicadcurve.h"


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


bool KICADCURVE::Read( SEXPR::SEXPR* aEntry, CURVE_TYPE aCurveType )
{
    if( CURVE_LINE != aCurveType && CURVE_ARC != aCurveType && CURVE_CIRCLE != aCurveType )
    {
        std::ostringstream ostr;
        ostr << "* Unsupported curve type: " << aCurveType;
        wxLogMessage( "%s\n", ostr.str().c_str() );
        return false;
    }

    m_form = aCurveType;

    int nchild = aEntry->GetNumberOfChildren();

    if( ( CURVE_CIRCLE == aCurveType && nchild < 5 )
        || ( CURVE_ARC == aCurveType && nchild < 6 )
        || ( CURVE_LINE == aCurveType && nchild < 5 ) )
    {
        std::ostringstream ostr;
        ostr << "* bad curve data; not enough parameters";
        wxLogMessage( "%s\n", ostr.str().c_str() );
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

        if( text == "start" || text == "center" )
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
                std::ostringstream ostr;
                ostr << "* bad angle data";
                wxLogMessage( "%s\n", ostr.str().c_str() );
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
            if( child->GetNumberOfChildren() < 2
                || !child->GetChild( 1 )->IsSymbol() )
            {
                std::ostringstream ostr;
                ostr << "* bad layer data";
                wxLogMessage( "%s\n", ostr.str().c_str() );
                return false;
            }

            text = child->GetChild( 1 )->GetSymbol();

            // NOTE: for the moment we only process Edge.Cuts
            if( text == "Edge.Cuts" )
                m_layer = LAYER_EDGE;
        }
    }

    return true;
}
