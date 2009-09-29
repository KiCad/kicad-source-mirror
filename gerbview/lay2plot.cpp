        /**********************************************/
        /* Routine de selection de couches pour trace */
        /**********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "gerbview.h"
#include "protos.h"


/* Variables locales : */

/* Routines Locales */

/*******************************************************************************/
void Print_PcbItems(BOARD * Pcb, wxDC *DC, int drawmode, int printmasklayer)
/*******************************************************************************/
/* routine de trace du pcb, avec selection des couches */
{
    DISPLAY_OPTIONS save_opt;
    TRACK * pt_piste;

    save_opt = DisplayOpt;
    DisplayOpt.DisplayPadFill = FILLED;
    DisplayOpt.DisplayViaFill = FILLED;
    DisplayOpt.DisplayPadNum = 0;
    DisplayOpt.DisplayPadNoConn = 0;
    DisplayOpt.DisplayPadIsol = 0;
    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems = FILLED;
    DisplayOpt.DisplayZonesMode = 0;

    /* trace des pistes */
    pt_piste = Pcb->m_Track;
    for( ; pt_piste != NULL ; pt_piste = pt_piste->Next() )
    {
//		if( (printmasklayer & ReturnMaskLayer(pt_piste) ) == 0 ) continue;
        Trace_Segment(NULL, DC, pt_piste, drawmode);
    }

    DisplayOpt = save_opt;
}



