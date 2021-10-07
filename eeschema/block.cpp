/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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
 * @file eeschema/block.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>

#include <general.h>
#include <class_library.h>
#include <lib_pin.h>
#include <list_operations.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <list_operations.h>

#include <preview_items/selection_area.h>
#include <sch_view.h>
#include <view/view_group.h>

static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                     const wxPoint& aPosition, bool aErase );

int SCH_EDIT_FRAME::BlockCommand( EDA_KEY key )
{
    int cmd = BLOCK_IDLE;

    switch( key )
    {
    default:
        cmd = key & 0xFFFF;

        if( cmd == 0 )      // All not handled values are seen as block move
            cmd = BLOCK_MOVE;

        break;

    case GR_KB_ALT:         // Should be BLOCK_ROTATE. Not suported: fall into move
    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_DUPLICATE;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_DRAG;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


void SCH_EDIT_FRAME::InitBlockPasteInfos()
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    block->GetItems().CopyList( m_blockItems.GetItems() );
    m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
}


void SCH_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    wxCHECK_RET( m_canvas->IsMouseCaptured(), "No block mouse capture callback is set" );

    if( block->GetCount() == 0 )
    {
        wxString msg;
        msg.Printf( wxT( "HandleBlockPLace() error : no items to place (cmd %d, state %d)" ),
                    block->GetCommand(), block->GetState() );
        wxFAIL_MSG( msg );
    }

    block->SetState( STATE_BLOCK_STOP );

    switch( block->GetCommand() )
    {
    case BLOCK_DRAG:        // Drag from mouse
    case BLOCK_DRAG_ITEM:   // Drag from a component selection and drag command
    case BLOCK_MOVE:
    case BLOCK_DUPLICATE:           /* Duplicate */
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        // If the block wasn't changed, don't update the schematic
        if( block->GetMoveVector() == wxPoint( 0, 0 ) && !block->AppendUndo() )
        {
            // This calls the block-abort command routine on cleanup
            m_canvas->EndMouseCapture( GetToolId(), GetGalCanvas()->GetCurrentCursor() );
            return;
        }

        if( block->GetCommand() != BLOCK_DUPLICATE )
            SaveCopyInUndoList( block->GetItems(), UR_CHANGED, block->AppendUndo(), block->GetMoveVector() );

        for( unsigned ii = 0; ii < block->GetItems().GetCount(); ii++ )
        {
            SCH_ITEM* item = static_cast<SCH_ITEM*>( block->GetItems().GetPickedItem( ii ) );
            item->Move( block->GetMoveVector() );
            item->SetFlags( IS_MOVED );
            GetCanvas()->GetView()->Update( item, KIGFX::GEOMETRY );
            item->ClearFlags();
        }
        break;

    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        DuplicateItemsInList( GetScreen(), block->GetItems(), block->GetMoveVector() );

        SaveCopyInUndoList( block->GetItems(), UR_CHANGED, block->AppendUndo() );
        break;

    case BLOCK_PASTE:
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        PasteListOfItems( DC );
        break;

    default:        // others are handled by HandleBlockEnd()
       break;
    }

    CheckListConnections( block->GetItems(), true );
    block->ClearItemsList();
    SchematicCleanUp( true );
    TestDanglingEnds();
    OnModify();

    // clear some flags and pointers
    GetScreen()->ClearDrawingState();
    GetScreen()->ClearBlockCommand();
    GetScreen()->SetCurItem( NULL );

    m_canvas->EndMouseCapture( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString, false );

    GetCanvas()->GetView()->ClearPreview();
    GetCanvas()->GetView()->ClearHiddenFlags();
}


