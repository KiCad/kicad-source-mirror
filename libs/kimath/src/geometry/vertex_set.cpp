/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <geometry/vertex_set.h>

void VERTEX_SET::SetBoundingBox( const BOX2I& aBBox ) { m_bbox = aBBox; }


/**
 * Take a #SHAPE_LINE_CHAIN and links each point into a circular, doubly-linked list.
 */
VERTEX* VERTEX_SET::createList( const SHAPE_LINE_CHAIN& points, VERTEX* aTail, void* aUserData )
{
    VERTEX* tail = aTail;
    double sum = 0.0;

    // Check for winding order
    for( int i = 0; i < points.PointCount(); i++ )
    {
        VECTOR2D p1 = points.CPoint( i );
        VECTOR2D p2 = points.CPoint( i + 1 );

        sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
    }

    VECTOR2L last_pt;
    bool first = true;

    auto addVertex = [&]( int i )
    {
        const VECTOR2I& pt = points.CPoint( i );

        if( first || pt.SquaredDistance( last_pt ) > m_simplificationLevel )
        {
            tail = insertVertex( i, pt, tail, aUserData );
            last_pt = pt;
            first = false;
        }
    };

    if( sum > 0.0 )
    {
        for( int i = points.PointCount() - 1; i >= 0; i-- )
            addVertex( i );
    }
    else
    {
        for( int i = 0; i < points.PointCount(); i++ )
            addVertex( i );
    }

    if( tail && ( *tail == *tail->next ) )
    {
        tail->next->remove();
    }

    return tail;
}


/**
 * Calculate the Morton code of the VERTEX
 * http://www.graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
 *
 */
