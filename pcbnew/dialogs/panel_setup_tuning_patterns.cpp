/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <panel_setup_tuning_patterns.h>
#include <router/pns_meander_placer.h>
#include <widgets/text_ctrl_eval.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <board.h>
#include <board_design_settings.h>

PANEL_SETUP_TUNING_PATTERNS::PANEL_SETUP_TUNING_PATTERNS( wxWindow* aParent, EDA_DRAW_FRAME* aFrame,
                                                          PNS::MEANDER_SETTINGS& aTrackSettings,
                                                          PNS::MEANDER_SETTINGS& aDiffPairSettings,
                                                          PNS::MEANDER_SETTINGS& aSkewSettings ) :
        PANEL_SETUP_TUNING_PATTERNS_BASE( aParent ),
        m_track_minA( aFrame, m_track_minALabel, m_track_minACtrl, m_track_minAUnits ),
        m_track_maxA( aFrame, m_track_maxALabel, m_track_maxACtrl, m_track_maxAUnits ),
        m_track_spacing( aFrame, m_track_spacingLabel, m_track_spacingCtrl, m_track_spacingUnits ),
        m_track_r( aFrame, m_track_rLabel, m_track_rCtrl, m_track_rUnits ),
        m_dp_minA( aFrame, m_dp_minALabel, m_dp_minACtrl, m_dp_minAUnits ),
        m_dp_maxA( aFrame, m_dp_maxALabel, m_dp_maxACtrl, m_dp_maxAUnits ),
        m_dp_spacing( aFrame, m_dp_spacingLabel, m_dp_spacingCtrl, m_dp_spacingUnits ),
        m_dp_r( aFrame, m_dp_rLabel, m_dp_rCtrl, m_dp_rUnits ),
        m_skew_minA( aFrame, m_skew_minALabel, m_skew_minACtrl, m_skew_minAUnits ),
        m_skew_maxA( aFrame, m_skew_maxALabel, m_skew_maxACtrl, m_skew_maxAUnits ),
        m_skew_spacing( aFrame, m_skew_spacingLabel, m_skew_spacingCtrl, m_skew_spacingUnits ),
        m_skew_r( aFrame, m_skew_rLabel, m_skew_rCtrl, m_skew_rUnits ),
        m_trackSettings( aTrackSettings ),
        m_dpSettings( aDiffPairSettings ),
        m_skewSettings( aSkewSettings )
{
    m_singleTrackLegend->SetBitmap( KiBitmapBundle( BITMAPS::tune_single_track_length_legend ) );
    m_diffPairLegend->SetBitmap( KiBitmapBundle( BITMAPS::tune_diff_pair_length_legend ) );
    m_skewLegend->SetBitmap( KiBitmapBundle( BITMAPS::tune_diff_pair_skew_legend ) );

    m_track_r.SetUnits( EDA_UNITS::PERCENT );
    m_dp_r.SetUnits( EDA_UNITS::PERCENT );
    m_skew_r.SetUnits( EDA_UNITS::PERCENT );
}


bool PANEL_SETUP_TUNING_PATTERNS::TransferDataToWindow()
{
    m_track_minA.SetValue( m_trackSettings.m_minAmplitude );
    m_track_maxA.SetValue( m_trackSettings.m_maxAmplitude );
    m_track_spacing.SetValue( m_trackSettings.m_spacing );
    m_track_cornerCtrl->SetSelection( m_trackSettings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );
    m_track_r.SetValue( m_trackSettings.m_cornerRadiusPercentage );
    m_track_singleSided->SetValue( m_trackSettings.m_singleSided );

    m_dp_minA.SetValue( m_dpSettings.m_minAmplitude );
    m_dp_maxA.SetValue( m_dpSettings.m_maxAmplitude );
    m_dp_spacing.SetValue( m_dpSettings.m_spacing );
    m_dp_cornerCtrl->SetSelection( m_dpSettings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );
    m_dp_r.SetValue( m_dpSettings.m_cornerRadiusPercentage );
    m_dp_singleSided->SetValue( m_dpSettings.m_singleSided );

    m_skew_minA.SetValue( m_skewSettings.m_minAmplitude );
    m_skew_maxA.SetValue( m_skewSettings.m_maxAmplitude );
    m_skew_spacing.SetValue( m_skewSettings.m_spacing );
    m_skew_cornerCtrl->SetSelection( m_skewSettings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );
    m_skew_r.SetValue( m_skewSettings.m_cornerRadiusPercentage );
    m_skew_singleSided->SetValue( m_skewSettings.m_singleSided );

    return true;
}


bool PANEL_SETUP_TUNING_PATTERNS::TransferDataFromWindow()
{
    m_trackSettings.m_minAmplitude = m_track_minA.GetIntValue();
    m_trackSettings.m_maxAmplitude = m_track_maxA.GetIntValue();
    m_trackSettings.m_spacing = m_track_spacing.GetIntValue();
    m_trackSettings.m_cornerStyle = m_track_cornerCtrl->GetSelection() ? PNS::MEANDER_STYLE_ROUND
                                                                       : PNS::MEANDER_STYLE_CHAMFER;
    m_trackSettings.m_cornerRadiusPercentage = m_track_r.GetIntValue();
    m_trackSettings.m_singleSided = m_track_singleSided->GetValue();

    m_dpSettings.m_minAmplitude = m_dp_minA.GetIntValue();
    m_dpSettings.m_maxAmplitude = m_dp_maxA.GetIntValue();
    m_dpSettings.m_spacing = m_dp_spacing.GetIntValue();
    m_dpSettings.m_cornerStyle = m_dp_cornerCtrl->GetSelection() ? PNS::MEANDER_STYLE_ROUND
                                                                 : PNS::MEANDER_STYLE_CHAMFER;
    m_dpSettings.m_cornerRadiusPercentage = m_dp_r.GetIntValue();
    m_dpSettings.m_singleSided = m_dp_singleSided->GetValue();

    m_skewSettings.m_minAmplitude = m_skew_minA.GetIntValue();
    m_skewSettings.m_maxAmplitude = m_skew_maxA.GetIntValue();
    m_skewSettings.m_spacing = m_skew_spacing.GetIntValue();
    m_skewSettings.m_cornerStyle = m_skew_cornerCtrl->GetSelection() ? PNS::MEANDER_STYLE_ROUND
                                                                     : PNS::MEANDER_STYLE_CHAMFER;
    m_skewSettings.m_cornerRadiusPercentage = m_skew_r.GetIntValue();
    m_skewSettings.m_singleSided = m_skew_singleSided->GetValue();

    return true;
}


void PANEL_SETUP_TUNING_PATTERNS::ImportSettingsFrom( BOARD* aBoard )
{
    PNS::MEANDER_SETTINGS savedTrackSettings = m_trackSettings;
    PNS::MEANDER_SETTINGS savedDPSettings = m_dpSettings;
    PNS::MEANDER_SETTINGS savedSkewSettings = m_skewSettings;

    m_trackSettings = aBoard->GetDesignSettings().m_SingleTrackMeanderSettings;
    m_dpSettings = aBoard->GetDesignSettings().m_DiffPairMeanderSettings;
    m_skewSettings = aBoard->GetDesignSettings().m_SkewMeanderSettings;
    TransferDataToWindow();

    m_trackSettings = std::move( savedTrackSettings );
    m_dpSettings = std::move( savedDPSettings );
    m_skewSettings = std::move( savedSkewSettings );
}
