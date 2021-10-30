/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2021 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sim/ngspice.h>

#include <confirm.h>

#include <wx/tokenzr.h>

#include <vector>
#include <utility>

/// @todo ngspice offers more types of analysis,
//so there are a few tabs missing (e.g. pole-zero, distortion, sensitivity)

// Helper function to shorten conditions
static bool empty( const wxTextEntryBase* aCtrl )
{
    return aCtrl->GetValue().IsEmpty();
}


static void setStringSelection( wxChoice* aCtrl, const wxString& aStr )
{
    aCtrl->SetSelection( aCtrl->FindString( aStr ) );
}


static wxString getStringSelection( const wxChoice* aCtrl )
{
    return aCtrl->GetString( aCtrl->GetSelection() );
}


DIALOG_SIM_SETTINGS::DIALOG_SIM_SETTINGS( wxWindow* aParent,
                                          std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& aSettings ) :
        DIALOG_SIM_SETTINGS_BASE( aParent ),
        m_exporter( nullptr ),
        m_settings( aSettings ),
        m_spiceEmptyValidator( true )
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

    refreshUIControls();

    // Hide pages that aren't fully implemented yet
    // wxPanel::Hide() isn't enough on some platforms
    m_simPages->RemovePage( m_simPages->FindPage( m_pgDistortion ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgNoise ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgPoleZero ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgSensitivity ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgTransferFunction ) );

    if( !dynamic_cast<NGSPICE_SIMULATOR_SETTINGS*>( aSettings.get() ) )
        m_compatibilityMode->Show( false );

    m_sdbSizerOK->SetDefault();
    updateNetlistOpts();
}

wxString DIALOG_SIM_SETTINGS::evaluateDCControls( wxChoice* aDcSource, wxTextCtrl* aDcStart,
                                                  wxTextCtrl* aDcStop, wxTextCtrl* aDcIncr )
{
    wxString  dcSource = aDcSource->GetString( aDcSource->GetSelection() );
    wxWindow* ctrlWithError = nullptr;

    if( dcSource.IsEmpty() )
    {
        DisplayError( this, _( "You need to select DC source" ) );
        ctrlWithError = aDcSource;
    }
    /// @todo for some reason it does not trigger the assigned SPICE_VALIDATOR,
    // hence try..catch below
    else if( !aDcStart->Validate() )
        ctrlWithError = aDcStart;
    else if( !aDcStop->Validate() )
        ctrlWithError = aDcStop;
    else if( !aDcIncr->Validate() )
        ctrlWithError = aDcIncr;

    if( ctrlWithError )
    {
        ctrlWithError->SetFocus();
        return wxEmptyString;
    }

    try
    {
        // pick device name from exporter when something different than temperature is selected
        if( dcSource.Cmp( "TEMP" ) )
            dcSource = m_exporter->GetSpiceDevice( dcSource );

        return wxString::Format( "%s %s %s %s", dcSource,
                                 SPICE_VALUE( aDcStart->GetValue() ).ToSpiceString(),
                                 SPICE_VALUE( aDcStop->GetValue() ).ToSpiceString(),
                                 SPICE_VALUE( aDcIncr->GetValue() ).ToSpiceString() );
    }
    catch( std::exception& e )
    {
        DisplayError( this, e.what() );
        return wxEmptyString;
    }
    catch( const KI_PARAM_ERROR& e )
    {
        DisplayError( this, e.What() );
        return wxEmptyString;
    }
    catch( ... )
    {
        return wxEmptyString;
    }
}


