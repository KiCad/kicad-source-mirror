/*************/
/* block.cpp */
/*************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "trigo.h"

#include "dialog_block_options_base.h"

#include "protos.h"

#define BLOCK_OUTLINE_COLOR YELLOW

/**
 * Function drawPickedItems
 * draws items currently selected in a block
 * @param aPanel = Current draw panel
 * @param aDC = Current device context
 * @param aOffset = Drawing offset
 **/
static void drawPickedItems( EDA_DRAW_PANEL* aPanel, wxDC* aDC, wxPoint aOffset );

/**
 * Function drawMovingBlock
 * handles drawing of a moving block
 * @param aPanel = Current draw panel
 * @param aDC = Current device context
 * @param aErase = Erase block at current position
 **/
static void drawMovingBlock( EDA_DRAW_PANEL* aPanel, wxDC* aDC, bool aErase );


static bool blockIncludeModules     = true;
static bool blockIncludeLockedModules = true;
static bool blockIncludeTracks      = true;
static bool blockIncludeZones       = true;
static bool blockIncludeItemsOnTechLayers  = true;
static bool blockIncludeBoardOutlineLayer = true;
static bool blockIncludePcbTexts   = true;
static bool blockDrawItems = true;

/************************************/
/* class DIALOG_BLOCK_OPTIONS */
/************************************/

class DIALOG_BLOCK_OPTIONS : public DIALOG_BLOCK_OPTIONS_BASE
{
private:
    WinEDA_BasePcbFrame* m_Parent;

public:

    DIALOG_BLOCK_OPTIONS( WinEDA_BasePcbFrame* parent,
                          const wxString&      title );
    ~DIALOG_BLOCK_OPTIONS()
    {
    }


private:
    void ExecuteCommand( wxCommandEvent& event );
    void OnCancel( wxCommandEvent& event ) { EndModal( -1 ); }
    void checkBoxClicked( wxCommandEvent& aEvent )
    {
        if( m_Include_Modules->GetValue() )
            m_IncludeLockedModules->Enable();
        else
            m_IncludeLockedModules->Disable();
    }
};


static bool InstallBlockCmdFrame( WinEDA_BasePcbFrame* parent, const wxString& title )
{
    int     nocmd;
    wxPoint oldpos = parent->GetScreen()->m_Curseur;

    parent->DrawPanel->m_IgnoreMouseEvents = true;
    DIALOG_BLOCK_OPTIONS dlg( parent, title );

    nocmd = dlg.ShowModal();

    parent->GetScreen()->m_Curseur = oldpos;

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = false;

    parent->DrawPanel->SetCursor( parent->DrawPanel->m_PanelCursor =
                                      parent->DrawPanel->m_PanelDefaultCursor );

    return nocmd ? false : true;
}


