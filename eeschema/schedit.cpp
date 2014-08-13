 /*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file schedit.cpp
 */

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <wxEeschemaStruct.h>
#include <kicad_device_context.h>
#include <hotkeys_basic.h>

#include <general.h>
#include <eeschema_id.h>
#include <protos.h>
#include <class_library.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>


void SCH_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    pos = wxGetMousePosition();

    pos.y += 20;

    // If needed, stop the current command and deselect current tool
    switch( id )
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_CANCEL_CURRENT_COMMAND:
    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
    case ID_POPUP_SCH_BEGIN_WIRE:
    case ID_POPUP_SCH_BEGIN_BUS:
    case ID_POPUP_END_LINE:
    case ID_POPUP_SCH_SET_SHAPE_TEXT:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_SCH_END_SHEET:
    case ID_POPUP_SCH_RESIZE_SHEET:
    case ID_POPUP_IMPORT_GLABEL:
    case ID_POPUP_SCH_INIT_CMP:
    case ID_POPUP_SCH_DISPLAYDOC_CMP:
    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DRAG_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
    case ID_POPUP_SCH_ENTER_SHEET:
    case ID_POPUP_SCH_LEAVE_SHEET:
    case ID_POPUP_SCH_ADD_JUNCTION:
    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_GETINFO_MARKER:

        /* At this point: Do nothing. these commands do not need to stop the
         * current command (mainly a block command) or reset the current state
         * They will be executed later, in next switch structure.
         */
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:

        // Stop the current command (if any) but keep the current tool
        m_canvas->EndMouseCapture();
        break;

    default:

        // Stop the current command and deselect the current tool
        m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        break;
    }

    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    item = screen->GetCurItem();    // Can be modified by previous calls.

    switch( id )
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( &dc, pos );
        SetRepeatItem( NULL );
        break;

    case wxID_CUT:
        if( screen->m_BlockLocate.GetCommand() != BLOCK_MOVE )
            break;

        screen->m_BlockLocate.SetCommand( BLOCK_DELETE );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        SetRepeatItem( NULL );
        SetSheetNumberAndCount();
        break;

    case wxID_PASTE:
        HandleBlockBegin( &dc, BLOCK_PASTE, GetCrossHairPosition() );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
        m_canvas->MoveCursorToCrossHair();
        SetBusEntryShape( &dc, dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ), '/' );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
        m_canvas->MoveCursorToCrossHair();
        SetBusEntryShape( &dc, dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ), '\\' );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->EndMouseCapture();
            SetToolID( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString );
        }
        else
        {
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        }

        break;

    case ID_POPUP_END_LINE:
        m_canvas->MoveCursorToCrossHair();
        EndSegment( &dc );
        break;

    case ID_POPUP_SCH_BEGIN_WIRE:
        m_canvas->MoveCursorToCrossHair();
        OnLeftClick( &dc, GetCrossHairPosition() );
        break;

    case ID_POPUP_SCH_BEGIN_BUS:
        m_canvas->MoveCursorToCrossHair();
        OnLeftClick( &dc, GetCrossHairPosition() );
        break;

    case ID_POPUP_SCH_SET_SHAPE_TEXT:
        // Not used
        break;

    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
        m_canvas->MoveCursorToCrossHair();
        DeleteConnection( id == ID_POPUP_SCH_DELETE_CONNECTION );
        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );
        screen->TestDanglingEnds( m_canvas, &dc );
        m_canvas->Refresh();
        break;

    case ID_POPUP_SCH_BREAK_WIRE:
        {
            DLIST< SCH_ITEM > oldWires;

            oldWires.SetOwnership( false );      // Prevent DLIST for deleting items in destructor.
            m_canvas->MoveCursorToCrossHair();
            screen->ExtractWires( oldWires, true );
            screen->BreakSegment( GetCrossHairPosition() );

            if( oldWires.GetCount() != 0 )
            {
                PICKED_ITEMS_LIST oldItems;

                oldItems.m_Status = UR_WIRE_IMAGE;

                while( oldWires.GetCount() != 0 )
                {
                    ITEM_PICKER picker = ITEM_PICKER( oldWires.PopFront(), UR_WIRE_IMAGE );
                    oldItems.PushItem( picker );
                }

                SaveCopyInUndoList( oldItems, UR_WIRE_IMAGE );
            }

            screen->TestDanglingEnds( m_canvas, &dc );
        }
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:
        if( item == NULL )
            break;

        DeleteItem( item );
        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );
        screen->TestDanglingEnds( m_canvas, &dc );
        SetSheetNumberAndCount();
        OnModify();
        break;

    case ID_POPUP_SCH_END_SHEET:
        m_canvas->MoveCursorToCrossHair();
        addCurrentItemToList( &dc );
        break;

    case ID_POPUP_SCH_RESIZE_SHEET:
        ReSizeSheet( (SCH_SHEET*) item, &dc );
        screen->TestDanglingEnds( m_canvas, &dc );
        break;

    case ID_POPUP_IMPORT_GLABEL:
        if( item != NULL && item->Type() == SCH_SHEET_T )
            screen->SetCurItem( ImportSheetPin( (SCH_SHEET*) item, &dc ) );
        break;

    case ID_POPUP_SCH_CLEANUP_SHEET:
        if( item != NULL && item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            if( !sheet->HasUndefinedPins() )
            {
                DisplayInfoMessage( this,
                                    _( "There are no undefined labels in this sheet to clean up." ) );
                return;
            }

            if( !IsOK( this, _( "Do you wish to cleanup this sheet?" ) ) )
                return;

            /* Save sheet in undo list before cleaning up unreferenced hierarchical labels. */
            SaveCopyInUndoList( sheet, UR_CHANGED );
            sheet->CleanupSheet();
            OnModify();
            m_canvas->RefreshDrawingRect( sheet->GetBoundingBox() );
        }
        break;

    case ID_POPUP_SCH_INIT_CMP:
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_SCH_EDIT_CONVERT_CMP:

        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            m_canvas->MoveCursorToCrossHair();
            ConvertPart( (SCH_COMPONENT*) item, &dc );
        }

        break;

    case ID_POPUP_SCH_DISPLAYDOC_CMP:

        // Ensure the struct is a component (could be a piece of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            if( PART_LIBS* libs = Prj().SchLibs() )
            {
                LIB_ALIAS* entry = libs->FindLibraryEntry( ( (SCH_COMPONENT*) item )->GetPartName() );

                if( entry && !!entry->GetDocFileName() )
                {
                    SEARCH_STACK* lib_search = Prj().SchSearchS();

                    GetAssociatedDocument( this, entry->GetDocFileName(), lib_search );
                }
            }
        }
        break;

    case ID_POPUP_SCH_ENTER_SHEET:

        if( item && (item->Type() == SCH_SHEET_T) )
        {
            m_CurrentSheet->Push( (SCH_SHEET*) item );
            DisplayCurrentSheet();
        }

        break;

    case ID_POPUP_SCH_LEAVE_SHEET:
        m_CurrentSheet->Pop();
        DisplayCurrentSheet();
        break;

    case wxID_COPY:         // really this is a Save block for paste
        screen->m_BlockLocate.SetCommand( BLOCK_SAVE );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        screen->m_BlockLocate.SetCommand( BLOCK_ZOOM );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_DELETE );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        SetSheetNumberAndCount();
        break;

    case ID_POPUP_COPY_BLOCK:
        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_COPY );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DRAG_BLOCK:
        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_DRAG );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_SCH_ADD_JUNCTION:
        m_canvas->MoveCursorToCrossHair();
        screen->SetCurItem( AddJunction( &dc, GetCrossHairPosition(), true ) );
        screen->TestDanglingEnds( m_canvas, &dc );
        screen->SetCurItem( NULL );
        break;

    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_ADD_GLABEL:
        screen->SetCurItem( CreateNewText( &dc, id == ID_POPUP_SCH_ADD_LABEL ?
                                           LAYER_LOCLABEL : LAYER_GLOBLABEL ) );
        item = screen->GetCurItem();

        if( item )
            addCurrentItemToList( &dc );

        break;

    case ID_POPUP_SCH_GETINFO_MARKER:
        if( item && item->Type() == SCH_MARKER_T )
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );

        break;

    default:        // Log error:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot process command event ID %d" ),
                                      event.GetId() ) );
        break;
    }

    // End switch ( id )    (Command execution)

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        SetRepeatItem( NULL );
}


