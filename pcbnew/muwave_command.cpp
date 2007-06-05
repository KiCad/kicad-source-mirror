	/*****************************************************/
	/* muwave_command.cpp: micro wave functions commands */
	/*****************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "id.h"

#include "protos.h"


/*********************************************************************/
void WinEDA_PcbFrame::ProcessMuWaveFunctions(wxCommandEvent& event)
/*********************************************************************/
/* Traite les selections d'outils et les commandes appelees du menu POPUP
*/
{
int id = event.GetId();
wxPoint pos;
wxClientDC dc(DrawPanel);

	DrawPanel->PrepareGraphicContext(&dc);

	wxGetMousePosition(&pos.x, &pos.y);

	pos.y += 20;

	switch ( id )	// Arret eventuel de la commande de déplacement en cours
	{
		case ID_POPUP_COPY_BLOCK:
			break;

		default:	// Arret de la commande de déplacement en cours
			if( DrawPanel->ManageCurseur &&
				DrawPanel->ForceCloseManageCurseur )
			{
				DrawPanel->ForceCloseManageCurseur(DrawPanel, &dc);
			}
			SetToolID(0, wxCURSOR_ARROW,wxEmptyString);
			break;
	}

	switch ( id )	// Traitement des commandes
	{

		case ID_PCB_MUWAVE_TOOL_SELF_CMD:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Line"));
			break;

		case ID_PCB_MUWAVE_TOOL_GAP_CMD:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Gap"));
			break;

		case ID_PCB_MUWAVE_TOOL_STUB_CMD:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Stub"));
			break;

		case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Arc Stub"));
			break;

		case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
			SetToolID( id, wxCURSOR_PENCIL, _("Add Polynomial Shape"));
			break;

		default:
			DisplayError(this, wxT("WinEDA_PcbFrame::ProcessMuWaveFunctions() id error"));
			break;
	}

	SetToolbars();
}



/***************************************************************************/
void WinEDA_PcbFrame::MuWaveCommand(wxDC * DC, const wxPoint& MousePos)
/***************************************************************************/
{
	switch ( m_ID_current_state )
	{
		case ID_PCB_MUWAVE_TOOL_SELF_CMD:
			Begin_Self(DC);
			break;

		case ID_PCB_MUWAVE_TOOL_GAP_CMD:
			Create_MuWaveComponent(DC, 0);
			break;

		case ID_PCB_MUWAVE_TOOL_STUB_CMD:
			Create_MuWaveComponent(DC, 1);
			break;

		case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
			Create_MuWaveComponent(DC, 2);
			break;

		case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
			Create_MuWavePolygonShape(DC);
			break;

		default :
			DrawPanel->SetCursor(wxCURSOR_ARROW);
			DisplayError(this, wxT("WinEDA_PcbFrame::MuWaveCommand() id error"));
			SetToolID(0, wxCURSOR_ARROW,wxEmptyString);
			break;
	}
}

