/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
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

#pragma once

#include <core/mirror.h>     // for FLIP_DIRECTION
#include <geometry/shape.h>
#include <base_units.h>
#include <math/vector2d.h>   // for VECTOR2I
#include <geometry/eda_angle.h>

class CIRCLE;
class SHAPE_CIRCLE;
class SHAPE_LINE_CHAIN;
class SHAPE_RECT;

class SHAPE_ARC : public SHAPE
{
public:

    SHAPE_ARC() :
        SHAPE( SH_ARC ),
        m_width( 0 ),
        m_radius( 0 )
    {};

    /**
     * Construct and arc using center, start, angle.
     *
     * Center and angle are used to calculate the mid and end points of the arc, and are not
     * stored.
     *
     * @param aArcCenter is the arc center.
     * @param aArcStartPoint is the arc start point.
     * @param aCenterAngle is the arc angle.
     * @param aWidth is the arc line thickness.
     */
    SHAPE_ARC( const VECTOR2I& aArcCenter, const VECTOR2I& aArcStartPoint,
               const EDA_ANGLE& aCenterAngle, int aWidth = 0 );

    /**
     * @param aArcStart is the arc start point.
     * @param aArcEnd is the arc end point.
     * @param aArcMid is the arc mid point.
     * @param aWidth is the arc line thickness.
     */
    SHAPE_ARC( const VECTOR2I& aArcStart, const VECTOR2I& aArcMid, const VECTOR2I& aArcEnd,
               int aWidth );

    /**
     * Build a SHAPE_ARC which is tangent to two segments and a given radius.
     *
     * @param aSegmentA is the first segment
     * @param aSegmentB is the second segment
     * @param aRadius is the arc radius
     * @param aWidth is the arc line thickness
     */
    SHAPE_ARC( const SEG& aSegmentA, const SEG& aSegmentB, int aRadius, int aWidth = 0 );

    SHAPE_ARC( const SHAPE_ARC& aOther );

    SHAPE_ARC( const SHAPE_ARC& aOther, int aWidth );

    virtual ~SHAPE_ARC() {}

    SHAPE* Clone() const override
    {
        return new SHAPE_ARC( *this );
    }

    /**
     * Construct this arc from the given start, end and angle.
     *
     * @param aStart is the arc starting point
     * @param aEnd is the arc endpoint
     * @param aAngle is the arc included angle
     * @param aWidth is the arc line thickness
     * @return this arc.
     */
    SHAPE_ARC& ConstructFromStartEndAngle( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                           const EDA_ANGLE& aAngle, double aWidth = 0 );

    /**
     * Constructs this arc from the given start, end and center.
     * @param aStart is the arc starting point
     * @param aEnd is the arc endpoint
     * @param aCenter is the arc center
     * @param aClockwise determines which of the two solutions to construct
     * @param aWidth is the arc line thickness
     * @return *this
     */
    SHAPE_ARC& ConstructFromStartEndCenter( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                            const VECTOR2I& aCenter, bool aClockwise = false,
                                            double aWidth = 0 );

    const VECTOR2I& GetP0() const { return m_start; }
    const VECTOR2I& GetP1() const { return m_end; }
    const VECTOR2I& GetArcMid() const { return m_mid; }
    const VECTOR2I& GetCenter() const;

    const BOX2I BBox( int aClearance = 0 ) const override;

    VECTOR2I NearestPoint( const VECTOR2I& aP ) const;