bool SCH_EDIT_FRAME::HandleBlockEnd( wxDC* aDC )
{
    bool            nextcmd = false;
    bool            append = false;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    auto panel =static_cast<SCH_DRAW_PANEL*>(m_canvas);
    auto view = panel->GetView();

    view->ShowSelectionArea( false );
    view->ClearHiddenFlags();

    if( block->GetCount() )
    {
        BLOCK_STATE_T   state   = block->GetState();
        BLOCK_COMMAND_T command = block->GetCommand();

        m_canvas->CallEndMouseCapture( aDC );

        block->SetState( state );
        block->SetCommand( command );
        m_canvas->SetMouseCapture( DrawAndSizingBlockOutlines, AbortBlockCurrentCommand );

        if( block->GetCommand() != BLOCK_ABORT
            && block->GetCommand() != BLOCK_DUPLICATE
            && block->GetCommand() != BLOCK_COPY
            && block->GetCommand() != BLOCK_CUT
            && block->GetCommand() != BLOCK_DELETE )
        {
            SetCrossHairPosition( block->GetEnd() );
            m_canvas->MoveCursorToCrossHair();
        }
    }

    if( m_canvas->IsMouseCaptured() )
    {
        switch( block->GetCommand() )
        {
        case BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace()" ) );
            break;

        case BLOCK_DRAG:
        case BLOCK_DRAG_ITEM:   // Drag from a drag command
        case BLOCK_MOVE:
        case BLOCK_DUPLICATE:
        case BLOCK_PRESELECT_MOVE:
            if( block->GetCommand() == BLOCK_DRAG_ITEM )
            {
                // This is a drag command, not a mouse block command
                // Only this item is put in list
                if( GetScreen()->GetCurItem() )
                {
                    ITEM_PICKER picker;
                    picker.SetItem( GetScreen()->GetCurItem() );
                    block->PushItem( picker );
                }
            }
            else if( block->GetCommand() != BLOCK_PRESELECT_MOVE )
            {
                // Collect all items in the locate block
                GetScreen()->UpdatePickList();
            }

            GetScreen()->SelectBlockItems();

            if( block->GetCommand() == BLOCK_DUPLICATE )
            {
                DuplicateItemsInList( GetScreen(), block->GetItems(), block->GetMoveVector() );
                block->SetLastCursorPosition( GetCrossHairPosition() );
                SaveCopyInUndoList( block->GetItems(), UR_NEW );
                block->SetAppendUndo();
            }

            if( block->GetCount() )
            {
                nextcmd = true;
                block->SetState( STATE_BLOCK_MOVE );

                if( block->GetCommand() != BLOCK_DRAG && block->GetCommand() != BLOCK_DRAG_ITEM )
                {
                    // Mark dangling pins at the edges of the block:
                    std::vector<DANGLING_END_ITEM> internalPoints;

                    for( unsigned i = 0; i < block->GetCount(); ++i )
                    {
                        auto item = static_cast<SCH_ITEM*>( block->GetItem( i ) );
                        item->GetEndPoints( internalPoints );
                    }

                    for( unsigned i = 0; i < block->GetCount(); ++i )
                    {
                        auto item = static_cast<SCH_ITEM*>( block->GetItem( i ) );
                        item->UpdateDanglingState( internalPoints );
                    }
                }

                m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
                m_canvas->CallMouseCapture( aDC, wxDefaultPosition, false );
            }
            else
            {
                m_canvas->CallMouseCapture( aDC, wxDefaultPosition, false );
                m_canvas->SetMouseCapture( NULL, NULL );
            }
            break;

        case BLOCK_CUT:
        case BLOCK_DELETE:
            GetScreen()->UpdatePickList();

            // The CUT variant needs to copy the items from their originial position
            if( ( block->GetCommand() == BLOCK_CUT ) && block->GetCount() )
            {
                wxPoint move_vector = -GetScreen()->m_BlockLocate.GetLastCursorPosition();
                copyBlockItems( block->GetItems(), move_vector );
            }

            // We set this in a while loop to catch any newly created items
            // as a result of the delete (e.g. merged wires)
            while( block->GetCount() )
            {
                DeleteItemsInList( block->GetItems(), append );
                SchematicCleanUp( true );
                OnModify();
                block->ClearItemsList();
                GetScreen()->UpdatePickList();
                append = true;
            }

            TestDanglingEnds();
            break;

        case BLOCK_COPY:    // Save a copy of items in paste buffer
            GetScreen()->UpdatePickList();

            if( block->GetCount() )
            {
                wxPoint move_vector = -GetScreen()->m_BlockLocate.GetLastCursorPosition();
                copyBlockItems( block->GetItems(), move_vector );
            }

            block->ClearItemsList();
            break;

        case BLOCK_ZOOM:
            Window_Zoom( GetScreen()->m_BlockLocate );
            break;

        default:
            break;
        }
    }

    if( block->GetCommand() == BLOCK_ABORT )
    {
        if( block->AppendUndo() )
        {
            PICKED_ITEMS_LIST* undo = GetScreen()->PopCommandFromUndoList();
            PutDataInPreviousState( undo, false );
            undo->ClearListAndDeleteItems();
            delete undo;
        }

        // We set the dangling ends to the block-scope, so we must set them back to
        // schematic-scope.
        TestDanglingEnds();

        view->ShowSelectionArea( false );
        view->ClearHiddenFlags();
    }

    if( !nextcmd )
    {
        GetScreen()->ClearBlockCommand();
        GetScreen()->ClearDrawingState();
        GetScreen()->SetCurItem( NULL );
        m_canvas->EndMouseCapture( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString,
                                   false );
    }

    view->ShowSelectionArea( false );
    if( !nextcmd )
        view->ClearPreview();
    view->ShowPreview( nextcmd );

    return nextcmd;
}


