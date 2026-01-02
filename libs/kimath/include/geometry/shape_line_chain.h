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

#ifndef __SHAPE_LINE_CHAIN
#define __SHAPE_LINE_CHAIN


#include <clipper2/clipper.h>
#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/corner_strategy.h>
#include <math/vector2d.h>

/**
 * Holds information on each point of a SHAPE_LINE_CHAIN that is retrievable
 * after an operation with Clipper2Lib
 */
struct CLIPPER_Z_VALUE
{
    CLIPPER_Z_VALUE()
    {
        m_FirstArcIdx = -1;
        m_SecondArcIdx = -1;
    }

    CLIPPER_Z_VALUE( const std::pair<ssize_t, ssize_t> aShapeIndices, ssize_t aOffset = 0 )
    {
        m_FirstArcIdx = aShapeIndices.first;
        m_SecondArcIdx = aShapeIndices.second;

        auto offsetVal = [&]( ssize_t& aVal )
                         {
                             if( aVal >= 0 )
                                 aVal += aOffset;
                         };

        offsetVal( m_FirstArcIdx );
        offsetVal( m_SecondArcIdx );
    }

    ssize_t m_FirstArcIdx;
    ssize_t m_SecondArcIdx;
};


/**
 * Represent a polyline containing arcs as well as line segments: A chain of connected line and/or
 * arc segments.
 *
 * The arc shapes are piecewise approximated for the purpose of boolean operations but are used as
 * arcs when doing collision checks.
 *
 * It is purposely not named "polyline" to avoid confusion with the existing CPolyLine
 * in Pcbnew.
 *
 * @note The SHAPE_LINE_CHAIN class shall not be used for polygons!
 */
class SHAPE_LINE_CHAIN : public SHAPE_LINE_CHAIN_BASE
{
public:
    typedef std::vector<VECTOR2I>::iterator point_iter;
    typedef std::vector<VECTOR2I>::const_iterator point_citer;

    /**
     * Represent an intersection between two line segments
     */
    struct INTERSECTION
    {
        /// Point of intersection between our and their.
        VECTOR2I p;

        /// Index of the intersecting corner/segment in the 'our' (== this) line.
        int index_our;

        /// index of the intersecting corner/segment in the 'their' (Intersect() method
        /// parameter) line.
        int index_their;

        /// When true, the corner [index_our] of the 'our' line lies exactly on 'their' line.
        bool is_corner_our;

        /// When true, the corner [index_their] of the 'their' line lies exactly on 'our' line.
        /// Note that when both is_corner_our and is_corner_their are set, the line chains touch
        /// with with corners.
        bool is_corner_their;

        /// Auxiliary flag to avoid copying intersection info to intersection refining code,
        /// used by the refining code (e.g. hull handling stuff in the P&S) to reject false
        /// intersection points.
        bool valid;

        INTERSECTION() :
            index_our( -1 ),
            index_their( -1 ),
            is_corner_our( false ),
            is_corner_their( false ),
            valid( false )
        {
        }
    };


    /**
     * A dynamic state checking if a point lies within polygon with a dynamically built outline (
     * with each piece of the outline added by AddPolyline ()
     */
    class POINT_INSIDE_TRACKER
    {
    public:
        POINT_INSIDE_TRACKER( const VECTOR2I& aPoint );

        void AddPolyline( const SHAPE_LINE_CHAIN& aPolyline );
        bool IsInside();

    private:

        bool processVertex ( const VECTOR2I& ip, const VECTOR2I& ipNext );

        VECTOR2I m_point;
        VECTOR2I m_lastPoint;
        VECTOR2I m_firstPoint;
        bool m_finished;
        int m_state;
        int m_count;
    };

    typedef std::vector<INTERSECTION> INTERSECTIONS;


    /**
     * Initialize an empty line chain.
     */
    SHAPE_LINE_CHAIN() :
            SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
            m_accuracy( 0 ),
            m_closed( false ),
            m_width( 0 )
    {}

