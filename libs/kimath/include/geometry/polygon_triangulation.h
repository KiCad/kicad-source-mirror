/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * Based on Uniform Plane Subdivision algorithm from Lamot, Marko, and Borut Žalik.
 * "A fast polygon triangulation algorithm based on uniform plane subdivision."
 * Computers & graphics 27, no. 2 (2003): 239-253.
 *
 * Code derived from:
 * K-3D which is Copyright (c) 2005-2006, Romain Behar, GPL-2, license above
 * earcut which is Copyright (c) 2016, Mapbox, ISC
 *
 * ISC License:
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */

#ifndef __POLYGON_TRIANGULATION_H
#define __POLYGON_TRIANGULATION_H

#include <algorithm>
#include <array>
#include <deque>
#include <cmath>
#include <vector>

#include <advanced_config.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/vertex_set.h>
#include <math/box2.h>
#include <math/vector2d.h>

#include <wx/log.h>

// ADVANCED_CFG::GetCfg() cannot be used on msys2/mingw builds (link failure)
// So we use the ADVANCED_CFG default values
#if defined( __MINGW32__ )
    #define TRIANGULATESIMPLIFICATIONLEVEL 50
    #define TRIANGULATEMINIMUMAREA 1000
#else
    #define TRIANGULATESIMPLIFICATIONLEVEL ADVANCED_CFG::GetCfg().m_TriangulateSimplificationLevel
    #define TRIANGULATEMINIMUMAREA ADVANCED_CFG::GetCfg().m_TriangulateMinimumArea
#endif

#define TRIANGULATE_TRACE "triangulate"

class POLYGON_TRIANGULATION : public VERTEX_SET
{
public:
    POLYGON_TRIANGULATION( SHAPE_POLY_SET::TRIANGULATED_POLYGON& aResult ) :
        VERTEX_SET( TRIANGULATESIMPLIFICATIONLEVEL ),
        m_vertices_original_size( 0 ), m_result( aResult )
    {};

    /**
     * Triangulate a polygon with holes by bridging holes directly into the
     * outer ring's VERTEX linked list, avoiding the Fracture() intermediate step.
     */
    bool TesselatePolygon( const SHAPE_POLY_SET::POLYGON& aPolygon,
                           SHAPE_POLY_SET::TRIANGULATED_POLYGON* aHintData )
    {
        if( aPolygon.empty() )
            return true;

        if( aPolygon.size() == 1 )
            return TesselatePolygon( aPolygon[0], aHintData );

        const SHAPE_LINE_CHAIN& outline = aPolygon[0];

        m_bbox = outline.BBox();

        for( size_t i = 1; i < aPolygon.size(); i++ )
            m_bbox.Merge( aPolygon[i].BBox() );

        m_result.Clear();

        if( !m_bbox.GetWidth() || !m_bbox.GetHeight() )
            return true;

        for( const SHAPE_LINE_CHAIN& chain : aPolygon )
        {
            for( const VECTOR2I& pt : chain.CPoints() )
                m_result.AddVertex( pt );
        }

        int baseIndex = 0;
        VERTEX* outerRing = createRing( outline, baseIndex, true );
        baseIndex += outline.PointCount();

        if( !outerRing || outerRing->prev == outerRing->next )
            return true;

        std::vector<VERTEX*> holeRings;

        for( size_t i = 1; i < aPolygon.size(); i++ )
        {
            VERTEX* holeRing = createRing( aPolygon[i], baseIndex, false );
            baseIndex += aPolygon[i].PointCount();

            // Reject rings collapsed below 3 vertices.  Match the outer-ring guard at
            // line 119 so degenerate holes (single point or two-vertex sliver) cannot
            // reach eliminateHoles().
            if( holeRing && holeRing->prev != holeRing->next )
                holeRings.push_back( holeRing );
        }

        m_vertices_original_size = m_vertices.size();

        if( !holeRings.empty() )
        {
            outerRing = eliminateHoles( outerRing, holeRings );

            if( !outerRing )
            {
                wxLogTrace( TRIANGULATE_TRACE, "Hole elimination failed" );
                return false;
            }
        }

        if( VERTEX* simplified = simplifyList( outerRing ) )
            outerRing = simplified;

        outerRing->updateList();

        auto retval = earcutList( outerRing );

        if( !retval )
        {
            wxLogTrace( TRIANGULATE_TRACE, "Tesselation with holes failed, logging remaining vertices" );
            logRemaining();
        }

        m_vertices.clear();
        return retval;
    }

