/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Modifications Copyright (C) 2018-2019 KiCad Developers
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
#include <cmath>
#include <vector>
#include <math/box2.h>

#include "clipper.hpp"

class PolygonTriangulation
{

public:

    PolygonTriangulation( SHAPE_POLY_SET::TRIANGULATED_POLYGON& aResult ) :
        m_result( aResult )
    {};

private:
    struct Vertex
    {
        Vertex( size_t aIndex, double aX, double aY, PolygonTriangulation* aParent ) :
                i( aIndex ), x( aX ), y( aY ), parent( aParent )
        {
        }
        Vertex& operator=( const Vertex& ) = delete;
        Vertex& operator=( Vertex&& ) = delete;

        bool operator==( const Vertex& rhs ) const
        {
            return this->x == rhs.x && this->y == rhs.y;
        }
        bool operator!=( const Vertex& rhs ) const { return !( *this == rhs ); }


        /**
         * Function split
         * Splits the referenced polygon between the reference point and
         * vertex b, assuming they are in the same polygon.  Notes that while we
         * create a new vertex pointer for the linked list, we maintain the same
         * vertex index value from the original polygon.  In this way, we have
         * two polygons that both share the same vertices.
         *
         * Returns the pointer to the newly created vertex in the polygon that
         * does not include the reference vertex.
         */
        Vertex* split( Vertex* b )
        {
            parent->m_vertices.emplace_back( i, x, y, parent );
            Vertex* a2 = &parent->m_vertices.back();
            parent->m_vertices.emplace_back( b->i, b->x, b->y, parent );
            Vertex* b2 = &parent->m_vertices.back();
            Vertex* an = next;
            Vertex* bp = b->prev;

            next = b;
            b->prev = this;

            a2->next = an;
            an->prev = a2;

            b2->next = a2;
            a2->prev = b2;

            bp->next = b2;
            b2->prev = bp;

            return b2;
        }

        /**
         * Function remove
         * Removes the node from the linked list and z-ordered linked list.
         */
        void remove()
        {
            next->prev = prev;
            prev->next = next;

            if( prevZ )
                prevZ->nextZ = nextZ;
            if( nextZ )
                nextZ->prevZ = prevZ;
            next = NULL;
            prev = NULL;
            nextZ = NULL;
            prevZ = NULL;
        }


        void updateOrder()
        {
            if( !z )
                z = parent->zOrder( x, y );
        }

        /**
         * Function updateList
         * After inserting or changing nodes, this function should be called to
         * remove duplicate vertices and ensure z-ordering is correct
         */
        void updateList()
        {
            Vertex* p = next;

            while( p != this )
            {
                /**
                 * Remove duplicates
                 */
                if( *p == *p->next )
                {
                    p = p->prev;
                    p->next->remove();

                    if( p == p->next )
                        break;
                }

                p->updateOrder();
                p = p->next;
            };

            updateOrder();
            zSort();
        }

        /**
         * Sort all vertices in this vertex's list by their Morton code
         */
        void zSort()
        {
            std::deque<Vertex*> queue;

            queue.push_back( this );

            for( auto p = next; p && p != this; p = p->next )
                queue.push_back( p );

            std::sort( queue.begin(), queue.end(), []( const Vertex* a, const Vertex* b)
            {
                return a->z < b->z;
            } );

            Vertex* prev_elem = nullptr;
            for( auto elem : queue )
            {
                if( prev_elem )
                    prev_elem->nextZ = elem;

                elem->prevZ = prev_elem;
                prev_elem = elem;
            }

            prev_elem->nextZ = nullptr;
        }


        /**
         * Check to see if triangle surrounds our current vertex
         */
        bool inTriangle( const Vertex& a, const Vertex& b, const Vertex& c )
        {
            return     ( c.x - x ) * ( a.y - y ) - ( a.x - x ) * ( c.y - y ) >= 0
                    && ( a.x - x ) * ( b.y - y ) - ( b.x - x ) * ( a.y - y ) >= 0
                    && ( b.x - x ) * ( c.y - y ) - ( c.x - x ) * ( b.y - y ) >= 0;
        }

        const size_t i;
        const double x;
        const double y;
        PolygonTriangulation* parent;

        // previous and next vertices nodes in a polygon ring
        Vertex* prev = nullptr;
        Vertex* next = nullptr;

        // z-order curve value
        int32_t z = 0;

        // previous and next nodes in z-order
        Vertex* prevZ = nullptr;
        Vertex* nextZ = nullptr;
    };

