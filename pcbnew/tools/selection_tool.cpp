/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits>
#include <functional>
using namespace std::placeholders;
#include <class_board.h>
#include <class_board_item.h>
#include <class_track.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <class_zone.h>
#include <collectors.h>
#include <confirm.h>
#include <dialog_find.h>
#include <dialog_block_options.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <preview_items/selection_area.h>
#include <painter.h>
#include <bitmaps.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <router/router_tool.h>
#include <connectivity/connectivity_data.h>
#include <footprint_viewer_frame.h>
#include <id.h>
#include "tool_event_utils.h"
#include "selection_tool.h"
#include "pcb_bright_box.h"
#include "pcb_actions.h"

#include "kicad_plugin.h"


class SELECT_MENU : public ACTION_MENU
{
public:
    SELECT_MENU() :
        ACTION_MENU( true )
    {
        SetTitle( _( "Select" ) );
        SetIcon( options_generic_xpm );

        Add( PCB_ACTIONS::filterSelection );

        AppendSeparator();

        Add( PCB_ACTIONS::selectConnection );
        Add( PCB_ACTIONS::selectCopper );
        Add( PCB_ACTIONS::selectNet );
        Add( PCB_ACTIONS::selectSameSheet );
    }

private:

    void update() override
    {
        using S_C = SELECTION_CONDITIONS;

        const auto& selection = getToolManager()->GetTool<SELECTION_TOOL>()->GetSelection();

        bool connItem = S_C::OnlyTypes( GENERAL_COLLECTOR::Tracks )( selection );
        bool sheetSelEnabled = ( S_C::OnlyType( PCB_MODULE_T ) )( selection );

        Enable( getMenuId( PCB_ACTIONS::selectNet ), connItem );
        Enable( getMenuId( PCB_ACTIONS::selectCopper ), connItem );
        Enable( getMenuId( PCB_ACTIONS::selectConnection ), connItem );
        Enable( getMenuId( PCB_ACTIONS::selectSameSheet ), sheetSelEnabled );
    }

    ACTION_MENU* create() const override
    {
        return new SELECT_MENU();
    }
};


/**
 * Private implementation of firewalled private data
 */
class SELECTION_TOOL::PRIV
{
public:
    DIALOG_BLOCK_OPTIONS::OPTIONS m_filterOpts;
};


SELECTION_TOOL::SELECTION_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractiveSelection" ),
        m_frame( NULL ),
        m_additive( false ),
        m_subtractive( false ),
        m_exclusive_or( false ),
        m_multiple( false ),
        m_skip_heuristics( false ),
        m_locked( true ),
        m_priv( std::make_unique<PRIV>() )
{
}


SELECTION_TOOL::~SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


bool SELECTION_TOOL::Init()
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();

    if( frame && ( frame->IsType( FRAME_PCB_MODULE_VIEWER )
                   || frame->IsType( FRAME_PCB_MODULE_VIEWER_MODAL ) ) )
    {
        frame->AddStandardSubMenus( m_menu );
        return true;
    }

    auto selectMenu = std::make_shared<SELECT_MENU>();
    selectMenu->SetTool( this );
    m_menu.AddSubMenu( selectMenu );

    auto& menu = m_menu.GetMenu();

    menu.AddMenu( selectMenu.get(), SELECTION_CONDITIONS::NotEmpty );
    menu.AddSeparator( 1000 );

    if( frame )
        frame->AddStandardSubMenus( m_menu );

    return true;
}


void SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();
    m_locked = true;

    if( aReason == TOOL_BASE::MODEL_RELOAD )
    {
        // Remove pointers to the selected items from containers
        // without changing their properties (as they are already deleted
        // while a new board is loaded)
        m_selection.Clear();
        getView()->GetPainter()->GetSettings()->SetHighlight( false );
    }
    else
    {
        // Restore previous properties of selected items and remove them from containers
        clearSelection();
    }

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    view()->Remove( &m_selection );
    view()->Add( &m_selection );
}


int SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( m_frame->ToolStackIsEmpty() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );

        bool dragAlwaysSelects = getEditFrame<PCB_BASE_FRAME>()->GetDragSelects();
        m_additive = m_subtractive = m_exclusive_or = false;

        if( evt->Modifier( MD_SHIFT ) && evt->Modifier( MD_CTRL ) )
            m_subtractive = true;
        else if( evt->Modifier( MD_SHIFT ) )
            m_additive = true;
        else if( evt->Modifier( MD_CTRL ) )
            m_exclusive_or = true;

        // Is the user requesting that the selection list include all possible
        // items without removing less likely selection candidates
        m_skip_heuristics = !!evt->Modifier( MD_ALT );

        // Single click? Select single object
        if( evt->IsClick( BUT_LEFT ) )
        {
            selectPoint( evt->Position() );
        }

        // right click? if there is any object - show the context menu
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                selectPoint( evt->Position(), false, &selectionCancelled );
                m_selection.SetIsHover( true );
            }

            if( !selectionCancelled )
                m_menu.ShowContextMenu( m_selection );
        }

        // double click? Display the properties window
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            if( m_selection.Empty() )
                selectPoint( evt->Position() );

            m_toolMgr->RunAction( PCB_ACTIONS::properties, true );
        }

        // drag with LMB? Select multiple objects (or at least draw a selection box) or drag them
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            if( m_additive || m_subtractive || m_exclusive_or || dragAlwaysSelects )
            {
                selectMultiple();
            }
            else
            {
                // selection is empty? try to start dragging the item under the point where drag
                // started
                if( m_selection.Empty() && selectCursor() )
                    m_selection.SetIsHover( true );

                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    m_toolMgr->RunAction( PCB_ACTIONS::move, true );
                }
                else
                {
                    // No -> drag a selection box
                    selectMultiple();
                }
            }
        }

        else if( evt->IsCancel() )
        {
            clearSelection();

            if( evt->FirstResponder() == this )
                m_toolMgr->RunAction( PCB_ACTIONS::clearHighlight );
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();
        }

        else
            evt->SetPassEvent();
    }

    // This tool is supposed to be active forever
    assert( false );

    return 0;
}


PCBNEW_SELECTION& SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