    SHAPE_LINE_CHAIN( const SHAPE_LINE_CHAIN& aShape ) :
            SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
            m_points( aShape.m_points ),
            m_shapes( aShape.m_shapes ),
            m_arcs( aShape.m_arcs ),
            m_accuracy( aShape.m_accuracy ),
            m_closed( aShape.m_closed ),
            m_width( aShape.m_width ),
            m_bbox( aShape.m_bbox )
    {}

    SHAPE_LINE_CHAIN( const std::vector<int>& aV );

    SHAPE_LINE_CHAIN( const std::vector<VECTOR2I>& aV, bool aClosed = false );

    SHAPE_LINE_CHAIN( const SHAPE_ARC& aArc, bool aClosed = false, std::optional<int> aMaxError = {} );

    SHAPE_LINE_CHAIN( const Clipper2Lib::Path64& aPath,
                      const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                      const std::vector<SHAPE_ARC>& aArcBuffer );

    virtual ~SHAPE_LINE_CHAIN()
    {}

    /**
     * Check if point \a aP lies closer to us than \a aClearance.
     *
     * Note: This is overridden as we want to ensure we test collisions with the arcs in this chain
     * as true arcs rather than segment approximations.
     *
     * @param aP the point to check for collisions with
     * @param aClearance minimum distance that does not qualify as a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true, when a collision has been found
     */
    virtual bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Check if segment \a aSeg lies closer to us than \a aClearance.
     *
     * Note: This is overridden as we want to ensure we test collisions with the arcs in this chain
     * as true arcs rather than segment approximations.
     *
     * @param aSeg the segment to check for collisions with
     * @param aClearance minimum distance that does not qualify as a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true, when a collision has been found
     */
    virtual bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                          VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Finds closest points between this and the other line chain. Doesn't test segments or arcs.
     *
     * @param aOther the line chain to test against.
     * @param aPt0 closest point on this line chain (output).
     * @param aPt1 closest point on the other line chain (output).
     * @param aDistance distance between points (output).
     * @return true, if the operation was successful.
     */
    bool ClosestPoints( const SHAPE_LINE_CHAIN& aOther, VECTOR2I& aPt0, VECTOR2I& aPt1 ) const;

    static bool ClosestPoints( const point_citer& aMyStart, const point_citer& aMyEnd,
                               const point_citer& aOtherStart, const point_citer& aOtherEnd,
                               VECTOR2I& aPt0, VECTOR2I& aPt1, int64_t& aDistSq );

    static bool ClosestSegments( const VECTOR2I& aMyPrevPt, const point_citer& aMyStart,
                                 const point_citer& aMyEnd, const VECTOR2I& aOtherPrevPt,
                                 const point_citer& aOtherStart, const point_citer& aOtherEnd,
                                 VECTOR2I& aPt0, VECTOR2I& aPt1, int64_t& aDistSq );

    /**
     * Finds closest points between segments of this and the other line chain. Doesn't guarantee
     * that the points are the absolute closest (use ClosestSegments for that) as there might
     * be edge cases, but it is much faster.
     *
     * @param aOther the line chain to test against.
     * @param aPt0 closest point on this line chain (output).
     * @param aPt1 closest point on the other line chain (output).
     * @param aDistance distance between points (output).
     * @return true, if the operation was successful.
     */
    bool ClosestSegmentsFast( const SHAPE_LINE_CHAIN& aOther, VECTOR2I& aPt0,
                              VECTOR2I& aPt1 ) const;

    SHAPE_LINE_CHAIN& operator=( const SHAPE_LINE_CHAIN& ) = default;

