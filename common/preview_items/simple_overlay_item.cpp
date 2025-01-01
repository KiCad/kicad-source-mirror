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

#include <preview_items/simple_overlay_item.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>


using namespace KIGFX::PREVIEW;


SIMPLE_OVERLAY_ITEM::SIMPLE_OVERLAY_ITEM():
    EDA_ITEM( NOT_USED ),    // this item is never added to a BOARD so it needs no type.
    m_fillColor( WHITE ),
    m_strokeColor( WHITE ),
    m_lineWidth( 1.0 )
{
}


void SIMPLE_OVERLAY_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    setupGal( gal );
    drawPreviewShape( aView );
}


std::vector<int> SIMPLE_OVERLAY_ITEM::ViewGetLayers() const
{
    std::vector<int> aLayers{ LAYER_GP_OVERLAY };
    return aLayers;
}


void SIMPLE_OVERLAY_ITEM::setupGal( KIGFX::GAL& aGal ) const
{
    // default impl: set up the GAL options we have - the
    // overriding class can add to this if needed
    aGal.SetLineWidth( m_lineWidth );
    aGal.SetStrokeColor( m_strokeColor );
    aGal.SetFillColor( m_fillColor );
    aGal.SetIsStroke( true );
    aGal.SetIsFill( true );
}