void SCH_EDIT_FRAME::OnMoveItem( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    if( screen->m_BlockLocate.GetState() != STATE_NO_BLOCK )
    {
        // trying to move an item when there is a block at the same time is not acceptable
        return;
    }

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = LocateAndShowItem( data->GetPosition(), SCH_COLLECTOR::MovableItems,
                                  aEvent.GetInt() );

        // Exit if no item found at the current location or the item is already being edited.
        if( (item == NULL) || (item->GetFlags() != 0) )
            return;
    }

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    switch( item->Type() )
    {
    case SCH_LINE_T:
        break;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
    case SCH_COMPONENT_T:
    case SCH_SHEET_PIN_T:
    case SCH_FIELD_T:
        MoveItem( item, &dc );
        break;

    case SCH_BITMAP_T:
        MoveImage( (SCH_BITMAP*) item, &dc );
        break;

    case SCH_SHEET_T:
        StartMoveSheet( (SCH_SHEET*) item, &dc );
        break;

    case SCH_MARKER_T:
    default:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot move item type %s" ),
                                      GetChars( item->GetClass() ) ) );
        break;
    }

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        SetRepeatItem( NULL );
}


void SCH_EDIT_FRAME::OnCancelCurrentCommand( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    if( screen->IsBlockActive() )
    {
        m_canvas->SetCursor( (wxStockCursor) m_canvas->GetDefaultCursor() );
        screen->ClearBlockCommand();

        // Stop the current command (if any) but keep the current tool
        m_canvas->EndMouseCapture();
    }
    else
    {
        if( m_canvas->IsMouseCaptured() ) // Stop the current command but keep the current tool
            m_canvas->EndMouseCapture();
        else                    // Deselect current tool
            m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
     }
}


