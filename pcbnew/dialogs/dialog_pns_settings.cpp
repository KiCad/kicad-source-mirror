/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
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

DIALOG_PNS_SETTINGS::DIALOG_PNS_SETTINGS( wxWindow* aParent, PNS_ROUTING_SETTINGS& aSettings ) :
    DIALOG_PNS_SETTINGS_BASE( aParent ), m_settings( aSettings )
{
    // "Figure out what's best" is not available yet
    m_mode->Enable( RM_Smart, false );

    // Load widgets' values from settings
    m_mode->SetSelection( m_settings.Mode() );
    m_shoveVias->SetValue( m_settings.ShoveVias() );
    m_backPressure->SetValue( m_settings.JumpOverObstacles() );
    m_removeLoops->SetValue( m_settings.RemoveLoops() );
    m_suggestEnding->SetValue( m_settings.SuggestFinish() );
    m_autoNeckdown->SetValue( m_settings.SmartPads() );
    m_effort->SetValue( m_settings.OptimizerEffort() );
    m_smoothDragged->SetValue( m_settings.SmoothDraggedSegments() );
    m_violateDrc->SetValue( m_settings.CanViolateDRC() );
}


void DIALOG_PNS_SETTINGS::OnClose( wxCloseEvent& aEvent )
{
    // Do nothing, it is result of ESC pressing
    EndModal( 0 );
}


void DIALOG_PNS_SETTINGS::OnOkClick( wxCommandEvent& aEvent )
{
    // Save widgets' values to settings
    m_settings.SetMode( (PNS_MODE) m_mode->GetSelection() );
    m_settings.SetShoveVias( m_shoveVias->GetValue() );
    m_settings.SetJumpOverObstacles( m_backPressure->GetValue() );
    m_settings.SetRemoveLoops( m_removeLoops->GetValue() );
    m_settings.SetSuggestFinish ( m_suggestEnding->GetValue() );
    m_settings.SetSmartPads( m_autoNeckdown->GetValue() );
    m_settings.SetOptimizerEffort( (PNS_OPTIMIZATION_EFFORT) m_effort->GetValue() );
    m_settings.SetSmoothDraggedSegments( m_smoothDragged->GetValue() );
    m_settings.SetCanViolateDRC( m_violateDrc->GetValue() );

    EndModal( 1 );
}


void DIALOG_PNS_SETTINGS::OnCancelClick( wxCommandEvent& aEvent )
{
    // Do nothing
    EndModal( 0 );
}