    // Move assignment operator
    SHAPE_LINE_CHAIN& operator=( SHAPE_LINE_CHAIN&& aOther ) noexcept
    {
        if (this != &aOther)
        {
            SHAPE_LINE_CHAIN_BASE::operator=( aOther );

            m_points = std::move( aOther.m_points );
            m_shapes = std::move( aOther.m_shapes );
            m_arcs = std::move( aOther.m_arcs );

            m_accuracy = aOther.m_accuracy;
            m_closed = aOther.m_closed;
            m_width = aOther.m_width;
            m_bbox = aOther.m_bbox;
        }

        return *this;
    }

    SHAPE* Clone() const override;

    /**
     * Remove all points from the line chain.
     */
    void Clear()
    {
        m_points.clear();
        m_arcs.clear();
        m_shapes.clear();
        m_closed = false;
    }

    /**
     * Mark the line chain as closed (i.e. with a segment connecting the last point with
     * the first point).
     *
     * @param aClosed: whether the line chain is to be closed or not.
     */
    void SetClosed( bool aClosed )
    {
        m_closed = aClosed;
        mergeFirstLastPointIfNeeded();
    }

    /**
     * @return true when our line is closed.
     */
    bool IsClosed() const override
    {
        return m_closed;
    }

    /**
     * Set the width of all segments in the chain.
     *
     * @param aWidth is the width in internal units.
     */
    void SetWidth( int aWidth ) override
    {
        m_width = aWidth;
    }

    /**
     * Get the current width of the segments in the chain.
     *
     * @return the width in internal units.
     */
    int Width() const
    {
        return m_width;
    }

    /**
     * Return the number of segments in this line chain.
     *
     * @return the number of segments.
     */
    int SegmentCount() const
    {
        int c = m_points.size() - 1;

        if( m_closed )
            c++;

        return std::max( 0, c );
    }

    /**
     * Return the number of shapes (line segments or arcs) in this line chain.
     *
     * This is kind of like SegmentCount() but will only count arcs as 1 segment.
     *
     * @return ArcCount() + the number of non-arc segments.
     */
    int ShapeCount() const;


    /**
     * Remove the duplicate points from the line chain.
    */
    void RemoveDuplicatePoints();

    /**
     * Simplify the line chain by removing colinear adjacent segments and duplicate vertices.
     *
     * @param aTolerance is the maximum tolerance in internal units.  Setting to 0 means that the
     * points must be exactly co-linear to be removed.
     */
    void Simplify( int aTolerance = 0 );

    // legacy function, used by the router. Please do not remove until I'll figure out
    // the root cause of rounding errors - Tom
    SHAPE_LINE_CHAIN& Simplify2( bool aRemoveColinear = true );

    /**
     * Return the number of points (vertices) in this line chain.
     *
     * @return the number of points.
     */
    int PointCount() const
    {
        return m_points.size();
    }

    /**
     * Return a copy of the aIndex-th segment in the line chain.
     *
     * @param aIndex is the index of the segment in the line chain. Negative values are counted
     *        from the end (i.e. -1 means the last segment in the line chain).
     * @return a segment at the \a aIndex in the line chain.
     */
    SEG Segment( int aIndex ) const;

    /**
     * Return a constant copy of the \a aIndex segment in the line chain.
     *
     * @param aIndex is the index of the segment in the line chain. Negative values are counted
     *        from the end (i.e. -1 means the last segment in the line chain).
     * @return a segment at \a aIndex in the line chain.
     */
    const SEG CSegment( int aIndex ) const { return Segment( aIndex ); }

    /**
     * Return the vertex index of the next shape in the chain, or -1 if \a aPointIndex is the
     * last shape.
     *
     * If \a aPointIndex is the start of a segment, this will be ( aPointIndex + 1 ).  If
     * \a aPointIndex is part of an arc, this will be the index of the start of the next shape after
     * the arc, in other words, the last point of the arc.
     *
     * @param aPointIndex is a vertex in the chain.
     * @return the vertex index of the start of the next shape after aPoint's shape or -1 if
     * the end was reached.
     */
    int NextShape( int aPointIndex ) const;

