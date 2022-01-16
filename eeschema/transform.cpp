/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <macros.h>
#include <trigo.h>
#include <transform.h>
#include <common.h>
#include <eda_rect.h>
#include <math/util.h>      // for KiROUND


bool TRANSFORM::operator==( const TRANSFORM& aTransform ) const
{
    return ( x1 == aTransform.x1 &&
             y1 == aTransform.y1 &&
             x2 == aTransform.x2 &&
             y2 == aTransform.y2 );
}


VECTOR2I TRANSFORM::TransformCoordinate( const VECTOR2I& aPoint ) const
{
    return VECTOR2I( ( x1 * aPoint.x ) + ( y1 * aPoint.y ), ( x2 * aPoint.x ) + ( y2 * aPoint.y ) );
}


EDA_RECT TRANSFORM::TransformCoordinate( const EDA_RECT& aRect ) const
{
    EDA_RECT rect;
    rect.SetOrigin( TransformCoordinate( aRect.GetOrigin() ) );
    rect.SetEnd( TransformCoordinate( aRect.GetEnd() ) );
    return rect;
}


TRANSFORM TRANSFORM::InverseTransform() const
{
    int invx1;
    int invx2;
    int invy1;
    int invy2;

    /* Calculates the inverse matrix coeffs:
     * for a matrix m{x1, x2, y1, y2}
     * the inverse matrix is 1/(x1*y2 -x2*y1) m{y2,-x2,-y1,x1)
     */
    int det = x1*y2 -x2*y1; // Is never null, because the inverse matrix exists
    invx1 = y2/det;
    invx2 = -x2/det;
    invy1 = -y1/det;
    invy2 = x1/det;

    TRANSFORM invtransform( invx1, invy1, invx2, invy2 );
    return invtransform;
}


bool TRANSFORM::MapAngles( EDA_ANGLE* aAngle1, EDA_ANGLE* aAngle2 ) const
{
    wxCHECK_MSG( aAngle1 != nullptr && aAngle2 != nullptr, false,
                 wxT( "Cannot map NULL point angles." ) );

    static const EDA_ANGLE epsilon( 0.1, DEGREES_T );

    double   x, y;
    VECTOR2D v;
    bool     swap = false;

    EDA_ANGLE delta = *aAngle2 - *aAngle1;

    if( delta >= ANGLE_180 )
    {
        *aAngle1 -= epsilon;
        *aAngle2 += epsilon;
    }

    x = aAngle1->Cos();
    y = aAngle1->Sin();
    v = VECTOR2D( x * x1 + y * y1, x * x2 + y * y2 );
    *aAngle1 = EDA_ANGLE( v );

    x = aAngle2->Cos();
    y = aAngle2->Sin();
    v = VECTOR2D( x * x1 + y * y1, x * x2 + y * y2 );
    *aAngle2 = EDA_ANGLE( v );

    aAngle1->Normalize();
    aAngle2->Normalize();

    if( *aAngle2 < *aAngle1 )
        *aAngle2 += ANGLE_360;

    if( *aAngle2 - *aAngle1 > ANGLE_180 ) // Need to swap the two angles
    {
        std::swap( *aAngle1, *aAngle2 );

        aAngle1->Normalize();
        aAngle2->Normalize();

        if( *aAngle2 < *aAngle1 )
            *aAngle2 += ANGLE_360;

        swap = true;
    }

    if( delta >= ANGLE_180 )
    {
        *aAngle1 += epsilon;
        *aAngle2 -= epsilon;
    }

    return swap;
}


