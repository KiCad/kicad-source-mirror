/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "wxEeschemaStruct.h"
#include "kicad_device_context.h"

#include "general.h"
#include "eeschema_id.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_sheet.h"


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
    case ID_POPUP_END_LINE:
    case ID_POPUP_SCH_EDIT_TEXT:
    case ID_POPUP_SCH_SET_SHAPE_TEXT:
    case ID_POPUP_SCH_ROTATE_TEXT:
    case ID_POPUP_SCH_EDIT_SHEET:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_SCH_END_SHEET:
    case ID_POPUP_SCH_RESIZE_SHEET:
    case ID_POPUP_IMPORT_GLABEL:
    case ID_POPUP_SCH_EDIT_SHEET_PIN:
    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_DRAG_WIRE_REQUEST:
    case ID_POPUP_SCH_EDIT_CMP:
    case ID_POPUP_SCH_INIT_CMP:
    case ID_POPUP_SCH_DISPLAYDOC_CMP:
    case ID_POPUP_SCH_EDIT_VALUE_CMP:
    case ID_POPUP_SCH_EDIT_REF_CMP:
    case ID_POPUP_SCH_EDIT_FOOTPRINT_CMP:
    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
    case ID_POPUP_SCH_ROTATE_FIELD:
    case ID_POPUP_SCH_EDIT_FIELD:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DRAG_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
    case ID_POPUP_SCH_ENTER_SHEET:
    case ID_POPUP_SCH_LEAVE_SHEET:
    case ID_POPUP_SCH_ADD_JUNCTION:
    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_GETINFO_MARKER:
    case ID_POPUP_SCH_ROTATE_IMAGE:
    case ID_POPUP_SCH_MIRROR_X_IMAGE:
    case ID_POPUP_SCH_MIRROR_Y_IMAGE:

        /* At this point: Do nothing. these commands do not need to stop the
         * current command (mainly a block command) or reset the current state
         * They will be executed later, in next switch structure.
         */
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:

        // Stop the current command (if any) but keep the current tool
        DrawPanel->EndMouseCapture();
        break;

    default:

        // Stop the current command and deselect the current tool
        DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );
        break;
    }

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );
    item = screen->GetCurItem();    // Can be modified by previous calls.

    switch( id )
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( &dc, pos );
        m_itemToRepeat = NULL;
        break;

    case wxID_CUT:
        if( screen->m_BlockLocate.m_Command != BLOCK_MOVE )
            break;
        HandleBlockEndByPopUp( BLOCK_DELETE, &dc );
        m_itemToRepeat = NULL;
        SetSheetNumberAndCount();
        break;

    case wxID_PASTE:
        HandleBlockBegin( &dc, BLOCK_PASTE, screen->GetCrossHairPosition() );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
        DrawPanel->MoveCursorToCrossHair();
        SetBusEntryShape( &dc, (SCH_BUS_ENTRY*) item, '/' );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
        DrawPanel->MoveCursorToCrossHair();
        SetBusEntryShape( &dc, (SCH_BUS_ENTRY*) item, '\\' );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( DrawPanel->IsMouseCaptured() )
        {
            DrawPanel->EndMouseCapture();
            SetToolID( GetToolId(), DrawPanel->GetCurrentCursor(), wxEmptyString );
        }
        else
            SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_POPUP_END_LINE:
        DrawPanel->MoveCursorToCrossHair();
        EndSegment( &dc );
        break;

    case ID_POPUP_SCH_EDIT_TEXT:
        EditSchematicText( (SCH_TEXT*) item );
        break;

    case ID_POPUP_SCH_ROTATE_TEXT:
        DrawPanel->MoveCursorToCrossHair();
        ChangeTextOrient( (SCH_TEXT*) item, &dc );
        break;

    case ID_POPUP_SCH_SET_SHAPE_TEXT:

        // Not used
        break;

    case ID_POPUP_SCH_ROTATE_FIELD:
        DrawPanel->MoveCursorToCrossHair();
        RotateField( (SCH_FIELD*) item, &dc );
        break;

    case ID_POPUP_SCH_EDIT_FIELD:
        EditComponentFieldText( (SCH_FIELD*) item, &dc );
        break;

    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
        DrawPanel->MoveCursorToCrossHair();
        DeleteConnection( id == ID_POPUP_SCH_DELETE_CONNECTION );
        screen->SetCurItem( NULL );
        m_itemToRepeat = NULL;
        screen->TestDanglingEnds( DrawPanel, &dc );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_SCH_BREAK_WIRE:
    {
        DrawPanel->MoveCursorToCrossHair();
        SCH_ITEM* oldWiresList = screen->ExtractWires( true );
        screen->BreakSegment( screen->GetCrossHairPosition() );

        if( oldWiresList )
            SaveCopyInUndoList( oldWiresList, UR_WIRE_IMAGE );

        screen->TestDanglingEnds( DrawPanel, &dc );
    }
    break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:
        if( item == NULL )
            break;

        DeleteItem( item );
        screen->SetCurItem( NULL );
        m_itemToRepeat = NULL;
        screen->TestDanglingEnds( DrawPanel, &dc );
        SetSheetNumberAndCount();
        OnModify();
        break;

    case ID_POPUP_SCH_END_SHEET:
        DrawPanel->MoveCursorToCrossHair();
        item->Place( this, &dc );
        break;

    case ID_POPUP_SCH_RESIZE_SHEET:
        ReSizeSheet( (SCH_SHEET*) item, &dc );
        screen->TestDanglingEnds( DrawPanel, &dc );
        break;

    case ID_POPUP_SCH_EDIT_SHEET:
        if( EditSheet( (SCH_SHEET*) item, &dc ) )
            OnModify();
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
            DrawPanel->RefreshDrawingRect( sheet->GetBoundingBox() );
        }
        break;

    case ID_POPUP_SCH_EDIT_SHEET_PIN:
        EditSheetPin( (SCH_SHEET_PIN*) item, &dc );
        break;

    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_DRAG_WIRE_REQUEST:
        DrawPanel->MoveCursorToCrossHair();

        // The easiest way to handle a drag component or sheet command
        // is to simulate a block drag command
        if( screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
        {
            if( !HandleBlockBegin( &dc, BLOCK_DRAG, screen->GetCrossHairPosition() ) )
                break;

            // Give a non null size to the search block:
            screen->m_BlockLocate.Inflate( 1 );
            HandleBlockEnd( &dc );
        }

        break;

    case ID_POPUP_SCH_EDIT_CMP:
        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
            EditComponent( (SCH_COMPONENT*) item );

        break;

    case ID_POPUP_SCH_INIT_CMP:
        DrawPanel->MoveCursorToCrossHair();
        break;

    case ID_POPUP_SCH_EDIT_VALUE_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( item != NULL && item->Type() == SCH_COMPONENT_T )
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( VALUE ), &dc );

        break;

    case ID_POPUP_SCH_EDIT_REF_CMP:

        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item != NULL && item->Type() == SCH_COMPONENT_T )
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( REFERENCE ), &dc );

        break;

    case ID_POPUP_SCH_EDIT_FOOTPRINT_CMP:

        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( FOOTPRINT ), &dc );

        break;


    case ID_POPUP_SCH_EDIT_CONVERT_CMP:

        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            DrawPanel->MoveCursorToCrossHair();
            ConvertPart( (SCH_COMPONENT*) item, &dc );
        }

        break;

    case ID_POPUP_SCH_DISPLAYDOC_CMP:

        // Ensure the struct is a component (could be a piece of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            LIB_ALIAS* LibEntry;
            LibEntry = CMP_LIBRARY::FindLibraryEntry( ( (SCH_COMPONENT*) item )->GetLibName() );

            if( LibEntry && LibEntry->GetDocFileName() != wxEmptyString )
            {
                GetAssociatedDocument( this, LibEntry->GetDocFileName(),
                                       &wxGetApp().GetLibraryPathList() );
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

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        break;

    case wxID_COPY:         // really this is a Save block for paste
        HandleBlockEndByPopUp( BLOCK_SAVE, &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        HandleBlockEndByPopUp( BLOCK_ZOOM, &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_DELETE, &dc );
        SetSheetNumberAndCount();
        break;

    case ID_POPUP_ROTATE_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_ROTATE, &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_MIRROR_X, &dc );
        break;

    case ID_POPUP_MIRROR_Y_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_MIRROR_Y, &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_COPY, &dc );
        break;

    case ID_POPUP_DRAG_BLOCK:
        DrawPanel->MoveCursorToCrossHair();
        HandleBlockEndByPopUp( BLOCK_DRAG, &dc );
        break;

    case ID_POPUP_SCH_ADD_JUNCTION:
        DrawPanel->MoveCursorToCrossHair();
        screen->SetCurItem( AddJunction( &dc, screen->GetCrossHairPosition(), true ) );
        screen->TestDanglingEnds( DrawPanel, &dc );
        screen->SetCurItem( NULL );
        break;

    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_ADD_GLABEL:
        screen->SetCurItem( CreateNewText( &dc, id == ID_POPUP_SCH_ADD_LABEL ?
                                           LAYER_LOCLABEL : LAYER_GLOBLABEL ) );
        item = screen->GetCurItem();

        if( item )
        {
            item->Place( this, &dc );
            screen->TestDanglingEnds( DrawPanel, &dc );
            screen->SetCurItem( NULL );
        }

        break;

    case ID_POPUP_SCH_GETINFO_MARKER:
        if( item && item->Type() == SCH_MARKER_T )
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );

        break;

    case ID_POPUP_SCH_EDIT_IMAGE:
        if( item && item->GetFlags() == 0 )
            EditImage( (SCH_BITMAP*) item );
        break;

    case ID_POPUP_SCH_ROTATE_IMAGE:
        if( item )
            RotateImage( (SCH_BITMAP*) item );
        break;

    case ID_POPUP_SCH_MIRROR_X_IMAGE:
        if( item )
            MirrorImage( (SCH_BITMAP*) item, true );
        break;

    case ID_POPUP_SCH_MIRROR_Y_IMAGE:
        if( item )
            MirrorImage( (SCH_BITMAP*) item, false );
        break;

    default:        // Log error:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::Process_Special_Functions error" ) );
        break;
    }

    // End switch ( id )    (Command execution)

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_itemToRepeat = NULL;
}