bool DIALOG_SIM_SETTINGS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // The simulator dependent settings always get transferred.
    NGSPICE_SIMULATOR_SETTINGS* ngspiceSettings =
            dynamic_cast<NGSPICE_SIMULATOR_SETTINGS*>( m_settings.get() );

    if( ngspiceSettings )
    {
        switch( m_compatibilityModeChoice->GetSelection() )
        {
        case 0: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::USER_CONFIG ); break;
        case 1: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::NGSPICE );     break;
        case 2: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::PSPICE );      break;
        case 3: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::LTSPICE );     break;
        case 4: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::LT_PSPICE );   break;
        case 5: ngspiceSettings->SetModelMode( NGSPICE_MODEL_MODE::HSPICE );      break;
        }
    }

    wxString previousSimCommand = m_simCommand;
    wxWindow* page = m_simPages->GetCurrentPage();

    if( page == m_pgAC )                // AC analysis
    {
        if( !m_pgAC->Validate() )
            return false;

        m_simCommand.Printf( ".ac %s %s %s %s",
                             scaleToString( m_acScale->GetSelection() ),
                             m_acPointsNumber->GetValue(),
                             SPICE_VALUE( m_acFreqStart->GetValue() ).ToSpiceString(),
                             SPICE_VALUE( m_acFreqStop->GetValue() ).ToSpiceString() );
    }
    else if( page == m_pgDC )           // DC transfer analysis
    {
        wxString simCmd = wxString( ".dc " );

        wxString src1 = evaluateDCControls( m_dcSource1, m_dcStart1, m_dcStop1, m_dcIncr1 );

        if( src1.IsEmpty() )
            return false;
        else
            simCmd += src1;

        if( m_dcEnable2->IsChecked() )
        {
            wxString src2 = evaluateDCControls( m_dcSource2, m_dcStart2, m_dcStop2, m_dcIncr2 );

            if( src2.IsEmpty() )
                return false;
            else
                simCmd += " " + src2;

            if( m_dcSource1->GetSelection() == m_dcSource2->GetSelection() )
            {
                DisplayError( this, _( "Source 1 and Source 2 must be different" ) );
                return false;
            }
        }

        m_simCommand = simCmd;
    }
    else if( page == m_pgNoise )        // Noise analysis
    {
        const std::map<wxString, int>& netMap = m_exporter->GetNetIndexMap();

        if( empty( m_noiseMeas ) || empty( m_noiseSrc ) || empty( m_noisePointsNumber )
                || empty( m_noiseFreqStart ) || empty( m_noiseFreqStop ) )
        {
            return false;
        }

        wxString ref;

        if( !empty( m_noiseRef ) )
            ref = wxString::Format( ", %d", netMap.at( m_noiseRef->GetValue() ) );

        wxString noiseSource = m_exporter->GetSpiceDevice( m_noiseSrc->GetValue() );

        // Add voltage source prefix if needed
        if( noiseSource[0] != 'v' && noiseSource[0] != 'V' )
            noiseSource += 'v' + noiseSource;

        m_simCommand.Printf( ".noise v(%d%s) %s %s %s %s %s",
                             netMap.at( m_noiseMeas->GetValue() ), ref,
                             noiseSource, scaleToString( m_noiseScale->GetSelection() ),
                             m_noisePointsNumber->GetValue(),
                             SPICE_VALUE( m_noiseFreqStart->GetValue() ).ToSpiceString(),
                             SPICE_VALUE( m_noiseFreqStop->GetValue() ).ToSpiceString() );
    }
    else if( page == m_pgOP )           // DC operating point analysis
    {
        m_simCommand = wxString( ".op" );
    }
    else if( page == m_pgTransient )    // Transient analysis
    {
        if( !m_pgTransient->Validate() )
            return false;

        wxString initial;

        if( !empty( m_transInitial ) )
            initial = SPICE_VALUE( m_transInitial->GetValue() ).ToSpiceString();

        m_simCommand.Printf( ".tran %s %s %s",
                             SPICE_VALUE( m_transStep->GetValue() ).ToSpiceString(),
                             SPICE_VALUE( m_transFinal->GetValue() ).ToSpiceString(),
                             initial );
    }
    else if( page == m_pgCustom )       // Custom directives
    {
        m_simCommand = m_customTxt->GetValue();
    }
    else
    {
        wxString extendedMsg;

        if( m_simCommand.IsEmpty() )
        {
            KIDIALOG dlg( this, _( "No valid simulation is configured." ), _( "Warning" ),
                          wxOK | wxCANCEL | wxICON_EXCLAMATION | wxCENTER );

            dlg.SetExtendedMessage( _( "A valid simulation can be configured by selecting a "
                                       "simulation tab, setting the simulation parameters and "
                                       "clicking the OK button with the tab selected." ) );
            dlg.SetOKCancelLabels(
                    wxMessageDialog::ButtonLabel( _( "Exit Without Valid Simulation" ) ),
                    wxMessageDialog::ButtonLabel( _( "Configure Valid Simulation" ) ) );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( dlg.ShowModal() == wxID_OK )
                return true;
        }

        return false;
    }

    if( previousSimCommand != m_simCommand )
        m_simCommand.Trim();

    updateNetlistOpts();

    m_settings->SetFixPassiveVals( m_netlistOpts & NET_ADJUST_PASSIVE_VALS );
    m_settings->SetFixIncludePaths( m_netlistOpts & NET_ADJUST_INCLUDE_PATHS );

    return true;
}


