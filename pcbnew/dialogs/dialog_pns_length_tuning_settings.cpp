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
 * Length tuner settings dialog.
 */

#include "dialog_pns_length_tuning_settings.h"
#include <router/pns_meander_placer.h>

DIALOG_PNS_LENGTH_TUNING_SETTINGS::DIALOG_PNS_LENGTH_TUNING_SETTINGS( wxWindow* aParent, PNS_MEANDER_SETTINGS& aSettings, PNS_ROUTER_MODE aMode ) :
    DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE( aParent ),
    m_minAmpl( this, m_minAmplText, m_minAmplUnit ),
    m_maxAmpl( this, m_maxAmplText, m_maxAmplUnit ),
    m_spacing( this, m_spacingText, m_spacingUnit ),
    m_targetLength( this, m_targetLengthText, m_targetLengthUnit ),
    m_settings( aSettings ),
    m_mode( aMode )
{
    m_miterStyle->Enable( false );
    m_radiusText->Enable( aMode != PNS_MODE_TUNE_DIFF_PAIR );
    //m_minAmpl.Enable ( aMode != PNS_MODE_TUNE_DIFF_PAIR_SKEW );

    m_minAmpl.SetValue( m_settings.m_minAmplitude );
    m_maxAmpl.SetValue( m_settings.m_maxAmplitude );

    m_spacing.SetValue( m_settings.m_spacing );
    m_radiusText->SetValue( wxString::Format( wxT( "%i" ), m_settings.m_cornerRadiusPercentage ) );

    m_miterStyle->SetSelection( m_settings.m_cornerType == PNS_MEANDER_SETTINGS::ROUND ? 1 : 0 );

    switch( aMode )
    {
        case PNS_MODE_TUNE_SINGLE:
            SetTitle( _( "Single track length tuning" ) );
            m_legend->SetBitmap( KiBitmap( tune_single_track_length_legend_xpm ) );
            m_targetLength.SetValue( m_settings.m_targetLength );
            break;

        case PNS_MODE_TUNE_DIFF_PAIR:
            SetTitle( _( "Differential pair length tuning" ) );
            m_legend->SetBitmap( KiBitmap( tune_diff_pair_length_legend_xpm ) );
            m_targetLength.SetValue( m_settings.m_targetLength );
            break;

        case PNS_MODE_TUNE_DIFF_PAIR_SKEW:
            SetTitle( _( "Differential pair skew tuning" ) );
            m_legend->SetBitmap( KiBitmap( tune_diff_pair_skew_legend_xpm ) );
            m_targetLengthLabel->SetLabel( _( "Target skew: " ) );
            m_targetLength.SetValue ( m_settings.m_targetSkew );
            break;

        default:
            break;
    }

    m_stdButtonsOK->SetDefault();
    m_targetLengthText->SetSelection( -1, -1 );
    m_targetLengthText->SetFocus();
}


void DIALOG_PNS_LENGTH_TUNING_SETTINGS::OnClose( wxCloseEvent& aEvent )
{
    // Do nothing, it is result of ESC pressing
    EndModal( 0 );
}


void DIALOG_PNS_LENGTH_TUNING_SETTINGS::OnOkClick( wxCommandEvent& aEvent )
{
    // fixme: use validators and TransferDataFromWindow
    m_settings.m_minAmplitude = m_minAmpl.GetValue();
    m_settings.m_maxAmplitude = m_maxAmpl.GetValue();
    m_settings.m_spacing = m_spacing.GetValue();

    m_settings.m_cornerRadiusPercentage = wxAtoi( m_radiusText->GetValue() );

    if( m_mode == PNS_MODE_TUNE_DIFF_PAIR_SKEW )
        m_settings.m_targetSkew = m_targetLength.GetValue();
    else
        m_settings.m_targetLength = m_targetLength.GetValue();

    if( m_settings.m_maxAmplitude < m_settings.m_minAmplitude )
        m_settings.m_maxAmplitude = m_settings.m_minAmplitude;

    m_settings.m_cornerType = m_miterStyle->GetSelection() ? PNS_MEANDER_SETTINGS::CHAMFER : PNS_MEANDER_SETTINGS::ROUND;

    EndModal( 1 );
}


void DIALOG_PNS_LENGTH_TUNING_SETTINGS::OnCancelClick( wxCommandEvent& aEvent )
{
    // Do nothing
    EndModal( 0 );
}
