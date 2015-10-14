/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <kicad_string.h>
#include <pcbnew_id.h>
#include <class_board.h>
#include <collectors.h>
#include <dialog_general_options.h>


DIALOG_GENERALOPTIONS::DIALOG_GENERALOPTIONS( PCB_EDIT_FRAME* parent ) :
    DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( parent )
{
    init();

    GetSizer()->SetSizeHints( this );
    Center();
}


void DIALOG_GENERALOPTIONS::init()
{
    SetFocus();
    m_sdbSizerOK->SetDefault();

    m_Board = GetParent()->GetBoard();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetParent()->GetDisplayOptions();

    /* Set display options */
    m_PolarDisplay->SetSelection( displ_opts->m_DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( g_UserUnit ? 1 : 0 );
    m_CursorShape->SetSelection( GetParent()->GetCursorShape() ? 1 : 0 );


    wxString rotationAngle;
    rotationAngle = AngleToStringDegrees( (double)GetParent()->GetRotationAngle() );
    m_RotationAngle->SetValue( rotationAngle );

    m_spinMaxUndoItems->SetValue( GetParent()->GetScreen()->GetMaxUndoItems() );

    wxString timevalue;
    timevalue << GetParent()->GetAutoSaveInterval() / 60;
    m_SaveTime->SetValue( timevalue );
    m_MaxShowLinks->SetValue( displ_opts->m_MaxLinksShowed );

    m_DrcOn->SetValue( g_Drc_On );
    m_ShowModuleRatsnest->SetValue( displ_opts->m_Show_Module_Ratsnest );
    m_ShowGlobalRatsnest->SetValue( m_Board->IsElementVisible( RATSNEST_VISIBLE ) );
    m_TrackAutodel->SetValue( g_AutoDeleteOldTrack );
    m_Track_45_Only_Ctrl->SetValue( g_Track_45_Only_Allowed );
    m_Segments_45_Only_Ctrl->SetValue( g_Segments_45_Only );
    m_ZoomCenterOpt->SetValue( ! GetParent()->GetCanvas()->GetEnableZoomNoCenter() );
    m_MiddleButtonPANOpt->SetValue( GetParent()->GetCanvas()->GetEnableMiddleButtonPan() );
    m_OptMiddleButtonPanLimited->SetValue( GetParent()->GetCanvas()->GetMiddleButtonPanLimited() );
    m_OptMiddleButtonPanLimited->Enable( m_MiddleButtonPANOpt->GetValue() );
    m_AutoPANOpt->SetValue( GetParent()->GetCanvas()->GetEnableAutoPan() );
    m_Track_DoubleSegm_Ctrl->SetValue( g_TwoSegmentTrackBuild );
    m_MagneticPadOptCtrl->SetSelection( g_MagneticPadOption );
    m_MagneticTrackOptCtrl->SetSelection( g_MagneticTrackOption );
    m_DumpZonesWhenFilling->SetValue ( g_DumpZonesWhenFilling );
}


void DIALOG_GENERALOPTIONS::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


void DIALOG_GENERALOPTIONS::OnOkClick( wxCommandEvent& event )
{
    EDA_UNITS_T ii;
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetParent()->GetDisplayOptions();

    displ_opts->m_DisplayPolarCood = ( m_PolarDisplay->GetSelection() == 0 ) ? false : true;
    ii = g_UserUnit;
    g_UserUnit = ( m_UnitsSelection->GetSelection() == 0 )  ? INCHES : MILLIMETRES;

    if( ii != g_UserUnit )
        GetParent()->ReCreateAuxiliaryToolbar();

    GetParent()->SetCursorShape( m_CursorShape->GetSelection() );
    GetParent()->SetAutoSaveInterval( m_SaveTime->GetValue() * 60 );
    GetParent()->SetRotationAngle( wxRound( 10.0 * wxAtof( m_RotationAngle->GetValue() ) ) );

    GetParent()->GetScreen()->SetMaxUndoItems( m_spinMaxUndoItems->GetValue() );

    /* Updating the combobox to display the active layer. */
    displ_opts->m_MaxLinksShowed = m_MaxShowLinks->GetValue();
    g_Drc_On = m_DrcOn->GetValue();

    if( m_Board->IsElementVisible(RATSNEST_VISIBLE) != m_ShowGlobalRatsnest->GetValue() )
    {
        GetParent()->SetElementVisibility( RATSNEST_VISIBLE, m_ShowGlobalRatsnest->GetValue() );
        GetParent()->GetCanvas()->Refresh();
        GetParent()->OnModify();
    }

    displ_opts->m_Show_Module_Ratsnest = m_ShowModuleRatsnest->GetValue();
    g_AutoDeleteOldTrack   = m_TrackAutodel->GetValue();
    g_Segments_45_Only = m_Segments_45_Only_Ctrl->GetValue();
    g_Track_45_Only_Allowed    = m_Track_45_Only_Ctrl->GetValue();

    GetParent()->GetCanvas()->SetEnableZoomNoCenter( ! m_ZoomCenterOpt->GetValue() );
    GetParent()->GetCanvas()->SetEnableMiddleButtonPan( m_MiddleButtonPANOpt->GetValue() );
    GetParent()->GetCanvas()->SetMiddleButtonPanLimited( m_OptMiddleButtonPanLimited->GetValue() );

    GetParent()->GetCanvas()->SetEnableAutoPan( m_AutoPANOpt->GetValue() );
    g_TwoSegmentTrackBuild = m_Track_DoubleSegm_Ctrl->GetValue();
    g_MagneticPadOption   = m_MagneticPadOptCtrl->GetSelection();
    g_MagneticTrackOption = m_MagneticTrackOptCtrl->GetSelection();
    g_DumpZonesWhenFilling = m_DumpZonesWhenFilling->GetValue();

    EndModal( wxID_OK );
}
