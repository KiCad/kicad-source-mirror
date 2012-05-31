/**
 * @file setvisu.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_module.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <class_DisplayFootprintsFrame.h>

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occurring.
 */
#include <3d_viewer.h>


/*
 * Create or Update the frame showing the current highlighted footprint
 * and (if showed) the 3D display frame
 */
void CVPCB_MAINFRAME::CreateScreenCmp()
{
    if( m_DisplayFootprintFrame == NULL )
    {
        m_DisplayFootprintFrame = new DISPLAY_FOOTPRINTS_FRAME( this, _( "Module" ),
                                                  wxPoint( 0, 0 ),
                                                  wxSize( 600, 400 ),
                                                  KICAD_DEFAULT_DRAWFRAME_STYLE );
        m_DisplayFootprintFrame->Show( true );
    }
    else
    {
        if( m_DisplayFootprintFrame->IsIconized() )
             m_DisplayFootprintFrame->Iconize( false );
    }

    m_DisplayFootprintFrame->InitDisplay();
}

/* Refresh the full display for this frame:
 * Set the title, the status line and redraw the canvas
 * Must be called after the footprint to display is modifed
 */
void DISPLAY_FOOTPRINTS_FRAME::InitDisplay()
{
    wxString msg;
    CVPCB_MAINFRAME * parentframe = (CVPCB_MAINFRAME *) GetParent();
    wxString footprintName = parentframe->m_FootprintList->GetSelectedFootprint();

    if( !footprintName.IsEmpty() )
    {
        msg = _( "Footprint: " ) + footprintName;
        SetTitle( msg );
        FOOTPRINT_INFO* module_info = parentframe->m_footprints.GetModuleInfo( footprintName );
        msg = _( "Lib: " );

        if( module_info )
            msg += module_info->m_LibName;
        else
            msg += wxT( "???" );

        SetStatusText( msg, 0 );

        if( GetBoard()->m_Modules.GetCount() )
        {
            // there is only one module in the list
            GetBoard()->m_Modules.DeleteAll();
        }

        MODULE* module = Get_Module( footprintName );

        if( module )
            GetBoard()->m_Modules.PushBack( module );

        Zoom_Automatique( false );

    }
    else   // No footprint to display. Erase old footprint, if any
    {
        if( GetBoard()->m_Modules.GetCount() )
        {
            GetBoard()->m_Modules.DeleteAll();
            Zoom_Automatique( false );
            SetStatusText( wxEmptyString, 0 );
        }
    }

    // Display new cursor coordinates and zoom value:
    UpdateStatusBar();

    GetCanvas()->Refresh();

    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();
}

/*
 * Draws the current highlighted footprint.
 */
void DISPLAY_FOOTPRINTS_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* Module = GetBoard()->m_Modules;

    if ( Module )
        Module->DisplayInfo( this );

    m_canvas->DrawCrossHair( DC );
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
