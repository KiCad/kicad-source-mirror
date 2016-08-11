/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_sim_settings.h"
#include <sim/netlist_exporter_pspice_sim.h>
#include <confirm.h>

/// @todo ngspice offers more types of analysis,
//so there are a few tabs missing (e.g. pole-zero, distortion, sensitivity)

DIALOG_SIM_SETTINGS::DIALOG_SIM_SETTINGS( wxWindow* aParent )
    : DIALOG_SIM_SETTINGS_BASE( aParent ), m_exporter( nullptr ), m_spiceEmptyValidator( true )
{
    m_posIntValidator.SetMin( 1 );

    m_acPointsNumber->SetValidator( m_posIntValidator );
    m_acFreqStart->SetValidator( m_spiceValidator );
    m_acFreqStop->SetValidator( m_spiceValidator );

    m_dcStart1->SetValidator( m_spiceValidator );
    m_dcStop1->SetValidator( m_spiceValidator );
    m_dcIncr1->SetValidator( m_spiceValidator );

    m_dcStart2->SetValidator( m_spiceValidator );
    m_dcStop2->SetValidator( m_spiceValidator );
    m_dcIncr2->SetValidator( m_spiceValidator );

    m_noisePointsNumber->SetValidator( m_posIntValidator );
    m_noiseFreqStart->SetValidator( m_spiceValidator );
    m_noiseFreqStop->SetValidator( m_spiceValidator );

    m_transStep->SetValidator( m_spiceValidator );
    m_transFinal->SetValidator( m_spiceValidator );
    m_transInitial->SetValidator( m_spiceEmptyValidator );

    m_sdbSizerOK->SetDefault();
    updateNetlistOpts();
}


bool DIALOG_SIM_SETTINGS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    wxWindow* page = m_simPages->GetCurrentPage();

    // AC analysis
    if( page == m_pgAC )
    {
        if( !m_pgAC->Validate() )
            return false;

        m_simCommand = wxString::Format( ".ac %s %s %s %s",
            scaleToString( m_acScale->GetSelection() ),
            m_acPointsNumber->GetValue(),
            SPICE_VALUE( m_acFreqStart->GetValue() ).ToSpiceString(),
            SPICE_VALUE( m_acFreqStop->GetValue() ).ToSpiceString() );
    }


    // DC transfer analysis
    else if( page == m_pgDC )
    {
        // At least one source has to be enabled
        if( !m_dcEnable1->IsChecked() && !m_dcEnable1->IsChecked() )
        {
            DisplayError( this, wxT( "You need to enable at least one source" ) );
            return false;
        }

        wxString simCmd = wxString( ".dc " );

        if( m_dcEnable1->IsChecked() )
        {
            if( m_dcSource1->GetValue().IsEmpty() )
            {
                DisplayError( this, wxT( "You need to select DC source (sweep 1)" ) );
                return false;
            }

            /// @todo for some reason it does not trigger the assigned SPICE_VALIDATOR,
            // hence try..catch below
            if( !m_dcStart1->Validate() || !m_dcStop1->Validate() || !m_dcIncr1->Validate() )
                return false;

            try
            {
                simCmd += wxString::Format( "v%s %s %s %s",
                    m_dcSource1->GetValue(),
                    SPICE_VALUE( m_dcStart1->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcStop1->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcIncr1->GetValue() ).ToSpiceString() );
            }
            catch( std::exception& e )
            {
                DisplayError( this, e.what() );
                return false;
            }
        }

        if( m_dcEnable2->IsChecked() )
        {
            if( m_dcSource2->GetValue().IsEmpty() )
            {
                DisplayError( this, wxT( "You need to select DC source (sweep 2)" ) );
                return false;
            }

            /// @todo for some reason it does not trigger the assigned SPICE_VALIDATOR,
            // hence try..catch below
            if( !m_dcStart2->Validate() || !m_dcStop2->Validate() || !m_dcIncr2->Validate() )
                return false;

            try
            {
                simCmd += wxString::Format( "v%s %s %s %s",
                    m_dcSource2->GetValue(),
                    SPICE_VALUE( m_dcStart2->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcStop2->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcIncr2->GetValue() ).ToSpiceString() );
            }
            catch( std::exception& e )
            {
                DisplayError( this, e.what() );
                return false;
            }
        }

        m_simCommand = simCmd;
    }


    // Noise analysis
    else if( page == m_pgNoise )
    {
        const NETLIST_EXPORTER_PSPICE::NET_INDEX_MAP& netMap = m_exporter->GetNetIndexMap();

        if( m_noiseMeas->GetValue().IsEmpty() || m_noiseSrc->GetValue().IsEmpty() ||
                m_noisePointsNumber->IsEmpty() || m_noiseFreqStart->IsEmpty() ||
                m_noiseFreqStop->IsEmpty() )
            return false;

        wxString ref = m_noiseRef->GetValue().IsEmpty() ? wxString()
            : wxString::Format( ", %d", netMap.at( m_noiseRef->GetValue() ) );

        m_simCommand = wxString::Format( ".noise v(%d%s) v%s %s %s %s %s",
            netMap.at( m_noiseMeas->GetValue() ), ref,
            m_noiseSrc->GetValue(), scaleToString( m_noiseScale->GetSelection() ),
            m_noisePointsNumber->GetValue(),
            SPICE_VALUE( m_noiseFreqStart->GetValue() ).ToSpiceString(),
            SPICE_VALUE( m_noiseFreqStop->GetValue() ).ToSpiceString() );
    }


    // DC operating point analysis
    else if( page == m_pgOP )
    {
        m_simCommand = wxString( ".op" );
    }


    // Transient analysis
    else if( page == m_pgTransient )
    {
        if( !m_pgTransient->Validate() )
            return false;

        wxString initial =
            m_transInitial->IsEmpty() ? "" : SPICE_VALUE( m_transInitial->GetValue() ).ToSpiceString();

        m_simCommand = wxString::Format( ".tran %s %s %s",
            SPICE_VALUE( m_transStep->GetValue() ).ToSpiceString(),
            SPICE_VALUE( m_transFinal->GetValue() ).ToSpiceString(),
            initial );
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

    updateNetlistOpts();

    return true;
}


