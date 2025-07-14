/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <advanced_config.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <preview_items/selection_area.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/selection.h>
#include <tools/pl_point_editor.h>
#include <tools/pl_selection_tool.h>
#include <tools/pl_actions.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_draw_item.h>
#include <collector.h>
#include <math/util.h>      // for KiROUND

#include "pl_editor_frame.h"


#define HITTEST_THRESHOLD_PIXELS 3


PL_SELECTION_TOOL::PL_SELECTION_TOOL() :
        SELECTION_TOOL( "common.InteractiveSelection" ),
        m_frame( nullptr )
{
}


bool PL_SELECTION_TOOL::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();

    auto& menu = m_menu->GetMenu();

    menu.AddSeparator( 200 );
    menu.AddItem( PL_ACTIONS::drawLine,      SELECTION_CONDITIONS::Empty, 200 );
    menu.AddItem( PL_ACTIONS::drawRectangle, SELECTION_CONDITIONS::Empty, 200 );
    menu.AddItem( PL_ACTIONS::placeText,     SELECTION_CONDITIONS::Empty, 200 );
    menu.AddItem( PL_ACTIONS::placeImage,    SELECTION_CONDITIONS::Empty, 200 );

    menu.AddSeparator( 1000 );
    m_frame->AddStandardSubMenus( *m_menu.get() );

    m_disambiguateTimer.SetOwner( this );
    Connect( m_disambiguateTimer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( PL_SELECTION_TOOL::onDisambiguationExpire ), nullptr, this );

    return true;
}


void PL_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        if( evt->IsMouseDown( BUT_LEFT ) )
        {
            // Avoid triggering when running under other tools
            PL_POINT_EDITOR *pt_tool = m_toolMgr->GetTool<PL_POINT_EDITOR>();

            if( m_frame->ToolStackIsEmpty() && pt_tool && !pt_tool->HasPoint() )
            {
                m_originalCursor = m_toolMgr->GetMousePosition();
                m_disambiguateTimer.StartOnce( ADVANCED_CFG::GetCfg().m_DisambiguationMenuDelay );
            }
        }
        // Single click? Select single object
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // If the timer has stopped, then we have already run the disambiguate routine
            // and we don't want to register an extra click here
            if( !m_disambiguateTimer.IsRunning() )
            {
                evt->SetPassEvent();
                continue;
            }

            m_disambiguateTimer.Stop();
            SelectPoint( evt->Position() );
        }

        // right click? if there is any object - show the context menu
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_disambiguateTimer.Stop();
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                SelectPoint( evt->Position(), &selectionCancelled );
                m_selection.SetIsHover( true );
            }

            // Show selection before opening menu
            m_frame->GetCanvas()->ForceRefresh();

            if( !selectionCancelled )
                m_menu->ShowContextMenu( m_selection );
        }

        // double click? Display the properties window
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            // No double-click actions currently defined
        }

        // drag with LMB? Select multiple objects (or at least draw a selection box) or drag them
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            if( hasModifier() || m_selection.Empty() )
            {
                selectMultiple();
            }
            else
            {
                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    m_toolMgr->RunAction( "plEditor.InteractiveMove.move" );
                }
                else
                {
                    // No -> clear the selection list
                    ClearSelection();
                }
            }
        }

        // Middle double click?  Do zoom to fit or zoom to objects
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
        }

        else if( evt->IsCancelInteractive() )
        {
            m_disambiguateTimer.Stop();
            ClearSelection();
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            ClearSelection();
        }

        else
            evt->SetPassEvent();


        if( m_frame->ToolStackIsEmpty() )
        {
            if( !hasModifier()
                    && !m_selection.Empty()
                    && m_frame->GetDragAction() == MOUSE_DRAG_ACTION::DRAG_SELECTED
                    && evt->HasPosition()
                    && selectionContains( evt->Position() ) )
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
            }
            else
            {
                if( m_additive )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ADD );
                else if( m_subtractive )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SUBTRACT );
                else if( m_exclusive_or )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::XOR );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            }
        }
    }

    return 0;
}


int PL_SELECTION_TOOL::disambiguateCursor( const TOOL_EVENT& aEvent )
{
    wxMouseState keyboardState = wxGetMouseState();

    setModifiersState( keyboardState.ShiftDown(), keyboardState.ControlDown(),
                       keyboardState.AltDown() );

    m_skip_heuristics = true;
    SelectPoint( m_originalCursor, &m_canceledMenu );
    m_skip_heuristics = false;

    return 0;
}


PL_SELECTION& PL_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


void PL_SELECTION_TOOL::SelectPoint( const VECTOR2I& aWhere, bool* aSelectionCancelledFlag )
{
    int threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );

    // locate items.
    COLLECTOR collector;

    for( DS_DATA_ITEM* dataItem : DS_DATA_MODEL::GetTheInstance().GetItems() )
    {
        for( DS_DRAW_ITEM_BASE* drawItem : dataItem->GetDrawItems() )
        {
            if( drawItem->HitTest( aWhere, threshold ) )
                collector.Append( drawItem );
        }
    }

    m_selection.ClearReferencePoint();

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
        guessSelectionCandidates( collector, aWhere );

    // If still more than one item we're going to have to ask the user.
    if( collector.GetCount() > 1 )
    {
        doSelectionMenu( &collector );

        if( collector.m_MenuCancelled )
        {
            if( aSelectionCancelledFlag )
                *aSelectionCancelledFlag = true;

            return;
        }
    }

    bool anyAdded      = false;
    bool anySubtracted = false;


    if( !m_additive && !m_subtractive && !m_exclusive_or )
    {
        if( collector.GetCount() == 0 )
            anySubtracted = true;

        ClearSelection();
    }

    if( collector.GetCount() > 0 )
    {
        for( int i = 0; i < collector.GetCount(); ++i )
        {
            if( m_subtractive || ( m_exclusive_or && collector[i]->IsSelected() ) )
            {
                unselect( collector[i] );
                anySubtracted = true;
            }
            else
            {
                select( collector[i] );
                anyAdded = true;
            }
        }
    }

    if( anyAdded )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    if( anySubtracted )
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
}