void SCH_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    // Stop the current command and deselect the current tool.
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( id, m_canvas->GetDefaultCursor(), _( "No tool selected" ) );
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Descend or ascend hierarchy" ) );
        break;

    case ID_NOCONN_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add no connect" ) );
        break;

    case ID_WIRE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add wire" ) );
        break;

    case ID_BUS_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add bus" ) );
        break;

    case ID_LINE_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add lines" ) );
        break;

    case ID_JUNCTION_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add junction" ) );
        break;

    case ID_LABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add label" ) );
        break;

    case ID_GLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add global label" ) );
        break;

    case ID_HIERLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add hierarchical label" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add text" ) );
        break;

    case ID_ADD_IMAGE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add image" ) );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add wire to bus entry" ) );
        break;

    case ID_BUSTOBUS_ENTRY_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add bus to bus entry" ) );
        break;

    case ID_SHEET_SYMBOL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add sheet" ) );
        break;

    case ID_SHEET_PIN_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add sheet pins" ) );
        break;

    case ID_IMPORT_HLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Import sheet pins" ) );
        break;

    case ID_SCH_PLACE_COMPONENT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add component" ) );
        break;

    case ID_PLACE_POWER_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add power" ) );
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    default:
        SetRepeatItem( NULL );
    }

    // Simulate left click event if we got here from a hot key.
    if( aEvent.GetClientObject() != NULL )
    {
        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxPoint pos = data->GetPosition();

        INSTALL_UNBUFFERED_DC( dc, m_canvas );
        OnLeftClick( &dc, pos );
    }
}


