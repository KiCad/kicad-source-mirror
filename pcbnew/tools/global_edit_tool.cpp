/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <bitmaps.h>
#include <class_zone.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <tools/edit_tool.h>
#include <dialogs/dialog_track_via_properties.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialogs/dialog_swap_layers.h>
#include <tools/global_edit_tool.h>
#include <board_commit.h>

#include <memory>


GLOBAL_EDIT_TOOL::GLOBAL_EDIT_TOOL() :
    PCB_TOOL_BASE( "pcbnew.GlobalEdit" )
{
}


void GLOBAL_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool GLOBAL_EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );
    wxASSERT_MSG( m_selectionTool, "pcbnew.InteractiveSelection tool is not available" );

    return true;
}


int GLOBAL_EDIT_TOOL::ExchangeFootprints( const TOOL_EVENT& aEvent )
{
    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection( EDIT_TOOL::FootprintFilter );
    MODULE*           mod = (selection.Empty() ? nullptr : selection.FirstOfKind<MODULE> () );
    bool              updateMode = false;
    bool              currentMode = false;

    if( aEvent.IsAction( &PCB_ACTIONS::updateFootprint ) )
    {
        updateMode = true;
        currentMode = true;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::updateFootprints ) )
    {
        updateMode = true;
        currentMode = false;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::changeFootprint ) )
    {
        updateMode = false;
        currentMode = true;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::changeFootprints ) )
    {
        updateMode = false;
        currentMode = false;
    }
    else
        wxFAIL_MSG( "ExchangeFootprints: unexpected action" );

    // Footprint exchange could remove modules, so they have to be
    // removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // invoke the exchange dialog process
    {
        PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
        DIALOG_EXCHANGE_FOOTPRINTS dialog( editFrame, mod, updateMode, currentMode );
        dialog.ShowQuasiModal();
    }

    return 0;
}


bool GLOBAL_EDIT_TOOL::swapBoardItem( BOARD_ITEM* aItem, PCB_LAYER_ID* new_layer )
{
    if( new_layer[ aItem->GetLayer() ] != aItem->GetLayer() )
    {
        m_commit->Modify( aItem );
        aItem->SetLayer( new_layer[ aItem->GetLayer() ] );
        frame()->GetCanvas()->GetView()->Update( aItem, KIGFX::GEOMETRY );
        return true;
    }

    return false;
}


int GLOBAL_EDIT_TOOL::SwapLayers( const TOOL_EVENT& aEvent )
{
    PCB_LAYER_ID new_layer[PCB_LAYER_ID_COUNT];

    DIALOG_SWAP_LAYERS dlg( frame(), new_layer );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    bool hasChanges = false;

    // Change tracks.
    for( auto segm : frame()->GetBoard()->Tracks() )
    {
        if( segm->Type() == PCB_VIA_T )
        {
            VIA*         via = (VIA*) segm;
            PCB_LAYER_ID top_layer, bottom_layer;

            if( via->GetViaType() == VIA_THROUGH )
                continue;

            via->LayerPair( &top_layer, &bottom_layer );

            if( new_layer[bottom_layer] != bottom_layer || new_layer[top_layer] != top_layer )
            {
                m_commit->Modify( via );
                via->SetLayerPair( new_layer[top_layer], new_layer[bottom_layer] );
                frame()->GetCanvas()->GetView()->Update( via, KIGFX::GEOMETRY );
                hasChanges = true;
            }
        }
        else
        {
            hasChanges |= swapBoardItem( segm, new_layer );
        }
    }

    for( BOARD_ITEM* zone : frame()->GetBoard()->Zones() )
        hasChanges |= swapBoardItem( zone, new_layer );

    for( BOARD_ITEM* drawing : frame()->GetBoard()->Drawings() )
        hasChanges |= swapBoardItem( drawing, new_layer );

    if( hasChanges )
    {
        frame()->OnModify();
        m_commit->Push( "Layers moved" );
        frame()->GetCanvas()->Refresh();
    }

    return 0;
}


void GLOBAL_EDIT_TOOL::setTransitions()
{
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::updateFootprint.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::updateFootprints.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::changeFootprint.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::changeFootprints.MakeEvent() );

    Go( &GLOBAL_EDIT_TOOL::SwapLayers,           PCB_ACTIONS::swapLayers.MakeEvent() );

    Go( &GLOBAL_EDIT_TOOL::EditTracksAndVias,    PCB_ACTIONS::editTracksAndVias.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::EditTextAndGraphics,  PCB_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::GlobalDeletions,      PCB_ACTIONS::globalDeletions.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::CleanupTracksAndVias, PCB_ACTIONS::cleanupTracksAndVias.MakeEvent() );
}


