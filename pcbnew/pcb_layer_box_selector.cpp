/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include "pcb_layer_box_selector.h"

#include <board.h>
#include <layer_ids.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_presentation.h>
#include <settings/color_settings.h>
#include <tools/pcb_actions.h>
#include <dpi_scaling_common.h>
#include <wx/wupdlock.h>


PCB_LAYER_BOX_SELECTOR::PCB_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxString& value,
                                                const wxPoint& pos, const wxSize& size, int n,
                                                const wxString choices[], int style ) :
        LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices ),
        m_boardFrame( nullptr ),
        m_showNotEnabledBrdlayers( false ),
        m_layerPresentation( std::make_unique<PCB_LAYER_PRESENTATION>( nullptr ) ) // The parent isn't always the frame
{
}


void PCB_LAYER_BOX_SELECTOR::SetBoardFrame( PCB_BASE_FRAME* aFrame )
{
    m_boardFrame = aFrame;
    m_layerPresentation->SetBoardFrame( m_boardFrame );
}


// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    wxWindowUpdateLocker updateLock( this );

    Clear();

    const int size = 14;

    LSET show = ( LSET::AllCuMask() | LSET::AllNonCuMask() ) & ~m_layerMaskDisable;
    LSET activated = getEnabledLayers() & ~m_layerMaskDisable;
    wxString layerstatus;

    for( PCB_LAYER_ID layerid : show.UIOrder() )
    {
        if( !m_showNotEnabledBrdlayers && !activated[layerid] )
            continue;
        else if( !activated[layerid] )
            layerstatus = wxT( " " ) + _( "(not activated)" );
        else
            layerstatus.Empty();

        wxVector<wxBitmap> bitmaps;

        for( int scale = 1; scale <= 3; scale++ )
        {
            wxBitmap bmp( size * scale, size * scale );

            m_layerPresentation->DrawColorSwatch( bmp, layerid );

            bmp.SetScaleFactor( scale );
            bitmaps.push_back( bmp );
        }

        wxString layername = m_layerPresentation->getLayerName( layerid ) + layerstatus;

        if( m_layerhotkeys )
        {
            TOOL_ACTION* action = PCB_ACTIONS::LayerIDToAction( layerid );

            if( action )
                layername = AddHotkeyName( layername, action->GetHotKey(), IS_COMMENT );
        }

        Append( layername, wxBitmapBundle::FromBitmaps( bitmaps ), (void*) (intptr_t) layerid );
    }

    if( !m_undefinedLayerName.IsEmpty() )
        Append( m_undefinedLayerName, wxNullBitmap, (void*)(intptr_t)UNDEFINED_LAYER );

    // Ensure the size of the widget is enough to show the text and the icon
    // We have to have a selected item when doing this, because otherwise GTK
    // will just choose a random size that might not fit the actual data
    // (such as in cases where the font size is very large). So we select
    // the first item, get the size of the control and make that the minimum size,
    // then remove the selection (which was the initial state).
    SetSelection( 0 );

    SetMinSize( wxSize( -1, -1 ) );
    wxSize bestSize = GetBestSize();

    bestSize.x = GetBestSize().x + size + 10;
    SetMinSize( bestSize );

    SetSelection( wxNOT_FOUND );
    Fit();
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::isLayerEnabled( int aLayer ) const
{
    return getEnabledLayers().test( aLayer );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    static const LSET footprintEditorLayers = LSET::AllLayersMask();

    if( m_boardFrame )
        return m_boardFrame->GetBoard()->GetEnabledLayers();
    else
        return footprintEditorLayers;
}
