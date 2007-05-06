/********************************************************************************/
/* tool_onrightclick.cpp: fonctions appelées par le bouton droit sur un TOOL */
/********************************************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "id.h"

#include "protos.h"



/*****************************************************************/
void WinEDA_PcbFrame::ToolOnRightClick(wxCommandEvent& event)
/*****************************************************************/
{
wxPoint pos;
int id = event.GetSelection();

	wxGetMousePosition(&pos.x, &pos.y);
	pos.x -= 400;
	pos.y -= 30;

	switch ( id )
		{
		case ID_TRACK_BUTT:
			InstallPcbOptionsFrame(pos, NULL, ID_PCB_TRACK_SIZE_SETUP);
			break;

		case ID_COMPONENT_BUTT:
			break;

		case ID_PCB_CIRCLE_BUTT:
		case ID_PCB_ARC_BUTT:
		case ID_LINE_COMMENT_BUTT:
		case ID_PCB_COTATION_BUTT:
		case ID_TEXT_COMMENT_BUTT:
			InstallPcbOptionsFrame(pos, NULL, ID_PCB_DRAWINGS_WIDTHS_SETUP);
			break;

		default:
			break;
		}
}

/************************************************************************/
void WinEDA_ModuleEditFrame::ToolOnRightClick(wxCommandEvent& event)
/************************************************************************/
{
wxPoint pos;
int id = event.GetSelection();

	wxGetMousePosition(&pos.x, &pos.y);
	pos.x -= 400;
	pos.y -= 30;

	switch ( id )
		{
		case ID_MODEDIT_ADD_PAD:
			InstallPadOptionsFrame(NULL, NULL, wxPoint(-1,-1) );
			break;

		case ID_PCB_CIRCLE_BUTT:
		case ID_PCB_ARC_BUTT:
		case ID_LINE_COMMENT_BUTT:
		case ID_PCB_COTATION_BUTT:
		case ID_TEXT_COMMENT_BUTT:
			InstallOptionsFrame(pos);
			break;

		default:
			DisplayError(this, wxT("ToolOnRightClick() error"));
			break;
		}
}


