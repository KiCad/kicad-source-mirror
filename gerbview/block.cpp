	/*****************************************************************/
	/* Operations sur Blocks : deplacement, rotation, effacement ... */
	/*****************************************************************/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "gerbview.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN

/* Routines Locales */

static void DrawMovingBlockOutlines(WinEDA_DrawPanel * panel, wxDC * DC, bool erase);

static TRACK * IsSegmentInBox(DrawBlockStruct & blocklocate,  TRACK * PtSegm );

/* Variables locales :*/

/*************************************************/
int WinEDA_GerberFrame::ReturnBlockCommand(int key)
/*************************************************/
/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
	the key (ALT, SHIFT ALT ..)
*/
{
int cmd = 0;
	
	switch ( key )
	{
		default:
		cmd = key & 0x255;
		break;

		case 0:
			cmd = BLOCK_MOVE;
			break;

		case GR_KB_SHIFT:
			break;

		case GR_KB_CTRL :
			break;

		case GR_KB_SHIFTCTRL :
			cmd = BLOCK_DELETE;
			break;

		case GR_KB_ALT :
			cmd = BLOCK_COPY;
			break;

		case MOUSE_MIDDLE:
			cmd = BLOCK_ZOOM;
			break;
	}

	return cmd ;
}


/*****************************************************/
void WinEDA_GerberFrame::HandleBlockPlace(wxDC * DC)
/*****************************************************/
/* Routine to handle the BLOCK PLACE commande */
{
bool err = FALSE;

	if(DrawPanel->ManageCurseur == NULL)
		{
		err = TRUE;
		DisplayError(this, wxT("Error in HandleBlockPLace : ManageCurseur = NULL") );
		}
	GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;

	switch(GetScreen()->BlockLocate.m_Command )
		{
		case  BLOCK_IDLE:
			err = TRUE;
			break;

		case BLOCK_DRAG: /* Drag */
		case BLOCK_MOVE: /* Move */
		case BLOCK_PRESELECT_MOVE: /* Move with preselection list*/
			if ( DrawPanel->ManageCurseur )
				DrawPanel->ManageCurseur(DrawPanel,DC, FALSE);
			Block_Move(DC);
			GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
			break;

		case BLOCK_COPY: /* Copy */
			if ( DrawPanel->ManageCurseur )
				DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);
			Block_Duplicate(DC);
			GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
			break;

		case BLOCK_PASTE:
			break;

		case BLOCK_ZOOM:	// Handle by HandleBlockEnd()
		case BLOCK_ROTATE:
		case BLOCK_INVERT:
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
	GetScreen()->BlockLocate.m_Flags = 0;
	GetScreen()->BlockLocate.m_State = STATE_NO_BLOCK;
	GetScreen()->BlockLocate.m_Command =  BLOCK_IDLE;
	if ( GetScreen()->BlockLocate.m_BlockDrawStruct )
		{
		DisplayError(this, wxT("Error in HandleBlockPLace DrawStruct != NULL") );
		GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
		}

	DisplayToolMsg(wxEmptyString);
}

/**********************************************/
int WinEDA_GerberFrame::HandleBlockEnd(wxDC * DC)
/**********************************************/
/* Routine de gestion de la commande BLOCK END
	returne :
	0 si aucun compos ant selectionne
	1 sinon
	-1 si commande terminée et composants trouvés (block delete, block save)
*/
{
int endcommande = TRUE;
bool zoom_command = FALSE;

if(DrawPanel->ManageCurseur )

	switch( GetScreen()->BlockLocate.m_Command )
		{
		case  BLOCK_IDLE:
			DisplayError(this, wxT("Error in HandleBlockPLace"));
			break;

		case BLOCK_DRAG: /* Drag (not used, for future enhancements)*/
		case BLOCK_MOVE: /* Move */
		case BLOCK_COPY: /* Copy */
		case BLOCK_PRESELECT_MOVE: /* Move with preselection list*/
			GetScreen()->BlockLocate.m_State = STATE_BLOCK_MOVE;
			endcommande = FALSE;
			DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);
			DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
			DrawPanel->ManageCurseur(DrawPanel,DC, FALSE);
			break;

		case BLOCK_DELETE: /* Delete */
			GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;
			DrawPanel->ManageCurseur(DrawPanel,DC, FALSE);
			Block_Delete(DC);
			break;

		case BLOCK_ROTATE: /* Unused */
			break;

		case BLOCK_INVERT: /* Fip */
			break;

		case BLOCK_SAVE: /* Save (not used)*/
			break;

		case BLOCK_PASTE: break;

		case BLOCK_ZOOM: /* Window Zoom */
			zoom_command = TRUE;
			break;

		case BLOCK_ABORT:
		case BLOCK_SELECT_ITEMS_ONLY:
		case BLOCK_MIRROR_X:
		case BLOCK_MIRROR_Y:
			break;
		}

	if ( endcommande == TRUE )
	{
		GetScreen()->BlockLocate.m_Flags = 0;
		GetScreen()->BlockLocate.m_State = STATE_NO_BLOCK;
		GetScreen()->BlockLocate.m_Command =  BLOCK_IDLE;
		GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
		DrawPanel->ManageCurseur = NULL;
		DrawPanel->ForceCloseManageCurseur = NULL;
		DisplayToolMsg(wxEmptyString);
	}

	if ( zoom_command )
		Window_Zoom( GetScreen()->BlockLocate );

	return(endcommande);
}


