	/************************************************************************/
	/* basepcbframe.cpp - fonctions des classes du type WinEDA_BasePcbFrame */
	/************************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"



	/*******************************/
	/* class WinEDA_BasePcbFrame */
	/*******************************/

	/****************/
	/* Constructeur */
	/****************/

WinEDA_BasePcbFrame::WinEDA_BasePcbFrame(wxWindow * father, WinEDA_App *parent,
					int idtype,
					const wxString & title, const wxPoint& pos, const wxSize& size) :
					WinEDA_DrawFrame(father, idtype, parent, title, pos, size)
{
	m_InternalUnits = 10000;		// Internal unit = 1/10000 inch
	m_CurrentScreen = NULL;
	m_Pcb = NULL;

	m_DisplayPadFill = TRUE;		// How to draw pads
	m_DisplayPadNum = TRUE;			// show pads number

	m_DisplayModEdge = FILLED;		// How to show module drawings
	m_DisplayModText = FILLED;		// How to show module texts
	m_DisplayPcbTrackFill = TRUE;	/* FALSE = sketch , TRUE = filled */
	m_Draw3DFrame = NULL;			// Display Window in 3D mode (OpenGL)
}

WinEDA_BasePcbFrame::~WinEDA_BasePcbFrame(void)
{
}


/**************************************/
int WinEDA_BasePcbFrame::BestZoom(void)
/**************************************/
{
int dx, dy, ii,jj ;
int bestzoom;
wxSize size;

	if ( m_Pcb == NULL ) return 32;

	m_Pcb->ComputeBoundaryBox();

	/* calcul du zoom montrant tout le dessim */
	dx = m_Pcb->m_BoundaryBox.GetWidth();
	dy = m_Pcb->m_BoundaryBox.GetHeight();

	size =  DrawPanel->GetClientSize();
	ii = (dx + (size.x/2)) / size.x;
	jj = (dy + (size.y/2)) / size.y;
	bestzoom = MAX(ii, jj) + 1;

	m_CurrentScreen->m_Curseur = m_Pcb->m_BoundaryBox.Centre();

	return(bestzoom);
}


void WinEDA_BasePcbFrame::ReCreateMenuBar(void)	// fonction virtuelle
{
}

#include "3d_viewer.h"

/***********************************************************/
void WinEDA_BasePcbFrame::Show3D_Frame(wxCommandEvent& event)
/***********************************************************/
/* Ouvre la frame d'affichage 3D
*/
{
#ifndef GERBVIEW
	// Create the main frame window
	if ( m_Draw3DFrame )
	{
		DisplayInfo(this, _("3D Frame already opened") );
		return;
	}
	m_Draw3DFrame = new WinEDA3D_DrawFrame(this, m_Parent, _("3D Viewer") );

	// Show the frame
	m_Draw3DFrame->Show(TRUE);
#endif
}


/* Virtual functions: Do nothing for WinEDA_BasePcbFrame window */

/***********************************************************************************/
void WinEDA_BasePcbFrame::SaveCopyInUndoList(EDA_BaseStruct * ItemToCopy, int flag)
/***********************************************************************************/
{
}

/********************************************************/
void WinEDA_BasePcbFrame::GetComponentFromUndoList(void)
/********************************************************/
{
}

/********************************************************/
void WinEDA_BasePcbFrame::GetComponentFromRedoList(void)
/********************************************************/
{
}

