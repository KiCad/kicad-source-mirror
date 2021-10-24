/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

/**
 * Push and Shove router settings dialog.
 */

#include "dialog_pns_settings.h"
#include <router/pns_routing_settings.h>

DIALOG_PNS_SETTINGS::DIALOG_PNS_SETTINGS( wxWindow* aParent, PNS::ROUTING_SETTINGS& aSettings ) :
    DIALOG_PNS_SETTINGS_BASE( aParent ), m_settings( aSettings )
{
    // Add tool tip to the mode radio box, one by option
    // (cannot be made with wxFormBuilder for each item )
    m_mode->SetItemToolTip( 0, _( "DRC violation: highlight obstacles" ) );
    m_mode->SetItemToolTip( 1, _( "DRC violation: shove tracks and vias" ) );
    m_mode->SetItemToolTip( 2, _( "DRC violation: walk around obstacles" ) );

    // Load widgets' values from settings
    m_mode->SetSelection( m_settings.Mode() );
    m_shoveVias->SetValue( m_settings.ShoveVias() );
    m_backPressure->SetValue( m_settings.JumpOverObstacles() );
    m_removeLoops->SetValue( m_settings.RemoveLoops() );
    m_suggestEnding->SetValue( m_settings.SuggestFinish() );
    m_smartPads->SetValue( m_settings.SmartPads() );
    m_smoothDragged->SetValue( m_settings.SmoothDraggedSegments() );
    m_violateDrc->SetValue( m_settings.GetAllowDRCViolationsSetting() );
    m_freeAngleMode->SetValue( m_settings.GetFreeAngleMode() );
    m_optimizeEntireDraggedTrack->SetValue( m_settings.GetOptimizeEntireDraggedTrack() );
    m_autoPosture->SetValue( m_settings.GetAutoPosture() );
    m_fixAllSegments->SetValue( m_settings.GetFixAllSegments() );

    // Enable/disable some options
    wxCommandEvent event;
    onModeChange( event );

    // Don't show options that are not implemented
    m_suggestEnding->Hide();
    m_shoveVias->Hide();

    SetDefaultItem( m_stdButtonsOK );
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIALOG_PNS_SETTINGS::OnOkClick( wxCommandEvent& aEvent )
{
    // Save widgets' values to settings
    m_settings.SetMode( (PNS::PNS_MODE) m_mode->GetSelection() );
    m_settings.SetShoveVias( m_shoveVias->GetValue() );
    m_settings.SetJumpOverObstacles( m_backPressure->GetValue() );
    m_settings.SetRemoveLoops( m_removeLoops->GetValue() );
    m_settings.SetSuggestFinish ( m_suggestEnding->GetValue() );
    m_settings.SetSmartPads( m_smartPads->GetValue() );
    m_settings.SetSmoothDraggedSegments( m_smoothDragged->GetValue() );
    m_settings.SetOptimizeEntireDraggedTrack( m_optimizeEntireDraggedTrack->GetValue() );
    m_settings.SetAutoPosture( m_autoPosture->GetValue() );
    m_settings.SetFixAllSegments( m_fixAllSegments->GetValue() );

    if( m_mode->GetSelection() == PNS::RM_MarkObstacles )
    {
        m_settings.SetAllowDRCViolations( m_violateDrc->GetValue() );
        m_settings.SetFreeAngleMode( m_freeAngleMode->GetValue() );
    }

    aEvent.Skip();      // ends returning wxID_OK (default behavior)
}


void DIALOG_PNS_SETTINGS::onModeChange( wxCommandEvent& aEvent )
{
    if( m_mode->GetSelection() == PNS::RM_MarkObstacles )
    {
        m_freeAngleMode->SetValue( m_settings.GetFreeAngleMode() );
        m_freeAngleMode->Enable();

        m_violateDrc->SetValue( m_settings.GetAllowDRCViolationsSetting() );
        m_violateDrc->Enable();
    }
    else
    {
        if( m_freeAngleMode->IsEnabled() )
            m_settings.SetFreeAngleMode( m_freeAngleMode->GetValue() );

        m_freeAngleMode->SetValue( false );
        m_freeAngleMode->Enable( false );

        if( m_violateDrc->IsEnabled() )
            m_settings.SetAllowDRCViolations( m_violateDrc->GetValue() );

        m_violateDrc->SetValue( false );
        m_violateDrc->Enable( false );
    }
}
