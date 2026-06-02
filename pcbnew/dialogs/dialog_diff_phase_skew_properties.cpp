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

#include "dialog_diff_phase_skew_properties.h"

#include "widgets/color_swatch.h"
#include <pcb_base_edit_frame.h>


DIALOG_DIFF_PHASE_SKEW_PROPERTIES::DIALOG_DIFF_PHASE_SKEW_PROPERTIES(
        PCB_BASE_EDIT_FRAME* aFrame, PCBNEW_SETTINGS::DIFF_PHASE_SKEW_SETTINGS* aSettings ) :
        DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE( aFrame ),
        m_settings{ aSettings }
{
    m_zeroSwatch->SetSwatchBackground( COLOR4D::BLACK );
    m_zeroSwatch->SetSupportsOpacity( false );

    m_positiveSwatch->SetSwatchBackground( COLOR4D::BLACK );
    m_positiveSwatch->SetSupportsOpacity( false );

    m_negativeSwatch->SetSwatchBackground( COLOR4D::BLACK );
    m_negativeSwatch->SetSupportsOpacity( false );

    m_unknownSwatch->SetSwatchBackground( COLOR4D::BLACK );
    m_unknownSwatch->SetSupportsOpacity( false );

    SetupStandardButtons();
    //SetInitialFocus( m_targetLengthCtrl );
    Centre();
}


bool DIALOG_DIFF_PHASE_SKEW_PROPERTIES::TransferDataToWindow()
{
    m_zeroSwatch->SetSwatchColor( m_settings->m_ZeroSkewColor, false );
    m_positiveSwatch->SetSwatchColor( m_settings->m_PositiveSkewColor, false );
    m_negativeSwatch->SetSwatchColor( m_settings->m_NegativeSkewColor, false );
    m_unknownSwatch->SetSwatchColor( m_settings->m_UnknownSkewColor, false );
    m_logScale->SetValue( m_settings->m_UseLogScale );

    return true;
}


bool DIALOG_DIFF_PHASE_SKEW_PROPERTIES::TransferDataFromWindow()
{
    m_settings->m_ZeroSkewColor = m_zeroSwatch->GetSwatchColor();
    m_settings->m_PositiveSkewColor = m_positiveSwatch->GetSwatchColor();
    m_settings->m_NegativeSkewColor = m_negativeSwatch->GetSwatchColor();
    m_settings->m_UseLogScale = m_logScale->GetValue();

    return true;
}
