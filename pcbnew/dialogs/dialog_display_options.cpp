/**
 * @file pcbnew/dialogs/dialog_general_options.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
    GetSizer()->SetSizeHints( this );
}

void DIALOG_DISPLAY_OPTIONS::init()
{
    SetFocus();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    m_OptDisplayTracks->SetValue( displ_opts->m_DisplayPcbTrackFill == SKETCH );

    switch ( displ_opts->m_ShowTrackClearanceMode )
    {
        case DO_NOT_SHOW_CLEARANCE:
            m_OptDisplayTracksClearance->SetSelection( 0 );
            break;

        case SHOW_CLEARANCE_NEW_TRACKS:
            m_OptDisplayTracksClearance->SetSelection( 1 );
            break;

        case SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS:
            m_OptDisplayTracksClearance->SetSelection( 3 );
            break;

        default:
        case SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS:
            m_OptDisplayTracksClearance->SetSelection( 2 );
            break;

        case SHOW_CLEARANCE_ALWAYS:
            m_OptDisplayTracksClearance->SetSelection( 4 );
            break;
    }

    m_OptDisplayPads->SetValue( displ_opts->m_DisplayPadFill == SKETCH );
    m_OptDisplayVias->SetValue( displ_opts->m_DisplayViaFill == SKETCH );

    m_Show_Page_Limits->SetValue( m_Parent->ShowPageLimits() );

    m_OptDisplayViaHole->SetSelection( displ_opts->m_DisplayViaMode );
    m_OptDisplayModTexts->SetValue( displ_opts->m_DisplayModTextFill == SKETCH );
    m_OptDisplayModOutlines->SetValue( displ_opts->m_DisplayModEdgeFill == SKETCH );
    m_OptDisplayPadClearence->SetValue( displ_opts->m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts->m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_Parent->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) ) );
    m_OptDisplayDrawings->SetValue( displ_opts->m_DisplayDrawItemsFill == SKETCH );
    m_ShowNetNamesOption->SetSelection( displ_opts->m_DisplayNetNamesMode );
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

    m_Parent->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    displ_opts->m_DisplayPcbTrackFill = not m_OptDisplayTracks->GetValue();

    displ_opts->m_DisplayViaMode = (VIA_DISPLAY_MODE_T) m_OptDisplayViaHole->GetSelection();

    switch ( m_OptDisplayTracksClearance->GetSelection() )
    {
        case 0:
            displ_opts->m_ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
            break;

        case 1:
            displ_opts->m_ShowTrackClearanceMode = SHOW_CLEARANCE_NEW_TRACKS;
            break;

        case 2:
            displ_opts->m_ShowTrackClearanceMode = SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS;
            break;

        case 3:
            displ_opts->m_ShowTrackClearanceMode = SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS;
            break;

        case 4:
            displ_opts->m_ShowTrackClearanceMode = SHOW_CLEARANCE_ALWAYS;
            break;
    }

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

    // Apply changes to the GAL
    KIGFX::VIEW* view = m_Parent->GetGalCanvas()->GetView();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( displ_opts );
    view->RecacheAllItems( true );

    m_Parent->GetCanvas()->Refresh();

    EndModal( 1 );
}