bool DIALOG_SIM_SETTINGS::TransferDataToWindow()
{
    /// @todo one day it could interpret the sim command and fill out appropriate fields..
    if( m_customTxt->IsEmpty() )
        loadDirectives();

    return true;
}


int DIALOG_SIM_SETTINGS::ShowModal()
{
    // Fill out comboboxes that allow to select nets
    // Map comoboxes to their current values
    std::map<wxComboBox*, wxString> cmbNet = {
        { m_noiseMeas, m_noiseMeas->GetStringSelection() },
        { m_noiseRef, m_noiseRef->GetStringSelection() }
    };

    for( auto c : cmbNet )
        c.first->Clear();

    for( auto net : m_exporter->GetNetIndexMap() )
    {
        for( auto c : cmbNet )
            c.first->Append( net.first );
    }

    // Try to restore the previous selection, if possible
    for( auto c : cmbNet )
    {
        int idx = c.first->FindString( c.second );

        if( idx != wxNOT_FOUND )
                c.first->SetSelection( idx );
    }


    // Fill out comboboxes that allow to select power sources
    std::map<wxComboBox*, wxString> cmbSrc = {
        { m_dcSource1, m_dcSource1->GetStringSelection() },
        { m_dcSource2, m_dcSource2->GetStringSelection() },
        { m_noiseSrc, m_noiseSrc->GetStringSelection() },
    };

    for( auto c : cmbSrc )
        c.first->Clear();

    for( auto item : m_exporter->GetSpiceItems() )
    {
        if( item.m_primitive == 'V' )
        {
            for( auto c : cmbSrc )
                c.first->Append( item.m_refName );
        }
    }

    // Try to restore the previous selection, if possible
    for( auto c : cmbSrc )
    {
        int idx = c.first->FindString( c.second );

        if( idx != wxNOT_FOUND )
                c.first->SetSelection( idx );
    }

    return DIALOG_SIM_SETTINGS_BASE::ShowModal();
}


void DIALOG_SIM_SETTINGS::loadDirectives()
{
    if( m_exporter )
        m_customTxt->SetValue( m_exporter->GetSheetSimCommand() );
}


void DIALOG_SIM_SETTINGS::updateNetlistOpts()
{
    m_netlistOpts = NET_ALL_FLAGS;

    if( !m_fixPassiveVals->IsChecked() )
        m_netlistOpts &= ~NET_ADJUST_PASSIVE_VALS;

    if( !m_fixIncludePaths->IsChecked() )
        m_netlistOpts &= ~NET_ADJUST_INCLUDE_PATHS;
}
