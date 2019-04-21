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


#include <sch_actions.h>
#include <core/typeinfo.h>
#include <sch_selection_tool.h>
#include <sch_base_frame.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <sch_actions.h>
#include <sch_collectors.h>
#include <painter.h>
#include <eeschema_id.h>

// Selection tool actions
TOOL_ACTION SCH_ACTIONS::selectionActivate( "eeschema.InteractiveSelection",
        AS_GLOBAL, 0, "", "", NULL, AF_ACTIVATE );      // No description, not shown anywhere

TOOL_ACTION SCH_ACTIONS::selectionCursor( "eeschema.InteractiveSelection.Cursor",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::selectionMenu( "eeschema.InteractiveSelection.SelectionMenu",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::selectItem( "eeschema.InteractiveSelection.SelectItem",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::selectItems( "eeschema.InteractiveSelection.SelectItems",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::unselectItem( "eeschema.InteractiveSelection.UnselectItem",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::unselectItems( "eeschema.InteractiveSelection.UnselectItems",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION SCH_ACTIONS::selectionClear( "eeschema.InteractiveSelection.Clear",
        AS_GLOBAL, 0, "", "" );    // No description, it is not supposed to be shown anywhere

    
SCH_SELECTION_TOOL::SCH_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveSelection" ),
        m_frame( NULL ),
        m_additive( false ),
        m_subtractive( false ),
        m_multiple( false ),
        m_skip_heuristics( false ),
        m_locked( true ),
        m_menu( *this )
{
}


SCH_SELECTION_TOOL::~SCH_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


bool SCH_SELECTION_TOOL::Init()
{
    auto frame = getEditFrame<SCH_BASE_FRAME>();

    if( frame )
        m_menu.AddStandardSubMenus( *frame );

    return true;
}


void SCH_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_locked = true;

    if( aReason == TOOL_BASE::MODEL_RELOAD )
    {
        // Remove pointers to the selected items from containers without changing their
        // properties (as they are already deleted while a new sheet is loaded)
        m_selection.Clear();
        getView()->GetPainter()->GetSettings()->SetHighlight( false );
    }
    else
        // Restore previous properties of selected items and remove them from containers
        clearSelection();

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );
}


int SCH_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Should selected items be added to the current selection or
        // become the new selection (discarding previously selected items)
        m_additive = evt->Modifier( MD_SHIFT );

        // Should selected items be REMOVED from the current selection?
        // This will be ignored if the SHIFT modifier is pressed
        m_subtractive = !m_additive && evt->Modifier( MD_CTRL );

        // Is the user requesting that the selection list include all possible
        // items without removing less likely selection candidates
        m_skip_heuristics = !!evt->Modifier( MD_ALT );

        // Single click? Select single object
        if( evt->IsClick( BUT_LEFT ) )
        {
            if( evt->Modifier( MD_CTRL ) && dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::highlightNet, true );
            }
            else
            {
                // If no modifier keys are pressed, clear the selection
                if( !m_additive )
                    clearSelection();

                SelectPoint( evt->Position());
            }
        }

        // right click? if there is any object - show the context menu
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                SelectPoint( evt->Position(), SCH_COLLECTOR::AllItems, &selectionCancelled );
                m_selection.SetIsHover( true );
            }

            if( !selectionCancelled )
                m_menu.ShowContextMenu( m_selection );
        }

        // double click? Display the properties window
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            if( m_selection.Empty() )
                SelectPoint( evt->Position());

            m_toolMgr->RunAction( SCH_ACTIONS::properties );
        }

        // drag with LMB? Select multiple objects (or at least draw a selection box) or drag them
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            if( m_additive || m_subtractive || m_selection.Empty() )
            {
                // JEY TODO: move block selection to SCH_SELECTION_TOOL
                //selectMultiple();
            }

            else
            {
                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    m_toolMgr->InvokeTool( "eeschema.InteractiveEdit" );
                }
                else
                {
                    // No -> clear the selection list
                    clearSelection();
                }
            }
        }

        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();

            if( evt->IsCancel() && dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                m_toolMgr->RunAction( SCH_ACTIONS::clearHighlight, true );
        }

        else if( evt->Action() == TA_CONTEXT_MENU_CLOSED )
        {
            m_menu.CloseContextMenu( evt );
        }
    }

    // This tool is supposed to be active forever
    assert( false );

    return 0;
}


SELECTION& SCH_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


SCH_ITEM* SCH_SELECTION_TOOL::SelectPoint( const VECTOR2I& aWhere, const KICAD_T* aFilterList,
                                           bool* aSelectionCancelledFlag, bool aCheckLocked )
{
    SCH_COLLECTOR collector;

    collector.Collect( m_frame->GetScreen()->GetDrawItems(), aFilterList, (wxPoint) aWhere );

    bool anyCollected = collector.GetCount() != 0;

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !selectable( collector[ i ] ) )
            collector.Remove( i );

        if( aCheckLocked && collector[ i ]->IsLocked() )
            collector.Remove( i );
    }

    m_selection.ClearReferencePoint();

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
    {
        guessSelectionCandidates( collector, aWhere );
    }

    // If still more than one item we're going to have to ask the user.
    if( collector.GetCount() > 1 )
    {
        if( !doSelectionMenu( &collector, _( "Clarify Selection" ) ) )
        {
            if( aSelectionCancelledFlag )
                *aSelectionCancelledFlag = true;

            m_frame->GetScreen()->SetCurItem( nullptr );
            return nullptr;
        }
    }

    if( collector.GetCount() == 1 )
    {
        SCH_ITEM* item = collector[ 0 ];

        toggleSelection( item );

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );

        return item;
    }

    if( !m_additive && anyCollected )
        clearSelection();

    m_frame->ClearMsgPanel();

    return nullptr;
}


