/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

/**
 * @file aperture_macro.cpp
 */


#include <gerbview.h>
#include <aperture_macro.h>
#include <gerber_draw_item.h>

void APERTURE_MACRO::InitLocalParams( const D_CODE* aDcode )
{
    // store the initial values coming from aDcode into m_localParamValues
    // for n parameters, they are local params $1 to $n
    m_localParamValues.clear();

    // Note: id_param = 1... n, not 0
    for( unsigned id_param = 1; id_param <= aDcode->GetParamCount(); id_param++ )
        m_localParamValues[id_param] = aDcode->GetParam( id_param );

    m_paramLevelEval = 0;
}


void APERTURE_MACRO::EvalLocalParams( const AM_PRIMITIVE& aPrimitive )
{
    // Evaluate m_localParamValues from current m_paramLevelEval to
    // aPrimitive.m_LocalParamLevel
    // if m_paramLevelEval >= m_LocalParamLevel, do nothing: the
    // m_localParamValues are already up to date

    if( m_paramLevelEval >= aPrimitive.m_LocalParamLevel )
        return;

    for( ; m_paramLevelEval < aPrimitive.m_LocalParamLevel; m_paramLevelEval++ )
    {
        AM_PARAM& am_param = m_localParamStack.at( m_paramLevelEval );
        int prm_index = am_param.GetIndex();

        double value = am_param.GetValueFromMacro( this );

        // if am_param value is not yet stored in m_localParamValues, add it.
        // if it is already in m_localParamValues, update its value;
        m_localParamValues[ prm_index ] = value;
    }
}


double APERTURE_MACRO::GetLocalParamValue( int aIndex )
{
    // return the local param value stored in m_localParamValues
    // if not existing, returns 0

    if( m_localParamValues.find( aIndex ) != m_localParamValues.end() )
        return m_localParamValues[ aIndex ];

    return 0.0;
}


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
                                                       const VECTOR2I& aShapePos )
{
    SHAPE_POLY_SET holeBuffer;

    m_shape.RemoveAllContours();
    D_CODE* dcode = aParent->GetDcodeDescr();
    InitLocalParams( dcode );

    for( AM_PRIMITIVE& prim_macro : m_primitivesList )
    {
        if( prim_macro.m_Primitive_id == AMP_COMMENT )
            continue;

        if( prim_macro.IsAMPrimitiveExposureOn( this ) )
        {
            prim_macro.ConvertBasicShapeToPolygon( this, m_shape );
        }
        else
        {
            prim_macro.ConvertBasicShapeToPolygon( this, holeBuffer );

            if( holeBuffer.OutlineCount() )     // we have a new hole in shape: remove the hole
            {
                m_shape.BooleanSubtract( holeBuffer );
                holeBuffer.RemoveAllContours();
            }
        }
    }

    // Merge and cleanup basic shape polygons
    m_shape.Simplify();

    // A hole can be is defined inside a polygon, or the polygons themselve can create
    // a hole when merged, so we must fracture the polygon to be able to drawn it
    // (i.e link holes by overlapping edges)
    m_shape.Fracture();

    // Move m_shape to the actual draw position:
    for( int icnt = 0; icnt < m_shape.OutlineCount(); icnt++ )
    {

        SHAPE_LINE_CHAIN& outline = m_shape.Outline( icnt );

        for( int jj = 0; jj < outline.PointCount(); jj++ )
        {
            VECTOR2I point = outline.CPoint( jj );
            point += aShapePos;
            point = aParent->GetABPosition( point );
            outline.SetPoint( jj, point );
        }
    }

    return &m_shape;
}