PCBNEW_SELECTION& SELECTION_TOOL::RequestSelection( CLIENT_SELECTION_FILTER aClientFilter,
                                                    std::vector<BOARD_ITEM*>* aFiltered,
                                                    bool aConfirmLockedItems )
{
    bool selectionEmpty = m_selection.Empty();
    m_selection.SetIsHover( selectionEmpty );

    if( selectionEmpty )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, aClientFilter );
        m_selection.ClearReferencePoint();
    }

    if ( aConfirmLockedItems && CheckLock() == SELECTION_LOCKED )
    {
        clearSelection();
    }

    if( aClientFilter )
    {
        GENERAL_COLLECTOR collector;

        for( auto item : m_selection )
            collector.Append( item );

        aClientFilter( VECTOR2I(), collector );

        /*
         * The first step is to find the items that may have been added by the client filter
         * This can happen if the locked pads select the module instead
         */
        std::vector<EDA_ITEM*> new_items;
        std::set_difference( collector.begin(), collector.end(),
                             m_selection.begin(), m_selection.end(),
                             std::back_inserter( new_items ) );

        /**
         * The second step is to find the items that were removed by the client filter
         */
        std::vector<EDA_ITEM*> diff;
        std::set_difference( m_selection.begin(), m_selection.end(),
                             collector.begin(), collector.end(),
                             std::back_inserter( diff ) );

        if( aFiltered )
        {
            for( auto item : diff )
                aFiltered->push_back( static_cast<BOARD_ITEM*>( item ) );
        }

        /**
         * Once we find the adjustments to m_selection that are required by the client filter, we
         * apply them both
         */
        for( auto item : diff )
            unhighlight( static_cast<BOARD_ITEM*>( item ), SELECTED, &m_selection );

        for( auto item : new_items )
            highlight( static_cast<BOARD_ITEM*>( item ), SELECTED, &m_selection );

        m_frame->GetCanvas()->ForceRefresh();
    }

    return m_selection;
}


const GENERAL_COLLECTORS_GUIDE SELECTION_TOOL::getCollectorsGuide() const
{
    GENERAL_COLLECTORS_GUIDE guide( board()->GetVisibleLayers(),
                                    (PCB_LAYER_ID) view()->GetTopLayer(), view() );

    // account for the globals
    guide.SetIgnoreMTextsMarkedNoShow( ! board()->IsElementVisible( LAYER_MOD_TEXT_INVISIBLE ) );
    guide.SetIgnoreMTextsOnBack( ! board()->IsElementVisible( LAYER_MOD_TEXT_BK ) );
    guide.SetIgnoreMTextsOnFront( ! board()->IsElementVisible( LAYER_MOD_TEXT_FR ) );
    guide.SetIgnoreModulesOnBack( ! board()->IsElementVisible( LAYER_MOD_BK ) );
    guide.SetIgnoreModulesOnFront( ! board()->IsElementVisible( LAYER_MOD_FR ) );
    guide.SetIgnorePadsOnBack( ! board()->IsElementVisible( LAYER_PAD_BK ) );
    guide.SetIgnorePadsOnFront( ! board()->IsElementVisible( LAYER_PAD_FR ) );
    guide.SetIgnoreThroughHolePads( ! board()->IsElementVisible( LAYER_PADS_TH ) );
    guide.SetIgnoreModulesVals( ! board()->IsElementVisible( LAYER_MOD_VALUES ) );
    guide.SetIgnoreModulesRefs( ! board()->IsElementVisible( LAYER_MOD_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! board()->IsElementVisible( LAYER_VIA_THROUGH ) );
    guide.SetIgnoreBlindBuriedVias( ! board()->IsElementVisible( LAYER_VIA_BBLIND ) );
    guide.SetIgnoreMicroVias( ! board()->IsElementVisible( LAYER_VIA_MICROVIA ) );
    guide.SetIgnoreTracks( ! board()->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


bool SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere, bool aOnDrag,
                                  bool* aSelectionCancelledFlag,
                                  CLIENT_SELECTION_FILTER aClientFilter )
{
    auto guide = getCollectorsGuide();
    GENERAL_COLLECTOR collector;
    auto displayOpts = (PCB_DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    guide.SetIgnoreZoneFills( displayOpts->m_DisplayZonesMode != 0 );

    collector.Collect( board(),
        m_editModules ? GENERAL_COLLECTOR::ModuleItems : GENERAL_COLLECTOR::AllBoardItems,
        wxPoint( aWhere.x, aWhere.y ), guide );

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !Selectable( collector[ i ] ) || ( aOnDrag && collector[i]->IsLocked() ) )
            collector.Remove( i );
    }

    m_selection.ClearReferencePoint();

    // Allow the client to do tool- or action-specific filtering to see if we
    // can get down to a single item
    if( aClientFilter )
        aClientFilter( aWhere, collector );

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
    {
        GuessSelectionCandidates( collector, aWhere );
    }

    // If still more than one item we're going to have to ask the user.
    if( collector.GetCount() > 1 )
    {
        if( aOnDrag )
            Wait( TOOL_EVENT( TC_ANY, TA_MOUSE_UP, BUT_LEFT ) );

        if( !doSelectionMenu( &collector, _( "Clarify Selection" ) ) )
        {
            if( aSelectionCancelledFlag )
                *aSelectionCancelledFlag = true;

            return false;
        }
    }

    if( !m_additive && !m_subtractive && !m_exclusive_or )
        clearSelection();

    if( collector.GetCount() == 1 )
    {
        BOARD_ITEM* item = collector[ 0 ];

        if( m_subtractive || ( m_exclusive_or && item->IsSelected() ) )
        {
            unselect( item );
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
            return false;
        }
        else
        {
            select( item );
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
            return true;
        }
    }

    return false;
}


bool SELECTION_TOOL::selectCursor( bool aForceSelect, CLIENT_SELECTION_FILTER aClientFilter )
{
    if( aForceSelect || m_selection.Empty() )
    {
        clearSelection();
        selectPoint( getViewControls()->GetCursorPosition( false ), false, NULL, aClientFilter );
    }

    return !m_selection.Empty();
}


bool SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool cancelled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_additive && !m_subtractive && !m_exclusive_or )
                clearSelection();

            // Start drawing a selection box
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            area.SetAdditive( m_additive );
            area.SetSubtractive( m_subtractive );
            area.SetExclusiveOr( m_exclusive_or );

            view->SetVisible( &area, true );
            view->Update( &area );
            getViewControls()->SetAutoPan( true );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( false );

            // End drawing the selection box
            view->SetVisible( &area, false );

            // Mark items within the selection box as selected
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

            // Filter the view items based on the selection box
            BOX2I selectionBox = area.ViewBBox();
            view->Query( selectionBox, selectedItems );         // Get the list of selected items

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR>::iterator it, it_end;

            int width = area.GetEnd().x - area.GetOrigin().x;
            int height = area.GetEnd().y - area.GetOrigin().y;

            /* Selection mode depends on direction of drag-selection:
             * Left > Right : Select objects that are fully enclosed by selection
             * Right > Left : Select objects that are crossed by selection
             */
            bool windowSelection = width >= 0 ? true : false;
            bool anyAdded = false;
            bool anySubtracted = false;

            if( view->IsMirroredX() )
                windowSelection = !windowSelection;

            // Construct an EDA_RECT to determine BOARD_ITEM selection
            EDA_RECT selectionRect( (wxPoint) area.GetOrigin(), wxSize( width, height ) );

            selectionRect.Normalize();

            for( it = selectedItems.begin(), it_end = selectedItems.end(); it != it_end; ++it )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it->first );

                if( !item || !Selectable( item ) )
                    continue;

                if( item->HitTest( selectionRect, windowSelection ) )
                {
                    if( m_subtractive || ( m_exclusive_or && item->IsSelected() ) )
                    {
                        unselect( item );
                        anySubtracted = true;
                    }
                    else
                    {
                        select( item );
                        anyAdded = true;
                    }
                }
            }

            m_selection.SetIsHover( false );

            // Inform other potentially interested tools
            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

            if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;  // Stop waiting for events
        }
    }

    getViewControls()->SetAutoPan( false );

    // Stop drawing the selection box
    view->Remove( &area );
    m_multiple = false;         // Multiple selection mode is inactive

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