void SCH_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void SCH_EDIT_FRAME::DeleteConnection( bool aFullConnection )
{
    PICKED_ITEMS_LIST   pickList;
    SCH_SCREEN*         screen = GetScreen();
    wxPoint             pos = GetCrossHairPosition();

    if( screen->GetConnection( pos, pickList, aFullConnection ) != 0 )
    {
        DeleteItemsInList( m_canvas, pickList );
        OnModify();
    }
}


bool SCH_EDIT_FRAME::DeleteItemAtCrossHair( wxDC* DC )
{
    SCH_ITEM*   item;
    SCH_SCREEN* screen = GetScreen();

    item = LocateItem( GetCrossHairPosition(), SCH_COLLECTOR::ParentItems );

    if( item )
    {
        bool itemHasConnections = item->IsConnectable();

        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );
        DeleteItem( item );

        if( itemHasConnections )
            screen->TestDanglingEnds( m_canvas, DC );

        OnModify();
        return true;
    }

    return false;
}


static void moveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*   item   = screen->GetCurItem();

    wxCHECK_RET( (item != NULL), wxT( "Cannot move invalid schematic item." ) );

#ifndef USE_WX_OVERLAY
    // Erase the current item at its current position.
    if( aErase )
        item->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
#endif

    item->SetPosition( aPanel->GetParent()->GetCrossHairPosition() );

    // Draw the item item at it's new position.
    item->SetWireImage();  // While moving, the item may choose to render differently
    item->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


static void abortMoveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN*     screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*       item = screen->GetCurItem();
    SCH_EDIT_FRAME* parent = (SCH_EDIT_FRAME*) aPanel->GetParent();

    parent->SetRepeatItem( NULL );
    screen->SetCurItem( NULL );

    if( item == NULL )  /* no current item */
        return;

    if( item->IsNew() )
    {
        delete item;
        item = NULL;
    }
    else
    {
        SCH_ITEM* oldItem = parent->GetUndoItem();

        SCH_ITEM* currentItem;

        // Items that are children of other objects are undone by swapping the contents
        // of the parent items.
        if( (item->Type() == SCH_SHEET_PIN_T) || (item->Type() == SCH_FIELD_T) )
        {
            currentItem = (SCH_ITEM*) item->GetParent();
        }
        else
        {
            currentItem = item;
        }

        wxCHECK_RET( oldItem != NULL && currentItem->Type() == oldItem->Type(),
                     wxT( "Cannot restore undefined or bad last schematic item." ) );

        // Never delete existing item, because it can be referenced by an undo/redo command
        // Just restore its data
        currentItem->SwapData( oldItem );

        // Erase the wire representation before the 'normal' view is drawn.
        if ( item->IsWireImage() )
            item->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

        item->ClearFlags();
    }

    aPanel->Refresh();
}


void SCH_EDIT_FRAME::MoveItem( SCH_ITEM* aItem, wxDC* aDC )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot move invalid schematic item" ) );

    SetRepeatItem( NULL );

    if( !aItem->IsNew() )
    {
        if( (aItem->Type() == SCH_SHEET_PIN_T) || (aItem->Type() == SCH_FIELD_T) )
            SetUndoItem( (SCH_ITEM*) aItem->GetParent() );
        else
            SetUndoItem( aItem );
    }

    aItem->SetFlags( IS_MOVED );
#ifdef USE_WX_OVERLAY
    this->Refresh();
    this->Update();
#endif
    m_canvas->CrossHairOff( aDC );

    if( aItem->Type() != SCH_SHEET_PIN_T )
        SetCrossHairPosition( aItem->GetPosition() );

    m_canvas->MoveCursorToCrossHair();

    OnModify();
    m_canvas->SetMouseCapture( moveItem, abortMoveItem );
    GetScreen()->SetCurItem( aItem );
    moveItem( m_canvas, aDC, wxDefaultPosition, true );
    m_canvas->CrossHairOn( aDC );
}


