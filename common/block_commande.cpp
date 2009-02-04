/****************************************************/
/* Routines de gestion des commandes sur blocks		*/
/* (section commune eeschema/pcbnew... 				*/
/****************************************************/

/* Fichier common.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "macros.h"
#include "base_struct.h"
#include "sch_item_struct.h"
#include "class_base_screen.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"


/*******************/
/* DrawBlockStruct */
/*******************/

/****************************************************************************/
DrawBlockStruct::DrawBlockStruct() :
    EDA_BaseStruct( BLOCK_LOCATE_STRUCT_TYPE )
    , EDA_Rect()
/****************************************************************************/
{
    m_State   = STATE_NO_BLOCK; /* Etat (enum BlockState) du block */
    m_Command = BLOCK_IDLE;     /* Type (enum CmdBlockType) d'operation */
    m_BlockDrawStruct = NULL;   /* pointeur sur la structure */
    m_Color = BROWN;
}


/****************************************/
DrawBlockStruct::~DrawBlockStruct()
/****************************************/
{
}


/***************************************************************/
void DrawBlockStruct::SetMessageBlock( WinEDA_DrawFrame* frame )
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
void DrawBlockStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC )
/**************************************************************/
{
    int w = panel->GetScreen()->Scale( GetWidth() );
    int h = panel->GetScreen()->Scale( GetHeight() );

    if(  w == 0 || h == 0 )
        GRLine( &panel->m_ClipBox, DC, GetX(), GetY(),
                GetRight(), GetBottom(), 0, m_Color );
    else
        GRRect( &panel->m_ClipBox, DC, GetX(), GetY(),
                GetRight(), GetBottom(), 0, m_Color );
}


/*************************************************************************/
bool WinEDA_DrawFrame::HandleBlockBegin( wxDC* DC, int key,
                                         const wxPoint& startpos )
/*************************************************************************/

/* 	First command block function:
 *  Init the Block infos: command type, initial position, and other variables..
 */
{
    DrawBlockStruct* Block = & GetBaseScreen()->BlockLocate;

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
        InitBlockLocateDatas( DrawPanel, startpos );
        break;

    case BLOCK_PASTE:
        InitBlockLocateDatas( DrawPanel, startpos );
        Block->m_BlockLastCursorPosition.x = 0;
        Block->m_BlockLastCursorPosition.y = 0;
        InitBlockPasteInfos();
        if( Block->m_BlockDrawStruct == NULL )      /* No data to paste */
        {
            DisplayError( this, wxT( "No Block to paste" ), 20 );
            GetBaseScreen()->BlockLocate.m_Command = BLOCK_IDLE;
            DrawPanel->ManageCurseur = NULL;
            return TRUE;
        }
        if( DrawPanel->ManageCurseur == NULL )
        {
            Block->m_BlockDrawStruct = NULL;
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
        if( (screen->BlockLocate.m_Command != BLOCK_PASTE)
           && screen->BlockLocate.m_BlockDrawStruct )
        {
            if( screen->BlockLocate.m_BlockDrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
            {
                DrawPickedStruct* PickedList;
                PickedList = (DrawPickedStruct*) screen->BlockLocate.m_BlockDrawStruct;
                PickedList->DeleteWrapperList();
            }
            screen->BlockLocate.m_BlockDrawStruct = NULL;
        }
    }

    screen->BlockLocate.m_Flags = 0;
    screen->BlockLocate.m_State = STATE_NO_BLOCK;

    screen->BlockLocate.m_Command = BLOCK_ABORT;
    Panel->m_Parent->HandleBlockEnd( DC );

    screen->BlockLocate.m_Command = BLOCK_IDLE;
    Panel->m_Parent->DisplayToolMsg( wxEmptyString );
}


/*************************************************************************/
void InitBlockLocateDatas( WinEDA_DrawPanel* Panel, const wxPoint& startpos )
/*************************************************************************/

/*
 *  Init the initial values of a BlockLocate, before starting a block command
 */
{
    BASE_SCREEN* screen = Panel->GetScreen();

    screen->BlockLocate.m_State = STATE_BLOCK_INIT;
    screen->BlockLocate.SetOrigin( startpos );
    screen->BlockLocate.SetSize( wxSize( 0, 0 ) );
    screen->BlockLocate.SetNext( NULL );
    screen->BlockLocate.m_BlockDrawStruct = NULL;
    Panel->ManageCurseur = DrawAndSizingBlockOutlines;
    Panel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
}


/********************************************************************************/
void DrawAndSizingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/********************************************************************************/

/* Redraw the outlines of the block which shows the search area for block commands
 *  The first point of the rectangle showing the area is initialised
 *  by InitBlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor
 */
{
    DrawBlockStruct* PtBlock;

    PtBlock = &panel->GetScreen()->BlockLocate;

    PtBlock->m_MoveVector = wxPoint( 0, 0 );

    GRSetDrawMode( DC, g_XorMode );

    /* Effacement ancien cadre */
    if( erase )
        PtBlock->Draw( panel, DC );

    PtBlock->m_BlockLastCursorPosition = panel->GetScreen()->m_Curseur;
    PtBlock->SetEnd( panel->GetScreen()->m_Curseur );

    PtBlock->Draw( panel, DC );

    if( PtBlock->m_State == STATE_BLOCK_INIT )
    {
        if( PtBlock->GetWidth() || PtBlock->GetHeight() )
            /* 2ieme point existant: le rectangle n'est pas de surface nulle */
            PtBlock->m_State = STATE_BLOCK_END;
    }
}
