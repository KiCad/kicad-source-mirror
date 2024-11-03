/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tool/point_editor_behavior.h"

#include <geometry/shape_poly_set.h>


void POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( EDIT_POINTS&          aPoints,
                                                       const SHAPE_POLY_SET& aOutline )
{
    const int cornersCount = aOutline.TotalVertices();

    for( auto iterator = aOutline.CIterateWithHoles(); iterator; iterator++ )
    {
        aPoints.AddPoint( *iterator );

        if( iterator.IsEndContour() )
            aPoints.AddBreak();
    }

    // Lines have to be added after creating edit points, as they use EDIT_POINT references
    for( int i = 0; i < cornersCount - 1; ++i )
    {
        if( aPoints.IsContourEnd( i ) )
            aPoints.AddLine( aPoints.Point( i ), aPoints.Point( aPoints.GetContourStartIdx( i ) ) );
        else
            aPoints.AddLine( aPoints.Point( i ), aPoints.Point( i + 1 ) );

        aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
    }

    // The last missing line, connecting the last and the first polygon point
    aPoints.AddLine( aPoints.Point( cornersCount - 1 ),
                     aPoints.Point( aPoints.GetContourStartIdx( cornersCount - 1 ) ) );

    aPoints.Line( aPoints.LinesSize() - 1 )
            .SetConstraint( new EC_PERPLINE( aPoints.Line( aPoints.LinesSize() - 1 ) ) );
}


void POLYGON_POINT_EDIT_BEHAVIOR::UpdatePointsFromOutline( const SHAPE_POLY_SET& aOutline,
                                                           EDIT_POINTS&          aPoints )
{
    // No size check here, as we can and will rebuild if that fails

    if( aPoints.PointsSize() != (unsigned) aOutline.TotalVertices() )
    {
        // Rebuild the points list
        aPoints.Clear();
        BuildForPolyOutline( aPoints, aOutline );
    }
    else
    {
        for( int i = 0; i < aOutline.TotalVertices(); ++i )
        {
            aPoints.Point( i ).SetPosition( aOutline.CVertex( i ) );
        }
    }
}


void POLYGON_POINT_EDIT_BEHAVIOR::UpdateOutlineFromPoints( SHAPE_POLY_SET&   aOutline,
                                                           const EDIT_POINT& aEditedPoint,
                                                           EDIT_POINTS&      aPoints )
{
    CHECK_POINT_COUNT_GE( aPoints, (unsigned) aOutline.TotalVertices() );

    for( int i = 0; i < aOutline.TotalVertices(); ++i )
    {
        aOutline.SetVertex( i, aPoints.Point( i ).GetPosition() );
    }

    for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
    {
        if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
        {
            aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
        }
    }
}
