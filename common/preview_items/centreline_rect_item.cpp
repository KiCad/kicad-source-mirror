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

#include <preview_items/centreline_rect_item.h>
#include <preview_items/two_point_geom_manager.h>
#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

static SHAPE_POLY_SET getRectangleAlongCentreLine( const VECTOR2D& aClStart,
                                                   const VECTOR2D& aClEnd, double aAspect )
{
    SHAPE_POLY_SET poly;
    poly.NewOutline();

    /*
     * The point layout of the rectangle goes like this,
     * but start/end don't have to be horz/vert
     *
     *  0 ---------------- 1 -----
     *  |                  |     ^
     *  s--------cl------->e   |cl|/aspect
     *  |                  |     v
     *  3----------------- 2 -----
     */

    // vector down the centre line of the rectangle
    VECTOR2D cl   = aClEnd - aClStart;

    // don't allow degenerate polygons
    if( cl.x == 0 && cl.y == 0 )
        cl.x = 1.0;

    // the "side" of the rectangle is the centre line rotated by 90 deg
    // and scaled by the aspect ratio
    VECTOR2D side = cl;
    RotatePoint( side, -ANGLE_90 );
    side = side * aAspect;

    VECTOR2D pt = aClStart + ( side / 2.0 );
    poly.Append( pt );

    pt += cl;
    poly.Append( pt );

    pt -= side;
    poly.Append( pt );

    pt -= cl;
    poly.Append( pt );

    return poly;
}


CENTRELINE_RECT_ITEM::CENTRELINE_RECT_ITEM( const TWO_POINT_GEOMETRY_MANAGER& aGeomMgr,
                                            double aAspect ) :
        m_geomMgr( aGeomMgr ),
        m_aspect( aAspect )
{
}


SHAPE_POLY_SET CENTRELINE_RECT_ITEM::getOutline() const
{
    return getRectangleAlongCentreLine( m_geomMgr.GetOrigin(), m_geomMgr.GetEnd(), m_aspect );
}


const BOX2I CENTRELINE_RECT_ITEM::ViewBBox() const
{
    return getOutline().BBox();
}


void CENTRELINE_RECT_ITEM::drawPreviewShape( KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    gal.DrawLine( m_geomMgr.GetOrigin(), m_geomMgr.GetEnd() );
    gal.DrawPolygon( getOutline() );
}