    bool TesselatePolygon( const SHAPE_LINE_CHAIN& aPoly,
                           SHAPE_POLY_SET::TRIANGULATED_POLYGON* aHintData )
    {
        m_bbox = aPoly.BBox();
        m_result.Clear();

        if( !m_bbox.GetWidth() || !m_bbox.GetHeight() )
            return true;

        /// Place the polygon Vertices into a circular linked list
        /// and check for lists that have only 0, 1 or 2 elements and
        /// therefore cannot be polygons
        VERTEX* firstVertex = createList( aPoly );

        for( const VECTOR2I& pt : aPoly.CPoints() )
            m_result.AddVertex( pt );

        if( !firstVertex || firstVertex->prev == firstVertex->next )
            return true;

        wxLogTrace( TRIANGULATE_TRACE, "Created list with %f area", firstVertex->area() );

        m_vertices_original_size = m_vertices.size();

        if( VERTEX* simplified = simplifyList( firstVertex ) )
            firstVertex = simplified;

        firstVertex->updateList();

        /**
         * If we have hint data, we can skip the tesselation process as long as the
         * hint source did not need to subdivide the polygon.  Hint triangle indices
         * refer to the original vertex list stored in m_result, not the simplified
         * working ring used internally during triangulation.
        */
        if( aHintData && aHintData->Vertices().size() == m_result.GetVertexCount() )
        {
            m_result.SetTriangles( aHintData->Triangles() );
            return true;
        }
        else
        {
            auto retval = earcutList( firstVertex );

            if( !retval )
            {
                wxLogTrace( TRIANGULATE_TRACE, "Tesselation failed, logging remaining vertices" );
                logRemaining();
            }

            m_vertices.clear();
            return retval;
        }
    }

    std::vector<double> PartitionAreaFractionsForTesting( const SHAPE_LINE_CHAIN& aPoly,
                                                          size_t aTargetLeaves ) const
    {
        std::vector<SHAPE_LINE_CHAIN> partitions = partitionPolygonBalanced( aPoly, aTargetLeaves );
        std::vector<double>           fractions;
        double                        totalArea = std::abs( aPoly.Area() );

        if( totalArea <= 0.0 )
            return fractions;

        fractions.reserve( partitions.size() );

        for( const SHAPE_LINE_CHAIN& part : partitions )
            fractions.push_back( std::abs( part.Area() ) / totalArea );

        return fractions;
    }
private:
    friend struct POLYGON_TRIANGULATION_TEST_ACCESS;
    friend class SHAPE_POLY_SET;

    struct SCANLINE_HIT
    {
        int      edgeIndex;
        VECTOR2I point;
    };

    bool collectScanlineHits( const SHAPE_LINE_CHAIN& aPoly, bool aVertical, int aCut,
                              std::array<SCANLINE_HIT, 2>& aHits ) const
    {
        int count = 0;

        for( int ii = 0; ii < aPoly.PointCount(); ++ii )
        {
            const VECTOR2I& a = aPoly.CPoint( ii );
            const VECTOR2I& b = aPoly.CPoint( ( ii + 1 ) % aPoly.PointCount() );

            if( aVertical )
            {
                if( a.x == b.x || aCut <= std::min( a.x, b.x ) || aCut >= std::max( a.x, b.x ) )
                    continue;

                if( count >= 2 )
                    return false;

                double t = static_cast<double>( aCut - a.x ) / static_cast<double>( b.x - a.x );
                int    y = static_cast<int>( std::lround( a.y + t * ( b.y - a.y ) ) );
                aHits[count++] = { ii, VECTOR2I( aCut, y ) };
            }
            else
            {
                if( a.y == b.y || aCut <= std::min( a.y, b.y ) || aCut >= std::max( a.y, b.y ) )
                    continue;

                if( count >= 2 )
                    return false;

                double t = static_cast<double>( aCut - a.y ) / static_cast<double>( b.y - a.y );
                int    x = static_cast<int>( std::lround( a.x + t * ( b.x - a.x ) ) );
                aHits[count++] = { ii, VECTOR2I( x, aCut ) };
            }
        }

        return count == 2;
    }

    SHAPE_LINE_CHAIN createSplitChild( const SHAPE_LINE_CHAIN& aPoly, int aStart, int aEnd ) const
    {
        SHAPE_LINE_CHAIN child;
        int              idx = aStart;
        int              guard = 0;
        const int        count = aPoly.PointCount();

        do
        {
            child.Append( aPoly.CPoint( idx ) );
            idx = ( idx + 1 ) % count;
            ++guard;
        } while( idx != ( aEnd + 1 ) % count && guard <= count + 2 );

        child.SetClosed( true );
        child.Simplify2( true );
        return child;
    }

    bool splitPolygonAtCoordinate( const SHAPE_LINE_CHAIN& aPoly, bool aVertical, int aCut,
                                   std::array<SHAPE_LINE_CHAIN, 2>& aChildren, double& aAreaA,
                                   double& aAreaB ) const
    {
        std::array<SCANLINE_HIT, 2> hits;

        if( !collectScanlineHits( aPoly, aVertical, aCut, hits ) )
            return false;

        SHAPE_LINE_CHAIN augmented( aPoly );
        augmented.Split( hits[0].point, true );
        augmented.Split( hits[1].point, true );

        int idxA = augmented.Find( hits[0].point );
        int idxB = augmented.Find( hits[1].point );

        if( idxA < 0 || idxB < 0 || idxA == idxB )
            return false;

        aChildren[0] = createSplitChild( augmented, idxA, idxB );
        aChildren[1] = createSplitChild( augmented, idxB, idxA );

        if( aChildren[0].PointCount() < 3 || aChildren[1].PointCount() < 3 )
            return false;

        aAreaA = std::abs( aChildren[0].Area() );
        aAreaB = std::abs( aChildren[1].Area() );
        return aAreaA > 0.0 && aAreaB > 0.0;
    }

