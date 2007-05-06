		/**********************************************/
		/* Routine de selection de couches pour trace */
		/**********************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"


/* Variables locales : */

/* Routines Locales */
	/****************************/
	/* int GetLayerNumber(void) */
	/****************************/
/* retourne le nombre de couches a tracer
*/
int GetLayerNumber(void)
{
int ii = 0;
//TO REDO
return ii;
}


/*******************************************************************************/
void Print_PcbItems(BOARD * Pcb, wxDC *DC, int drawmode, int printmasklayer)
/*******************************************************************************/
/* routine de trace du pcb, avec selection des couches */
{
DISPLAY_OPTIONS save_opt;
TRACK * pt_piste;

	save_opt = DisplayOpt;
	DisplayOpt.DisplayPadFill = FILLED;
	DisplayOpt.DisplayPadNum = 0;
	DisplayOpt.DisplayPadNoConn = 0;
	DisplayOpt.DisplayPadIsol = 0;
	DisplayOpt.DisplayPcbTrackFill = FILLED;
	DisplayOpt.DisplayTrackIsol = 0;
	DisplayOpt.DisplayDrawItems = FILLED;
	DisplayOpt.DisplayZones = 1;

	/* trace des pistes */
	pt_piste = Pcb->m_Track;
	for ( ; pt_piste != NULL ; pt_piste = (TRACK*) pt_piste->Pnext )
		{
//		if( (printmasklayer & ReturnMaskLayer(pt_piste) ) == 0 ) continue;
		Trace_Segment(NULL, DC, pt_piste, drawmode);
		}

	DisplayOpt = save_opt;
}



