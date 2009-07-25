/****************************************************/
/* Routines de gestion des commandes sur blocks		*/
/* (section commune eeschema/pcbnew...              */
/****************************************************/

/* Fichier common.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "macros.h"
#include "base_struct.h"
#include "class_base_screen.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"


/*******************/
/* BLOCK_SELECTOR */
/*******************/

/****************************************************************************/
BLOCK_SELECTOR::BLOCK_SELECTOR() :
    EDA_BaseStruct( BLOCK_LOCATE_STRUCT_TYPE ),
    EDA_Rect()
/****************************************************************************/
{
    m_State   = STATE_NO_BLOCK; /* Etat (enum BlockState) du block */
    m_Command = BLOCK_IDLE;     /* Type (enum CmdBlockType) d'operation */
    m_Color   = BROWN;
}


/****************************************/
BLOCK_SELECTOR::~BLOCK_SELECTOR()
/****************************************/
{
}


/***************************************************************/
void BLOCK_SELECTOR::SetMessageBlock( WinEDA_DrawFrame* frame )
/***************************************************************/

/*
 *  Print block command message (Block move, Block copy ...) in status bar
 */
{
    wxString msg;

    switch( m_Command )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        msg = _( "Block Move" );
        break;

    case BLOCK_DRAG:     /* Drag */
        msg = _( "Block Drag" );
        break;

    case BLOCK_COPY:     /* Copy */
        msg = _( "Block Copy" );
        break;

    case BLOCK_DELETE:     /* Delete */
        msg = _( "Block Delete" );
        break;

    case BLOCK_SAVE:     /* Save */
        msg = _( "Block Save" );
        break;

    case BLOCK_PASTE:
        msg = _( "Block Paste" );
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        msg = _( "Win Zoom" );
        break;

    case BLOCK_ROTATE:     /* Rotate 90 deg */
        msg = _( "Block Rotate" );
        break;

    case BLOCK_INVERT:     /* Flip */
        msg = _( "Block Invert" );
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:     /* mirror */
        msg = _( "Block Mirror" );
        break;

    case BLOCK_ABORT:
        break;

    default:
        msg = wxT( "????" );
        break;
    }

    frame->DisplayToolMsg( msg );
}


/**************************************************************/
void BLOCK_SELECTOR::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset,
                           int aDrawMode,
                           int aColor )