    /**
     * Move a point to a specific location.
     *
     * @param aIndex is the index of the point to move.  Negative indexes are from the back.
     * @param aPos is the new absolute location of the point.
     */
    void SetPoint( int aIndex, const VECTOR2I& aPos );

    /**
     * Return a reference to a given point in the line chain.
     *
     * @param aIndex is the index of the point.  Negative indexes are from the back.
     * @return a const reference to the point.
     */
    const VECTOR2I& CPoint( int aIndex ) const
    {
        if( aIndex < 0 )
            aIndex += PointCount();
        else if( aIndex >= PointCount() )
            aIndex -= PointCount();

        return m_points[aIndex];
    }

    const std::vector<VECTOR2I>& CPoints() const { return m_points; }

    /**
     * Return the last point in the line chain.
     */
    const VECTOR2I& CLastPoint() const
    {
        return m_points[static_cast<size_t>( PointCount() ) - 1];
    }

    /**
     * @return the vector of stored arcs.
     */
    const std::vector<SHAPE_ARC>& CArcs() const
    {
        return m_arcs;
    }

    /**
     * @return the vector of values indicating shape type and location.
     */
    const std::vector<std::pair<ssize_t, ssize_t>>& CShapes() const
    {
        return m_shapes;
    }

    /// @copydoc SHAPE::BBox()
    const BOX2I BBox( int aClearance = 0 ) const override
    {
        BOX2I bbox;
        bbox.Compute( m_points );

        if( aClearance != 0 || m_width != 0 )
            bbox.Inflate( aClearance + m_width );

        return bbox;
    }

    void GenerateBBoxCache() const
    {
        m_bbox.Compute( m_points );

        if( m_width != 0 )
            m_bbox.Inflate( m_width );
    }

    BOX2I* GetCachedBBox() const override
    {
        return &m_bbox;
    }

    /**
     * Reverse point order in the line chain.
     *
     * @return line chain with reversed point order (original A-B-C-D: returned D-C-B-A).
     */
    const SHAPE_LINE_CHAIN Reverse() const;

    /**
     * Remove all arc references in the line chain, resulting in a chain formed
     * only of straight segments. Any arcs in the chain are removed and only the
     * piecewise linear approximation remains.
     */
    void ClearArcs();

    /**
     * Return length of the line chain in Euclidean metric.
     *
     * @return the length of the line chain.
     */
    long long int Length() const;

    /**
     * Allocate a number of points all at once (for performance).
     */
    void ReservePoints( size_t aSize )
    {
        m_points.reserve( aSize );
        m_shapes.reserve( aSize );
    }

    /**
     * Append a new point at the end of the line chain.
     *
     * @param aX is X coordinate of the new point.
     * @param aY is Y coordinate of the new point.
     * @param aAllowDuplication set to true to append the new point even if it is the same as
     *        the last entered point, false (default) to skip it if it is the same as the last
     *        entered point.
     */
    void Append( int aX, int aY, bool aAllowDuplication = false )
    {
        VECTOR2I v( aX, aY );
        Append( v, aAllowDuplication );
    }

    /**
     * Append a new point at the end of the line chain.
     *
     * @param aP is the new point.
     * @param aAllowDuplication set to true to append the new point even it is the same as the
     *        last entered point or false (default) to skip it if it is the same as the last
     *        entered point.
     */
    void Append( const VECTOR2I& aP, bool aAllowDuplication = false )
    {
        if( m_points.size() == 0 )
            m_bbox = BOX2I( aP, VECTOR2I( 0, 0 ) );

        if( m_points.size() == 0 || aAllowDuplication || CLastPoint() != aP )
        {
            m_points.push_back( aP );
            m_shapes.push_back( SHAPES_ARE_PT );
            m_bbox.Merge( aP );
        }
    }