DIALOG_BLOCK_OPTIONS::DIALOG_BLOCK_OPTIONS( WinEDA_BasePcbFrame* aParent,
                                            const wxString&      aTitle ) :
    DIALOG_BLOCK_OPTIONS_BASE( aParent, -1, aTitle )
{
    m_Parent = aParent;

    m_Include_Modules->SetValue( blockIncludeModules );
    m_IncludeLockedModules->SetValue( blockIncludeLockedModules );

    if( m_Include_Modules->GetValue() )
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();

    m_Include_Tracks->SetValue( blockIncludeTracks );
    m_Include_Zones->SetValue( blockIncludeZones );
    m_Include_Draw_Items->SetValue( blockIncludeItemsOnTechLayers );
    m_Include_Edges_Items->SetValue( blockIncludeBoardOutlineLayer );
    m_Include_PcbTextes->SetValue( blockIncludePcbTexts );
    m_DrawBlockItems->SetValue( blockDrawItems );
    m_sdbSizer1OK->SetDefault();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_BLOCK_OPTIONS::ExecuteCommand( wxCommandEvent& event )
{
    blockIncludeModules     = m_Include_Modules->GetValue();
    blockIncludeLockedModules = m_IncludeLockedModules->GetValue();
    blockIncludeTracks      = m_Include_Tracks->GetValue();
    blockIncludeZones       = m_Include_Zones->GetValue();
    blockIncludeItemsOnTechLayers  = m_Include_Draw_Items->GetValue();
    blockIncludeBoardOutlineLayer = m_Include_Edges_Items->GetValue();
    blockIncludePcbTexts   = m_Include_PcbTextes->GetValue();
    blockDrawItems = m_DrawBlockItems->GetValue();

    EndModal( 0 );
}


/**
 * Function ReturnBlockCommand
 * Returns the block command internat code (BLOCK_MOVE, BLOCK_COPY...)
 * corresponding to the keys pressed (ALT, SHIFT, SHIFT ALT ..) when
 * block command is started by dragging the mouse.
 * @param aKey = the key modifiers (Alt, Shift ...)
 * @return the block command id (BLOCK_MOVE, BLOCK_COPY...)
 */
int WinEDA_PcbFrame::ReturnBlockCommand( int aKey )
{
    int cmd = 0;

    switch( aKey )
    {
    default:
        cmd = aKey & 0x255;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_ROTATE;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_FLIP;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/**
 * Function HandleBlockPlace( )
 * Called after HandleBlockEnd, when a block command needs to be
 * executed after the block is moved to its new place
 * (bloc move, drag, copy .. )
 * Parameters must be initialized in GetScreen()->m_BlockLocate
 */
void WinEDA_PcbFrame::HandleBlockPlace( wxDC* DC )
{
    bool err = false;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = true;
        DisplayError( this, wxT( "Error in HandleBlockPLace : ManageCurseur = NULL" ) );
    }
    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case BLOCK_IDLE:
        err = true;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        Block_Move();
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        Block_Duplicate();
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_PASTE:
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd()
    default:
        break;
    }

    OnModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur   = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        DisplayError( this, wxT( "Error in HandleBlockPLace some items left in list" ) );
        GetScreen()->m_BlockLocate.ClearItemsList();
    }

    DisplayToolMsg( wxEmptyString );
    DrawPanel->SetCursor( DrawPanel->m_PanelCursor =
                                      DrawPanel->m_PanelDefaultCursor );
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
bool WinEDA_PcbFrame::HandleBlockEnd( wxDC* DC )
{
    bool nextcmd = false;       // Will be set to true if a block place is needed
    bool cancelCmd = false;

    // If coming here after cancel block, clean up and exit
    if( GetScreen()->m_BlockLocate.m_State == STATE_NO_BLOCK )
    {
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur   = NULL;
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        GetScreen()->m_BlockLocate.ClearItemsList();
        DisplayToolMsg( wxEmptyString );
        return false;
    }

    // Show dialog if there are no selected items and
    // we're not zooming
    if( !GetScreen()->m_BlockLocate.GetCount()
        && GetScreen()->m_BlockLocate.m_Command != BLOCK_ZOOM )
    {
        if( !InstallBlockCmdFrame( this, _( "Block Operation" ) ) )
        {
            cancelCmd = true;
            DrawPanel->ManageCurseur( DrawPanel, DC, false );   // undraw block outline
        }
        else
        {
            DrawAndSizingBlockOutlines( DrawPanel, DC, false );
            Block_SelectItems();

            // Exit if no items found
            if( !GetScreen()->m_BlockLocate.GetCount() )
                cancelCmd = true;
//            else
//                nextcmd = true;
        }
    }

    if( !cancelCmd && DrawPanel->ManageCurseur )
    {
        switch( GetScreen()->m_BlockLocate.m_Command )
        {
        case BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
            break;

        case BLOCK_DRAG:            /* Drag (not used, for future enhancements)*/
        case BLOCK_MOVE:            /* Move */
        case BLOCK_COPY:            /* Copy */
        case BLOCK_PRESELECT_MOVE:  /* Move with preselection list*/
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            nextcmd = true;
            DrawPanel->ManageCurseur = drawMovingBlock;
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
            break;

        case BLOCK_DELETE: /* Delete */
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            Block_Delete();
            break;

        case BLOCK_ROTATE: /* Rotation */
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            Block_Rotate();
            break;

        case BLOCK_FLIP: /* Flip */
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            Block_Flip();
            break;

        case BLOCK_SAVE: /* Save (not used, for future enhancements)*/
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            if( GetScreen()->m_BlockLocate.GetCount() )
            {
                // @todo (if useful)         Save_Block( );
            }
            break;

        case BLOCK_PASTE:
            break;

        case BLOCK_ZOOM: /* Window Zoom */

            // Turn off the redraw block routine now so it is not displayed
            // with one corner at the new center of the screen
            DrawPanel->ManageCurseur = NULL;
            Window_Zoom( GetScreen()->m_BlockLocate );
            break;

        default:
            break;
        }
    }

    if( ! nextcmd )
    {
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        GetScreen()->m_BlockLocate.ClearItemsList();
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        DisplayToolMsg( wxEmptyString );
    }

    return nextcmd;
}


/* Block operations: */

/*
 * Function Block_SelectItems
 * Uses GetScreen()->m_BlockLocate
 * select items within the selected block.
 * selected items are put in the pick list
 */
void WinEDA_PcbFrame::Block_SelectItems()
{
    int masque_layer;

    GetScreen()->m_BlockLocate.Normalize();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    ITEM_PICKER        picker( NULL, UR_UNSPECIFIED );

    if( blockIncludeModules )
    {
        for( MODULE* module = m_Pcb->m_Modules; module != NULL;
            module = module->Next() )
        {
            if( module->HitTest( GetScreen()->m_BlockLocate )
               && ( !module->IsLocked() || blockIncludeLockedModules ) )
            {
                picker.m_PickedItem     = module;
                picker.m_PickedItemType = module->Type();
                itemsList->PushItem( picker );
            }
        }
    }

    /* Remove tracks and vias */
    if( blockIncludeTracks )
    {
        for( TRACK* pt_segm = m_Pcb->m_Track; pt_segm != NULL;
            pt_segm = pt_segm->Next() )
        {
            if( pt_segm->HitTest( GetScreen()->m_BlockLocate ) )
            {
                /* This track is in bloc: select it */
                picker.m_PickedItem     = pt_segm;
                picker.m_PickedItemType = pt_segm->Type();
                itemsList->PushItem( picker );
            }
        }
    }

    /* Select graphic items */
    masque_layer = EDGE_LAYER;
    if( blockIncludeItemsOnTechLayers )
        masque_layer = ALL_LAYERS;

    if( !blockIncludeBoardOutlineLayer )
        masque_layer &= ~EDGE_LAYER;

    for( BOARD_ITEM* PtStruct = m_Pcb->m_Drawings; PtStruct != NULL;
        PtStruct = PtStruct->Next() )
    {
        bool select_me = false;
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        case TYPE_TEXTE:
            if( !blockIncludePcbTexts )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        case TYPE_MIRE:
            if( ( g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer ) == 0 )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        case TYPE_DIMENSION:
            if( ( g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer ) == 0 )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        default:
            break;
        }

        if( select_me )
        {
            picker.m_PickedItem     = PtStruct;
            picker.m_PickedItemType = PtStruct->Type();
            itemsList->PushItem( picker );
        }
    }

    /* Zone selection */
    if( blockIncludeZones )
    {
#if 0

        /* This section can creates problems if selected:
         * m_Pcb->m_Zone can have a *lot* of items (100 000 is easily possible)
         * so it is not selected (and TODO: will be removed, one day)
         */
        for( SEGZONE* pt_segm = m_Pcb->m_Zone; pt_segm != NULL;
            pt_segm = pt_segm->Next() )
        {
            /* Segments used in Zone filling selection */

            if( pt_segm->HitTest( GetScreen()->m_BlockLocate ) )
            {
                picker.m_PickedItem     = pt_segm;
                picker.m_PickedItemType = pt_segm->Type();
                itemsList->PushItem( picker );
            }
        }

#endif
        for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
        {
            if( m_Pcb->GetArea( ii )->HitTest( GetScreen()->m_BlockLocate ) )
            {
                BOARD_ITEM* zone_c = m_Pcb->GetArea( ii );
                picker.m_PickedItem     = zone_c;
                picker.m_PickedItemType = zone_c->Type();
                itemsList->PushItem( picker );
            }
        }
    }
}


