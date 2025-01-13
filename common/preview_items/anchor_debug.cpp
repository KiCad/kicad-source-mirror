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

#include "preview_items/anchor_debug.h"

#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <render_settings.h>

using namespace KIGFX;

ANCHOR_DEBUG::ANCHOR_DEBUG() :
        EDA_ITEM( nullptr, NOT_USED ) // Never added to a BOARD/SCHEMATIC so it needs no type
{
}


ANCHOR_DEBUG* ANCHOR_DEBUG::Clone() const
{
    return new ANCHOR_DEBUG();
}


const BOX2I ANCHOR_DEBUG::ViewBBox() const
{
    // We could be a bit more careful here, but also we need to
    // know the world scale to cover everything exactly, and there
    // is only one of these.
    BOX2I bbox;
    bbox.SetMaximum();
    return bbox;
}


std::vector<int> ANCHOR_DEBUG::ViewGetLayers() const
{
    return { LAYER_GP_OVERLAY };
}


void ANCHOR_DEBUG::ClearAnchors()
{
    m_nearest.reset();
    m_anchors.clear();
}


void ANCHOR_DEBUG::AddAnchor( const VECTOR2I& aAnchor )
{
    m_anchors[aAnchor]++;
}


void ANCHOR_DEBUG::SetNearest( const OPT_VECTOR2I& aNearest )
{
    m_nearest = aNearest;
}


void ANCHOR_DEBUG::ViewDraw( int, VIEW* aView ) const
{
    GAL& gal = *aView->GetGAL();
    RENDER_SETTINGS& settings = *aView->GetPainter()->GetSettings();

    const COLOR4D textColor = settings.GetLayerColor( LAYER_AUX_ITEMS );

    const BOX2I viewport = BOX2ISafe( aView->GetViewport() );

    gal.SetIsFill( false );
    gal.SetIsStroke( true );
    gal.SetLineWidth( 1 );

    const int      markerRad = aView->ToWorld( 3 );
    const int      markerTextHeight = aView->ToWorld( 6 );
    const int      markerTextGap = aView->ToWorld( 3 );
    const int      summaryTextHeight = aView->ToWorld( 10 );
    const VECTOR2I textOffset = { markerRad + markerTextGap, 0 };

    TEXT_ATTRIBUTES attributes;
    attributes.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attributes.m_Size = VECTOR2I( markerTextHeight, markerTextHeight );

    const KIFONT::METRICS& fontMetrics = KIFONT::METRICS::Default();
    const KIFONT::FONT&    font = *KIFONT::FONT::GetFont();

    std::size_t total = 0;

    for( const auto& [anchor, count] : m_anchors )
    {
        if( m_nearest && *m_nearest == anchor )
            gal.SetStrokeColor( RED );
        else
            gal.SetStrokeColor( YELLOW );

        gal.DrawCircle( anchor, markerRad );

        const std::string countStr = std::to_string( count );
        font.Draw( &gal, countStr, anchor + textOffset, attributes, fontMetrics );

        total += count;
    }

    gal.SetStrokeColor( textColor );

    const int boundaryMargin = aView->ToWorld( 20 );
    VECTOR2I  fontPos{ viewport.GetLeft(), viewport.GetTop() };
    fontPos += VECTOR2I{ boundaryMargin, boundaryMargin };

    attributes.m_Size = VECTOR2I{ summaryTextHeight, summaryTextHeight };

    wxString totalStr = wxString::Format( "Current snap anchors: %lu", total );
    font.Draw( &gal, totalStr, fontPos, attributes, fontMetrics );
}
