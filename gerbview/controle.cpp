		/********************************************************/
		/* Routines generales de gestion des commandes usuelles */
		/********************************************************/

	/* fichier controle.cpp */
/*
 Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
*/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "id.h"
#include "protos.h"

/* Routines Locales : */

/* Variables Locales */


/**********************************************************************/
EDA_BaseStruct * WinEDA_GerberFrame::GerberGeneralLocateAndDisplay(void)
/**********************************************************************/
{
	return Locate(CURSEUR_OFF_GRILLE);
}



/****************************************************************/
void WinEDA_BasePcbFrame::GeneralControle(wxDC *DC, wxPoint Mouse)
/****************************************************************/
/* traitement des touches de fonctions utilisees ds tous les menus
	Zoom
	Redessin d'ecran
	Cht Unites
	Cht couches
	Remise a 0 de l'origine des coordonnees relatives
*/
{
wxSize delta;
wxPoint curpos, oldpos;
int hotkey = 0;

	if( GetScreen()->IsRefreshReq() )
		{
		RedrawActiveWindow(DC, TRUE);
                // We must return here, instead of proceeding.
                // If we let the cursor move during a refresh request,
                // the cursor be displayed in the wrong place
                // during delayed repaint events that occur when
                // you move the mouse when a message dialog is on
                // the screen, and then you dismiss the dialog by
                // typing the Enter key.
                return;
		}

	curpos = DrawPanel->CursorRealPosition(Mouse);
	oldpos = GetScreen()->m_Curseur;

	delta.x = GetScreen()->GetGrid().x / GetScreen()->GetZoom();
	delta.y = GetScreen()->GetGrid().y / GetScreen()->GetZoom();
	if( delta.x == 0 ) delta.x = 1;
	if( delta.y == 0 ) delta.y = 1;

	switch(g_KeyPressed)
		{
		case WXK_NUMPAD_SUBTRACT :
		case WXK_SUBTRACT :
		case '-' :
			if(GetScreen()->m_Active_Layer > 0)
				GetScreen()->m_Active_Layer--;
			break ;

		case WXK_NUMPAD_ADD :
		case WXK_ADD :
		case '+' :
			if(GetScreen()->m_Active_Layer < 31)
				GetScreen()->m_Active_Layer ++;
			break ;

		case 'F' | GR_KB_CTRL :
		case 'f' | GR_KB_CTRL :
			DisplayOpt.DisplayPcbTrackFill ^= 1; DisplayOpt.DisplayPcbTrackFill &= 1 ;
			GetScreen()->SetRefreshReq();
			break ;

		case ' ' : /* Mise a jour de l'origine des coord relatives */
			GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
			break ;


		case 'U' | GR_KB_CTRL :
		case 'u' | GR_KB_CTRL :
			if (g_UnitMetric == INCHES ) g_UnitMetric = MILLIMETRE ;
			else 	g_UnitMetric = INCHES ;
			break ;

		case EDA_PANNING_UP_KEY :
			OnZoom(ID_ZOOM_PANNING_UP);
			curpos = m_CurrentScreen->m_Curseur;
			break;

		case EDA_PANNING_DOWN_KEY :
			OnZoom(ID_ZOOM_PANNING_DOWN);
			curpos = m_CurrentScreen->m_Curseur;
			break;

		case EDA_PANNING_LEFT_KEY :
			OnZoom(ID_ZOOM_PANNING_LEFT);
			curpos = m_CurrentScreen->m_Curseur;
			break;

		case EDA_PANNING_RIGHT_KEY :
			OnZoom(ID_ZOOM_PANNING_RIGHT);
			curpos = m_CurrentScreen->m_Curseur;
			break;

		case WXK_F1 :
			OnZoom(ID_ZOOM_PLUS_KEY);
			curpos = GetScreen()->m_Curseur;
			break;

		case WXK_F2 :
			OnZoom(ID_ZOOM_MOINS_KEY);
			curpos = GetScreen()->m_Curseur;
			break;

		case WXK_F3 :
			OnZoom(ID_ZOOM_REDRAW_KEY);
			break;

		case WXK_F4 :
			OnZoom(ID_ZOOM_CENTER_KEY);
			curpos = GetScreen()->m_Curseur;
			break;

		case WXK_NUMPAD8  :	/* Deplacement curseur vers le haut */
		case WXK_UP	:
			Mouse.y -= delta.y;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD2:	/* Deplacement curseur vers le bas */
		case WXK_DOWN:
			Mouse.y += delta.y;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD4:	/* Deplacement curseur vers la gauche */
		case WXK_LEFT :
			Mouse.x -= delta.x;
			DrawPanel->MouseTo(Mouse);
			break ;

		case WXK_NUMPAD6:  /* Deplacement curseur vers la droite */
		case WXK_RIGHT :
			Mouse.x += delta.x;
			DrawPanel->MouseTo(Mouse);
			break ;

		default: hotkey = g_KeyPressed;
			break;
		}

	/* Recalcul de la position du curseur schema */
	GetScreen()->m_Curseur = curpos;
	/* Placement sur la grille generale */
	PutOnGrid( & GetScreen()->m_Curseur);

	if ( (oldpos.x != GetScreen()->m_Curseur.x) ||
		 (oldpos.y != GetScreen()->m_Curseur.y) )
	{
		curpos = GetScreen()->m_Curseur;
		GetScreen()->m_Curseur = oldpos;
		DrawPanel->CursorOff(DC);

		GetScreen()->m_Curseur = curpos;
		DrawPanel->CursorOn(DC);

		if(DrawPanel->ManageCurseur)
		{
			DrawPanel->ManageCurseur(DrawPanel, DC, TRUE);
		}
	}

	SetToolbars();
	Affiche_Status_Box();	 /* Affichage des coord curseur */

	if ( hotkey )
	{
		OnHotKey(DC, hotkey, NULL);
	}
}


