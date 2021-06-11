/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
using namespace std::placeholders;

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <ratsnest/ratsnest_data.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/global_edit_tool.h>
#include <dialog_global_deletion.h>


DIALOG_GLOBAL_DELETION::DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent ) :
    DIALOG_GLOBAL_DELETION_BASE( parent )
{
    m_Parent = parent;
    m_currentLayer = F_Cu;
    m_trackFilterLocked->Enable( m_delTracks->GetValue() );
    m_trackFilterUnlocked->Enable( m_delTracks->GetValue() );
    m_trackFilterVias->Enable( m_delTracks->GetValue() );
    m_footprintFilterLocked->Enable( m_delFootprints->GetValue() );
    m_footprintFilterUnlocked->Enable( m_delFootprints->GetValue() );
    m_drawingFilterLocked->Enable( m_delDrawings->GetValue() );
    m_drawingFilterUnlocked->Enable( m_delDrawings->GetValue() );
    m_sdbSizer1OK->SetDefault();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Centre();
}


int GLOBAL_EDIT_TOOL::GlobalDeletions( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GLOBAL_DELETION dlg( editFrame );

    dlg.SetCurrentLayer( frame()->GetActiveLayer() );
    dlg.ShowModal();
    return 0;
}


void DIALOG_GLOBAL_DELETION::SetCurrentLayer( LAYER_NUM aLayer )
{
    m_currentLayer = aLayer;
    m_textCtrlCurrLayer->SetValue( m_Parent->GetBoard()->GetLayerName( ToLAYER_ID( aLayer ) ) );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteTracks( wxCommandEvent& event )
{
    m_trackFilterLocked->Enable( m_delTracks->GetValue() );
    m_trackFilterUnlocked->Enable( m_delTracks->GetValue() );
    m_trackFilterVias->Enable( m_delTracks->GetValue() );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteFootprints( wxCommandEvent& event )
{
    m_footprintFilterLocked->Enable( m_delFootprints->GetValue() );
    m_footprintFilterUnlocked->Enable( m_delFootprints->GetValue() );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteDrawings( wxCommandEvent& event )
{
    m_drawingFilterLocked->Enable( m_delDrawings->GetValue() );
    m_drawingFilterUnlocked->Enable( m_delDrawings->GetValue() );
}


void DIALOG_GLOBAL_DELETION::doGlobalDeletions()
{
    bool gen_rastnest = false;

    // Clear selection before removing any items
    m_Parent->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    bool delete_all = false;

    if( m_delAll->GetValue() )
    {
        if( !IsOK( this, _( "Are you sure you want to delete the entire board?" ) ) )
            return;

        delete_all = true;
    }
    else if( !IsOK( this, _( "Are you sure you want to delete the selected items?" ) ) )
    {
        return;
    }

    BOARD*       board = m_Parent->GetBoard();
    BOARD_COMMIT commit( m_Parent );
    LSET         layers_filter = LSET().set();

    if( m_rbLayersOption->GetSelection() != 0 )     // Use current layer only
        layers_filter = LSET( ToLAYER_ID( m_currentLayer ) );

    if( delete_all || m_delZones->GetValue() )
    {
        int area_index = 0;
        auto item = board->GetArea( area_index );

        while( item )
        {
            if( delete_all || layers_filter[item->GetLayer()] )
            {
                commit.Remove( item );
                gen_rastnest = true;
            }

            area_index++;
            item = board->GetArea( area_index );
        }
    }

    bool delete_shapes = m_delDrawings->GetValue() || m_delBoardEdges->GetValue();
    bool delete_texts = m_delTexts->GetValue();

    if( delete_all || delete_shapes || delete_texts )
    {
        // Layer mask for texts
        LSET del_text_layers = layers_filter;

        // Layer mask for drawings
        LSET masque_layer;

        if( m_delDrawings->GetValue() )
            masque_layer = LSET::AllNonCuMask().set( Edge_Cuts, false );

        if( m_delBoardEdges->GetValue() )
            masque_layer.set( Edge_Cuts );

        masque_layer &= layers_filter;

        for( BOARD_ITEM* item : board->Drawings() )
        {
            KICAD_T   type = item->Type();
            LAYER_NUM layer = item->GetLayer();

            if( !delete_all )
            {
                if( type == PCB_SHAPE_T )
                {
                    if( !delete_shapes || !masque_layer[layer] )
                        continue;

                    if( item->IsLocked() && !m_drawingFilterLocked->GetValue() )
                        continue;

                    if( !item->IsLocked() && !m_drawingFilterUnlocked->GetValue() )
                        continue;
                }
                else if( type == PCB_TEXT_T )
                {
                    if( !delete_texts || !del_text_layers[layer] )
                        continue;
                }
            }

            commit.Remove( item );
        }
    }

    if( delete_all || m_delFootprints->GetValue() )
    {
        for( FOOTPRINT* footprint : board->Footprints() )
        {
            if( !delete_all )
            {
                if( footprint->IsLocked() && !m_footprintFilterLocked->GetValue() )
                    continue;

                if( !footprint->IsLocked() && !m_footprintFilterUnlocked->GetValue() )
                    continue;

                if( !layers_filter[footprint->GetLayer()] )
                    continue;
            }

            commit.Remove( footprint );
            gen_rastnest = true;
        }
    }

    if( delete_all || m_delTracks->GetValue() )
    {
        for( PCB_TRACK* track : board->Tracks() )
        {
            if( !delete_all )
            {
                if( track->Type() == PCB_TRACE_T )
                {
                    if( track->IsLocked() && !m_trackFilterLocked->GetValue() )
                        continue;

                    if( !track->IsLocked() && !m_trackFilterUnlocked->GetValue() )
                        continue;
                }

                if( ( track->Type() == PCB_VIA_T ) && !m_trackFilterVias->GetValue() )
                    continue;

                if( ( track->GetLayerSet() & layers_filter ) == 0 )
                    continue;
            }

            commit.Remove( track );
            gen_rastnest = true;
        }
    }

    commit.Push( "Global delete" );

    if( m_delMarkers->GetValue() )
        board->DeleteMARKERs();

    if( gen_rastnest )
        m_Parent->Compile_Ratsnest( true );

    // There is a chance that some of tracks have changed their nets, so rebuild ratsnest from scratch
    m_Parent->GetCanvas()->Refresh();
}
