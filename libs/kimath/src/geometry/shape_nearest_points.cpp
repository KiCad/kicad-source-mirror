/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the https://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cmath>
#include <limits>

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_simple.h>
#include <math/vector2d.h>

typedef VECTOR2I::extended_type ecoord;


static bool NearestPoints( const SHAPE_LINE_CHAIN_BASE& aA, const SHAPE_LINE_CHAIN_BASE& aB,
                           VECTOR2I& aPtA, VECTOR2I& aPtB );

static bool NearestPoints( const SEG& aSeg, const SHAPE_LINE_CHAIN_BASE& aChain,
                           VECTOR2I& aPtA, VECTOR2I& aPtB );

/**
 * Find the nearest points between two circles.
 *
 * @param aA first circle
 * @param aB second circle
 * @param aPtA [out] nearest point on first circle
 * @param aPtB [out] nearest point on second circle
 * @return true (circles always have nearest points)
 */
static bool NearestPoints( const SHAPE_CIRCLE& aA, const SHAPE_CIRCLE& aB,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const VECTOR2I delta = aB.GetCenter() - aA.GetCenter();
    const int dist = delta.EuclideanNorm();

    if( dist == 0 )
    {
        // Circles are concentric - pick arbitrary points
        aPtA = aA.GetCenter() + VECTOR2I( aA.GetRadius(), 0 );
        aPtB = aB.GetCenter() + VECTOR2I( aB.GetRadius(), 0 );
    }
    else
    {
        // Points lie on line between centers
        aPtA = aA.GetCenter() + delta.Resize( aA.GetRadius() );
        aPtB = aB.GetCenter() - delta.Resize( aB.GetRadius() );
    }

    return true;
}


/**
 * Find the nearest points between a circle and a rectangle.
 *
 * @param aCircle the circle
 * @param aRect the rectangle
 * @param aPtA [out] nearest point on circle
 * @param aPtB [out] nearest point on rectangle
 * @return true (always succeeds)
 */
static bool NearestPoints( const SHAPE_CIRCLE& aCircle, const SHAPE_RECT& aRect,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const VECTOR2I c = aCircle.GetCenter();
    const VECTOR2I p0 = aRect.GetPosition();
    const VECTOR2I size = aRect.GetSize();

    // Clamp circle center to rectangle bounds to find nearest point on rect
    aPtB.x = std::max( p0.x, std::min( c.x, p0.x + size.x ) );
    aPtB.y = std::max( p0.y, std::min( c.y, p0.y + size.y ) );

    // Find nearest point on circle
    if( aPtB == c )
    {
        // Center is inside rectangle - find nearest edge
        int distToLeft = c.x - p0.x;
        int distToRight = p0.x + size.x - c.x;
        int distToTop = c.y - p0.y;
        int distToBottom = p0.y + size.y - c.y;

        int minDist = std::min( { distToLeft, distToRight, distToTop, distToBottom } );

        if( minDist == distToLeft )
        {
            aPtB = VECTOR2I( p0.x, c.y );
            aPtA = c - VECTOR2I( aCircle.GetRadius(), 0 );
        }
        else if( minDist == distToRight )
        {
            aPtB = VECTOR2I( p0.x + size.x, c.y );
            aPtA = c + VECTOR2I( aCircle.GetRadius(), 0 );
        }
        else if( minDist == distToTop )
        {
            aPtB = VECTOR2I( c.x, p0.y );
            aPtA = c - VECTOR2I( 0, aCircle.GetRadius() );
        }
        else
        {
            aPtB = VECTOR2I( c.x, p0.y + size.y );
            aPtA = c + VECTOR2I( 0, aCircle.GetRadius() );
        }
    }
    else
    {
        VECTOR2I dir = ( aPtB - c ).Resize( aCircle.GetRadius() );
        aPtA = c + dir;
    }

    return true;
}


/**
 * Find the nearest points between a circle and a line segment.
 *
 * @param aCircle the circle
 * @param aSeg the segment
 * @param aPtA [out] nearest point on circle
 * @param aPtB [out] nearest point on segment
 * @return true (always succeeds)
 */
static bool NearestPoints( const SHAPE_CIRCLE& aCircle, const SEG& aSeg,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    aPtB = aSeg.NearestPoint( aCircle.GetCenter() );

    if( aPtB == aCircle.GetCenter() )
    {
        // Center is on segment - pick perpendicular direction
        VECTOR2I dir = ( aSeg.B - aSeg.A ).Perpendicular().Resize( aCircle.GetRadius() );
        aPtA = aCircle.GetCenter() + dir;
    }
    else
    {
        VECTOR2I dir = ( aPtB - aCircle.GetCenter() ).Resize( aCircle.GetRadius() );
        aPtA = aCircle.GetCenter() + dir;
    }

    return true;
}


