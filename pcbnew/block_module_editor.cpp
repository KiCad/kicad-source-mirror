/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file block_module_editor.cpp
 * @brief Footprint editor block handling implementation.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <block_commande.h>
#include <macros.h>

#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <pcbplot.h>
#include <trigo.h>

#include <pcbnew.h>

#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_mire.h>
#include <class_module.h>
#include <class_dimension.h>
#include <class_edge_mod.h>

#include <dialogs/dialog_move_exact.h>


#define BLOCK_COLOR BROWN

// Functions defined here, but used also in other files
// These 3 functions are used in modedit to rotate, mirror or move the
// whole footprint so they are called with force_all = true
void MirrorMarkedItems( MODULE* module, wxPoint offset, bool force_all = false );
void RotateMarkedItems( MODULE* module, wxPoint offset, bool force_all = false );
void MoveMarkedItemsExactly( MODULE* module, const wxPoint& centre,
                             const wxPoint& translation, double rotation,
                             bool force_all = false );

// Local functions:
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );
static int  MarkItemsInBloc( MODULE* module, EDA_RECT& Rect );

static void ClearMarkItems( MODULE* module );
static void CopyMarkedItems( MODULE* module, wxPoint offset, bool aIncrement );
static void MoveMarkedItems( MODULE* module, wxPoint offset );
static void DeleteMarkedItems( MODULE* module );


int FOOTPRINT_EDIT_FRAME::BlockCommand( int key )
{
    int cmd;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case - 1:
        cmd = BLOCK_PRESELECT_MOVE;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_MIRROR_Y;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_ROTATE;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


bool FOOTPRINT_EDIT_FRAME::HandleBlockEnd( wxDC* DC )
{
    int  itemsCount    = 0;
    bool nextcmd = false;
    MODULE* currentModule = GetBoard()->m_Modules;

    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        // Set the SELECTED flag of all preselected items, and clear preselect list
        ClearMarkItems( currentModule );
        PICKED_ITEMS_LIST* list = &GetScreen()->m_BlockLocate.GetItems();

        for( unsigned ii = 0, e = list->GetCount(); ii < e; ++ii )
        {
            BOARD_ITEM* item = (BOARD_ITEM*) list->GetPickedItem( ii );
            item->SetFlags( SELECTED );
            ++itemsCount;
        }

        GetScreen()->m_BlockLocate.ClearItemsList();
    }

    switch( GetScreen()->m_BlockLocate.GetCommand() )
    {
    case  BLOCK_IDLE:
        DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
        break;

    case BLOCK_DRAG:                // Drag
    case BLOCK_DRAG_ITEM:           // Drag a given item (not used here)
    case BLOCK_MOVE:                // Move
    case BLOCK_COPY:                // Copy
    case BLOCK_COPY_AND_INCREMENT:  // Specific to duplicate with increment command

        // Find selected items if we didn't already set them manually
        if( itemsCount == 0 )
            itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
        {
            nextcmd = true;

            if( m_canvas->IsMouseCaptured() )
            {
                m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
                m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
                m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
            }

            GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_MOVE );
            m_canvas->Refresh( true );
        }

        break;

    case BLOCK_MOVE_EXACT:
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
        {
            wxPoint translation;
            double rotation = 0;

            DIALOG_MOVE_EXACT dialog( this, translation, rotation  );
            int ret = dialog.ShowModal();

            if( ret == wxID_OK )
            {
                SaveCopyInUndoList( currentModule, UR_MODEDIT );
                const wxPoint blockCentre = GetScreen()->m_BlockLocate.Centre();
                MoveMarkedItemsExactly( currentModule, blockCentre, translation, rotation );
            }
        }
        break;

    case BLOCK_PRESELECT_MOVE:     // Move with preselection list
        nextcmd = true;
        m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
        GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_MOVE );
        break;

    case BLOCK_DELETE:     // Delete
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );

        DeleteMarkedItems( currentModule );
        break;

    case BLOCK_SAVE:     // Save
    case BLOCK_PASTE:
        break;

    case BLOCK_ROTATE:
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );

        RotateMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_FLIP:     // mirror
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );

        MirrorMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:     // Window Zoom
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;

    case BLOCK_ABORT:
        break;

    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    if( !nextcmd )
    {
        if( GetScreen()->m_BlockLocate.GetCommand() != BLOCK_SELECT_ITEMS_ONLY )
        {
            ClearMarkItems( currentModule );
        }

        GetScreen()->ClearBlockCommand();
        SetCurItem( NULL );
        m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString,
                                   false );
        m_canvas->Refresh( true );
    }

    return nextcmd;
}


