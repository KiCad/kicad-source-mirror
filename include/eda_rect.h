/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Class EDA_RECT
 * handles the component boundary box.
 * This class is similar to wxRect, but some wxRect functions are very curious,
 * and are working only if dimensions are >= 0 (not always the case in KiCad)
 * and also KiCad needs some specific method.
 * so I prefer this more suitable class
 */
class EDA_RECT
{
private:
    wxPoint m_Pos;      // Rectangle Origin
    wxSize  m_Size;     // Rectangle Size
    bool    m_init;     // Is the rectangle initialized

public:
    EDA_RECT() : m_init( false ) { };

    EDA_RECT( const wxPoint& aPos, const wxSize& aSize ) :
        m_Pos( aPos ),
        m_Size( aSize ),
        m_init( true )
    { }

    virtual ~EDA_RECT() { };

    wxPoint Centre() const
    {
        return wxPoint( m_Pos.x + ( m_Size.x >> 1 ),
                        m_Pos.y + ( m_Size.y >> 1 ) );
    }

    /**
     * Function Move
     * moves the rectangle by the \a aMoveVector.
     * @param aMoveVector A wxPoint that is the value to move this rectangle
     */
    void Move( const wxPoint& aMoveVector );

    /**
     * Function Normalize
     * ensures that the height ant width are positive.
     */
    void Normalize();

    /**
     * Function Contains
     * @param aPoint = the wxPoint to test
     * @return true if aPoint is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( const wxPoint& aPoint ) const;

    /**
     * Function Contains
     * @param x = the x coordinate of the point to test
     * @param y = the x coordinate of the point to test
     * @return true if point is inside the boundary box. A point on a edge is seen as inside
     */
    bool Contains( int x, int y ) const { return Contains( wxPoint( x, y ) ); }

    /**
     * Function Contains
     * @param aRect = the EDA_RECT to test
     * @return true if aRect is Contained. A common edge is seen as contained
     */
    bool Contains( const EDA_RECT& aRect ) const;

    const wxSize GetSize() const { return m_Size; }

    /**
     * @brief GetSizeMax
     * @return the max size dimension
     */
    int GetSizeMax() const { return ( m_Size.x > m_Size.y )?m_Size.x:m_Size.y; }

    int GetX() const { return m_Pos.x; }
    int GetY() const { return m_Pos.y; }

    const wxPoint GetOrigin() const { return m_Pos; }
    const wxPoint GetPosition() const { return m_Pos; }
    const wxPoint GetEnd() const { return wxPoint( m_Pos.x + m_Size.x, m_Pos.y + m_Size.y ); }
    const wxPoint GetCenter() const { return wxPoint( m_Pos.x + ( m_Size.x / 2 ), m_Pos.y + ( m_Size.y / 2 ) ); }

    int GetWidth() const { return m_Size.x; }
    int GetHeight() const { return m_Size.y; }
    int GetRight() const { return m_Pos.x + m_Size.x; }
    int GetLeft() const { return m_Pos.x; }
    int GetTop() const { return m_Pos.y; }
    int GetBottom() const { return m_Pos.y + m_Size.y; }    // Y axis from top to bottom

    bool IsValid() const
    {
        return m_init;
    }

    void SetOrigin( const wxPoint &pos )
    {
        m_Pos = pos;
        m_init = true;
    }

    void SetOrigin( int x, int y )
    {
        m_Pos.x = x;
        m_Pos.y = y;
        m_init = true;
    }

    void SetSize( const wxSize &size )
    {
        m_Size = size;
        m_init = true;
    }

    void SetSize( int w, int h )
    {
        m_Size.x = w;
        m_Size.y = h;
        m_init = true;
    }

    void Offset( int dx, int dy )
    {
        m_Pos.x += dx;
        m_Pos.y += dy;
    }

    void Offset( const wxPoint &offset )
    {
        m_Pos += offset;
    }

    void SetX( int val )
    {
        m_Pos.x = val;
        m_init = true;
    }

    void SetY( int val )
    {
        m_Pos.y = val;
        m_init = true;
    }

    void SetWidth( int val )
    {
        m_Size.x = val;
        m_init = true;
    }

    void SetHeight( int val )
    {
        m_Size.y = val;
        m_init = true;
    }

    void SetEnd( int x, int y )
    {
        SetEnd( wxPoint( x, y ) );
        m_init = true;
    }

    void SetEnd( const wxPoint &pos )
    {
        m_Size.x = pos.x - m_Pos.x;
        m_Size.y = pos.y - m_Pos.y;
        m_init = true;
    }

