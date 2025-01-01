/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_simple.h>

#include <convert_basic_shapes_to_polygon.h>


bool SHAPE::Parse( std::stringstream& aStream )
{
    assert( false );
    return false;
}


const std::string SHAPE::Format( bool aCplusPlus ) const
{
    std::stringstream ss;
    ss << "shape " << m_type;
    return ss.str();
}


int SHAPE::GetClearance( const SHAPE* aOther ) const
{
    int actual_clearance = std::numeric_limits<int>::max();
    std::vector<const SHAPE*> a_shapes;
    std::vector<const SHAPE*> b_shapes;

    /// POLY_SETs contain a bunch of polygons that are triangulated.
    /// But there are way more triangles than necessary for collision detection.
    /// Triangles check three vertices each but for the outline, we only need one.
    /// These are also fractured, so we don't need to worry about holes
    if( Type() == SHAPE_TYPE::SH_POLY_SET )
    {
        const SHAPE_POLY_SET* polySet = static_cast<const SHAPE_POLY_SET*>( this );
        if( polySet->OutlineCount() > 0 )
            a_shapes.push_back( &polySet->COutline( 0 ) );
    }
    else
    {
        GetIndexableSubshapes( a_shapes );
    }

    if( aOther->Type() == SHAPE_TYPE::SH_POLY_SET )
    {
        const SHAPE_POLY_SET* polySet = static_cast<const SHAPE_POLY_SET*>( aOther );
        if( polySet->OutlineCount() > 0 )
            b_shapes.push_back( &polySet->COutline( 0 ) );
    }
    else
    {
        aOther->GetIndexableSubshapes( b_shapes );
    }

    if( GetIndexableSubshapeCount() == 0 )
        a_shapes.push_back( this );

    if( aOther->GetIndexableSubshapeCount() == 0 )
        b_shapes.push_back( aOther );

    for( const SHAPE* a : a_shapes )
    {
        for( const SHAPE* b : b_shapes )
        {
            int temp_dist = 0;
            a->Collide( b, std::numeric_limits<int>::max() / 2, &temp_dist );

            if( temp_dist < actual_clearance )
                actual_clearance = temp_dist;
        }
    }

    return actual_clearance;
}


int SHAPE::Distance( const VECTOR2I& aP ) const
{
    return sqrt( SquaredDistance( aP, false ) );
}


SEG::ecoord SHAPE::SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    SHAPE_POLY_SET buffer;
    TransformToPolygon( buffer, 0, ERROR_INSIDE );

    if( buffer.OutlineCount() < 1 )
        return VECTOR2I::ECOORD_MAX;

    return buffer.COutline( 0 ).SquaredDistance( aP, aOutlineOnly );
}


bool SHAPE::PointInside( const VECTOR2I& aPt, int aAccuracy, bool aUseBBoxCache ) const
{
    SHAPE_POLY_SET buffer;
    TransformToPolygon( buffer, aAccuracy, ERROR_INSIDE );

    if( buffer.OutlineCount() < 1 )
        return false;

    return buffer.COutline( 0 ).PointInside( aPt, aAccuracy, aUseBBoxCache );
}


void SHAPE_SIMPLE::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                                       ERROR_LOC aErrorLoc ) const
{
    aBuffer.AddOutline( m_points );
}