static void drawPickedItems( EDA_DRAW_PANEL* aPanel, wxDC* aDC, wxPoint aOffset )
{
    PICKED_ITEMS_LIST*   itemsList = &aPanel->GetScreen()->m_BlockLocate.m_ItemsSelection;
    WinEDA_BasePcbFrame* frame     = (WinEDA_BasePcbFrame*) aPanel->GetParent();

    g_Offset_Module = -aOffset;
    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        switch( item->Type() )
        {
        case TYPE_MODULE:
            frame->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
            DrawModuleOutlines( aPanel, aDC, (MODULE*) item );
            break;

        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_TRACK:
        case TYPE_VIA:
        case TYPE_MIRE:
        case TYPE_DIMENSION:    // Currently markers are not affected by block commands
        case TYPE_MARKER_PCB:
            item->Draw( aPanel, aDC, GR_XOR, aOffset );
            break;

        case TYPE_ZONE_CONTAINER:
            item->Draw( aPanel, aDC, GR_XOR, aOffset );
            ((ZONE_CONTAINER*) item)->DrawFilledArea( aPanel, aDC, GR_XOR, aOffset );
            break;

        default:
            break;
        }
    }

    g_Offset_Module = wxPoint( 0, 0 );
}


static void drawMovingBlock( EDA_DRAW_PANEL* aPanel, wxDC* aDC, bool aErase )
{
    BASE_SCREEN* screen = aPanel->GetScreen();

    if( aErase )
    {
        if( screen->m_BlockLocate.m_MoveVector.x
            || screen->m_BlockLocate.m_MoveVector.y )
        {
            screen->m_BlockLocate.Draw( aPanel, aDC, screen->m_BlockLocate.m_MoveVector,
                                        GR_XOR, BLOCK_OUTLINE_COLOR );
            if( blockDrawItems )
                drawPickedItems( aPanel, aDC, screen->m_BlockLocate.m_MoveVector );
        }
    }

    if( screen->m_BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->m_BlockLocate.m_MoveVector = screen->m_Curseur -
                                             screen->m_BlockLocate.m_BlockLastCursorPosition;
    }

    if( screen->m_BlockLocate.m_MoveVector.x
        || screen->m_BlockLocate.m_MoveVector.y )
    {
        screen->m_BlockLocate.Draw( aPanel, aDC, screen->m_BlockLocate.m_MoveVector,
                                    GR_XOR, BLOCK_OUTLINE_COLOR );
        if( blockDrawItems )
            drawPickedItems( aPanel, aDC, screen->m_BlockLocate.m_MoveVector );
    }
}


