/****************************************************/
/*  BLOCK.CPP                                       */
/****************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"

#include "general.h"
#include "class_library.h"
#include "lib_pin.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_text.h"
#include "sch_component.h"
#include "sch_sheet.h"

#include <boost/foreach.hpp>


// Imported functions:
extern void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen );
extern void MoveItemsInList( PICKED_ITEMS_LIST& aItemsList, const wxPoint aMoveVector );
extern void RotateListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& Center );
extern void Mirror_X_ListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& aMirrorPoint );
extern void MirrorListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& Center );
extern void DeleteItemsInList( EDA_DRAW_PANEL* panel, PICKED_ITEMS_LIST& aItemsList );
extern void DuplicateItemsInList( SCH_SCREEN*        screen,
                                  PICKED_ITEMS_LIST& aItemsList,
                                  const wxPoint      aMoveVector );

static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase );


/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
int SCH_EDIT_FRAME::ReturnBlockCommand( int key )
{
    int cmd;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_ALT:
    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
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


/* Init the parameters used by the block paste command
 */
void SCH_EDIT_FRAME::InitBlockPasteInfos()
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    block->m_ItemsSelection.CopyList( m_blockItems.m_ItemsSelection );
    DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
}


/* Routine to handle the BLOCK PLACE command
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copy & paste
 */
void SCH_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    bool            err   = false;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = true;
        DisplayError( this, wxT( "HandleBlockPLace() : ManageCurseur = NULL" ) );
    }

    if( block->GetCount() == 0 )
    {
        wxString msg;
        err = true;
        msg.Printf( wxT( "HandleBlockPLace() error : no items to place (cmd %d, state %d)" ),
                    block->m_Command, block->m_State );
        DisplayError( this, msg );
    }

    block->m_State = STATE_BLOCK_STOP;

    switch( block->m_Command )
    {
    case BLOCK_IDLE:
        err = true;
        break;

    case BLOCK_ROTATE:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        SaveCopyInUndoList( block->m_ItemsSelection, UR_MOVED, block->m_MoveVector );
        MoveItemsInList( block->m_ItemsSelection, block->m_MoveVector );
        block->ClearItemsList();
        break;

    case BLOCK_COPY:                /* Copy */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        DuplicateItemsInList( GetScreen(), block->m_ItemsSelection, block->m_MoveVector );

        SaveCopyInUndoList( block->m_ItemsSelection,
                            ( block->m_Command == BLOCK_PRESELECT_MOVE ) ? UR_CHANGED : UR_NEW );

        block->ClearItemsList();
        break;

    case BLOCK_PASTE:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        PasteListOfItems( DC );
        block->ClearItemsList();
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd()
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_FLIP:
    case BLOCK_ABORT:
    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    OnModify();

    // clear struct.m_Flags.
    GetScreen()->ClearDrawingState();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    block->m_Flags   = 0;
    block->m_State   = STATE_NO_BLOCK;
    block->m_Command = BLOCK_IDLE;
    GetScreen()->SetCurItem( NULL );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );

    if( block->GetCount() )
    {
        DisplayError( this, wxT( "HandleBlockPLace() error: some items left in buffer" ) );
        block->ClearItemsList();
    }

    SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
    DrawPanel->Refresh();
}


/**
 * Function HandleBlockEnd( )
 * Handle the "end"  of a block command,
 * i.e. is called at the end of the definition of the area of a block.
 * depending on the current block command, this command is executed
 * or parameters are initialized to prepare a call to HandleBlockPlace
 * in GetScreen()->m_BlockLocate
 * @return false if no item selected, or command finished,
 * true if some items found and HandleBlockPlace must be called later
 */