/**
 * Find the nearest points between a circle and a line chain.
 */
static bool NearestPoints( const SHAPE_CIRCLE& aCircle, const SHAPE_LINE_CHAIN_BASE& aChain,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN* chain = dynamic_cast<const SHAPE_LINE_CHAIN*>( &aChain );
    int64_t minDistSq = std::numeric_limits<int64_t>::max();

    for( size_t i = 0; i < aChain.GetSegmentCount(); i++ )
    {
        if( chain && chain->IsArcSegment( i ) )
            continue;

        VECTOR2I ptA, ptB;
        if( NearestPoints( aCircle, aChain.GetSegment( i ), ptA, ptB ) )
        {
            int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();
            if( distSq < minDistSq )
            {
                minDistSq = distSq;
                aPtA = ptA;
                aPtB = ptB;
            }
        }
    }

    // Also handle arcs if this is a SHAPE_LINE_CHAIN
    if( chain )
    {
        for( size_t j = 0; j < chain->ArcCount(); j++ )
        {
            const SHAPE_ARC& arc = chain->Arc( j );

            VECTOR2I ptA, ptB;
            int64_t distSq;

            // Reverse the output points to match the arc_from_chain/circle order
            if( arc.NearestPoints( aCircle, ptB, ptA, distSq ) )
            {
                if( distSq < minDistSq )
                {
                    minDistSq = distSq;
                    aPtA = ptA;
                    aPtB = ptB;
                }
            }
        }
    }

    return minDistSq < std::numeric_limits<int64_t>::max();
}


/**
 * Find the nearest points between two rectangles.
 */
static bool NearestPoints( const SHAPE_RECT& aA, const SHAPE_RECT& aB,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    // Convert rectangles to line chains and use that algorithm
    const SHAPE_LINE_CHAIN outlineA = aA.Outline();
    const SHAPE_LINE_CHAIN outlineB = aB.Outline();

    return NearestPoints( outlineA, outlineB, aPtA, aPtB );
}


/**
 * Find the nearest points between a rectangle and a segment.
 */
static bool NearestPoints( const SHAPE_RECT& aRect, const SEG& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN outline = aRect.Outline();

    // Reverse the output points to match the seg/rect_outline order
    return NearestPoints( aSeg, outline, aPtB, aPtA );
}


/**
 * Find the nearest points between a rectangle and a line chain.
 */
static bool NearestPoints( const SHAPE_RECT& aRect, const SHAPE_LINE_CHAIN_BASE& aChain,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN outline = aRect.Outline();

    return NearestPoints( outline, aChain, aPtA, aPtB );
}


/**
 * Find the nearest points between two segments.
 */
static bool NearestPoints( const SEG& aA, const SEG& aB,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    aPtA = aA.NearestPoint( aB );
    aPtB = aB.NearestPoint( aPtA );

    return true;
}


/**
 * Find the nearest points between a segment and a line chain.
 */
static bool NearestPoints( const SEG& aSeg, const SHAPE_LINE_CHAIN_BASE& aChain,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN* chain = dynamic_cast<const SHAPE_LINE_CHAIN*>( &aChain );
    int64_t minDistSq = std::numeric_limits<int64_t>::max();

    for( size_t i = 0; i < aChain.GetSegmentCount(); i++ )
    {
        if( chain && chain->IsArcSegment( i ) )
            continue;

        VECTOR2I ptA, ptB;

        if( NearestPoints( aSeg, aChain.GetSegment( i ), ptA, ptB ) )
        {
            int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();
            if( distSq < minDistSq )
            {
                minDistSq = distSq;
                aPtA = ptA;
                aPtB = ptB;
            }
        }
    }

    // Also handle arcs if this is a SHAPE_LINE_CHAIN
    if( chain )
    {
        for( size_t j = 0; j < chain->ArcCount(); j++ )
        {
            const SHAPE_ARC& arc = chain->Arc( j );

            VECTOR2I ptA, ptB;
            int64_t distSq;

            // Reverse the output points to match the arc_from_chain/seg order
            if( arc.NearestPoints( aSeg, ptB, ptA, distSq ) )
            {
                if( distSq < minDistSq )
                {
                    minDistSq = distSq;
                    aPtA = ptA;
                    aPtB = ptB;
                }
            }
        }
    }

    return minDistSq < std::numeric_limits<int64_t>::max();
}


