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

#include <preview_items/polygon_item.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <view/view.h>

using namespace KIGFX::PREVIEW;

const double POLYGON_ITEM::POLY_LINE_WIDTH = 1;

POLYGON_ITEM::POLYGON_ITEM() :
        SIMPLE_OVERLAY_ITEM()
{
    m_lineColor = KIGFX::COLOR4D::UNSPECIFIED;
    m_leaderColor = KIGFX::COLOR4D::UNSPECIFIED;
}


void POLYGON_ITEM::SetLineColor( KIGFX::COLOR4D lineColor )
{
    m_lineColor = lineColor;
}


void POLYGON_ITEM::SetLeaderColor( KIGFX::COLOR4D leaderColor )
{
    m_leaderColor = leaderColor;
}


void POLYGON_ITEM::SetPoints( const SHAPE_LINE_CHAIN& aLockedInPts,
                              const SHAPE_LINE_CHAIN& aLeaderPts, const SHAPE_LINE_CHAIN& aLoopPts )
{
    m_lockedChain = aLockedInPts;
    m_leaderChain = aLeaderPts;
    m_loopChain = aLoopPts;

    m_polyfill.RemoveAllContours();
    m_polyfill.NewOutline();

    for( int i = 0; i < aLockedInPts.PointCount(); ++i )
        m_polyfill.Append( aLockedInPts.CPoint( i ) );

    for( int i = 0; i < aLeaderPts.PointCount(); ++i )
        m_polyfill.Append( aLeaderPts.CPoint( i ) );

    for( int i = 0; i < aLoopPts.PointCount(); ++i )
        m_polyfill.Append( aLoopPts.CPoint( i ) );
}


void POLYGON_ITEM::drawPreviewShape( KIGFX::VIEW* aView ) const
{
    KIGFX::GAL&      gal = *aView->GetGAL();
    RENDER_SETTINGS* renderSettings = aView->GetPainter()->GetSettings();

    gal.SetIsStroke( true );

    if( m_lockedChain.PointCount() >= 2 )
    {
        if( m_lineColor != KIGFX::COLOR4D::UNSPECIFIED )
            gal.SetStrokeColor( m_lineColor );

        gal.SetLineWidth( (float) aView->ToWorld( POLY_LINE_WIDTH ) );
        gal.DrawPolyline( m_lockedChain );
    }

    // draw the leader line in a different color
    if( m_leaderChain.PointCount() >= 2 )
    {
        if( m_leaderColor != KIGFX::COLOR4D::UNSPECIFIED )
            gal.SetStrokeColor( m_leaderColor );
        else
            gal.SetStrokeColor( renderSettings->GetLayerColor( LAYER_AUX_ITEMS ) );

        gal.DrawPolyline( m_leaderChain );
    }

    gal.SetIsStroke( false );

    for( int j = 0; j < m_polyfill.OutlineCount(); ++j )
    {
        const SHAPE_LINE_CHAIN& outline = m_polyfill.COutline( j );

        if( outline.PointCount() >= 2 )
            gal.DrawPolygon( outline );
    }
}


const BOX2I POLYGON_ITEM::ViewBBox() const
{
    return m_polyfill.BBox();
}
