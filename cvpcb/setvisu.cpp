/*********************************************************************/
/** setvisu.cpp: initialisations de l'ecran d'affichage du composant **/
/*********************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "id.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occuring.
 */
#include "3d_viewer.h"



/*
 * Create or Update the frame showing the current highlighted footprint
 * and (if showed) the 3D display frame
 */
void WinEDA_CvpcbFrame::CreateScreenCmp()
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
        FOOTPRINT* Module = GetModuleDescrByName( FootprintName, m_footprints );
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
        DrawFrame->UpdateStatusBar();    /* Display new cursor coordinates and zoom value */
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



/*
 * Draws the current highlighted footprint.
 */
void WinEDA_DisplayFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
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
        Module->DisplayInfo( this );
    DrawPanel->Trace_Curseur( DC );
}



/*
 * Redraw the BOARD items but not cursors, axis or grid.
 */
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset )
{
    if( m_Modules )
    {
        m_Modules->Draw( aPanel, DC, GR_COPY );
    }
}
