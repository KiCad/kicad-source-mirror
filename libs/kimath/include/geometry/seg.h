/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef __SEG_H
#define __SEG_H

#include <math.h>                       // for sqrt
#include <stdlib.h>                     // for abs
#include <optional>
#include <ostream>                      // for operator<<, ostream, basic_os...
#include <type_traits>                  // for swap

#include <math/vector2d.h>
#include <geometry/eda_angle.h>

typedef std::optional<VECTOR2I> OPT_VECTOR2I;

class SEG
{
public:
    using ecoord = VECTOR2I::extended_type;
    friend inline std::ostream& operator<<( std::ostream& aStream, const SEG& aSeg );

    /* Start and the of the segment. Public, to make access simpler.
     */
    VECTOR2I A;
    VECTOR2I B;

    /**
     * Create an empty (0, 0) segment.
     */
    SEG()
    {
        m_index = -1;
    }

    /**
     * Create a segment between (aX1, aY1) and (aX2, aY2).
     */
    SEG( int aX1, int aY1, int aX2, int aY2 ) :
        A( VECTOR2I( aX1, aY1 ) ),
        B( VECTOR2I( aX2, aY2 ) )
    {
        m_index = -1;
    }

    /**
     * Create a segment between (aA) and (aB).
     */
    SEG( const VECTOR2I& aA, const VECTOR2I& aB ) :
        A( aA ),
        B( aB )
    {
        m_index = -1;
    }

    /**
     * Create a segment between (aA) and (aB), referenced to a multi-segment shape.
     *
     * @param aA reference to the start point in the parent shape
     * @param aB reference to the end point in the parent shape
     * @param aIndex index of the segment within the parent shape
     */
    SEG( const VECTOR2I& aA, const VECTOR2I& aB, int aIndex ) :
        A( aA ),
        B( aB )
    {
        m_index = aIndex;
    }

    /**
     * Copy constructor.
     */
    SEG( const SEG& aSeg ) :
        A( aSeg.A ),
        B( aSeg.B ),
        m_index( aSeg.m_index )
    {
    }

    SEG& operator=( const SEG& aSeg )
    {
        A = aSeg.A;
        B = aSeg.B;
        m_index = aSeg.m_index;

        return *this;
    }

    bool operator==( const SEG& aSeg ) const
    {
        return (A == aSeg.A && B == aSeg.B) ;
    }

    bool operator!=( const SEG& aSeg ) const
    {
        return (A != aSeg.A || B != aSeg.B);
    }

    static SEG::ecoord Square( int a )
    {
        return ecoord( a ) * a;
    }

    /**
      * Compute the perpendicular projection point of aP on a line passing through
      * ends of the segment.
      *
      * @param aP point to project
      * @return projected point
      */
    VECTOR2I LineProject( const VECTOR2I& aP ) const;

    /**
      * Determine on which side of directed line passing via segment ends point aP lies.
      *
      * @param aP point to determine the orientation wrs to self
      * @return: < 0: left, 0 : on the line, > 0 : right
      */
    int Side( const VECTOR2I& aP ) const
    {
        const ecoord det = ( B - A ).Cross( aP - A );

        return det < 0 ? -1 : ( det > 0 ? 1 : 0 );
    }

    /**
      * Return the closest Euclidean distance between point aP and the line defined by
      * the ends of segment (this).
      *
      * @param aP the point to test
      * @param aDetermineSide: when true, the sign of the returned value indicates
      * the side of the line at which we are (negative = left)
      * @return the distance
      */
    int LineDistance( const VECTOR2I& aP, bool aDetermineSide = false ) const;

    /**
      * Determine the smallest angle between two segments
      *
      * @param aOther point to determine the orientation wrs to self
      * @return smallest angle between this and aOther
      */
    EDA_ANGLE Angle( const SEG& aOther ) const;

    /**
      * Compute a point on the segment (this) that is closest to point \a aP.
      *
      * @return the nearest point
      */
    const VECTOR2I NearestPoint( const VECTOR2I& aP ) const;

    /**
      * Compute a point on the segment (this) that is closest to any point on \a aSeg.
      *
      * @return the nearest point
      */
    const VECTOR2I NearestPoint( const SEG &aSeg ) const;

