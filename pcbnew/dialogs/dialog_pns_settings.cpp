/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
        DIALOG_PNS_SETTINGS_BASE( aParent ),
        m_settings( aSettings )
{
    // Load widgets' values from settings
    switch( m_settings.Mode() )
    {
    case PNS::RM_MarkObstacles: m_rbMarkObstacles->SetValue( true ); break;
    case PNS::RM_Shove:         m_rbShove->SetValue( true );         break;
    case PNS::RM_Walkaround:    m_rbWalkaround->SetValue( true );    break;
    }

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

    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_PNS_SETTINGS::TransferDataFromWindow()
{
    // Save widgets' values to settings
    if     ( m_rbMarkObstacles->GetValue() ) m_settings.SetMode( PNS::RM_MarkObstacles );
    else if( m_rbShove->GetValue() )         m_settings.SetMode( PNS::RM_Shove );
    else if( m_rbWalkaround->GetValue() )    m_settings.SetMode( PNS::RM_Walkaround );

    m_settings.SetShoveVias( m_shoveVias->GetValue() );
    m_settings.SetJumpOverObstacles( m_backPressure->GetValue() );
    m_settings.SetRemoveLoops( m_removeLoops->GetValue() );
    m_settings.SetSuggestFinish ( m_suggestEnding->GetValue() );
    m_settings.SetSmartPads( m_smartPads->GetValue() );
    m_settings.SetSmoothDraggedSegments( m_smoothDragged->GetValue() );
    m_settings.SetOptimizeEntireDraggedTrack( m_optimizeEntireDraggedTrack->GetValue() );
    m_settings.SetAutoPosture( m_autoPosture->GetValue() );
    m_settings.SetFixAllSegments( m_fixAllSegments->GetValue() );
    m_settings.SetAllowDRCViolations( m_violateDrc->GetValue() );
    m_settings.SetFreeAngleMode( m_freeAngleMode->GetValue() );

    return true;
}


void DIALOG_PNS_SETTINGS::onModeChange( wxCommandEvent& aEvent )
{
    m_freeAngleMode->Enable( m_rbMarkObstacles->GetValue() );
    m_violateDrc->Enable( m_rbMarkObstacles->GetValue() );

    m_shoveVias->Enable( m_rbShove->GetValue() );
    m_backPressure->Enable( m_rbShove->GetValue() );
}