void FOOTPRINT_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    MODULE* currentModule = GetBoard()->m_Modules;

    if( !m_canvas->IsMouseCaptured() )
    {
        DisplayError( this, wxT( "HandleBlockPLace : m_mouseCaptureCallback = NULL" ) );
    }

    GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_STOP );

    const BLOCK_COMMAND_T command = GetScreen()->m_BlockLocate.GetCommand();

    switch( command )
    {
    case  BLOCK_IDLE:
        break;

    case BLOCK_DRAG:                // Drag
    case BLOCK_MOVE:                // Move
    case BLOCK_PRESELECT_MOVE:      // Move with preselection list
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MoveMarkedItems( currentModule, GetScreen()->m_BlockLocate.GetMoveVector() );
        m_canvas->Refresh( true );
        break;

    case BLOCK_COPY:                // Copy
    case BLOCK_COPY_AND_INCREMENT:  // Copy and increment references
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        CopyMarkedItems( currentModule, GetScreen()->m_BlockLocate.GetMoveVector(),
                         command == BLOCK_COPY_AND_INCREMENT );
        break;

    case BLOCK_PASTE:     // Paste
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_FLIP:      // Mirror by popup menu, from block move
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MirrorMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ROTATE:
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        RotateMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    default:
        break;
    }

    OnModify();

    GetScreen()->m_BlockLocate.SetState( STATE_NO_BLOCK );
    GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
    SetCurItem( NULL );
    m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString, false );
    m_canvas->Refresh( true );
}


/* Traces the outline of the search block structures
 * The entire block follows the cursor
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase )
{
    BASE_SCREEN*            screen = aPanel->GetScreen();
    FOOTPRINT_EDIT_FRAME*   moduleEditFrame = static_cast<FOOTPRINT_EDIT_FRAME*>( aPanel->GetParent() );

    wxASSERT( moduleEditFrame );
    MODULE* currentModule = moduleEditFrame->GetBoard()->m_Modules;

    BLOCK_SELECTOR* block = &screen->m_BlockLocate;
    GRSetDrawMode( aDC, g_XorMode );

    if( aErase )
    {
        block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );

        if( currentModule )
        {
            wxPoint     move_offset = -block->GetMoveVector();
            BOARD_ITEM* item = currentModule->GraphicalItems();

            for( ; item != NULL; item = item->Next() )
            {
                if( !item->IsSelected() )
                    continue;

                switch( item->Type() )
                {
                case PCB_MODULE_TEXT_T:
                case PCB_MODULE_EDGE_T:
                    item->Draw( aPanel, aDC, g_XorMode, move_offset );
                    break;

                default:
                    break;
                }
            }

            D_PAD* pad = currentModule->Pads();

            for( ; pad != NULL; pad = pad->Next() )
            {
                if( !pad->IsSelected() )
                    continue;

                pad->Draw( aPanel, aDC, g_XorMode, move_offset );
            }
        }
    }

    // Repaint new view.
    block->SetMoveVector( moduleEditFrame->GetCrossHairPosition() - block->GetLastCursorPosition() );

    block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );

    if( currentModule )
    {
        BOARD_ITEM* item = currentModule->GraphicalItems();
        wxPoint     move_offset = - block->GetMoveVector();

        for( ; item != NULL; item = item->Next() )
        {
            if( !item->IsSelected() )
                continue;

            switch( item->Type() )
            {
            case PCB_MODULE_TEXT_T:
            case PCB_MODULE_EDGE_T:
                item->Draw( aPanel, aDC, g_XorMode, move_offset );
                break;

            default:
                break;
            }
        }

        D_PAD* pad = currentModule->Pads();

        for( ; pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsSelected() )
                continue;

            pad->Draw( aPanel, aDC, g_XorMode, move_offset );
        }
    }
}


/* Copy marked items, at new position = old position + offset
 */
void CopyMarkedItems( MODULE* module, wxPoint offset, bool aIncrement )
{
    if( module == NULL )
        return;

    // Reference and value cannot be copied, they are unique.
    // Ensure they are not selected
    module->Reference().ClearFlags();
    module->Value().ClearFlags();

    for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
    {
        if( !pad->IsSelected() )
            continue;

        pad->ClearFlags( SELECTED );
        D_PAD* NewPad = new D_PAD( *pad );
        NewPad->SetParent( module );
        NewPad->SetFlags( SELECTED );
        module->Pads().PushFront( NewPad );

        if( aIncrement )
            NewPad->IncrementItemReference();
    }

    BOARD_ITEM* newItem;

    for( BOARD_ITEM* item = module->GraphicalItems();  item;  item = item->Next() )
    {
        if( !item->IsSelected() )
            continue;

        item->ClearFlags( SELECTED );

        newItem = (BOARD_ITEM*)item->Clone();
        newItem->SetParent( module );
        newItem->SetFlags( SELECTED );
        module->GraphicalItems().PushFront( newItem );

        if( aIncrement )
            newItem->IncrementItemReference();
    }

    MoveMarkedItems( module, offset );
}


