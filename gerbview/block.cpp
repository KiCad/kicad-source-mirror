/*****************************************************************/
/* Operations sur Blocks : deplacement, rotation, effacement ... */
/*****************************************************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "confirm.h"

#include "gerbview.h"
#include "protos.h"


#define BLOCK_COLOR BROWN

/* Routines Locales */

static void     DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

static TRACK*   IsSegmentInBox( BLOCK_SELECTOR& blocklocate, TRACK* PtSegm );

/* Variables locales :*/

/*************************************************/
int WinEDA_GerberFrame::ReturnBlockCommand( int key )
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
        break;

    case GR_KB_CTRL:
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_COPY;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/*****************************************************/
void WinEDA_GerberFrame::HandleBlockPlace( wxDC* DC )
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
    case  BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Move( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        Block_Duplicate( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_PASTE:
        break;

    case BLOCK_ZOOM:        // Handle by HandleBlockEnd()
    case BLOCK_ROTATE:
    case BLOCK_FLIP:
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    case BLOCK_SELECT_ITEMS_ONLY:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        break;
    }

    GetScreen()->SetModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        DisplayError( this, wxT( "HandleBlockPLace error: some items left" ) );
        GetScreen()->m_BlockLocate.ClearItemsList();
    }

    DisplayToolMsg( wxEmptyString );
}


/**********************************************/
int WinEDA_GerberFrame::HandleBlockEnd( wxDC* DC )
/**********************************************/

/* Routine de gestion de la commande BLOCK END
 *  returne :
 *  0 si aucun compos ant selectionne
 *  1 sinon
 *  -1 si commande terminée et composants trouvés (block delete, block save)
 */
{
    int  endcommande  = TRUE;
    bool zoom_command = FALSE;

    if( DrawPanel->ManageCurseur )

        switch( GetScreen()->m_BlockLocate.m_Command )
        {
        case  BLOCK_IDLE:
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
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            Block_Delete( DC );
            break;

        case BLOCK_MIRROR_X: /* Mirror*/
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            Block_Mirror_X( DC );
            break;

        case BLOCK_ROTATE: /* Unused */
            break;

        case BLOCK_FLIP: /* Flip, unused */
            break;

        case BLOCK_SAVE: /* Save (not used)*/
            break;

        case BLOCK_PASTE:
            break;

        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = TRUE;
            break;

        case BLOCK_ABORT:
        case BLOCK_SELECT_ITEMS_ONLY:
        case BLOCK_MIRROR_Y:
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

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return endcommande;
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
        screen->m_BlockLocate.Draw( panel, DC, wxPoint(0,0),g_XorMode, Color );
        if( screen->m_BlockLocate.m_MoveVector.x || screen->m_BlockLocate.m_MoveVector.y )
        {
            screen->m_BlockLocate.Draw( panel, DC, screen->m_BlockLocate.m_MoveVector,g_XorMode, Color );
        }
    }

    if( panel->GetScreen()->m_BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->m_BlockLocate.m_MoveVector.x = screen->m_Curseur.x - screen->m_BlockLocate.GetRight();
        screen->m_BlockLocate.m_MoveVector.y = screen->m_Curseur.y - screen->m_BlockLocate.GetBottom();
    }

    screen->m_BlockLocate.Draw( panel, DC, wxPoint(0,0),g_XorMode, Color );
    if( screen->m_BlockLocate.m_MoveVector.x || screen->m_BlockLocate.m_MoveVector.y )
    {
        screen->m_BlockLocate.Draw( panel, DC, screen->m_BlockLocate.m_MoveVector,g_XorMode, Color );
    }
}


/************************************************/
void WinEDA_GerberFrame::Block_Delete( wxDC* DC )
/************************************************/

/*
 *  routine d'effacement du block deja selectionne
 */
{
    if( !IsOK( this, _( "Ok to delete block ?" ) ) )
        return;

    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();
    GetScreen()->SetCurItem( NULL );

    /* Effacement des Pistes */
    TRACK* pt_segm, * NextS;
    for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = NextS )
    {
        NextS = pt_segm->Next();
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, pt_segm ) )
        {
            /* la piste est ici bonne a etre efface */
            pt_segm->Draw( DrawPanel, DC, GR_XOR );
            pt_segm->DeleteStructure();
        }
    }

    /* Effacement des Zones */
    for( pt_segm = m_Pcb->m_Zone; pt_segm != NULL; pt_segm = NextS )
    {
        NextS = pt_segm->Next();
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, pt_segm ) )
        {
            /* la piste est ici bonne a etre efface */
            pt_segm->Draw( DrawPanel, DC, GR_XOR );
            pt_segm->DeleteStructure();
        }
    }

    /* Rafraichissement de l'ecran : */
    RedrawActiveWindow( DC, TRUE );
}


/************************************************/
void WinEDA_GerberFrame::Block_Move( wxDC* DC )
/************************************************/

