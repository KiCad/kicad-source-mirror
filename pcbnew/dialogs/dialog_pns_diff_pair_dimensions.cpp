/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014-2015  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/text_ctrl_eval.h>
#include <router/pns_sizes_settings.h>
#include <eda_draw_frame.h>
#include <confirm.h>

DIALOG_PNS_DIFF_PAIR_DIMENSIONS::DIALOG_PNS_DIFF_PAIR_DIMENSIONS( EDA_DRAW_FRAME* aParent,
                                                                  PNS::SIZES_SETTINGS& aSizes ) :
    DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE( aParent ),
    m_traceWidth( aParent, m_traceWidthLabel, m_traceWidthText, m_traceWidthUnit ),
    m_traceGap( aParent, m_traceGapLabel, m_traceGapText, m_traceGapUnit ),
    m_viaGap( aParent, m_viaGapLabel, m_viaGapText, m_viaGapUnit ),
    m_sizes( aSizes )
{
    SetupStandardButtons();

    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_PNS_DIFF_PAIR_DIMENSIONS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_traceGap.GetValue() <= 0 )
    {
        DisplayErrorMessage( this, _( "Track gap must be greater than 0." ) );
        m_traceGapText->SetFocus();
        return false;
    }

    // Save widgets' values to settings
    m_sizes.SetDiffPairGap( m_traceGap.GetValue() );
    m_sizes.SetDiffPairViaGap( m_viaGap.GetValue() );
    m_sizes.SetDiffPairWidth( m_traceWidth.GetValue() );

    m_sizes.SetDiffPairGapSource( _( "user choice" ) );
    m_sizes.SetDiffPairWidthSource( _( "user choice" ) );

    return true;
}


bool DIALOG_PNS_DIFF_PAIR_DIMENSIONS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_traceWidth.SetValue( m_sizes.DiffPairWidth() );
    m_traceGap.SetValue( m_sizes.DiffPairGap() );
    m_viaGap.SetValue( m_sizes.DiffPairViaGap() );
    m_viaTraceGapEqual->SetValue( m_sizes.DiffPairViaGapSameAsTraceGap() );
    updateCheckbox();

    return true;
}


void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::updateCheckbox()
{
    m_sizes.SetDiffPairViaGapSameAsTraceGap( m_viaTraceGapEqual->GetValue() );
    m_viaGapText->Enable( !m_viaTraceGapEqual->GetValue() );
    m_viaGapLabel->Enable( !m_viaTraceGapEqual->GetValue() );
    m_viaGapUnit->Enable( !m_viaTraceGapEqual->GetValue() );
}


void DIALOG_PNS_DIFF_PAIR_DIMENSIONS::OnViaTraceGapEqualCheck( wxCommandEvent& event )
{
    updateCheckbox();
}
