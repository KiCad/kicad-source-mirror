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

#ifndef VERTEX_SET_H
#define VERTEX_SET_H

#include <algorithm>
#include <deque>

#include <math/box2.h>
#include <geometry/shape_line_chain.h>

class VERTEX_SET;

class VERTEX
{
    public:

    VERTEX( int aIndex, double aX, double aY, VERTEX_SET* aParent, void* aUserData = nullptr ) :
            i( aIndex ),
            x( aX ),
            y( aY ),
            parent( aParent ),
            m_userData( aUserData )
    {
    }

    VERTEX& operator=( const VERTEX& ) = delete;
    VERTEX& operator=( VERTEX&& ) = delete;

    bool operator==( const VERTEX& rhs ) const
    {
        return this->x == rhs.x && this->y == rhs.y;
    }
    bool operator!=( const VERTEX& rhs ) const { return !( *this == rhs ); }

    void* GetUserData() const { return m_userData; }

    /**
     * Remove the node from the linked list and z-ordered linked list.
     */
    void remove()
    {
        next->prev = prev;
        prev->next = next;

        if( prevZ )
            prevZ->nextZ = nextZ;

        if( nextZ )
            nextZ->prevZ = prevZ;

        next = nullptr;
        prev = nullptr;
        nextZ = nullptr;
        prevZ = nullptr;
    }



    /**
     * Split the referenced polygon between the reference point and
     * vertex b, assuming they are in the same polygon.  Notes that while we
     * create a new vertex pointer for the linked list, we maintain the same
     * vertex index value from the original polygon.  In this way, we have
     * two polygons that both share the same vertices.
     *
     * @return the newly created vertex in the polygon that does not include the
     *         reference vertex.
     */
    VERTEX* split( VERTEX* b );

    void updateOrder();

    /**
     * After inserting or changing nodes, this function should be called to
     * remove duplicate vertices and ensure z-ordering is correct.
     */
    void updateList()
    {
        VERTEX* p = next;

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
     * Sort all vertices in this vertex's list by their Morton code.
     */
    void zSort()
    {
        std::deque<VERTEX*> queue;

        queue.push_back( this );

        for( VERTEX* p = next; p && p != this; p = p->next )
            queue.push_back( p );

        std::sort( queue.begin(), queue.end(), []( const VERTEX* a, const VERTEX* b )
        {
            if( a->z != b->z )
                return a->z < b->z;

            if( a->x != b->x )
                return a->x < b->x;

            if( a->y != b->y )
                return a->y < b->y;

            return a->i < b->i;
        } );

        VERTEX* prev_elem = nullptr;

        for( VERTEX* elem : queue )
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
    bool inTriangle( const VERTEX& a, const VERTEX& b, const VERTEX& c )
    {
        return     ( c.x - x ) * ( a.y - y ) - ( a.x - x ) * ( c.y - y ) >= 0
                && ( a.x - x ) * ( b.y - y ) - ( b.x - x ) * ( a.y - y ) >= 0
                && ( b.x - x ) * ( c.y - y ) - ( c.x - x ) * ( b.y - y ) >= 0;
    }

    /**
     * Returns the signed area of the polygon connected to the current vertex,
     * optionally ending at a specified vertex.
     */
    double area( const VERTEX* aEnd = nullptr ) const
    {
        const VERTEX* p = this;
        double a = 0.0;

        do
        {
            a += ( p->x + p->next->x ) * ( p->next->y - p->y );
            p = p->next;
        } while( p != this && p != aEnd );

        if( p != this )
            a += ( p->x + x ) * ( y - p->y );

        return a / 2;
    }

    /**
     * Check whether the given vertex is in the middle of an ear.
     *
     * This works by walking forward and backward in zOrder to the limits of the minimal
     * bounding box formed around the triangle, checking whether any points are located
     * inside the given triangle.
     *
     * @param aMatchUserData if true, the user data of the vertices must match.  This can be
     *                       useful when the vertices come from different polygons.
     *
     *
     * @return true if aEar is the apex point of a ear in the polygon.
     */
    bool isEar( bool aMatchUserData = false ) const;

    const int    i;
    const double x;
    const double y;
    VERTEX_SET*  parent;

    // previous and next vertices nodes in a polygon ring
    VERTEX* prev = nullptr;
    VERTEX* next = nullptr;

    // z-order curve value
    uint32_t z = 0;

    // previous and next nodes in z-order
    VERTEX* prevZ = nullptr;
    VERTEX* nextZ = nullptr;

    void* m_userData = nullptr;
};

class VERTEX_SET
{
    friend class VERTEX;

public:
    VERTEX_SET( int aSimplificationLevel )
    {
        m_simplificationLevel = aSimplificationLevel * ( VECTOR2I::extended_type ) aSimplificationLevel;
    }
    ~VERTEX_SET() {}

    void SetBoundingBox( const BOX2I& aBBox );

    /**
     * Insert a vertex into the vertex set
     * @param aIndex the index of the vertex
     * @param pt the point to insert
     * @param last the last vertex in the list
     * @param aUserData user data to associate with the vertex
     * @return the newly inserted vertex
    */
    VERTEX* insertVertex( int aIndex, const VECTOR2I& pt, VERTEX* last, void* aUserData = nullptr );

    /**
     * Create a list of vertices from a line chain
     * @param points the line chain to create the list from
     * @param aTail the optional vertex to which to append the list
     * @param aUserData user data to associate with the vertices
     * @return the first vertex in the list
    */
    VERTEX* createList( const SHAPE_LINE_CHAIN& points, VERTEX* aTail = nullptr, void* aUserData = nullptr );

protected:

    /**
     * Get the next vertex in the outline, avoiding steiner points
     * and points that overlap with splits
     * @param aPt the current vertex
     * @return the next vertex in the outline
    */
    VERTEX* getNextOutlineVertex( const VERTEX* aPt ) const;

    /**
     * Get the previous vertex in the outline, avoiding steiner points
     * and points that overlap with splits
     * @param aPt the current vertex
     * @return the previous vertex in the outline
    */
    VERTEX* getPrevOutlineVertex( const VERTEX* aPt ) const;

    /**
     * Check whether the segment from vertex a -> vertex b is inside the polygon
     * around the immediate area of vertex a.
     *
     * We don't define the exact area over which the segment is inside but it is guaranteed to
     * be inside the polygon immediately adjacent to vertex a.
     *
     * @return true if the segment from a->b is inside a's polygon next to vertex a.
    */
    bool    locallyInside( const VERTEX* a, const VERTEX* b ) const;

    /**
     * Check if the middle of the segment from a to b is inside the polygon
     * @param a the first vertex
     * @param b the second vertex
     * @return true if the point is in the middle of the triangle
    */
    bool middleInside( const VERTEX* a, const VERTEX* b ) const;

    /**
     * Check if two vertices are at the same point
     * @param aA the first vertex
     * @param aB the second vertex
     * @return true if the vertices are at the same point
    */
    bool    same_point( const VERTEX* aA, const VERTEX* aB ) const;

    /**
     * Note that while the inputs are doubles, these are scaled
     * by the size of the bounding box to fit into a 32-bit Morton code
     * @return the Morton code for the point (aX, aY)
    */
    uint32_t zOrder( const double aX, const double aY ) const;

    /**
     * @return the area of the triangle defined by the three vertices
    */
    double  area( const VERTEX* p, const VERTEX* q, const VERTEX* r ) const;

    BOX2I                   m_bbox;
    std::deque<VERTEX>      m_vertices;
    VECTOR2I::extended_type m_simplificationLevel;
};



#endif // VERTEX_SET_H
