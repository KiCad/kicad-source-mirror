/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __SHAPE_SEGMENT_H
#define __SHAPE_SEGMENT_H

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <trigo.h>

#include <algorithm>

class SHAPE_SEGMENT : public SHAPE
{
public:
    SHAPE_SEGMENT() :
        SHAPE( SH_SEGMENT ),
        m_width( 0 )
    {};

    SHAPE_SEGMENT( const VECTOR2I& aA, const VECTOR2I& aB, int aWidth = 0 ) :
        SHAPE( SH_SEGMENT ),
        m_seg( aA, aB ),
        m_width( aWidth )
    {};

    SHAPE_SEGMENT( const SEG& aSeg, int aWidth = 0 ) :
        SHAPE( SH_SEGMENT ),
        m_seg( aSeg ),
        m_width( aWidth )
    {};

    static SHAPE_SEGMENT BySizeAndCenter( const VECTOR2I& aSize, const VECTOR2I& aCenter, const EDA_ANGLE& aRotation );

    ~SHAPE_SEGMENT() {};

    SHAPE* Clone() const override
    {
        return new SHAPE_SEGMENT( m_seg, m_width );
    }

    const BOX2I BBox( int aClearance = 0 ) const override
    {
        return BOX2I( m_seg.A, m_seg.B - m_seg.A ).Inflate( aClearance + ( m_width + 1 ) / 2 );
    }

    bool Collide( const SHAPE* aShape, int aClearance, VECTOR2I* aMTV ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aMTV );
    }

    bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aActual, aLocation );
    }

    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        if( aSeg.A == aSeg.B )
            return Collide( aSeg.A, aClearance, aActual, aLocation );

        int min_dist = ( m_width + 1 ) / 2 + aClearance;
        ecoord dist_sq = m_seg.SquaredDistance( aSeg );

        if( dist_sq == 0 || dist_sq < SEG::Square( min_dist ) )
        {
            if( aLocation )
                *aLocation = m_seg.NearestPoint( aSeg );

            if( aActual )
                *aActual = std::max( 0, (int) sqrt( dist_sq ) - ( m_width + 1 ) / 2 );

            return true;
        }

        return false;
    }

    bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        int min_dist = ( m_width + 1 ) / 2 + aClearance;
        ecoord dist_sq = m_seg.SquaredDistance( aP );

        if( dist_sq == 0 || dist_sq < SEG::Square( min_dist ) )
        {
            if( aLocation )
                *aLocation = m_seg.NearestPoint( aP );

            if( aActual )
                *aActual = std::max( 0, (int) sqrt( dist_sq ) - ( m_width + 1 ) / 2 );

            return true;
        }

        return false;
    }

    void SetSeg( const SEG& aSeg )
    {
        m_seg = aSeg;
    }

    const SEG& GetSeg() const
    {
        return m_seg;
    }

    VECTOR2I GetStart() const override { return m_seg.A; }
    VECTOR2I GetEnd() const override { return m_seg.B; }

    void SetWidth( int aWidth ) override
    {
        m_width = aWidth;
    }

    int GetWidth() const override
    {
        return m_width;
    }

    /**
     * Get the total length of the segment, from tip to tip.
     */
    int GetTotalLength() const
    {
        return m_seg.Length() + m_width;
    }

    VECTOR2I GetCenter() const
    {
        return m_seg.Center();
    }

    EDA_ANGLE GetAngle() const
    {
        return EDA_ANGLE( m_seg.B - m_seg.A );
    }

    bool IsSolid() const override
    {
        return true;
    }

    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override
    {
        RotatePoint( m_seg.A, aCenter, aAngle );
        RotatePoint( m_seg.B, aCenter, aAngle );
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_seg.A += aVector;
        m_seg.B += aVector;
    }

    bool Is45Degree( EDA_ANGLE aTollerance = EDA_ANGLE( 1.0, DEGREES_T ) ) const;

    virtual const std::string Format( bool aCplusPlus = true ) const override;

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override;

private:
    SEG m_seg;
    int m_width;
};

#endif