void SCH_EDIT_FRAME::OnRotate( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    // Allows block rotate operation on hot key.
    if( screen->m_BlockLocate.GetState() != STATE_NO_BLOCK )
    {
        screen->m_BlockLocate.SetCommand( BLOCK_ROTATE );
        HandleBlockEnd( &dc );
        return;
    }

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = LocateAndShowItem( data->GetPosition(), SCH_COLLECTOR::RotatableItems,
                                  aEvent.GetInt() );

        // Exit if no item found at the current location or the item is already being edited.
        if( (item == NULL) || (item->GetFlags() != 0) )
            return;
    }

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        if( aEvent.GetId() == ID_SCH_ROTATE_CLOCKWISE )
            OrientComponent( CMP_ROTATE_CLOCKWISE );
        else if( aEvent.GetId() == ID_SCH_ROTATE_COUNTERCLOCKWISE )
            OrientComponent( CMP_ROTATE_COUNTERCLOCKWISE );
        else
            wxFAIL_MSG( wxT( "Unknown rotate item command ID." ) );

        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
        m_canvas->MoveCursorToCrossHair();
        ChangeTextOrient( (SCH_TEXT*) item, &dc );
        break;

    case SCH_FIELD_T:
        m_canvas->MoveCursorToCrossHair();
        RotateField( (SCH_FIELD*) item, &dc );
        break;

    case SCH_BITMAP_T:
        RotateImage( (SCH_BITMAP*) item );
        break;

    case SCH_SHEET_T:           /// @todo allow sheet rotate on hotkey
    default:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot rotate schematic item type %s." ),
                                      GetChars( item->GetClass() ) ) );
    }

    if( item->GetFlags() == 0 )
        screen->SetCurItem( NULL );
}


void SCH_EDIT_FRAME::OnEditItem( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        // Set the locat filter, according to the edit command
        const KICAD_T* filterList = SCH_COLLECTOR::EditableItems;
        const KICAD_T* filterListAux = NULL;

        switch( aEvent.GetId() )
        {
        case ID_SCH_EDIT_COMPONENT_REFERENCE:
            filterList = SCH_COLLECTOR::CmpFieldReferenceOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        case ID_SCH_EDIT_COMPONENT_VALUE:
            filterList = SCH_COLLECTOR::CmpFieldValueOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        case ID_SCH_EDIT_COMPONENT_FOOTPRINT:
            filterList = SCH_COLLECTOR::CmpFieldFootprintOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        default:
            break;
        }

        item = LocateAndShowItem( data->GetPosition(), filterList, aEvent.GetInt() );

        // If no item found, and if an auxiliary filter exists, try to use it
        if( !item && filterListAux )
            item = LocateAndShowItem( data->GetPosition(), filterListAux, aEvent.GetInt() );

        // Exit if no item found at the current location or the item is already being edited.
        if( (item == NULL) || (item->GetFlags() != 0) )
            return;
    }

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        switch( aEvent.GetId() )
        {
        case ID_SCH_EDIT_COMPONENT_REFERENCE:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( REFERENCE ) );
            break;

        case ID_SCH_EDIT_COMPONENT_VALUE:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( VALUE ) );
            break;

        case ID_SCH_EDIT_COMPONENT_FOOTPRINT:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( FOOTPRINT ) );
            break;

        case ID_SCH_EDIT_ITEM:
            EditComponent( (SCH_COMPONENT*) item );
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Invalid schematic component edit command ID %d" ),
                                          aEvent.GetId() ) );
        }

        break;
    }

    case SCH_SHEET_T:
        EditSheet( (SCH_SHEET*) item, &dc );
        break;

    case SCH_SHEET_PIN_T:
        EditSheetPin( (SCH_SHEET_PIN*) item, &dc );
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
        EditSchematicText( (SCH_TEXT*) item );
        break;

    case SCH_FIELD_T:
        EditComponentFieldText( (SCH_FIELD*) item );
        break;

    case SCH_BITMAP_T:
        EditImage( (SCH_BITMAP*) item );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot edit schematic item type %s." ),
                                      GetChars( item->GetClass() ) ) );
    }

    if( item->GetFlags() == 0 )
        screen->SetCurItem( NULL );
}


