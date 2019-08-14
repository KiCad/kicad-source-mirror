/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
#include <tool/picker_tool.h>
#include <tools/pl_selection_tool.h>
#include <tools/pl_actions.h>
#include <tools/pl_edit_tool.h>
#include <ws_data_model.h>
#include <ws_draw_item.h>
#include <bitmaps.h>
#include <confirm.h>
#include <base_struct.h>
#include <pl_editor_frame.h>
#include <pl_editor_id.h>


PL_EDIT_TOOL::PL_EDIT_TOOL() :
        TOOL_INTERACTIVE( "plEditor.InteractiveEdit" ),
        m_frame( nullptr ),
        m_selectionTool( nullptr )
{
}


bool PL_EDIT_TOOL::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "plEditor.InteractiveSelection tool is not available" );

    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive,     SELECTION_CONDITIONS::ShowAlways, 1 );

    ctxMenu.AddSeparator( 200 );
    ctxMenu.AddItem( ACTIONS::doDelete,              SELECTION_CONDITIONS::NotEmpty, 200 );

    // Finally, add the standard zoom/grid items
    m_frame->AddStandardSubMenus( m_menu );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( ACTIONS::cut,               SELECTION_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( ACTIONS::copy,              SELECTION_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( ACTIONS::paste,             SELECTION_CONDITIONS::ShowAlways, 200 );
    selToolMenu.AddItem( PL_ACTIONS::move,           SELECTION_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( ACTIONS::doDelete,          SELECTION_CONDITIONS::NotEmpty, 200 );

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

    controls->SetSnapping( true );
    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    PL_SELECTION& selection = m_selectionTool->RequestSelection();
    bool          unselect = selection.IsHover();

    if( selection.Empty() )
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool        restore_state = false;
    bool        chain_commands = false;
    TOOL_EVENT* evt = const_cast<TOOL_EVENT*>( &aEvent );
    VECTOR2I    prevPos;

    if( !selection.Front()->IsNew() )
        m_frame->SaveCopyInUndoList();

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

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
                for( EDA_ITEM* item : selection )
                    moveItem( item, m_moveOffset );

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
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

            for( EDA_ITEM* item : selection )
                moveItem( item, delta );

            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_moveInProgress )
                restore_state = true;

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
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &ACTIONS::doDelete ) )
            {
                // Exit on a remove operation; there is no further processing for removed items.
                break;
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }
        else
            evt->SetPassEvent();

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    selection.ClearReferencePoint();

    for( auto item : selection )
        item->ClearEditFlags();

    if( unselect )
        m_toolMgr->RunAction( PL_ACTIONS::clearSelection, true );

    if( restore_state )
        m_frame->RollbackFromUndo();
    else
        m_frame->OnModify();

    m_moveInProgress = false;
    m_frame->PopTool( tool );
    return 0;
}


void PL_EDIT_TOOL::moveItem( EDA_ITEM* aItem, VECTOR2I aDelta )
{
    WS_DRAW_ITEM_BASE*  drawItem = static_cast<WS_DRAW_ITEM_BASE*>( aItem );
    WS_DATA_ITEM* dataItem = drawItem->GetPeer();

    dataItem->MoveToUi( dataItem->GetStartPosUi() + (wxPoint) aDelta );

    for( WS_DRAW_ITEM_BASE* item : dataItem->GetDrawItems() )
    {
        getView()->Update( item );
        item->SetFlags( IS_MOVED );
    }
}


bool PL_EDIT_TOOL::updateModificationPoint( PL_SELECTION& aSelection )
{
    if( m_moveInProgress && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        WS_DRAW_ITEM_BASE* item =  static_cast<WS_DRAW_ITEM_BASE*>( aSelection.Front() );
        aSelection.SetReferencePoint( item->GetPosition() );
    }
    // ...otherwise modify items with regard to the grid-snapped cursor position
    else
    {
        m_cursor = getViewControls()->GetCursorPosition( true );
        aSelection.SetReferencePoint( m_cursor );
    }

    return true;
}


int PL_EDIT_TOOL::ImportWorksheetContent( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );

    wxCommandEvent evt( wxEVT_NULL, ID_APPEND_DESCR_FILE );
    m_frame->Files_io( evt );

    return 0;
}