    bool splitPolygonBalanced( const SHAPE_LINE_CHAIN& aPoly,
                               std::array<SHAPE_LINE_CHAIN, 2>& aChildren ) const
    {
        const BOX2I  bbox = aPoly.BBox();
        const bool   verticalFirst = bbox.GetWidth() >= bbox.GetHeight();
        const double totalArea = std::abs( aPoly.Area() );

        if( totalArea <= 0.0 )
            return false;

        auto tryAxis =
                [&]( bool aVertical ) -> bool
                {
                    const int low = ( aVertical ? bbox.GetX() : bbox.GetY() ) + 1;
                    const int high = ( aVertical ? bbox.GetRight() : bbox.GetBottom() ) - 1;

                    if( high <= low )
                        return false;

                    double bestImbalance = std::numeric_limits<double>::infinity();
                    std::array<SHAPE_LINE_CHAIN, 2> bestChildren;

                    constexpr int kSamples = 15;

                    for( int ii = 1; ii <= kSamples; ++ii )
                    {
                        int cut = low + ( ( high - low ) * ii ) / ( kSamples + 1 );
                        std::array<SHAPE_LINE_CHAIN, 2> candidate;
                        double areaA = 0.0;
                        double areaB = 0.0;

                        if( !splitPolygonAtCoordinate( aPoly, aVertical, cut, candidate, areaA, areaB ) )
                            continue;

                        double imbalance = std::abs( areaA - areaB ) / totalArea;

                        if( imbalance < bestImbalance )
                        {
                            bestImbalance = imbalance;
                            bestChildren = std::move( candidate );
                        }
                    }

                    if( !std::isfinite( bestImbalance ) || bestImbalance > 0.35 )
                        return false;

                    aChildren = std::move( bestChildren );
                    return true;
                };

        return tryAxis( verticalFirst ) || tryAxis( !verticalFirst );
    }

    std::vector<SHAPE_LINE_CHAIN> partitionPolygonBalanced( const SHAPE_LINE_CHAIN& aPoly,
                                                            size_t aTargetLeaves ) const
    {
        std::vector<SHAPE_LINE_CHAIN> leaves = { aPoly };

        if( aTargetLeaves < 2 )
            return leaves;

        while( leaves.size() < aTargetLeaves )
        {
            int    bestLeaf = -1;
            double bestArea = 0.0;

            for( size_t ii = 0; ii < leaves.size(); ++ii )
            {
                double area = std::abs( leaves[ii].Area() );

                if( area > bestArea )
                {
                    bestArea = area;
                    bestLeaf = static_cast<int>( ii );
                }
            }

            if( bestLeaf < 0 )
                break;

            std::array<SHAPE_LINE_CHAIN, 2> children;

            if( !splitPolygonBalanced( leaves[bestLeaf], children ) )
                break;

            leaves[bestLeaf] = std::move( children[0] );
            leaves.push_back( std::move( children[1] ) );
        }

        return leaves;
    }

    size_t suggestedPartitionLeafCount( const SHAPE_LINE_CHAIN& aPoly ) const
    {
        constexpr size_t kVerticesPerLeaf = 50000;
        constexpr size_t kMaxLeaves = 8;
        size_t           leaves = 1;

        while( leaves < kMaxLeaves
               && static_cast<size_t>( aPoly.PointCount() ) / leaves > kVerticesPerLeaf )
        {
            leaves *= 2;
        }

        return leaves;
    }

    /**
     * Outputs a list of vertices that have not yet been triangulated.
    */
    void logRemaining()
    {
        std::set<VERTEX*> seen;
        wxLog::EnableLogging();
        for( VERTEX& p : m_vertices )
        {
            if( !p.next || p.next == &p || seen.find( &p ) != seen.end() )
                continue;

            logVertices( &p, &seen );
        }
    }

    void logVertices( VERTEX* aStart, std::set<VERTEX*>* aSeen )
    {
        if( aSeen && aSeen->count( aStart ) )
            return;

        if( aSeen )
            aSeen->insert( aStart );

        int count = 1;
        VERTEX* p = aStart->next;
        wxString msg = wxString::Format( "Vertices: %d,%d,", static_cast<int>( aStart->x ),
                                         static_cast<int>( aStart->y ) );

        do
        {
            msg += wxString::Format( "%d,%d,", static_cast<int>( p->x ), static_cast<int>( p->y ) );

            if( aSeen )
                aSeen->insert( p );

            p = p->next;
            count++;
        } while( p != aStart );

        if( count < 3 )   // Don't log anything that only has 2 or fewer points
            return;

        msg.RemoveLast();
        wxLogTrace( TRIANGULATE_TRACE, msg );
    }

    /**
     * Simplify the line chain by removing points that are too close to each other.
     * If no points are removed, it returns nullptr.
     */
    VERTEX* simplifyList( VERTEX* aStart )
    {
        if( !aStart || aStart->next == aStart->prev )
            return aStart;

        VERTEX* p = aStart;
        VERTEX* next = p->next;
        VERTEX* retval = aStart;
        int     count = 0;

        double sq_dist = TRIANGULATESIMPLIFICATIONLEVEL;
        sq_dist *= sq_dist;

        do
        {
            VECTOR2D diff = VECTOR2D( next->x - p->x, next->y - p->y );

            if( diff.SquaredEuclideanNorm() < sq_dist )
            {
                if( next == aStart )
                {
                    retval = p;
                    aStart->remove();
                    count++;
                    break;
                }

                next = next->next;
                p->next->remove();
                count++;
                retval = p;
            }
            else
            {
                p = next;
                next = next->next;
            }
        } while( p != aStart && next && p );

        wxLogTrace( TRIANGULATE_TRACE, "Removed %d points in simplifyList", count );

        if( count )
            return retval;

        return nullptr;
    }