uint32_t VERTEX_SET::zOrder( const double aX, const double aY ) const
{
    double limit_x = std::clamp( ( aX - m_bbox.GetX() ) / m_bbox.GetWidth(), 0.0, 1.0 );
    double limit_y = std::clamp( ( aY - m_bbox.GetY() ) / m_bbox.GetHeight(), 0.0, 1.0 );

    uint32_t x = static_cast<uint32_t>( limit_x * 32767.0 );
    uint32_t y = static_cast<uint32_t>( limit_y * 32767.0 );

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
 * Return the twice the signed area of the triangle formed by vertices p, q, and r.
 */
double VERTEX_SET::area( const VERTEX* p, const VERTEX* q, const VERTEX* r ) const
{
    return ( q->y - p->y ) * ( r->x - q->x ) - ( q->x - p->x ) * ( r->y - q->y );
}


bool VERTEX_SET::same_point( const VERTEX* aA, const VERTEX* aB ) const
{
    return aA && aB && aA->x == aB->x && aA->y == aB->y;
}

VERTEX* VERTEX_SET::getNextOutlineVertex( const VERTEX* aPt ) const
{
    VERTEX* nz = aPt->nextZ;
    VERTEX* pz = aPt->prevZ;

    // If we hit a fracture point, we want to continue around the
    // edge we are working on and not switch to the pair edge
    // However, this will depend on which direction the initial
    // fracture hit is.  If we find that we skip directly to
    // a new fracture point, then we know that we are proceeding
    // in the wrong direction from the fracture and should
    // fall through to the next point
    if( same_point( aPt, nz ) && same_point( aPt->next, nz->prev )
            && aPt->y == aPt->next->y )
    {
        return nz->next;
    }

    if( same_point( aPt, pz ) && same_point( aPt->next, pz->prev )
            && aPt->y == aPt->next->y )
    {
        return pz->next;
    }

    return aPt->next;
}

VERTEX* VERTEX_SET::getPrevOutlineVertex( const VERTEX* aPt ) const
{
    VERTEX* nz = aPt->nextZ;
    VERTEX* pz = aPt->prevZ;

    // If we hit a fracture point, we want to continue around the
    // edge we are working on and not switch to the pair edge
    // However, this will depend on which direction the initial
    // fracture hit is.  If we find that we skip directly to
    // a new fracture point, then we know that we are proceeding
    // in the wrong direction from the fracture and should
    // fall through to the next point
    if( same_point( aPt, nz )
            && aPt->y == aPt->prev->y)
    {
        return nz->prev;
    }

    if( same_point( aPt, pz )
            && aPt->y == aPt->prev->y )
    {
        return pz->prev;
    }

    return aPt->prev;

}


bool VERTEX_SET::locallyInside( const VERTEX* a, const VERTEX* b ) const
{
    const VERTEX* an = getNextOutlineVertex( a );
    const VERTEX* ap = getPrevOutlineVertex( a );

    if( area( ap, a, an ) < 0 )
        return area( a, b, an ) >= 0 && area( a, ap, b ) >= 0;
    else
        return area( a, b, ap ) < 0 || area( a, an, b ) < 0;
}


bool VERTEX_SET::middleInside( const VERTEX* a, const VERTEX* b ) const
{
    const VERTEX* p = a;
    bool          inside = false;
    double        px = ( a->x + b->x ) / 2;
    double        py = ( a->y + b->y ) / 2;

    do
    {
        if( ( ( p->y > py ) != ( p->next->y > py ) )
            && ( px < ( p->next->x - p->x ) * ( py - p->y ) / ( p->next->y - p->y ) + p->x ) )
            inside = !inside;

        p = p->next;
    } while( p != a );

    return inside;
}

/**
 * Create an entry in the vertices lookup and optionally inserts the newly created vertex
 * into an existing linked list.
 *
 * @return a pointer to the newly created vertex.
 */
VERTEX* VERTEX_SET::insertVertex( int aIndex, const VECTOR2I& pt, VERTEX* last, void* aUserData )
{
    m_vertices.emplace_back( aIndex, pt.x, pt.y, this, aUserData );

    VERTEX* p = &m_vertices.back();

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


VERTEX* VERTEX::split( VERTEX* b )
{
    parent->m_vertices.emplace_back( i, x, y, parent, m_userData );
    VERTEX* a2 = parent->insertVertex( i, VECTOR2I( x, y ), nullptr, m_userData );
    parent->m_vertices.emplace_back( b->i, b->x, b->y, parent, m_userData );
    VERTEX* b2 = &parent->m_vertices.back();
    VERTEX* an = next;
    VERTEX* bp = b->prev;

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


void VERTEX::updateOrder()
{
    if( !z )
        z = parent->zOrder( x, y );
}


bool VERTEX::isEar( bool aMatchUserData ) const
{
    const VERTEX* a = prev;
    const VERTEX* b = this;
    const VERTEX* c = next;

    if( aMatchUserData )
    {
        while( a->GetUserData() != m_userData )
            a = a->prev;

        while( c->GetUserData() != m_userData )
            c = c->next;
    }

    // If the area >=0, then the three points for a concave sequence
    // with b as the reflex point
    if( parent->area( a, b, c ) >= 0 )
        return false;

    // triangle bbox
    const double minTX = std::min( a->x, std::min( b->x, c->x ) );
    const double minTY = std::min( a->y, std::min( b->y, c->y ) );
    const double maxTX = std::max( a->x, std::max( b->x, c->x ) );
    const double maxTY = std::max( a->y, std::max( b->y, c->y ) );

    // z-order range for the current triangle bounding box
    const uint32_t minZ = parent->zOrder( minTX, minTY );
    const uint32_t maxZ = parent->zOrder( maxTX, maxTY );

    // first look for points inside the triangle in increasing z-order
    VERTEX* p = nextZ;

    while( p && p->z <= maxZ )
    {
        if( ( !aMatchUserData || p->GetUserData() == m_userData )
                && p != a && p != c
                && p->inTriangle( *a, *b, *c )
                && parent->area( p->prev, p, p->next ) >= 0 )
            return false;

        p = p->nextZ;
    }

    // then look for points in decreasing z-order
    p = prevZ;

    while( p && p->z >= minZ )
    {
        if( ( !aMatchUserData || p->GetUserData() == m_userData )
                && p != a && p != c
                && p->inTriangle( *a, *b, *c )
                && parent->area( p->prev, p, p->next ) >= 0 )
            return false;

        p = p->prevZ;
    }

    return true;
}