    /**
     * Append another line chain at the end.
     *
     * @param aOtherLine is the line chain to be appended.
     */
    void Append( const SHAPE_LINE_CHAIN& aOtherLine );

    void Append( const SHAPE_ARC& aArc );
    void Append( const SHAPE_ARC& aArc, int aMaxError );

    void Insert( size_t aVertex, const VECTOR2I& aP );

    void Insert( size_t aVertex, const SHAPE_ARC& aArc );
    void Insert( size_t aVertex, const SHAPE_ARC& aArc, int aMaxError );

    /**
     * Replace points with indices in range [start_index, end_index] with a single point \a aP.
     *
     * @param aStartIndex is the start of the point range to be replaced (inclusive).
     * @param aEndIndex is the end of the point range to be replaced (inclusive).
     * @param aP is the replacement point.
     */
    void Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP );

    /**
     * Replace points with indices in range [start_index, end_index] with the points from line
     * chain \a aLine.
     *
     * @param aStartIndex is the start of the point range to be replaced (inclusive).
     * @param aEndIndex is the end of the point range to be replaced (inclusive).
     * @param aLine is the replacement line chain.
     */
    void Replace( int aStartIndex, int aEndIndex, const SHAPE_LINE_CHAIN& aLine );

    /**
     * Remove the range of points [start_index, end_index] from the line chain.
     *
     * @param aStartIndex is the start of the point range to be replaced (inclusive).
     * @param aEndIndex is the end of the point range to be replaced (inclusive).
     */
    void Remove( int aStartIndex, int aEndIndex );

    /**
     * Remove the aIndex-th point from the line chain.
     *
     * @param aIndex is the index of the point to be removed.
     */
    void Remove( int aIndex )
    {
        Remove( aIndex, aIndex );
    }

    /**
     * Remove the shape at the given index from the line chain.
     *
     * If the given index is inside an arc, the entire arc will be removed.
     * Otherwise this is equivalent to Remove( aPointIndex ).
     *
     * @param aPointIndex is the index of the point to remove.
     */
    void RemoveShape( int aPointIndex );

    /**
     * Insert the point aP belonging to one of the our segments, splitting the adjacent segment
     * in two.
     * @param aP is the point to be inserted.
     * @param aExact set to skip the split logic when an exact point match exists.
     * @return index of the newly inserted point (or a negative value if aP does not lie on
     *         our line).
     */
    int Split( const VECTOR2I& aP, bool aExact = false );

    /**
     * Search for point \a aP.
     *
     * @param aP is the point to be looked for.
     * @return the index of the corresponding point in the line chain or negative when not found.
     */
    int Find( const VECTOR2I& aP, int aThreshold = 0 ) const;

    /**
     * Search for segment containing point \a aP.
     *
     * @param aP is the point to be looked for.
     * @return index of the corresponding segment in the line chain or negative when not found.
     */
    int FindSegment( const VECTOR2I& aP, int aThreshold = 1 ) const;

    /**
     * Return a subset of this line chain containing the [start_index, end_index] range of points.
     *
     * @param aStartIndex is the start of the point range to be returned (inclusive).
     * @param aEndIndex is the end of the point range to be returned (inclusive).
     * @return the cut line chain.
     */
    const SHAPE_LINE_CHAIN Slice( int aStartIndex, int aEndIndex ) const;
    const SHAPE_LINE_CHAIN Slice( int aStartIndex, int aEndIndex, int aMaxError ) const;

    struct compareOriginDistance
    {
        compareOriginDistance( const VECTOR2I& aOrigin ):
            m_origin( aOrigin )
        {}

        bool operator()( const INTERSECTION& aA, const INTERSECTION& aB )
        {
            return ( m_origin - aA.p ).EuclideanNorm() < ( m_origin - aB.p ).EuclideanNorm();
        }

        VECTOR2I m_origin;
    };

    bool Intersects( const SEG& aSeg) const;
    bool Intersects( const SHAPE_LINE_CHAIN& aChain ) const;

    /**
     * Find all intersection points between our line chain and the segment \a aSeg.
     *
     * @param aSeg is the segment chain to find intersections with.
     * @param aIp is the reference to a vector to store found intersections. Intersection points
     *        are sorted with increasing distances from point aSeg.a.
     * @return the number of intersections found.
     */
    int Intersect( const SEG& aSeg, INTERSECTIONS& aIp ) const;

    /**
     * Find all intersection points between our line chain and the line chain \a aChain.
     *
     * @param aChain is the line chain to find intersections with.
     * @param aIp is reference to a vector to store found intersections. Intersection points are
     *        sorted with increasing path lengths from the starting point of \a aChain.
     * @return the number of intersections found.
     */
    int Intersect( const SHAPE_LINE_CHAIN& aChain, INTERSECTIONS& aIp,
                   bool aExcludeColinearAndTouching = false,
                   BOX2I* aChainBBox = nullptr ) const;

    /**
     * Compute the walk path length from the beginning of the line chain and the point \a aP
     * belonging to our line.
     *
     * @return the path length in Euclidean metric or -1 if aP does not belong to the line chain.
     */
    int PathLength( const VECTOR2I& aP, int aIndex = -1 ) const;

    /**
     * Check if point \a aP is closer to (or on) an edge or vertex of the line chain.
     *
     * @param aP is the point to check.
     * @param aDist is the distance in internal units.
     * @return true if the point is equal to or closer than \a aDist to the line chain.
     */
    bool CheckClearance( const VECTOR2I& aP, const int aDist) const;

    /**
     * Check if the line chain is self-intersecting. Only processes line segments (not arcs).
     *
     * @return (optional) first found self-intersection point.
     */
    const std::optional<INTERSECTION> SelfIntersecting() const;

    /**
     * Check if the line chain is self-intersecting. Also processes arcs. Might be slower.
     *
     * @return (optional) first found self-intersection point.
     */
    const std::optional<INTERSECTION> SelfIntersectingWithArcs() const;

    /**
     * Find the segment nearest the given point.
     *
     * @param aP is the point to compare with.
     * @return the index of the segment closest to the point.
     */
    int NearestSegment( const VECTOR2I& aP ) const;

    /**
     * Find a point on the line chain that is closest to point \a aP.
     *
     * @param aP is the point to find.
     * @param aAllowInternalShapePoints if false will not return points internal to an arc (i.e.
     *                                  only the arc endpoints are possible candidates)
     * @return the nearest point.
     */
    const VECTOR2I NearestPoint( const VECTOR2I& aP, bool aAllowInternalShapePoints = true ) const;

    /**
     * Find a point on the line chain that is closest to the line defined by the points of
     * segment \a aSeg, also returns the distance.
     *
     * @param aSeg is the segment defining the line.
     * @param dist is the reference receiving the distance to the nearest point.
     * @return the nearest point.
     */
    const VECTOR2I NearestPoint( const SEG& aSeg, int& dist ) const;

    /// @copydoc SHAPE::Format()
    const std::string Format( bool aCplusPlus = true ) const override;

    /// @copydoc SHAPE::Parse()
    bool Parse( std::stringstream& aStream ) override;

    bool operator!=( const SHAPE_LINE_CHAIN& aRhs ) const
    {
        if( PointCount() != aRhs.PointCount() )
            return true;

        for( int i = 0; i < PointCount(); i++ )
        {
            if( CPoint( i ) != aRhs.CPoint( i ) )
                return true;
        }

        return false;
    }

    /**
     * Compare this line chain with another one.
     *
     * @param aOther is the other line chain to compare with.
     * @param aCyclicalCompare if true, will consider line chains equal even if they start at different points
     * @param aEpsilon tolerance for point difference
     *
     * @return true if both line chains have the same points
     */
    bool CompareGeometry( const SHAPE_LINE_CHAIN& aOther, bool aCyclicalCompare = false, int aEpsilon = 0 ) const;

    void Move( const VECTOR2I& aVector ) override
    {
        for( auto& pt : m_points )
            pt += aVector;

        for( auto& arc : m_arcs )
            arc.Move( aVector );

        m_bbox.Move( aVector );
    }

    /**
     * Mirror the line points about y or x (or both).
     *
     * @param aRef sets the reference point about which to mirror.
     * @param aFlipDirection is the direction to mirror.
     */
    void Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection );

    /**
     * Mirror the line points using an given axis.
     *
     * @param axis is the axis on which to mirror.
     */
    void Mirror( const SEG& axis );

    /**
     * Rotate all vertices by a given angle.
     *
     * @param aCenter is the rotation center.
     * @param aAngle is the rotation angle.
     */
    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override;

    bool IsSolid() const override
    {
        return false;
    }

    const VECTOR2I PointAlong( int aPathLength ) const;

    /**
     * Return the area of this chain
     * @param aAbsolute If true, returns a positive value. Otherwise the value depends on the
     * orientation of the chain
     */
    double Area( bool aAbsolute = true ) const;

    /**
     * Extract parts of this line chain, depending on the starting and ending points.
     *
     * @param aStart first split point.
     * @param aEnd second split point.
     * @param aPre part before the aStart point.
     * @param aMid part between aStart and aEnd.
     * @param aPost part after the aEnd point.
     */
    void Split( const VECTOR2I& aStart, const VECTOR2I& aEnd, SHAPE_LINE_CHAIN& aPre,
                SHAPE_LINE_CHAIN& aMid, SHAPE_LINE_CHAIN& aPost ) const;

    /**
     * Creates line chains \a aLeft and \a aRight offset to this line chain.
     *
     * @param aAmount is the amount to offset.
     * @param aCornerStrategy is the corner rounding strategy.
     * @param aMaxError is the max error used for rounding.
     * @param aLeft left line chain output.
     * @param aRight right line chain output.
     * @param aSimplify set to run Simplify on the inflated polygon.
     */
    bool OffsetLine( int aAmount, CORNER_STRATEGY aCornerStrategy, int aMaxError,
                     SHAPE_LINE_CHAIN& aLeft, SHAPE_LINE_CHAIN& aRight,
                     bool aSimplify = false ) const;

    size_t ArcCount() const
    {
        return m_arcs.size();
    }

    /**
     * Return the arc index for the given segment index.
     */
    ssize_t ArcIndex( size_t aSegment ) const
    {
        if( IsSharedPt( aSegment ) )
            return m_shapes[aSegment].second;
        else
            return m_shapes[aSegment].first;
    }

    const SHAPE_ARC& Arc( size_t aArc ) const
    {
        return m_arcs[aArc];
    }

    /**
     * Test if a point is shared between multiple shapes
     * @param aIndex
     * @return
    */
    bool IsSharedPt( size_t aIndex ) const;

    bool IsPtOnArc( size_t aPtIndex ) const;

    bool IsArcSegment( size_t aSegment ) const;

    bool IsArcStart( size_t aIndex ) const;

    bool IsArcEnd( size_t aIndex ) const;

    using SHAPE::Distance;

    int Distance( const VECTOR2I& aP, bool aOutlineOnly ) const
    {
        return sqrt( SquaredDistance( aP, aOutlineOnly ) );
    }

    virtual const VECTOR2I GetPoint( int aIndex ) const override { return CPoint(aIndex); }
    virtual const SEG GetSegment( int aIndex ) const override { return CSegment(aIndex); }
    virtual size_t GetPointCount() const override { return PointCount(); }
    virtual size_t GetSegmentCount() const override { return SegmentCount(); }

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override;

