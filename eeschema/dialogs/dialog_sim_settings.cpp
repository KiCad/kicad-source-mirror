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

#include <wx/tokenzr.h>

/// @todo ngspice offers more types of analysis,
//so there are a few tabs missing (e.g. pole-zero, distortion, sensitivity)

// Helper function to shorten conditions
static bool empty( const wxTextEntryBase* aCtrl )
{
    return aCtrl->GetValue().IsEmpty();
}


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

    // Hide pages that aren't fully implemented yet
    // wxPanel::Hide() isn't enough on some platforms
    m_simPages->RemovePage( m_simPages->FindPage( m_pgDistortion ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgNoise ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgPoleZero ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgSensitivity ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgTransferFunction ) );

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
        if( !m_dcEnable1->IsChecked() && !m_dcEnable2->IsChecked() )
        {
            DisplayError( this, _( "You need to enable at least one source" ) );
            return false;
        }

        wxString simCmd = wxString( ".dc " );

        if( m_dcEnable1->IsChecked() )
        {
            if( empty( m_dcSource1 ) )
            {
                DisplayError( this, _( "You need to select DC source (sweep 1)" ) );
                return false;
            }

            /// @todo for some reason it does not trigger the assigned SPICE_VALIDATOR,
            // hence try..catch below
            if( !m_dcStart1->Validate() || !m_dcStop1->Validate() || !m_dcIncr1->Validate() )
                return false;

            try
            {
                wxString dcSource = m_exporter->GetSpiceDevice( m_dcSource1->GetValue() );

                simCmd += wxString::Format( "%s %s %s %s",
                    dcSource,
                    SPICE_VALUE( m_dcStart1->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcStop1->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcIncr1->GetValue() ).ToSpiceString() );
            }
            catch( std::exception& e )
            {
                DisplayError( this, e.what() );
                return false;
            }
            catch( const KI_PARAM_ERROR& e )
            {
                DisplayError( this, e.What() );
                return false;
            }
            catch( ... )
            {
                return false;
            }
        }

        if( m_dcEnable2->IsChecked() )
        {
            if( empty( m_dcSource2 ) )
            {
                DisplayError( this, _( "You need to select DC source (sweep 2)" ) );
                return false;
            }

            if( m_dcEnable1->IsChecked() && m_dcSource1->GetValue() == m_dcSource2->GetValue() )
            {
                DisplayError( this, _( "Source 1 and Source 2 must be different" ) );
                return false;
            }

            /// @todo for some reason it does not trigger the assigned SPICE_VALIDATOR,
            // hence try..catch below
            if( !m_dcStart2->Validate() || !m_dcStop2->Validate() || !m_dcIncr2->Validate() )
                return false;

            try
            {
                wxString dcSource = m_exporter->GetSpiceDevice( m_dcSource2->GetValue() );

                if( m_dcEnable1->IsChecked() )
                    simCmd += " ";

                simCmd += wxString::Format( "%s %s %s %s",
                    dcSource,
                    SPICE_VALUE( m_dcStart2->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcStop2->GetValue() ).ToSpiceString(),
                    SPICE_VALUE( m_dcIncr2->GetValue() ).ToSpiceString() );
            }
            catch( std::exception& e )
            {
                DisplayError( this, e.what() );
                return false;
            }
            catch( const KI_PARAM_ERROR& e )
            {
                DisplayError( this, e.What() );
                return false;
            }
            catch( ... )
            {
                return false;
            }

        }

        m_simCommand = simCmd;
    }


    // Noise analysis
    else if( page == m_pgNoise )
    {
        const NETLIST_EXPORTER_PSPICE::NET_INDEX_MAP& netMap = m_exporter->GetNetIndexMap();

        if( empty( m_noiseMeas ) || empty( m_noiseSrc ) || empty( m_noisePointsNumber )
                || empty( m_noiseFreqStart ) || empty( m_noiseFreqStop ) )
            return false;

        wxString ref = empty( m_noiseRef )
            ? wxString() : wxString::Format( ", %d", netMap.at( m_noiseRef->GetValue() ) );

        wxString noiseSource = m_exporter->GetSpiceDevice( m_noiseSrc->GetValue() );

        // Add voltage source prefix if needed
        if( noiseSource[0] != 'v' && noiseSource[0] != 'V' )
            noiseSource += 'v' + noiseSource;

        m_simCommand = wxString::Format( ".noise v(%d%s) %s %s %s %s %s",
            netMap.at( m_noiseMeas->GetValue() ), ref,
            noiseSource, scaleToString( m_noiseScale->GetSelection() ),
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

        wxString initial = empty( m_transInitial )
            ? "" : SPICE_VALUE( m_transInitial->GetValue() ).ToSpiceString();

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

    m_simCommand.Trim();
    updateNetlistOpts();

    return true;
}


bool DIALOG_SIM_SETTINGS::TransferDataToWindow()
{
    /// @todo one day it could interpret the sim command and fill out appropriate fields..
    if( empty( m_customTxt ) )
        loadDirectives();

    if( m_simCommand.IsEmpty() && !empty( m_customTxt ) )
        return parseCommand( m_customTxt->GetValue() );

    return true;
}


int DIALOG_SIM_SETTINGS::ShowModal()
{
    // Fill out comboboxes that allows one to select nets
    // Map comoboxes to their current values
    std::map<wxComboBox*, wxString> cmbNet = {
        { m_noiseMeas, m_noiseMeas->GetStringSelection() },
        { m_noiseRef, m_noiseRef->GetStringSelection() }
    };

    for( auto c : cmbNet )
        c.first->Clear();

    for( const auto& net : m_exporter->GetNetIndexMap() )
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


    // Fill out comboboxes that allows one to select power sources
    std::map<wxComboBox*, wxString> cmbSrc = {
        { m_dcSource1, m_dcSource1->GetStringSelection() },
        { m_dcSource2, m_dcSource2->GetStringSelection() },
        { m_noiseSrc, m_noiseSrc->GetStringSelection() },
    };

    for( auto c : cmbSrc )
        c.first->Clear();

    for( const auto& item : m_exporter->GetSpiceItems() )
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


bool DIALOG_SIM_SETTINGS::parseCommand( const wxString& aCommand )
{
    if( aCommand.IsEmpty() )
        return false;

    wxStringTokenizer tokenizer( aCommand, " " );
    wxString tkn = tokenizer.GetNextToken().Lower();

    try {
        if( tkn == ".ac" )
        {
            m_simPages->SetSelection( m_simPages->FindPage( m_pgAC ) );

            tkn = tokenizer.GetNextToken().Lower();

            if( tkn == "dec" )
                m_acScale->SetSelection( 0 );
            if( tkn == "oct" )
                m_acScale->SetSelection( 1 );
            if( tkn == "lin" )
                m_acScale->SetSelection( 2 );
            else
                return false;

            // If the fields below are empty, it will be caught by the exception handler
            m_acPointsNumber->SetValue( tokenizer.GetNextToken() );
            m_acFreqStart->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
            m_acFreqStop->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
        }

        else if( tkn == ".dc" )
        {
            m_simPages->SetSelection( m_simPages->FindPage( m_pgDC ) );

            tkn = tokenizer.GetNextToken();

            if( !tkn.IsEmpty() )
            {
                m_dcSource1->SetValue( tkn );
                m_dcStart1->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
                m_dcStop1->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
                m_dcIncr1->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );

                // Check the 'Enabled' field, if all values are filled
                m_dcEnable1->SetValue( !empty( m_dcSource1 ) && !empty( m_dcStart1 )
                        && !empty( m_dcStop1 ) && !empty( m_dcIncr1 ) );
            }

            tkn = tokenizer.GetNextToken();

            if( !tkn.IsEmpty() )
            {
                m_dcSource2->SetValue( tkn );
                m_dcStart2->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
                m_dcStop2->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
                m_dcIncr2->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );

                // Check the 'Enabled' field, if all values are filled
                m_dcEnable2->SetValue( !empty( m_dcSource2 ) && !empty( m_dcStart2 )
                        && !empty( m_dcStop2 ) && !empty( m_dcIncr2 ) );
            }

            // Check if the directive is complete
            if( !m_dcEnable1->IsChecked() || !m_dcEnable2->IsChecked() )
                return false;
        }

        else if( tkn == ".tran" )
        {
            m_simPages->SetSelection( m_simPages->FindPage( m_pgTransient ) );

            // If the fields below are empty, it will be caught by the exception handler
            m_transStep->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
            m_transFinal->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );

            // Initial time is an optional field
            tkn = tokenizer.GetNextToken();

            if( !tkn.IsEmpty() )
                m_transInitial->SetValue( SPICE_VALUE( tkn ).ToSpiceString() );
        }

        // Custom directives
        else if( !empty( m_customTxt ) )
        {
            m_simPages->SetSelection( m_simPages->FindPage( m_pgCustom ) );
        }
    }
    catch( ... )
    {
        // Nothing really bad has happened
        return false;
    }

    return true;
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