    /**
     * Iterate through the list to remove NULL triangles if they exist.
     *
     * This should only be called as a last resort when tesselation fails
     * as the NULL triangles are inserted as Steiner points to improve the
     * triangulation regularity of polygons
     */
    VERTEX* removeNullTriangles( VERTEX* aStart )
    {
        VERTEX* retval = nullptr;
        size_t count = 0;

        if( ( retval = simplifyList( aStart ) ) )
            aStart = retval;

        wxASSERT( aStart->next && aStart->prev );

        VERTEX* p = aStart->next;

        while( p != aStart && p->next && p->prev )
        {
            // We make a dummy triangle that is actually part of the existing line segment
            // and measure its area.  This will not be exactly zero due to floating point
            // errors.  We then look for areas that are less than 4 times the area of the
            // dummy triangle.  For small triangles, this is a small number
            VERTEX tmp( 0, 0.5 * ( p->prev->x + p->next->x ), 0.5 * ( p->prev->y + p->next->y ), this );
            double null_area = 4.0 * std::abs( area( p->prev, &tmp, p->next ) );

            if( *p == *( p->next ) || std::abs( area( p->prev, p, p->next ) ) <= null_area )
            {
                // This is a spike, remove it, leaving only one point
                if( *( p->next ) == *( p->prev ) )
                    p->next->remove();

                p = p->prev;
                p->next->remove();
                retval = p;
                ++count;

                if( p == p->next )
                    break;

                // aStart was removed above, so we need to reset it
                if( !aStart->next )
                    aStart = p->prev;

                continue;
            }

            p = p->next;
        };

        /// We've removed all possible triangles
        if( !p->next || p->next == p || p->next == p->prev )
            return p;

        // We needed an end point above that wouldn't be removed, so
        // here we do the final check for this as a Steiner point
        VERTEX tmp( 0, 0.5 * ( p->prev->x + p->next->x ),
                       0.5 * ( p->prev->y + p->next->y ), this );
        double null_area = 4.0 * std::abs( area( p->prev, &tmp, p->next ) );

        if( std::abs( area( p->prev, p, p->next ) ) <= null_area )
        {
            retval = p->next;
            p->remove();
            ++count;
        }

        wxLogTrace( TRIANGULATE_TRACE, "Removed %zu NULL triangles", count );

        return retval;
    }

    /**
     * Walk through a circular linked list starting at \a aPoint.
     *
     * For each point, test to see if the adjacent points form a triangle that is completely
     * enclosed by the remaining polygon (an "ear" sticking off the polygon).  If the three
     * points form an ear, we log the ear's location and remove the center point from the
     * linked list.
     *
     * This function can be called recursively in the case of difficult polygons.  In cases
     * where there is an intersection (not technically allowed by KiCad, but could exist in
     * an edited file), we create a single triangle and remove both vertices before attempting
     * to.
     */
    bool earcutList( VERTEX* aPoint, int pass = 0 )
    {
        constexpr int kMaxRecursion = 64;

        if( pass >= kMaxRecursion )
        {
            wxLogTrace( TRIANGULATE_TRACE, "earcutList recursion limit reached; aborting triangulation", pass );
            return false;
        }

        wxLogTrace( TRIANGULATE_TRACE, "earcutList starting at %p for pass %d", aPoint, pass );

        if( !aPoint )
            return true;

        VERTEX* stop = aPoint;
        VERTEX* prev;
        VERTEX* next;
        int internal_pass = 1;
        constexpr int kEarLookahead = 2;

        while( aPoint->prev != aPoint->next )
        {
            prev = aPoint->prev;
            next = aPoint->next;

            VERTEX* bestEar = nullptr;
            double  bestScore = -1.0;
            int     lookahead = 0;

            for( VERTEX* candidate = aPoint; candidate && lookahead < kEarLookahead;
                 candidate = candidate->next, ++lookahead )
            {
                if( !candidate->isEar() || isTooSmall( candidate ) )
                    continue;

                const double score = earScore( candidate->prev, candidate, candidate->next );

                if( !bestEar || score > bestScore )
                {
                    bestEar = candidate;
                    bestScore = score;
                }
            }

            if( bestEar )
            {
                prev = bestEar->prev;
                next = bestEar->next;
                m_result.AddTriangle( prev->i, bestEar->i, next->i );
                bestEar->remove();

                // Skip one vertex as the triangle will account for the prev node
                aPoint = next->next;
                stop = next->next;
                continue;
            }

            VERTEX* nextNext = next->next;

            if( *prev != *nextNext && intersects( prev, aPoint, next, nextNext ) &&
                    locallyInside( prev, nextNext ) &&
                    locallyInside( nextNext, prev ) )
            {
                wxLogTrace( TRIANGULATE_TRACE,
                            "Local intersection detected.  Merging minor triangle with area %f",
                            area( prev, aPoint, nextNext ) );
                m_result.AddTriangle( prev->i, aPoint->i, nextNext->i );

                // remove two nodes involved
                next->remove();
                aPoint->remove();

                aPoint = nextNext;
                stop = nextNext;

                continue;
            }

            aPoint = next;

            /*
             * We've searched the entire polygon for available ears and there are still
             * un-sliced nodes remaining.
             */
            if( aPoint == stop && aPoint->prev != aPoint->next )
            {
                VERTEX* newPoint;

                // Removing null triangles will remove steiner points as well as colinear points
                // that are three in a row.  Because our next step is to subdivide the polygon,
                // we need to allow it to add the subdivided points first.  This is why we only
                // run the RemoveNullTriangles function after the first pass.
                if( ( internal_pass == 2 ) && ( newPoint = removeNullTriangles( aPoint ) ) )
                {
                    // There are no remaining triangles in the list
                    if( newPoint->next == newPoint->prev )
                        break;

                    aPoint = newPoint;
                    stop = newPoint;
                    continue;
                }

                ++internal_pass;

                // This will subdivide the polygon 2 times.  The first pass will add enough points
                // such that each edge is less than the average edge length.  If this doesn't work
                // The next pass will remove the null triangles (above) and subdivide the polygon
                // again, this time adding one point to each long edge (and thereby changing the locations)
                if( internal_pass < 4 )
                {
                    wxLogTrace( TRIANGULATE_TRACE, "Subdividing polygon" );
                    subdividePolygon( aPoint, internal_pass );
                    continue;
                }

                // If we don't have any NULL triangles left, cut the polygon in two and try again
                wxLogTrace( TRIANGULATE_TRACE, "Splitting polygon" );

                if( !splitPolygon( aPoint, pass + 1 ) )
                    return false;

                break;
            }
        }

        // Check to see if we are left with only three points in the polygon
        if( aPoint->next && aPoint->prev == aPoint->next->next )
        {
            // Three concave points will never be able to be triangulated because they were
            // created by an intersecting polygon, so just drop them.
            if( area( aPoint->prev, aPoint, aPoint->next ) >= 0 )
                return true;
        }

        /*
         * At this point, our polygon should be fully tessellated.
         */
        if( aPoint->prev != aPoint->next )
            return std::abs( aPoint->area() ) > TRIANGULATEMINIMUMAREA;

        return true;
    }