/**
 * Find the nearest points between two line chains.
 */
static bool NearestPoints( const SHAPE_LINE_CHAIN_BASE& aA, const SHAPE_LINE_CHAIN_BASE& aB,
                          VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN* chainA = dynamic_cast<const SHAPE_LINE_CHAIN*>( &aA );
    const SHAPE_LINE_CHAIN* chainB = dynamic_cast<const SHAPE_LINE_CHAIN*>( &aB );
    int64_t minDistSq = std::numeric_limits<int64_t>::max();

    // Check all segment pairs
    for( size_t i = 0; i < aA.GetSegmentCount(); i++ )
    {
        if( chainA && chainA->IsArcSegment( i ) )
            continue;

        for( size_t j = 0; j < aB.GetSegmentCount(); j++ )
        {
            if( chainB && chainB->IsArcSegment( j ) )
                continue;

            VECTOR2I ptA, ptB;
            if( NearestPoints( aA.GetSegment( i ), aB.GetSegment( j ), ptA, ptB ) )
            {
                int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();
                if( distSq < minDistSq )
                {
                    minDistSq = distSq;
                    aPtA = ptA;
                    aPtB = ptB;
                }
            }
        }
    }

    // Also handle arcs if this is a SHAPE_LINE_CHAIN
    if( chainA )
    {
        for( size_t i = 0; i < chainA->ArcCount(); i++ )
        {
            const SHAPE_ARC& arcA = chainA->Arc( i );

            if( chainB )
            {
                // Arc to arc
                for( size_t j = 0; j < chainB->ArcCount(); j++ )
                {
                    VECTOR2I ptA, ptB;
                    int64_t distSq;
                    if( arcA.NearestPoints( chainB->Arc( j ), ptA, ptB, distSq ) )
                    {
                        if( distSq < minDistSq )
                        {
                            minDistSq = distSq;
                            aPtA = ptA;
                            aPtB = ptB;
                        }
                    }
                }
            }

            // Arc to segments
            for( size_t j = 0; j < aB.GetSegmentCount(); j++ )
            {
                VECTOR2I ptA, ptB;
                int64_t distSq;
                if( arcA.NearestPoints( aB.GetSegment( j ), ptA, ptB, distSq ) )
                {
                    if( distSq < minDistSq )
                    {
                        minDistSq = distSq;
                        aPtA = ptA;
                        aPtB = ptB;
                    }
                }
            }
        }
    }

    if( chainB && !chainA )
    {
        // Handle arcs in chainB vs segments in aA
        for( size_t j = 0; j < chainB->ArcCount(); j++ )
        {
            const SHAPE_ARC& arcB = chainB->Arc( j );

            for( size_t i = 0; i < aA.GetSegmentCount(); i++ )
            {
                VECTOR2I ptA, ptB;
                int64_t distSq;
                if( arcB.NearestPoints( aA.GetSegment( i ), ptB, ptA, distSq ) )
                {
                    if( distSq < minDistSq )
                    {
                        minDistSq = distSq;
                        aPtA = ptA;
                        aPtB = ptB;
                    }
                }
            }
        }
    }

    return minDistSq < std::numeric_limits<int64_t>::max();
}


/**
 * Find the nearest points between an arc and other shapes.
 * Uses the arc's built-in NearestPoints methods.
 */
static bool NearestPoints( const SHAPE_ARC& aArc, const SHAPE_CIRCLE& aCircle, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    int64_t distSq;
    return aArc.NearestPoints( aCircle, aPtA, aPtB, distSq );
}

static bool NearestPoints( const SHAPE_ARC& aArc, const SHAPE_RECT& aRect, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    int64_t distSq;
    return aArc.NearestPoints( aRect, aPtA, aPtB, distSq );
}

static bool NearestPoints( const SHAPE_ARC& aArc, const SHAPE_SEGMENT& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    int64_t distSq;
    bool    retVal = aArc.NearestPoints( aSeg.GetSeg(), aPtA, aPtB, distSq );

    // Adjust point B by half the seg width towards point A
    VECTOR2I dir = ( aPtA - aPtB ).Resize( aSeg.GetWidth() / 2 );
    aPtB += dir;

    return retVal;
}

static bool NearestPoints( const SHAPE_ARC& aArcA, const SHAPE_ARC& aArcB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    int64_t distSq;
    return aArcA.NearestPoints( aArcB, aPtA, aPtB, distSq );
}

