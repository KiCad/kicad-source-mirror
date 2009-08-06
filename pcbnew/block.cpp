/*****************************************************************/
/* Operations sur Blocks : deplacement, rotation, effacement ... */
/*****************************************************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN

/* Routines Locales */

static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/* Variables locales :*/
static bool Block_Include_Modules     = TRUE;
static bool Block_Include_Tracks      = TRUE;
static bool Block_Include_Zones       = TRUE;
static bool Block_Include_Draw_Items  = TRUE;
static bool Block_Include_Edges_Items = TRUE;
static bool Block_Include_PcbTextes   = TRUE;

/************************************/
/* class WinEDA_ExecBlockCmdFrame */
/************************************/

class WinEDA_ExecBlockCmdFrame : public wxDialog
{
private:

    WinEDA_BasePcbFrame* m_Parent;
    wxCheckBox*          m_Include_Modules;
    wxCheckBox*          m_Include_Tracks;
    wxCheckBox*          m_Include_Zones;
    wxCheckBox*          m_Include_Draw_Items;
    wxCheckBox*          m_Include_Edges_Items;
    wxCheckBox*          m_Include_PcbTextes;

public:

    // Constructor and destructor
    WinEDA_ExecBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                              const wxString&      title );
    ~WinEDA_ExecBlockCmdFrame()
    {
    }


private:
    void ExecuteCommand( wxCommandEvent& event );
    void Cancel( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_ExecBlockCmdFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_ExecBlockCmdFrame::ExecuteCommand )
EVT_BUTTON( wxID_CANCEL, WinEDA_ExecBlockCmdFrame::Cancel )
END_EVENT_TABLE()


/**************************************************************/
static bool InstallBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                                  const wxString&      title )
/**************************************************************/
{
    int     nocmd;
    wxPoint oldpos = parent->GetScreen()->m_Curseur;

    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_ExecBlockCmdFrame* frame = new WinEDA_ExecBlockCmdFrame( parent, title );

    nocmd = frame->ShowModal();
    frame->Destroy();

    parent->GetScreen()->m_Curseur = oldpos;

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;

    parent->DrawPanel->SetCursor(
        parent->DrawPanel->m_PanelCursor = parent->DrawPanel->m_PanelDefaultCursor );

    return nocmd ? FALSE : TRUE;
}


/******************************************************************************/
WinEDA_ExecBlockCmdFrame::WinEDA_ExecBlockCmdFrame( WinEDA_BasePcbFrame* parent,
                                                    const wxString&      title ) :
    wxDialog( parent, -1, title, wxPoint( -1, -1 ), wxDefaultSize,
              DIALOG_STYLE )
