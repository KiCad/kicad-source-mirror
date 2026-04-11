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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PREVIEW_ITEMS_CENTERLINE_RECT_ITEM_H
#define PREVIEW_ITEMS_CENTERLINE_RECT_ITEM_H

#include <preview_items/simple_overlay_item.h>

#include <geometry/shape_poly_set.h>

#include <math/vector2d.h>

namespace KIGFX
{
class GAL;
class VIEW;

namespace PREVIEW
{
class TWO_POINT_GEOMETRY_MANAGER;

/**
 * Represent an area drawn by drawing a rectangle of a given aspect along a vector, with the
 * midpoint of one side on the start point and the mid point of the opposite side on the end.
 *
 * The center line does not have to horizontal or vertical, it can be at any angle.
 */
class CENTRELINE_RECT_ITEM : public SIMPLE_OVERLAY_ITEM
{
public:

    CENTRELINE_RECT_ITEM( const TWO_POINT_GEOMETRY_MANAGER& aGeomMgr, double aAspect );

    ///< Gets the bounding box of the rectangle
    virtual const BOX2I ViewBBox() const override;

private:

    ///< Get the rectangular outline
    SHAPE_POLY_SET getOutline() const;

    ///< Draw rectangle and center line onto GAL
    void drawPreviewShape( KIGFX::VIEW* aView ) const override;

    const TWO_POINT_GEOMETRY_MANAGER& m_geomMgr;

    ///< The aspect ratio of the rectangle to draw
    double m_aspect;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_ITEMS_CENTERLINE_RECT_ITEM_H
