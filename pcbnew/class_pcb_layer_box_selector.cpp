/**
 * @file class_pcb_layer_box_selector.cpp
 * @brief a derived class of LAYER_BOX_SELECTOR to handle the layer box selector
 * in Pcbnew
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <common.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <colors_selection.h>
#include <layers_id_colors_and_visibility.h>

#include <class_board.h>
#include <hotkeys.h>

#include <wx/ownerdrw.h>
#include <wx/menuitem.h>
#include <wx/bmpcbox.h>
#include <wx/wx.h>

#include <class_pcb_layer_box_selector.h>

/* class to display a layer list.
 *
 */


// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    Clear();

    static DECLARE_LAYERS_ORDER_LIST( layertranscode );
    static DECLARE_LAYERS_HOTKEY( layerhk );

    for( int i = 0; i < LAYER_COUNT; i++ )
    {
        wxBitmap   layerbmp( 14, 14 );
        wxString   layername;
        int        layerid = i;

        if( m_layerorder )
            layerid = layertranscode[i];

        if( ! IsLayerEnabled( layerid ) )
            continue;

        SetBitmapLayer( layerbmp, layerid );

        layername = GetLayerName( layerid );

        if( m_layerhotkeys && m_hotkeys != NULL )
            layername = AddHotkeyName( layername, m_hotkeys, layerhk[layerid], IS_COMMENT );

        Append( layername, layerbmp, (void*)(intptr_t) layerid );
    }
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::IsLayerEnabled( int aLayerIndex ) const
{
    PCB_BASE_FRAME* pcbFrame = (PCB_BASE_FRAME*) GetParent()->GetParent();
    BOARD* board = pcbFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->IsLayerEnabled( aLayerIndex );
}


// Returns a color index from the layer id
EDA_COLOR_T PCB_LAYER_BOX_SELECTOR::GetLayerColor( int aLayerIndex ) const
{
    PCB_BASE_FRAME* pcbFrame = (PCB_BASE_FRAME*) GetParent()->GetParent();
    BOARD* board = pcbFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->GetLayerColor( aLayerIndex );
}


// Returns the name of the layer id
wxString PCB_LAYER_BOX_SELECTOR::GetLayerName( int aLayerIndex ) const
{
    PCB_BASE_FRAME* pcbFrame = (PCB_BASE_FRAME*) GetParent()->GetParent();
    BOARD* board = pcbFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->GetLayerName( aLayerIndex );
}
