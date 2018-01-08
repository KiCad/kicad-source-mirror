/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* functions relatives to the dialog opened from the main menu :
    Preferences/display
*/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcb_display_options.h>
#include <config_map.h>

#include <pcbnew_id.h>

#include <dialog_display_options.h>
#include <dialog_display_options_base.h>

#include <class_draw_panel_gal.h>
#include <pcb_view.h>
#include <pcb_painter.h>

#include <widgets/gal_options_panel.h>


static const UTIL::CFG_MAP<PCB_DISPLAY_OPTIONS::TRACE_CLEARANCE_DISPLAY_MODE_T> traceClearanceSelectMap =
{
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,            2 },     // Default
    { PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE,                              0 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS,                          1 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS, 3 },
    { PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_ALWAYS,                              4 },
};


void PCB_EDIT_FRAME::InstallDisplayOptionsDialog( wxCommandEvent& aEvent )
{
    DIALOG_DISPLAY_OPTIONS dlg( this );
    dlg.ShowModal();
}


DIALOG_DISPLAY_OPTIONS::DIALOG_DISPLAY_OPTIONS( PCB_EDIT_FRAME* parent ) :
    DIALOG_DISPLAY_OPTIONS_BASE( parent ),
    m_parent( parent ),
    m_last_scale( -1 )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_parent->GetGalDisplayOptions();
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );

    sLeftSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    m_scaleSlider->SetStep( 25 );

    SetFocus();

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_DISPLAY_OPTIONS::TransferDataToWindow()
{
    const PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*) m_parent->GetDisplayOptions();

    m_OptDisplayTracks->SetValue( displ_opts->m_DisplayPcbTrackFill == SKETCH );

    m_OptDisplayTracksClearance->SetSelection( UTIL::GetConfigForVal(
            traceClearanceSelectMap, displ_opts->m_ShowTrackClearanceMode ) );

    m_OptDisplayPads->SetValue( displ_opts->m_DisplayPadFill == SKETCH );
    m_OptDisplayVias->SetValue( displ_opts->m_DisplayViaFill == SKETCH );

    m_Show_Page_Limits->SetValue( m_parent->ShowPageLimits() );

    m_OptDisplayModTexts->SetValue( displ_opts->m_DisplayModTextFill == SKETCH );
    m_OptDisplayModOutlines->SetValue( displ_opts->m_DisplayModEdgeFill == SKETCH );
    m_OptDisplayPadClearence->SetValue( displ_opts->m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts->m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_parent->IsElementVisible( LAYER_NO_CONNECTS ) );
    m_OptDisplayDrawings->SetValue( displ_opts->m_DisplayDrawItemsFill == SKETCH );
    m_ShowNetNamesOption->SetSelection( displ_opts->m_DisplayNetNamesMode );

    const int scale_fourths = m_parent->GetIconScale();

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( m_parent ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
    }

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


/*
 * Update variables with new options
 */
bool DIALOG_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*) m_parent->GetDisplayOptions();

    m_parent->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    displ_opts->m_DisplayPcbTrackFill = not m_OptDisplayTracks->GetValue();

    displ_opts->m_ShowTrackClearanceMode = UTIL::GetValFromConfig(
            traceClearanceSelectMap, m_OptDisplayTracksClearance->GetSelection() );

    displ_opts->m_DisplayModTextFill = not m_OptDisplayModTexts->GetValue();
    displ_opts->m_DisplayModEdgeFill = not m_OptDisplayModOutlines->GetValue();

    displ_opts->m_DisplayPadFill = not m_OptDisplayPads->GetValue();
    displ_opts->m_DisplayViaFill = not m_OptDisplayVias->GetValue();

    displ_opts->m_DisplayPadIsol = m_OptDisplayPadClearence->GetValue();

    displ_opts->m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    m_parent->SetElementVisibility( LAYER_NO_CONNECTS,
                                    m_OptDisplayPadNoConn->GetValue() );

    displ_opts->m_DisplayDrawItemsFill = not m_OptDisplayDrawings->GetValue();
    displ_opts->m_DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();

    m_galOptsPanel->TransferDataFromWindow();

    // Apply changes to the GAL
    KIGFX::VIEW* view = m_parent->GetGalCanvas()->GetView();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( displ_opts );
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;

    if( m_parent->GetIconScale() != scale_fourths )
        m_parent->SetIconScale( scale_fourths );

    m_parent->GetCanvas()->Refresh();

    return true;
}


void DIALOG_DISPLAY_OPTIONS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
    aEvent.Skip();
}


void DIALOG_DISPLAY_OPTIONS::OnScaleAuto( wxCommandEvent& aEvent )
{
    if( m_scaleAuto->GetValue() )
    {
        m_last_scale = m_scaleSlider->GetValue();
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_scaleSlider->SetValue( m_last_scale );
    }
}
