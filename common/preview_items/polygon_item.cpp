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
#include <view/view.h>

using namespace KIGFX::PREVIEW;


POLYGON_ITEM::POLYGON_ITEM():
        SIMPLE_OVERLAY_ITEM()
{
}

void POLYGON_ITEM::SetPoints( const std::vector<VECTOR2I>& aLockedPts,
                              const std::vector<VECTOR2I>& aLeaderPts )
{
    m_lockedChain.Clear();
    m_leaderChain.Clear();

    m_polyfill.RemoveAllContours();
    m_polyfill.NewOutline();

    for( auto& pt: aLockedPts )
    {
        m_lockedChain.Append( pt, false );
        m_polyfill.Append( pt );
    }

    for( auto& pt: aLeaderPts )
    {
        m_leaderChain.Append( pt, false );
        m_polyfill.Append( pt );
    }
}


void POLYGON_ITEM::drawPreviewShape( KIGFX::GAL& aGal ) const
{
    aGal.DrawPolyline( m_lockedChain );
    aGal.DrawPolygon( m_polyfill );

    // draw the leader line in a different color
    aGal.SetStrokeColor( PreviewOverlayDefaultColor() );
    aGal.DrawPolyline( m_leaderChain );
}


const BOX2I POLYGON_ITEM::ViewBBox() const
{
    return m_polyfill.BBox();
}