void SCH_SELECTION_TOOL::guessSelectionCandidates( SCH_COLLECTOR& collector,
                                                   const VECTOR2I& aWhere )
{
    // There are certain parent/child and enclosure combinations that can be handled
    // automatically.  Since schematics are meant to be human-readable we don't have
    // all the various overlap and coverage issues that we do in Pcbnew.
    if( collector.GetCount() == 2 )
    {
        SCH_ITEM* a = collector[ 0 ];
        SCH_ITEM* b = collector[ 1 ];

        if( a->GetParent() == b )
            collector.Remove( b );
        else if( a == b->GetParent() )
            collector.Remove( a );
        else if( a->Type() == SCH_SHEET_T && b->Type() != SCH_SHEET_T )
            collector.Remove( a );
        else if( b->Type() == SCH_SHEET_T && a->Type() != SCH_SHEET_T )
            collector.Remove( b );
    }
}


bool SCH_SELECTION_TOOL::selectCursor( const KICAD_T aFilterList[], bool aForceSelect  )
{
    if( aForceSelect || m_selection.Empty() )
    {
        VECTOR2D cursorPos = getViewControls()->GetCursorPosition( false );

        clearSelection();
        SelectPoint( cursorPos, aFilterList );
    }

    return !m_selection.Empty();
}