    /**
     * Check whether a given vertex is too small to matter.
     */

    bool isTooSmall( const VERTEX* aPoint ) const
    {
        double min_area = TRIANGULATEMINIMUMAREA;
        double prev_sq_len = ( aPoint->prev->x - aPoint->x ) * ( aPoint->prev->x - aPoint->x ) +
                             ( aPoint->prev->y - aPoint->y ) * ( aPoint->prev->y - aPoint->y );
        double next_sq_len = ( aPoint->next->x - aPoint->x ) * ( aPoint->next->x - aPoint->x ) +
                             ( aPoint->next->y - aPoint->y ) * ( aPoint->next->y - aPoint->y );
        double opp_sq_len = ( aPoint->next->x - aPoint->prev->x ) * ( aPoint->next->x - aPoint->prev->x ) +
                            ( aPoint->next->y - aPoint->prev->y ) * ( aPoint->next->y - aPoint->prev->y );

        return ( prev_sq_len < min_area || next_sq_len < min_area || opp_sq_len < min_area );
    }

    double earScore( const VERTEX* a, const VERTEX* b, const VERTEX* c ) const
    {
        const double ab_sq = ( a->x - b->x ) * ( a->x - b->x ) + ( a->y - b->y ) * ( a->y - b->y );
        const double bc_sq = ( b->x - c->x ) * ( b->x - c->x ) + ( b->y - c->y ) * ( b->y - c->y );
        const double ca_sq = ( c->x - a->x ) * ( c->x - a->x ) + ( c->y - a->y ) * ( c->y - a->y );
        const double norm = ab_sq + bc_sq + ca_sq;

        if( norm <= 0.0 )
            return 0.0;

        return std::abs( area( a, b, c ) ) / norm;
    }

    /**
     * Create a VERTEX linked list from a SHAPE_LINE_CHAIN with a global index offset.
     * The winding direction is controlled by aWantCCW.
     */
    VERTEX* createRing( const SHAPE_LINE_CHAIN& aPoints, int aBaseIndex, bool aWantCCW )
    {
        VERTEX*  tail = nullptr;
        double   sum = 0.0;
        VECTOR2L last_pt;
        bool     first = true;

        for( int i = 0; i < aPoints.PointCount(); i++ )
        {
            VECTOR2D p1 = aPoints.CPoint( i );
            VECTOR2D p2 = aPoints.CPoint( ( i + 1 ) % aPoints.PointCount() );
            sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
        }

        bool isCW = sum > 0.0;
        bool needReverse = ( aWantCCW == isCW );

        auto addVertex = [&]( int i )
        {
            const VECTOR2I& pt = aPoints.CPoint( i );

            if( first || pt.SquaredDistance( last_pt ) > m_simplificationLevel )
            {
                tail = insertVertex( aBaseIndex + i, pt, tail );
                last_pt = pt;
                first = false;
            }
        };

        if( needReverse )
        {
            for( int i = aPoints.PointCount() - 1; i >= 0; i-- )
                addVertex( i );
        }
        else
        {
            for( int i = 0; i < aPoints.PointCount(); i++ )
                addVertex( i );
        }

        // Collapse a final duplicate, but never on a single-vertex ring.  When the
        // simplification pass leaves only one vertex, tail->next == tail and removing
        // it would leave the caller holding a vertex with null next/prev pointers.
        if( tail && tail->next != tail && ( *tail == *tail->next ) )
            tail->next->remove();

        return tail;
    }