bool DIALOG_SIM_SETTINGS::TransferDataToWindow()
{
    /// @todo one day it could interpret the sim command and fill out appropriate fields.
    if( empty( m_customTxt ) )
        loadDirectives();

    m_fixPassiveVals->SetValue( m_settings->GetFixPassiveVals() );
    m_fixIncludePaths->SetValue( m_settings->GetFixIncludePaths() );
    updateNetlistOpts();

    NGSPICE_SIMULATOR_SETTINGS* ngspiceSettings =
            dynamic_cast<NGSPICE_SIMULATOR_SETTINGS*>( m_settings.get() );

    if( ngspiceSettings )
    {
        switch( ngspiceSettings->GetModelMode() )
        {
        case NGSPICE_MODEL_MODE::USER_CONFIG: m_compatibilityModeChoice->SetSelection( 0 ); break;
        case NGSPICE_MODEL_MODE::NGSPICE:     m_compatibilityModeChoice->SetSelection( 1 ); break;
        case NGSPICE_MODEL_MODE::PSPICE:      m_compatibilityModeChoice->SetSelection( 2 ); break;
        case NGSPICE_MODEL_MODE::LTSPICE:     m_compatibilityModeChoice->SetSelection( 3 ); break;
        case NGSPICE_MODEL_MODE::LT_PSPICE:   m_compatibilityModeChoice->SetSelection( 4 ); break;
        case NGSPICE_MODEL_MODE::HSPICE:      m_compatibilityModeChoice->SetSelection( 5 ); break;
        default:
            wxFAIL_MSG( wxString::Format( "Unknown NGSPICE_MODEL_MODE %d.",
                                          ngspiceSettings->GetModelMode() ) );
            break;
        }
    }

    if( !m_dcSource1->GetCount() )
    {
        wxChar type1 = getStringSelection( m_dcSourceType1 ).Upper().GetChar( 0 );
        updateDCSources( type1, m_dcSource1 );
    }

    if( !m_dcSource2->GetCount() )
    {
        wxChar type2 = getStringSelection( m_dcSourceType2 ).Upper().GetChar( 0 );
        updateDCSources( type2, m_dcSource2 );
    }

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

    return DIALOG_SIM_SETTINGS_BASE::ShowModal();
}


void DIALOG_SIM_SETTINGS::updateDCSources( wxChar aType, wxChoice* aSource )
{
    wxString prevSelection;

    if( !aSource->IsEmpty() )
        prevSelection = aSource->GetString( aSource->GetSelection() );

    std::set<wxString> sourcesList;
    bool               enableSrcSelection = true;

    if( aType != 'T' )
    {
        for( const auto& item : m_exporter->GetSpiceItems() )
        {
            if( item.m_primitive == aType && !item.m_refName.IsEmpty() )
                sourcesList.insert( item.m_refName );
        }

        if( aSource == m_dcSource2 && !m_dcEnable2->IsChecked() )
            enableSrcSelection = false;
    }
    else
    {
        prevSelection = wxT( "TEMP" );
        sourcesList.insert( prevSelection );
        enableSrcSelection = false;
    }

    aSource->Enable( enableSrcSelection );

    aSource->Clear();

    for( const wxString& src : sourcesList )
        aSource->Append( src );

    // Try to restore the previous selection, if possible
    aSource->SetStringSelection( prevSelection );
}