    /**
      * Compute closest points between this segment and \a aSeg.
      *
      * @param aPtA point on this segment (output)
      * @param aPtB point on the other segment (output)
      * @param aDistSq squared distance between points (output)
      * @return true if the operation was successful
      */
    bool NearestPoints( const SEG& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const;

    /**
      * Reflect a point using this segment as axis.
      *
      * @return the reflected point
      */
    const VECTOR2I ReflectPoint( const VECTOR2I& aP ) const;

    /**
     * Compute intersection point of segment (this) with segment \a aSeg.
     *
     * @param aSeg: segment to intersect with
     * @param aIgnoreEndpoints: don't treat corner cases (i.e. end of one segment touching the
     * other) as intersections.
     * @param aLines: treat segments as infinite lines
     * @return intersection point, if exists
     */
    OPT_VECTOR2I Intersect( const SEG& aSeg, bool aIgnoreEndpoints = false,
                            bool aLines = false ) const;

    bool Intersects( const SEG& aSeg ) const;

    /**
     * Compute the intersection point of lines passing through ends of (this) and \a aSeg.
     *
     * @param aSeg segment defining the line to intersect with
     * @return intersection point, if exists
     */
    OPT_VECTOR2I IntersectLines( const SEG& aSeg ) const
    {
        return Intersect( aSeg, false, true );
    }

    /**
     * Check if this segment intersects a line defined by slope \a aSlope and offset \a aOffset.
     *
     * @param aSlope slope of the line
     * @param aOffset offset of the line
     * @param aIntersection output intersection point, if exists
     * @return true if the segment intersects the line, false otherwise
     */
    bool IntersectsLine( double aSlope, double aOffset, VECTOR2I& aIntersection ) const;

    /**
     * Compute a segment perpendicular to this one, passing through point \a aP.
     *
     * @param aP Point through which the new segment will pass
     * @return SEG perpendicular to this passing through point aP
     */
    SEG PerpendicularSeg( const VECTOR2I& aP ) const;

    /**
     * Compute a segment parallel to this one, passing through point \a aP.
     *
     * @param aP Point through which the new segment will pass
     * @return SEG parallel to this passing through point aP
     */
    SEG ParallelSeg( const VECTOR2I& aP ) const;

    bool Collide( const SEG& aSeg, int aClearance, int* aActual = nullptr ) const;

    ecoord SquaredDistance( const SEG& aSeg ) const;

    /**
     * Compute minimum Euclidean distance to segment \a aSeg.
     *
     * @param aSeg other segment
     * @return minimum distance
     */
    int Distance( const SEG& aSeg ) const;

    ecoord SquaredDistance( const VECTOR2I& aP ) const;

    /**
     * Compute minimum Euclidean distance to point \a aP.
     *
     * @param aP the point
     * @return minimum distance
     */
    int Distance( const VECTOR2I& aP ) const;

    void CanonicalCoefs( ecoord& qA, ecoord& qB, ecoord& qC ) const
    {
        qA = ecoord{ A.y } - B.y;
        qB = ecoord{ B.x } - A.x;
        qC = -qA * A.x - qB * A.y;
    }

    /**
     * Check if segment aSeg lies on the same line as (this).
     *
     * @param aSeg the segment to check colinearity with
     * @return true, when segments are collinear.
     */
    bool Collinear( const SEG& aSeg ) const
    {
        ecoord qa, qb, qc;
        CanonicalCoefs( qa, qb, qc );

        ecoord d1 = std::abs( aSeg.A.x * qa + aSeg.A.y * qb + qc );
        ecoord d2 = std::abs( aSeg.B.x * qa + aSeg.B.y * qb + qc );

        return ( d1 <= 1 && d2 <= 1 );
    }

    bool ApproxCollinear( const SEG& aSeg, int aDistanceThreshold = 1 ) const;
    bool ApproxParallel( const SEG& aSeg, int aDistanceThreshold = 1 ) const;
    bool ApproxPerpendicular( const SEG& aSeg ) const;

    bool Overlaps( const SEG& aSeg ) const
    {
        if( aSeg.A == aSeg.B ) // single point corner case
        {
            if( A == aSeg.A || B == aSeg.A )
                return false;

            return Contains( aSeg.A );
        }

        if( !Collinear( aSeg ) )
            return false;

        if( Contains( aSeg.A ) || Contains( aSeg.B ) )
            return true;

        if( aSeg.Contains( A ) || aSeg.Contains( B ) )
            return true;

        return false;
    }


    bool Contains( const SEG& aSeg ) const
    {
        if( aSeg.A == aSeg.B ) // single point corner case
            return Contains( aSeg.A );

        if( !Collinear( aSeg ) )
            return false;

        if( Contains( aSeg.A ) && Contains( aSeg.B ) )
            return true;

        return false;
    }

    /**
     * Return the length (this).
     *
     * @return length
     */
    int Length() const
    {
        return ( A - B ).EuclideanNorm();
    }

    ecoord SquaredLength() const
    {
        return ( A - B ).SquaredEuclideanNorm();
    }

    ecoord TCoef( const VECTOR2I& aP ) const;

    /**
     * Return the index of this segment in its parent shape (applicable only to non-local
     * segments).
     *
     * @return index value
     */
    int Index() const
    {
        return m_index;
    }

    bool Contains( const VECTOR2I& aP ) const;

    void Reverse()
    {
        std::swap( A, B );
    }

    SEG Reversed() const
    {
        return SEG( B, A );
    }

    ///< Returns the center point of the line
    VECTOR2I Center() const
    {
        return A + ( B - A ) / 2;
    }

    bool operator<( const SEG& aSeg ) const
    {
        if( A == aSeg.A )
            return B < aSeg.B;

        return A < aSeg.A;
    }

private:

    bool checkCollinearOverlap( const SEG& aSeg, bool useXAxis, bool aIgnoreEndpoints, VECTOR2I* aPt ) const;
    bool intersects( const SEG& aSeg, bool aIgnoreEndpoints = false, bool aLines = false,
                     VECTOR2I* aPt = nullptr ) const;

    bool mutualDistanceSquared( const SEG& aSeg, ecoord& aD1, ecoord& aD2 ) const;

private:
    ///< index within the parent shape (used when m_is_local == false)
    int m_index;
};

inline SEG::ecoord SEG::TCoef( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    return d.Dot( aP - A);
}

inline std::ostream& operator<<( std::ostream& aStream, const SEG& aSeg )
{
    aStream << "[ " << aSeg.A << " - " << aSeg.B << " ]";

    return aStream;
}

#endif // __SEG_H