/**************************************************************************/
static void DrawMovingBlockOutlines(WinEDA_DrawPanel * panel, wxDC * DC, bool erase)
/**************************************************************************/
/* Retrace le contour du block de repositionnement des structures a déplacer
 */
{
int Color;
BASE_SCREEN * screen = panel->GetScreen();

	Color = YELLOW; GRSetDrawMode(DC, g_XorMode);

	/* Effacement ancien cadre */
	if( erase )
	{
		screen->BlockLocate.Draw(panel, DC);
		if ( screen->BlockLocate.m_MoveVector.x || screen->BlockLocate.m_MoveVector.y )
		{
			screen->BlockLocate.Offset(screen->BlockLocate.m_MoveVector);
			screen->BlockLocate.Draw(panel, DC);
			screen->BlockLocate.Offset(-screen->BlockLocate.m_MoveVector.x, -screen->BlockLocate.m_MoveVector.y);
		}
	}

	if ( panel->m_Parent->GetScreen()->BlockLocate.m_State != STATE_BLOCK_STOP )
	{
		screen->BlockLocate.m_MoveVector.x = screen->m_Curseur.x - screen->BlockLocate.GetRight();
		screen->BlockLocate.m_MoveVector.y = screen->m_Curseur.y - screen->BlockLocate.GetBottom();
	}

	screen->BlockLocate.Draw(panel, DC);
	if ( screen->BlockLocate.m_MoveVector.x || screen->BlockLocate.m_MoveVector.y )
	{
		screen->BlockLocate.Offset(screen->BlockLocate.m_MoveVector);
		screen->BlockLocate.Draw(panel, DC);
		screen->BlockLocate.Offset(-screen->BlockLocate.m_MoveVector.x, -screen->BlockLocate.m_MoveVector.y);
	}
}



/************************************************/
void WinEDA_BasePcbFrame::Block_Delete(wxDC *DC)
/************************************************/
/*
	routine d'effacement du block deja selectionne
*/
{
	if ( !IsOK(this, _("Ok to delete block ?") ) ) return;

	GetScreen()->SetModify();
	GetScreen()->BlockLocate.Normalize();
	GetScreen()->m_CurrentItem = NULL;

	/* Effacement des Pistes */
	TRACK * pt_segm, * NextS;
	for ( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = NextS)
		{
		NextS = pt_segm->Next();
		if( IsSegmentInBox(GetScreen()->BlockLocate, pt_segm ) )
			{	/* la piste est ici bonne a etre efface */
			pt_segm->Draw(DrawPanel, DC, GR_XOR) ;
			DeleteStructure(pt_segm);
			}
		}


	/* Effacement des Zones */
	for ( pt_segm = m_Pcb->m_Zone; pt_segm != NULL; pt_segm = NextS)
		{
		NextS = pt_segm->Next();
		if( IsSegmentInBox(GetScreen()->BlockLocate, pt_segm ) )
			{/* la piste est ici bonne a etre efface */
			pt_segm->Draw(DrawPanel, DC, GR_XOR) ;
			DeleteStructure(pt_segm);
			}
		}

	/* Rafraichissement de l'ecran : */
	RedrawActiveWindow(DC, TRUE);
}