SELECTION_LOCK_FLAGS SELECTION_TOOL::CheckLock()
{
    if( !m_locked || m_editModules )
        return SELECTION_UNLOCKED;

    bool containsLocked = false;

    // Check if the selection contains locked items
    for( const auto& item : m_selection )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_T:
            if( static_cast<MODULE*>( item )->IsLocked() )
                containsLocked = true;
            break;

        case PCB_MODULE_EDGE_T:
        case PCB_MODULE_TEXT_T:
            if( static_cast<MODULE*>( item->GetParent() )->IsLocked() )
                containsLocked = true;
            break;

        default:    // suppress warnings
            break;
        }
    }

    if( containsLocked )
    {
        if( IsOK( m_frame, _( "Selection contains locked items. Do you want to continue?" ) ) )
        {
            m_locked = false;
            return SELECTION_LOCK_OVERRIDE;
        }
        else
            return SELECTION_LOCKED;
    }

    return SELECTION_UNLOCKED;
}


int SELECTION_TOOL::CursorSelection( const TOOL_EVENT& aEvent )
{
    CLIENT_SELECTION_FILTER aClientFilter = aEvent.Parameter<CLIENT_SELECTION_FILTER>();

    selectCursor( false, aClientFilter );

    return 0;
}


int SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    clearSelection();

    return 0;
}


