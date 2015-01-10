/**
 * @file pcbnew/dialogs/dialog_general_options.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

    m_buttonOK->SetDefault();
    GetSizer()->SetSizeHints( this );
}

void DIALOG_DISPLAY_OPTIONS::init()
{
    SetFocus();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    if ( displ_opts->m_DisplayPcbTrackFill )
        m_OptDisplayTracks->SetSelection( 1 );
    else
        m_OptDisplayTracks->SetSelection( 0 );

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

    if ( displ_opts->m_DisplayPadFill )
        m_OptDisplayPads->SetSelection( 1 );
    else
        m_OptDisplayPads->SetSelection( 0 );

    if ( displ_opts->m_DisplayViaFill )
        m_OptDisplayVias->SetSelection( 1 );
    else
        m_OptDisplayVias->SetSelection( 0 );

    m_Show_Page_Limits->SetSelection( m_Parent->ShowPageLimits() ? 0 : 1 );

    m_OptDisplayViaHole->SetSelection( displ_opts->m_DisplayViaMode );
    m_OptDisplayModTexts->SetSelection( displ_opts->m_DisplayModText );
    m_OptDisplayModEdges->SetSelection( displ_opts->m_DisplayModEdge );
    m_OptDisplayPadClearence->SetValue( displ_opts->m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts->m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_Parent->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) ) );
    m_OptDisplayDrawings->SetSelection( displ_opts->m_DisplayDrawItems );
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

    if ( m_Show_Page_Limits->GetSelection() == 0 )
        m_Parent->SetShowPageLimits( true );
    else
        m_Parent->SetShowPageLimits( false );

    displ_opts->m_DisplayPcbTrackFill = m_OptDisplayTracks->GetSelection() == 1;

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

    displ_opts->m_DisplayModText = m_OptDisplayModTexts->GetSelection();
    displ_opts->m_DisplayModEdge = m_OptDisplayModEdges->GetSelection();

    displ_opts->m_DisplayPadFill = m_OptDisplayPads->GetSelection() == 1;
    displ_opts->m_DisplayViaFill = m_OptDisplayVias->GetSelection() == 1;

    displ_opts->m_DisplayPadIsol = m_OptDisplayPadClearence->GetValue();

    displ_opts->m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    m_Parent->SetElementVisibility( PCB_VISIBLE(NO_CONNECTS_VISIBLE),
                                    m_OptDisplayPadNoConn->GetValue() );

    displ_opts->m_DisplayDrawItems = m_OptDisplayDrawings->GetSelection();
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
