/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <tool/tool_manager.h>
#include <tools/selection_tool.h>
#include <tools/pcbnew_picker_tool.h>
#include <tools/edit_tool.h>
#include <painter.h>
#include <connectivity/connectivity_data.h>
#include <profile.h>
#include "pcb_inspection_tool.h"


PCB_INSPECTION_TOOL::PCB_INSPECTION_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InspectionTool" ),
        m_frame( nullptr )
{
    m_probingSchToPcb = false;
    m_lastNetcode = -1;

    m_slowRatsnest = false;
}


bool PCB_INSPECTION_TOOL::Init()
{
    m_ratsnestTimer.SetOwner( this );
    Connect( m_ratsnestTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( PCB_INSPECTION_TOOL::ratsnestTimer ), NULL, this );

    return true;
}


void PCB_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int PCB_INSPECTION_TOOL::ShowStatisticsDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_STATISTICS dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


int PCB_INSPECTION_TOOL::CrossProbePcbToSch( const TOOL_EVENT& aEvent )
{
    // Don't get in an infinite loop PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingSchToPcb )
        return 0;

    SELECTION_TOOL*         selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.Size() == 1 )
        m_frame->SendMessageToEESCHEMA( static_cast<BOARD_ITEM*>( selection.Front() ) );
    else
        m_frame->SendMessageToEESCHEMA( nullptr );

    return 0;
}


int PCB_INSPECTION_TOOL::HighlightItem( const TOOL_EVENT& aEvent )
{
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    m_probingSchToPcb = true;   // recursion guard
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        if( item )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, (void*) item );
    }
    m_probingSchToPcb = false;

    return 0;
}


/**
 * Look for a BOARD_CONNECTED_ITEM in a given spot and if one is found - it enables
 * highlight for its net.
 *
 * @param aPosition is the point where an item is expected (world coordinates).
 * @param aUseSelection is true if we should use the current selection to pick the netcode
 */
 bool PCB_INSPECTION_TOOL::highlightNet( const VECTOR2D& aPosition, bool aUseSelection )
{
    BOARD*                  board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();

    int net = -1;
    bool enableHighlight = false;

    if( aUseSelection )
    {
        SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

        const PCBNEW_SELECTION& selection = selectionTool->GetSelection();

        for( auto item : selection )
        {
            if( BOARD_CONNECTED_ITEM::ClassOf( item ) )
            {
                auto ci = static_cast<BOARD_CONNECTED_ITEM*>( item );

                int item_net = ci->GetNetCode();

                if( net < 0 )
                    net = item_net;
                else if( net != item_net )  // more than one net selected: do nothing
                    return false;
            }
        }

        enableHighlight = ( net >= 0 && net != settings->GetHighlightNetCode() );
    }

    // If we didn't get a net to highlight from the selection, use the cursor
    if( net < 0 )
    {
        auto guide = m_frame->GetCollectorsGuide();
        GENERAL_COLLECTOR collector;

        // Find a connected item for which we are going to highlight a net
        collector.Collect( board, GENERAL_COLLECTOR::PadsOrTracks, (wxPoint) aPosition, guide );

        if( collector.GetCount() == 0 )
            collector.Collect( board, GENERAL_COLLECTOR::Zones, (wxPoint) aPosition, guide );

        // Clear the previous highlight
        m_frame->SendMessageToEESCHEMA( nullptr );

        for( int i = 0; i < collector.GetCount(); i++ )
        {
            if( ( collector[i]->GetLayerSet() & LSET::AllCuMask() ).none() )
                collector.Remove( i );

            if( collector[i]->Type() == PCB_PAD_T )
            {
                m_frame->SendMessageToEESCHEMA( static_cast<BOARD_CONNECTED_ITEM*>( collector[i] ) );
                break;
            }
        }

        enableHighlight = ( collector.GetCount() > 0 );

        // Obtain net code for the clicked item
        if( enableHighlight )
            net = static_cast<BOARD_CONNECTED_ITEM*>( collector[0] )->GetNetCode();
    }

    // Toggle highlight when the same net was picked
    if( net > 0 && net == settings->GetHighlightNetCode() )
        enableHighlight = !settings->IsHighlightEnabled();

    if( enableHighlight != settings->IsHighlightEnabled()
            || net != settings->GetHighlightNetCode() )
    {
        m_lastNetcode = settings->GetHighlightNetCode();
        settings->SetHighlight( enableHighlight, net );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }

    // Store the highlighted netcode in the current board (for dialogs for instance)
    if( enableHighlight && net >= 0 )
    {
        board->SetHighLightNet( net );

        NETINFO_ITEM* netinfo = board->FindNet( net );

        if( netinfo )
        {
            MSG_PANEL_ITEMS items;
            netinfo->GetMsgPanelInfo( m_frame->GetUserUnits(), items );
            m_frame->SetMsgPanel( items );
            m_frame->SendCrossProbeNetName( netinfo->GetNetname() );
        }
    }
    else
    {
        board->ResetNetHighLight();
        m_frame->SetMsgPanel( board );
        m_frame->SendCrossProbeNetName( "" );
    }

    return true;
}