    BOX2I m_bbox;
    std::deque<Vertex> m_vertices;
    SHAPE_POLY_SET::TRIANGULATED_POLYGON& m_result;

    /**
     * Calculate the Morton code of the Vertex
     * http://www.graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
     *
     */
    int32_t zOrder( const double aX, const double aY ) const
    {
        int32_t x = static_cast<int32_t>( 32767.0 * ( aX - m_bbox.GetX() ) / m_bbox.GetWidth() );
        int32_t y = static_cast<int32_t>( 32767.0 * ( aY - m_bbox.GetY() ) / m_bbox.GetHeight() );

        x = ( x | ( x << 8 ) ) & 0x00FF00FF;
        x = ( x | ( x << 4 ) ) & 0x0F0F0F0F;
        x = ( x | ( x << 2 ) ) & 0x33333333;
        x = ( x | ( x << 1 ) ) & 0x55555555;

        y = ( y | ( y << 8 ) ) & 0x00FF00FF;
        y = ( y | ( y << 4 ) ) & 0x0F0F0F0F;
        y = ( y | ( y << 2 ) ) & 0x33333333;
        y = ( y | ( y << 1 ) ) & 0x55555555;

        return x | ( y << 1 );
    }

    /**
     * Function removeNullTriangles
     * Iterates through the list to remove NULL triangles if they exist.
     * This should only be called as a last resort when tesselation fails
     * as the NULL triangles are inserted as Steiner points to improve the
     * triangulation regularity of polygons
     */
    Vertex* removeNullTriangles( Vertex* aStart )
    {
        Vertex* retval = nullptr;
        Vertex* p = aStart->next;

        while( p != aStart )
        {
            if( area( p->prev, p, p->next ) == 0.0 )
            {
                p = p->prev;
                p->next->remove();
                retval = aStart;

                if( p == p->next )
                    break;
            }
            p = p->next;
        };

        // We needed an end point above that wouldn't be removed, so
        // here we do the final check for this as a Steiner point
        if( area( aStart->prev, aStart, aStart->next ) == 0.0 )
        {
            retval = p->next;
            p->remove();
        }

        return retval;
    }

    /**
     * Function createList
     * Takes a Clipper path and converts it into a circular, doubly-linked
     * list for triangulation
     */
    Vertex* createList( const ClipperLib::Path& aPath )
    {
        Vertex* tail = nullptr;
        double sum = 0.0;
        auto len = aPath.size();

        // Check for winding order
        for( size_t i = 0; i < len; i++ )
        {
            auto p1 = aPath.at( i );
            auto p2 = aPath.at( ( i + 1 ) < len ? i + 1 : 0 );

            sum += ( ( p2.X - p1.X ) * ( p2.Y + p1.Y ) );
        }

        if( sum <= 0.0 )
        {
            for( auto point : aPath )
                tail = insertVertex( VECTOR2I( point.X, point.Y ), tail );
        }
        else
        {
            for( size_t i = 0; i < len; i++ )
            {
                auto p = aPath.at( len - i - 1 );
                tail = insertVertex( VECTOR2I( p.X, p.Y ), tail );
            }
        }

        if( tail && ( *tail == *tail->next ) )
        {
            tail->next->remove();
        }

        return tail;

    }

    /**
     * Function createList
     * Takes the SHAPE_LINE_CHAIN and links each point into a
     * circular, doubly-linked list
     */
    Vertex* createList( const SHAPE_LINE_CHAIN& points )
    {
        Vertex* tail = nullptr;
        double sum = 0.0;

        // Check for winding order
        for( int i = 0; i < points.PointCount(); i++ )
        {
            VECTOR2D p1 = points.CPoint( i );
            VECTOR2D p2 = points.CPoint( i + 1 );

            sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
        }

        if( sum > 0.0 )
            for( int i = points.PointCount() - 1; i >= 0; i--)
                tail = insertVertex( points.CPoint( i ), tail );
        else
            for( int i = 0; i < points.PointCount(); i++ )
                tail = insertVertex( points.CPoint( i ), tail );

        if( tail && ( *tail == *tail->next ) )
        {
            tail->next->remove();
        }

        return tail;
    }

