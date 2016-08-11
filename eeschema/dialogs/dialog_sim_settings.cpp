/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_sim_settings.h"
#include <wx/log.h>

/// @todo ngspice offers more types of analysis,
//so there are a few tabs missing (e.g. pole-zero, distortion, sensitivity)

DIALOG_SIM_SETTINGS::DIALOG_SIM_SETTINGS( wxWindow* aParent )
    : DIALOG_SIM_SETTINGS_BASE( aParent ), m_exporter( nullptr )
{
    m_posFloatValidator.SetMin( 0 + std::numeric_limits<double>::epsilon() );
    m_posFloatValidator.SetPrecision( 3 );
    m_posIntValidator.SetMin( 1 );

    m_acPointsNumber->SetValidator( m_posIntValidator );
    m_acFreqStart->SetValidator( m_posFloatValidator );
    m_acFreqStop->SetValidator( m_posFloatValidator );

    m_dcStart1->SetValidator( m_posFloatValidator );
    m_dcStop1->SetValidator( m_posFloatValidator );
    m_dcIncr1->SetValidator( m_posFloatValidator );

    m_dcStart2->SetValidator( m_posFloatValidator );
    m_dcStop2->SetValidator( m_posFloatValidator );
    m_dcIncr2->SetValidator( m_posFloatValidator );

    m_noisePointsNumber->SetValidator( m_posIntValidator );
    m_noiseFreqStart->SetValidator( m_posFloatValidator );
    m_noiseFreqStop->SetValidator( m_posFloatValidator );

    m_transStep->SetValidator( m_posFloatValidator );
    m_transFinal->SetValidator( m_posFloatValidator );
    m_transInitial->SetValidator( m_posFloatValidator );
}


bool DIALOG_SIM_SETTINGS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    wxWindow* page = m_simPages->GetCurrentPage();

    // AC analysis
    if( page == m_pgAC )
    {
        if( m_acPointsNumber->IsEmpty() || m_acFreqStart->IsEmpty() || m_acFreqStop->IsEmpty() )
            return false;

        m_simCommand = wxString::Format( ".ac %s %s %s %s",
            scaleToString( m_acScale->GetSelection() ),
            m_acPointsNumber->GetValue(), m_acFreqStart->GetValue(), m_acFreqStop->GetValue() );
    }


    // DC transfer analysis
    else if( page == m_pgDC )
    {
        // At least one source has to be enabled
        if( !m_dcEnable1->IsChecked() && !m_dcEnable1->IsChecked() )
            return false;

        wxString simCmd = wxString( ".dc " );

        if( m_dcEnable1->IsChecked() )
        {
            if( m_dcSource1->GetValue().IsEmpty() || m_dcStart1->IsEmpty() ||
                    m_dcStop1->IsEmpty() || m_dcIncr1->IsEmpty() )
                return false;

            simCmd += wxString::Format( "%s %s %s %s",
               m_dcSource1->GetValue(), m_dcStart1->GetValue(),
               m_dcStop1->GetValue(), m_dcIncr1->GetValue() );
        }

        if( m_dcEnable2->IsChecked() )
        {
            if( m_dcSource2->GetValue().IsEmpty() || m_dcStart2->IsEmpty() ||
                    m_dcStop2->IsEmpty() || m_dcIncr2->IsEmpty() )
                return false;

            simCmd += wxString::Format( "%s %s %s %s",
               m_dcSource2->GetValue(), m_dcStart2->GetValue(),
               m_dcStop2->GetValue(), m_dcIncr2->GetValue() );
        }

        m_simCommand = simCmd;
    }


    // Noise analysis
    else if( page == m_pgNoise )
    {
        if( m_noiseMeas->GetValue().IsEmpty() || m_noiseSrc->GetValue().IsEmpty() ||
                m_noisePointsNumber->IsEmpty() || m_noiseFreqStart->IsEmpty() ||
                m_noiseFreqStop->IsEmpty() )
            return false;

        // TODO missing node number
        wxString ref = m_noiseRef->GetValue().IsEmpty() ? wxString() : wxString::Format( ", %d", 42 );
        m_simCommand = wxString::Format( ".noise v(%d%s) %s %s %s %s %s",
            42, ref, m_noiseSrc->GetValue(), scaleToString( m_noiseScale->GetSelection() ),
            m_noisePointsNumber->GetValue(), m_noiseFreqStart->GetValue(), m_noiseFreqStop->GetValue() );
    }


    // DC operating point analysis
    else if( page == m_pgOP )
    {
        m_simCommand = wxString( ".op" );
    }


    // Transient analysis
    else if( page == m_pgTransient )
    {
        if( m_transStep->IsEmpty() || m_transFinal->IsEmpty() )
            return false;

        m_simCommand = wxString::Format( ".trans %s %s %s",
            m_transStep->GetValue(), m_transFinal->GetValue(), m_transInitial->GetValue() );
    }


    // Custom directives
    else if( page == m_pgCustom )
    {
        m_simCommand = m_customTxt->GetValue();
    }

    else
    {
        return false;
    }

    return true;
}


bool DIALOG_SIM_SETTINGS::TransferDataToWindow()
{
    /// @todo one day it could interpret the sim command and fill out appropriate fields..
    return true;
}


void DIALOG_SIM_SETTINGS::onLoadDirectives( wxCommandEvent& event )
{
}