/************************************************/
void WinEDA_BasePcbFrame::Block_Move(wxDC * DC)
/************************************************/
/*
	routine de deplacement des elements du block deja selectionne
*/
{
int deltaX, deltaY;
wxPoint oldpos;

	oldpos = GetScreen()->m_Curseur;
	DrawPanel->ManageCurseur = NULL;

	GetScreen()->m_Curseur = oldpos;
	DrawPanel->MouseToCursorSchema();
	GetScreen()->SetModify();
	GetScreen()->BlockLocate.Normalize();

	/* calcul du vecteur de deplacement pour les deplacements suivants */
	deltaX = GetScreen()->BlockLocate.m_MoveVector.x ;
	deltaY = GetScreen()->BlockLocate.m_MoveVector.y ;

/* Deplacement des Segments de piste */
	TRACK * track;
	track = m_Pcb->m_Track;
	while( track )
		{
		if( IsSegmentInBox(GetScreen()->BlockLocate, track ) )
			{	/* la piste est ici bonne a etre deplacee */
			m_Pcb->m_Status_Pcb = 0 ;
			track->Draw(DrawPanel, DC, GR_XOR) ; // effacement
			track->m_Start.x += deltaX ; track->m_Start.y += deltaY ;
			track->m_End.x += deltaX ; track->m_End.y += deltaY ;
			track->m_Param += deltaX ; track->m_Sous_Netcode += deltaY ;
			track->Draw(DrawPanel, DC, GR_OR) ; // reaffichage
			}
		track = (TRACK*) track->Pnext;
		}

	/* Deplacement des Segments de Zone */
	track = (TRACK*)m_Pcb->m_Zone;
	while( track )
		{
		if( IsSegmentInBox(GetScreen()->BlockLocate, track ) )
			{  /* la piste est ici bonne a etre deplacee */
			track->Draw(DrawPanel, DC, GR_XOR) ; // effacement
			track->m_Start.x += deltaX ; track->m_Start.y += deltaY ;
			track->m_End.x += deltaX ; track->m_End.y += deltaY ;
			track->m_Param += deltaX ; track->m_Sous_Netcode += deltaY ;
			track->Draw(DrawPanel, DC, GR_OR) ; // reaffichage
			}
		track = (TRACK*) track->Pnext;
		}

	DrawPanel->Refresh(TRUE);
}


/**************************************************/
void WinEDA_BasePcbFrame::Block_Duplicate(wxDC * DC)
/**************************************************/
/*
	routine de duplication des elements du block deja selectionne
*/
{
int deltaX, deltaY;
wxPoint oldpos;

	oldpos = GetScreen()->m_Curseur;
	DrawPanel->ManageCurseur = NULL;

	GetScreen()->m_Curseur = oldpos;
	DrawPanel->MouseToCursorSchema();
	GetScreen()->SetModify();
	GetScreen()->BlockLocate.Normalize();


	/* calcul du vecteur de deplacement pour les deplacements suivants */
	deltaX = GetScreen()->BlockLocate.m_MoveVector.x ;
	deltaY = GetScreen()->BlockLocate.m_MoveVector.y ;

	/* Deplacement des Segments de piste */
	TRACK * track, *next_track, *new_track;

	track = m_Pcb->m_Track;
	while( track )
	{
		next_track = track->Next();
		if( IsSegmentInBox(GetScreen()->BlockLocate, track ) )
		{	/* la piste est ici bonne a etre deplacee */
			m_Pcb->m_Status_Pcb = 0 ;
			new_track = track->Copy(1);
			new_track->Insert(m_Pcb,NULL);
			new_track->m_Start.x += deltaX ; new_track->m_Start.y += deltaY ;
			new_track->m_End.x += deltaX ; new_track->m_End.y += deltaY ;
			new_track->Draw(DrawPanel, DC, GR_OR) ; // reaffichage
		}
		track = next_track;
	}

	/* Deplacement des Segments de Zone */
	track = (TRACK*)m_Pcb->m_Zone;
	while( track )
	{
		next_track = track->Next();
		if( IsSegmentInBox(GetScreen()->BlockLocate, track ) )
		{  /* la piste est ici bonne a etre deplacee */
			new_track = new TRACK(m_Pcb);
			new_track = track->Copy(1);
			new_track->Insert(m_Pcb,NULL);
			new_track->m_Start.x += deltaX ; new_track->m_Start.y += deltaY ;
			new_track->m_End.x += deltaX ; new_track->m_End.y += deltaY ;
			new_track->Draw(DrawPanel, DC, GR_OR) ; // reaffichage
		}
		track = next_track;
	}
}


/**************************************************************************/
static TRACK * IsSegmentInBox( DrawBlockStruct & blocklocate, TRACK * PtSegm )
/**************************************************************************/
/* Teste si la structure PtStruct est inscrite dans le block selectionne
	Retourne PtSegm si oui
			NULL si non
*/
{
	if ( blocklocate.Inside(PtSegm->m_Start.x ,PtSegm->m_Start.y) )
		return ( PtSegm );

	if ( blocklocate.Inside(PtSegm->m_End.x, PtSegm->m_End.y) )
		return ( PtSegm );

	return ( NULL );
}