int PL_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    PL_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Size() == 0 )
        return 0;

    m_frame->SaveCopyInUndoList();

    while( selection.Front() )
    {
        WS_DRAW_ITEM_BASE*  drawItem = static_cast<WS_DRAW_ITEM_BASE*>( selection.Front() );
        WS_DATA_ITEM* dataItem = drawItem->GetPeer();
        WS_DATA_MODEL::GetTheInstance().Remove( dataItem );

        for( WS_DRAW_ITEM_BASE* item : dataItem->GetDrawItems() )
        {
            // Note: repeat items won't be selected but must be removed & deleted

            if( item->IsSelected() )
                m_selectionTool->RemoveItemFromSel( item );

            getView()->Remove( item );
            delete item;
        }

        delete dataItem;
    }

    m_frame->OnModify();

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int PL_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    std::string  tool = aEvent.GetCommandStr().get();
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( wxStockCursor( wxCURSOR_BULLSEYE ) );
    m_pickerItem = nullptr;

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition ) -> bool
        {
            if( m_pickerItem )
            {
                PL_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();
                selectionTool->UnbrightenItem( m_pickerItem );
                selectionTool->AddItemToSel( m_pickerItem, true );
                m_toolMgr->RunAction( ACTIONS::doDelete, true );
                m_pickerItem = nullptr;
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            int threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            EDA_ITEM* item = nullptr;

            for( WS_DATA_ITEM* dataItem : WS_DATA_MODEL::GetTheInstance().GetItems() )
            {
                for( WS_DRAW_ITEM_BASE* drawItem : dataItem->GetDrawItems() )
                {
                    if( drawItem->HitTest( (wxPoint) aPos, threshold ) )
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
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

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
    std::vector<WS_DATA_ITEM*> items;
    WS_DATA_MODEL&             model = WS_DATA_MODEL::GetTheInstance();
    wxString                   sexpr;

    if( selection.GetSize() == 0 )
        return 0;

    for( EDA_ITEM* item : selection.GetItems() )
        items.push_back( static_cast<WS_DRAW_ITEM_BASE*>( item )->GetPeer() );

    try
    {
        model.SaveInString( items, sexpr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxMessageBox( ioe.What(), _( "Error writing objects to clipboard" ) );
    }

    if( m_toolMgr->SaveClipboard( TO_UTF8( sexpr ) ) )
        return 0;
    else
        return -1;
}


int PL_EDIT_TOOL::Paste( const TOOL_EVENT& aEvent )
{
    PL_SELECTION&  selection = m_selectionTool->GetSelection();
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    std::string    sexpr = m_toolMgr->GetClipboard();

    m_selectionTool->ClearSelection();

    model.SetPageLayout( sexpr.c_str(), true, wxT( "clipboard" ) );

    // Build out draw items and select the first of each data item
    for( WS_DATA_ITEM* dataItem : WS_DATA_MODEL::GetTheInstance().GetItems() )
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
        WS_DRAW_ITEM_BASE* item = (WS_DRAW_ITEM_BASE*) selection.GetTopLeftItem();

        selection.SetReferencePoint( item->GetPosition() );
        m_toolMgr->RunAction( PL_ACTIONS::move, false );
    }

    return 0;
}


void PL_EDIT_TOOL::setTransitions()
{
    Go( &PL_EDIT_TOOL::Main,                   PL_ACTIONS::move.MakeEvent() );

    Go( &PL_EDIT_TOOL::ImportWorksheetContent, PL_ACTIONS::appendImportedWorksheet.MakeEvent() );

    Go( &PL_EDIT_TOOL::Undo,                   ACTIONS::undo.MakeEvent() );
    Go( &PL_EDIT_TOOL::Redo,                   ACTIONS::redo.MakeEvent() );

    Go( &PL_EDIT_TOOL::Cut,                    ACTIONS::cut.MakeEvent() );
    Go( &PL_EDIT_TOOL::Copy,                   ACTIONS::copy.MakeEvent() );
    Go( &PL_EDIT_TOOL::Paste,                  ACTIONS::paste.MakeEvent() );
    Go( &PL_EDIT_TOOL::DoDelete,               ACTIONS::doDelete.MakeEvent() );

    Go( &PL_EDIT_TOOL::DeleteItemCursor,       ACTIONS::deleteTool.MakeEvent() );
}
