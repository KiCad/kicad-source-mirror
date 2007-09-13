/*********************************************************************/
/** setvisu.cpp: initialisations de l'ecran d'affichage du composant **/
/*********************************************************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
#include "3d_viewer.h"
#include "id.h"
#include "bitmaps.h"

#include "protos.h"

/*******************************************/
void WinEDA_CvpcbFrame::CreateScreenCmp()
/*******************************************/
/* Creation de la fenetre d'affichage du composant
*/
{
wxString msg, FootprintName;
bool IsNew = FALSE;

	FootprintName = m_FootprintList->GetSelectedFootprint();

	if ( DrawFrame == NULL)
	{
		DrawFrame = new WinEDA_DisplayFrame(this, m_Parent, _("Module"),
						wxPoint(0,0) , wxSize(600,400) );
		IsNew = TRUE;
	}
	else DrawFrame->Maximize(FALSE);

	DrawFrame->SetFocus();	/* Active entree clavier */
	DrawFrame->Show(TRUE);

	if( ! FootprintName.IsEmpty() )
	{
		msg = _("Footprint: ") + FootprintName;
		DrawFrame->SetTitle(msg);
		STOREMOD * Module = GetModuleDescrByName(FootprintName);
		msg = _("Lib: ");
		if ( Module ) msg += Module->m_LibName;
		else msg += wxT("???");
		DrawFrame->SetStatusText(msg, 0);
		if ( DrawFrame->m_Pcb->m_Modules )
		{
			DeleteStructure( DrawFrame->m_Pcb->m_Modules );
			DrawFrame->m_Pcb->m_Modules = NULL;
		}
		DrawFrame->m_Pcb->m_Modules = DrawFrame->Get_Module(FootprintName);
		DrawFrame->Zoom_Automatique(FALSE);
		if ( DrawFrame->m_Draw3DFrame )
			DrawFrame->m_Draw3DFrame->NewDisplay();
	}

	else if ( !IsNew )
	{
		DrawFrame->ReDrawPanel();
		if ( DrawFrame->m_Draw3DFrame )
			DrawFrame->m_Draw3DFrame->NewDisplay();
	}
}


