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

#include <wx/log.h>
#include <wx/msgdlg.h>
#include <fmt/format.h>

#include <tool/tool_manager.h>
#include <tool/picker_tool.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_draw_item.h>
#include <bitmaps.h>
#include <clipboard.h>
#include <confirm.h>
#include <eda_item.h>
#include <macros.h>
#include <string_utils.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <math/util.h>      // for KiROUND

#include "tools/pl_selection_tool.h"
#include "tools/pl_actions.h"
#include "tools/pl_edit_tool.h"
#include "pl_draw_panel_gal.h"
#include "pl_editor_frame.h"
#include "pl_editor_id.h"

PL_EDIT_TOOL::PL_EDIT_TOOL() :
        TOOL_INTERACTIVE( "plEditor.InteractiveEdit" ),
        m_frame( nullptr ),
        m_selectionTool( nullptr ),
        m_moveInProgress( false ),
        m_moveOffset( 0, 0 ),
        m_cursor( 0, 0 ),
        m_pickerItem( nullptr )
{
}


bool PL_EDIT_TOOL::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "plEditor.InteractiveSelection tool is not available" );

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive,     SELECTION_CONDITIONS::ShowAlways, 1 );

    ctxMenu.AddSeparator( 200 );
    ctxMenu.AddItem( ACTIONS::doDelete,              SELECTION_CONDITIONS::NotEmpty, 200 );

    // Finally, add the standard zoom/grid items
    m_frame->AddStandardSubMenus( *m_menu.get() );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( PL_ACTIONS::move,           SELECTION_CONDITIONS::NotEmpty, 250 );

    selToolMenu.AddSeparator( 250 );
    selToolMenu.AddItem( ACTIONS::cut,               SELECTION_CONDITIONS::NotEmpty, 250 );
    selToolMenu.AddItem( ACTIONS::copy,              SELECTION_CONDITIONS::NotEmpty, 250 );
    selToolMenu.AddItem( ACTIONS::paste,             SELECTION_CONDITIONS::ShowAlways, 250 );
    selToolMenu.AddItem( ACTIONS::doDelete,          SELECTION_CONDITIONS::NotEmpty, 250 );

    return true;
}


void PL_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    PL_SELECTION& selection = m_selectionTool->RequestSelection();
    bool          unselect = selection.IsHover();

    if( selection.Empty() || m_moveInProgress )
        return 0;

    std::set<DS_DATA_ITEM*> unique_peers;

    for( EDA_ITEM* item : selection )
    {
        DS_DRAW_ITEM_BASE* drawItem = static_cast<DS_DRAW_ITEM_BASE*>( item );
        unique_peers.insert( drawItem->GetPeer() );
    }

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool        restore_state = false;
    bool        chain_commands = false;
    TOOL_EVENT  copy = aEvent;
    TOOL_EVENT* evt = &copy;
    VECTOR2I    prevPos;

    if( !selection.Front()->IsNew() )
    {
        try
        {
            m_frame->SaveCopyInUndoList();
        }
        catch( const fmt::format_error& exc )
        {
            wxLogWarning( wxS( "Exception \"%s\" serializing string ocurred." ),
                          exc.what() );
            return 1;
        }
    }

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );

        if( evt->IsAction( &PL_ACTIONS::move ) || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
            || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            //------------------------------------------------------------------------
            // Start a move operation
            //
            if( !m_moveInProgress )
            {
                // Apply any initial offset in case we're coming from a previous command.
                //
                for( DS_DATA_ITEM* item : unique_peers )
                    moveItem( item, m_moveOffset );

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( DS_DATA_ITEM* item : unique_peers )
                        moveItem( item, delta );

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin,
                    // so the movement vector can be computed later
                    updateModificationPoint( selection );
                    m_cursor = originalCursorPos;
                }
                else
                {
                    updateModificationPoint( selection );
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = controls->GetCursorPosition();
            VECTOR2I delta( m_cursor - prevPos );
            selection.SetReferencePoint( m_cursor );

            m_moveOffset += delta;
            prevPos = m_cursor;

            for( DS_DATA_ITEM* item : unique_peers )
                moveItem( item, delta );

            m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( evt->IsCancelInteractive() )
                m_frame->GetInfoBar()->Dismiss();

            if( m_moveInProgress )
            {
                if( evt->IsActivate() )
                {
                    // Allowing other tools to activate during a move runs the risk of race
                    // conditions in which we try to spool up both event loops at once.

                    m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel move." ) );

                    evt->SetPassEvent( false );
                    continue;
                }

                evt->SetPassEvent( false );
                restore_state = true;
            }

            break;
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }
        else if( evt->IsAction( &ACTIONS::doDelete ) )
        {
            evt->SetPassEvent();
            // Exit on a delete; there will no longer be anything to drag.
            break;
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) )
        {
            if( selection.Front()->IsNew() )
            {
                // This doesn't really make sense; we'll just end up dragging a stack of
                // objects so we ignore the duplicate and just carry on.
                continue;
            }

            // Move original back and exit.  The duplicate will run in its own loop.
            restore_state = true;
            unselect = false;
            chain_commands = true;
            break;
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( m_moveInProgress );

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    selection.ClearReferencePoint();

    for( EDA_ITEM* item : selection )
        item->ClearEditFlags();

    if( restore_state )
        m_frame->RollbackFromUndo();
    else
        m_frame->OnModify();

    if( unselect )
        m_toolMgr->RunAction( ACTIONS::selectionClear );
    else
        m_toolMgr->PostEvent( EVENTS::SelectedEvent );

    m_moveInProgress = false;
    m_frame->PopTool( aEvent );
    return 0;
}


