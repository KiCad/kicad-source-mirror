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

#include <gal/color4d.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiplatform/ui.h>
#include <layer_ids.h>
#include <wx/settings.h>

#include "preview_items/preview_utils.h"
#include "router_status_view_item.h"
#include "gr_text.h"

using namespace KIGFX;


const BOX2I ROUTER_STATUS_VIEW_ITEM::ViewBBox() const
{
    BOX2I tmp;

    // this is an edit-time artefact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}

std::vector<int> ROUTER_STATUS_VIEW_ITEM::ViewGetLayers() const
{
    return { LAYER_UI_START, LAYER_UI_START + 1 };
}

void ROUTER_STATUS_VIEW_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL* gal = aView->GetGAL();
    bool        viewFlipped = gal->IsFlippedX();
    bool        drawingDropShadows = ( aLayer == LAYER_UI_START );

    gal->Save();
    gal->Scale( { 1., 1. } );

    KIGFX::PREVIEW::TEXT_DIMS textDims = KIGFX::PREVIEW::GetConstantGlyphHeight( gal, -1 );
    KIGFX::PREVIEW::TEXT_DIMS hintDims = KIGFX::PREVIEW::GetConstantGlyphHeight( gal, -2 );
    KIFONT::FONT*             font = KIFONT::FONT::GetFont();
    const KIFONT::METRICS&    fontMetrics = KIFONT::METRICS::Default();
    TEXT_ATTRIBUTES           textAttrs;
    int                       textWidth;

    textWidth = std::max( GRTextWidth( m_status, font, textDims.GlyphSize, textDims.StrokeWidth,
                                       false, false, fontMetrics ),
                          GRTextWidth( m_hint, font, hintDims.GlyphSize, hintDims.StrokeWidth,
                                       false, false, fontMetrics ) );

    VECTOR2I margin = KiROUND( textDims.GlyphSize.x * 0.4, textDims.GlyphSize.y * 0.6 );
    VECTOR2I size( textWidth + margin.x, KiROUND( textDims.GlyphSize.y * 1.7 ) );
    VECTOR2I offset( margin.x * 5, -( size.y + margin.y * 5 ) );

    if( !m_hint.IsEmpty() )
        size.y += KiROUND( hintDims.GlyphSize.y * 1.2 );

    if( drawingDropShadows )
    {
        gal->SetIsFill( true );
        gal->SetIsStroke( true );
        gal->SetLineWidth( gal->GetScreenWorldMatrix().GetScale().x * 2 );
        gal->SetStrokeColor( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
        KIGFX::COLOR4D bgColor( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
        gal->SetFillColor( bgColor.WithAlpha( 0.9 ) );

        gal->DrawRectangle( GetPosition() + offset - margin,
                            GetPosition() + offset + size + margin );
        gal->Restore();
        return;
    }

    COLOR4D bg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
    COLOR4D normal = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
    COLOR4D red;

    if( viewFlipped )
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    else
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;

    gal->SetIsFill( false );
    gal->SetIsStroke( true );
    gal->SetStrokeColor( normal );
    textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;

    // Prevent text flipping when view is flipped
    if( gal->IsFlippedX() )
    {
        textAttrs.m_Mirrored = true;
        textAttrs.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    }

    textAttrs.m_Size = textDims.GlyphSize;
    textAttrs.m_StrokeWidth = textDims.StrokeWidth;

    VECTOR2I textPos = GetPosition() + offset + margin;
    font->Draw( gal, m_status, textPos, textAttrs, KIFONT::METRICS::Default() );

    if( !m_hint.IsEmpty() )
    {
        textAttrs.m_Size = hintDims.GlyphSize;
        textAttrs.m_StrokeWidth = hintDims.StrokeWidth;

        textPos.y += KiROUND( textDims.GlyphSize.y * 1.6 );
        font->Draw( gal, m_hint, textPos, textAttrs, KIFONT::METRICS::Default() );
    }

    gal->Restore();
}

