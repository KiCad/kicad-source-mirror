/*******************/
/* Display modules */
/*******************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "drag.h"

/* Font of characters for the trace text routine. */
extern char* graphic_fonte_shape[];

#include "protos.h"

#define L_MIN_DESSIN 1   /* line width for segments other than traces. */


/* Trace the pads of a module in sketch mode.
 * Used to display a module pads when it is not displayed by the display
 * options Module setting.
 *
 * The pads posters must appear on the data layers by MasqueLayer
 */
void Trace_Pads_Only( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module,
                      int ox, int oy, int MasqueLayer, int draw_mode )
{
    int                  tmp;
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;

    screen = (PCB_SCREEN*) panel->GetScreen();
    frame  = (WinEDA_BasePcbFrame*) panel->GetParent();

    tmp = frame->m_DisplayPadFill;
    frame->m_DisplayPadFill = FALSE;

    /* Draw pads. */
    for( D_PAD* pad = Module->m_Pads;  pad;  pad = pad->Next() )
    {
        if( (pad->m_Masque_Layer & MasqueLayer) == 0 )
            continue;

        pad->Draw( panel, DC, draw_mode, wxPoint( ox, oy ) );
    }

    frame->m_DisplayPadFill = tmp;
}
