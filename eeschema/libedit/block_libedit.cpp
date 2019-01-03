/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file block_libedit.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>

#include <general.h>
#include <class_library.h>
#include <lib_edit_frame.h>

#include <preview_items/selection_area.h>
#include <sch_view.h>
#include <view/view_group.h>

static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );


int LIB_EDIT_FRAME::BlockSelectItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock, int aUnit, int aConvert, bool aSyncPinEdit )
{
    int itemCount = 0;

    for( LIB_ITEM& item : aPart->GetDrawItems() )
    {
        item.ClearFlags( SELECTED );

        if( ( item.GetUnit() && item.GetUnit() != aUnit )
            || ( item.GetConvert() && item.GetConvert() != aConvert ) )
        {
            if( item.Type() != LIB_PIN_T )
                continue;

             // Specific rules for pins:
             // - do not select pins in other units when synchronized pin edit mode is disabled
             // - do not select pins in other units when units are not interchangeable
             // - in other cases verify if the pin belongs to the requested unit
            if( !aSyncPinEdit || aPart->UnitsLocked()
                || ( item.GetConvert() && item.GetConvert() != aConvert ) )
                continue;
        }

        if( item.Inside( *aBlock ) )
        {
            auto picker = ITEM_PICKER( &item );
            aBlock->PushItem( picker );
            item.SetFlags( SELECTED );
            itemCount++;
        }
    }

    return itemCount;
}


void LIB_EDIT_FRAME::BlockClearSelectedItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock )
{
    for( LIB_ITEM& item : aPart->GetDrawItems() )
    {
        item.ClearFlags();
    }
    aBlock->ClearItemsList();
}


void LIB_EDIT_FRAME::BlockMoveSelectedItems( const wxPoint& aOffset, LIB_PART* aPart, BLOCK_SELECTOR* aBlock )
{
    for( LIB_ITEM& item : aPart->GetDrawItems() )
    {
        if( !item.IsSelected() )
            continue;

        item.SetOffset( aOffset );
        item.ClearFlags();
    }

    // view update
}


void LIB_EDIT_FRAME::BlockDeleteSelectedItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock )
{
    LIB_ITEMS_CONTAINER::ITERATOR item = aPart->GetDrawItems().begin();

    // We *do not* remove the 2 mandatory fields: reference and value
    // so skip them (do not remove) if they are flagged selected.
    // Skip also not visible items.
    // But I think fields must not be deleted by a block delete command or other global command
    // because they are not really graphic items
    while( item != aPart->GetDrawItems().end() )
    {
        if( item->Type() == LIB_FIELD_T )
        {
            item->ClearFlags( SELECTED );
        }

        if( !item->IsSelected() )
            ++item;
        else
            item = aPart->GetDrawItems().erase( item );
    }

    // view update

}


void LIB_EDIT_FRAME::BlockCopySelectedItems( const wxPoint& aOffset, LIB_PART* aPart, BLOCK_SELECTOR* aBlock )
{
    PICKED_ITEMS_LIST& aItemsList = aBlock->GetItems();
    LIB_ITEM* oldItem;
    LIB_ITEM* newItem;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        oldItem = static_cast<LIB_ITEM*>( aItemsList.GetPickedItem( ii ) );

        // We *do not* copy fields because they are unique for the whole component
        // so skip them (do not duplicate) if they are flagged selected.
        if( oldItem->Type() == LIB_FIELD_T )
            oldItem->ClearFlags( SELECTED );

        if( !oldItem->IsSelected() )
            continue;

        newItem = (LIB_ITEM*) oldItem->Clone();
        newItem->SetFlags( SELECTED );
        oldItem->ClearFlags( SELECTED );

        newItem->SetOffset( aBlock->GetMoveVector() );

        aItemsList.SetPickedItem( newItem, ii );
        aItemsList.SetPickedItemStatus( UR_NEW, ii );

        aPart->GetDrawItems().push_back( newItem );
    }
}


