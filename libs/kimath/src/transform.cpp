/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <hash.h>
#include <trigo.h>
#include <transform.h>
#include <math/util.h>      // for KiROUND
#include <math/box2.h>

// a transform matrix, to display symbols in lib editor
TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, 1 );


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


BOX2I TRANSFORM::TransformCoordinate( const BOX2I& aRect ) const
{
    BOX2I rect;
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
    static const EDA_ANGLE epsilon( 0.1, DEGREES_T );

    wxCHECK_MSG( aAngle1 != nullptr && aAngle2 != nullptr, false,
                 wxT( "Cannot map NULL point angles." ) );

    double   x, y;
    VECTOR2D v;
    bool     swap = false;

    EDA_ANGLE delta = *aAngle2 - *aAngle1;

    x = aAngle1->Cos();
    y = aAngle1->Sin();
    v = VECTOR2D( x * x1 + y * y1, x * x2 + y * y2 );
    *aAngle1 = EDA_ANGLE( v );

    x = aAngle2->Cos();
    y = aAngle2->Sin();
    v = VECTOR2D( x * x1 + y * y1, x * x2 + y * y2 );
    *aAngle2 = EDA_ANGLE( v );

    EDA_ANGLE deltaTransformed = *aAngle2 - *aAngle1;
    EDA_ANGLE residualError( deltaTransformed - delta );
    residualError.Normalize();

    if( residualError > epsilon || residualError < epsilon.Invert().Normalize() )
    {
        std::swap( *aAngle1, *aAngle2 );
        swap = true;
    }

    if( *aAngle2 < *aAngle1 )
    {
        if( *aAngle2 < ANGLE_0 )
            aAngle2->Normalize();
        else
            *aAngle1 = aAngle1->Normalize() - ANGLE_360;
    }

    return swap;
}


size_t std::hash<TRANSFORM>::operator()( const TRANSFORM& s ) const
{
    size_t seed = std::hash<int>{}( s.x1 );
    hash_combine( seed, s.y1, s.x2, s.y2 );
    return seed;
}