/**************************************************************/
{
    int w = aPanel->GetScreen()->Scale( GetWidth() );
    int h = aPanel->GetScreen()->Scale( GetHeight() );

    GRSetDrawMode( aDC, aDrawMode );
    if(  w == 0 || h == 0 )
        GRLine( &aPanel->m_ClipBox, aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
    else
        GRRect( &aPanel->m_ClipBox, aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
}


/*************************************************************************/
void BLOCK_SELECTOR::InitData( WinEDA_DrawPanel* aPanel, const wxPoint& startpos )
/*************************************************************************/

/** function InitData
 *  Init the initial values of a BLOCK_SELECTOR, before starting a block command
 */
{
    m_State = STATE_BLOCK_INIT;
    SetOrigin( startpos );
    SetSize( wxSize( 0, 0 ) );
    m_ItemsSelection.ClearItemsList();
    aPanel->ManageCurseur = DrawAndSizingBlockOutlines;
    aPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
}


/** Function ClearItemsList
 * delete only the list of EDA_BaseStruct * pointers, NOT the pointed data itself
 */
void BLOCK_SELECTOR::ClearItemsList()
{
    m_ItemsSelection.ClearItemsList();
}

/** Function ClearListAndDeleteItems
 * delete only the list of EDA_BaseStruct * pointers, AND the data pinted by m_Item
 */
void BLOCK_SELECTOR::ClearListAndDeleteItems()
{
     m_ItemsSelection.ClearListAndDeleteItems();
}

/** Function PushItem
 * Add aItem to the list of items
 * @param aItem = an ITEM_PICKER to add to the list
 */
void BLOCK_SELECTOR::PushItem( ITEM_PICKER& aItem )
{
    m_ItemsSelection.PushItem(  aItem );
}



/*************************************************************************/
bool WinEDA_DrawFrame::HandleBlockBegin( wxDC* DC, int key,
                                         const wxPoint& startpos )
/*************************************************************************/

/*  First command block function:
 *  Init the Block infos: command type, initial position, and other variables..
 */
{
    BLOCK_SELECTOR* Block = &GetBaseScreen()->m_BlockLocate;

    if( ( Block->m_Command != BLOCK_IDLE )
       || ( Block->m_State != STATE_NO_BLOCK ) )
        return FALSE;

    Block->m_Flags   = 0;
    Block->m_Command = (CmdBlockType) ReturnBlockCommand( key );
    if( Block->m_Command == 0 )
        return FALSE;

    switch( Block->m_Command )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                /* Move */
    case BLOCK_DRAG:                /* Drag */
    case BLOCK_COPY:                /* Copy */
    case BLOCK_DELETE:              /* Delete */
    case BLOCK_SAVE:                /* Save */
    case BLOCK_ROTATE:              /* Rotate 90 deg */
    case BLOCK_INVERT:              /* Flip */
    case BLOCK_ZOOM:                /* Window Zoom */
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:            /* mirror */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        Block->InitData( DrawPanel, startpos );
        break;

    case BLOCK_PASTE:
        Block->InitData( DrawPanel, startpos );
        Block->m_BlockLastCursorPosition.x = 0;
        Block->m_BlockLastCursorPosition.y = 0;
        InitBlockPasteInfos();
        if( Block->m_ItemsSelection.GetCount() == 0 )      /* No data to paste */
        {
            DisplayError( this, wxT( "No Block to paste" ), 20 );
            GetBaseScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
            DrawPanel->ManageCurseur = NULL;
            return TRUE;
        }
        if( DrawPanel->ManageCurseur == NULL )
        {
            Block->m_ItemsSelection.ClearItemsList();
            DisplayError( this,
                         wxT( "WinEDA_DrawFrame::HandleBlockBegin() Err: ManageCurseur NULL" ) );
            return TRUE;
        }
        Block->m_State = STATE_BLOCK_MOVE;
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        break;

    default:
    {
        wxString msg;
        msg << wxT( "WinEDA_DrawFrame::HandleBlockBegin() error: Unknown command " ) <<
        Block->m_Command;
        DisplayError( this, msg );
    }
    break;
    }

    Block->SetMessageBlock( this );
    return TRUE;
}

/********************************************************************************/
void DrawAndSizingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/********************************************************************************/

/* Redraw the outlines of the block which shows the search area for block commands
 *  The first point of the rectangle showing the area is initialised
 *  by Initm_BlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor
 */
{
    BLOCK_SELECTOR* PtBlock;

    PtBlock = &panel->GetScreen()->m_BlockLocate;

    PtBlock->m_MoveVector = wxPoint( 0, 0 );

    /* Effacement ancien cadre */
    if( erase )
        PtBlock->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, PtBlock->m_Color );

    PtBlock->m_BlockLastCursorPosition = panel->GetScreen()->m_Curseur;
    PtBlock->SetEnd( panel->GetScreen()->m_Curseur );

    PtBlock->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, PtBlock->m_Color );

    if( PtBlock->m_State == STATE_BLOCK_INIT )
    {
        if( PtBlock->GetWidth() || PtBlock->GetHeight() )
            /* 2ieme point existant: le rectangle n'est pas de surface nulle */
            PtBlock->m_State = STATE_BLOCK_END;
    }
}


/******************************************************************/
void AbortBlockCurrentCommand( WinEDA_DrawPanel* Panel, wxDC* DC )
/******************************************************************/

/*
 *  Cancel Current block operation.
 */
{
    BASE_SCREEN* screen = Panel->GetScreen();

    if( Panel->ManageCurseur )                      /* Erase current drawing on screen */
    {
        Panel->ManageCurseur( Panel, DC, FALSE );   /* Efface dessin fantome */
        Panel->ManageCurseur = NULL;
        Panel->ForceCloseManageCurseur = NULL;
        screen->SetCurItem( NULL );

        /* Delete the picked wrapper if this is a picked list. */
        if( screen->m_BlockLocate.m_Command != BLOCK_PASTE )
            screen->m_BlockLocate.ClearItemsList();
    }

    screen->m_BlockLocate.m_Flags = 0;
    screen->m_BlockLocate.m_State = STATE_NO_BLOCK;

    screen->m_BlockLocate.m_Command = BLOCK_ABORT;
    Panel->m_Parent->HandleBlockEnd( DC );

    screen->m_BlockLocate.m_Command = BLOCK_IDLE;
    Panel->m_Parent->DisplayToolMsg( wxEmptyString );
}

