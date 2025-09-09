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

#include <functional>
using namespace std::placeholders;

#include <property_holder.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <ratsnest/ratsnest_data.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <pcb_generator.h>
#include <pcb_track.h>
#include <generators/pcb_tuning_pattern.h>
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
    m_viaFilterLocked->Enable( m_delTracks->GetValue() );
    m_viaFilterUnlocked->Enable( m_delTracks->GetValue() );
    m_footprintFilterLocked->Enable( m_delFootprints->GetValue() );
    m_footprintFilterUnlocked->Enable( m_delFootprints->GetValue() );
    m_drawingFilterLocked->Enable( m_delDrawings->GetValue() );
    m_drawingFilterUnlocked->Enable( m_delDrawings->GetValue() );

    // This is a destructive dialog.  Don't save control state; we always want to come up in a benign state.
    OptOut( this );

    SetupStandardButtons();

    SetFocus();
    GetSizer()->SetSizeHints( this );
    Centre();
}


int GLOBAL_EDIT_TOOL::GlobalDeletions( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_GLOBAL_DELETION dlg( editFrame );

    dlg.SetCurrentLayer( frame()->GetActiveLayer() );

    if( dlg.ShowModal() == wxID_OK )
        dlg.DoGlobalDeletions();

    return 0;
}


