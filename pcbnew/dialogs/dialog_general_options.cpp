/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

/**
 * @file dialog_general_options.cpp
 */

/* functions relatives to the dialogs opened from the main menu :
 *   Preferences/general
 *   Preferences/display
 */
#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <kicad_string.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <collectors.h>
#include <pgm_base.h>
#include <dialog_general_options.h>
#include <widgets/stepped_slider.h>


DIALOG_GENERALOPTIONS::DIALOG_GENERALOPTIONS( PCB_EDIT_FRAME* parent ) :
    DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( parent ),
    m_last_scale( -1 )
{
    init();

    m_scaleSlider->SetStep( 25 );

    GetSizer()->SetSizeHints( this );
    Center();
}


void DIALOG_GENERALOPTIONS::init()
{
    SetFocus();
    m_sdbSizerOK->SetDefault();

    m_Board = GetParent()->GetBoard();
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetParent()->GetDisplayOptions();

    /* Set display options */
    m_PolarDisplay->SetSelection( displ_opts->m_DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( g_UserUnit ? 1 : 0 );

    wxString rotationAngle;
    rotationAngle = AngleToStringDegrees( (double)GetParent()->GetRotationAngle() );
    m_RotationAngle->SetValue( rotationAngle );

    wxString timevalue;
    timevalue << GetParent()->GetAutoSaveInterval() / 60;
    m_SaveTime->SetValue( timevalue );

    m_DrcOn->SetValue( GetParent()->Settings().m_legacyDrcOn );
    m_ShowGlobalRatsnest->SetValue( m_Board->IsElementVisible( LAYER_RATSNEST ) );
    m_TrackAutodel->SetValue( GetParent()->Settings().m_legacyAutoDeleteOldTrack );
    m_Track_45_Only_Ctrl->SetValue( GetParent()->Settings().m_legacyUse45DegreeTracks );
    m_Segments_45_Only_Ctrl->SetValue( GetParent()->Settings().m_use45DegreeGraphicSegments );
    m_ZoomCenterOpt->SetValue( ! GetParent()->GetCanvas()->GetEnableZoomNoCenter() );
    m_MousewheelPANOpt->SetValue( GetParent()->GetCanvas()->GetEnableMousewheelPan() );
    m_AutoPANOpt->SetValue( GetParent()->GetCanvas()->GetEnableAutoPan() );
    m_Track_DoubleSegm_Ctrl->SetValue( GetParent()->Settings().m_legacyUseTwoSegmentTracks );
    m_MagneticPadOptCtrl->SetSelection( GetParent()->Settings().m_magneticPads );
    m_MagneticTrackOptCtrl->SetSelection( GetParent()->Settings().m_magneticTracks );
    m_UseEditKeyForWidth->SetValue( GetParent()->Settings().m_editActionChangesTrackWidth );
    m_dragSelects->SetValue( GetParent()->Settings().m_dragSelects );

    m_Show_Page_Limits->SetValue( GetParent()->ShowPageLimits() );

    const int scale_fourths = GetParent()->GetIconScale();

    if( scale_fourths <= 0 )
    {
        m_scaleAuto->SetValue( true );
        m_scaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        m_scaleAuto->SetValue( false );
        m_scaleSlider->SetValue( scale_fourths * 25 );
    }

    m_checkBoxIconsInMenus->SetValue( Pgm().GetUseIconsInMenus() );
}


void DIALOG_GENERALOPTIONS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_scaleAuto->SetValue( false );
    aEvent.Skip();
}


void DIALOG_GENERALOPTIONS::OnScaleAuto( wxCommandEvent& aEvent )
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


void DIALOG_GENERALOPTIONS::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


void DIALOG_GENERALOPTIONS::OnOkClick( wxCommandEvent& event )
{
    EDA_UNITS_T ii;
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetParent()->GetDisplayOptions();

    displ_opts->m_DisplayPolarCood = ( m_PolarDisplay->GetSelection() == 0 ) ? false : true;
    ii = g_UserUnit;
    g_UserUnit = ( m_UnitsSelection->GetSelection() == 0 )  ? INCHES : MILLIMETRES;

    if( ii != g_UserUnit )
        GetParent()->ReCreateAuxiliaryToolbar();

    GetParent()->SetAutoSaveInterval( m_SaveTime->GetValue() * 60 );
    GetParent()->SetRotationAngle( wxRound( 10.0 * wxAtof( m_RotationAngle->GetValue() ) ) );

    /* Updating the combobox to display the active layer. */
    GetParent()->Settings().m_legacyDrcOn = m_DrcOn->GetValue();

    if( m_Board->IsElementVisible( LAYER_RATSNEST ) != m_ShowGlobalRatsnest->GetValue() )
    {
        GetParent()->SetElementVisibility( LAYER_RATSNEST, m_ShowGlobalRatsnest->GetValue() );
        GetParent()->GetCanvas()->Refresh();
        GetParent()->OnModify();
    }

    GetParent()->Settings().m_legacyAutoDeleteOldTrack   = m_TrackAutodel->GetValue();
    GetParent()->Settings().m_use45DegreeGraphicSegments = m_Segments_45_Only_Ctrl->GetValue();
    GetParent()->Settings().m_legacyUse45DegreeTracks    = m_Track_45_Only_Ctrl->GetValue();

    GetParent()->GetCanvas()->SetEnableZoomNoCenter( ! m_ZoomCenterOpt->GetValue() );
    GetParent()->GetCanvas()->SetEnableMousewheelPan( m_MousewheelPANOpt->GetValue() );
    GetParent()->GetCanvas()->SetEnableAutoPan( m_AutoPANOpt->GetValue() );

    GetParent()->Settings().m_legacyUseTwoSegmentTracks = m_Track_DoubleSegm_Ctrl->GetValue();
    GetParent()->Settings().m_magneticPads   = (MAGNETIC_PAD_OPTION_VALUES) m_MagneticPadOptCtrl->GetSelection();
    GetParent()->Settings().m_magneticTracks = (MAGNETIC_PAD_OPTION_VALUES) m_MagneticTrackOptCtrl->GetSelection();
    GetParent()->Settings().m_editActionChangesTrackWidth = m_UseEditKeyForWidth->GetValue();
    GetParent()->Settings().m_dragSelects = m_dragSelects->GetValue();

    GetParent()->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    const int scale_fourths = m_scaleAuto->GetValue() ? -1 : m_scaleSlider->GetValue() / 25;

    if( GetParent()->GetIconScale() != scale_fourths )
        GetParent()->SetIconScale( scale_fourths );

    if( Pgm().GetUseIconsInMenus() != m_checkBoxIconsInMenus->GetValue() )
    {
        Pgm().SetUseIconsInMenus( m_checkBoxIconsInMenus->GetValue() );
        GetParent()->ReCreateMenuBar();
    }

    EndModal( wxID_OK );
}