    /**
     * Bridge all hole rings into the outer ring by sorting holes left-to-right
     * and connecting each hole's leftmost vertex to the nearest visible point
     * on the outer boundary via VERTEX::split().
     */
    VERTEX* eliminateHoles( VERTEX* aOuterRing, std::vector<VERTEX*>& aHoleRings )
    {
        struct HoleInfo
        {
            VERTEX* leftmost;
            double  leftX;
        };

        std::vector<HoleInfo> holes;
        holes.reserve( aHoleRings.size() );

        for( VERTEX* hole : aHoleRings )
        {
            VERTEX* leftmost = hole;
            VERTEX* p = hole->next;

            while( p != hole )
            {
                if( p->x < leftmost->x || ( p->x == leftmost->x && p->y < leftmost->y ) )
                    leftmost = p;

                p = p->next;
            }

            holes.push_back( { leftmost, leftmost->x } );
        }

        std::sort( holes.begin(), holes.end(),
                   []( const HoleInfo& a, const HoleInfo& b ) { return a.leftX < b.leftX; } );

        for( const HoleInfo& hi : holes )
        {
            VERTEX* bridge = findHoleBridge( hi.leftmost, aOuterRing );

            if( bridge )
            {
                VERTEX* bridgeReverse = bridge->split( hi.leftmost );
                filterPoints( bridgeReverse, bridgeReverse->next );
                aOuterRing = filterPoints( bridge, bridge->next );
            }
            else
            {
                wxLogTrace( TRIANGULATE_TRACE, "Failed to find bridge for hole at (%f, %f)",
                            hi.leftmost->x, hi.leftmost->y );
            }
        }

        return aOuterRing;
    }

    /**
     * Remove consecutive duplicate vertices from the linked list.
     */
    VERTEX* filterPoints( VERTEX* aStart, VERTEX* aEnd = nullptr )
    {
        if( !aStart )
            return aStart;

        if( !aEnd )
            aEnd = aStart;

        VERTEX* p = aStart;
        bool    again;

        do
        {
            again = false;

            if( *p == *p->next )
            {
                VERTEX* toRemove = p->next;
                
                if( toRemove == aEnd )
                    aEnd = p;

                toRemove->remove();

                if( p == p->next )
                    return p;

                p = p->prev;
                again = true;
            }
            else
            {
                p = p->next;
            }
        } while( again || p != aEnd );

        return aEnd;
    }


    /**
     * Find a vertex on the outer ring visible from the hole's leftmost vertex
     * by casting a horizontal ray to the left.
     */
    VERTEX* findHoleBridge( VERTEX* aHole, VERTEX* aOuterStart )
    {
        VERTEX* p = aOuterStart;
        double  hx = aHole->x;
        double  hy = aHole->y;
        double  qx = -std::numeric_limits<double>::infinity();
        VERTEX* m = nullptr;

        do
        {
            if( hy <= p->y && hy >= p->next->y && p->next->y != p->y )
            {
                double x = p->x + ( hy - p->y ) * ( p->next->x - p->x )
                                                  / ( p->next->y - p->y );

                if( x <= hx && x > qx )
                {
                    qx = x;

                    if( x == hx )
                    {
                        if( hy == p->y )
                            return p;

                        if( hy == p->next->y )
                            return p->next;
                    }

                    m = ( p->x < p->next->x ) ? p : p->next;
                }
            }

            p = p->next;
        } while( p != aOuterStart );

        if( !m )
            return nullptr;

        if( hx == qx )
            return m;

        // Pick the vertex inside the visibility triangle closest to the ray
        const VERTEX* stop = m;
        double        mx = m->x;
        double        my = m->y;
        double        tanMin = std::numeric_limits<double>::infinity();

        p = m;

        do
        {
            if( hx >= p->x && p->x >= mx && hx != p->x )
            {
                bool inside;

                if( hy < my )
                    inside = triArea( hx, hy, mx, my, p->x, p->y ) >= 0
                             && triArea( mx, my, qx, hy, p->x, p->y ) >= 0
                             && triArea( qx, hy, hx, hy, p->x, p->y ) >= 0;
                else
                    inside = triArea( qx, hy, mx, my, p->x, p->y ) >= 0
                             && triArea( mx, my, hx, hy, p->x, p->y ) >= 0
                             && triArea( hx, hy, qx, hy, p->x, p->y ) >= 0;

                if( inside )
                {
                    double t = std::abs( hy - p->y ) / ( hx - p->x );

                    if( locallyInside( p, aHole )
                        && ( t < tanMin
                             || ( t == tanMin
                                  && ( p->x > m->x
                                       || ( p->x == m->x
                                            && sectorContainsSector( m, p ) ) ) ) ) )
                    {
                        m = p;
                        tanMin = t;
                    }
                }
            }

            p = p->next;
        } while( p != stop );

        return m;
    }

