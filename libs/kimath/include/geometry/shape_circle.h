/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SHAPE_CIRCLE_H
#define __SHAPE_CIRCLE_H

#include <geometry/shape.h>
#include <geometry/circle.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <trigo.h>

#include <algorithm>

class SHAPE_CIRCLE : public SHAPE
{
public:
    SHAPE_CIRCLE() :
        SHAPE( SH_CIRCLE ),
        m_circle()
    {}

    SHAPE_CIRCLE( const VECTOR2I& aCenter, int aRadius ) :
        SHAPE( SH_CIRCLE ),
        m_circle( aCenter, aRadius )
    {}

    SHAPE_CIRCLE( const CIRCLE& aCircle ) :
        SHAPE( SH_CIRCLE ),
        m_circle( aCircle )
    {}

    SHAPE_CIRCLE( const SHAPE_CIRCLE& aOther ) :
        SHAPE( SH_CIRCLE ),
        m_circle( aOther.m_circle )
    {};

    ~SHAPE_CIRCLE()
    {}

    SHAPE* Clone() const override
    {
        return new SHAPE_CIRCLE( *this );
    }

    SHAPE_CIRCLE& operator=( const SHAPE_CIRCLE& ) = default;

    const BOX2I BBox( int aClearance = 0 ) const override
    {
        const VECTOR2I rc( m_circle.Radius + aClearance, m_circle.Radius + aClearance );

        return BOX2I( m_circle.Center - rc, rc * 2 );
    }

    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        int      minDist = aClearance + m_circle.Radius;
        VECTOR2I pn = aSeg.NearestPoint( m_circle.Center );
        ecoord   dist_sq = ( pn - m_circle.Center ).SquaredEuclideanNorm();

        if( dist_sq == 0 || dist_sq < SEG::Square( minDist ) )
        {
            if( aLocation )
            {
                if( std::vector<VECTOR2I> pts = m_circle.Intersect( aSeg );
                    !pts.empty() && dist_sq == 0 )
                {
                    *aLocation = m_circle.Intersect( aSeg )[0];
                }
                else
                {
                    *aLocation = pn;
                }
            }

            if( aActual )
                *aActual = std::max( 0, (int) sqrt( dist_sq ) - m_circle.Radius );

            return true;
        }

        return false;
    }

    void SetRadius( int aRadius )
    {
        m_circle.Radius = aRadius;
    }

    void SetCenter( const VECTOR2I& aCenter )
    {
        m_circle.Center = aCenter;
    }

    int GetRadius() const
    {
        return m_circle.Radius;
    }

    const VECTOR2I GetCenter() const
    {
        return m_circle.Center;
    }

    const CIRCLE GetCircle() const
    {
        return m_circle;
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_circle.Center += aVector;
    }

    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override
    {
        RotatePoint( m_circle.Center, aCenter, aAngle );
    }

    bool IsSolid() const override
    {
        return true;
    }

    virtual const std::string Format( bool aCplusPlus = true ) const override;

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override;

private:
    CIRCLE m_circle;
};

#endif