int LIB_EDIT_FRAME::BlockCommand( EDA_KEY key )
{
    int cmd = BLOCK_IDLE;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case EDA_KEY_C( 0xffffffff ):   // -1
        // Historically, -1 has been used as a key, which can cause bit flag
        // clashes with unaware code. On debug builds, catch any old code that
        // might still be doing this. TODO: remove if sure all this old code is gone.
        wxFAIL_MSG( "negative EDA_KEY value should be converted to GR_KEY_INVALID" );
        // fall through on release builds

    case GR_KEY_INVALID:
        cmd = BLOCK_PRESELECT_MOVE;
        break;

    case GR_KEY_NONE:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_DUPLICATE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_ROTATE;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_MIRROR_Y;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


bool LIB_EDIT_FRAME::HandleBlockEnd( wxDC* aDC )
{
    int ItemCount = 0;
    bool nextCmd = false;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;
    wxPoint pt;

    auto panel =static_cast<SCH_DRAW_PANEL*>(m_canvas);
    auto view = panel->GetView();
    auto area = view->GetSelectionArea();

    auto start = area->GetOrigin();
    auto end = area->GetEnd();

    block->SetOrigin( wxPoint( start.x, start.y ) );
    block->SetEnd( wxPoint( end.x, end.y ) );

    view->ShowSelectionArea( false );
    view->ClearHiddenFlags();

    if( block->GetCount() )
    {
        BLOCK_STATE_T state     = block->GetState();
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
        case  BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
            break;

        case BLOCK_DRAG:        // Drag
        case BLOCK_DRAG_ITEM:
        case BLOCK_MOVE:        // Move
        case BLOCK_DUPLICATE:   // Duplicate
            if( GetCurPart() )
                ItemCount = BlockSelectItems( GetCurPart(), block, m_unit, m_convert, m_syncPinEdit );

            if( ItemCount )
            {
                nextCmd = true;
                block->SetState( STATE_BLOCK_MOVE );

                if( block->GetCommand() == BLOCK_DUPLICATE )
                {
                    if( block->AppendUndo() )
                        ; // UR_LIBEDIT saves entire state, so no need to append anything more
                    else
                    {
                        SaveCopyInUndoList( GetCurPart(), UR_LIBEDIT );
                        block->SetAppendUndo();
                    }

                    BlockCopySelectedItems( pt, GetCurPart(), block );
                    block->SetLastCursorPosition( GetCrossHairPosition( true ) );
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

        case BLOCK_COPY:    // Save a copy of items in the clipboard buffer
        case BLOCK_CUT:
            if( GetCurPart() )
                ItemCount = BlockSelectItems( GetCurPart(), block, m_unit, m_convert, m_syncPinEdit );

            if( ItemCount )
            {
                copySelectedItems();
                auto cmd = block->GetCommand();

                if( cmd == BLOCK_COPY )
                {
                    BlockClearSelectedItems( GetCurPart(), block );
                    block->ClearItemsList();
                }
                else if( cmd == BLOCK_CUT )
                {
                    if( block->AppendUndo() )
                        ; // UR_LIBEDIT saves entire state, so no need to append anything more
                    else
                    {
                        SaveCopyInUndoList( GetCurPart(), UR_LIBEDIT );
                        block->SetAppendUndo();
                    }

                    BlockDeleteSelectedItems( GetCurPart(), block );
                    RebuildView();
                    GetCanvas()->Refresh();
                    OnModify();
                }
            }
            break;

        case BLOCK_DELETE:     // Delete
            if( GetCurPart() )
                ItemCount = BlockSelectItems( GetCurPart(), block, m_unit, m_convert, m_syncPinEdit );

            if( block->AppendUndo() )
                ; // UR_LIBEDIT saves entire state, so no need to append anything more
            else if( ItemCount )
            {
                SaveCopyInUndoList( GetCurPart(), UR_LIBEDIT );
                block->SetAppendUndo();
            }

            if( GetCurPart() )
            {
                BlockDeleteSelectedItems( GetCurPart(), block );
                RebuildView();
                GetCanvas()->Refresh();
                OnModify();
            }
            break;

        case BLOCK_PASTE:
        case BLOCK_ROTATE:
        case BLOCK_MIRROR_X:
        case BLOCK_MIRROR_Y:
        case BLOCK_FLIP:
            wxFAIL; // should not happen
            break;

        case BLOCK_ZOOM:     // Window Zoom
            Window_Zoom( *block );
            break;

        case BLOCK_ABORT:
            break;

        case BLOCK_SELECT_ITEMS_ONLY:
            break;

        case BLOCK_PRESELECT_MOVE:          // not used in LibEdit
        case BLOCK_DUPLICATE_AND_INCREMENT: // not used in Eeschema
        case BLOCK_MOVE_EXACT:              // not used in Eeschema
            break;
        }
    }

    if( block->GetCommand() == BLOCK_ABORT )
    {
        GetScreen()->ClearDrawingState();
    }

    if( !nextCmd )
    {
        if( block->GetCommand() != BLOCK_SELECT_ITEMS_ONLY && GetCurPart() )
            BlockClearSelectedItems( GetCurPart(), block );

        GetScreen()->ClearBlockCommand();
        GetScreen()->SetCurItem( NULL );
        m_canvas->EndMouseCapture( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString,
                                   false );
    }

    view->ShowSelectionArea( false );
    view->ShowPreview( nextCmd );

    return nextCmd;
}


void LIB_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;
    wxPoint pt;

    if( !m_canvas->IsMouseCaptured() )
    {
        DisplayError( this, wxT( "HandleBlockPLace : m_mouseCaptureCallback = NULL" ) );
    }

    block->SetState( STATE_BLOCK_STOP );

    switch( block->GetCommand() )
    {
    case  BLOCK_IDLE:
        break;

    case BLOCK_DRAG:                // Drag
    case BLOCK_DRAG_ITEM:
    case BLOCK_MOVE:                // Move
    case BLOCK_DUPLICATE:           // Duplicate
        if( GetCurPart() && !block->AppendUndo() )
            SaveCopyInUndoList( GetCurPart() );

        pt = block->GetMoveVector();

        if( GetCurPart() )
            BlockMoveSelectedItems( pt, GetCurPart(), block );

        block->ClearItemsList();
        break;

    case BLOCK_PASTE:       // Paste (recopy the last block saved)
        if( GetCurPart() )
            SaveCopyInUndoList( GetCurPart() );

        pt = block->GetMoveVector();

        pasteClipboard( pt );

        block->ClearItemsList();
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_COPY:
    case BLOCK_ABORT:
    default:
        break;
    }

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();

    block->SetState( STATE_NO_BLOCK );
    block->SetCommand( BLOCK_IDLE );
    GetScreen()->SetCurItem( NULL );
    m_canvas->EndMouseCapture( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString, false );

    GetCanvas()->GetView()->ClearPreview();
    GetCanvas()->GetView()->ClearHiddenFlags();
}


void LIB_EDIT_FRAME::InitBlockPasteInfos()
{
    BLOCK_SELECTOR& block = GetScreen()->m_BlockLocate;

    // Copy the clipboard contents to the screen block selector
    // (only the copy, the new instances will be appended to the part once the items are placed)
    block.GetItems().CopyList( m_clipboard.GetItems() );

    // Set block items to the current unit & DeMorgan variant
    for( size_t i = 0; i < m_clipboard.GetItems().GetCount(); ++i )
    {
        LIB_ITEM* item = dynamic_cast<LIB_ITEM*>( m_clipboard.GetItem( i ) );

        if( item )
        {
            item->SetUnit( m_unit );
            item->SetConvert( m_convert );
        }
    }

    // Set the paste reference point
    block.SetLastCursorPosition( m_clipboard.GetLastCursorPosition() );
    m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
}


void LIB_EDIT_FRAME::copySelectedItems()
{
    LIB_PART* part = GetCurPart();

    if( !part )
        return;

    m_clipboard.ClearListAndDeleteItems();   // delete previous saved list, if exists
    m_clipboard.SetLastCursorPosition( GetScreen()->m_BlockLocate.GetEnd() );    // store the reference point

    for( LIB_ITEM& item : part->GetDrawItems() )
    {
        // We *do not* copy fields because they are unique for the whole component
        // so skip them (do not duplicate) if they are flagged selected.
        if( item.Type() == LIB_FIELD_T )
            item.ClearFlags( SELECTED );

        if( !item.IsSelected() )
            continue;

        // Do not clear the 'selected' flag. It is required to have items drawn when they are pasted.
        LIB_ITEM* copy = (LIB_ITEM*) item.Clone();
        copy->SetFlags( copy->GetFlags() | UR_TRANSIENT );
        ITEM_PICKER picker( copy, UR_NEW );
        m_clipboard.PushItem( picker );
    }
}


void LIB_EDIT_FRAME::pasteClipboard( const wxPoint& aOffset )
{
    LIB_PART* part = GetCurPart();

    if( !part || m_clipboard.GetCount() == 0 )
        return;

    for( unsigned int i = 0; i < m_clipboard.GetCount(); i++ )
    {
        // Append a copy to the current part, so the clipboard buffer might be pasted multiple times
        LIB_ITEM* item = (LIB_ITEM*) m_clipboard.GetItem( i )->Clone();
        item->SetParent( part );
        item->SetSelected();
        item->SetUnit( GetUnit() );
        part->AddDrawItem( item );
    }

    BlockMoveSelectedItems( aOffset, GetCurPart(), &GetScreen()->m_BlockLocate );
    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


/*
 * Traces the outline of the search block structures
 * The entire block follows the cursor
 */
void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                              bool aErase )
{
    SCH_DRAW_PANEL*    panel =static_cast<SCH_DRAW_PANEL*>( aPanel );
    LIB_EDIT_FRAME*    frame = (LIB_EDIT_FRAME*) aPanel->GetParent();
    KIGFX::SCH_VIEW*   view = panel->GetView();
    KIGFX::VIEW_GROUP* preview = view->GetPreview();

    BASE_SCREEN*       screen = aPanel->GetScreen();
    BLOCK_SELECTOR*    block = &screen->m_BlockLocate;
    LIB_PART*          component = frame->GetCurPart();

    if( component == NULL )
        return;

    block->SetMoveVector( frame->GetCrossHairPosition( true ) - block->GetLastCursorPosition() );

    preview->Clear();
    view->SetVisible( preview, true );

    for( unsigned ii = 0; ii < block->GetCount(); ii++ )
    {
        LIB_ITEM* libItem = (LIB_ITEM*) block->GetItem( ii );
        LIB_ITEM* copy = static_cast<LIB_ITEM*>( libItem->Clone() );

        copy->Move( copy->GetPosition() + block->GetMoveVector() );
        copy->SetFlags( IS_MOVED );
        preview->Add( copy );

        view->Hide( libItem );
    }

    view->Update( preview );
}
