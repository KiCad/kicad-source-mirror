/*****************/
/** setvisu.cpp **/
/*****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"
#include "class_DisplayFootprintsFrame.h"

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occurring.
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
        DrawFrame = new DISPLAY_FOOTPRINTS_FRAME( this, _( "Module" ),
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
        DrawFrame->DrawPanel->Refresh();
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
void DISPLAY_FOOTPRINTS_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    ActiveScreen = (PCB_SCREEN*) GetScreen();

    DrawPanel->DrawBackGround( DC );
    GetBoard()->Draw( DrawPanel, DC, GR_COPY );

    MODULE* Module = GetBoard()->m_Modules;
    if ( Module )
        Module->DisplayInfo( this );

    DrawPanel->DrawCursor( DC );
}



/*
 * Redraw the BOARD items but not cursors, axis or grid.
 */
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode, const wxPoint& aOffset )
{
    if( m_Modules )
    {
        m_Modules->Draw( aPanel, aDC, GR_COPY );
    }
}
