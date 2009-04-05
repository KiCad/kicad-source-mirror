/*********************************************************************/
/** setvisu.cpp: initialisations de l'ecran d'affichage du composant **/
/*********************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "id.h"

#include "3d_viewer.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


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
        DrawFrame = new WinEDA_DisplayFrame( this, _( "Module" ),
                                             wxPoint( 0, 0 ),
                                             wxSize( 600, 400 ),
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

        if( DrawFrame->GetBoard()->m_Modules.GetCount() )
        {
            // there is only one module in the list
            DrawFrame->GetBoard()->m_Modules.DeleteAll();
        }

        MODULE* mod = DrawFrame->Get_Module( FootprintName );
        if( mod )
            DrawFrame->GetBoard()->m_Modules.PushBack( mod );

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
    if( !GetBoard() )
        return;

    ActiveScreen = (PCB_SCREEN*) GetScreen();

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );
    GetBoard()->Draw( DrawPanel, DC, GR_COPY );

    MODULE* Module = GetBoard()->m_Modules;
    if ( Module )
        Module->Display_Infos( this );
    UpdateStatusBar();
    DrawPanel->Trace_Curseur( DC );
}


/********************************************************************/
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset )
/********************************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
{
    if( m_Modules )
    {
        m_Modules->Draw( aPanel, DC, GR_COPY );
   }
}
