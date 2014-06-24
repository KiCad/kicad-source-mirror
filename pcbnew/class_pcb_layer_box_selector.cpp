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

#include <class_pcb_layer_box_selector.h>


#define DECLARE_LAYERS_HOTKEY(list) int list[] = \
        { \
            HK_SWITCH_LAYER_TO_COPPER,   \
            HK_SWITCH_LAYER_TO_INNER1,   \
            HK_SWITCH_LAYER_TO_INNER2,   \
            HK_SWITCH_LAYER_TO_INNER3,   \
            HK_SWITCH_LAYER_TO_INNER4,   \
            HK_SWITCH_LAYER_TO_INNER5,   \
            HK_SWITCH_LAYER_TO_INNER6,   \
            HK_SWITCH_LAYER_TO_INNER7,   \
            HK_SWITCH_LAYER_TO_INNER8,   \
            HK_SWITCH_LAYER_TO_INNER9,   \
            HK_SWITCH_LAYER_TO_INNER10,  \
            HK_SWITCH_LAYER_TO_INNER11,  \
            HK_SWITCH_LAYER_TO_INNER12,  \
            HK_SWITCH_LAYER_TO_INNER13,  \
            HK_SWITCH_LAYER_TO_INNER14,  \
            HK_SWITCH_LAYER_TO_COMPONENT \
        };


// class to display a layer list in a wxBitmapComboBox.

// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    Clear();

    static const DECLARE_LAYERS_HOTKEY( layerhk );

    // Tray to fix a minimum width fot the BitmapComboBox
    int minwidth = 80, h;

    wxClientDC dc( GetParent() );   // The DC for "this" is not always initialized

    #define BM_SIZE 14

    LSET show = getEnabledLayers() & ~m_layerMaskDisable;

    for( LSEQ seq = show.UIOrder();  seq;  ++seq )
    {
        LAYER_ID   layerid = *seq;

        wxBitmap   layerbmp( BM_SIZE, BM_SIZE );

        SetBitmapLayer( layerbmp, layerid );

        wxString layername = GetLayerName( layerid );

        if( m_layerhotkeys && m_hotkeys && layerid < DIM(layerhk) )
        {
            layername = AddHotkeyName( layername, m_hotkeys,
                                       layerhk[layerid], IS_COMMENT );
        }

        Append( layername, layerbmp, (void*)(intptr_t) layerid );

        int w;
        dc.GetTextExtent ( layername, &w, &h );
        minwidth = std::max( minwidth, w );
    }

    minwidth += BM_SIZE + 35;    // Take in account the bitmap size and margins
    SetMinSize( wxSize( minwidth, -1 ) );
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::IsLayerEnabled( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame != NULL );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->IsLayerEnabled( (LAYER_ID) aLayer );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    wxASSERT( m_boardFrame != NULL );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->GetEnabledLayers();
}


// Returns a color index from the layer id
EDA_COLOR_T PCB_LAYER_BOX_SELECTOR::GetLayerColor( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board );

    return board->GetLayerColor( (LAYER_ID) aLayer );
}


// Returns the name of the layer id
wxString PCB_LAYER_BOX_SELECTOR::GetLayerName( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board );

    return board->GetLayerName( (LAYER_ID) aLayer );
}

