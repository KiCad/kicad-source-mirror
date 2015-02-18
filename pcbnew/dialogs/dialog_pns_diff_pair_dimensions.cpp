/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014-2015  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

/**
 * Push and Shove diff pair dimensions (gap) settings dialog.
 */

#include "dialog_pns_diff_pair_dimensions.h"
#include <router/pns_sizes_settings.h>

DIALOG_PNS_DIFF_PAIR_DIMENSIONS::DIALOG_PNS_DIFF_PAIR_DIMENSIONS( wxWindow* aParent, PNS_SIZES_SETTINGS& aSizes ) :
    DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE( aParent ),
    m_traceWidth ( this, m_traceWidthText, m_traceWidthUnit ),
    m_traceGap (this, m_traceGapText, m_traceGapUnit ),
    m_viaGap ( this, m_viaGapText, m_viaGapUnit ),
    m_sizes( aSizes )
    
{
    m_traceWidth.SetValue ( aSizes.DiffPairWidth() );
    m_traceGap.SetValue ( aSizes.DiffPairGap() );
    m_viaGap.SetValue ( aSizes.DiffPairViaGap() );
    m_viaTraceGapEqual->SetValue ( m_sizes.DiffPairViaGapSameAsTraceGap() );
    
    updateCheckbox();
}

void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::updateCheckbox()
{
    printf("Checked: %d", m_viaTraceGapEqual->GetValue());
    if( m_viaTraceGapEqual->GetValue() )
    {
        m_sizes.SetDiffPairViaGapSameAsTraceGap( true );
        m_viaGapText->Disable();
        m_viaGapLabel->Disable();
        m_viaGapUnit->Disable();
    } else {
        m_sizes.SetDiffPairViaGapSameAsTraceGap( false );
        m_viaGapText->Enable();
        m_viaGapLabel->Enable();
        m_viaGapUnit->Enable();
    }
}

void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::OnClose( wxCloseEvent& aEvent )
{
    // Do nothing, it is result of ESC pressing
    EndModal( 0 );
}


void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::OnOkClick( wxCommandEvent& aEvent )
{
    // Save widgets' values to settings
    m_sizes.SetDiffPairGap ( m_traceGap.GetValue() );
    m_sizes.SetDiffPairViaGap ( m_viaGap.GetValue() );
    m_sizes.SetDiffPairWidth ( m_traceWidth.GetValue() );
    
    // todo: verify against design rules
    EndModal( 1 );
}


void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::OnCancelClick( wxCommandEvent& aEvent )
{
    // Do nothing
    EndModal( 0 );
}

void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::OnViaTraceGapEqualCheck( wxCommandEvent& event )
{
    event.Skip();
    updateCheckbox();
}
    