/* Traces the outline of the search block structures
 * The entire block follows the cursor
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase )
{
    SCH_DRAW_PANEL*    panel =static_cast<SCH_DRAW_PANEL*>( aPanel );
    KIGFX::SCH_VIEW*   view = panel->GetView();
    KIGFX::VIEW_GROUP* preview = view->GetPreview();

    BASE_SCREEN*       screen = aPanel->GetScreen();
    BLOCK_SELECTOR*    block = &screen->m_BlockLocate;
    SCH_ITEM*          schitem;

    block->SetMoveVector( panel->GetParent()->GetCrossHairPosition() - block->GetLastCursorPosition() );

    preview->Clear();
    view->SetVisible( preview, true );

    for( unsigned ii = 0; ii < block->GetCount(); ii++ )
    {
        schitem = (SCH_ITEM*) block->GetItem( ii );
        SCH_ITEM* copy = static_cast<SCH_ITEM*>( schitem->Clone() );

        copy->Move( block->GetMoveVector() );
        copy->SetFlags( IS_MOVED );
        preview->Add( copy );

        view->Hide( schitem );
    }

    view->Update( preview );
}


void SCH_EDIT_FRAME::copyBlockItems( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMoveVector )
{
    m_blockItems.ClearListAndDeleteItems();   // delete previous saved list, if exists

    wxRect bounds;

    if( aItemsList.GetCount() > 0 )
        bounds = aItemsList.GetPickedItem( 0 )->GetBoundingBox();

    for( unsigned i = 1; i < aItemsList.GetCount(); ++i )
        bounds.Union( aItemsList.GetPickedItem( i )->GetBoundingBox() );

    wxPoint center( ( bounds.GetLeft() + bounds.GetRight() ) / 2,
                    ( bounds.GetTop() + bounds.GetBottom() ) / 2 );
    center = GetNearestGridPosition( center );

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        // Clear m_Flag member of selected items:
        aItemsList.GetPickedItem( ii )->ClearFlags();

        /* Make a copy of the original picked item. */
        SCH_ITEM* copy = DuplicateStruct( (SCH_ITEM*) aItemsList.GetPickedItem( ii ) );
        copy->SetParent( NULL );
        copy->SetFlags( copy->GetFlags() | UR_TRANSIENT );
        copy->Move( -center );
        ITEM_PICKER item( copy, UR_NEW );

        m_blockItems.PushItem( item );
    }
}


