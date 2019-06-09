/**
 * @file class_pcb_layer_box_selector.cpp
 * @brief a derived class of LAYER_BOX_SELECTOR to handle the layer box selector
 * in Pcbnew
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <layers_id_colors_and_visibility.h>

#include <class_board.h>
#include <pcb_layer_box_selector.h>
#include <tools/pcb_actions.h>

// translate aLayer to its hotkey
static TOOL_ACTION* layer2action( PCB_LAYER_ID aLayer )
{
    switch( aLayer )
    {
    case F_Cu:      return &PCB_ACTIONS::layerTop;

    case B_Cu:      return &PCB_ACTIONS::layerBottom;

    case In1_Cu:    return &PCB_ACTIONS::layerInner1;
    case In2_Cu:    return &PCB_ACTIONS::layerInner2;
    case In3_Cu:    return &PCB_ACTIONS::layerInner3;
    case In4_Cu:    return &PCB_ACTIONS::layerInner4;
    case In5_Cu:    return &PCB_ACTIONS::layerInner5;
    case In6_Cu:    return &PCB_ACTIONS::layerInner6;

    default:
        return nullptr;
    }
}


// class to display a layer list in a wxBitmapComboBox.

// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    Clear();

    // Tray to fix a minimum width fot the BitmapComboBox
    int minwidth = 80;

    wxClientDC dc( GetParent() );   // The DC for "this" is not always initialized

    const int BM_SIZE = 14;

    LSET show = LSET::AllLayersMask() & ~m_layerMaskDisable;
    LSET activated = getEnabledLayers() & ~m_layerMaskDisable;
    wxString layerstatus;

    for( LSEQ seq = show.UIOrder();  seq;  ++seq )
    {
        PCB_LAYER_ID   layerid = *seq;

        if( !m_showNotEnabledBrdlayers && !activated[layerid] )
            continue;
        else if( !activated[layerid] )
            layerstatus = wxT( " " ) + _( "(not activated)" );
        else
            layerstatus.Empty();

        wxBitmap bmp( BM_SIZE, BM_SIZE );
        DrawColorSwatch( bmp, GetLayerColor( LAYER_PCB_BACKGROUND ), GetLayerColor( layerid ) );

        wxString layername = GetLayerName( layerid ) + layerstatus;

        if( m_layerhotkeys )
        {
            TOOL_ACTION* action = layer2action( layerid );

            if( action )
                layername = AddHotkeyName( layername, action->GetHotKey(), IS_COMMENT );
        }

        Append( layername, bmp, (void*)(intptr_t) layerid );

        int w, h;
        dc.GetTextExtent ( layername, &w, &h );
        minwidth = std::max( minwidth, w );
    }

    // Approximate bitmap size and margins
    minwidth += BM_SIZE + 32 + ConvertDialogToPixels( wxSize( 8, 0 ) ).x;
    SetMinSize( wxSize( minwidth, -1 ) );
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::IsLayerEnabled( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame != NULL );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->IsLayerEnabled( ToLAYER_ID( aLayer ) );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    wxASSERT( m_boardFrame != NULL );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board != NULL );

    return board->GetEnabledLayers();
}


// Returns a color index from the layer id
COLOR4D PCB_LAYER_BOX_SELECTOR::GetLayerColor( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame );

    return m_boardFrame->Settings().Colors().GetLayerColor( aLayer );
}


// Returns the name of the layer id
wxString PCB_LAYER_BOX_SELECTOR::GetLayerName( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame );
    BOARD* board = m_boardFrame->GetBoard();
    wxASSERT( board );

    return board->GetLayerName( ToLAYER_ID( aLayer ) );
}