/*
 *  Function to move items in the current selected block
 */
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    /* calcul du vecteur de deplacement pour les deplacements suivants */
    delta = GetScreen()->m_BlockLocate.m_MoveVector;

    /* Move the Track segments in block */
    TRACK* track = m_Pcb->m_Track;
    while( track )
    {
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, track ) )
        {
            m_Pcb->m_Status_Pcb = 0;
            track->Draw( DrawPanel, DC, GR_XOR );   // erase the display
            track->m_Start += delta;
            track->m_End   += delta;
            // the two parameters are used in gerbview to store centre coordinates for arcs.
            // move this centre
            track->m_Param  += delta.x;
            track->SetSubNet( track->GetSubNet() + delta.y );

            track->Draw( DrawPanel, DC, GR_OR ); // redraw the moved track
        }
        track = track->Next();
    }

    /* Move the Zone segments in block */
    SEGZONE * zsegment= m_Pcb->m_Zone;
    while( zsegment )
    {
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, zsegment ) )
        {
            zsegment->Draw( DrawPanel, DC, GR_XOR );   // erase the display
            zsegment->m_Start += delta;
            zsegment->m_End   += delta;
            // the two parameters are used in gerbview to store centre coordinates for arcs.
            // move this centre
            zsegment->m_Param  += delta.x;
            zsegment->SetSubNet( zsegment->GetSubNet() + delta.y );
            zsegment->Draw( DrawPanel, DC, GR_OR ); // redraw the moved zone zegment
        }
        zsegment = zsegment->Next();
    }

    DrawPanel->Refresh( TRUE );
}


/************************************************/
void WinEDA_GerberFrame::Block_Mirror_X( wxDC* DC )
/************************************************/

/*
 *  Function to mirror items in the current selected block
 */
{
    int     xoffset = 0;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    /* Calculate offset to mirror track points from block edges */
    xoffset = GetScreen()->m_BlockLocate.m_Pos.x + GetScreen()->m_BlockLocate.m_Pos.x
              + GetScreen()->m_BlockLocate.m_Size.x;

    /* Move the Track segments in block */
    for( TRACK* track = m_Pcb->m_Track;  track;  track = track->Next() )
    {
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, track ) )
        {
            m_Pcb->m_Status_Pcb = 0;
            track->Draw( DrawPanel, DC, GR_XOR );   // erase the display
            track->m_Start.x = xoffset - track->m_Start.x;
            track->m_End.x   = xoffset - track->m_End.x;

            // the two parameters are used in gerbview to store centre coordinates for arcs.
            // move this centre
            track->m_Param = xoffset - track->m_Param;
            track->Draw( DrawPanel, DC, GR_OR ); // redraw the moved track
        }
    }

    /* Move the Zone segments in block */
    for( SEGZONE* zsegment = m_Pcb->m_Zone; zsegment; zsegment = zsegment->Next() )
    {
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, zsegment ) )
        {
            zsegment->Draw( DrawPanel, DC, GR_XOR );   // erase the display
            zsegment->m_Start.x = xoffset - zsegment->m_Start.x;
            zsegment->m_End.x   = xoffset - zsegment->m_End.x;

            // the two parameters are used in gerbview to store centre coordinates for arcs.
            // move this centre
            zsegment->m_Param = xoffset - zsegment->m_Param;
            zsegment->Draw( DrawPanel, DC, GR_OR ); // redraw the moved zone zegment
        }
    }

    DrawPanel->Refresh( TRUE );
}


/**************************************************/
void WinEDA_GerberFrame::Block_Duplicate( wxDC* DC )
/**************************************************/

/*
 *  Function to duplicate items in the current selected block
 */
{
    wxPoint     delta;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();


    /* calcul du vecteur de deplacement pour les deplacements suivants */
    delta = GetScreen()->m_BlockLocate.m_MoveVector;

    /* Copy selected track segments and move the new track its new location */
    TRACK* track = m_Pcb->m_Track;
    while( track )
    {
        TRACK* next_track = track->Next();
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, track ) )
        {
            /* this track segment must be duplicated */
            m_Pcb->m_Status_Pcb = 0;
            TRACK* new_track = track->Copy();

            m_Pcb->Add( new_track );

            new_track->m_Start += delta;
            new_track->m_End   += delta;

            new_track->Draw( DrawPanel, DC, GR_OR ); // draw the new created segment
        }
        track = next_track;
    }

    /* Copy the Zone segments  and move the new segment to its new location */
    SEGZONE * zsegment= m_Pcb->m_Zone;
    while( zsegment )
    {
        SEGZONE * next_zsegment = zsegment->Next();
        if( IsSegmentInBox( GetScreen()->m_BlockLocate, zsegment ) )
        {
            /* this zone segment must be duplicated */
            SEGZONE * new_zsegment = (SEGZONE*) zsegment->Copy();

            m_Pcb->Add( new_zsegment );

            new_zsegment->m_Start += delta;
            new_zsegment->m_End   += delta;

            new_zsegment->Draw( DrawPanel, DC, GR_OR ); // draw the new created segment
        }
        zsegment = next_zsegment;
    }
}


/**************************************************************************/
static TRACK* IsSegmentInBox( BLOCK_SELECTOR& blocklocate, TRACK* PtSegm )
/**************************************************************************/

/* Teste si la structure PtStruct est inscrite dans le block selectionne
 *  Retourne PtSegm si oui
 *          NULL si non
 */
{
    if( blocklocate.Inside( PtSegm->m_Start.x, PtSegm->m_Start.y ) )
        return PtSegm;

    if( blocklocate.Inside( PtSegm->m_End.x, PtSegm->m_End.y ) )
        return PtSegm;

    return NULL;
}