    /**
     * Function: earcutList
     * Walks through a circular linked list starting at aPoint.  For each point,
     * test to see if the adjacent points form a triangle that is completely enclosed
     * by the remaining polygon (an "ear" sticking off the polygon).  If the three points
     * form an ear, we log the ear's location and remove the center point from the linked list.
     *
     * This function can be called recursively in the case of difficult polygons.  In cases where
     * there is an intersection (not technically allowed by KiCad, but could exist in an edited file),
     * we create a single triangle and remove both vertices before attempting to
     */
    bool earcutList( Vertex* aPoint, int pass = 0 )
    {
        if( !aPoint )
            return true;

        Vertex* stop = aPoint;
        Vertex* prev;
        Vertex* next;

        while( aPoint->prev != aPoint->next )
        {
            prev = aPoint->prev;
            next = aPoint->next;

            if( isEar( aPoint ) )
            {
                m_result.AddTriangle( prev->i, aPoint->i, next->i );
                aPoint->remove();

                // Skip one vertex as the triangle will account for the prev node
                aPoint = next->next;
                stop = next->next;

                continue;
            }

            Vertex* nextNext = next->next;

            if( *prev != *nextNext && intersects( prev, aPoint, next, nextNext ) &&
                    locallyInside( prev, nextNext ) &&
                    locallyInside( nextNext, prev ) )
            {
                m_result.AddTriangle( prev->i, aPoint->i, nextNext->i );

                // remove two nodes involved
                next->remove();
                aPoint->remove();

                aPoint = nextNext;
                stop = nextNext;

                continue;
            }

            aPoint = next;

            /**
             * We've searched the entire polygon for available ears and there are still un-sliced nodes
             * remaining
             */
            if( aPoint == stop )
            {
                // First, try to remove the remaining steiner points
                // If aPoint is a steiner, we need to re-assign both the start and stop points
                if( auto newPoint = removeNullTriangles( aPoint ) )
                {
                    aPoint = newPoint;
                    stop = newPoint;
                    continue;
                }

                // If we don't have any NULL triangles left, cut the polygon in two and try again
                splitPolygon( aPoint );
                break;
            }
        }

        /**
         * At this point, our polygon should be fully tesselated.
         */
        return( aPoint->prev == aPoint->next );
    }

    /**
     * Function isEar
     * Checks whether the given vertex is in the middle of an ear.
     * This works by walking forward and backward in zOrder to the limits
     * of the minimal bounding box formed around the triangle, checking whether
     * any points are located inside the given triangle.
     *
     * Returns true if aEar is the apex point of a ear in the polygon
     */
    bool isEar( Vertex* aEar ) const
    {
        const Vertex* a = aEar->prev;
        const Vertex* b = aEar;
        const Vertex* c = aEar->next;

        // If the area >=0, then the three points for a concave sequence
        // with b as the reflex point
        if( area( a, b, c ) >= 0 )
            return false;

        // triangle bbox
        const double minTX = std::min( a->x, std::min( b->x, c->x ) );
        const double minTY = std::min( a->y, std::min( b->y, c->y ) );
        const double maxTX = std::max( a->x, std::max( b->x, c->x ) );
        const double maxTY = std::max( a->y, std::max( b->y, c->y ) );

        // z-order range for the current triangle bounding box
        const int32_t minZ = zOrder( minTX, minTY );
        const int32_t maxZ = zOrder( maxTX, maxTY );

        // first look for points inside the triangle in increasing z-order
        Vertex* p = aEar->nextZ;

        while( p && p->z <= maxZ )
        {
            if( p != a && p != c
                    && p->inTriangle( *a, *b, *c )
                    && area( p->prev, p, p->next ) >= 0 )
                return false;
            p = p->nextZ;
        }

        // then look for points in decreasing z-order
        p = aEar->prevZ;

        while( p && p->z >= minZ )
        {
            if( p != a && p != c
                    && p->inTriangle( *a, *b, *c )
                    && area( p->prev, p, p->next ) >= 0 )
                return false;
            p = p->prevZ;
        }

        return true;
    }

    /**
     * Function splitPolygon
     * If we cannot find an ear to slice in the current polygon list, we
     * use this to split the polygon into two separate lists and slice them each
     * independently.  This is assured to generate at least one new ear if the
     * split is successful
     */
    void splitPolygon( Vertex* start )
    {
        Vertex* origPoly = start;
        do
        {
            Vertex* marker = origPoly->next->next;
            while( marker != origPoly->prev )
            {
                // Find a diagonal line that is wholly enclosed by the polygon interior
                if( origPoly->i != marker->i && goodSplit( origPoly, marker ) )
                {
                    Vertex* newPoly = origPoly->split( marker );

                    origPoly->updateList();
                    newPoly->updateList();

                    earcutList( origPoly );
                    earcutList( newPoly );
                    return;
                }
                marker = marker->next;
            }
            origPoly = origPoly->next;
        } while( origPoly != start );
    }

