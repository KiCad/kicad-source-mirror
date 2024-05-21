/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <pcb_edit_frame.h>
#include <layer_ids.h>
#include <settings/color_settings.h>
#include <board.h>
#include <pcb_layer_box_selector.h>
#include <tools/pcb_actions.h>
#include <dpi_scaling_common.h>


// class to display a layer list in a wxBitmapComboBox.

// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    Freeze();
    Clear();

    const int size = 14;

    LSET show = LSET::AllLayersMask() & ~m_layerMaskDisable;
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

        wxBitmap bmp( size, size );
        DrawColorSwatch( bmp, getLayerColor( LAYER_PCB_BACKGROUND ), getLayerColor( layerid ) );

        wxString layername = getLayerName( layerid ) + layerstatus;

        if( m_layerhotkeys )
        {
            TOOL_ACTION* action = PCB_ACTIONS::LayerIDToAction( layerid );

            if( action )
                layername = AddHotkeyName( layername, action->GetHotKey(), IS_COMMENT );
        }

        Append( layername, bmp, (void*)(intptr_t) layerid );
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
    Thaw();
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::isLayerEnabled( int aLayer ) const
{
    return getEnabledLayers().test( aLayer );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    static LSET footprintEditorLayers = LSET::AllLayersMask() & ~LSET::ForbiddenFootprintLayers();

    if( m_boardFrame )
        return m_boardFrame->GetBoard()->GetEnabledLayers();
    else
        return footprintEditorLayers;
}


// Returns a color index from the layer id
COLOR4D PCB_LAYER_BOX_SELECTOR::getLayerColor( int aLayer ) const
{
    if( m_boardFrame )
    {
        return m_boardFrame->GetColorSettings()->GetColor( aLayer );
    }
    else
    {
        SETTINGS_MANAGER&          mgr = Pgm().GetSettingsManager();
        FOOTPRINT_EDITOR_SETTINGS* settings = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();
        COLOR_SETTINGS*            current  = mgr.GetColorSettings( settings->m_ColorTheme );

        return current->GetColor( aLayer );
    }
}


// Returns the name of the layer id
wxString PCB_LAYER_BOX_SELECTOR::getLayerName( int aLayer ) const
{
    if( m_boardFrame )
        return m_boardFrame->GetBoard()->GetLayerName( ToLAYER_ID( aLayer ) );
    else
        return BOARD::GetStandardLayerName( ToLAYER_ID( aLayer ) );
}