protected:
    friend class SHAPE_POLY_SET;

    /**
     * Convert an arc to only a point chain by removing the arc and references
     *
     * @param aArcIndex index of the arc to convert to points
     */
    void convertArc( ssize_t aArcIndex );

    /**
     * Splits an arc into two arcs at aPtIndex. Parameter \p aCoincident controls whether the two
     * arcs are to be coincident at aPtIndex or whether a short straight segment should be created
     * instead
     *
     * @param aPtIndex index of the point in the chain in which to split the arc
     * @param aCoincident If true, the end point of the first arc will be coincident with the start
                          point of the second arc at aPtIndex.
                          If false, the end point of the first arc will be at aPtIndex-1 and the
                          start point of the second arc will be at aPtIndex, resulting in a short
                          straight line segment between aPtIndex-1 and aPtIndex.
     */
    void splitArc( ssize_t aPtIndex, bool aCoincident = false );

    void amendArc( size_t aArcIndex, const VECTOR2I& aNewStart, const VECTOR2I& aNewEnd );

    void amendArcStart( size_t aArcIndex, const VECTOR2I& aNewStart )
    {
        amendArc( aArcIndex, aNewStart, m_arcs[aArcIndex].GetP1() );
    }

    void amendArcEnd( size_t aArcIndex, const VECTOR2I& aNewEnd )
    {
        amendArc( aArcIndex, m_arcs[aArcIndex].GetP0(), aNewEnd );
    }

    /**
     * Return the arc index for the given segment index, looking backwards
     */
    ssize_t reversedArcIndex( size_t aSegment ) const
    {
        if( IsSharedPt( aSegment ) )
            return m_shapes[aSegment].first;
        else
            return m_shapes[aSegment].second;
    }

    /**
     * Create a new Clipper2 path from the SHAPE_LINE_CHAIN in a given orientation
     */
    Clipper2Lib::Path64 convertToClipper2( bool aRequiredOrientation,
            std::vector<CLIPPER_Z_VALUE> &aZValueBuffer,
            std::vector<SHAPE_ARC> &aArcBuffer ) const;

    /**
     * Fix indices of this chain to ensure arcs are not split between the end and start indices
     */
    void fixIndicesRotation();

    /**
     * Merge the first and last point if they are the same and this chain is closed.
     */
    void mergeFirstLastPointIfNeeded();

