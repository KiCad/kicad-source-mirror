/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013 CERN
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

#ifndef __BOX2_H
#define __BOX2_H

#include <algorithm>
#include <limits>
#include <optional>

#include <math/vector2d.h>
#include <geometry/eda_angle.h>
#include <core/kicad_algo.h>
#include <trigo.h>

/**
 * A 2D bounding box built on top of an origin point and size vector.
 */
template <class Vec>
class BOX2
{
public:
    typedef typename Vec::coord_type               coord_type;
    typedef typename Vec::extended_type            size_type;
    typedef typename Vec::extended_type            ecoord_type;
    typedef VECTOR2<size_type>                     SizeVec;
    typedef std::numeric_limits<coord_type>        coord_limits;

    constexpr BOX2() :
        m_Pos( 0, 0 ),
        m_Size( 0, 0 ),
        m_init( false )
    {};

    constexpr BOX2( const Vec& aPos, const SizeVec& aSize = SizeVec(0, 0) ) :
        m_Pos( aPos ),
        m_Size( aSize ),
        m_init( true )
    {
        // Range check
        KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) + m_Size.x );
        KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) + m_Size.y );

        Normalize();
    }

    static constexpr BOX2<Vec> ByCorners( const Vec& aCorner1, const Vec& aCorner2 )
    {
        return BOX2( aCorner1, aCorner2 - aCorner1 );
    }

    static constexpr BOX2<Vec> ByCenter( const Vec& aCenter, const SizeVec& aSize )
    {
        return BOX2( aCenter - aSize / 2, aSize );
    }

    constexpr void SetMaximum()
    {
        if constexpr( std::is_floating_point<coord_type>() )
        {
            m_Pos.x = m_Pos.y = coord_limits::lowest() / 2.0;
            m_Size.x = m_Size.y = coord_limits::max();
        }
        else
        {
            // We want to be able to invert the box, so don't use lowest()
            m_Pos.x = m_Pos.y = -coord_limits::max();
            m_Size.x = m_Size.y = size_type( coord_limits::max() ) + coord_limits::max();
        }

        m_init = true;
    }

    constexpr Vec Centre() const
    {
        return Vec( KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) + m_Size.x / 2 ),
                    KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) + m_Size.y / 2 ) );
    }

    /**
     * Compute the bounding box from a given list of points.
     *
     * @param aPointList is the list points of the object.
     */
    template <class Container>
    void Compute( const Container& aPointList )
    {
        Vec vmin, vmax;

        typename Container::const_iterator i;

        if( !aPointList.size() )
            return;

        vmin = vmax = aPointList[0];

        for( i = aPointList.begin(); i != aPointList.end(); ++i )
        {
            Vec p( *i );
            vmin.x  = std::min( vmin.x, p.x );
            vmin.y  = std::min( vmin.y, p.y );
            vmax.x  = std::max( vmax.x, p.x );
            vmax.y  = std::max( vmax.y, p.y );
        }

        SetOrigin( vmin );
        SetSize( vmax - vmin );
    }

    /**
     * Move the rectangle by the \a aMoveVector.
     *
     * @param aMoveVector is a point that is the value to move this rectangle.
     */
    constexpr void Move( const Vec& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Ensure that the height and width are positive.
     */
    constexpr BOX2<Vec>& Normalize()
    {
        if( m_Size.y < 0 )
        {
            m_Size.y = -m_Size.y;
            m_Pos.y = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) - m_Size.y );
        }

        if( m_Size.x < 0 )
        {
            m_Size.x = -m_Size.x;
            m_Pos.x = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) - m_Size.x );
        }

        return *this;
    }

    /**
     * @param aPoint is the point to test.
     *
     * @return true if \a aPoint is inside the boundary box. A point on a edge is seen as inside.
     */
    constexpr bool Contains( const Vec& aPoint ) const
    {
        Vec rel_pos = aPoint - m_Pos;
        Vec size    = m_Size;

        if( size.x < 0 )
        {
            size.x    = -size.x;
            rel_pos.x += size.x;
        }

        if( size.y < 0 )
        {
            size.y    = -size.y;
            rel_pos.y += size.y;
        }

        return ( rel_pos.x >= 0 ) && ( rel_pos.y >= 0 ) && ( rel_pos.y <= size.y) &&
               ( rel_pos.x <= size.x);
    }

    /**
     * @param x is the x coordinate of the point to test.
     * @param y is the x coordinate of the point to test.
     * @return true if point is inside the boundary box. A point on a edge is seen as inside.
     */
    constexpr bool Contains( coord_type x, coord_type y ) const { return Contains( Vec( x, y ) ); }

    /**
     * @param aRect is the the area to test.
     *
     * @return true if \a aRect is contained. A common edge is seen as contained.
     */
    constexpr bool Contains( const BOX2<Vec>& aRect ) const
    {
        return Contains( aRect.GetOrigin() ) && Contains( aRect.GetEnd() );
    }

    constexpr const SizeVec& GetSize() const { return m_Size; }
    constexpr coord_type GetX() const { return m_Pos.x; }
    constexpr coord_type GetY() const { return m_Pos.y; }

    constexpr const Vec& GetOrigin() const { return m_Pos; }
    constexpr const Vec& GetPosition() const { return m_Pos; }
    constexpr const Vec GetEnd() const { return Vec( GetRight(), GetBottom() ); }

    constexpr size_type GetWidth() const { return m_Size.x; }
    constexpr size_type GetHeight() const { return m_Size.y; }

    constexpr coord_type GetRight() const
    {
        return KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) + m_Size.x );
    }

    constexpr coord_type GetBottom() const
    {
        return KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) + m_Size.y );
    }

    // Compatibility aliases
    constexpr coord_type GetLeft() const { return GetX(); }
    constexpr coord_type GetTop() const { return GetY(); }
    constexpr const Vec GetCenter() const { return Centre(); }

    /**
     * @return the width or height, whichever is greater.
     */
    constexpr int GetSizeMax() const { return ( m_Size.x > m_Size.y ) ? m_Size.x : m_Size.y; }

    constexpr void SetOrigin( const Vec& pos )
    {
        m_Pos = pos;
        m_init = true;
    }

    constexpr void SetOrigin( coord_type x, coord_type y )
    {
        SetOrigin( Vec( x, y ) );
    }

    constexpr void SetSize( const SizeVec& size )
    {
        m_Size = size;
        m_init = true;
    }

    constexpr void SetSize( size_type w, size_type h )
    {
        SetSize( SizeVec( w, h ) );
    }

    constexpr void Offset( coord_type dx, coord_type dy )
    {
        m_Pos.x += dx;
        m_Pos.y += dy;
    }

    constexpr void Offset( const Vec& offset )
    {
        Offset( offset.x, offset.y );
    }

    constexpr BOX2<Vec> GetWithOffset( const Vec& aMoveVector ) const
    {
        BOX2<Vec> ret( *this );
        ret.Move( aMoveVector );
        return ret;
    }

    constexpr void SetX( coord_type val )
    {
        SetOrigin( val, m_Pos.y );
    }

    constexpr void SetY( coord_type val )
    {
        SetOrigin( m_Pos.x, val );
    }

    constexpr void SetWidth( size_type val )
    {
        SetSize( val, m_Size.y );
    }

    constexpr void SetHeight( size_type val )
    {
        SetSize( m_Size.x, val );
    }

    constexpr void SetEnd( coord_type x, coord_type y )
    {
        SetEnd( Vec( x, y ) );
    }

    constexpr void SetEnd( const Vec& pos )
    {
        SetSize( SizeVec( pos ) - m_Pos );
    }

    /**
     * @return true if the argument rectangle intersects this rectangle.
     *         (i.e. if the 2 rectangles have at least a common point)
     */
    constexpr bool Intersects( const BOX2<Vec>& aRect ) const
    {
        // this logic taken from wxWidgets' geometry.cpp file:
        bool        rc;

        BOX2<Vec>   me( *this );
        BOX2<Vec>   rect( aRect );
        me.Normalize();         // ensure size is >= 0
        rect.Normalize();       // ensure size is >= 0

        // calculate the left common area coordinate:
        ecoord_type left = std::max( me.m_Pos.x, rect.m_Pos.x );

        // calculate the right common area coordinate:
        ecoord_type right = std::min( ecoord_type( me.m_Pos.x ) + me.m_Size.x,
                                      ecoord_type( rect.m_Pos.x ) + rect.m_Size.x );

        // calculate the upper common area coordinate:
        ecoord_type top = std::max( me.m_Pos.y, rect.m_Pos.y );

        // calculate the lower common area coordinate:
        ecoord_type bottom = std::min( ecoord_type( me.m_Pos.y ) + me.m_Size.y,
                                       ecoord_type( rect.m_Pos.y ) + rect.m_Size.y );

        // if a common area exists, it must have a positive (null accepted) size
        if( left <= right && top <= bottom )
            rc = true;
        else
            rc = false;

        return rc;
    }

    /**
     * @return true if this rectangle intersects \a aRect.
     */
    constexpr BOX2<Vec> Intersect( const BOX2<Vec>& aRect )
    {
        BOX2<Vec> me( *this );
        BOX2<Vec> rect( aRect );
        me.Normalize();         // ensure size is >= 0
        rect.Normalize();       // ensure size is >= 0

        Vec topLeft, bottomRight;

        topLeft.x = std::max( me.m_Pos.x, rect.m_Pos.x );

        bottomRight.x = std::min( size_type( me.m_Pos.x ) + me.m_Size.x,
                                  size_type( rect.m_Pos.x ) + rect.m_Size.x );

        topLeft.y = std::max( me.m_Pos.y, rect.m_Pos.y );

        bottomRight.y = std::min( size_type( me.m_Pos.y ) + me.m_Size.y,
                                  size_type( rect.m_Pos.y ) + rect.m_Size.y );

        if( topLeft.x < bottomRight.x && topLeft.y < bottomRight.y )
            return BOX2<Vec>( topLeft, SizeVec( bottomRight ) - topLeft );
        else
            return BOX2<Vec>( Vec( 0, 0 ), SizeVec( 0, 0 ) );
    }

    /**
     * @return true if this rectangle intersects a line from \a aPoint1 to \a aPoint2
     */
    bool Intersects( const Vec& aPoint1, const Vec& aPoint2 ) const
    {
        Vec point2, point4;

        if( Contains( aPoint1 ) || Contains( aPoint2 ) )
            return true;

        point2.x = GetEnd().x;
        point2.y = GetOrigin().y;
        point4.x = GetOrigin().x;
        point4.y = GetEnd().y;

        //Only need to test 3 sides since a straight line can't enter and exit on same side
        if( SegmentIntersectsSegment( aPoint1, aPoint2, GetOrigin(), point2 ) )
            return true;

        if( SegmentIntersectsSegment( aPoint1, aPoint2, point2, GetEnd() ) )
            return true;

        if( SegmentIntersectsSegment( aPoint1, aPoint2, GetEnd(), point4 ) )
            return true;

        return false;
    }

    /**
     * @return true if this rectangle intersects a rotated rect given by \a aRect and
     *         \a aRotaiton.
     */
    bool Intersects( const BOX2<Vec>& aRect, const EDA_ANGLE& aRotation ) const
    {
        if( !m_init )
            return false;

        EDA_ANGLE rotation = aRotation;
        rotation.Normalize();

        /*
         * Most rectangles will be axis aligned.  It is quicker to check for this case and pass
         * the rect to the simpler intersection test.
         */

        // Prevent floating point comparison errors
        static const EDA_ANGLE ROT_EPSILON( 0.000000001, DEGREES_T );

        static const EDA_ANGLE ROT_PARALLEL[]      = { ANGLE_0, ANGLE_180, ANGLE_360 };
        static const EDA_ANGLE ROT_PERPENDICULAR[] = { ANGLE_0, ANGLE_90,  ANGLE_270 };

        // Test for non-rotated rectangle
        for( EDA_ANGLE ii : ROT_PARALLEL )
        {
            if( std::abs( rotation - ii ) < ROT_EPSILON )
                return Intersects( aRect );
        }

        // Test for rectangle rotated by multiple of 90 degrees
        for( EDA_ANGLE jj : ROT_PERPENDICULAR )
        {
            if( std::abs( rotation - jj ) < ROT_EPSILON )
            {
                BOX2<Vec> rotRect;

                // Rotate the supplied rect by 90 degrees
                rotRect.SetOrigin( aRect.Centre() );
                rotRect.Inflate( aRect.GetHeight(), aRect.GetWidth() );
                return Intersects( rotRect );
            }
        }

        /* There is some non-orthogonal rotation.
         * There are three cases to test:
         * A) One point of this rect is inside the rotated rect
         * B) One point of the rotated rect is inside this rect
         * C) One of the sides of the rotated rect intersect this
         */

        VECTOR2I corners[4];

        /* Test A : Any corners exist in rotated rect? */
        corners[0] = VECTOR2I( GetLeft(), GetTop() );
        corners[1] = VECTOR2I( GetRight(), GetTop() );
        corners[2] = VECTOR2I( GetRight(), GetBottom() );
        corners[3] = VECTOR2I( GetLeft(), GetBottom() );

        VECTOR2I rCentre = aRect.Centre();

        for( int i = 0; i < 4; i++ )
        {
            VECTOR2I delta = corners[i] - rCentre;
            RotatePoint( delta, -rotation );
            delta += rCentre;

            if( aRect.Contains( delta ) )
                return true;
        }

        /* Test B : Any corners of rotated rect exist in this one? */
        int w = KiCheckedCast<ecoord_type, coord_type>( aRect.GetWidth() / 2 );
        int h = KiCheckedCast<ecoord_type, coord_type>( aRect.GetHeight() / 2 );

        // Construct corners around center of shape
        corners[0] = VECTOR2I( -w, -h );
        corners[1] = VECTOR2I( w, -h );
        corners[2] = VECTOR2I( w, h );
        corners[3] = VECTOR2I( -w, h );

        // Rotate and test each corner
        for( int j = 0; j < 4; j++ )
        {
            RotatePoint( corners[j], rotation );
            corners[j] += rCentre;

            if( Contains( corners[j] ) )
                return true;
        }

        /* Test C : Any sides of rotated rect intersect this */
        if( Intersects( corners[0], corners[1] ) || Intersects( corners[1], corners[2] )
                || Intersects( corners[2], corners[3] ) || Intersects( corners[3], corners[0] ) )
        {
            return true;
        }

        return false;
    }

    /**
     * @return true if this rectangle intersects the circle defined by \a aCenter and \a aRadius.
     */
    bool IntersectsCircle( const Vec& aCenter, const int aRadius ) const
    {
        if( !m_init )
            return false;

        Vec closest = NearestPoint( aCenter );

        double dx = static_cast<double>( aCenter.x ) - closest.x;
        double dy = static_cast<double>( aCenter.y ) - closest.y;

        double r = static_cast<double>( aRadius );

        return ( dx * dx + dy * dy ) <= ( r * r );
    }

    /**
     * @return true if this rectangle intersects the edge of a circle defined by \a aCenter
     *         and \a aRadius.
     */
    bool IntersectsCircleEdge( const Vec& aCenter, const int aRadius, const int aWidth ) const
    {
        if( !m_init )
            return false;

        BOX2<Vec> me( *this );
        me.Normalize(); // ensure size is >= 0

        // Test if the circle intersects at all
        if( !IntersectsCircle( aCenter, aRadius + aWidth / 2 ) )
            return false;

        Vec farpt = FarthestPointTo( aCenter );
        // Farthest point must be further than the inside of the line
        double fx = (double) farpt.x - aCenter.x;
        double fy = (double) farpt.y - aCenter.y;

        double r = (double) aRadius - (double) aWidth / 2;

        return ( fx * fx + fy * fy ) > ( r * r );
    }

    const std::string Format() const
    {
        std::stringstream ss;

        ss << "( box corner " << m_Pos.Format() << " w " << m_Size.x << " h " << m_Size.y << " )";

        return ss.str();
    }

    /**
     * Inflates the rectangle horizontally by \a dx and vertically by \a dy. If \a dx
     * and/or \a dy is negative the rectangle is deflated.
     */
    constexpr BOX2<Vec>& Inflate( coord_type dx, coord_type dy )
    {
        if( m_Size.x >= 0 )
        {
            if( m_Size.x < -2 * dx )
            {
                // Don't allow deflate to eat more width than we have,
                m_Pos.x = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) + m_Size.x / 2 );
                m_Size.x = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.x  -= dx;
                m_Size.x +=  2 * dx;
            }
        }
        else    // size.x < 0:
        {
            if( m_Size.x > 2 * dx )
            {
                // Don't allow deflate to eat more width than we have,
                m_Pos.x = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.x ) - m_Size.x / 2 );
                m_Size.x = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.x  += dx;
                m_Size.x -= 2 * dx; // m_Size.x <0: inflate when dx > 0
            }
        }

        if( m_Size.y >= 0 )
        {
            if( m_Size.y < -2 * dy )
            {
                // Don't allow deflate to eat more height than we have,
                m_Pos.y = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) + m_Size.y / 2 );
                m_Size.y = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.y  -= dy;
                m_Size.y += 2 * dy;
            }
        }
        else    // size.y < 0:
        {
            if( m_Size.y > 2 * dy )
            {
                // Don't allow deflate to eat more height than we have,
                m_Pos.y = KiCheckedCast<ecoord_type, coord_type>( ecoord_type( m_Pos.y ) - m_Size.y / 2 );
                m_Size.y = 0;
            }
            else
            {
                // The inflate is valid.
                m_Pos.y  += dy;
                m_Size.y -= 2 * dy; // m_Size.y <0: inflate when dy > 0
            }
        }

        return *this;
    }

    /**
     * Inflate the rectangle horizontally and vertically by \a aDelta. If \a aDelta
     * is negative the rectangle is deflated.
     */
    constexpr BOX2<Vec>& Inflate( coord_type aDelta )
    {
        Inflate( aDelta, aDelta );
        return *this;
    }

    /**
     * Get a new rectangle that is this one, inflated by \a aDx and \a aDy.
     */
    constexpr BOX2<Vec> GetInflated( coord_type aDx, coord_type aDy ) const
    {
        BOX2<Vec> ret( *this );
        ret.Inflate( aDx, aDy );
        return ret;
    }

    /**
     * Get a new rectangle that is this one, inflated by \a aDelta.
     */
    constexpr BOX2<Vec> GetInflated( coord_type aDelta ) const
    {
        return GetInflated( aDelta, aDelta );
    }

    /**
     * Modify the position and size of the rectangle in order to contain \a aRect.
     *
     * @param aRect is the rectangle to merge with this rectangle.
     */
    constexpr BOX2<Vec>& Merge( const BOX2<Vec>& aRect )
    {
        if( !m_init )
        {
            if( aRect.m_init )
            {
                m_Pos  = aRect.GetPosition();
                m_Size = aRect.GetSize();
                m_init = true;
            }

            return *this;
        }

        Normalize();        // ensure width and height >= 0
        BOX2<Vec> rect = aRect;
        rect.Normalize();   // ensure width and height >= 0
        Vec  end = GetEnd();
        Vec  rect_end = rect.GetEnd();

        // Change origin and size in order to contain the given rect
        m_Pos.x = std::min( m_Pos.x, rect.m_Pos.x );
        m_Pos.y = std::min( m_Pos.y, rect.m_Pos.y );
        end.x   = std::max( end.x, rect_end.x );
        end.y   = std::max( end.y, rect_end.y );
        SetEnd( end );
        return *this;
    }

    /**
     * Modify the position and size of the rectangle in order to contain the given point.
     *
     * @param aPoint is the point to merge with the rectangle.
     */
    constexpr BOX2<Vec>& Merge( const Vec& aPoint )
    {
        if( !m_init )
        {
            m_Pos  = aPoint;
            m_Size = VECTOR2I( 0, 0 );
            m_init = true;
            return *this;
        }

        Normalize();        // ensure width and height >= 0

        Vec end = GetEnd();

        // Change origin and size in order to contain the given rectangle.
        m_Pos.x = std::min( m_Pos.x, aPoint.x );
        m_Pos.y = std::min( m_Pos.y, aPoint.y );
        end.x   = std::max( end.x, aPoint.x );
        end.y   = std::max( end.y, aPoint.y );
        SetEnd( end );
        return *this;
    }

    /**
     * Useful to calculate bounding box of rotated items, when rotation is not cardinal.
     *
     * @return the bounding box of this, after rotation.
     */
    const BOX2<Vec> GetBoundingBoxRotated( const VECTOR2I& aRotCenter,
                                           const EDA_ANGLE& aAngle ) const
    {
        VECTOR2I corners[4];

        // Build the corners list
        corners[0]   = GetOrigin();
        corners[2]   = GetEnd();
        corners[1].x = corners[0].x;
        corners[1].y = corners[2].y;
        corners[3].x = corners[2].x;
        corners[3].y = corners[0].y;

        // Rotate all corners, to find the bounding box
        for( int ii = 0; ii < 4; ii++ )
            RotatePoint( corners[ii], aRotCenter, aAngle );

        // Find the corners bounding box
        VECTOR2I start = corners[0];
        VECTOR2I end = corners[0];

        for( int ii = 1; ii < 4; ii++ )
        {
            start.x = std::min( start.x, corners[ii].x );
            start.y = std::min( start.y, corners[ii].y );
            end.x   = std::max( end.x, corners[ii].x );
            end.y   = std::max( end.y, corners[ii].y );
        }

        BOX2<Vec> bbox;
        bbox.SetOrigin( start );
        bbox.SetEnd( end );

        return bbox;
    }

    /**
     * Return the area of the rectangle.
     *
     * @return The area of the rectangle.
     */
    constexpr ecoord_type GetArea() const
    {
        return (ecoord_type) GetWidth() * (ecoord_type) GetHeight();
    }

    /**
     * Return the length of the diagonal of the rectangle.
     *
     * @return The length of the rectangle diagonal.
     */
    ecoord_type Diagonal() const
    {
        return m_Size.EuclideanNorm();
    }

    /**
     * Return the square of the length of the diagonal of the rectangle.
     *
     * When all you need is a comparison, this is faster than Diagonal().
     */
    constexpr ecoord_type SquaredDiagonal() const
    {
        return m_Size.SquaredEuclideanNorm();
    }

    constexpr ecoord_type SquaredDistance( const Vec& aP ) const
    {
        ecoord_type x2 = m_Pos.x + m_Size.x;
        ecoord_type y2 = m_Pos.y + m_Size.y;
        ecoord_type xdiff = std::max( aP.x < m_Pos.x ? m_Pos.x - aP.x : m_Pos.x - x2,
                                      (ecoord_type) 0 );
        ecoord_type ydiff = std::max( aP.y < m_Pos.y ? m_Pos.y - aP.y : m_Pos.y - y2,
                                      (ecoord_type) 0 );
        return xdiff * xdiff + ydiff * ydiff;
    }

    ecoord_type Distance( const Vec& aP ) const
    {
        return sqrt( SquaredDistance( aP ) );
    }

    /**
     * Return the square of the minimum distance between self and box \a aBox
     *
     * @param aBox is the other box.
     * @return The distance squared from \a aBox.
     */
    constexpr ecoord_type SquaredDistance( const BOX2<Vec>& aBox ) const
    {
        ecoord_type s = 0;

        if( aBox.m_Pos.x + aBox.m_Size.x < m_Pos.x )
        {
            ecoord_type d = aBox.m_Pos.x + aBox.m_Size.x - m_Pos.x;
            s += d * d;
        }
        else if( aBox.m_Pos.x > m_Pos.x + m_Size.x )
        {
            ecoord_type d = aBox.m_Pos.x - m_Size.x - m_Pos.x;
            s += d * d;
        }

        if( aBox.m_Pos.y + aBox.m_Size.y < m_Pos.y )
        {
            ecoord_type d = aBox.m_Pos.y + aBox.m_Size.y - m_Pos.y;
            s += d * d;
        }
        else if( aBox.m_Pos.y > m_Pos.y + m_Size.y )
        {
            ecoord_type d = aBox.m_Pos.y - m_Size.y - m_Pos.y;
            s += d * d;
        }

        return s;
    }

    /**
     * Return the minimum distance between self and \a aBox.
     *
     * @param aBox is the other box to get the distance from.
     * @return The distance from \a aBox.
     */
    ecoord_type Distance( const BOX2<Vec>& aBox ) const
    {
        return sqrt( SquaredDistance( aBox ) );
    }

    /**
     * Return the point in this rect that is closest to the provided point
     */
    constexpr Vec NearestPoint( const Vec& aPoint ) const
    {
        BOX2<Vec> me( *this );

        me.Normalize(); // ensure size is >= 0

        // Determine closest point to the circle centre within this rect
        const coord_type nx = std::clamp( aPoint.x, me.GetLeft(), me.GetRight() );
        const coord_type ny = std::clamp( aPoint.y, me.GetTop(), me.GetBottom() );

        return Vec( nx, ny );
    }

    /**
     * Return the point in this rect that is farthest from the provided point
     */
    constexpr Vec FarthestPointTo( const Vec& aPoint ) const
    {
        BOX2<Vec> me( *this );

        me.Normalize(); // ensure size is >= 0

        coord_type fx;
        coord_type fy;

        Vec center = me.GetCenter();

        if( aPoint.x < center.x )
            fx = me.GetRight();
        else
            fx = me.GetLeft();

        if( aPoint.y < center.y )
            fy = me.GetBottom();
        else
            fy = me.GetTop();

        return Vec( fx, fy );
    }

    constexpr bool operator==( const BOX2<Vec>& aOther ) const
    {
        auto t1 ( *this );
        auto t2 ( aOther );
        t1.Normalize();
        t2.Normalize();
        return ( t1.m_Pos == t2.m_Pos && t1.m_Size == t2.m_Size );
    }

    constexpr bool operator!=( const BOX2<Vec>& aOther ) const
    {
        auto t1 ( *this );
        auto t2 ( aOther );
        t1.Normalize();
        t2.Normalize();
        return ( t1.m_Pos != t2.m_Pos || t1.m_Size != t2.m_Size );
    }

    constexpr bool IsValid() const
    {
        return m_init;
    }

