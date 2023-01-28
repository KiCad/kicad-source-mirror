/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file aperture_macro.cpp
 */


#include <gerbview.h>
#include <aperture_macro.h>


void APERTURE_MACRO::AddPrimitiveToList( AM_PRIMITIVE& aPrimitive )
{
    m_primitivesList.push_back( aPrimitive );
    m_primitivesList.back().m_LocalParamLevel = m_localParamStack.size();
}

void APERTURE_MACRO::AddLocalParamDefToStack()
{
    m_localParamStack.push_back( AM_PARAM() );
}


AM_PARAM& APERTURE_MACRO::GetLastLocalParamDefFromStack()
{
    return m_localParamStack.back();
}


SHAPE_POLY_SET* APERTURE_MACRO::GetApertureMacroShape( const GERBER_DRAW_ITEM* aParent,
                                                       const VECTOR2I&         aShapePos )
{
    SHAPE_POLY_SET holeBuffer;

    m_shape.RemoveAllContours();

    for( AM_PRIMITIVE& prim_macro : m_primitivesList )
    {
        if( prim_macro.m_Primitive_id == AMP_COMMENT )
            continue;

        if( prim_macro.IsAMPrimitiveExposureOn( aParent ) )
        {
            prim_macro.ConvertBasicShapeToPolygon( aParent, m_shape, aShapePos );
        }
        else
        {
            prim_macro.ConvertBasicShapeToPolygon( aParent, holeBuffer, aShapePos );

            if( holeBuffer.OutlineCount() )     // we have a new hole in shape: remove the hole
            {
                m_shape.BooleanSubtract( holeBuffer, SHAPE_POLY_SET::PM_FAST );
                holeBuffer.RemoveAllContours();
            }
        }
    }

    // Merge and cleanup basic shape polygons
    m_shape.Simplify( SHAPE_POLY_SET::PM_FAST );

    // A hole can be is defined inside a polygon, or the polygons themselve can create
    // a hole when merged, so we must fracture the polygon to be able to drawn it
    // (i.e link holes by overlapping edges)
    m_shape.Fracture( SHAPE_POLY_SET::PM_FAST );

    return &m_shape;
}


double APERTURE_MACRO::GetLocalParam( const D_CODE* aDcode, unsigned aParamId ) const
{
    // find parameter descr.
    const AM_PARAM * param = nullptr;

    for( unsigned ii = 0; ii < m_localParamStack.size(); ii ++ )
    {
        if( m_localParamStack[ii].GetIndex() == aParamId )
        {
            param = &m_localParamStack[ii];
            break;
        }
    }

    if ( param == nullptr )    // not found
        return 0.0;

    // Evaluate parameter
    double value = param->GetValue( aDcode );

    return value;
}