bool SCH_EDIT_FRAME::HandleBlockEnd( wxDC* DC )
{
    bool            nextcmd = false;
    bool            zoom_command = false;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( block->GetCount() )
    {
        BlockState   state   = block->m_State;
        CmdBlockType command = block->m_Command;

        if( DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );

        block->m_State   = state;
        block->m_Command = command;
        DrawPanel->ManageCurseur = DrawAndSizingBlockOutlines;
        DrawPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
        GetScreen()->m_Curseur = block->GetEnd();

        if( block->m_Command != BLOCK_ABORT )
            DrawPanel->MouseToCursorSchema();
    }

    if( DrawPanel->ManageCurseur != NULL )
    {
        switch( block->m_Command )
        {
        case BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace()" ) );
            break;

        case BLOCK_DRAG:    /* Drag */
            GetScreen()->BreakSegmentsOnJunctions();
            // fall through
        case BLOCK_ROTATE:
        case BLOCK_MIRROR_X:
        case BLOCK_MIRROR_Y:
        case BLOCK_MOVE:    /* Move */
        case BLOCK_COPY:    /* Copy */
            GetScreen()->UpdatePickList();
            // fall through
        case BLOCK_PRESELECT_MOVE: /* Move with preselection list*/
            if( block->GetCount() )
            {
                nextcmd = true;
                GetScreen()->SelectBlockItems();
                DrawPanel->ManageCurseur( DrawPanel, DC, false );
                DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
                DrawPanel->ManageCurseur( DrawPanel, DC, false );
                block->m_State = STATE_BLOCK_MOVE;
            }
            else
            {
                DrawPanel->ManageCurseur( DrawPanel, DC, false );
                DrawPanel->ManageCurseur = NULL;
                DrawPanel->ForceCloseManageCurseur = NULL;
            }
            break;

        case BLOCK_DELETE: /* Delete */
            GetScreen()->UpdatePickList();
            DrawAndSizingBlockOutlines( DrawPanel, DC, false );

            if( block->GetCount() )
            {
                DeleteItemsInList( DrawPanel, block->m_ItemsSelection );
                OnModify();
            }

            block->ClearItemsList();
            GetScreen()->TestDanglingEnds( DrawPanel, DC );
            DrawPanel->Refresh();
            break;

        case BLOCK_SAVE:  /* Save */
            GetScreen()->UpdatePickList();
            DrawAndSizingBlockOutlines( DrawPanel, DC, false );

            if( block->GetCount() )
            {
                wxPoint move_vector = -GetScreen()->m_BlockLocate.m_BlockLastCursorPosition;
                copyBlockItems( block->m_ItemsSelection );
                MoveItemsInList( m_blockItems.m_ItemsSelection, move_vector );
             }

            block->ClearItemsList();
            break;

        case BLOCK_PASTE:
            block->m_State = STATE_BLOCK_MOVE;
            break;

        case BLOCK_FLIP: /* pcbnew only! */
            break;


        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = true;
            break;

        case BLOCK_SELECT_ITEMS_ONLY:   /* Not used */
        case BLOCK_ABORT:               /* not executed here */
            break;
        }
    }

    if( block->m_Command  == BLOCK_ABORT )
    {
        GetScreen()->ClearDrawingState();
    }

    if( ! nextcmd )
    {
        block->m_Flags   = 0;
        block->m_State   = STATE_NO_BLOCK;
        block->m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return nextcmd ;
}


/* Manage end block command from context menu.
 * Can be called only :
 *      after HandleBlockEnd
 *      and if the current command is block move.
 * Execute a command other than block move from the current block move selected items list.
 * Due to (minor) problems in undo/redo or/and display block,
 * a mirror/rotate command is immediately executed and multiple block commands
 * are not allowed (multiple commands are tricky to undo/redo in one time)
 */
void SCH_EDIT_FRAME::HandleBlockEndByPopUp( int Command, wxDC* DC )
{
    bool blockCmdFinished = true;   /* set to false for block command which
                                     * have a next step
                                     * and true if the block command is finished here
                                     */
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    // can convert only a block move command to an other command
    if( block->m_Command != BLOCK_MOVE )
        return;

    // Useless if the new command is block move because we are already in block move.
    if( Command == BLOCK_MOVE )
        return;

    block->m_Command = (CmdBlockType) Command;
    block->SetMessageBlock( this );

    switch( block->m_Command )
    {
    case BLOCK_COPY:     /* move to copy */
        block->m_State = STATE_BLOCK_MOVE;
        blockCmdFinished = false;
        break;

    case BLOCK_DRAG:     /* move to Drag */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        // Clear list of items to move, and rebuild it with items to drag:
        block->ClearItemsList();

        GetScreen()->BreakSegmentsOnJunctions();
        GetScreen()->UpdatePickList();

        if( block->GetCount() )
        {
            blockCmdFinished = false;
            GetScreen()->SelectBlockItems();

            if( DrawPanel->ManageCurseur )
                DrawPanel->ManageCurseur( DrawPanel, DC, false );

            block->m_State = STATE_BLOCK_MOVE;
        }
        break;

    case BLOCK_DELETE:     /* move to Delete */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        if( block->GetCount() )
        {
            DeleteItemsInList( DrawPanel, block->m_ItemsSelection );
            OnModify();
        }

        GetScreen()->TestDanglingEnds( DrawPanel, DC );
        DrawPanel->Refresh();
        break;

    case BLOCK_SAVE:     /* Save list in paste buffer*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        if( block->GetCount() )
        {
            wxPoint move_vector = -GetScreen()->m_BlockLocate.m_BlockLastCursorPosition;
            copyBlockItems( block->m_ItemsSelection );
            MoveItemsInList( m_blockItems.m_ItemsSelection, move_vector );
        }
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        DrawPanel->SetCursor( DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor );
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;


    case BLOCK_ROTATE:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        if( block->GetCount() )
        {
            /* Compute the rotation center and put it on grid */
            wxPoint rotationPoint = block->Centre();
            PutOnGrid( &rotationPoint );
            SaveCopyInUndoList( block->m_ItemsSelection, UR_ROTATED, rotationPoint );
            RotateListOfItems( block->m_ItemsSelection, rotationPoint );
            OnModify();
        }

        GetScreen()->TestDanglingEnds( DrawPanel, DC );
        DrawPanel->Refresh();
        break;

    case BLOCK_MIRROR_X:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        if( block->GetCount() )
        {
            /* Compute the mirror center and put it on grid */
            wxPoint mirrorPoint = block->Centre();
            PutOnGrid( &mirrorPoint );
            SaveCopyInUndoList( block->m_ItemsSelection, UR_MIRRORED_X, mirrorPoint );
            Mirror_X_ListOfItems( block->m_ItemsSelection, mirrorPoint );
            OnModify();
        }
        GetScreen()->TestDanglingEnds( DrawPanel, DC );
        DrawPanel->Refresh();
        break;

    case BLOCK_MIRROR_Y:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );

        if( block->GetCount() )
        {
            /* Compute the mirror center and put it on grid */
            wxPoint mirrorPoint = block->Centre();
            PutOnGrid( &mirrorPoint );
            SaveCopyInUndoList( block->m_ItemsSelection, UR_MIRRORED_Y, mirrorPoint );
            MirrorListOfItems( block->m_ItemsSelection, mirrorPoint );
            OnModify();
        }

        GetScreen()->TestDanglingEnds( DrawPanel, DC );
        DrawPanel->Refresh();
        break;

    default:
        break;
    }

    if( blockCmdFinished )
    {
        block->ClearItemsList();
        block->m_Flags   = 0;
        block->m_State   = STATE_NO_BLOCK;
        block->m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
    }
}