private:
    Vec     m_Pos;  // Rectangle Origin
    SizeVec m_Size; // Rectangle Size

    bool m_init;    // Is the rectangle initialized
};

/* Default specializations */
typedef BOX2<VECTOR2I>    BOX2I;
typedef BOX2<VECTOR2D>    BOX2D;
typedef BOX2<VECTOR2L>    BOX2L;

typedef std::optional<BOX2I> OPT_BOX2I;


inline constexpr BOX2I BOX2ISafe( const BOX2D& aInput )
{
    constexpr double high = std::numeric_limits<int>::max();
    constexpr double low = -std::numeric_limits<int>::max();

    int left = (int) std::clamp( aInput.GetLeft(), low, high );
    int top = (int) std::clamp( aInput.GetTop(), low, high );

    int64_t right = (int64_t) std::clamp( aInput.GetRight(), low, high );
    int64_t bottom = (int64_t) std::clamp( aInput.GetBottom(), low, high );

    return BOX2I( VECTOR2I( left, top ), VECTOR2L( right - left, bottom - top ) );
}


/**
 * Check if a BOX2 is safe for use with BOX2D
 * (probably BOX2D or BOX2L)
 */
template <typename Vec>
inline constexpr bool IsBOX2Safe( const BOX2<Vec>& aInput )
{
    constexpr double high = std::numeric_limits<int>::max();
    constexpr double low = -std::numeric_limits<int>::max();

    return ( aInput.GetLeft() >= low && aInput.GetTop() >= low &&
             aInput.GetRight() <= high && aInput.GetBottom() <= high );
}