/******************************************************************************/
{
    wxPoint   pos;
    wxButton* m_button1;
    wxButton* m_button2;

    m_Parent = parent;
    Centre();
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );
    this->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90,
                           false, wxEmptyString ) );

    /* Sizer 1 creation */
    wxFlexGridSizer* fgSizer1;
    fgSizer1 = new wxFlexGridSizer( 1, 1, 0, 0 );
    fgSizer1->SetFlexibleDirection( wxBOTH );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    // Selection des options :
    m_Include_Modules = new wxCheckBox( this, -1, _(
                                            "Include Modules" ), wxDefaultPosition, wxDefaultSize,
                                        0 );
    m_Include_Modules->SetValue( Block_Include_Modules );
    fgSizer1->Add( m_Include_Modules, 0, wxALL, 5 );

    m_Include_Tracks = new wxCheckBox( this, -1, _(
                                           "Include tracks" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Tracks->SetValue( Block_Include_Tracks );
    fgSizer1->Add( m_Include_Tracks, 0, wxALL, 5 );

    m_Include_Zones = new wxCheckBox( this, -1, _(
                                          "Include zones" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_Include_Zones->SetValue( Block_Include_Zones );
    fgSizer1->Add( m_Include_Zones, 0, wxALL, 5 );

    m_Include_PcbTextes = new wxCheckBox( this, -1,
                                          _(
                                              "Include Text on copper layers" ), wxDefaultPosition,
                                          wxDefaultSize, 0 );
    m_Include_PcbTextes->SetValue( Block_Include_PcbTextes );
    fgSizer1->Add( m_Include_PcbTextes, 0, wxALL, 5 );

    m_Include_Draw_Items = new wxCheckBox( this, -1, _(
                                               "Include drawings" ), wxDefaultPosition,
                                           wxDefaultSize, 0 );
    m_Include_Draw_Items->SetValue( Block_Include_Draw_Items );
    fgSizer1->Add( m_Include_Draw_Items, 0, wxALL, 5 );

    m_Include_Edges_Items = new wxCheckBox( this, -1, _(
                                                "Include board outline layer" ), wxDefaultPosition,
                                            wxDefaultSize, 0 );
    m_Include_Edges_Items->SetValue( Block_Include_Edges_Items );
    fgSizer1->Add( m_Include_Edges_Items, 0, wxALL, 5 );

    /* Sizer 2 creation */
    wxFlexGridSizer* fgSizer2;
    fgSizer2 = new wxFlexGridSizer( 1, 2, 0, 0 );
    fgSizer2->SetFlexibleDirection( wxBOTH );
    fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    /* Creation des boutons de commande */
    m_button2 = new wxButton( this, wxID_CANCEL, _(
                                  "Cancel" ), wxDefaultPosition, wxDefaultSize, 0 );
    fgSizer2->Add( m_button2, 0, wxALL, 5 );
    m_button1 = new wxButton( this, wxID_OK, _( "OK" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_button1->SetDefault();
    fgSizer2->Add( m_button1, 0, wxALL, 5 );

    fgSizer1->Add( fgSizer2, 1, wxALIGN_RIGHT, 5 );
    this->SetSizer( fgSizer1 );
    this->Layout();
    fgSizer1->Fit( this );
}


/**********************************************************************/
void WinEDA_ExecBlockCmdFrame::Cancel( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( -1 );
}


/*******************************************************************/
void WinEDA_ExecBlockCmdFrame::ExecuteCommand( wxCommandEvent& event )
/*******************************************************************/
{
    Block_Include_Modules     = m_Include_Modules->GetValue();
    Block_Include_Tracks      = m_Include_Tracks->GetValue();
    Block_Include_Zones       = m_Include_Zones->GetValue();
    Block_Include_Draw_Items  = m_Include_Draw_Items->GetValue();
    Block_Include_Edges_Items = m_Include_Edges_Items->GetValue();
    Block_Include_PcbTextes   = m_Include_PcbTextes->GetValue();

    EndModal( 0 );
}


/*************************************************/
int WinEDA_PcbFrame::ReturnBlockCommand( int key )
/*************************************************/

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
{
    int cmd = 0;

    switch( key )
    {
    default:
        cmd = key & 0x255;
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
        cmd = BLOCK_INVERT;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/*****************************************************/
void WinEDA_PcbFrame::HandleBlockPlace( wxDC* DC )
/*****************************************************/
/* Routine to handle the BLOCK PLACE commande */
{
    bool err = FALSE;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "Error in HandleBlockPLace : ManageCurseur = NULL" ) );
    }
    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Move();
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Duplicate();
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_PASTE:
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd()
    default:
        break;
    }

    GetScreen()->SetModify();

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
}


/**********************************************/
int WinEDA_PcbFrame::HandleBlockEnd( wxDC* DC )
/**********************************************/

/* Routine de gestion de la commande BLOCK END
 *  returne :
 *  0 si aucun composant selectionne
 *  1 sinon
 *  -1 si commande terminée et composants trouvés (block delete, block save)
 */
{
    int endcommande = TRUE;

    if( DrawPanel->ManageCurseur )
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
            endcommande = FALSE;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            break;

        case BLOCK_DELETE: /* Delete */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Delete();
            break;

        case BLOCK_ROTATE: /* Rotation */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Rotate();
            break;

        case BLOCK_INVERT: /* Flip */

            // Turn off the block rectangle now so it is not redisplayed
            DrawPanel->ManageCurseur = NULL;
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            Block_Flip();
            break;

        case BLOCK_SAVE: /* Save (not used, for future enhancements)*/
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            if( GetScreen()->m_BlockLocate.GetCount() )
            {
                DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );

// TODO	(if useful)			Save_Block( );
            }
            break;

        case BLOCK_PASTE:
            break;

        case BLOCK_ZOOM: /* Window Zoom */

            //Turn off the redraw block routine now so it is not displayed
            // with one corner at the new center of the screen
            DrawPanel->ManageCurseur = NULL;
            Window_Zoom( GetScreen()->m_BlockLocate );
            break;

        default:
            break;
        }

    if( endcommande == TRUE )
    {
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        GetScreen()->m_BlockLocate.ClearItemsList();
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        DisplayToolMsg( wxEmptyString );
    }

    return endcommande;
}


/* Block operations: */

/**
 * Function Block_SelectItems
 * Uses  GetScreen()->m_BlockLocate
 * select items within the selected block.
 * selected items are put in the pick list
 * @param none
 */
void WinEDA_PcbFrame::Block_SelectItems()
{
    int         masque_layer;

    GetScreen()->m_BlockLocate.Normalize();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    ITEM_PICKER        picker( NULL, UR_UNSPECIFIED );

    /* Effacement des modules */
    if( Block_Include_Modules )
    {
        for( MODULE* module = m_Pcb->m_Modules; module != NULL; module = module->Next() )
        {
            if( module->HitTest( GetScreen()->m_BlockLocate ) )
            {
                picker.m_PickedItem     = module;
                picker.m_PickedItemType = module->Type();
                itemsList->PushItem( picker );
            }
        }
    }

    /* Remove tracks and vias */
    if( Block_Include_Tracks )
    {
        for( TRACK* pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
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
    if( Block_Include_Draw_Items )
        masque_layer = ALL_LAYERS;

    if( !Block_Include_Edges_Items )
        masque_layer &= ~EDGE_LAYER;

    for( BOARD_ITEM* PtStruct = m_Pcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
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
            if( !Block_Include_PcbTextes )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        case TYPE_MIRE:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
                break;
            if( !PtStruct->HitTest( GetScreen()->m_BlockLocate ) )
                break;
            select_me = true; // This item is in bloc: select it
            break;

        case TYPE_COTATION:
            if( (g_TabOneLayerMask[PtStruct->GetLayer()] & masque_layer) == 0 )
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
    if( Block_Include_Zones )
    {
#if 0  
        /* This section can creates problems if selected:
         * m_Pcb->m_Zone can have a *lot* of items (100 000 is easily possible)
         * so it is not selected (and TODO: will be removed, one day)
         */
        for( SEGZONE* pt_segm = m_Pcb->m_Zone; pt_segm != NULL; pt_segm = pt_segm->Next() )
        {    /* Segments used in Zone filling selection */

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


/**************************************************************************/
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/**************************************************************************/

/* Retrace le contour du block de repositionnement des structures a déplacer
 */
{
    int          Color;
    BASE_SCREEN* screen = panel->GetScreen();

    Color = YELLOW;

    /* Effacement ancien cadre */
    if( erase )
    {
        screen->m_BlockLocate.Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, Color );
        if( screen->m_BlockLocate.m_MoveVector.x || screen->m_BlockLocate.m_MoveVector.y )
        {
            screen->m_BlockLocate.Draw( panel,
                                        DC,
                                        screen->m_BlockLocate.m_MoveVector,
                                        g_XorMode,
                                        Color );
        }
    }

    if( screen->m_BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->m_BlockLocate.m_MoveVector.x = screen->m_Curseur.x -
                                               screen->m_BlockLocate.GetRight();
        screen->m_BlockLocate.m_MoveVector.y = screen->m_Curseur.y -
                                               screen->m_BlockLocate.GetBottom();
    }

    screen->m_BlockLocate.Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, Color );
    if( screen->m_BlockLocate.m_MoveVector.x || screen->m_BlockLocate.m_MoveVector.y )
    {
        screen->m_BlockLocate.Draw( panel,
                                    DC,
                                    screen->m_BlockLocate.m_MoveVector,
                                    g_XorMode,
                                    Color );
    }
}


/************************************************/
void WinEDA_PcbFrame::Block_Delete()
/************************************************/

/*
 *  routine d'effacement du block deja selectionne
 */
{
    if( !InstallBlockCmdFrame( this, _( "Delete Block" ) ) )
        return;

    Block_SelectItems();
    if( GetScreen()->m_BlockLocate.GetCount() == 0 )
        return;

    GetScreen()->SetModify();
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

        case TYPE_ZONE_CONTAINER:        // a zone area
            m_Pcb->Remove( item );
            break;

        case TYPE_DRAWSEGMENT:              // a segment not on copper layers
        case TYPE_TEXTE:                    // a text on a layer
        case TYPE_TRACK:                    // a track segment (segment on a copper layer)
        case TYPE_VIA:                      // a via (like atrack segment on a copper layer)
        case TYPE_COTATION:                 // a dimension (graphic item)
        case TYPE_MIRE:                     // a target (graphic item)
            item->UnLink();
            break;

        // These items are deleted, but not put in undo list
        case TYPE_MARKER_PCB:               // a marker used to show something
        case TYPE_ZONE:                     // a segment used to fill a zome area (segment on a copper layer)
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

    Compile_Ratsnest( NULL, TRUE );
    DrawPanel->Refresh( TRUE );
}


/**
 * Function Block_Rotate
 * Rotate all items within the selected block.
 * The rotation centre is the centre of the block
 * @param none
 */
void WinEDA_PcbFrame::Block_Rotate()
{
    wxPoint oldpos;
    wxPoint centre;         // rotation centre for the rotation transform
    int rotAngle = 900;     // rottaion angle in 0.1 deg.

    if( !InstallBlockCmdFrame( this, _( "Rotate Block" ) ) )
        return;

    Block_SelectItems();
    if( GetScreen()->m_BlockLocate.GetCount() == 0 )
        return;

    oldpos = GetScreen()->m_Curseur;
    centre = GetScreen()->m_BlockLocate.Centre();   // This is the rotation centre

    GetScreen()->SetModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_ROTATED;

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        wxASSERT(item);
        itemsList->SetPickedItemStatus( UR_ROTATED, ii );
        item->Rotate(centre, rotAngle);
        switch( item->Type() )
        {
        case TYPE_MODULE:
            ((MODULE*) item)->m_Flags     = 0;
            m_Pcb->m_Status_Pcb = 0;
        break;

        /* Move and rotate the track segments */
        case TYPE_TRACK:                    // a track segment (segment on a copper layer)
        case TYPE_VIA:                      // a via (like atrack segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
        break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_COTATION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:                     // a segment used to fill a zome area (segment on a copper layer)
            itemsList->RemovePicker( ii );
            ii--;
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Rotate( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_ROTATED, centre );

    Compile_Ratsnest( NULL, TRUE );
    DrawPanel->Refresh( TRUE );
}


/**
 * Function Block_Flip
 * Flip items within the selected block.
 * The flip centre is the centre of the block
 * @param none
 */
void WinEDA_PcbFrame::Block_Flip()
{
#define INVERT( pos ) (pos) = center.y - ( (pos) - center.y )
    wxPoint memo;
    wxPoint center; /* position de l'axe d'inversion de l'ensemble des elements */

    Block_SelectItems();
    if( GetScreen()->m_BlockLocate.GetCount() == 0 )
        return;

    GetScreen()->SetModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;
    itemsList->m_Status = UR_FLIPPED;

    memo = GetScreen()->m_Curseur;

    /* calcul du centre d'inversion */
    center = GetScreen()->m_BlockLocate.Centre();

    for( unsigned ii = 0; ii < itemsList->GetCount(); ii++ )
    {
        BOARD_ITEM* item = (BOARD_ITEM*) itemsList->GetPickedItem( ii );
        wxASSERT(item);
        itemsList->SetPickedItemStatus( UR_FLIPPED, ii );
        item->Flip(center);
        switch( item->Type() )
        {
        case TYPE_MODULE:
            ((MODULE*) item)->m_Flags     = 0;
            m_Pcb->m_Status_Pcb = 0;
        break;

        /* Move and rotate the track segments */
        case TYPE_TRACK:                    // a track segment (segment on a copper layer)
        case TYPE_VIA:                      // a via (like atrack segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
            break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_COTATION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:                     // a segment used to fill a zome area (segment on a copper layer)
            itemsList->RemovePicker( ii );
            ii--;
            break;


        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Flip( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_FLIPPED, center );
    Compile_Ratsnest( NULL, TRUE );
    DrawPanel->Refresh( TRUE );
}


/**
 * Function Block_Move
 * moves all tracks and segments within the selected block.
 * New location is determined by the current offset from the selected block's original location.
 * @param none
 */
void WinEDA_PcbFrame::Block_Move()
{
    if( !InstallBlockCmdFrame( this, _( "Move Block" ) ) )
        return;

    Block_SelectItems();
    if( GetScreen()->m_BlockLocate.GetCount() == 0 )
        return;

    GetScreen()->SetModify();

    wxPoint MoveVector = GetScreen()->m_BlockLocate.m_MoveVector;

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
            m_Pcb->m_Status_Pcb    = 0;
            ((MODULE*) item)->m_Flags        = 0;
        break;

        /* Move track segments */
        case TYPE_TRACK:                    // a track segment (segment on a copper layer)
        case TYPE_VIA:                      // a via (like a track segment on a copper layer)
            m_Pcb->m_Status_Pcb = 0;
        break;

        case TYPE_ZONE_CONTAINER:
        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_COTATION:
            break;

        // This item is not put in undo list
        case TYPE_ZONE:                     // a segment used to fill a zome area (segment on a copper layer)
            itemsList->RemovePicker( ii );
            ii--;
            break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Move( ) error: unexpected type" ) );
            break;
        }
    }

    SaveCopyInUndoList( *itemsList, UR_MOVED, MoveVector );

    Compile_Ratsnest( NULL, TRUE );
    DrawPanel->Refresh( TRUE );
}


/**
 * Function Block_Duplicate
 * Duplicate all items within the selected block.
 * New location is determined by the current offset from the selected block's original location.
 * @param none
 */
void WinEDA_PcbFrame::Block_Duplicate()
{
    wxPoint MoveVector = GetScreen()->m_BlockLocate.m_MoveVector;

    if( !InstallBlockCmdFrame( this, _( "Copy Block" ) ) )
        return;

    Block_SelectItems();
    if( GetScreen()->m_BlockLocate.GetCount() == 0 )
        return;

    GetScreen()->SetModify();

    PICKED_ITEMS_LIST* itemsList = &GetScreen()->m_BlockLocate.m_ItemsSelection;

    PICKED_ITEMS_LIST newList;
    newList.m_Status = UR_NEW;

    ITEM_PICKER picker(NULL, UR_NEW);
    BOARD_ITEM * newitem;

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

        case TYPE_ZONE:                  // a segment used to fill a zome area (segment on a copper layer)
        {
            // SEG_ZONE items are not copied or put in undo list
            // they must be recreated by zone filling
        }
        break;

        case TYPE_ZONE_CONTAINER:
        {
            ZONE_CONTAINER* new_zone = new ZONE_CONTAINER( (BOARD*) item->GetParent() );
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

        case TYPE_COTATION:
        {
            COTATION* new_cotation = new COTATION( m_Pcb );
            new_cotation->Copy( (COTATION*) item );
            m_Pcb->Add( new_cotation );
            newitem = new_cotation;
        }
        break;

        default:
            wxMessageBox( wxT( "WinEDA_PcbFrame::Block_Duplicate( ) error: unexpected type" ) );
            break;
        }

        if ( newitem )
        {
            newitem->Move( MoveVector );
            picker.m_PickedItem = newitem;
            picker.m_PickedItemType = newitem->Type();
            newList.PushItem(picker);
        }
    }

    if( newList.GetCount() )
        SaveCopyInUndoList( newList, UR_NEW );

    Compile_Ratsnest( NULL, TRUE );
    DrawPanel->Refresh( TRUE );
}
