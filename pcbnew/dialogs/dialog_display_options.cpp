/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* functions relatives to the dialog opened from the main menu :
    Preferences/display
*/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbstruct.h>

#include <pcbnew_id.h>

#include <dialog_display_options.h>
#include <dialog_display_options_base.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <pcb_painter.h>
#include <gal/gal_display_options.h>


static void setRadioFromGridStyle( wxRadioBox& aRBox,
                                   KIGFX::GRID_STYLE aStyle )
{
    aRBox.SetSelection( aStyle != KIGFX::GRID_STYLE::DOTS );
}


static KIGFX::GRID_STYLE getGridStyleFromRadio( const wxRadioBox& aRBox )
{
    return aRBox.GetSelection() == 0 ? KIGFX::GRID_STYLE::DOTS : KIGFX::GRID_STYLE::LINES;
}


static void setRadioFromClearanceMode( wxRadioBox& aCtrl,
                                       TRACE_CLEARANCE_DISPLAY_MODE_T aClearance )
{
    int value = 0;

    switch ( aClearance )
    {
        case DO_NOT_SHOW_CLEARANCE:
            value = 0;
            break;

        case SHOW_CLEARANCE_NEW_TRACKS:
            value = 1;
            break;

        case SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS:
            value = 3;
            break;

        case SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS:
            value = 2;
            break;

        case SHOW_CLEARANCE_ALWAYS:
            value = 4;
            break;


    }

    aCtrl.SetSelection( value );
}


static TRACE_CLEARANCE_DISPLAY_MODE_T getClearanceModeFromRadio(
        const wxRadioBox& aCtrl )
{
    TRACE_CLEARANCE_DISPLAY_MODE_T mode = SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS;

    switch ( aCtrl.GetSelection() )
    {
        case 0:
            mode = DO_NOT_SHOW_CLEARANCE;
            break;

        case 1:
            mode = SHOW_CLEARANCE_NEW_TRACKS;
            break;

        case 2:
            mode = SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS;
            break;

        case 3:
            mode = SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS;
            break;

        case 4:
            mode = SHOW_CLEARANCE_ALWAYS;
            break;
    }

    return mode;
}


static void setCtrlFromAntiAliasMode( wxChoice& aCtrl,
                                      KIGFX::OPENGL_ANTIALIASING_MODE mode )
{
    int value = 0;

    switch( mode )
    {
        case KIGFX::OPENGL_ANTIALIASING_MODE::NONE:
            value = 0;
            break;

        case KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_HIGH:
            value = 1;
            break;

        case KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_ULTRA:
            value = 2;
            break;

        case KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X2:
            value = 3;
            break;

        case KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X4:
            value = 4;
            break;
    }

    aCtrl.Select( value );
}

static KIGFX::OPENGL_ANTIALIASING_MODE getAntiAliasModeFromRadio(
        const wxChoice& aCtrl )
{
    auto mode = KIGFX::OPENGL_ANTIALIASING_MODE::NONE;

    switch( aCtrl.GetSelection() )
    {
        case 0:
            mode = KIGFX::OPENGL_ANTIALIASING_MODE::NONE;
            break;

        case 1:
            mode = KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_HIGH;
            break;

        case 2:
            mode = KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_ULTRA;
            break;

        case 3:
            mode = KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X2;
            break;

        case 4:
            mode = KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X4;
            break;
    }

    return mode;
}


void PCB_EDIT_FRAME::InstallDisplayOptionsDialog( wxCommandEvent& aEvent )
{
    DIALOG_DISPLAY_OPTIONS dlg( this );
    dlg.ShowModal();
}


DIALOG_DISPLAY_OPTIONS::DIALOG_DISPLAY_OPTIONS( PCB_EDIT_FRAME* parent ) :
    DIALOG_DISPLAY_OPTIONS_BASE( parent )
{
    m_Parent = parent;

    init();
    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}

void DIALOG_DISPLAY_OPTIONS::init()
{
    SetFocus();
    const DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();
    const KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = m_Parent->GetGalDisplayOptions();

    m_OptDisplayTracks->SetValue( displ_opts->m_DisplayPcbTrackFill == SKETCH );

    setRadioFromClearanceMode( *m_OptDisplayTracksClearance,
                               displ_opts->m_ShowTrackClearanceMode );

    m_OptDisplayPads->SetValue( displ_opts->m_DisplayPadFill == SKETCH );
    m_OptDisplayVias->SetValue( displ_opts->m_DisplayViaFill == SKETCH );

    m_Show_Page_Limits->SetValue( m_Parent->ShowPageLimits() );

    m_OptDisplayModTexts->SetValue( displ_opts->m_DisplayModTextFill == SKETCH );
    m_OptDisplayModOutlines->SetValue( displ_opts->m_DisplayModEdgeFill == SKETCH );
    m_OptDisplayPadClearence->SetValue( displ_opts->m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts->m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_Parent->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) ) );
    m_OptDisplayDrawings->SetValue( displ_opts->m_DisplayDrawItemsFill == SKETCH );
    m_ShowNetNamesOption->SetSelection( displ_opts->m_DisplayNetNamesMode );

    setCtrlFromAntiAliasMode( *m_choiceAntialiasing,
                              gal_opts.gl_antialiasing_mode );

    setRadioFromGridStyle( *m_gridStyle, gal_opts.m_gridStyle );
}


void DIALOG_DISPLAY_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


/* Update variables with new options
*/
void DIALOG_DISPLAY_OPTIONS::OnOkClick(wxCommandEvent& event)
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = m_Parent->GetGalDisplayOptions();

    m_Parent->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    displ_opts->m_DisplayPcbTrackFill = not m_OptDisplayTracks->GetValue();

    displ_opts->m_ShowTrackClearanceMode = getClearanceModeFromRadio(
            *m_OptDisplayTracksClearance );

    displ_opts->m_DisplayModTextFill = not m_OptDisplayModTexts->GetValue();
    displ_opts->m_DisplayModEdgeFill = not m_OptDisplayModOutlines->GetValue();

    displ_opts->m_DisplayPadFill = not m_OptDisplayPads->GetValue();
    displ_opts->m_DisplayViaFill = not m_OptDisplayVias->GetValue();

    displ_opts->m_DisplayPadIsol = m_OptDisplayPadClearence->GetValue();

    displ_opts->m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    m_Parent->SetElementVisibility( PCB_VISIBLE(NO_CONNECTS_VISIBLE),
                                    m_OptDisplayPadNoConn->GetValue() );

    displ_opts->m_DisplayDrawItemsFill = not m_OptDisplayDrawings->GetValue();
    displ_opts->m_DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();

    gal_opts.gl_antialiasing_mode = getAntiAliasModeFromRadio(
            *m_choiceAntialiasing );

    gal_opts.m_gridStyle = getGridStyleFromRadio( *m_gridStyle );

    gal_opts.NotifyChanged();

    // Apply changes to the GAL
    KIGFX::VIEW* view = m_Parent->GetGalCanvas()->GetView();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( displ_opts );
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    m_Parent->GetCanvas()->Refresh();

    EndModal( 1 );
}