int SELECTION_TOOL::SelectItems( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
    {
        // Perform individual selection of each item before processing the event.
        for( auto item : *items )
            select( item );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int SELECTION_TOOL::SelectItem( const TOOL_EVENT& aEvent )
{
    AddItemToSel( aEvent.Parameter<BOARD_ITEM*>() );
    return 0;
}


void SELECTION_TOOL::AddItemToSel( BOARD_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        select( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
    {
        // Perform individual unselection of each item before processing the event
        for( auto item : *items )
            unselect( item );

        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


int SELECTION_TOOL::UnselectItem( const TOOL_EVENT& aEvent )
{
    // Check if there is an item to be selected
    BOARD_ITEM* item = aEvent.Parameter<BOARD_ITEM*>();

    if( item )
    {
        unselect( item );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


void SELECTION_TOOL::BrightenItem( BOARD_ITEM* aItem )
{
    highlight( aItem, BRIGHTENED );
}


void SELECTION_TOOL::UnbrightenItem( BOARD_ITEM* aItem )
{
    unhighlight( aItem, BRIGHTENED );
}


void connectedTrackFilter( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
{
    /* Narrow the collection down to a single TRACK item for a trivial
     * connection, or multiple TRACK items for non-trivial connections.
     */
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        if( !dynamic_cast<TRACK*>( aCollector[i] ) )
            aCollector.Remove( i );
    }

    ROUTER_TOOL::NeighboringSegmentFilter( aPt, aCollector );
}


int SELECTION_TOOL::selectConnection( const TOOL_EVENT& aEvent )
{
    if( !m_selection.HasType( PCB_TRACE_T ) && !m_selection.HasType( PCB_VIA_T ) )
        selectCursor( true, connectedTrackFilter );

    if( !m_selection.HasType( PCB_TRACE_T ) && !m_selection.HasType( PCB_VIA_T ) )
        return 0;

    return expandConnection( aEvent );
}


int SELECTION_TOOL::expandConnection( const TOOL_EVENT& aEvent )
{
    // copy the selection, since we're going to iterate and modify
    auto selection = m_selection.GetItems();

    // We use the BUSY flag to mark connections
    for( auto item : selection )
        item->SetState( BUSY, false );

    for( auto item : selection )
    {
        TRACK* trackItem = dynamic_cast<TRACK*>( item );

        // Track items marked BUSY have already been visited
        //  therefore their connections have already been marked
        if( trackItem && !trackItem->GetState( BUSY ) )
        {
            if( aEvent.GetCommandId()
                    && *aEvent.GetCommandId() == PCB_ACTIONS::expandSelectedConnection.GetId() )
                selectAllItemsConnectedToItem( *trackItem );
            else
                selectAllItemsConnectedToTrack( *trackItem );
        }
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void connectedItemFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector )
{
    /* Narrow the collection down to a single BOARD_CONNECTED_ITEM for each
     * represented net.  All other items types are removed.
     */
    std::set<int> representedNets;

    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( aCollector[i] );
        if( !item )
            aCollector.Remove( i );
        else if ( representedNets.count( item->GetNetCode() ) )
            aCollector.Remove( i );
        else
            representedNets.insert( item->GetNetCode() );
    }
}


int SELECTION_TOOL::selectCopper( const TOOL_EVENT& aEvent )
{
    bool haveCopper = false;

    for( auto item : m_selection.GetItems() )
    {
        if( dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            haveCopper = true;;
    }

    if( !haveCopper )
        selectCursor( true, connectedItemFilter );

    // copy the selection, since we're going to iterate and modify
    auto selection  = m_selection.GetItems();

    for( auto item : selection )
    {
        BOARD_CONNECTED_ITEM* connItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( item );

        if( connItem )
            selectAllItemsConnectedToItem( *connItem );
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void SELECTION_TOOL::selectAllItemsConnectedToTrack( TRACK& aSourceTrack )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    auto              connectivity = board()->GetConnectivity();

    for( auto item : connectivity->GetConnectedItems(
                 static_cast<BOARD_CONNECTED_ITEM*>( &aSourceTrack ), types ) )
        select( item );
}


void SELECTION_TOOL::selectAllItemsConnectedToItem( BOARD_CONNECTED_ITEM& aSourceItem )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_VIA_T, PCB_PAD_T, EOT };
    auto connectivity = board()->GetConnectivity();

    for( auto item : connectivity->GetConnectedItems( &aSourceItem, types ) )
    {
        // We want to select items connected through pads but not pads
        // otherwise, the common use case of "Select Copper"->Delete will
        // remove footprints in addition to traces and vias
        if( item->Type() != PCB_PAD_T )
            select( item );
    }
}


void SELECTION_TOOL::selectAllItemsOnNet( int aNetCode )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_VIA_T, EOT };
    auto connectivity = board()->GetConnectivity();

    for( auto item : connectivity->GetNetItems( aNetCode, types ) )
        select( item );
}


int SELECTION_TOOL::selectNet( const TOOL_EVENT& aEvent )
{
    if( !selectCursor() )
        return 0;

    // copy the selection, since we're going to iterate and modify
    auto selection = m_selection.GetItems();

    for( auto i : selection )
    {
        auto item = static_cast<BOARD_ITEM*>( i );

        // only connected items get a net code
        if( item->IsConnected() )
        {
            auto& connItem = static_cast<BOARD_CONNECTED_ITEM&>( *item );

            selectAllItemsOnNet( connItem.GetNetCode() );
        }
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void SELECTION_TOOL::selectAllItemsOnSheet( wxString& aSheetpath )
{
    std::list<MODULE*> modList;

    // store all modules that are on that sheet
    for( auto mitem : board()->Modules() )
    {
        if( mitem != NULL && mitem->GetPath().Contains( aSheetpath ) )
        {
            modList.push_back( mitem );
        }
    }

    //Generate a list of all pads, and of all nets they belong to.
    std::list<int> netcodeList;
    std::list<BOARD_CONNECTED_ITEM*> padList;
    for( MODULE* mmod : modList )
    {
        for( auto pad : mmod->Pads() )
        {
            if( pad->IsConnected() )
            {
                netcodeList.push_back( pad->GetNetCode() );
                padList.push_back( pad );
            }
        }
    }
    // remove all duplicates
    netcodeList.sort();
    netcodeList.unique();

    // auto select trivial connections segments which are launched from the pads
    std::list<TRACK*> launchTracks;

    for( auto pad : padList )
    {
        selectAllItemsConnectedToItem( *pad );
    }

    // now we need to find all modules that are connected to each of these nets
    // then we need to determine if these modules are in the list of modules
    // belonging to this sheet ( modList )
    std::list<int> removeCodeList;
    constexpr KICAD_T padType[] = { PCB_PAD_T, EOT };

    for( int netCode : netcodeList )
    {
        for( BOARD_CONNECTED_ITEM* mitem : board()->GetConnectivity()->GetNetItems( netCode, padType ) )
        {
            if( mitem->Type() == PCB_PAD_T)
            {
                bool found = ( std::find( modList.begin(), modList.end(),
                                    mitem->GetParent() ) != modList.end() );

                if( !found )
                {
                    // if we cannot find the module of the pad in the modList
                    // then we can assume that that module is not located in the same
                    // schematic, therefore invalidate this netcode.
                    removeCodeList.push_back( netCode );
                    break;
                }
            }
        }
    }

    // remove all duplicates
    removeCodeList.sort();
    removeCodeList.unique();

    for( int removeCode : removeCodeList )
    {
        netcodeList.remove( removeCode );
    }

    std::list<BOARD_CONNECTED_ITEM*> localConnectionList;
    constexpr KICAD_T trackViaType[] = { PCB_TRACE_T, PCB_VIA_T, EOT };

    for( int netCode : netcodeList )
    {
        for( BOARD_CONNECTED_ITEM* item : board()->GetConnectivity()->GetNetItems( netCode, trackViaType ) )
        {
            localConnectionList.push_back( item );
        }
    }

    for( BOARD_ITEM* i : modList )
    {
        if( i != NULL )
            select( i );
    }

    for( BOARD_CONNECTED_ITEM* i : localConnectionList )
    {
        if( i != NULL )
            select( i );
    }
}


void SELECTION_TOOL::zoomFitSelection()
{
    //Should recalculate the view to zoom in on the selection
    auto selectionBox = m_selection.ViewBBox();
    auto view = getView();

    VECTOR2D screenSize = view->ToWorld( m_frame->GetCanvas()->GetClientSize(), false );

    if( selectionBox.GetWidth() != 0  || selectionBox.GetHeight() != 0 )
    {
        VECTOR2D vsize = selectionBox.GetSize();
        double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                fabs( vsize.y / screenSize.y ) );
        view->SetScale( scale );
        view->SetCenter( selectionBox.Centre() );
        view->Add( &m_selection );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


int SELECTION_TOOL::selectSheetContents( const TOOL_EVENT& aEvent )
{
    clearSelection();
    wxString* sheetpath = aEvent.Parameter<wxString*>();

    selectAllItemsOnSheet( *sheetpath );

    zoomFitSelection();

    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int SELECTION_TOOL::selectSameSheet( const TOOL_EVENT& aEvent )
{
    if( !selectCursor( true ) )
        return 0;

    // this function currently only supports modules since they are only
    // on one sheet.
    auto item = m_selection.Front();

    if( !item )
        return 0;

    if( item->Type() != PCB_MODULE_T )
        return 0;

    auto mod = dynamic_cast<MODULE*>( item );

    clearSelection();

    // get the lowest subsheet name for this.
    wxString sheetPath = mod->GetPath();
    sheetPath = sheetPath.BeforeLast( '/' );
    sheetPath = sheetPath.AfterLast( '/' );

    selectAllItemsOnSheet( sheetPath );

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void SELECTION_TOOL::findCallback( BOARD_ITEM* aItem )
{
    clearSelection();

    if( aItem )
    {
        select( aItem );
        m_frame->FocusOnLocation( aItem->GetPosition() );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


int SELECTION_TOOL::find( const TOOL_EVENT& aEvent )
{
    DIALOG_FIND dlg( m_frame );
    dlg.SetCallback( std::bind( &SELECTION_TOOL::findCallback, this, _1 ) );
    dlg.ShowModal();

    return 0;
}


/**
 * Function itemIsIncludedByFilter()
 *
 * Determine if an item is included by the filter specified
 *
 * @return true if the parameter indicate the items should be selected
 * by this filter (i..e not filtered out)
 */
static bool itemIsIncludedByFilter( const BOARD_ITEM& aItem,
                                    const BOARD& aBoard,
                                    const DIALOG_BLOCK_OPTIONS::OPTIONS& aBlockOpts )
{
    bool include = true;
    const PCB_LAYER_ID layer = aItem.GetLayer();

    // can skip without even checking item type
    // fixme: selecting items on invisible layers does not work in GAL
    if( !aBlockOpts.includeItemsOnInvisibleLayers
        && !aBoard.IsLayerVisible( layer ) )
    {
        include = false;
    }

    // if the item needs to be checked against the options
    if( include )
    {
        switch( aItem.Type() )
        {
        case PCB_MODULE_T:
        {
            const auto& module = static_cast<const MODULE&>( aItem );

            include = aBlockOpts.includeModules;

            if( include && !aBlockOpts.includeLockedModules )
            {
                include = !module.IsLocked();
            }

            break;
        }
        case PCB_TRACE_T:
        {
            include = aBlockOpts.includeTracks;
            break;
        }
        case PCB_VIA_T:
        {
            include = aBlockOpts.includeVias;
            break;
        }
        case PCB_ZONE_AREA_T:
        {
            include = aBlockOpts.includeZones;
            break;
        }
        case PCB_LINE_T:
        case PCB_TARGET_T:
        case PCB_DIMENSION_T:
        {
            if( layer == Edge_Cuts )
                include = aBlockOpts.includeBoardOutlineLayer;
            else
                include = aBlockOpts.includeItemsOnTechLayers;
            break;
        }
        case PCB_TEXT_T:
        {
            include = aBlockOpts.includePcbTexts;
            break;
        }
        default:
        {
            // no filtering, just select it
            break;
        }
        }
    }

    return include;
}


int SELECTION_TOOL::filterSelection( const TOOL_EVENT& aEvent )
{
    auto& opts = m_priv->m_filterOpts;
    DIALOG_BLOCK_OPTIONS dlg( m_frame, opts, false, _( "Filter selection" ) );

    const int cmd = dlg.ShowModal();

    if( cmd != wxID_OK )
        return 0;

    const auto& board = *getModel<BOARD>();

    // copy current selection
    auto selection = m_selection.GetItems();

    // clear current selection
    clearSelection();

    // copy selection items from the saved selection
    // according to the dialog options
    for( auto i : selection )
    {
        auto item = static_cast<BOARD_ITEM*>( i );
        bool include = itemIsIncludedByFilter( *item, board, opts );

        if( include )
        {
            select( item );
        }
    }
    return 0;
}


void SELECTION_TOOL::clearSelection()
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( static_cast<BOARD_ITEM*>( m_selection.Front() ), SELECTED, &m_selection );

    view()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    m_locked = true;

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
    m_toolMgr->RunAction( PCB_ACTIONS::hideDynamicRatsnest, true );
}


void SELECTION_TOOL::RebuildSelection()
{
    m_selection.Clear();

    INSPECTOR_FUNC inspector = [&] ( EDA_ITEM* item, void* testData )
    {
        if( item->IsSelected() )
        {
            EDA_ITEM* parent = item->GetParent();

            // Flags on module children might be set only because the parent is selected.
            if( parent && parent->Type() == PCB_MODULE_T && parent->IsSelected() )
                return SEARCH_CONTINUE;

            highlight( (BOARD_ITEM*) item, SELECTED, &m_selection );
        }

        return SEARCH_CONTINUE;
    };

    board()->Visit( inspector, nullptr,  m_editModules ? GENERAL_COLLECTOR::ModuleItems
                                                       : GENERAL_COLLECTOR::AllBoardItems );
}


int SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    GENERAL_COLLECTOR* collector = aEvent.Parameter<GENERAL_COLLECTOR*>();

    doSelectionMenu( collector, wxEmptyString );

    return 0;
}


bool SELECTION_TOOL::doSelectionMenu( GENERAL_COLLECTOR* aCollector, const wxString& aTitle )
{
    BOARD_ITEM*      current = nullptr;
    PCBNEW_SELECTION highlightGroup;
    ACTION_MENU      menu( true );

    highlightGroup.SetLayer( LAYER_SELECT_OVERLAY );
    getView()->Add( &highlightGroup );

    int limit = std::min( 9, aCollector->GetCount() );

    for( int i = 0; i < limit; ++i )
    {
        wxString text;
        BOARD_ITEM* item = ( *aCollector )[i];
        text = item->GetSelectMenuText( m_frame->GetUserUnits() );

        wxString menuText = wxString::Format("&%d. %s", i + 1, text );
        menu.Add( menuText, i + 1, item->GetMenuImage() );
    }

    if( aTitle.Length() )
        menu.SetTitle( aTitle );

    menu.SetIcon( info_xpm );
    menu.DisplayTitle( true );
    SetContextMenu( &menu, CMENU_NOW );

    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->Action() == TA_CHOICE_MENU_UPDATE )
        {
            if( current )
                unhighlight( current, BRIGHTENED, &highlightGroup );

            int id = *evt->GetCommandId();

            // User has pointed an item, so show it in a different way
            if( id > 0 && id <= limit )
            {
                current = ( *aCollector )[id - 1];
                highlight( current, BRIGHTENED, &highlightGroup );
            }
            else
            {
                current = NULL;
            }
        }
        else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( current )
                unhighlight( current, BRIGHTENED, &highlightGroup );

            OPT<int> id = evt->GetCommandId();

            // User has selected an item, so this one will be returned
            if( id && ( *id > 0 ) )
                current = ( *aCollector )[*id - 1];
            else
                current = NULL;

            break;
        }
    }
    getView()->Remove( &highlightGroup );

    if( current )
    {
        aCollector->Empty();
        aCollector->Append( current );
        return true;
    }

    return false;
}


BOARD_ITEM* SELECTION_TOOL::pickSmallestComponent( GENERAL_COLLECTOR* aCollector )
{
    int count = aCollector->GetPrimaryCount();     // try to use preferred layer

    if( 0 == count )
        count = aCollector->GetCount();

    for( int i = 0; i < count; ++i )
    {
        if( ( *aCollector )[i]->Type() != PCB_MODULE_T )
            return NULL;
    }

    // All are modules, now find smallest MODULE
    int minDim = 0x7FFFFFFF;
    int minNdx = 0;

    for( int i = 0; i < count; ++i )
    {
        MODULE* module = (MODULE*) ( *aCollector )[i];

        int lx = module->GetFootprintRect().GetWidth();
        int ly = module->GetFootprintRect().GetHeight();

        int lmin = std::min( lx, ly );

        if( lmin < minDim )
        {
            minDim = lmin;
            minNdx = i;
        }
    }

    return (*aCollector)[minNdx];
}


bool SELECTION_TOOL::Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly ) const
{
    // Is high contrast mode enabled?
    bool highContrast = getView()->GetPainter()->GetSettings()->GetHighContrast();

    int layers[KIGFX::VIEW::VIEW_MAX_LAYERS], layers_count;

    // Filter out items that do not belong to active layers
    std::set<unsigned int> activeLayers = getView()->GetPainter()->GetSettings()->GetActiveLayers();

    // The markers layer is considered to be always active
    activeLayers.insert( (unsigned int) LAYER_DRC );

    aItem->ViewGetLayers( layers, layers_count );

    if( highContrast )
    {
        bool onActive = false;          // Is the item on any of active layers?

        for( int i = 0; i < layers_count; ++i )
        {
            if( activeLayers.count( layers[i] ) > 0 ) // Item is on at least one of the active layers
            {
                onActive = true;
                break;
            }
        }

        if( !onActive ) // We do not want to select items that are in the background
        {
            return false;
        }
    }

    switch( aItem->Type() )
    {
    case PCB_ZONE_AREA_T:
        // Keepout zones can exist on multiple layers!
        {
            auto* zone = static_cast<const ZONE_CONTAINER*>( aItem );

            if( zone->GetIsKeepout() )
            {
                auto zoneLayers = zone->GetLayerSet().Seq();

                for( unsigned int i = 0; i < zoneLayers.size(); i++ )
                {
                    if( board()->IsLayerVisible( zoneLayers[i] ) )
                    {
                        return true;
                    }
                }

                // No active layers selected!
                return false;
            }
        }
        break;

    case PCB_TRACE_T:
        {
            if( !board()->IsElementVisible( LAYER_TRACKS ) )
                return false;
        }
        break;

    case PCB_VIA_T:
        {
            const VIA* via = static_cast<const VIA*>( aItem );

            // Check if appropriate element layer is visible
            switch( via->GetViaType() )
            {
                case VIA_THROUGH:
                    if( !board()->IsElementVisible( LAYER_VIA_THROUGH ) )
                        return false;
                    break;

                case VIA_BLIND_BURIED:
                    if( !board()->IsElementVisible( LAYER_VIA_BBLIND ) )
                        return false;
                    break;

                case VIA_MICROVIA:
                    if( !board()->IsElementVisible( LAYER_VIA_MICROVIA ) )
                        return false;
                    break;

                default:
                    wxFAIL;
                    return false;
            }

            // For vias it is enough if only one of its layers is visible
            return ( board()->GetVisibleLayers() & via->GetLayerSet() ).any();
        }

    case PCB_MODULE_T:
    {
        // In modedit, we do not want to select the module itself.
        if( m_editModules )
            return false;

        // Allow selection of footprints if some part of the footprint is visible.

        MODULE* module = const_cast<MODULE*>( static_cast<const MODULE*>( aItem ) );

        for( auto item : module->GraphicalItems() )
        {
            if( Selectable( item, true ) )
                return true;
        }

        for( auto pad : module->Pads() )
        {
            if( Selectable( pad, true ) )
                return true;
        }

        return false;
    }

    case PCB_MODULE_TEXT_T:
        // Multiple selection is only allowed in modedit mode.  In pcbnew, you have to select
        // module subparts one by one, rather than with a drag selection.  This is so you can
        // pick up items under an (unlocked) module without also moving the module's sub-parts.
        if( !m_editModules && !checkVisibilityOnly )
        {
            if( m_multiple )
                return false;
        }

        if( !m_editModules && !view()->IsVisible( aItem ) )
            return false;

        break;

    case PCB_MODULE_EDGE_T:
    case PCB_PAD_T:
    {
        // Multiple selection is only allowed in modedit mode.  In pcbnew, you have to select
        // module subparts one by one, rather than with a drag selection.  This is so you can
        // pick up items under an (unlocked) module without also moving the module's sub-parts.
        if( !m_editModules && !checkVisibilityOnly )
        {
            if( m_multiple )
                return false;
        }

        if( aItem->Type() == PCB_PAD_T )
        {
            auto pad = static_cast<const D_PAD*>( aItem );

            // In pcbnew, locked modules prevent individual pad selection.
            // In modedit, we don't enforce this as the module is assumed to be edited by design.
            if( !m_editModules && !checkVisibilityOnly )
            {
                if( pad->GetParent() && pad->GetParent()->IsLocked() )
                    return false;
            }

            // Check render mode (from the Items tab) first
            switch( pad->GetAttribute() )
            {
            case PAD_ATTRIB_STANDARD:
            case PAD_ATTRIB_HOLE_NOT_PLATED:
                if( !board()->IsElementVisible( LAYER_PADS_TH ) )
                    return false;
                break;

            case PAD_ATTRIB_CONN:
            case PAD_ATTRIB_SMD:
                if( pad->IsOnLayer( F_Cu ) && !board()->IsElementVisible( LAYER_PAD_FR ) )
                    return false;
                else if( pad->IsOnLayer( B_Cu ) && !board()->IsElementVisible( LAYER_PAD_BK ) )
                    return false;
                break;
            }

            // Otherwise, pads are selectable if any draw layer is visible

            // Shortcut: check copper layer visibility
            if( board()->IsLayerVisible( F_Cu ) && pad->IsOnLayer( F_Cu ) )
                return true;

            if( board()->IsLayerVisible( B_Cu ) && pad->IsOnLayer( B_Cu ) )
                return true;

            // Now check the non-copper layers

            bool draw_layer_visible = false;

            int pad_layers[KIGFX::VIEW::VIEW_MAX_LAYERS], pad_layers_count;
            pad->ViewGetLayers( pad_layers, pad_layers_count );

            for( int i = 0; i < pad_layers_count; ++i )
            {
                // NOTE: Only checking the regular layers (not GAL meta-layers)
                if( ( ( pad_layers[i] < PCB_LAYER_ID_COUNT ) &&
                      board()->IsLayerVisible( static_cast<PCB_LAYER_ID>( pad_layers[i] ) ) ) )
                {
                    draw_layer_visible = true;
                }
            }

            return draw_layer_visible;
        }

        break;
    }


    case PCB_MARKER_T:  // Always selectable
        return true;

    // These are not selectable
    case NOT_USED:
    case TYPE_NOT_INIT:
        return false;

    default:    // Suppress warnings
        break;
    }

    // All other items are selected only if the layer on which they exist is visible
    return board()->IsLayerVisible( aItem->GetLayer() );
}


void SELECTION_TOOL::select( BOARD_ITEM* aItem )
{
    if( aItem->IsSelected() )
    {
        return;
    }

    if( aItem->Type() == PCB_PAD_T )
    {
        MODULE* module = static_cast<MODULE*>( aItem->GetParent() );

        if( m_selection.Contains( module ) )
            return;
    }

    highlight( aItem, SELECTED, &m_selection );
    view()->Update( &m_selection );
}


void SELECTION_TOOL::unselect( BOARD_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
    view()->Update( &m_selection );

    if( m_selection.Empty() )
        m_locked = true;
}


void SELECTION_TOOL::highlight( BOARD_ITEM* aItem, int aMode, PCBNEW_SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aGroup )
    {
        // Hide the original item, so it is shown only on overlay
        view()->Hide( aItem, true );

        aGroup->Add( aItem );
    }

    // Modules are treated in a special way - when they are highlighted, we have to
    // highlight all the parts that make the module, not the module itself
    if( aItem->Type() == PCB_MODULE_T )
    {
        static_cast<MODULE*>( aItem )->RunOnChildren( [&] ( BOARD_ITEM* item )
        {
            if( aMode == SELECTED )
                item->SetSelected();
            else if( aMode == BRIGHTENED )
            {
                item->SetBrightened();

                if( aGroup )
                    aGroup->Add( item );
            }

            if( aGroup )
                view()->Hide( item, true );
        });
    }

    view()->Update( aItem );

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void SELECTION_TOOL::unhighlight( BOARD_ITEM* aItem, int aMode, PCBNEW_SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aGroup )
    {
        aGroup->Remove( aItem );

        // Restore original item visibility
        view()->Hide( aItem, false );
    }

    // Modules are treated in a special way - when they are highlighted, we have to
    // highlight all the parts that make the module, not the module itself
    if( aItem->Type() == PCB_MODULE_T )
    {
        static_cast<MODULE*>( aItem )->RunOnChildren( [&] ( BOARD_ITEM* item )
        {
            if( aMode == SELECTED )
                item->ClearSelected();
            else if( aMode == BRIGHTENED )
                item->ClearBrightened();

            // N.B. if we clear the selection flag for sub-elements, we need to also
            // remove the element from the selection group (if it exists)
            if( aGroup )
            {
                aGroup->Remove( item );

                view()->Hide( item, false );
                view()->Update( item );
            }
        });
    }

    view()->Update( aItem );

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


bool SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    VECTOR2I margin = getView()->ToWorld( VECTOR2I( GRIP_MARGIN, GRIP_MARGIN ), false );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( auto item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin.x, margin.y );    // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


static EDA_RECT getRect( const BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_MODULE_T )
        return static_cast<const MODULE*>( aItem )->GetFootprintRect();

    return aItem->GetBoundingBox();
}


static double calcArea( const BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_TRACE_T )
    {
        const TRACK* t = static_cast<const TRACK*>( aItem );
        return ( t->GetWidth() + t->GetLength() ) * t->GetWidth();
    }

    return getRect( aItem ).GetArea();
}


/*static double calcMinArea( GENERAL_COLLECTOR& aCollector, KICAD_T aType )
{
    double best = std::numeric_limits<double>::max();

    if( !aCollector.GetCount() )
        return 0.0;

    for( int i = 0; i < aCollector.GetCount(); i++ )
    {
        BOARD_ITEM* item = aCollector[i];
        if( item->Type() == aType )
            best = std::min( best, calcArea( item ) );
    }

    return best;
}*/


static double calcMaxArea( GENERAL_COLLECTOR& aCollector, KICAD_T aType )
{
    double best = 0.0;

    for( int i = 0; i < aCollector.GetCount(); i++ )
    {
        BOARD_ITEM* item = aCollector[i];
        if( item->Type() == aType )
            best = std::max( best, calcArea( item ) );
    }

    return best;
}


static inline double calcCommonArea( const BOARD_ITEM* aItem, const BOARD_ITEM* aOther )
{
    if( !aItem || !aOther )
        return 0;

    return getRect( aItem ).Common( getRect( aOther ) ).GetArea();
}


double calcRatio( double a, double b )
{
    if( a == 0.0 && b == 0.0 )
        return 1.0;

    if( b == 0.0 )
        return std::numeric_limits<double>::max();

    return a / b;
}


// The general idea here is that if the user clicks directly on a small item inside a larger
// one, then they want the small item.  The quintessential case of this is clicking on a pad
// within a footprint, but we also apply it for text within a footprint, footprints within
// larger footprints, and vias within either larger pads or longer tracks.
//
// These "guesses" presume there is area within the larger item to click in to select it.  If
// an item is mostly covered by smaller items within it, then the guesses are inappropriate as
// there might not be any area left to click to select the larger item.  In this case we must
// leave the items in the collector and bring up a Selection Clarification menu.
//
// We currently check for pads and text mostly covering a footprint, but we don’t check for
// smaller footprints mostly covering a larger footprint.
//
void SELECTION_TOOL::GuessSelectionCandidates( GENERAL_COLLECTOR& aCollector,
                                               const VECTOR2I& aWhere ) const
{
    std::set<BOARD_ITEM*> preferred;
    std::set<BOARD_ITEM*> rejected;
    std::set<BOARD_ITEM*> forced;
    wxPoint               where( aWhere.x, aWhere.y );

    // footprints which are below this percentage of the largest footprint will be considered
    // for selection; all others will not
    constexpr double footprintToFootprintMinRatio = 0.20;
    // pads which are below this percentage of their parent's area will exclude their parent
    constexpr double padToFootprintMinRatio = 0.45;
    // footprints containing items with items-to-footprint area ratio higher than this will be
    // forced to stay on the list
    constexpr double footprintMaxCoverRatio = 0.80;
    constexpr double viaToPadMinRatio = 0.50;
    constexpr double trackViaLengthRatio = 2.0;
    constexpr double trackTrackLengthRatio = 0.3;
    constexpr double textToFeatureMinRatio = 0.2;
    constexpr double textToFootprintMinRatio = 0.4;
    // If the common area of two compared items is above the following threshold, they cannot
    // be rejected (it means they overlap and it might be hard to pick one by selecting
    // its unique area).
    constexpr double commonAreaRatio = 0.6;

    PCB_LAYER_ID activeLayer = (PCB_LAYER_ID) view()->GetTopLayer();
    LSET         silkLayers( 2, B_SilkS, F_SilkS );

    if( silkLayers[activeLayer] )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            BOARD_ITEM* item = aCollector[i];
            KICAD_T type = item->Type();

            if( ( type == PCB_MODULE_TEXT_T || type == PCB_TEXT_T || type == PCB_LINE_T )
                    && silkLayers[item->GetLayer()] )
            {
                preferred.insert( item );
            }
        }

        if( preferred.size() > 0 )
        {
            aCollector.Empty();

            for( BOARD_ITEM* item : preferred )
                aCollector.Append( item );
            return;
        }
    }

    int numZones = aCollector.CountType( PCB_ZONE_AREA_T );

    // Zone edges are very specific; zone fills much less so.
    if( numZones > 0 )
    {
        for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
        {
            if( aCollector[i]->Type() == PCB_ZONE_AREA_T )
            {
                auto zone = static_cast<ZONE_CONTAINER*>( aCollector[i] );

                if( zone->HitTestForEdge( where, 5 * aCollector.GetGuide()->OnePixelInIU() ) )
                    preferred.insert( zone );
                else
                    rejected.insert( zone );
            }
        }

        if( preferred.size() > 0 )
        {
            aCollector.Empty();

            for( BOARD_ITEM* item : preferred )
                aCollector.Append( item );
            return;
        }
    }

    if( aCollector.CountType( PCB_MODULE_TEXT_T ) > 0 )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( TEXTE_MODULE* txt = dyn_cast<TEXTE_MODULE*>( aCollector[i] ) )
            {
                double textArea = calcArea( txt );

                for( int j = 0; j < aCollector.GetCount(); ++j )
                {
                    if( i == j )
                        continue;

                    BOARD_ITEM* item = aCollector[j];
                    double itemArea = calcArea( item );
                    double areaRatio = calcRatio( textArea, itemArea );
                    double commonArea = calcCommonArea( txt, item );
                    double itemCommonRatio = calcRatio( commonArea, itemArea );
                    double txtCommonRatio = calcRatio( commonArea, textArea );

                    if( item->Type() == PCB_MODULE_T )
                    {
                        // when text area is small compared to an overlapping footprint,
                        // then it's a clear sign the text is the selection target
                        if( areaRatio < textToFootprintMinRatio && itemCommonRatio < commonAreaRatio )
                            rejected.insert( item );
                    }

                    switch( item->Type() )
                    {
                        case PCB_TRACE_T:
                        case PCB_PAD_T:
                        case PCB_LINE_T:
                        case PCB_VIA_T:
                        case PCB_MODULE_T:
                            if( areaRatio > textToFeatureMinRatio && txtCommonRatio < commonAreaRatio )
                                rejected.insert( txt );
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    if( aCollector.CountType( PCB_PAD_T ) > 0 )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( D_PAD* pad = dyn_cast<D_PAD*>( aCollector[i] ) )
            {
                MODULE* parent = pad->GetParent();
                double ratio = calcRatio( calcArea( pad ), calcArea( parent ) );

                // when pad area is small compared to the parent footprint,
                // then it is a clear sign the pad is the selection target
                if( ratio < padToFootprintMinRatio )
                    rejected.insert( pad->GetParent() );
            }
        }
    }

    int moduleCount = aCollector.CountType( PCB_MODULE_T );

    if( moduleCount > 0 )
    {
        double maxArea = calcMaxArea( aCollector, PCB_MODULE_T );
        BOX2D viewportD = getView()->GetViewport();
        BOX2I viewport( VECTOR2I( viewportD.GetPosition() ), VECTOR2I( viewportD.GetSize() ) );
        double maxCoverRatio = footprintMaxCoverRatio;

        // MODULE::CoverageRatio() doesn't take zone handles & borders into account so just
        // use a more aggressive cutoff point if zones are involved.
        if(  aCollector.CountType( PCB_ZONE_AREA_T ) )
            maxCoverRatio /= 2;

        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( MODULE* mod = dyn_cast<MODULE*>( aCollector[i] ) )
            {
                // filter out components larger than the viewport
                if( mod->ViewBBox().Contains( viewport ) )
                    rejected.insert( mod );
                // footprints completely covered with other features have no other
                // means of selection, so must be kept
                else if( mod->CoverageRatio( aCollector ) > maxCoverRatio )
                    rejected.erase( mod );
                // if a footprint is much smaller than the largest overlapping
                // footprint then it should be considered for selection
                else if( calcRatio( calcArea( mod ), maxArea ) <= footprintToFootprintMinRatio )
                    continue;
                // if there are multiple footprints for selection at this point, prefer
                // one that is on the active layer
                else if( moduleCount > 1 && mod->GetLayer() == activeLayer )
                    preferred.insert( mod );
                // reject ALL OTHER footprints if there's still something else left
                // to select
                else if( (int)( rejected.size() + 1 ) < aCollector.GetCount() )
                    rejected.insert( mod );
            }
        }
    }

    if( aCollector.CountType( PCB_VIA_T ) > 0 )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( VIA* via = dyn_cast<VIA*>( aCollector[i] ) )
            {
                double viaArea = calcArea( via );

                for( int j = 0; j < aCollector.GetCount(); ++j )
                {
                    if( i == j )
                        continue;

                    BOARD_ITEM* item = aCollector[j];
                    double areaRatio = calcRatio( viaArea, calcArea( item ) );

                    if( item->Type() == PCB_MODULE_T && areaRatio < padToFootprintMinRatio )
                        rejected.insert( item );

                    if( item->Type() == PCB_PAD_T && areaRatio < viaToPadMinRatio )
                        rejected.insert( item );

                    if( TRACK* track = dyn_cast<TRACK*>( item ) )
                    {
                        if( track->GetNetCode() != via->GetNetCode() )
                            continue;

                        double lenRatio = (double) ( track->GetLength() + track->GetWidth() ) /
                                          (double) via->GetWidth();

                        if( lenRatio > trackViaLengthRatio )
                            rejected.insert( track );
                    }
                }
            }
        }
    }

    int nTracks = aCollector.CountType( PCB_TRACE_T );

    if( nTracks > 0 )
    {
        double maxLength = 0.0;
        double minLength = std::numeric_limits<double>::max();
        double maxArea = 0.0;
        const TRACK* maxTrack = nullptr;

        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( TRACK* track = dyn_cast<TRACK*>( aCollector[i] ) )
            {
                maxLength = std::max( track->GetLength(), maxLength );
                maxLength = std::max( (double) track->GetWidth(), maxLength );

                minLength = std::min( std::max( track->GetLength(), (double) track->GetWidth() ), minLength );

                double area = track->GetLength() * track->GetWidth();

                if( area > maxArea )
                {
                    maxArea = area;
                    maxTrack = track;
                }
            }
        }

        if( maxLength > 0.0 && minLength / maxLength < trackTrackLengthRatio && nTracks > 1 )
        {
            for( int i = 0; i < aCollector.GetCount(); ++i )
             {
                if( TRACK* track = dyn_cast<TRACK*>( aCollector[i] ) )
                {
                    double ratio = std::max( (double) track->GetWidth(), track->GetLength() ) / maxLength;

                    if( ratio > trackTrackLengthRatio )
                        rejected.insert( track );
                }
            }
        }

        for( int j = 0; j < aCollector.GetCount(); ++j )
        {
            if( MODULE* mod = dyn_cast<MODULE*>( aCollector[j] ) )
            {
                double ratio = calcRatio( maxArea, mod->GetFootprintRect().GetArea() );

                if( ratio < padToFootprintMinRatio && calcCommonArea( maxTrack, mod ) < commonAreaRatio )
                    rejected.insert( mod );
            }
        }
    }

    if( (unsigned) aCollector.GetCount() > rejected.size() )  // do not remove everything
    {
        for( BOARD_ITEM* item : rejected )
        {
            aCollector.Remove( item );
        }
    }
}


