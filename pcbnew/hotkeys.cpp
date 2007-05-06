	/***************/
	/* hotkeys.cpp */
	/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "id.h"

#include "protos.h"

/* Routines locales */

/* variables externes */

/***********************************************************/
void WinEDA_PcbFrame::OnHotKey(wxDC * DC, int hotkey,
					EDA_BaseStruct * DrawStruct)
/***********************************************************/
/* Gestion des commandes rapides (Raccourcis claviers) concernant l'element
sous le courseur souris
 Les majuscules/minuscules sont indifferenciees
	touche DELETE: Effacement (Module ou piste selon commande en cours)
	touche V: Place via en cours de trace de piste
	touche R: Rotation module
	touche S: Change couche module (Composant <-> Cuivre)
	touche M: Start Move module
	touche G: Start Drag module
*/
{
bool PopupOn = GetScreen()->m_CurrentItem  &&
			GetScreen()->m_CurrentItem->m_Flags;
bool ItemFree = (GetScreen()->m_CurrentItem == 0 )  ||
			(GetScreen()->m_CurrentItem->m_Flags == 0);

	if ( hotkey == 0 ) return;

MODULE* module = NULL;

	switch (hotkey)
	{
		case WXK_DELETE:
		case WXK_NUMPAD_DELETE:
			OnHotkeyDeleteItem(DC, DrawStruct);
			break;

		case 'v':	// Rotation
		case 'V':
			if ( m_ID_current_state != ID_TRACK_BUTT ) return;
			if ( ItemFree )
			{
				Other_Layer_Route( NULL, DC);
				break;
			}
			if ( GetScreen()->m_CurrentItem->m_StructType != TYPETRACK )
				return;
			if ( (GetScreen()->m_CurrentItem->m_Flags & IS_NEW) == 0 )
				return;
			Other_Layer_Route( (TRACK *) GetScreen()->m_CurrentItem, DC);
			if ( DisplayOpt.ContrastModeDisplay )
				GetScreen()->SetRefreshReq();
			break;

		case 'r':	// Rotation
		case 'R':
			if ( ItemFree )
				module = Locate_Prefered_Module(m_Pcb, CURSEUR_ON_GRILLE);
			else if (GetScreen()->m_CurrentItem->m_StructType == TYPEMODULE)
				module = (MODULE*)GetScreen()->m_CurrentItem;
			if ( module )
				{
				GetScreen()->m_CurrentItem = module;
				module->Display_Infos(this);
				Rotate_Module(DC, module, 900, TRUE);
				}
			break;

		case 's':	// move to other side
		case 'S':
			if ( ItemFree )
				module = Locate_Prefered_Module(m_Pcb, CURSEUR_ON_GRILLE);
			else if (GetScreen()->m_CurrentItem->m_StructType == TYPEMODULE)
				module = (MODULE*)GetScreen()->m_CurrentItem;
			if ( module )
				{
				GetScreen()->m_CurrentItem = module;
				module->Display_Infos(this);
				Change_Side_Module(module, DC);
				}
			break;

		case 'g':
		case 'G':	// Start move (and drag) module
			g_Drag_Pistes_On = TRUE;
		case 'm':
		case 'M':	// Start move module
			if ( PopupOn ) break;
			if ( (module = Locate_Prefered_Module(m_Pcb, CURSEUR_ON_GRILLE)) != NULL)
			{
				GetScreen()->m_CurrentItem = module;
				module->Display_Infos(this);
				StartMove_Module( module, DC);
			}
			break;
	}
}




/***********************************************************/
void WinEDA_ModuleEditFrame::OnHotKey(wxDC * DC, int hotkey,
					EDA_BaseStruct * DrawStruct)
/***********************************************************/
/* Gestion des commandes rapides (Raccourcis claviers) concernant l'element
sous le courseur souris
 Les majuscules/minuscules sont indifferenciees
*/
{
bool PopupOn = GetScreen()->m_CurrentItem  &&
			GetScreen()->m_CurrentItem->m_Flags;
	if ( hotkey == 0 ) return;

	switch (hotkey)
	{
		case WXK_DELETE:
		case WXK_NUMPAD_DELETE:
			if ( PopupOn ) break;
			break;

		case 'r':	// Rotation
		case 'R':
			break;

		case 'y':	// Mirror Y (drawlibpart)
		case 'Y':
			break;

		case 'x':	// Mirror X (drawlibpart)
		case 'X':
			break;

		case 'n':
		case 'N':	// Orient 0, no mirror (drawlibpart)
			break;

		case 'm':
		case 'M':	// Start move drawlibpart
			if ( PopupOn ) break;
			break;
	}
}

/******************************************************************************/
bool WinEDA_PcbFrame::OnHotkeyDeleteItem(wxDC * DC, EDA_BaseStruct * DrawStruct)
/******************************************************************************/
/* Efface l'item pointe par la souris, en reponse a la touche "Del"
	Effet dependant de l'outil selectionne:
		Outil trace de pistes
			Efface le segment en cours ou la piste si pas d'element
		Outil module:
			Efface le module.
*/
{
bool ItemFree = (GetScreen()->m_CurrentItem == NULL )  ||
			(GetScreen()->m_CurrentItem->m_Flags == 0);

	switch ( m_ID_current_state )
		{
		case ID_TRACK_BUTT:
			if ( GetScreen()->m_Active_Layer > CMP_N ) return FALSE;
			if ( ItemFree )
			{
				DrawStruct = PcbGeneralLocateAndDisplay();
				if ( DrawStruct && DrawStruct->m_StructType != TYPETRACK ) return FALSE;
				Delete_Track(DC, (TRACK*)DrawStruct);
			}
			else if ( GetScreen()->m_CurrentItem->m_StructType == TYPETRACK  )
			{
			GetScreen()->m_CurrentItem =
				Delete_Segment(DC, (TRACK*)GetScreen()->m_CurrentItem);
			GetScreen()->SetModify();
			return TRUE;
			}
			break;

		case ID_COMPONENT_BUTT:
			if ( ItemFree )
			{
			MODULE * module = Locate_Prefered_Module(m_Pcb, CURSEUR_ON_GRILLE);
				if ( module == NULL ) return FALSE;
				RemoveStruct(module, DC);
			}
			else return FALSE;
			break;

		default:
			return FALSE;
		}

	GetScreen()->SetModify();
	GetScreen()->m_CurrentItem = NULL;
	return TRUE;
}