void PL_EDIT_TOOL::moveItem( DS_DATA_ITEM* aItem, const VECTOR2I& aDelta )
{
    aItem->MoveToIU( aItem->GetStartPosIU() + aDelta );

    for( DS_DRAW_ITEM_BASE* item : aItem->GetDrawItems() )
    {
        getView()->Update( item );
        item->SetFlags( IS_MOVING );
    }
}


bool PL_EDIT_TOOL::updateModificationPoint( PL_SELECTION& aSelection )
{
    if( m_moveInProgress && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        aSelection.SetReferencePoint( aSelection.Front()->GetPosition() );
    }
    // ...otherwise modify items with regard to the grid-snapped cursor position
    else
    {
        m_cursor = getViewControls()->GetCursorPosition( true );
        aSelection.SetReferencePoint( m_cursor );
    }

    return true;
}


int PL_EDIT_TOOL::ImportDrawingSheetContent( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( ACTIONS::cancelInteractive );

    wxCommandEvent evt( wxEVT_NULL, ID_APPEND_DESCR_FILE );
    m_frame->Files_io( evt );

    return 0;
}


int PL_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    PL_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Size() == 0 )
        return 0;


    // Do not delete an item if it is currently a new item being created to avoid a crash
    // In this case the selection contains only one item.
    DS_DRAW_ITEM_BASE* currItem = static_cast<DS_DRAW_ITEM_BASE*>( selection.Front() );

    if( currItem->GetFlags() & ( IS_NEW ) )
        return 0;

    m_frame->SaveCopyInUndoList();

    while( selection.Front() )
    {
        DS_DRAW_ITEM_BASE* drawItem = static_cast<DS_DRAW_ITEM_BASE*>( selection.Front() );
        DS_DATA_ITEM*      dataItem = drawItem->GetPeer();
        DS_DATA_MODEL::GetTheInstance().Remove( dataItem );

        for( DS_DRAW_ITEM_BASE* item : dataItem->GetDrawItems() )
        {
            // Note: repeat items won't be selected but must be removed & deleted

            if( item->IsSelected() )
                m_selectionTool->RemoveItemFromSel( item );

            getView()->Remove( item );
        }

        delete dataItem;
    }

    m_frame->OnModify();

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int PL_EDIT_TOOL::InteractiveDelete( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::REMOVE );
    m_pickerItem = nullptr;

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition ) -> bool
        {
            if( m_pickerItem )
            {
                PL_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();
                selectionTool->UnbrightenItem( m_pickerItem );
                selectionTool->AddItemToSel( m_pickerItem, true /*quiet mode*/ );
                m_toolMgr->RunAction( ACTIONS::doDelete );
                m_pickerItem = nullptr;
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            int threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            EDA_ITEM* item = nullptr;

            for( DS_DATA_ITEM* dataItem : DS_DATA_MODEL::GetTheInstance().GetItems() )
            {
                for( DS_DRAW_ITEM_BASE* drawItem : dataItem->GetDrawItems() )
                {
                    if( drawItem->HitTest( aPos, threshold ) )
                    {
                        item = drawItem;
                        break;
                    }
                }
            }

            if( m_pickerItem != item )
            {
                PL_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();

                if( m_pickerItem )
                    selectionTool->UnbrightenItem( m_pickerItem );

                m_pickerItem = item;

                if( m_pickerItem )
                    selectionTool->BrightenItem( m_pickerItem );
            }
        } );

    picker->SetFinalizeHandler(
        [this] ( const int& aFinalState )
        {
            if( m_pickerItem )
                m_toolMgr->GetTool<PL_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

            // Wake the selection tool after exiting to ensure the cursor gets updated
            m_toolMgr->PostAction( ACTIONS::selectionActivate );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


int PL_EDIT_TOOL::Undo( const TOOL_EVENT& aEvent )
{
    m_frame->GetLayoutFromUndoList();
    return 0;
}


int PL_EDIT_TOOL::Redo( const TOOL_EVENT& aEvent )
{
    m_frame->GetLayoutFromRedoList();
    return 0;
}


int PL_EDIT_TOOL::Cut( const TOOL_EVENT& aEvent )
{
    int retVal = Copy( aEvent );

    if( retVal == 0 )
        retVal = DoDelete( aEvent );

    return retVal;
}


int PL_EDIT_TOOL::Copy( const TOOL_EVENT& aEvent )
{
    PL_SELECTION&              selection = m_selectionTool->RequestSelection();
    std::vector<DS_DATA_ITEM*> items;
    DS_DATA_MODEL&             model = DS_DATA_MODEL::GetTheInstance();
    wxString                   sexpr;

    if( selection.GetSize() == 0 )
        return 0;

    for( EDA_ITEM* item : selection.GetItems() )
        items.push_back( static_cast<DS_DRAW_ITEM_BASE*>( item )->GetPeer() );

    try
    {
        model.SaveInString( items, &sexpr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxMessageBox( ioe.What(), _( "Error writing objects to clipboard" ) );
    }

    if( SaveClipboard( TO_UTF8( sexpr ) ) )
        return 0;
    else
        return -1;
}


int PL_EDIT_TOOL::Paste( const TOOL_EVENT& aEvent )
{
    PL_SELECTION&  selection = m_selectionTool->GetSelection();
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    if( std::unique_ptr<wxImage> clipImg = GetImageFromClipboard() )
    {
        auto image = std::make_unique<BITMAP_BASE>();
        image->SetImage( *clipImg );
        auto dataItem = std::make_unique<DS_DATA_ITEM_BITMAP>( image.release() );
        model.Append( dataItem.release() );
    }
    else
    {
        m_selectionTool->ClearSelection();

        const std::string clipText = GetClipboardUTF8();
        model.SetPageLayout( clipText.c_str(), true, wxT( "clipboard" ) );
    }

    // Build out draw items and select the first of each data item
    for( DS_DATA_ITEM* dataItem : model.GetItems() )
    {
        if( dataItem->GetDrawItems().empty() )
        {
            dataItem->SyncDrawItems( nullptr, getView() );
            dataItem->GetDrawItems().front()->SetSelected();
        }
    }

    m_selectionTool->RebuildSelection();

    if( !selection.Empty() )
    {
        selection.SetReferencePoint( selection.GetTopLeftItem()->GetPosition() );
        m_toolMgr->PostAction( PL_ACTIONS::move );
    }

    return 0;
}


void PL_EDIT_TOOL::setTransitions()
{
    Go( &PL_EDIT_TOOL::Main,                      PL_ACTIONS::move.MakeEvent() );

    Go( &PL_EDIT_TOOL::ImportDrawingSheetContent, PL_ACTIONS::appendImportedDrawingSheet.MakeEvent() );

    Go( &PL_EDIT_TOOL::Undo,                      ACTIONS::undo.MakeEvent() );
    Go( &PL_EDIT_TOOL::Redo,                      ACTIONS::redo.MakeEvent() );

    Go( &PL_EDIT_TOOL::Cut,                       ACTIONS::cut.MakeEvent() );
    Go( &PL_EDIT_TOOL::Copy,                      ACTIONS::copy.MakeEvent() );
    Go( &PL_EDIT_TOOL::Paste,                     ACTIONS::paste.MakeEvent() );
    Go( &PL_EDIT_TOOL::DoDelete,                  ACTIONS::doDelete.MakeEvent() );

    Go( &PL_EDIT_TOOL::InteractiveDelete,         ACTIONS::deleteTool.MakeEvent() );
}