/* Traces the outline of the search block structures
 * The entire block follows the cursor
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase )
{
    BLOCK_SELECTOR* block = &panel->GetScreen()->m_BlockLocate;;
    BASE_SCREEN*    screen = panel->GetScreen();
    SCH_ITEM*       schitem;

    /* Erase old block contents. */
    if( erase )
    {
        block->Draw( panel, DC, block->m_MoveVector, g_XorMode, block->m_Color );

        for( unsigned ii = 0; ii < block->GetCount(); ii++ )
        {
            schitem = (SCH_ITEM*) block->m_ItemsSelection.GetPickedItem( ii );
            schitem->Draw( panel, DC, block->m_MoveVector, g_XorMode, g_GhostColor );
        }
    }

    /* Repaint new view. */
    block->m_MoveVector = screen->m_Curseur - block->m_BlockLastCursorPosition;

    block->Draw( panel, DC, block->m_MoveVector, g_XorMode, block->m_Color );

    for( unsigned ii = 0; ii < block->GetCount(); ii++ )
    {
        schitem = (SCH_ITEM*) block->m_ItemsSelection.GetPickedItem( ii );
        schitem->Draw( panel, DC, block->m_MoveVector, g_XorMode, g_GhostColor );
    }
}


void SCH_EDIT_FRAME::copyBlockItems( PICKED_ITEMS_LIST& aItemsList )
{
    m_blockItems.ClearListAndDeleteItems();   // delete previous saved list, if exists

    /* save the new list: */
    ITEM_PICKER item;

    // In list the wrapper is owner of the schematic item, we can use the UR_DELETED
    // status for the picker because pickers with this status are owner of the picked item
    // (or TODO ?: create a new status like UR_DUPLICATE)
    item.m_UndoRedoStatus = UR_DELETED;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        // Clear m_Flag member of selected items:
        aItemsList.GetPickedItem( ii )->m_Flags = 0;
        /* Make a copy of the original picked item. */
        SCH_ITEM* DrawStructCopy = DuplicateStruct( (SCH_ITEM*) aItemsList.GetPickedItem( ii ) );
        DrawStructCopy->SetParent( NULL );
        item.m_PickedItem = DrawStructCopy;
        m_blockItems.PushItem( item );
    }
}


/*****************************************************************************
* Routine to paste a structure from the m_blockItems stack.
*   This routine is the same as undelete but original list is NOT removed.
*****************************************************************************/
void SCH_EDIT_FRAME::PasteListOfItems( wxDC* DC )
{
    SCH_ITEM* Struct;

    if( m_blockItems.GetCount() == 0 )
    {
        DisplayError( this, wxT( "No struct to paste" ) );
        return;
    }

    PICKED_ITEMS_LIST picklist;

    // Creates data, and push it as new data in undo item list buffer
    ITEM_PICKER       picker( NULL, UR_NEW );

    for( unsigned ii = 0; ii < m_blockItems.GetCount(); ii++ )
    {
        Struct = DuplicateStruct( (SCH_ITEM*) m_blockItems.m_ItemsSelection.GetPickedItem( ii ) );
        picker.m_PickedItem = Struct;
        picklist.PushItem( picker );

        // Clear annotation and init new time stamp for the new components:
        if( Struct->Type() == SCH_COMPONENT_T )
        {
            ( (SCH_COMPONENT*) Struct )->m_TimeStamp = GetTimeStamp();
            ( (SCH_COMPONENT*) Struct )->ClearAnnotation( NULL );
        }
        SetSchItemParent( Struct, GetScreen() );
        Struct->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        Struct->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( Struct );
    }

    SaveCopyInUndoList( picklist, UR_NEW );

    MoveItemsInList( picklist, GetScreen()->m_BlockLocate.m_MoveVector );

    /* clear .m_Flags member for all items */
    GetScreen()->ClearDrawingState();

    OnModify();

    return;
}