/* Move marked items, at new position = old position + offset
 */
void MoveMarkedItems( MODULE* module, wxPoint offset )
{
    EDA_ITEM* item;

    if( module == NULL )
        return;

    if( module->Reference().IsSelected() )
        module->Reference().Move( offset );

    if( module->Value().IsSelected() )
        module->Value().Move( offset );

    D_PAD* pad = module->Pads();

    for( ; pad != NULL; pad = pad->Next() )
    {
        if( !pad->IsSelected() )
            continue;

        pad->SetPosition( pad->GetPosition() + offset );
        pad->SetPos0( pad->GetPos0() + offset );
    }

    item = module->GraphicalItems();

    for( ; item != NULL; item = item->Next() )
    {
        if( !item->IsSelected() )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            static_cast<TEXTE_MODULE*>( item )->Move( offset );
            break;

        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* em = (EDGE_MODULE*) item;
            em->SetStart( em->GetStart() + offset );
            em->SetEnd( em->GetEnd() + offset );
            em->SetStart0( em->GetStart0() + offset );
            em->SetEnd0( em->GetEnd0() + offset );
        }
        break;

        default:
            ;
        }
    }

    ClearMarkItems( module );
}


/* Delete marked items
 */
void DeleteMarkedItems( MODULE* module )
{
    BOARD_ITEM* item;
    BOARD_ITEM* next_item;
    D_PAD*      pad;
    D_PAD*      next_pad;

    if( module == NULL )
        return;

    pad = module->Pads();

    for( ; pad != NULL; pad = next_pad )
    {
        next_pad = pad->Next();

        if( !pad->IsSelected() )
            continue;

        pad->DeleteStructure();
    }

    item = module->GraphicalItems();

    for( ; item != NULL; item = next_item )
    {
        next_item = item->Next();

        if( !item->IsSelected() )
            continue;

        item->DeleteStructure();
    }

    // Ref and value can be flagged, but cannot be deleted
    ClearMarkItems( module );
}


/** Mirror marked items, refer to a Vertical axis at position offset
 * Note: because this function is used in global transform,
 * if force_all is true, all items will be mirrored
 */
void MirrorMarkedItems( MODULE* module, wxPoint offset, bool force_all )
{
#define SETMIRROR( z ) (z) -= offset.x; (z) = -(z); (z) += offset.x;
    wxPoint     tmp;
    wxSize      tmpz;

    if( module == NULL )
        return;

    if( module->Reference().IsSelected() || force_all )
        module->Reference().Mirror( offset, false );

    if( module->Value().IsSelected() || force_all )
        module->Value().Mirror( offset, false );

    for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
    {
        // Skip pads not selected, i.e. not inside the block to mirror:
        if( !pad->IsSelected() && !force_all )
            continue;

        tmp = pad->GetPosition();
        SETMIRROR( tmp.x );
        pad->SetPosition( tmp );

        pad->SetX0( pad->GetPosition().x );

        tmp = pad->GetOffset();
        tmp.x = -tmp.x;
        pad->SetOffset( tmp );

        tmpz = pad->GetDelta();
        tmpz.x = -tmpz.x;
        pad->SetDelta( tmpz );

        pad->SetOrientation( 1800 - pad->GetOrientation() );
    }

    for( EDA_ITEM* item = module->GraphicalItems();  item;  item = item->Next() )
    {
        // Skip items not selected, i.e. not inside the block to mirror:
        if( !item->IsSelected() && !force_all )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            ((EDGE_MODULE*) item)->Mirror( offset, false );
            break;

        case PCB_MODULE_TEXT_T:
            static_cast<TEXTE_MODULE*>( item )->Mirror( offset, false );
            break;

        default:
            break;
        }
    }

    ClearMarkItems( module );
}


/** Rotate marked items, refer to a rotation point at position offset
 * Note: because this function is used in global transform,
 * if force_all is true, all items will be rotated
 */