    /**
     * Check if a segment joining two vertices lies fully inside the polygon.
     * To do this, we first ensure that the line isn't along the polygon edge.
     * Next, we know that if the line doesn't intersect the polygon, then it is
     * either fully inside or fully outside the polygon.  Finally, by checking whether
     * the segment is enclosed by the local triangles, we distinguish between
     * these two cases and no further checks are needed.
     */
    bool goodSplit( const Vertex* a, const Vertex* b ) const
    {
        return a->next->i != b->i &&
               a->prev->i != b->i &&
               !intersectsPolygon( a, b ) &&
               locallyInside( a, b );
    }

    /**
     * Function area
     * Returns the twice the signed area of the triangle formed by vertices
     * p, q, r.
     */
    double area( const Vertex* p, const Vertex* q, const Vertex* r ) const
    {
        return ( q->y - p->y ) * ( r->x - q->x ) - ( q->x - p->x ) * ( r->y - q->y );
    }

    /**
     * Function intersects
     * Checks for intersection between two segments, end points included.
     * Returns true if p1-p2 intersects q1-q2
     */
    bool intersects( const Vertex* p1, const Vertex* q1, const Vertex* p2, const Vertex* q2 ) const
    {
        if( ( *p1 == *q1 && *p2 == *q2 ) || ( *p1 == *q2 && *p2 == *q1 ) )
            return true;

        return ( area( p1, q1, p2 ) > 0 ) != ( area( p1, q1, q2 ) > 0 )
                && ( area( p2, q2, p1 ) > 0 ) != ( area( p2, q2, q1 ) > 0 );
    }

    /**
     * Function intersectsPolygon
     * Checks whether the segment from vertex a -> vertex b crosses any of the segments
     * of the polygon of which vertex a is a member.
     * Return true if the segment intersects the edge of the polygon
     */
    bool intersectsPolygon( const Vertex* a, const Vertex* b ) const
    {
        const Vertex* p = a->next;
        do
        {
            if( p->i != a->i &&
                p->next->i != a->i &&
                p->i != b->i &&
                p->next->i != b->i && intersects( p, p->next, a, b ) )
                return true;

            p = p->next;
        } while( p != a );

        return false;
    }

    /**
     * Function locallyInside
     * Checks whether the segment from vertex a -> vertex b is inside the polygon
     * around the immediate area of vertex a.  We don't define the exact area
     * over which the segment is inside but it is guaranteed to be inside the polygon
     * immediately adjacent to vertex a.
     * Returns true if the segment from a->b is inside a's polygon next to vertex a
     */
    bool locallyInside( const Vertex* a, const Vertex* b ) const
    {
        if( area( a->prev, a, a->next ) < 0 )
            return area( a, b, a->next ) >= 0 && area( a, a->prev, b ) >= 0;
        else
            return area( a, b, a->prev ) < 0 || area( a, a->next, b ) < 0;
    }

    /**
     * Function insertVertex
     * Creates an entry in the vertices lookup and optionally inserts the newly
     * created vertex into an existing linked list.
     * Returns a pointer to the newly created vertex
     */
    Vertex* insertVertex( const VECTOR2I& pt, Vertex* last )
    {
        m_result.AddVertex( pt );
        m_vertices.emplace_back( m_result.GetVertexCount() - 1, pt.x, pt.y, this );

        Vertex* p = &m_vertices.back();
        if( !last )
        {
            p->prev = p;
            p->next = p;
        }
        else
        {
            p->next = last->next;
            p->prev = last;
            last->next->prev = p;
            last->next = p;
        }
        return p;
    }


public:

    bool TesselatePolygon( const SHAPE_LINE_CHAIN& aPoly )
    {
        m_bbox = aPoly.BBox();
        m_result.Clear();

        if( !m_bbox.GetWidth() || !m_bbox.GetHeight() )
            return false;

        /// Place the polygon Vertices into a circular linked list
        /// and check for lists that have only 0, 1 or 2 elements and
        /// therefore cannot be polygons
        Vertex* firstVertex = createList( aPoly );
        if( !firstVertex || firstVertex->prev == firstVertex->next )
            return false;

        firstVertex->updateList();

        auto retval = earcutList( firstVertex );
        m_vertices.clear();
        return retval;
    }
};

#endif //__POLYGON_TRIANGULATION_H
