/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <geometry/shape_rect.h>

/**
 * A round rectangle shape, based on a rectangle and a radius.
 *
 * For now, not an inheritor of SHAPE as that means implementing a
 * lot of Collision logic. Then again, this is a common shape, could be more efficient
 * to do the collision using arcs rather than Clipper'ing pad outsets.
 */
class ROUNDRECT
{
public:
    ROUNDRECT() : m_rect(), m_radius( 0 ) {}

    /**
     * Creates a ROUNDRECT instance
     * @param aRect in the rectangle model
     * @param aRadius is the radius of the round rect
     * @param aNormalizeOnCreate allows normalization of the created rect, i.e ensure
     * width and height > 0, regardless width and height of aRect
     * allowing normalization can modifiy the value of origin of created instance from aRect
     */
    ROUNDRECT( SHAPE_RECT aRect, int aRadius, bool aNormalizeOnCreate = false );

    static ROUNDRECT OutsetFrom( const SHAPE_RECT& aRect, int aOutset );

    int GetRoundRadius() const { return m_radius; }

    /**
     * Get the basis rectangle of the roundrect.
     *
     * This is the rectangle without the rounded corners.
     */
    const SHAPE_RECT& GetRect() const { return m_rect; }

    /**
     * Shortcut for common values
     */
    int GetWidth() const { return m_rect.GetWidth(); }

    int GetHeight() const { return m_rect.GetHeight(); }

    VECTOR2I GetPosition() const { return m_rect.GetPosition(); }

    /**
     * Get the bounding box of the roundrect.
     *
     * (This is always the same as the basis rectangle's bounding box.)
     */
    BOX2I BBox() const { return m_rect.BBox(); }

    /**
     * Get the roundrect with the size increased by aOutset in all directions.
     * (the radius increases by aOutset as well).
     */
    ROUNDRECT GetInflated( int aOutset ) const;

    /**
     * Get the polygonal representation of the roundrect.
     */
    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aMaxError ) const;

private:
    SHAPE_RECT m_rect;
    int        m_radius;
};
