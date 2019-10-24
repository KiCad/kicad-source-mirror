/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/polygon_item.h>

#include <preview_items/preview_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <painter.h>
#include <view/view.h>

using namespace KIGFX::PREVIEW;

const double POLYGON_ITEM::POLY_LINE_WIDTH = 150000.0;

POLYGON_ITEM::POLYGON_ITEM():
        SIMPLE_OVERLAY_ITEM()
{
}


void POLYGON_ITEM::SetPoints( const SHAPE_LINE_CHAIN& aLockedInPts,
                              const SHAPE_LINE_CHAIN& aLeaderPts )
{
    m_lockedChain = aLockedInPts;
    m_leaderChain = aLeaderPts;

    m_polyfill.RemoveAllContours();
    m_polyfill.NewOutline();

    for( int i = 0; i < aLockedInPts.PointCount(); ++i )
        m_polyfill.Append( aLockedInPts.CPoint( i ) );

    for( int i = 0; i < aLeaderPts.PointCount(); ++i )
        m_polyfill.Append( aLeaderPts.CPoint( i ) );
}


void POLYGON_ITEM::drawPreviewShape( KIGFX::VIEW* aView ) const
{
    auto& gal = *aView->GetGAL();
    auto rs = aView->GetPainter()->GetSettings();

    gal.SetLineWidth( POLY_LINE_WIDTH );
    gal.DrawPolyline( m_lockedChain );

    // draw the leader line in a different color
    gal.SetStrokeColor( rs->GetLayerColor( LAYER_AUX_ITEMS ) );
    gal.DrawPolyline( m_leaderChain );

    gal.DrawPolygon( m_polyfill );
}


const BOX2I POLYGON_ITEM::ViewBBox() const
{
    return m_polyfill.BBox();
}
