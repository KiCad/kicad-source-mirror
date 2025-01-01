/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef PREVIEW_POLYGON_ITEM__H_
#define PREVIEW_POLYGON_ITEM__H_

#include <preview_items/simple_overlay_item.h>

#include <gal/color4d.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>

namespace KIGFX
{

class GAL;
class VIEW;

namespace PREVIEW
{

/**
 * A preview item which shows an in-progress polygon, which can be used for zone outlines, etc.
 */
class POLYGON_ITEM : public SIMPLE_OVERLAY_ITEM
{

public:
    POLYGON_ITEM();

    ///< Sets the color of the preview outline
    void SetLineColor( KIGFX::COLOR4D lineColor );

    ///< Sets the color of the outline leader line
    void SetLeaderColor( KIGFX::COLOR4D leaderColor );

    ///< Gets the bounding box of the polygon
    virtual const BOX2I ViewBBox() const override;

    /**
     * Set the polygon points
     *
     * @param aLockedInPts - the "fixed points" of the outline
     * @param aLeaderPts - the lines from the last fixed point to
     *        another point, eg the cursor.
     * @param aLoopPts - the lines from the cursor to the start point.
     */
    void SetPoints( const SHAPE_LINE_CHAIN& aLockedInPts, const SHAPE_LINE_CHAIN& aLeaderPts,
                    const SHAPE_LINE_CHAIN& aLoopPts );

private:
    ///< Draw rectangle and center line onto GAL
    void drawPreviewShape( KIGFX::VIEW* aView ) const override;

    ///< complete polyline of locked in, leader and looping points
    SHAPE_LINE_CHAIN m_lockedChain, m_leaderChain, m_loopChain;

    ///< polygon fill
    SHAPE_POLY_SET m_polyfill;

    ///< the preview outline color
    KIGFX::COLOR4D m_lineColor;

    ///< the preview leader line color
    KIGFX::COLOR4D m_leaderColor;

    static const double POLY_LINE_WIDTH;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_POLYGON_ITEM__H_