void SCH_EDIT_FRAME::PasteListOfItems( wxDC* DC )
{
    unsigned       i;
    SCH_ITEM*      item;
    SCH_SHEET_LIST hierarchy( g_RootSheet, false );    // This is the entire schematic hierarcy.

    if( m_blockItems.GetCount() == 0 )
    {
        DisplayError( this, _( "No item to paste." ) );
        return;
    }

    wxFileName destFn = m_CurrentSheet->Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( Prj().GetProjectPath() );

    // Make sure any sheets in the block to be pasted will not cause recursion in
    // the destination sheet. Moreover new sheets create new sheetpaths, and component
    // alternante references must be created and cleared
    bool hasSheetPasted = false;

    // Keep trace of existing sheet paths. Paste block can modify this list
    SCH_SHEET_LIST initial_sheetpathList( g_RootSheet, false );

    for( i = 0; i < m_blockItems.GetCount(); i++ )
    {
        item = (SCH_ITEM*) m_blockItems.GetItem( i );

        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*)item;
            wxFileName srcFn = sheet->GetFileName();

            if( srcFn.IsRelative() )
                srcFn.MakeAbsolute( Prj().GetProjectPath() );

            SCH_SHEET_LIST sheetHierarchy( sheet );

            if( hierarchy.TestForRecursion( sheetHierarchy,
                                            destFn.GetFullPath( wxPATH_UNIX ) ) )
            {
                wxString msg;

                msg.Printf( _( "The sheet changes cannot be made because the destination "
                               "sheet already has the sheet \"%s\" or one of it's subsheets "
                               "as a parent somewhere in the schematic hierarchy." ),
                            GetChars( sheet->GetFileName() ) );
                DisplayError( this, msg );
                return;
            }

            // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
            // based sheet name and update the time stamp for each sheet in the block.
            timestamp_t timeStamp = GetNewTimeStamp();

            sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)timeStamp ) );
            sheet->SetTimeStamp( timeStamp );
            hasSheetPasted = true;
        }
    }

    PICKED_ITEMS_LIST picklist;

    for( i = 0; i < m_blockItems.GetCount(); i++ )
    {
        item = DuplicateStruct( (SCH_ITEM*) m_blockItems.GetItem( i ) );

        // Creates data, and push it as new data in undo item list buffer
        ITEM_PICKER picker( item, UR_NEW );
        picklist.PushItem( picker );

        // Clear annotation and init new time stamp for the new components and sheets:
        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* cmp = static_cast<SCH_COMPONENT*>( item );
            cmp->SetTimeStamp( GetNewTimeStamp() );

            // clear the annotation, but preserve the selected unit
            int unit = cmp->GetUnit();
            cmp->ClearAnnotation( NULL );
            cmp->SetUnit( unit );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            ( (SCH_SHEET*) item )->SetTimeStamp( GetNewTimeStamp() );
        }
    }

    SaveCopyInUndoList( picklist, UR_NEW );

    for( i = 0; i < picklist.GetCount(); ++i )
    {
        item = (SCH_ITEM*) picklist.GetPickedItem( i );

        item->Move( GetScreen()->m_BlockLocate.GetMoveVector() );

        SetSchItemParent( item, GetScreen() );
        AddToScreen( item );
    }

    if( hasSheetPasted )
    {
        // We clear annotation of new sheet paths.
        // Annotation of new components added in current sheet is already cleared.
        SCH_SCREENS screensList( g_RootSheet );
        screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
    }

    // Clear flags for all items.
    GetScreen()->ClearDrawingState();

    OnModify();
}

void DrawAndSizingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                 bool aErase )
{
    auto panel =static_cast<SCH_DRAW_PANEL*>(aPanel);
    auto area = panel->GetView()->GetSelectionArea();
    auto frame = static_cast<EDA_BASE_FRAME*>(aPanel->GetParent());

    BLOCK_SELECTOR* block;
    bool isLibEdit = frame->IsType( FRAME_SCH_LIB_EDITOR );

    block = &aPanel->GetScreen()->m_BlockLocate;
    block->SetMoveVector( wxPoint( 0, 0 ) );
    block->SetLastCursorPosition( aPanel->GetParent()->GetCrossHairPosition( isLibEdit ) );
    block->SetEnd( aPanel->GetParent()->GetCrossHairPosition() );

    panel->GetView()->ClearPreview();
    panel->GetView()->ClearHiddenFlags();

    area->SetOrigin( block->GetOrigin() );;
    area->SetEnd( block->GetEnd() );

    panel->GetView()->SetVisible( area );
    panel->GetView()->Hide( area, false );
    panel->GetView()->Update( area );

    if( block->GetState() == STATE_BLOCK_INIT )
    {
        if( block->GetWidth() || block->GetHeight() )
            // 2nd point exists: the rectangle is not surface anywhere
            block->SetState( STATE_BLOCK_END );
    }
}