void SCH_EDIT_FRAME::OnMoveItem( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    if( item == NULL )
        return;

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    switch( item->Type() )
    {
    case SCH_LINE_T:
        break;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_BUS_ENTRY_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
    case SCH_COMPONENT_T:
        MoveItem( item, &dc );
        break;

    case SCH_BITMAP_T:
        MoveImage( (SCH_BITMAP*) item, &dc );
        break;

    case SCH_SHEET_T:
        StartMoveSheet( (SCH_SHEET*) item, &dc );
        break;

    case SCH_FIELD_T:
        MoveField( (SCH_FIELD*) item, &dc );
        break;

    case SCH_SHEET_PIN_T:
        MoveSheetPin( (SCH_SHEET_PIN*) item, &dc );
        break;

    case SCH_MARKER_T:
    default:
        wxFAIL_MSG( wxString::Format( wxT( "Cannot move item type %s" ),
                                      GetChars( item->GetClass() ) ) );
        break;
    }

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        m_itemToRepeat = NULL;
}


void SCH_EDIT_FRAME::OnCancelCurrentCommand( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    if( screen->IsBlockActive() )
    {
        DrawPanel->SetCursor( wxCursor( DrawPanel->GetDefaultCursor() ) );
        screen->ClearBlockCommand();

        // Stop the current command (if any) but keep the current tool
        DrawPanel->EndMouseCapture();
    }
    else
    {
        if( DrawPanel->IsMouseCaptured() ) // Stop the current command but keep the current tool
            DrawPanel->EndMouseCapture();
        else                    // Deselect current tool
            DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );
     }
}


void SCH_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    // Stop the current command and deselect the current tool.
    DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( id, DrawPanel->GetDefaultCursor(), _( "No tool selected" ) );
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
        m_itemToRepeat = NULL;
    }
}


void SCH_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_VToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void SCH_EDIT_FRAME::DeleteConnection( bool aFullConnection )
{
    PICKED_ITEMS_LIST pickList;
    SCH_SCREEN* screen = GetScreen();
    wxPoint pos = screen->GetCrossHairPosition();

    if( screen->GetConnection( pos, pickList, aFullConnection ) != 0 )
    {
        DeleteItemsInList( DrawPanel, pickList );
        OnModify();
    }
}


bool SCH_EDIT_FRAME::DeleteItemAtCrossHair( wxDC* DC )
{
    SCH_ITEM* item;
    SCH_SCREEN* screen = GetScreen();

    item = LocateItem( screen->GetCrossHairPosition(), SCH_COLLECTOR::ParentItems );

    if( item )
    {
        bool itemHasConnections = item->IsConnectable();

        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );
        DeleteItem( item );

        if( itemHasConnections )
            screen->TestDanglingEnds( DrawPanel, DC );

        OnModify();
        return true;
    }

    return false;
}