void SCH_EDIT_FRAME::OnDragItem( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    // The easiest way to handle a menu or a hot key drag command
    // is to simulate a block drag command
    //
    // When a drag item is requested, some items use a BLOCK_DRAG_ITEM drag type
    // an some items use a BLOCK_DRAG drag type  (mainly a junction)
    // a BLOCK_DRAG collects all items in a block (here a 2x2 rect centered on the cursor)
    // and BLOCK_DRAG_ITEM drag only the selected item
    BLOCK_COMMAND_T dragType = BLOCK_DRAG_ITEM;

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = LocateAndShowItem( data->GetPosition(), SCH_COLLECTOR::DraggableItems,
                                  aEvent.GetInt() );

        // Exit if no item found at the current location or the item is already being edited.
        if( (item == NULL) || (item->GetFlags() != 0) )
            return;

        // When a junction or a node is found, a BLOCK_DRAG is better
        if( m_collectedItems.IsCorner() || m_collectedItems.IsNode( false )
            || m_collectedItems.IsDraggableJunction() )
            dragType = BLOCK_DRAG;
    }

    switch( item->Type() )
    {
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_LINE_T:
    case SCH_JUNCTION_T:
    case SCH_COMPONENT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_SHEET_T:
        m_canvas->MoveCursorToCrossHair();

        if( screen->m_BlockLocate.GetState() == STATE_NO_BLOCK )
        {
            INSTALL_UNBUFFERED_DC( dc, m_canvas );

            if( !HandleBlockBegin( &dc, dragType, GetCrossHairPosition() ) )
                break;

            // Give a non null size to the search block:
            screen->m_BlockLocate.Inflate( 1 );
            HandleBlockEnd( &dc );
        }

        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot drag schematic item type %s." ),
                                      GetChars( item->GetClass() ) ) );
    }
}


void SCH_EDIT_FRAME::OnOrient( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item   = screen->GetCurItem();

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    // Allows block rotate operation on hot key.
    if( screen->m_BlockLocate.GetState() != STATE_NO_BLOCK )
    {
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
        {
            m_canvas->MoveCursorToCrossHair();
            screen->m_BlockLocate.SetMessageBlock( this );
            screen->m_BlockLocate.SetCommand( BLOCK_MIRROR_X );
            HandleBlockEnd( &dc );
        }
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
        {
            m_canvas->MoveCursorToCrossHair();
            screen->m_BlockLocate.SetMessageBlock( this );
            screen->m_BlockLocate.SetCommand( BLOCK_MIRROR_Y );
            HandleBlockEnd( &dc );
        }
        else
        {
            wxFAIL_MSG( wxT( "Unknown block oriention command ID." ) );
        }

        return;
    }

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = LocateAndShowItem( data->GetPosition(), SCH_COLLECTOR::OrientableItems,
                                  aEvent.GetInt() );

        // Exit if no item found at the current location or the item is already being edited.
        if( (item == NULL) || (item->GetFlags() != 0) )
            return;
    }


    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
            OrientComponent( CMP_MIRROR_X );
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
            OrientComponent( CMP_MIRROR_Y );
        else if( aEvent.GetId() == ID_SCH_ORIENT_NORMAL )
            OrientComponent( CMP_NORMAL );
        else
            wxFAIL_MSG( wxT( "Invalid orient schematic component command ID." ) );

        break;

    case SCH_BITMAP_T:
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
            MirrorImage( (SCH_BITMAP*) item, true );
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
            MirrorImage( (SCH_BITMAP*) item, false );

        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Schematic object type %s cannot be oriented." ),
                                      GetChars( item->GetClass() ) ) );
    }

    if( item->GetFlags() == 0 )
        screen->SetCurItem( NULL );
}
