/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014-2018 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/text_ctrl_eval.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>

DIALOG_PNS_LENGTH_TUNING_SETTINGS::DIALOG_PNS_LENGTH_TUNING_SETTINGS( EDA_DRAW_FRAME* aParent,
                        PNS::MEANDER_SETTINGS& aSettings, PNS::ROUTER_MODE aMode )
    :
    DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE( aParent ),
    m_minAmpl( aParent, m_minAmplLabel, m_minAmplText, m_minAmplUnit, true ),
    m_maxAmpl( aParent, m_maxAmplLabel, m_maxAmplText, m_maxAmplUnit, true ),
    m_spacing( aParent, m_spacingLabel, m_spacingText, m_spacingUnit, true ),
    m_targetLength( aParent, m_targetLengthLabel, m_targetLengthText, m_targetLengthUnit ),
    m_radius( aParent, m_radiusLabel, m_radiusText, m_radiusUnit, false, false ),
    m_settings( aSettings ),
    m_mode( aMode )
{
    m_stdButtonsOK->SetDefault();
    m_targetLengthText->SetSelection( -1, -1 );
    m_targetLengthText->SetFocus();
    m_radius.SetUnits( PERCENT );

    GetSizer()->SetSizeHints(this);
    Centre();
}


bool DIALOG_PNS_LENGTH_TUNING_SETTINGS::TransferDataToWindow()
{

    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_mode == PNS::PNS_MODE_TUNE_DIFF_PAIR )
    {
        // TODO: fix diff-pair meandering so we can use non-100% radii
        m_radiusText->SetValue( wxT( "100" ) );
        m_radiusText->Enable( false );
    }
    else
    {
        m_radiusText->SetValue( wxString::Format( wxT( "%i" ), m_settings.m_cornerRadiusPercentage ) );
    }

    m_minAmpl.SetValue( m_settings.m_minAmplitude );
    m_maxAmpl.SetValue( m_settings.m_maxAmplitude );
    m_spacing.SetValue( m_settings.m_spacing );
    m_miterStyle->SetSelection( m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );

    switch( m_mode )
    {
    case PNS::PNS_MODE_TUNE_SINGLE:
        SetTitle( _( "Single Track Length Tuning" ) );
        m_legend->SetBitmap( KiBitmap( tune_single_track_length_legend_xpm ) );
        m_targetLength.SetValue( m_settings.m_targetLength );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR:
        SetTitle( _( "Differential Pair Length Tuning" ) );
        m_legend->SetBitmap( KiBitmap( tune_diff_pair_length_legend_xpm ) );
        m_targetLength.SetValue( m_settings.m_targetLength );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW:
        SetTitle( _( "Differential Pair Skew Tuning" ) );
        m_legend->SetBitmap( KiBitmap( tune_diff_pair_skew_legend_xpm ) );
        m_targetLengthLabel->SetLabel( _( "Target skew: " ) );
        m_targetLength.SetValue( m_settings.m_targetSkew );
        break;

    default:
        break;
    }

    // Bitmap has a new size, so recalculate sizes
    GetSizer()->SetSizeHints(this);

    return true;
}


bool DIALOG_PNS_LENGTH_TUNING_SETTINGS::AcceptOptions( )
{
    if( !m_minAmpl.Validate( 0, INT_MAX ) )
        return false;
    if( !m_maxAmpl.Validate( m_minAmpl.GetValue(), INT_MAX ) )
        return false;
    if( !m_spacing.Validate( 0, INT_MAX ) )
        return false;
    if( !m_targetLength.Validate( 0, INT_MAX ) )
        return false;
    if( !m_radius.Validate( 0, 100 ) )
        return false;

    return true;
}

bool DIALOG_PNS_LENGTH_TUNING_SETTINGS::TransferDataFromWindow()
{
    if( !AcceptOptions() || !wxDialog::TransferDataToWindow() )
        return false;

    m_settings.m_minAmplitude = m_minAmpl.GetValue();
    m_settings.m_maxAmplitude = m_maxAmpl.GetValue();
    m_settings.m_spacing = m_spacing.GetValue();
    m_settings.m_cornerRadiusPercentage = m_radius.GetValue();

    if( m_mode == PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW )
        m_settings.m_targetSkew = m_targetLength.GetValue();
    else
        m_settings.m_targetLength = m_targetLength.GetValue();

    if( m_settings.m_maxAmplitude < m_settings.m_minAmplitude )
        m_settings.m_maxAmplitude = m_settings.m_minAmplitude;

    m_settings.m_cornerStyle = m_miterStyle->GetSelection() ?
        PNS::MEANDER_STYLE_ROUND : PNS::MEANDER_STYLE_CHAMFER;

    return true;
}