static bool NearestPoints( const SHAPE_ARC& aArc, const SHAPE_LINE_CHAIN_BASE& aChain, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    const SHAPE_LINE_CHAIN* chain = dynamic_cast<const SHAPE_LINE_CHAIN*>( &aChain );
    int64_t distSq;
    int64_t minDistSq = std::numeric_limits<int64_t>::max();
    VECTOR2I tmp_ptA, tmp_ptB;

    for( size_t i = 0; i < aChain.GetSegmentCount(); i++ )
    {
        if( chain && chain->IsArcSegment( i ) )
            continue;

        if( aArc.NearestPoints( aChain.GetSegment( i ), tmp_ptA, tmp_ptB, distSq ) )
        {
            if( distSq < minDistSq )
            {
                aPtA = tmp_ptA;
                aPtB = tmp_ptB;
                minDistSq = distSq;
            }
        }
    }

    // Also handle arcs if this is a SHAPE_LINE_CHAIN
    if( chain )
    {
        for( size_t j = 0; j < chain->ArcCount(); j++ )
        {
            const SHAPE_ARC& arc = chain->Arc( j );

            if( aArc.NearestPoints( arc, tmp_ptA, tmp_ptB, distSq ) )
            {
                if( distSq < minDistSq )
                {
                    minDistSq = distSq;
                    aPtA = tmp_ptA;
                    aPtB = tmp_ptB;
                }
            }
        }
    }

    return true;
}


/**
 * Find nearest points between SHAPE_SEGMENT and other shapes
 */
static bool NearestPoints( const SHAPE_SEGMENT& aSeg, const SHAPE_CIRCLE& aCircle, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    if( NearestPoints( aCircle, aSeg.GetSeg(), aPtB, aPtA ) )
    {
        // Adjust point A by half the segment width towards point B
        VECTOR2I dir = ( aPtB - aPtA ).Resize( aSeg.GetWidth() / 2 );
        aPtA += dir;
        return true;
    }

    return false;
}

static bool NearestPoints( const SHAPE_SEGMENT& aSeg, const SHAPE_RECT& aRect, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    if( NearestPoints( aRect, aSeg.GetSeg(), aPtB, aPtA ) )
    {
        // Adjust point A by half the segment width towards point B
        VECTOR2I dir = ( aPtB - aPtA ).Resize( aSeg.GetWidth() / 2 );
        aPtA += dir;
        return true;
    }

    return false;
}

static bool NearestPoints( const SHAPE_SEGMENT& aSegA, const SHAPE_SEGMENT& aSegB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    // Find nearest points between two segments
    if( NearestPoints( aSegA.GetSeg(), aSegB.GetSeg(), aPtA, aPtB ) )
    {
        // Adjust point A by half the segment width towards point B
        VECTOR2I dir = ( aPtB - aPtA ).Resize( aSegA.GetWidth() / 2 );
        aPtA += dir;
        // Adjust point B by half the segment width towards point A
        dir = ( aPtA - aPtB ).Resize( aSegB.GetWidth() / 2 );
        aPtB += dir;
        return true;
    }

    return false;
}

static bool NearestPoints( const SHAPE_SEGMENT& aSeg, const SHAPE_LINE_CHAIN_BASE& aChain, VECTOR2I& aPtA,
                           VECTOR2I& aPtB )
{
    if( NearestPoints( aSeg.GetSeg(), aChain, aPtA, aPtB ) )
    {
        // Adjust point A by half the segment width towards point B
        VECTOR2I dir = ( aPtB - aPtA ).Resize( aSeg.GetWidth() / 2 );
        aPtA += dir;
        return true;
    }

    return false;
}


/**
 * Template functions to handle shape conversions and reversals
 */
template<class T_a, class T_b>
inline bool NearestPointsCase( const SHAPE* aA, const SHAPE* aB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    return NearestPoints( *static_cast<const T_a*>( aA ), *static_cast<const T_b*>( aB ),
                          aPtA, aPtB );
}

template<class T_a, class T_b>
inline bool NearestPointsCaseReversed( const SHAPE* aA, const SHAPE* aB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    return NearestPoints( *static_cast<const T_b*>( aB ), *static_cast<const T_a*>( aA ),
                          aPtB, aPtA );
}


/**
 * Main dispatcher for finding nearest points between arbitrary shapes
 */