void DIALOG_GLOBAL_DELETION::SetCurrentLayer( int aLayer )
{
    m_currentLayer = aLayer;
    m_rbLayersOption->SetString( 1, wxString::Format( m_rbLayersOption->GetString( 1 ),
                                                      m_Parent->GetBoard()->GetLayerName( ToLAYER_ID( aLayer ) ) ) );
    m_rbLayersOption->SetSelection( 0 );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteTracks( wxCommandEvent& event )
{
    m_trackFilterLocked->Enable( m_delTracks->GetValue() );
    m_trackFilterUnlocked->Enable( m_delTracks->GetValue() );
    m_viaFilterLocked->Enable( m_delTracks->GetValue() );
    m_viaFilterUnlocked->Enable( m_delTracks->GetValue() );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteFootprints( wxCommandEvent& event )
{
    m_footprintFilterLocked->Enable( m_delFootprints->GetValue() );
    m_footprintFilterUnlocked->Enable( m_delFootprints->GetValue() );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteDrawings( wxCommandEvent& event )
{
    bool enable = m_delDrawings->GetValue() || m_delBoardEdges->GetValue();

    m_drawingFilterLocked->Enable( enable );
    m_drawingFilterUnlocked->Enable( enable );
}


void DIALOG_GLOBAL_DELETION::onCheckDeleteBoardOutlines( wxCommandEvent& event )
{
    bool enable = m_delDrawings->GetValue() || m_delBoardEdges->GetValue();

    m_drawingFilterLocked->Enable( enable );
    m_drawingFilterUnlocked->Enable( enable );
}


void DIALOG_GLOBAL_DELETION::DoGlobalDeletions()
{
    bool gen_rastnest = false;
    bool delete_all = m_delAll->GetValue();

    // Clear selection before removing any items
    m_Parent->GetToolManager()->RunAction( ACTIONS::selectionClear );

    BOARD*       board = m_Parent->GetBoard();
    BOARD_COMMIT commit( m_Parent );
    LSET         all_layers = LSET().set();
    LSET         layers_filter;

    if( m_rbLayersOption->GetSelection() != 0 )
        layers_filter.set( m_currentLayer );
    else
        layers_filter = all_layers;

    auto processItem =
            [&]( BOARD_ITEM* item, const LSET& layers_mask )
            {
                if( ( item->GetLayerSet() & layers_mask ).any() )
                    commit.Remove( item );
            };

    auto processConnectedItem =
            [&]( BOARD_ITEM* item, const LSET& layers_mask )
            {
                if( ( item->GetLayerSet() & layers_mask ).any() )
                {
                    commit.Remove( item );
                    gen_rastnest = true;
                }
            };

    for( ZONE* zone : board->Zones() )
    {
        if( delete_all )
        {
            processConnectedItem( zone, all_layers );
        }
        else if( zone->IsTeardropArea() )
        {
            if( m_delTeardrops->GetValue() )
                processConnectedItem( zone, layers_filter );
        }
        else
        {
            if( m_delZones->GetValue() )
                processConnectedItem( zone, layers_filter );
        }
    }

    bool delete_shapes = m_delDrawings->GetValue() || m_delBoardEdges->GetValue();
    bool delete_texts = m_delTexts->GetValue();

    if( delete_all || delete_shapes || delete_texts )
    {
        // Layer mask for drawings
        LSET drawing_layers_filter;

        if( m_delDrawings->GetValue() )
            drawing_layers_filter = LSET::AllNonCuMask().set( Edge_Cuts, false );

        if( m_delBoardEdges->GetValue() )
            drawing_layers_filter.set( Edge_Cuts );

        drawing_layers_filter &= layers_filter;

        for( BOARD_ITEM* item : board->Drawings() )
        {
            if( delete_all )
            {
                processItem( item, all_layers );
            }
            else if( delete_shapes )
            {
                if( item->Type() == PCB_SHAPE_T && item->IsLocked() )
                {
                    if( m_drawingFilterLocked->GetValue() )
                        processItem( item, drawing_layers_filter );
                }
                else if( item->Type() == PCB_SHAPE_T && !item->IsLocked() )
                {
                    if( m_drawingFilterUnlocked->GetValue() )
                        processItem( item, drawing_layers_filter );
                }
            }
            else if( delete_texts )
            {
                if( item->Type() == PCB_TEXT_T || item->Type() == PCB_TEXTBOX_T )
                    processItem( item, layers_filter );
            }
        }
    }

    if( delete_all || m_delFootprints->GetValue() )
    {
        for( FOOTPRINT* footprint : board->Footprints() )
        {
            if( delete_all )
            {
                processConnectedItem( footprint, all_layers );
            }
            else if( footprint->IsLocked() )
            {
                if( m_footprintFilterLocked->GetValue() )
                    processConnectedItem( footprint, layers_filter );
            }
            else
            {
                if( m_footprintFilterUnlocked->GetValue() )
                    processConnectedItem( footprint, layers_filter );
            }
        }
    }

    if( delete_all || m_delTracks->GetValue() )
    {
        for( PCB_TRACK* track : board->Tracks() )
        {
            if( delete_all )
            {
                processConnectedItem( track, all_layers );
            }
            else if( track->Type() == PCB_VIA_T )
            {
                if( track->IsLocked() )
                {
                    if( m_viaFilterLocked->GetValue() )
                        processConnectedItem( track, layers_filter );
                }
                else
                {
                    if( m_viaFilterUnlocked->GetValue() )
                        processConnectedItem( track, layers_filter );
                }
            }
            else
            {
                if( track->IsLocked() )
                {
                    if( m_trackFilterLocked->GetValue() )
                        processConnectedItem( track, layers_filter );
                }
                else
                {
                    if( m_trackFilterUnlocked->GetValue() )
                        processConnectedItem( track, layers_filter );
                }
            }
        }

        for( PCB_GENERATOR* generator : board->Generators() )
        {
            if( PCB_TUNING_PATTERN* pattern = dynamic_cast<PCB_TUNING_PATTERN*>( generator ) )
            {
                if( pattern->GetBoardItems().empty() )
                    commit.Remove( pattern );
            }
        }
    }

    commit.Push( _( "Global Delete" ) );

    if( m_delMarkers->GetValue() )
        board->DeleteMARKERs();

    if( gen_rastnest )
        m_Parent->Compile_Ratsnest( true );

    // There is a chance that some of tracks have changed their nets, so rebuild ratsnest
    // from scratch.
    m_Parent->GetCanvas()->Refresh();
}