/*
 * Erase selected block.
 */
void WinEDA_PcbFrame::Block_Delete()
{
    OnModify();
    SetCurItem( NULL );

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_DELETED;

    /* unlink items and clear flags */
    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        itemsList->SetPickedItemStatus( UR_DELETED, ii );
        switch( item->Type() )
        {
        case TYPE_MODULE:
        {
            MODULE* module = (MODULE*) item;
            module->m_Flags = 0;
            module->UnLink();
            m_Pcb->m_Status_Pcb = 0;
        }
        break;

        case TYPE_ZONE_CONTAINER:   // a zone area
            m_Pcb->Remove( item );
            break;

        case TYPE_DRAWSEGMENT:      // a segment not on copper layers
        case TYPE_TEXTE:            // a text on a layer
        case TYPE_TRACK:            // a track segment (segment on a copper layer)
        case TYPE_VIA:              // a via (like atrack segment on a copper layer)
        case TYPE_DIMENSION:        // a dimension (graphic item)
        case TYPE_MIRE:             // a target (graphic item)
            item->UnLink();
            break;

        // These items are deleted, but not put in undo list
        case TYPE_MARKER_PCB:               // a marker used to show something
        case TYPE_ZONE:                     // SEG_ZONE items are now deprecated
            item->UnLink();
            itemsList->RemovePicker( ii );
            ii--;
            item->DeleteStructure();
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Delete( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_DELETED );

    Compile_Ratsnest( NULL, true );
    DrawPanel->Refresh( true );
}


/*
 * Function Block_Rotate
 * Rotate all items within the selected block.
 * The rotation center is the center of the block
 */
void WinEDA_PcbFrame::Block_Rotate()
{
    wxPoint oldpos;
    wxPoint centre;         // rotation cent-re for the rotation transform
    int     rotAngle = 900; // rotation angle in 0.1 deg.

    oldpos = GetScreen()->m_Curseur;
    centre = GetScreen()->m_BlockLocate.Centre();

    OnModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_ROTATED;

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        wxASSERT( item );
        itemsList->SetPickedItemStatus( UR_ROTATED, ii );
        item->Rotate( centre, rotAngle );
        switch( item->Type() )
        {
        case TYPE_MODULE:
            ( (MODULE*) item )->m_Flags = 0;
            m_Pcb->m_Status_Pcb = 0;
            break;

        /* Move and rotate the track segments */
        case TYPE_TRACK:        // a track segment (segment on a copper layer)
        case TYPE_VIA:          // a via (like atrack segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
            break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_DIMENSION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:         // SEG_ZONE items are now deprecated
            itemsList->RemovePicker( ii );
            ii--;
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Rotate( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_ROTATED, centre );

    Compile_Ratsnest( NULL, true );
    DrawPanel->Refresh( true );
}


/**
 * Function Block_Flip
 * flips items within the selected block.
 * The flip center is the center of the block
 */
void WinEDA_PcbFrame::Block_Flip()
{
#define INVERT( pos ) (pos) = center.y - ( (pos) - center.y )
    wxPoint memo;
    wxPoint center; /* Position of the axis for inversion of all elements */

    OnModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_FLIPPED;

    memo = GetScreen()->m_Curseur;

    center = GetScreen()->m_BlockLocate.Centre();

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        wxASSERT( item );
        itemsList->SetPickedItemStatus( UR_FLIPPED, ii );
        item->Flip( center );
        switch( item->Type() )
        {
        case TYPE_MODULE:
            item->m_Flags = 0;
            m_Pcb->m_Status_Pcb = 0;
            break;

        /* Move and rotate the track segments */
        case TYPE_TRACK:        // a track segment (segment on a copper layer)
        case TYPE_VIA:          // a via (like atrack segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
            break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_DIMENSION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:         // SEG_ZONE items are now deprecated
            itemsList->RemovePicker( ii );
            ii--;
            break;


        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Flip( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_FLIPPED, center );
    Compile_Ratsnest( NULL, true );
    DrawPanel->Refresh( true );
}


/*
 * Function Block_Move
 * moves all tracks and segments within the selected block.
 * New location is determined by the current offset from the selected block's
 * original location.
 */
void WinEDA_PcbFrame::Block_Move()
{
    OnModify();

    wxPoint            MoveVector = GetScreen()->m_BlockLocate.m_MoveVector;

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_MOVED;

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        itemsList->SetPickedItemStatus( UR_MOVED, ii );
        item->Move( MoveVector );

        switch( item->Type() )
        {
        case TYPE_MODULE:
            m_Pcb->m_Status_Pcb = 0;
            item->m_Flags = 0;
            break;

        /* Move track segments */
        case TYPE_TRACK:        // a track segment (segment on a copper layer)
        case TYPE_VIA:          // a via (like a track segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
            break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_DIMENSION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:        // SEG_ZONE items are now deprecated
            itemsList->RemovePicker( ii );
            ii--;
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Move( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_MOVED, MoveVector );

    Compile_Ratsnest( NULL, true );
    DrawPanel->Refresh( true );
}


/*
 * Function Block_Duplicate
 * duplicates all items within the selected block.
 * New location is determined by the current offset from the selected block's
 * original location.
 */
void WinEDA_PcbFrame::Block_Duplicate()
{
    wxPoint MoveVector = GetScreen()->m_BlockLocate.m_MoveVector;

    OnModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;

    PICKED_ITEMS_LIST  newList;
    newList.m_Status = UR_NEW;

    ITEM_PICKER        picker( NULL, UR_NEW );
    BOARD_ITEM*        newitem;

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        newitem = NULL;
        switch( item->Type() )
        {
        case TYPE_MODULE:
            {
                MODULE* module = (MODULE*) item;
                MODULE* new_module;
                m_Pcb->m_Status_Pcb = 0;
                module->m_Flags     = 0;
                newitem = new_module = new MODULE( m_Pcb );
                new_module->Copy( module );
                new_module->m_TimeStamp = GetTimeStamp();
                m_Pcb->m_Modules.PushFront( new_module );
            }
            break;

        case TYPE_TRACK:
        case TYPE_VIA:
            {
                TRACK* track = (TRACK*) item;
                m_Pcb->m_Status_Pcb = 0;
                TRACK* new_track = track->Copy();
                newitem = new_track;
                m_Pcb->m_Track.PushFront( new_track );
            }
            break;

        case TYPE_ZONE:                  // SEG_ZONE items are now deprecated
            break;

        case TYPE_ZONE_CONTAINER:
            {
                ZONE_CONTAINER* new_zone =
                    new ZONE_CONTAINER( (BOARD*) item->GetParent() );
                new_zone->Copy( (ZONE_CONTAINER*) item );
                new_zone->m_TimeStamp = GetTimeStamp();
                newitem = new_zone;
                m_Pcb->Add( new_zone );
            }
            break;

        case TYPE_DRAWSEGMENT:
            {
                DRAWSEGMENT* new_drawsegment = new DRAWSEGMENT( m_Pcb );
                new_drawsegment->Copy( (DRAWSEGMENT*) item );
                m_Pcb->Add( new_drawsegment );
                newitem = new_drawsegment;
            }
            break;

        case TYPE_TEXTE:
        {
            TEXTE_PCB* new_pcbtext = new TEXTE_PCB( m_Pcb );
            new_pcbtext->Copy( (TEXTE_PCB*) item );
            m_Pcb->Add( new_pcbtext );
            newitem = new_pcbtext;
        }
        break;

        case TYPE_MIRE:
            {
                MIREPCB* new_mire = new MIREPCB( m_Pcb );
                new_mire->Copy( (MIREPCB*) item );
                m_Pcb->Add( new_mire );
                newitem = new_mire;
            }
            break;

        case TYPE_DIMENSION:
            {
                DIMENSION* new_cotation = new DIMENSION( m_Pcb );
                new_cotation->Copy( (DIMENSION*) item );
                m_Pcb->Add( new_cotation );
                newitem = new_cotation;
            }
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Duplicate( ) error: unexpected type" ) );
            break;
        }

        if( newitem )
            {
                newitem->Move( MoveVector );
                picker.m_PickedItem     = newitem;
                picker.m_PickedItemType = newitem->Type();
                newList.PushItem( picker );
            }
    }

    if( newList.GetCount() )
        SaveCopyInUndoList( newList, UR_NEW );

    Compile_Ratsnest( NULL, true );
    DrawPanel->Refresh( true );
}