static bool nearestPointsSingleShapes( const SHAPE* aA, const SHAPE* aB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    switch( aA->Type() )
    {
    case SH_RECT:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCase<SHAPE_RECT, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCaseReversed<SHAPE_RECT, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCase<SHAPE_RECT, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCaseReversed<SHAPE_RECT, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_RECT, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            return NearestPointsCaseReversed<SHAPE_RECT, SHAPE_ARC>( aA, aB, aPtA, aPtB );
        default:
            break;
        }
        break;

    case SH_CIRCLE:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCase<SHAPE_CIRCLE, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCase<SHAPE_CIRCLE, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCaseReversed<SHAPE_CIRCLE, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_CIRCLE, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            return NearestPointsCaseReversed<SHAPE_CIRCLE, SHAPE_ARC>( aA, aB, aPtA, aPtB );
        default:
            break;
        }
        break;

    case SH_LINE_CHAIN:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_LINE_CHAIN, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            // Special handling for arc
            {
                const SHAPE_LINE_CHAIN* chain = static_cast<const SHAPE_LINE_CHAIN*>( aA );
                const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aB );
                int64_t minDistSq = std::numeric_limits<int64_t>::max();

                // Check segments
                for( int i = 0; i < chain->SegmentCount(); i++ )
                {
                    VECTOR2I ptA, ptB;
                    int64_t distSq;

                    // Reverse the output points to match the arc/segment_from_rect order
                    if( arc->NearestPoints( chain->CSegment( i ), ptB, ptA, distSq ) )
                    {
                        if( distSq < minDistSq )
                        {
                            minDistSq = distSq;
                            aPtA = ptA;
                            aPtB = ptB;
                        }
                    }
                }

                // Check arcs
                for( size_t i = 0; i < chain->ArcCount(); i++ )
                {
                    VECTOR2I ptA, ptB;
                    int64_t distSq;
                    if( chain->Arc( i ).NearestPoints( *arc, ptA, ptB, distSq ) )
                    {
                        if( distSq < minDistSq )
                        {
                            minDistSq = distSq;
                            aPtA = ptA;
                            aPtB = ptB;
                        }
                    }
                }

                return minDistSq < std::numeric_limits<int64_t>::max();
            }
        default:
            break;
        }
        break;

    case SH_SEGMENT:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCase<SHAPE_SEGMENT, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCase<SHAPE_SEGMENT, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCase<SHAPE_SEGMENT, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCase<SHAPE_SEGMENT, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_SEGMENT, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            return NearestPointsCaseReversed<SHAPE_SEGMENT, SHAPE_ARC>( aA, aB, aPtA, aPtB );
        default:
            break;
        }
        break;

    case SH_SIMPLE:
    case SH_POLY_SET_TRIANGLE:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN_BASE, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN_BASE, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN_BASE, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCaseReversed<SHAPE_LINE_CHAIN_BASE, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_LINE_CHAIN_BASE, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            // Handle arc specially
            {
                const SHAPE_LINE_CHAIN_BASE* chain = static_cast<const SHAPE_LINE_CHAIN_BASE*>( aA );
                const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( aB );
                int64_t minDistSq = std::numeric_limits<int64_t>::max();

                for( size_t i = 0; i < chain->GetSegmentCount(); i++ )
                {
                    VECTOR2I ptA, ptB;
                    int64_t distSq;

                    // Reverse the output points to match the arc/segment_from_line_chain order
                    if( arc->NearestPoints( chain->GetSegment( i ), ptB, ptA, distSq ) )
                    {
                        if( distSq < minDistSq )
                        {
                            minDistSq = distSq;
                            aPtA = ptA;
                            aPtB = ptB;
                        }
                    }
                }

                return minDistSq < std::numeric_limits<int64_t>::max();
            }
        default:
            break;
        }
        break;

    case SH_ARC:
        switch( aB->Type() )
        {
        case SH_RECT:
            return NearestPointsCase<SHAPE_ARC, SHAPE_RECT>( aA, aB, aPtA, aPtB );
        case SH_CIRCLE:
            return NearestPointsCase<SHAPE_ARC, SHAPE_CIRCLE>( aA, aB, aPtA, aPtB );
        case SH_LINE_CHAIN:
            return NearestPointsCase<SHAPE_ARC, SHAPE_LINE_CHAIN>( aA, aB, aPtA, aPtB );
        case SH_SEGMENT:
            return NearestPointsCase<SHAPE_ARC, SHAPE_SEGMENT>( aA, aB, aPtA, aPtB );
        case SH_SIMPLE:
        case SH_POLY_SET_TRIANGLE:
            return NearestPointsCase<SHAPE_ARC, SHAPE_LINE_CHAIN_BASE>( aA, aB, aPtA, aPtB );
        case SH_ARC:
            return NearestPointsCase<SHAPE_ARC, SHAPE_ARC>( aA, aB, aPtA, aPtB );
        default:
            break;
        }
        break;

    case SH_POLY_SET:
        // For polygon sets, find nearest points to all edges
        {
            const SHAPE_POLY_SET* polySet = static_cast<const SHAPE_POLY_SET*>( aA );
            int64_t minDistSq = std::numeric_limits<int64_t>::max();

            for( auto it = polySet->CIterateSegmentsWithHoles(); it; ++it )
            {
                SHAPE_SEGMENT seg( *it );
                VECTOR2I ptA, ptB;

                if( nearestPointsSingleShapes( &seg, aB, ptA, ptB ) )
                {
                    int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();
                    if( distSq < minDistSq )
                    {
                        minDistSq = distSq;
                        aPtA = ptA;
                        aPtB = ptB;
                    }
                }
            }

            return minDistSq < std::numeric_limits<int64_t>::max();
        }
        break;

    default:
        break;
    }

    // Handle SHAPE_POLY_SET as second shape
    if( aB->Type() == SH_POLY_SET )
    {
        const SHAPE_POLY_SET* polySet = static_cast<const SHAPE_POLY_SET*>( aB );
        int64_t minDistSq = std::numeric_limits<int64_t>::max();

        for( auto it = polySet->CIterateSegments(); it; ++it )
        {
            SHAPE_SEGMENT seg( *it );
            VECTOR2I ptA, ptB;

            if( nearestPointsSingleShapes( aA, &seg, ptA, ptB ) )
            {
                int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();
                if( distSq < minDistSq )
                {
                    minDistSq = distSq;
                    aPtA = ptA;
                    aPtB = ptB;
                }
            }
        }

        return minDistSq < std::numeric_limits<int64_t>::max();
    }

    return false;
}


