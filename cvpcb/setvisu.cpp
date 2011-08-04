/*****************/
/** setvisu.cpp **/
/*****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "cvpcb_mainframe.h"
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
void CVPCB_MAINFRAME::CreateScreenCmp()
{
    wxString msg, FootprintName;
    bool     IsNew = false;

    FootprintName = m_FootprintList->GetSelectedFootprint();

    if( m_DisplayFootprintFrame == NULL )
    {
        m_DisplayFootprintFrame = new DISPLAY_FOOTPRINTS_FRAME( this, _( "Module" ),
                                                  wxPoint( 0, 0 ),
                                                  wxSize( 600, 400 ),
                                                  KICAD_DEFAULT_DRAWFRAME_STYLE );
        IsNew = true;
        m_DisplayFootprintFrame->Show( true );
    }
    else
    {
        // Raising the window does not show the window on Windows if iconized.
        // This should work on any platform.
        if( m_DisplayFootprintFrame->IsIconized() )
             m_DisplayFootprintFrame->Iconize( false );
        m_DisplayFootprintFrame->Raise();

        // Raising the window does not set the focus on Linux.  This should work on any platform.
        if( wxWindow::FindFocus() != m_DisplayFootprintFrame )
            m_DisplayFootprintFrame->SetFocus();
    }

    if( !FootprintName.IsEmpty() )
    {
        msg = _( "Footprint: " ) + FootprintName;
        m_DisplayFootprintFrame->SetTitle( msg );
        FOOTPRINT_INFO* Module = m_footprints.GetModuleInfo( FootprintName );
        msg = _( "Lib: " );

        if( Module )
            msg += Module->m_LibName;
        else
            msg += wxT( "???" );

        m_DisplayFootprintFrame->SetStatusText( msg, 0 );

        if( m_DisplayFootprintFrame->GetBoard()->m_Modules.GetCount() )
        {
            // there is only one module in the list
            m_DisplayFootprintFrame->GetBoard()->m_Modules.DeleteAll();
        }

        MODULE* mod = m_DisplayFootprintFrame->Get_Module( FootprintName );

        if( mod )
            m_DisplayFootprintFrame->GetBoard()->m_Modules.PushBack( mod );

        m_DisplayFootprintFrame->Zoom_Automatique( false );
        m_DisplayFootprintFrame->DrawPanel->Refresh();
        m_DisplayFootprintFrame->UpdateStatusBar();    /* Display new cursor coordinates and zoom value */

        if( m_DisplayFootprintFrame->m_Draw3DFrame )
            m_DisplayFootprintFrame->m_Draw3DFrame->NewDisplay();
    }
    else if( !IsNew )
    {
        m_DisplayFootprintFrame->Refresh();

        if( m_DisplayFootprintFrame->m_Draw3DFrame )
            m_DisplayFootprintFrame->m_Draw3DFrame->NewDisplay();
    }
}



/*
 * Draws the current highlighted footprint.
 */
void DISPLAY_FOOTPRINTS_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    DrawPanel->DrawBackGround( DC );
    GetBoard()->Draw( DrawPanel, DC, GR_COPY );

    MODULE* Module = GetBoard()->m_Modules;

    if ( Module )
        Module->DisplayInfo( this );

    DrawPanel->DrawCrossHair( DC );
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

/* dummy_functions:
 *
 *  These functions are used in some classes.
 *  they are useful in pcbnew, but have no meaning or are never used
 *  in cvpcb or gerbview.
 *  but they must exist because they appear in some classes.
 *  Do nothing in CvPcb.
 */
TRACK* Marque_Une_Piste( BOARD* aPcb,
                         TRACK* aStartSegm,
                         int*   aSegmCount,
                         int*   aTrackLen,
                         bool   aReorder )
{
    return NULL;
}