    /**
     * Function RevertYAxis
     * Mirror the rectangle from the X axis (negate Y pos and size)
     */
    void RevertYAxis()
    {
        m_Pos.y  = -m_Pos.y;
        m_Size.y = -m_Size.y;
        Normalize();
    }

    /**
     * Function Intersects
     * tests for a common area between rectangles.
     *
     * @param aRect A rectangle to test intersection with.
     * @return bool - true if the argument rectangle intersects this rectangle.
     * (i.e. if the 2 rectangles have at least a common point)
     */
    bool Intersects( const EDA_RECT& aRect ) const;

    /**
     * Tests for a common area between this rectangle,
     * and a rectangle with arbitrary rotation
     *
     * @param aRect a rectangle to test intersection with
     * @param aRot rectangle rotation (in 1/10 degrees)
     */
    bool Intersects( const EDA_RECT& aRect, double aRot ) const;

    /**
     * Function Intersects
     * tests for a common area between a segment and this rectangle.
     *
     * @param aPoint1 First point of the segment to test intersection with.
     * @param aPoint2 Second point of the segment to test intersection with.
     * @return bool - true if the argument segment intersects this rectangle.
     * (i.e. if the segment and rectangle have at least a common point)
     */
    bool Intersects( const wxPoint& aPoint1, const wxPoint& aPoint2 ) const;

    /**
     * Return the point in this rect that is closest to the provided point
     */
    const wxPoint ClosestPointTo( const wxPoint& aPoint ) const;

    /**
     * Return the point in this rect that is farthest from the provided point
     */
    const wxPoint FarthestPointTo( const wxPoint& aPoint ) const;

    /**
     * Function IntersectsCircle
     * tests for a common area between a circle and this rectangle
     *
     * @param aCenter center of the circle
     * @param aRadius radius of the circle
     */
    bool IntersectsCircle( const wxPoint& aCenter, const int aRadius ) const;

    /**
     * IntersectsCircleEdge
     * Tests for intersection between this rect and the edge (radius) of a circle
     *
     * @param aCenter center of the circle
     * @param aRadius radius of the circle
     * @param aWidth width of the circle edge
     */
    bool IntersectsCircleEdge( const wxPoint& aCenter, const int aRadius, const int aWidth ) const;

    /**
     * Function operator(wxRect)
     * overloads the cast operator to return a wxRect
     * wxRect does not accept negative values for size, so ensure the
     * wxRect size is always >= 0
     */
    operator wxRect() const
    {
        EDA_RECT rect( m_Pos, m_Size );
        rect.Normalize();
        return wxRect( rect.m_Pos, rect.m_Size );
    }

    /**
     * Function operator(BOX2I)
     * overloads the cast operator to return a BOX2I
     * @return BOX2I - this box shaped as a BOX2I object.
     */
    operator BOX2I() const
    {
        EDA_RECT rect( m_Pos, m_Size );
        rect.Normalize();
        return BOX2I( rect.GetOrigin(), rect.GetSize() );
    }

    /**
     * Function Inflate
     * inflates the rectangle horizontally by \a dx and vertically by \a dy. If \a dx
     * and/or \a dy is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( wxCoord dx, wxCoord dy );

    /**
     * Function Inflate
     * inflates the rectangle horizontally and vertically by \a aDelta. If \a aDelta
     * is negative the rectangle is deflated.
     */
    EDA_RECT& Inflate( int aDelta );

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain \a aRect.  It is
     * mainly used to calculate bounding boxes.
     * @param aRect  The rectangle to merge with this rectangle.
     */
    void Merge( const EDA_RECT& aRect );

    /**
     * Function Merge
     * modifies the position and size of the rectangle in order to contain the given point.
     * @param aPoint The point to merge with the rectangle.
     */
    void Merge( const wxPoint& aPoint );

    /**
     * Function GetArea
     * returns the area of the rectangle.
     * @return The area of the rectangle.
     */
    double GetArea() const;

    /**
     * Function Common
     * returns the area that is common with another rectangle.
     * @param aRect is the rectangle to find the common area with.
     * @return The common area rect or 0-sized rectangle if there is no intersection.
     */
    EDA_RECT Common( const EDA_RECT& aRect ) const;

    /**
     * Function GetBoundingBoxRotated
     * @return the bounding box of this, after rotation
     * @param aAngle = the rotation angle in 0.1 deg.
     * @param aRotCenter = the rotation point.
     * useful to calculate bounding box of rotated items, when
     * rotation if not k*90 degrees
     */
    const EDA_RECT GetBoundingBoxRotated( wxPoint aRotCenter, double aAngle );
};


#endif // EDA_RECT_H