int SCH_SELECTION_TOOL::SelectItems( const TOOL_EVENT& aEvent )
{
    std::vector<SCH_ITEM*>* items = aEvent.Parameter<std::vector<SCH_ITEM*>*>();

    if( items )
    {
        // Perform individual selection of each item before processing the event.
        for( auto item : *items )
            select( item );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int SCH_SELECTION_TOOL::SelectItem( const TOOL_EVENT& aEvent )
{
    // Check if there is an item to be selected
    SCH_ITEM* item = aEvent.Parameter<SCH_ITEM*>();

    if( item )
    {
        select( item );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int SCH_SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    std::vector<SCH_ITEM*>* items = aEvent.Parameter<std::vector<SCH_ITEM*>*>();

    if( items )
    {
        // Perform individual unselection of each item before processing the event
        for( auto item : *items )
            unselect( item );

        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


int SCH_SELECTION_TOOL::UnselectItem( const TOOL_EVENT& aEvent )
{
    // Check if there is an item to be selected
    SCH_ITEM* item = aEvent.Parameter<SCH_ITEM*>();

    if( item )
    {
        unselect( item );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


int SCH_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    clearSelection();

    return 0;
}


int SCH_SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    SCH_COLLECTOR* collector = aEvent.Parameter<SCH_COLLECTOR*>();
    doSelectionMenu( collector, wxEmptyString );

    return 0;
}


bool SCH_SELECTION_TOOL::doSelectionMenu( SCH_COLLECTOR* aCollector, const wxString& aTitle )
{
    SCH_ITEM* current = nullptr;
    CONTEXT_MENU menu;

    int limit = std::min( MAX_SELECT_ITEM_IDS, aCollector->GetCount() );

    for( int i = 0; i < limit; ++i )
    {
        wxString text;
        SCH_ITEM* item = ( *aCollector )[i];
        text = item->GetSelectMenuText( m_frame->GetUserUnits() );

        wxString menuText = wxString::Format("&%d. %s", i + 1, text );
        menu.Add( menuText, i + 1, item->GetMenuImage() );
    }

    if( aTitle.Length() )
        menu.SetTitle( aTitle );

    menu.SetIcon( info_xpm );
    menu.DisplayTitle( true );
    SetContextMenu( &menu, CMENU_NOW );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->Action() == TA_CONTEXT_MENU_UPDATE )
        {
            if( current )
                unhighlight( current, BRIGHTENED );

            int id = *evt->GetCommandId();

            // User has pointed an item, so show it in a different way
            if( id > 0 && id <= limit )
            {
                current = ( *aCollector )[id - 1];
                highlight( current, BRIGHTENED );
            }
            else
            {
                current = NULL;
            }
        }
        else if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
        {
            if( current )
                unhighlight( current, BRIGHTENED );

            OPT<int> id = evt->GetCommandId();

            // User has selected an item, so this one will be returned
            if( id && ( *id > 0 ) )
                current = ( *aCollector )[*id - 1];
            else
                current = NULL;

            break;
        }
    }

    if( current )
    {
        unhighlight( current, BRIGHTENED );

        toggleSelection( current );

        aCollector->Empty();
        aCollector->Append( current );
        return true;
    }

    return false;
}


bool SCH_SELECTION_TOOL::selectable( const SCH_ITEM* aItem, bool checkVisibilityOnly ) const
{
    // NOTE: in the future this is where eeschema layer/itemtype visibility will be handled

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
        if( !static_cast<const SCH_PIN*>( aItem )->IsVisible() && !m_frame->GetShowAllPins() )
            return false;
        break;

    case LIB_PART_T:    // In libedit we do not want to select the symbol itself.
        return false;

    case SCH_MARKER_T:  // Always selectable
        return true;

    default:            // Suppress warnings
        break;
    }

    return true;
}


void SCH_SELECTION_TOOL::clearSelection()
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( static_cast<SCH_ITEM*>( m_selection.Front() ), SELECTED, &m_selection );

    getView()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    if( m_frame )
        m_frame->GetScreen()->SetCurItem( nullptr );

    m_locked = true;

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
}


void SCH_SELECTION_TOOL::toggleSelection( SCH_ITEM* aItem, bool aForce )
{
    if( aItem->IsSelected() )
    {
        unselect( aItem );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
    else
    {
        if( !m_additive )
            clearSelection();

        // Prevent selection of invisible or inactive items
        if( aForce || selectable( aItem ) )
        {
            select( aItem );

            // Inform other potentially interested tools
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
        }
    }

    if( m_frame )
        m_frame->GetGalCanvas()->ForceRefresh();
}


void SCH_SELECTION_TOOL::select( SCH_ITEM* aItem )
{
    if( aItem->IsSelected() )
        return;

    highlight( aItem, SELECTED, &m_selection );
    getView()->Update( &m_selection );

    if( m_frame )
    {
        if( m_selection.Size() == 1 )
        {
            // Set as the current item, so the information about selection is displayed
            m_frame->GetScreen()->SetCurItem( aItem );
        }
        else if( m_selection.Size() == 2 )  // Check only for 2, so it will not be
        {                                   // called for every next selected item
            // If multiple items are selected, do not show the information about the selected item
            m_frame->GetScreen()->SetCurItem( nullptr );
        }
    }
}


void SCH_SELECTION_TOOL::unselect( SCH_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
    getView()->Update( &m_selection );

    if( m_frame && m_frame->GetScreen()->GetCurItem() == aItem )
        m_frame->GetScreen()->SetCurItem( nullptr );

    if( m_selection.Empty() )
        m_locked = true;
}


void SCH_SELECTION_TOOL::highlight( SCH_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aGroup )
        aGroup->Add( aItem );

    // Highlight pins and fields.  (All the other component children are currently only
    // represented in the LIB_PART.)
    if( aItem->Type() == SCH_COMPONENT_T )
    {
        SCH_PINS pins = static_cast<SCH_COMPONENT*>( aItem )->GetPins();

        for( SCH_PIN& pin : pins )
        {
            if( aMode == SELECTED )
                pin.SetSelected();
            else if( aMode == BRIGHTENED )
                pin.SetBrightened();
        }

        std::vector<SCH_FIELD*> fields;
        static_cast<SCH_COMPONENT*>( aItem )->GetFields( fields, false );

        for( auto field : fields )
        {
            if( aMode == SELECTED )
                field->SetSelected();
            else if( aMode == BRIGHTENED )
                field->SetBrightened();

            // JEY TODO: do these need hiding from view and adding to aGroup?
        }
    }

    // JEY TODO: Sheets and sheet pins?

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void SCH_SELECTION_TOOL::unhighlight( SCH_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aGroup )
        aGroup->Remove( aItem );

    // Unhighlight pins and fields.  (All the other component children are currently only
    // represented in the LIB_PART.)
    if( aItem->Type() == SCH_COMPONENT_T )
    {
        SCH_PINS pins = static_cast<SCH_COMPONENT*>( aItem )->GetPins();

        for( SCH_PIN& pin : pins )
        {
            if( aMode == SELECTED )
                pin.ClearSelected();
            else if( aMode == BRIGHTENED )
                pin.ClearBrightened();
        }

        std::vector<SCH_FIELD*> fields;
        static_cast<SCH_COMPONENT*>( aItem )->GetFields( fields, false );

        for( auto field : fields )
        {
            if( aMode == SELECTED )
                field->ClearSelected();
            else if( aMode == BRIGHTENED )
                field->ClearBrightened();

            // JEY TODO: do these need showing and updating?
        }
    }

    // JEY TODO: Sheets and sheet pins?

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


bool SCH_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
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


void SCH_SELECTION_TOOL::setTransitions()
{
    Go( &SCH_SELECTION_TOOL::Main, SCH_ACTIONS::selectionActivate.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::ClearSelection, SCH_ACTIONS::selectionClear.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectItem, SCH_ACTIONS::selectItem.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectItems, SCH_ACTIONS::selectItems.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::UnselectItem, SCH_ACTIONS::unselectItem.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::UnselectItems, SCH_ACTIONS::unselectItems.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectionMenu, SCH_ACTIONS::selectionMenu.MakeEvent() );
}