    /**
      * Compute closest points between this arc and \a aArc.
      *
      * @param aPtA point on this arc (output)
      * @param aPtB point on the other arc (output)
      * @param aDistSq squared distance between points (output)
      * @return true if the operation was successful
      */
    bool NearestPoints( const SHAPE_ARC& aArc, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const;

    /**
      * Compute closest points between this arc and \a aCircle.
      *
      * @param aPtA point on this arc (output)
      * @param aPtB point on the circle (output)
      * @param aDistSq squared distance between points (output)
      * @return true if the operation was successful
      */
    bool NearestPoints( const SHAPE_CIRCLE& aCircle, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const;

    /**
      * Compute closest points between this arc and \a aSeg.
      *
      * @param aPtA point on this arc (output)
      * @param aPtB point on the segment (output)
      * @param aDistSq squared distance between points (output)
      * @return true if the operation was successful
      */
    bool NearestPoints( const SEG& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const;

    /**
      * Compute closest points between this arc and \a aRect.
      *
      * @param aPtA point on this arc (output)
      * @param aPtB point on the rectangle (output)
      * @param aDistSq squared distance between points (output)
      * @return true if the operation was successful
      */
    bool NearestPoints( const SHAPE_RECT& aRect, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const;

    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;
    bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;


    bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aActual, aLocation );
    }

    /**
     * Find intersection points between this arc and aSeg, treating aSeg as an infinite line.
     * Ignores arc width.
     *
     * @param aSeg Line to intersect against (treated as an infinite line)
     * @param aIpsBuffer Buffer to store the resulting intersection points (if any)
     * @return Number of intersection points found
     */
    int IntersectLine( const SEG& aSeg, std::vector<VECTOR2I>* aIpsBuffer ) const;

    /**
     * Find intersection points between this arc and a CIRCLE. Ignores arc width.
     *
     * @param aCircle Circle to intersect against
     * @param aIpsBuffer Buffer to store the resulting intersection points (if any)
     * @return Number of intersection points found
     */
    int Intersect( const CIRCLE& aArc, std::vector<VECTOR2I>* aIpsBuffer ) const;

    /**
     * Find intersection points between this arc and another arc. Ignores arc width.
     *
     * @param aArc Arc to intersect against
     * @param aIpsBuffer Buffer to store the resulting intersection points (if any)
     * @return Number of intersection points found
     */
    int Intersect( const SHAPE_ARC& aArc, std::vector<VECTOR2I>* aIpsBuffer ) const;

    VECTOR2I GetStart() const override { return m_start; }
    VECTOR2I GetEnd() const override { return m_end; }

    void SetWidth( int aWidth ) override
    {
        m_width = aWidth;
    }

    int GetWidth() const override
    {
        return m_width;
    }

    bool IsSolid() const override
    {
        return true;
    }

    bool IsEffectiveLine() const;

    void Move( const VECTOR2I& aVector ) override;

    /**
     * Rotate the arc by a given angle about a point.
     *
     * @param aCenter is the rotation center.
     * @param aAngle rotation angle.
     */
    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter ) override;

    void Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection );

    void Mirror( const SEG& axis );

    void Reverse();

    SHAPE_ARC Reversed() const;

    double GetRadius() const;

    SEG GetChord() const
    {
        return SEG( m_start, m_end );
    }

    /**
     * Get the "central angle" of the arc - this is the angle at the point of the "pie slice".
     *
     * @return the central angle of the arc shape, normalized between -360..360 deg.
     */
    EDA_ANGLE GetCentralAngle() const;

    /**
     * @return the start angle of the arc shape, normalized between 0..360 deg.
     */
    EDA_ANGLE GetStartAngle() const;

    /**
     * @return the end angle of the arc shape, normalized between 0..360 deg.
     */
    EDA_ANGLE GetEndAngle() const;

    /**
     * @return the length of the arc shape.
     */
    double GetLength() const;

    /**
     * @note The default is #ARC_HIGH_DEF in Pcbnew units.  This is to allow common geometry
     *       collision functions.  Other programs should call this using explicit accuracy
     *       values.
     *
     * @todo Unify KiCad internal units.
     *
     * @return a default accuracy value for ConvertToPolyline() to build the polyline.
     */
    static int DefaultAccuracyForPCB() { return ARC_HIGH_DEF; }

    /**
     * Construct a SHAPE_LINE_CHAIN of segments from a given arc.
     *
     * @note The default is #ARC_HIGH_DEF in Pcbnew units.  This is to allow common geometry
     *       collision functions.  Other programs should call this using explicit accuracy
     *       values.
     *
     * @todo Unify KiCad internal units.
     *
     * @param aMaxError maximum divergence from true arc given in internal units.
     * @param aActualError is the actual divergence from true arc given.
     *                     the approximation error is between -aActualError/2 and
     *                     +aActualError/2 in internal units
     * @return a #SHAPE_LINE_CHAIN.
     */
    const SHAPE_LINE_CHAIN ConvertToPolyline( int aMaxError = DefaultAccuracyForPCB(),
                                              int* aActualError = nullptr ) const;

    bool operator==( SHAPE_ARC const& aArc ) const
    {
        return ( aArc.m_start == m_start ) && ( aArc.m_end == m_end ) && ( aArc.m_mid == m_mid )
               && ( aArc.m_width == m_width );
    }

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aMaxError, ERROR_LOC aErrorLoc ) const override;

    /**
     * @return true if the arc is counter-clockwise.
     */
    bool IsCCW() const
    {
        VECTOR2L mid = m_mid;
        VECTOR2L v1 = m_end - mid;
        VECTOR2L v2 = m_start - mid;

        return v1.Cross( v2 ) > 0;
    }

    bool IsClockwise() const { return !IsCCW(); }

private:
    void update_values();

    bool sliceContainsPoint( const VECTOR2I& p ) const;

private:
    VECTOR2I m_start;
    VECTOR2I m_mid;
    VECTOR2I m_end;
    int      m_width;

    BOX2I    m_bbox;   // Calculated value
    VECTOR2I m_center; // Calculated value
    double   m_radius; // Calculated value
};

// Required for Boost Test BOOST_CHECK_EQUAL:
std::ostream& operator<<( std::ostream& aStream, const SHAPE_ARC& aArc );