    /**
     * Signed area of triangle (ax,ay), (bx,by), (cx,cy).
     */
    static double triArea( double ax, double ay, double bx, double by,
                           double cx, double cy )
    {
        return ( bx - ax ) * ( cy - ay ) - ( by - ay ) * ( cx - ax );
    }

    /**
     * Whether sector in vertex m contains sector in vertex p in the same
     * coordinate frame.
     */
    bool sectorContainsSector( const VERTEX* m, const VERTEX* p ) const
    {
        return area( m->prev, m, p->prev ) < 0 && area( p->next, m, m->next ) < 0;
    }

    /**
     * Inserts a new vertex halfway between each existing pair of vertices.
     */
    void subdividePolygon( VERTEX* aStart, int pass = 0 )
    {
        VERTEX* p = aStart;

        struct VertexComparator {
            bool operator()(const std::pair<VERTEX*,double>& a, const std::pair<VERTEX*,double>& b) const {
                return a.second > b.second;
            }
        };

        std::set<std::pair<VERTEX*,double>, VertexComparator> longest;
        double avg = 0.0;

        do
        {
            double len = ( p->x - p->next->x ) * ( p->x - p->next->x ) +
                         ( p->y - p->next->y ) * ( p->y - p->next->y );
            longest.emplace( p, len );

            avg += len;
            p = p->next;
        } while (p != aStart);

        avg /= longest.size();
        wxLogTrace( TRIANGULATE_TRACE, "Average length: %f", avg );

        constexpr double kSubdivideThresholdFactor = 1.1;
        const double     subdivideThreshold = avg * kSubdivideThresholdFactor;

        for( auto it = longest.begin(); it != longest.end() && it->second > subdivideThreshold;
             ++it )
        {
            wxLogTrace( TRIANGULATE_TRACE, "Subdividing edge with length %f", it->second );
            VERTEX* a = it->first;
            VERTEX* b = a->next;
            VERTEX* last = a;

            // We adjust the number of divisions based on the pass in order to progressively
            // subdivide the polygon when triangulation fails
            int divisions = avg / it->second + 2 + pass;
            double step = 1.0 / divisions;

            for( int i = 1; i < divisions; i++ )
            {
                double x = a->x * ( 1.0 - step * i ) + b->x * ( step * i );
                double y = a->y * ( 1.0 - step * i ) + b->y * ( step * i );
                last = insertTriVertex( VECTOR2I( x, y ), last );
            }
        }

        // update z-order of the vertices
        aStart->updateList();
    }

    /**
     * If we cannot find an ear to slice in the current polygon list, we
     * use this to split the polygon into two separate lists and slice them each
     * independently.  This is assured to generate at least one new ear if the
     * split is successful
     */
    bool splitPolygon( VERTEX* start, int aPass )
    {
        VERTEX* origPoly = start;

        // If we have fewer than 4 points, we cannot split the polygon
        if( !start || !start->next || start->next == start->prev
            || start->next->next == start->prev )
        {
            return true;
        }

        // Our first attempts to split the polygon will be at overlapping points.
        // These are natural split points and we only need to switch the loop directions
        // to generate two new loops.  Since they are overlapping, we are do not
        // need to create a new segment to disconnect the two loops.
        do
        {
            std::vector<VERTEX*> overlapPoints;
            VERTEX* z_pt = origPoly;

            while ( z_pt->prevZ && *z_pt->prevZ == *origPoly )
                z_pt = z_pt->prevZ;

            overlapPoints.push_back( z_pt );

            while( z_pt->nextZ && *z_pt->nextZ == *origPoly )
            {
                z_pt = z_pt->nextZ;
                overlapPoints.push_back( z_pt );
            }

            if( overlapPoints.size() != 2 || overlapPoints[0]->next == overlapPoints[1]
                || overlapPoints[0]->prev == overlapPoints[1] )
            {
                origPoly = origPoly->next;
                continue;
            }

            if( overlapPoints[0]->area( overlapPoints[1] ) < 0 || overlapPoints[1]->area( overlapPoints[0] ) < 0 )
            {
                wxLogTrace( TRIANGULATE_TRACE, "Split generated a hole, skipping" );
                origPoly = origPoly->next;
                continue;
            }

            wxLogTrace( TRIANGULATE_TRACE, "Splitting at overlap point %f, %f", overlapPoints[0]->x, overlapPoints[0]->y );
            std::swap( overlapPoints[0]->next, overlapPoints[1]->next );
            overlapPoints[0]->next->prev = overlapPoints[0];
            overlapPoints[1]->next->prev = overlapPoints[1];

            overlapPoints[0]->updateList();
            overlapPoints[1]->updateList();
            logVertices( overlapPoints[0], nullptr );
            logVertices( overlapPoints[1], nullptr );
            bool retval = earcutList( overlapPoints[0], aPass )
                          && earcutList( overlapPoints[1], aPass );

            wxLogTrace( TRIANGULATE_TRACE, "%s at first overlap split", retval ? "Success" : "Failed" );
            return retval;


        } while ( origPoly != start );

        // If we've made it through the split algorithm and we still haven't found a
        // set of overlapping points, we need to create a new segment to split the polygon
        // into two separate polygons.  We do this by finding the two vertices that form
        // a valid line (does not cross the existing polygon)
        do
        {
            VERTEX* marker = origPoly->next->next;

            while( marker != origPoly->prev )
            {
                // Find a diagonal line that is wholly enclosed by the polygon interior
                if( origPoly->next && origPoly->i != marker->i && goodSplit( origPoly, marker ) )
                {
                    VERTEX* newPoly = origPoly->split( marker );

                    origPoly->updateList();
                    newPoly->updateList();

                    bool retval = earcutList( origPoly, aPass ) && earcutList( newPoly, aPass );

                    wxLogTrace( TRIANGULATE_TRACE, "%s at split", retval ? "Success" : "Failed" );
                    return retval;
                }

                marker = marker->next;
            }

            origPoly = origPoly->next;
        } while( origPoly != start );

        wxLogTrace( TRIANGULATE_TRACE, "Could not find a valid split point" );
        return false;
    }