void PL_SELECTION_TOOL::guessSelectionCandidates( COLLECTOR& collector, const VECTOR2I& aPos )
{
    // There are certain conditions that can be handled automatically.

    // Prefer an exact hit to a sloppy one
    for( int i = 0; collector.GetCount() == 2 && i < 2; ++i )
    {
        EDA_ITEM* item = collector[ i ];
        EDA_ITEM* other = collector[ ( i + 1 ) % 2 ];

        if( item->HitTest( aPos, 0 ) && !other->HitTest( aPos, 0 ) )
            collector.Transfer( other );
    }
}


PL_SELECTION& PL_SELECTION_TOOL::RequestSelection()
{
    // If nothing is selected do a hover selection
    if( m_selection.Empty() )
    {
        VECTOR2D cursorPos = getViewControls()->GetCursorPosition( true );

        ClearSelection();
        SelectPoint( cursorPos );
        m_selection.SetIsHover( true );
    }

    return m_selection;
}


bool PL_SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool cancelled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( TOOL_EVENT* evt = Wait() )
    {
        /* Selection mode depends on direction of drag-selection:
         * Left > Right : Select objects that are fully enclosed by selection
         * Right > Left : Select objects that are crossed by selection
         */
        bool windowSelection = area.GetEnd().x > area.GetOrigin().x;

        m_frame->GetCanvas()->SetCurrentCursor( windowSelection ? KICURSOR::SELECT_WINDOW
                                                                : KICURSOR::SELECT_LASSO );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_drag_additive && !m_drag_subtractive )
                ClearSelection();

            // Start drawing a selection box
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            area.SetAdditive( m_drag_additive );
            area.SetSubtractive( m_drag_subtractive );
            area.SetExclusiveOr( false );
            area.SetMode( windowSelection ? SELECTION_MODE::INSIDE_RECTANGLE
                                          : SELECTION_MODE::TOUCHING_RECTANGLE );

            view->SetVisible( &area, true );
            view->Update( &area );
            getViewControls()->SetAutoPan( true );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( false );

            // End drawing the selection box
            view->SetVisible( &area, false );

            bool anyAdded = false;
            bool anySubtracted = false;

            // Construct a BOX2I to determine EDA_ITEM selection
            BOX2I selectionRect( area.ViewBBox() );

            selectionRect.Normalize();

            for( DS_DATA_ITEM* dataItem : DS_DATA_MODEL::GetTheInstance().GetItems() )
            {
                for( DS_DRAW_ITEM_BASE* item : dataItem->GetDrawItems() )
                {
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
            }

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


int PL_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    ClearSelection();
    return 0;
}


void PL_SELECTION_TOOL::RebuildSelection()
{
    m_selection.Clear();

    for( DS_DATA_ITEM* dataItem : DS_DATA_MODEL::GetTheInstance().GetItems() )
    {
        for( DS_DRAW_ITEM_BASE* item : dataItem->GetDrawItems() )
        {
            if( item->IsSelected() )
                select( item );
        }
    }
}


void PL_SELECTION_TOOL::ClearSelection()
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( m_selection.Front(), SELECTED, &m_selection );

    getView()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
}


void PL_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    highlight( aItem, SELECTED, &m_selection );
}


void PL_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}


void PL_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aGroup )
        aGroup->Add( aItem );

    getView()->Update( aItem );
}


void PL_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aGroup )
        aGroup->Remove( aItem );

    getView()->Update( aItem );
}


bool PL_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    VECTOR2I margin = getView()->ToWorld( VECTOR2I( GRIP_MARGIN, GRIP_MARGIN ), false );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( EDA_ITEM* item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin.x, margin.y );    // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


void PL_SELECTION_TOOL::setTransitions()
{
    Go( &PL_SELECTION_TOOL::UpdateMenu,            ACTIONS::updateMenu.MakeEvent() );

    Go( &PL_SELECTION_TOOL::Main,                  ACTIONS::selectionActivate.MakeEvent() );
    Go( &PL_SELECTION_TOOL::ClearSelection,        ACTIONS::selectionClear.MakeEvent() );

    Go( &PL_SELECTION_TOOL::AddItemToSel,          ACTIONS::selectItem.MakeEvent() );
    Go( &PL_SELECTION_TOOL::AddItemsToSel,         ACTIONS::selectItems.MakeEvent() );
    Go( &PL_SELECTION_TOOL::RemoveItemFromSel,     ACTIONS::unselectItem.MakeEvent() );
    Go( &PL_SELECTION_TOOL::RemoveItemsFromSel,    ACTIONS::unselectItems.MakeEvent() );
    Go( &PL_SELECTION_TOOL::SelectionMenu,         ACTIONS::selectionMenu.MakeEvent() );

    Go( &PL_SELECTION_TOOL::disambiguateCursor,    EVENTS::DisambiguatePoint );
}