private:

    static const ssize_t SHAPE_IS_PT;

    static const std::pair<ssize_t, ssize_t> SHAPES_ARE_PT;

    /// array of vertices
    std::vector<VECTOR2I> m_points;

    /**
     * Array of indices that refer to the index of the shape if the point is part of a larger
     * shape, e.g. arc or spline.
     * If the value is -1, the point is just a point.
     *
     * There can be up to two shapes associated with a single point (e.g. the end point of
     * one arc might be the start point of another).
     *
     * Generally speaking only the first element of the pair will be populated (i.e. with a value
     * not equal to SHAPE_IS_PT), unless the point is shared between two arc shapes. If the point
     * is shared, then both the first and second element of the pair should be populated.
     *
     * The second element must always be SHAPE_IS_PT if the first element is SHAPE_IS_PT.
     */
    std::vector<std::pair<ssize_t, ssize_t>> m_shapes;

    std::vector<SHAPE_ARC> m_arcs;

    // the maxError to use when converting arcs to points
    int m_accuracy;

    /// is the line chain closed?
    bool m_closed;

    /// Width of the segments (for BBox calculations in RTree)
    /// TODO Adjust usage of SHAPE_LINE_CHAIN to account for where we need a width and where not
    /// Alternatively, we could split the class into a LINE_CHAIN (no width) and SHAPE_LINE_CHAIN
    /// that derives from SHAPE as well that does have a width.  Not sure yet on the correct path.
    /// TODO Note that we also have SHAPE_SIMPLE which is a closed, filled SHAPE_LINE_CHAIN.
    int m_width;

    /// cached bounding box
    mutable BOX2I m_bbox;
};


#endif // __SHAPE_LINE_CHAIN