/**
 * Handle compound shapes by finding nearest points between all sub-shape pairs
 */
static bool nearestPoints( const SHAPE* aA, const SHAPE* aB, VECTOR2I& aPtA, VECTOR2I& aPtB )
{
    int64_t minDistSq = std::numeric_limits<int64_t>::max();
    bool found = false;

    auto checkNearestPoints =
            [&]( const SHAPE* shapeA, const SHAPE* shapeB ) -> bool
            {
                VECTOR2I ptA, ptB;

                if( nearestPointsSingleShapes( shapeA, shapeB, ptA, ptB ) )
                {
                    int64_t distSq = ( ptB - ptA ).SquaredEuclideanNorm();

                    if( distSq < minDistSq )
                    {
                        minDistSq = distSq;
                        aPtA = ptA;
                        aPtB = ptB;
                        found = true;
                    }

                    return true;
                }

                return false;
            };

    if( aA->Type() == SH_COMPOUND && aB->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpA = static_cast<const SHAPE_COMPOUND*>( aA );
        const SHAPE_COMPOUND* cmpB = static_cast<const SHAPE_COMPOUND*>( aB );

        for( const SHAPE* elemA : cmpA->Shapes() )
        {
            for( const SHAPE* elemB : cmpB->Shapes() )
            {
                checkNearestPoints( elemA, elemB );
            }
        }
    }
    else if( aA->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpA = static_cast<const SHAPE_COMPOUND*>( aA );

        for( const SHAPE* elemA : cmpA->Shapes() )
        {
            checkNearestPoints( elemA, aB );
        }
    }
    else if( aB->Type() == SH_COMPOUND )
    {
        const SHAPE_COMPOUND* cmpB = static_cast<const SHAPE_COMPOUND*>( aB );

        for( const SHAPE* elemB : cmpB->Shapes() )
        {
            checkNearestPoints( aA, elemB );
        }
    }
    else
    {
        return nearestPointsSingleShapes( aA, aB, aPtA, aPtB );
    }

    return found;
}


/**
 * Public interface for finding nearest points between two shapes.
 *
 * @param aA first shape
 * @param aB second shape
 * @param aPtA [out] nearest point on first shape
 * @param aPtB [out] nearest point on second shape
 * @return true if nearest points were found, false otherwise
 */
bool SHAPE::NearestPoints( const SHAPE* aOther, VECTOR2I& aPtThis, VECTOR2I& aPtOther ) const
{
    return nearestPoints( this, aOther, aPtThis, aPtOther );
}