inline constexpr BOX2I BOX2ISafe( const VECTOR2D& aPos, const VECTOR2D& aSize )
{
    constexpr double high = std::numeric_limits<int>::max();
    constexpr double low = -std::numeric_limits<int>::max();

    int left = (int) std::clamp( aPos.x, low, high );
    int top = (int) std::clamp( aPos.y, low, high );

    int64_t right = (int64_t) std::clamp( aPos.x + aSize.x, low, high );
    int64_t bottom = (int64_t) std::clamp( aPos.y + aSize.y, low, high );

    return BOX2I( VECTOR2I( left, top ), VECTOR2L( right - left, bottom - top ) );
}


template <typename S, std::enable_if_t<std::is_integral<S>::value, int> = 0>
inline constexpr BOX2I BOX2ISafe( const VECTOR2I& aPos, const VECTOR2<S>& aSize )
{
    constexpr int64_t high = std::numeric_limits<int>::max();
    constexpr int64_t low = -std::numeric_limits<int>::max();

    int64_t ext_right = int64_t( aPos.x ) + aSize.x;
    int64_t ext_bottom = int64_t( aPos.y ) + aSize.y;

    int64_t right = std::clamp( ext_right, low, high );
    int64_t bottom = std::clamp( ext_bottom, low, high );

    return BOX2I( aPos, VECTOR2L( right - aPos.x, bottom - aPos.y ) );
}

/* KiROUND specialization for double -> int boxes */
inline constexpr BOX2I KiROUND( const BOX2D& aBoxD )
{
    return BOX2I( KiROUND( aBoxD.GetOrigin() ), KiROUND( aBoxD.GetSize() ) );
}

#endif
