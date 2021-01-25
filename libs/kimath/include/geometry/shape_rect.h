/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __SHAPE_RECT_H
#define __SHAPE_RECT_H

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <math/box2.h>
#include <math/vector2d.h>

class SHAPE_RECT : public SHAPE
{
public:
    /**
     * Create an empty (0-sized) rectangle.
     */
    SHAPE_RECT() :
        SHAPE( SH_RECT ),
        m_w( 0 ),
        m_h( 0 )
    {}

    /**
     * Create a rectangle defined by top-left corner (aX0, aY0), width aW and height aH.
     */
    SHAPE_RECT( int aX0, int aY0, int aW, int aH ) :
        SHAPE( SH_RECT ),
        m_p0( aX0, aY0 ),
        m_w( aW ),
        m_h( aH )
    {}

    /**
     * Create a rectangle defined by top-left corner aP0, width aW and height aH.
     */
    SHAPE_RECT( const VECTOR2I& aP0, int aW, int aH ) :
        SHAPE( SH_RECT ),
        m_p0( aP0 ),
        m_w( aW ),
        m_h( aH )
    {}

    SHAPE_RECT( const SHAPE_RECT& aOther ) :
        SHAPE( SH_RECT ),
        m_p0( aOther.m_p0 ),
        m_w( aOther.m_w ),
        m_h( aOther.m_h )
    {};

    SHAPE* Clone() const override
    {
        return new SHAPE_RECT( *this );
    }

    /// @copydoc SHAPE::BBox()
    const BOX2I BBox( int aClearance = 0 ) const override
    {
        BOX2I bbox( VECTOR2I( m_p0.x - aClearance,  m_p0.y - aClearance ),
                    VECTOR2I( m_w + 2 * aClearance, m_h + 2 * aClearance ) );
        return bbox;
    }

    /**
     * Return length of the diagonal of the rectangle.
     *
     * @return diagonal length
     */
    int Diagonal() const
    {
        return VECTOR2I( m_w, m_h ).EuclideanNorm();
    }

    bool Collide( const SHAPE* aShape, int aClearance, VECTOR2I* aMTV ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aMTV );
    }

    bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override
    {
        return SHAPE::Collide( aShape, aClearance, aActual, aLocation );
    }

    /// @copydoc SHAPE::Collide()
    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;

    /**
     * @return the top left corner of the rectangle.
     */
    const VECTOR2I& GetPosition() const
    {
        return m_p0;
    }

    /**
     * @return the size of the rectangle.
     */
    const VECTOR2I GetSize() const
    {
        return VECTOR2I( m_w, m_h );
    }

    /**
     * @return the width of the rectangle.
     */
     const int GetWidth() const
     {
         return m_w;
     }

    /**
     * @return the height of the rectangle.
     */
    const int GetHeight() const
    {
        return m_h;
    }

    void Move( const VECTOR2I& aVector ) override
    {
        m_p0 += aVector;
    }

    /**
     * This function has limited utility for SHAPE_RECT as non-cartesian rotations will distort
     * the rectangle.  If you might need to handle non-90º rotations then the SHAPE_RECT should
     * first be converted to a SHAPE_SIMPLE which can then be free-rotated.
     */
    void Rotate( double aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override
    {
        m_p0 -= aCenter;
        m_p0 = m_p0.Rotate( aAngle );
        m_p0 += aCenter;

        if( abs( sin( aAngle ) ) == 1 )
            std::swap( m_h, m_w );
    }

    bool IsSolid() const override
    {
        return true;
    }

    const SHAPE_LINE_CHAIN Outline() const
    {
        SHAPE_LINE_CHAIN rv;
        rv.Append( m_p0 );
        rv.Append( m_p0.x, m_p0.y + m_h );
        rv.Append( m_p0.x + m_w, m_p0.y + m_h );
        rv.Append( m_p0.x + m_w, m_p0.y );
        rv.Append( m_p0 );
        rv.SetClosed( true );
        return rv;
    }

    virtual const std::string Format( ) const override;

private:
    VECTOR2I m_p0;      ///< Top-left corner
    int      m_w;       ///< Width
    int      m_h;       ///< Height
};

#endif // __SHAPE_RECT_H
