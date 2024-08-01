/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_tuning_pattern_properties.h"
#include <bitmaps.h>
#include <pcb_base_edit_frame.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>


long long int g_lastTargetLength = PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED;
int           g_lastTargetSkew = 0;


DIALOG_TUNING_PATTERN_PROPERTIES::DIALOG_TUNING_PATTERN_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame,
                                                                    PNS::MEANDER_SETTINGS& aSettings,
                                                                    PNS::ROUTER_MODE aMeanderType,
                                                                    const DRC_CONSTRAINT& aConstraint ) :
        DIALOG_TUNING_PATTERN_PROPERTIES_BASE( aFrame ),
        m_constraint( aConstraint ),
        m_targetLength( aFrame, m_targetLengthLabel, m_targetLengthCtrl, m_targetLengthUnits ),
        m_minA( aFrame, m_track_minALabel, m_minACtrl, m_minAUnits ),
        m_maxA( aFrame, m_maxALabel, m_maxACtrl, m_maxAUnits ),
        m_spacing( aFrame, m_spacingLabel, m_spacingCtrl, m_spacingUnits ),
        m_r( aFrame, m_rLabel, m_rCtrl, m_rUnits ),
        m_settings( aSettings ),
        m_mode( aMeanderType )
{
    m_r.SetUnits( EDA_UNITS::PERCENT );

    switch( m_mode )
    {
    case PNS::PNS_MODE_TUNE_SINGLE:
        m_legend->SetBitmap( KiBitmapBundle( BITMAPS::tune_single_track_length_legend ) );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR:
        m_legend->SetBitmap( KiBitmapBundle( BITMAPS::tune_diff_pair_length_legend ) );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW:
        m_legend->SetBitmap( KiBitmapBundle( BITMAPS::tune_diff_pair_skew_legend ) );
        m_targetLengthLabel->SetLabel( _( "Target skew: ") );
        break;

    default:
        break;
    }

    // Bitmap has a new size, so recalculate sizes
    GetSizer()->SetSizeHints( this );
    SetupStandardButtons();

    SetInitialFocus( m_targetLengthCtrl );

    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_TUNING_PATTERN_PROPERTIES::TransferDataToWindow()
{
    if( m_mode == PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW )
    {
        m_targetLength.SetValue( m_settings.m_targetSkew.Opt() );

        if( m_targetLength.GetValue() == PNS::MEANDER_SETTINGS::SKEW_UNCONSTRAINED )
            m_targetLength.SetValue( g_lastTargetSkew );

        if( m_targetLength.GetValue() == PNS::MEANDER_SETTINGS::SKEW_UNCONSTRAINED )
            m_targetLengthCtrl->SetValue( wxEmptyString );
    }
    else
    {
        m_targetLength.SetValue( m_settings.m_targetLength.Opt() );

        if( m_targetLength.GetValue() == PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED )
            m_targetLength.SetValue( g_lastTargetLength );

        if( m_targetLength.GetValue() == PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED )
            m_targetLength.SetValue( wxEmptyString );
    }

    m_overrideCustomRules->SetValue( m_settings.m_overrideCustomRules );

    m_targetLength.Enable( m_constraint.IsNull() || m_settings.m_overrideCustomRules );

    if( !m_constraint.IsNull() )
        m_sourceInfo->SetLabel( wxString::Format( _( "(from %s)" ), m_constraint.GetName() ) );

    m_sourceInfo->Show( !m_constraint.IsNull() && !m_settings.m_overrideCustomRules );

    m_minA.SetValue( m_settings.m_minAmplitude );
    m_maxA.SetValue( m_settings.m_maxAmplitude );
    m_spacing.SetValue( m_settings.m_spacing );
    m_cornerCtrl->SetSelection( m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );
    m_r.SetValue( m_settings.m_cornerRadiusPercentage );
    m_singleSided->SetValue( m_settings.m_singleSided );

    return true;
}


bool DIALOG_TUNING_PATTERN_PROPERTIES::TransferDataFromWindow()
{
    if( m_mode == PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW )
    {
        if( m_targetLengthCtrl->GetValue().IsEmpty() )
            g_lastTargetSkew = PNS::MEANDER_SETTINGS::SKEW_UNCONSTRAINED;
        else
            g_lastTargetSkew = m_targetLength.GetIntValue();

        if( g_lastTargetSkew != m_constraint.GetValue().Opt() )
            m_settings.SetTargetSkew( g_lastTargetSkew );
        else
            m_settings.m_targetSkew = m_constraint.GetValue();
    }
    else
    {
        if( m_targetLengthCtrl->GetValue().IsEmpty() )
            g_lastTargetLength = PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED;
        else
            g_lastTargetLength = m_targetLength.GetIntValue();

        if( g_lastTargetLength != m_constraint.GetValue().Opt() )
            m_settings.SetTargetLength( g_lastTargetLength );
        else
            m_settings.SetTargetLength( m_constraint.GetValue() );
    }

    m_settings.m_overrideCustomRules = m_overrideCustomRules->GetValue();

    m_settings.m_minAmplitude = m_minA.GetIntValue();
    m_settings.m_maxAmplitude = m_maxA.GetIntValue();
    m_settings.m_spacing = m_spacing.GetIntValue();
    m_settings.m_cornerStyle = m_cornerCtrl->GetSelection() ? PNS::MEANDER_STYLE_ROUND
                                                            : PNS::MEANDER_STYLE_CHAMFER;
    m_settings.m_cornerRadiusPercentage = m_r.GetIntValue();
    m_settings.m_singleSided = m_singleSided->GetValue();

    return true;
}


void DIALOG_TUNING_PATTERN_PROPERTIES::onOverrideCustomRules( wxCommandEvent& event )
{
    m_targetLength.Enable( event.IsChecked() || m_constraint.IsNull() );

    if( !event.IsChecked() && !m_constraint.IsNull() )
    {
        m_targetLength.SetValue( m_constraint.GetValue().Opt() );
        m_sourceInfo->Show( true );
    }
    else
    {
        m_sourceInfo->Show( false );
    }
}