int PCB_INSPECTION_TOOL::HighlightNet( const TOOL_EVENT& aEvent )
{
    int                     netcode = aEvent.Parameter<intptr_t>();
    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    if( netcode > 0 )
    {
        m_lastNetcode = settings->GetHighlightNetCode();
        settings->SetHighlight( true, netcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::toggleLastNetHighlight ) )
    {
        int temp = settings->GetHighlightNetCode();
        settings->SetHighlight( true, m_lastNetcode );
        m_toolMgr->GetView()->UpdateAllLayersColor();
        m_lastNetcode = temp;
    }
    else    // Highlight the net belonging to the item under the cursor
    {
        highlightNet( getViewControls()->GetMousePosition(), false );
    }

    return 0;
}


int PCB_INSPECTION_TOOL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    BOARD*                  board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    KIGFX::RENDER_SETTINGS* settings = m_toolMgr->GetView()->GetPainter()->GetSettings();

    board->ResetNetHighLight();
    settings->SetHighlight( false );
    m_toolMgr->GetView()->UpdateAllLayersColor();
    m_frame->SetMsgPanel( board );
    m_frame->SendCrossProbeNetName( "" );
    return 0;
}


int PCB_INSPECTION_TOOL::HighlightNetTool( const TOOL_EVENT& aEvent )
{
    std::string         tool = aEvent.GetCommandStr().get();
    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    // If the keyboard hotkey was triggered and we are already in the highlight tool, behave
    // the same as a left-click.  Otherwise highlight the net of the selected item(s), or if
    // there is no selection, then behave like a ctrl-left-click.
    if( aEvent.IsAction( &PCB_ACTIONS::highlightNetSelection ) )
    {
        bool use_selection = m_frame->IsCurrentTool( PCB_ACTIONS::highlightNetTool );
        highlightNet( getViewControls()->GetMousePosition(), use_selection );
    }

    picker->SetClickHandler(
        [this] ( const VECTOR2D& pt ) -> bool
        {
            highlightNet( pt, false );
            return true;
        } );

    picker->SetLayerSet( LSET::AllCuMask() );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int PCB_INSPECTION_TOOL::LocalRatsnestTool( const TOOL_EVENT& aEvent )
{
    std::string          tool = aEvent.GetCommandStr().get();
    PCBNEW_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    BOARD*               board = getModel<BOARD>();
    PCB_DISPLAY_OPTIONS* opt = displayOptions();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetClickHandler(
        [this, board, opt]( const VECTOR2D& pt ) -> bool
        {
            SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
            m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, EDIT_TOOL::PadFilter );
            PCBNEW_SELECTION& selection = selectionTool->GetSelection();

            if( selection.Empty() )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true,
                                      EDIT_TOOL::FootprintFilter );
                selection = selectionTool->GetSelection();
            }

            if( selection.Empty() )
            {
                // Clear the previous local ratsnest if we click off all items
                for( auto mod : board->Modules() )
                {
                    for( auto pad : mod->Pads() )
                        pad->SetLocalRatsnestVisible( opt->m_ShowGlobalRatsnest );
                }
            }
            else
            {
                for( auto item : selection )
                {
                    if( auto pad = dyn_cast<D_PAD*>(item) )
                    {
                        pad->SetLocalRatsnestVisible( !pad->GetLocalRatsnestVisible() );
                    }
                    else if( auto mod = dyn_cast<MODULE*>(item) )
                    {
                        bool enable = !( *( mod->Pads().begin() ) )->GetLocalRatsnestVisible();

                        for( auto modpad : mod->Pads() )
                            modpad->SetLocalRatsnestVisible( enable );
                    }
                }
            }

            m_toolMgr->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );

            return true;
        } );

    picker->SetFinalizeHandler(
        [board, opt] ( int aCondition )
        {
            if( aCondition != PCBNEW_PICKER_TOOL::END_ACTIVATE )
            {
                for( auto mod : board->Modules() )
                {
                    for( auto pad : mod->Pads() )
                        pad->SetLocalRatsnestVisible( opt->m_ShowGlobalRatsnest );
                }
            }
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int PCB_INSPECTION_TOOL::UpdateSelectionRatsnest( const TOOL_EVENT& aEvent )
{
    auto selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto& selection = selectionTool->GetSelection();
    auto connectivity = getModel<BOARD>()->GetConnectivity();

    if( selection.Empty() )
    {
        connectivity->ClearDynamicRatsnest();
    }
    else if( m_slowRatsnest )
    {
        // Compute ratsnest only when user stops dragging for a moment
        connectivity->HideDynamicRatsnest();
        m_ratsnestTimer.Start( 20 );
    }
    else
    {
        // Check how much time doest it take to calculate ratsnest
        PROF_COUNTER counter;
        calculateSelectionRatsnest();
        counter.Stop();

        // If it is too slow, then switch to 'slow ratsnest' mode when
        // ratsnest is calculated when user stops dragging items for a moment
        if( counter.msecs() > 25 )
        {
            m_slowRatsnest = true;
            connectivity->HideDynamicRatsnest();
        }
    }

    return 0;
}


int PCB_INSPECTION_TOOL::HideDynamicRatsnest( const TOOL_EVENT& aEvent )
{
    getModel<BOARD>()->GetConnectivity()->HideDynamicRatsnest();
    m_slowRatsnest = false;
    return 0;
}


void PCB_INSPECTION_TOOL::ratsnestTimer( wxTimerEvent& aEvent )
{
    m_ratsnestTimer.Stop();
    calculateSelectionRatsnest();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
}


void PCB_INSPECTION_TOOL::calculateSelectionRatsnest()
{
    SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    SELECTION& selection = selectionTool->GetSelection();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
    std::vector<BOARD_ITEM*> items;

    for( EDA_ITEM* item : selection )
    {
        BOARD_CONNECTED_ITEM* boardItem = static_cast<BOARD_CONNECTED_ITEM*>( item );

        if( boardItem->Type() == PCB_MODULE_T )
        {
            for( auto pad : static_cast<MODULE*>( item )->Pads() )
            {
                if( pad->GetLocalRatsnestVisible() || displayOptions()->m_ShowModuleRatsnest )
                    items.push_back( pad );
            }
        }
        else if( boardItem->GetLocalRatsnestVisible() || displayOptions()->m_ShowModuleRatsnest )
        {
            items.push_back( boardItem );
        }
    }

    connectivity->ComputeDynamicRatsnest( items );
}


void PCB_INSPECTION_TOOL::setTransitions()
{
    Go( &PCB_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::SelectedEvent );
    Go( &PCB_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::UnselectedEvent );
    Go( &PCB_INSPECTION_TOOL::CrossProbePcbToSch,     EVENTS::ClearedEvent );

    Go( &PCB_INSPECTION_TOOL::LocalRatsnestTool,      PCB_ACTIONS::localRatsnestTool.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::HideDynamicRatsnest,    PCB_ACTIONS::hideDynamicRatsnest.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::UpdateSelectionRatsnest,PCB_ACTIONS::updateLocalRatsnest.MakeEvent() );

    Go( &PCB_INSPECTION_TOOL::ListNets,               PCB_ACTIONS::listNets.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::ShowStatisticsDialog,   PCB_ACTIONS::boardStatistics.MakeEvent() );

    Go( &PCB_INSPECTION_TOOL::HighlightNet,           PCB_ACTIONS::highlightNet.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::HighlightNet,           PCB_ACTIONS::highlightNetSelection.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::HighlightNet,           PCB_ACTIONS::toggleLastNetHighlight.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::ClearHighlight,         PCB_ACTIONS::clearHighlight.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::HighlightNetTool,       PCB_ACTIONS::highlightNetTool.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::ClearHighlight,         ACTIONS::cancelInteractive.MakeEvent() );
    Go( &PCB_INSPECTION_TOOL::HighlightItem,          PCB_ACTIONS::highlightItem.MakeEvent() );
}
