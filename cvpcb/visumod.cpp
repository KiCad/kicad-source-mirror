/************************************************/
/* Routines de visualisation du module courant  */
/************************************************/


#include "fctsys.h"
#include "common.h"
#include "cvpcb.h"
#include "macros.h"
#include "pcbnew.h"

#include "protos.h"

/* defines locaux */

/* Variables locales */


/*******************************************************************/
void WinEDA_DisplayFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/
/* Affiche le module courant */
{
    if( !m_Pcb )
        return;

    MODULE* Module = m_Pcb->m_Modules;

    ActiveScreen = (PCB_SCREEN*) GetScreen();

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    if( Module )
    {
        Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_COPY );
        Module->Display_Infos( this );
    }

    Affiche_Status_Box();
    DrawPanel->Trace_Curseur( DC );
}