bool DIALOG_SIM_SETTINGS::parseCommand( const wxString& aCommand )
{
    if( aCommand.IsEmpty() )
        return false;

    wxStringTokenizer tokenizer( aCommand, " " );
    wxString tkn = tokenizer.GetNextToken().Lower();

    try
    {
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
            SPICE_DC_PARAMS src1, src2;
            src2.m_vincrement = SPICE_VALUE( -1 );

            if( !m_exporter->ParseDCCommand( aCommand, &src1, &src2 ) )
                return false;

            m_simPages->SetSelection( m_simPages->FindPage( m_pgDC ) );

            if( src1.m_source.IsSameAs( wxT( "TEMP" ), false ) )
                setStringSelection( m_dcSourceType1, wxT( "TEMP" ) );
            else
                setStringSelection( m_dcSourceType1, src1.m_source.GetChar( 0 ) );

            updateDCSources( src1.m_source.GetChar( 0 ), m_dcSource1 );
            m_dcSource1->SetStringSelection( src1.m_source );
            m_dcStart1->SetValue( src1.m_vstart.ToSpiceString() );
            m_dcStop1->SetValue( src1.m_vend.ToSpiceString() );
            m_dcIncr1->SetValue( src1.m_vincrement.ToSpiceString() );

            if( src2.m_vincrement.ToDouble() != -1 )
            {
                if( src2.m_source.IsSameAs( wxT( "TEMP" ), false ) )
                    setStringSelection( m_dcSourceType2, wxT( "TEMP" ) );
                else
                    setStringSelection( m_dcSourceType2, src2.m_source.GetChar( 0 ) );

                updateDCSources( src2.m_source.GetChar( 0 ), m_dcSource2 );
                m_dcSource2->SetStringSelection( src2.m_source );
                m_dcStart2->SetValue( src2.m_vstart.ToSpiceString() );
                m_dcStop2->SetValue( src2.m_vend.ToSpiceString() );
                m_dcIncr2->SetValue( src2.m_vincrement.ToSpiceString() );

                m_dcEnable2->SetValue( true );
            }

            refreshUIControls();
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
        else if( tkn == ".op" )
        {
            m_simPages->SetSelection( m_simPages->FindPage( m_pgOP ) );
        }
        else if( !empty( m_customTxt ) )        // Custom directives
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


void DIALOG_SIM_SETTINGS::onSwapDCSources( wxCommandEvent& event )
{
    std::vector<std::pair<wxTextEntry*, wxTextEntry*>> textCtrl = { { m_dcStart1, m_dcStart2 },
                                                                    { m_dcStop1, m_dcStop2 },
                                                                    { m_dcIncr1, m_dcIncr2 } };

    for( auto& couple : textCtrl )
    {
        wxString tmp = couple.first->GetValue();
        couple.first->SetValue( couple.second->GetValue() );
        couple.second->SetValue( tmp );
    }

    int src1 = m_dcSource1->GetSelection();
    int src2 = m_dcSource2->GetSelection();

    int sel = m_dcSourceType1->GetSelection();
    m_dcSourceType1->SetSelection( m_dcSourceType2->GetSelection() );
    m_dcSourceType2->SetSelection( sel );

    wxChar type1 = getStringSelection( m_dcSourceType1 ).Upper().GetChar( 0 );
    updateDCSources( type1, m_dcSource1 );
    wxChar type2 = getStringSelection( m_dcSourceType2 ).Upper().GetChar( 0 );
    updateDCSources( type2, m_dcSource2 );

    m_dcSource1->SetSelection( src2 );
    m_dcSource2->SetSelection( src1 );

    updateDCUnits( type1, m_dcSource1, m_src1DCStartValUnit, m_src1DCEndValUnit, m_src1DCStepUnit );
    updateDCUnits( type2, m_dcSource2, m_src2DCStartValUnit, m_src2DCEndValUnit, m_src2DCStepUnit );
}


void DIALOG_SIM_SETTINGS::onDCEnableSecondSource( wxCommandEvent& event )
{
    bool   is2ndSrcEnabled = m_dcEnable2->IsChecked();
    wxChar type = getStringSelection( m_dcSourceType2 ).Upper().GetChar( 0 );

    m_dcSourceType2->Enable( is2ndSrcEnabled );
    m_dcSource2->Enable( is2ndSrcEnabled && type != 'T' );
    m_dcStart2->Enable( is2ndSrcEnabled );
    m_dcStop2->Enable( is2ndSrcEnabled );
    m_dcIncr2->Enable( is2ndSrcEnabled );
}


void DIALOG_SIM_SETTINGS::updateDCUnits( wxChar aType, wxChoice* aSource,
                                         wxStaticText* aStartValUnit, wxStaticText* aEndValUnit,
                                         wxStaticText* aStepUnit )
{
    wxString unit;

    switch( aType )
    {
    case 'V': unit = _( "Volts" );     break;
    case 'I': unit = _( "Amperes" );   break;
    case 'R': unit = _( "Ohms" );      break;
    case 'T': unit = wxT( "\u00B0C" ); break;
    }

    aStartValUnit->SetLabel( unit );
    aEndValUnit->SetLabel( unit );
    aStepUnit->SetLabel( unit );

    m_pgDC->Refresh();
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