void RotateMarkedItems( MODULE* module, wxPoint offset, bool force_all )
{
#define ROTATE( z ) RotatePoint( (&z), offset, 900 )

    if( module == NULL )
        return;

    if( module->Reference().IsSelected() || force_all )
        module->Reference().Rotate( offset, 900 );

    if( module->Value().IsSelected() || force_all )
        module->Value().Rotate( offset, 900 );

    for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
    {
        if( !pad->IsSelected() && !force_all )
            continue;

        wxPoint pos = pad->GetPos0();
        ROTATE( pos );
        pad->SetPos0( pos );
        pad->SetOrientation( pad->GetOrientation() + 900 );

        pad->SetDrawCoord();
    }

    for( EDA_ITEM* item = module->GraphicalItems();  item;  item = item->Next() )
    {
        if( !item->IsSelected() && !force_all )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            ((EDGE_MODULE*) item)->Rotate( offset, 900 );
            break;

        case PCB_MODULE_TEXT_T:
            static_cast<TEXTE_MODULE*>( item )->Rotate( offset, 900 );
            break;

        default:
            break;
        }
    }

    ClearMarkItems( module );
}


void ClearMarkItems( MODULE* module )
{
    if( module == NULL )
        return;

    module->Reference().ClearFlags();
    module->Value().ClearFlags();

    EDA_ITEM* item = module->GraphicalItems();

    for( ; item != NULL; item = item->Next() )
    {
        item->ClearFlags();
    }

    item = module->Pads();

    for( ; item != NULL; item = item->Next() )
    {
        item->ClearFlags();
    }
}


void MoveMarkedItemsExactly( MODULE* module, const wxPoint& centre,
                             const wxPoint& translation,
                             double rotation, bool force_all )
{
    if( module == NULL )
        return;

    if( module->Reference().IsSelected() || force_all )
    {
        module->Reference().Rotate( centre, rotation );
        module->Reference().Move( translation );
    }

    if( module->Value().IsSelected() || force_all )
    {
        module->Value().Rotate( centre, rotation );
        module->Value().Move( translation );
    }

    D_PAD* pad = module->Pads();

    for( ; pad != NULL; pad = pad->Next() )
    {
        if( !pad->IsSelected() && !force_all )
            continue;

        // rotate about centre point,
        wxPoint newPos = pad->GetPosition();
        RotatePoint( &newPos, centre, rotation );

        // shift and update
        newPos += translation;
        pad->SetPosition( newPos );
        pad->SetPos0( newPos );

        // finally apply rotation to the pad itself
        pad->Rotate( newPos, rotation );
    }

    EDA_ITEM* item = module->GraphicalItems();

    for( ; item != NULL; item = item->Next() )
    {
        if( !item->IsSelected() && !force_all )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

            text->Rotate( centre, rotation );
            text->Move( translation );
            break;
        }
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* em = static_cast<EDGE_MODULE*>( item );
            em->Rotate( centre, rotation );
            em->Move( translation );
            break;
        }
        default:
            ;
        }
    }

    ClearMarkItems( module );
}


/* Mark items inside rect.
 *  Items are inside rect when an end point is inside rect
 */
int MarkItemsInBloc( MODULE* module, EDA_RECT& Rect )
{
    EDA_ITEM* item;
    int       ItemsCount = 0;
    wxPoint   pos;
    D_PAD*    pad;

    if( module == NULL )
        return 0;

    ClearMarkItems( module );   // Just in case ...

    pos = module->Reference().GetTextPosition();

    if( Rect.Contains( pos ) )
    {
        module->Reference().SetFlags( SELECTED );
        ItemsCount++;
    }

    pos = module->Value().GetTextPosition();

    if( Rect.Contains( pos ) )
    {
        module->Value().SetFlags( SELECTED );
        ItemsCount++;
    }

    pad = module->Pads();

    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->ClearFlags( SELECTED );
        pos = pad->GetPosition();

        if( Rect.Contains( pos ) )
        {
            pad->SetFlags( SELECTED );
            ItemsCount++;
        }
    }

    item = module->GraphicalItems();

    for( ; item != NULL; item = item->Next() )
    {
        item->ClearFlags( SELECTED );

        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            if( ((EDGE_MODULE*)item )->HitTest( Rect ) )
            {
                item->SetFlags( SELECTED );
                ItemsCount++;
            }

            break;

        case PCB_MODULE_TEXT_T:
            pos = static_cast<TEXTE_MODULE*>( item )->GetTextPosition();

            if( Rect.Contains( pos ) )
            {
                item->SetFlags( SELECTED );
                ItemsCount++;
            }

            break;

        default:
            break;
        }
    }

    return ItemsCount;
}
