/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eda_rect.h
 */

#ifndef EDA_RECT_H
#define EDA_RECT_H

#include <wx/gdicmn.h>
#include <math/box2.h>

/**
 * Handle the component boundary box.
 *
 * This class is similar to wxRect, but some wxRect functions are very curious, and are
 * working only if dimensions are >= 0 (not always the case in KiCad) and also KiCad needs
 * some specific method which makes this a more suitable class.
 */
class EDA_RECT
{
public:
    EDA_RECT() : m_init( false ) { };

    EDA_RECT( const VECTOR2I& aPos, const VECTOR2I& aSize ) :
            m_pos( aPos ),
            m_size( aSize ),
            m_init( true )
    { }

    EDA_RECT( const wxPoint& aPos, const wxSize& aSize ) :
            EDA_RECT( VECTOR2I( aPos.x, aPos.y ), VECTOR2I( aSize.x, aSize.y ) )
    { }

    template<class T>
    EDA_RECT( const BOX2<T> aBox )
    {
        m_pos = aBox.GetPosition();
        m_size.x = aBox.GetWidth();
        m_size.y = aBox.GetHeight();
        m_init   = true;
    }

    virtual ~EDA_RECT() { };

    VECTOR2I Centre() const
    {
        return VECTOR2I( m_pos.x + ( m_size.x >> 1 ), m_pos.y + ( m_size.y >> 1 ) );
    }

    /**
     * Move the rectangle by the \a aMoveVector.
     *
     * @param aMoveVector A wxPoint that is the value to move this rectangle.
     */
    void Move( const VECTOR2I& aMoveVector );

    /**
     * Ensures that the height ant width are positive.
     */
    void Normalize();

    /**
     * @param aPoint the wxPoint to test.
     * @return true if aPoint is inside the boundary box. A point on a edge is seen as inside.
     */
    bool Contains( const VECTOR2I& aPoint ) const;

    /**
     * @param x the x coordinate of the point to test.
     * @param y the x coordinate of the point to test.
     * @return true if point is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( int x, int y ) const { return Contains( VECTOR2I( x, y ) ); }

    /**
     * @param aRect the EDA_RECT to test.
     * @return true if aRect is Contained. A common edge is seen as contained.
     */
    bool Contains( const EDA_RECT& aRect ) const;

    const VECTOR2I GetSize() const { return m_size; }

    /**
     * @return the max size dimension.
     */
    int GetSizeMax() const { return ( m_size.x > m_size.y ) ? m_size.x : m_size.y; }

    int GetX() const { return m_pos.x; }
    int GetY() const { return m_pos.y; }

    const VECTOR2I GetOrigin() const { return m_pos; }
    const VECTOR2I GetPosition() const { return m_pos; }
    const VECTOR2I GetEnd() const { return VECTOR2I( m_pos.x + m_size.x, m_pos.y + m_size.y ); }
    const VECTOR2I GetCenter() const
    {
        return VECTOR2I( m_pos.x + ( m_size.x / 2 ), m_pos.y + ( m_size.y / 2 ) );
    }

    int GetWidth() const { return m_size.x; }
    int GetHeight() const { return m_size.y; }
    int GetRight() const { return m_pos.x + m_size.x; }
    int GetLeft() const { return m_pos.x; }
    int GetTop() const { return m_pos.y; }
    int GetBottom() const { return m_pos.y + m_size.y; }    // Y axis from top to bottom

    bool IsValid() const
    {
        return m_init;
    }

    void SetOrigin( const VECTOR2I& pos )
    {
        m_pos = pos;
        m_init = true;
    }

    void SetOrigin( int x, int y )
    {
        m_pos.x = x;
        m_pos.y = y;
        m_init = true;
    }

    void SetSize( const VECTOR2I& size )
    {
        m_size = size;
        m_init = true;
    }

    void SetSize( int w, int h )
    {
        m_size.x = w;
        m_size.y = h;
        m_init = true;
    }

    void Offset( int dx, int dy )
    {
        m_pos.x += dx;
        m_pos.y += dy;
    }

    void Offset( const VECTOR2I& offset )
    {
        m_pos += offset;
    }

    void SetX( int val )
    {
        m_pos.x = val;
        m_init = true;
    }

    void SetY( int val )
    {
        m_pos.y = val;
        m_init = true;
    }

    void SetWidth( int val )
    {
        m_size.x = val;
        m_init = true;
    }

    void SetHeight( int val )
    {
        m_size.y = val;
        m_init = true;
    }

    void SetEnd( int x, int y )
    {
        SetEnd( VECTOR2I( x, y ) );
        m_init = true;
    }

    void SetEnd( const VECTOR2I& pos )
    {
        m_size.x = pos.x - m_pos.x;
        m_size.y = pos.y - m_pos.y;
        m_init = true;
    }

    /**
     * Mirror the rectangle from the X axis (negate Y pos and size).
     */
    void RevertYAxis()
    {
        m_pos.y  = -m_pos.y;
        m_size.y = -m_size.y;
        Normalize();
    }

