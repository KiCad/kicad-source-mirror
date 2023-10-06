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

#include "dialog_meander_properties.h"
#include <router/pns_meander_placer.h>
#include <widgets/text_ctrl_eval.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>

DIALOG_MEANDER_PROPERTIES::DIALOG_MEANDER_PROPERTIES( EDA_DRAW_FRAME* aFrame,
                                                      PNS::MEANDER_SETTINGS& aSettings,
                                                      PNS::ROUTER_MODE aMeanderType ) :
        DIALOG_MEANDER_PROPERTIES_BASE( aFrame ),
        m_minA( aFrame, m_track_minALabel, m_minACtrl, m_minAUnits ),
        m_maxA( aFrame, m_maxALabel, m_maxACtrl, m_maxAUnits ),
        m_spacing( aFrame, m_spacingLabel, m_spacingCtrl, m_spacingUnits ),
        m_r( aFrame, m_rLabel, m_rCtrl, m_rUnits ),
        m_settings( aSettings )
{
    m_r.SetUnits( EDA_UNITS::PERCENT );

    switch( aMeanderType )
    {
    case PNS::PNS_MODE_TUNE_SINGLE:
        m_legend->SetBitmap( KiBitmap( BITMAPS::tune_single_track_length_legend ) );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR:
        m_legend->SetBitmap( KiBitmap( BITMAPS::tune_diff_pair_length_legend ) );
        m_r.Enable( false );
        break;

    case PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW:
        m_legend->SetBitmap( KiBitmap( BITMAPS::tune_diff_pair_skew_legend ) );
        break;

    default:
        break;
    }

    // Bitmap has a new size, so recalculate sizes
    GetSizer()->SetSizeHints(this);
    SetupStandardButtons();

    GetSizer()->SetSizeHints(this);
    Centre();
}


bool DIALOG_MEANDER_PROPERTIES::TransferDataToWindow()
{
    m_minA.SetValue( m_settings.m_minAmplitude );
    m_maxA.SetValue( m_settings.m_maxAmplitude );
    m_spacing.SetValue( m_settings.m_spacing );
    m_cornerCtrl->SetSelection( m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND ? 1 : 0 );
    m_r.SetValue( m_settings.m_cornerRadiusPercentage );
    m_singleSided->SetValue( m_settings.m_singleSided );

    return true;
}


bool DIALOG_MEANDER_PROPERTIES::TransferDataFromWindow()
{
    m_settings.m_minAmplitude = m_minA.GetIntValue();
    m_settings.m_maxAmplitude = m_maxA.GetIntValue();
    m_settings.m_spacing = m_spacing.GetIntValue();
    m_settings.m_cornerStyle = m_cornerCtrl->GetSelection() ? PNS::MEANDER_STYLE_ROUND
                                                            : PNS::MEANDER_STYLE_CHAMFER;
    m_settings.m_cornerRadiusPercentage = m_r.GetValue();
    m_settings.m_singleSided = m_singleSided->GetValue();

    return true;
}