    /**
     * Check if a segment joining two vertices lies fully inside the polygon.
     * To do this, we first ensure that the line isn't along the polygon edge.
     * Next, we know that if the line doesn't intersect the polygon, then it is
     * either fully inside or fully outside the polygon.  Next, we ensure that
     * the proposed split is inside the local area of the polygon at both ends
     * and the midpoint. Finally, we check to split creates two new polygons,
     * each with positive area.
     */
    bool goodSplit( const VERTEX* a, const VERTEX* b ) const
    {
        bool a_on_edge = ( a->nextZ && *a == *a->nextZ ) || ( a->prevZ && *a == *a->prevZ );
        bool b_on_edge = ( b->nextZ && *b == *b->nextZ ) || ( b->prevZ && *b == *b->prevZ );
        bool no_intersect = a->next->i != b->i && a->prev->i != b->i && !intersectsPolygon( a, b );
        bool local_split = locallyInside( a, b ) && locallyInside( b, a ) && middleInside( a, b );
        bool same_dir = area( a->prev, a, b->prev ) != 0.0 || area( a, b->prev, b ) != 0.0;
        bool has_len = ( *a == *b ) && area( a->prev, a, a->next ) > 0 && area( b->prev, b, b->next ) > 0;
        bool pos_area = a->area( b ) > 0 && b->area( a ) > 0;

        return no_intersect && local_split && ( same_dir || has_len ) && !a_on_edge && !b_on_edge && pos_area;

    }


    constexpr int sign( double aVal ) const
    {
        return ( aVal > 0 ) - ( aVal < 0 );
    }

    /**
     * If p, q, and r are collinear and r lies between p and q, then return true.
    */
    constexpr bool overlapping( const VERTEX* p, const VERTEX* q, const VERTEX* r ) const
    {
        return q->x <= std::max( p->x, r->x ) &&
               q->x >= std::min( p->x, r->x ) &&
               q->y <= std::max( p->y, r->y ) &&
               q->y >= std::min( p->y, r->y );
    }

    /**
     * Check for intersection between two segments, end points included.
     *
     * @return true if p1-p2 intersects q1-q2.
     */
    bool intersects( const VERTEX* p1, const VERTEX* q1, const VERTEX* p2, const VERTEX* q2 ) const
    {
        int sign1 = sign( area( p1, q1, p2 ) );
        int sign2 = sign( area( p1, q1, q2 ) );
        int sign3 = sign( area( p2, q2, p1 ) );
        int sign4 = sign( area( p2, q2, q1 ) );

        if( sign1 != sign2 && sign3 != sign4 )
            return true;

        if( sign1 == 0 && overlapping( p1, p2, q1 ) )
            return true;

        if( sign2 == 0 && overlapping( p1, q2, q1 ) )
            return true;

        if( sign3 == 0 && overlapping( p2, p1, q2 ) )
            return true;

        if( sign4 == 0 && overlapping( p2, q1, q2 ) )
            return true;


        return false;
    }

    /**
     * Check whether the segment from vertex a -> vertex b crosses any of the segments
     * of the polygon of which vertex a is a member.
     *
     * @return true if the segment intersects the edge of the polygon.
     */
    bool intersectsPolygon( const VERTEX* a, const VERTEX* b ) const
    {
        for( size_t ii = 0; ii < m_vertices_original_size; ii++ )
        {
            const VERTEX* p = &m_vertices[ii];
            const VERTEX* q = &m_vertices[( ii + 1 ) % m_vertices_original_size];

            if( p->i == a->i || p->i == b->i || q->i == a->i || q->i == b->i )
                continue;

            if( intersects( p, q, a, b ) )
                return true;
        }

        return false;
    }

    /**
     * Create an entry in the vertices lookup and optionally inserts the newly created vertex
     * into an existing linked list.
     *
     * @return a pointer to the newly created vertex.
     */
    VERTEX* insertTriVertex( const VECTOR2I& pt, VERTEX* last )
    {
        m_result.AddVertex( pt );
        return insertVertex( m_result.GetVertexCount() - 1, pt, last );
    }

private:
    size_t                                m_vertices_original_size;
    SHAPE_POLY_SET::TRIANGULATED_POLYGON& m_result;
};

#endif //__POLYGON_TRIANGULATION_H
