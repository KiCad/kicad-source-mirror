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

/* Create or Update the frame showing the current highlighted footprint
 *  and (if showed) the 3D display frame
 */
{
    wxString msg, FootprintName;
    bool     IsNew = FALSE;

    FootprintName = m_FootprintList->GetSelectedFootprint();

    if( DrawFrame == NULL )
    {
        DrawFrame = new WinEDA_DisplayFrame( this, m_Parent, _( "Module" ),
                                             wxPoint( 0, 0 ), wxSize( 600,
                                                                      400 ),
                                             KICAD_DEFAULT_DRAWFRAME_STYLE |
                                             wxFRAME_FLOAT_ON_PARENT );
        IsNew = TRUE;
        DrawFrame->Show( TRUE );
    }

    if( !FootprintName.IsEmpty() )
    {
        msg = _( "Footprint: " ) + FootprintName;
        DrawFrame->SetTitle( msg );
        STOREMOD* Module = GetModuleDescrByName( FootprintName );
        msg = _( "Lib: " );

        if( Module )
            msg += Module->m_LibName;
        else
            msg += wxT( "???" );

        DrawFrame->SetStatusText( msg, 0 );

        if( DrawFrame->m_Pcb->m_Modules )
        {
            // there is only one module in the list
            DrawFrame->m_Pcb->m_Modules->DeleteStructure();

            DrawFrame->m_Pcb->m_Modules = NULL;
        }

        DrawFrame->m_Pcb->m_Modules = DrawFrame->Get_Module( FootprintName );

        DrawFrame->Zoom_Automatique( FALSE );
        if( DrawFrame->m_Draw3DFrame )
            DrawFrame->m_Draw3DFrame->NewDisplay();
    }
    else if( !IsNew )
    {
        DrawFrame->Refresh();
        if( DrawFrame->m_Draw3DFrame )
            DrawFrame->m_Draw3DFrame->NewDisplay();
    }
}



/*******************************************************************/
void WinEDA_DisplayFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/
/* Draws the current highlighted footprint */
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
        Module->Draw( DrawPanel, DC, GR_COPY );
        Module->Display_Infos( this );
    }

    Affiche_Status_Box();
    DrawPanel->Trace_Curseur( DC );
}