    /**
     * Test for a common area between rectangles.
     *
     * @param aRect A rectangle to test intersection with.
     * @return true if the argument rectangle intersects this rectangle.
     * (i.e. if the 2 rectangles have at least a common point)
     */
    bool Intersects( const EDA_RECT& aRect ) const;

    /**
     * Tests for a common area between this rectangle, and a rectangle with arbitrary rotation
     *
     * @param aRect a rectangle to test intersection with.
     * @param aRot rectangle rotation (in 1/10 degrees).
     */
    bool Intersects( const EDA_RECT& aRect, double aRot ) const;

    /**
     * Test for a common area between a segment and this rectangle.
     *
     * @param aPoint1 First point of the segment to test intersection with.
     * @param aPoint2 Second point of the segment to test intersection with.
     * @return true if the argument segment intersects this rectangle.
     * (i.e. if the segment and rectangle have at least a common point)
     */
    bool Intersects( const VECTOR2I& aPoint1, const VECTOR2I& aPoint2 ) const;

    /**
     * Test for intersection between a segment and this rectangle, returning the intersections.
     *
     * @param aPoint1 is the first point of the segment to test intersection with.
     * @param aPoint2 is the second point of the segment to test intersection with.
     * @param aIntersection1 will be filled with the first intersection point, if any.
     * @param aIntersection2 will be filled with the second intersection point, if any.
     * @return true if the segment intersects the rect.
     */
    bool Intersects( const VECTOR2I& aPoint1, const VECTOR2I& aPoint2, VECTOR2I* aIntersection1,
                     VECTOR2I* aIntersection2 ) const;

    /**
     * Return the point in this rect that is closest to the provided point
     */
    const VECTOR2I ClosestPointTo( const VECTOR2I& aPoint ) const;

    /**
     * Return the point in this rect that is farthest from the provided point
     */
    const VECTOR2I FarthestPointTo( const VECTOR2I& aPoint ) const;

    /**
     * Test for a common area between a circle and this rectangle.
     *
     * @param aCenter center of the circle.
     * @param aRadius radius of the circle.
     */
    bool IntersectsCircle( const VECTOR2I& aCenter, const int aRadius ) const;

    /**
     * Test for intersection between this rect and the edge (radius) of a circle.
     *
     * @param aCenter center of the circle.
     * @param aRadius radius of the circle.
     * @param aWidth width of the circle edge.
     */
    bool IntersectsCircleEdge( const VECTOR2I& aCenter, const int aRadius, const int aWidth ) const;

    /**
     * Overload the cast operator to return a wxRect.
     *
     * wxRect does not accept negative values for size, so ensure the wxRect size is always >= 0.
     */
    operator wxRect() const
    {
        EDA_RECT rect( m_pos, m_size );
        rect.Normalize();
        return wxRect( (wxPoint)rect.m_pos, (wxSize)rect.m_size );
    }

    /**
     * Overload the cast operator to return a BOX2I.
     *
     * @return this box shaped as a BOX2I object.
     */
    operator BOX2I() const
    {
        EDA_RECT rect( m_pos, m_size );
        rect.Normalize();
        return BOX2I( rect.GetOrigin(), rect.GetSize() );
    }

    /**
     * Inflate the rectangle horizontally by \a dx and vertically by \a dy. If \a dx
     * and/or \a dy is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( wxCoord dx, wxCoord dy );

    /**
     * Inflate the rectangle horizontally and vertically by \a aDelta. If \a aDelta
     * is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( int aDelta );

    /**
     * Modify the position and size of the rectangle in order to contain \a aRect.
     *
     * It is mainly used to calculate bounding boxes.
     *
     * @param aRect  The rectangle to merge with this rectangle.
     */
    void Merge( const EDA_RECT& aRect );

    /**
     * Modify the position and size of the rectangle in order to contain the given point.
     *
     * @param aPoint The point to merge with the rectangle.
     */
    void Merge( const VECTOR2I& aPoint );

    /**
     * Return the area of the rectangle.
     *
     * @return The area of the rectangle.
     */
    double GetArea() const;

    /**
     * Return the area that is common with another rectangle.
     *
     * @param aRect is the rectangle to find the common area with.
     * @return The common area rect or 0-sized rectangle if there is no intersection.
     */
    EDA_RECT Common( const EDA_RECT& aRect ) const;

    /**
     * Useful to calculate bounding box of rotated items, when rotation if not k*90 degrees.
     *
     * @param aAngle the rotation angle in 0.1 deg.
     * @param aRotCenter the rotation point.
     * @return the bounding box of this, after rotation.
     */
    const EDA_RECT GetBoundingBoxRotated( const VECTOR2I& aRotCenter, double aAngle ) const;

private:
    VECTOR2I m_pos;      // Rectangle Origin
    VECTOR2I m_size;     // Rectangle Size
    bool     m_init;     // Is the rectangle initialized
};


#endif // EDA_RECT_H