int SELECTION_TOOL::updateSelection( const TOOL_EVENT& aEvent )
{
    getView()->Update( &m_selection );

    return 0;
}


int SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( m_selection );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


void SELECTION_TOOL::setTransitions()
{
    Go( &SELECTION_TOOL::UpdateMenu,          ACTIONS::updateMenu.MakeEvent() );

    Go( &SELECTION_TOOL::Main,                PCB_ACTIONS::selectionActivate.MakeEvent() );
    Go( &SELECTION_TOOL::CursorSelection,     PCB_ACTIONS::selectionCursor.MakeEvent() );
    Go( &SELECTION_TOOL::ClearSelection,      PCB_ACTIONS::selectionClear.MakeEvent() );

    Go( &SELECTION_TOOL::SelectItem,          PCB_ACTIONS::selectItem.MakeEvent() );
    Go( &SELECTION_TOOL::SelectItems,         PCB_ACTIONS::selectItems.MakeEvent() );
    Go( &SELECTION_TOOL::UnselectItem,        PCB_ACTIONS::unselectItem.MakeEvent() );
    Go( &SELECTION_TOOL::UnselectItems,       PCB_ACTIONS::unselectItems.MakeEvent() );
    Go( &SELECTION_TOOL::SelectionMenu,       PCB_ACTIONS::selectionMenu.MakeEvent() );

    Go( &SELECTION_TOOL::find,                ACTIONS::find.MakeEvent() );

    Go( &SELECTION_TOOL::filterSelection,     PCB_ACTIONS::filterSelection.MakeEvent() );
    Go( &SELECTION_TOOL::selectConnection,    PCB_ACTIONS::selectConnection.MakeEvent() );
    Go( &SELECTION_TOOL::expandConnection,    PCB_ACTIONS::expandSelectedConnection.MakeEvent() );
    Go( &SELECTION_TOOL::selectCopper,        PCB_ACTIONS::selectCopper.MakeEvent() );
    Go( &SELECTION_TOOL::selectNet,           PCB_ACTIONS::selectNet.MakeEvent() );
    Go( &SELECTION_TOOL::selectSameSheet,     PCB_ACTIONS::selectSameSheet.MakeEvent() );
    Go( &SELECTION_TOOL::selectSheetContents, PCB_ACTIONS::selectOnSheetFromEeschema.MakeEvent() );
    Go( &SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsModified );
}


