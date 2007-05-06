	/******************************************************/
	/* edit.cpp: fonctions generales de l'edition du PCB */
	/******************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "id.h"

#include "protos.h"

/********************************************************************************/
void WinEDA_GerberFrame::OnRightClick(const wxPoint& MousePos, wxMenu * PopMenu)
/********************************************************************************/
/* Prepare le menu PullUp affiché par un click sur le bouton droit
de la souris.
   Ce menu est ensuite complété par la liste des commandes de ZOOM
*/
{
EDA_BaseStruct *DrawStruct = GetScreen()->m_CurrentItem;
wxString msg;
bool BlockActive = (m_CurrentScreen->BlockLocate.m_Command !=  BLOCK_IDLE);

	DrawPanel->m_CanStartBlock = -1;	// Ne pas engager un debut de bloc sur validation menu

	 // Simple localisation des elements si possible
	if ( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
		{
		DrawStruct = GerberGeneralLocateAndDisplay();
		}

	// Si commande en cours: affichage fin de commande
	if (  m_ID_current_state )
		{
		if ( DrawStruct && DrawStruct->m_Flags )
			PopMenu->Append(ID_POPUP_CANCEL_CURRENT_COMMAND, _("Cancel"));
		else PopMenu->Append(ID_POPUP_CLOSE_CURRENT_TOOL, _("End Tool"));
		PopMenu->AppendSeparator();
		}

	else
	{
		if ( (DrawStruct && DrawStruct->m_Flags) || BlockActive )
		{
			if ( BlockActive )
			{
				PopMenu->Append(ID_POPUP_CANCEL_CURRENT_COMMAND, _("Cancel Block") );
				PopMenu->Append(ID_POPUP_ZOOM_BLOCK, _("Zoom Block (Midd butt drag)") );
				PopMenu->AppendSeparator();
				PopMenu->Append(ID_POPUP_PLACE_BLOCK, _("Place Block") );
				PopMenu->Append(ID_POPUP_COPY_BLOCK, _("Copy Block (shift mouse)") );
				PopMenu->Append(ID_POPUP_DELETE_BLOCK, _("Delete Block (ctrl + drag mouse)") );
			}
			else PopMenu->Append(ID_POPUP_CANCEL_CURRENT_COMMAND, _("Cancel"));
			PopMenu->AppendSeparator();
		}
	}

	if ( BlockActive ) return;

	if ( DrawStruct == NULL ) return;

	GetScreen()->m_CurrentItem = DrawStruct;

	switch ( DrawStruct->m_StructType )
		{

		case TYPETRACK:
//			PopMenu->Append(ID_POPUP_PCB_EDIT_TRACK, _("Edit"));
//			PopMenu->Append(ID_POPUP_PCB_DELETE_TRACKSEG, _("Delete"));
			break;


		default:
			msg.Printf(
				wxT("WinEDA_GerberFrame::OnRightClick Error: iilegal or unknown DrawType %d"),
				DrawStruct->m_StructType);
			DisplayError(this, msg );
			break;
		}
	PopMenu->AppendSeparator();
}

