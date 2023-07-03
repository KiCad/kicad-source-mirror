/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_sim_command.h"
#include <sim/ngspice_circuit_model.h>
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


DIALOG_SIM_COMMAND::DIALOG_SIM_COMMAND( wxWindow* aParent,
                                        std::shared_ptr<NGSPICE_CIRCUIT_MODEL> aCircuitModel,
                                        std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& aSettings ) :
        DIALOG_SIM_COMMAND_BASE( aParent ),
        m_circuitModel( aCircuitModel ),
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
    m_transMaxStep->SetValidator( m_spiceEmptyValidator );

    wxChar type1 = getStringSelection( m_dcSourceType1 ).Upper().GetChar( 0 );
    updateDCSources( type1, m_dcSource1 );

    wxChar type2 = getStringSelection( m_dcSourceType2 ).Upper().GetChar( 0 );
    updateDCSources( type2, m_dcSource2 );

    // NoiseRef is optional
    m_noiseRef->Append( wxEmptyString );

    for( const std::string& net : m_circuitModel->GetNets() )
    {
        m_noiseMeas->Append( net );
        m_noiseRef->Append( net );
    }

    for( const SPICE_ITEM& item : m_circuitModel->GetItems() )
    {
        if( item.model->GetDeviceType() == SIM_MODEL::DEVICE_T::V )
            m_noiseSrc->Append( item.refName );
    }

    refreshUIControls();

    // Hide pages that aren't fully implemented yet
    // wxPanel::Hide() isn't enough on some platforms
    m_simPages->RemovePage( m_simPages->FindPage( m_pgDistortion ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgPoleZero ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgSensitivity ) );
    m_simPages->RemovePage( m_simPages->FindPage( m_pgTransferFunction ) );

    if( !dynamic_cast<NGSPICE_SIMULATOR_SETTINGS*>( aSettings.get() ) )
        m_compatibilityMode->Show( false );

    SetupStandardButtons();
}

wxString DIALOG_SIM_COMMAND::evaluateDCControls( wxChoice* aDcSource, wxTextCtrl* aDcStart,
                                                 wxTextCtrl* aDcStop, wxTextCtrl* aDcIncr )
{
    wxString  dcSource;
    wxWindow* ctrlWithError = nullptr;

    if( aDcSource->GetSelection() >= 0 )
        dcSource = aDcSource->GetString( aDcSource->GetSelection() );

    if( dcSource.IsEmpty() )
    {
        DisplayError( this, _( "A DC source must be specified." ) );
        ctrlWithError = aDcSource;
    }
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

    // pick device name from exporter when something different than temperature is selected
    if( dcSource.Cmp( "TEMP" ) )
        dcSource = m_circuitModel->GetItemName( std::string( dcSource.ToUTF8() ) );

    return wxString::Format( "%s %s %s %s", dcSource,
                             SPICE_VALUE( aDcStart->GetValue() ).ToSpiceString(),
                             SPICE_VALUE( aDcStop->GetValue() ).ToSpiceString(),
                             SPICE_VALUE( aDcIncr->GetValue() ).ToSpiceString() );
}


bool DIALOG_SIM_COMMAND::TransferDataFromWindow()
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

            if( m_dcSource1->GetStringSelection() == m_dcSource2->GetStringSelection() )
            {
                DisplayError( this, _( "Source 1 and Source 2 must be different." ) );
                return false;
            }
        }

        m_simCommand = simCmd;
    }
    else if( page == m_pgNoise )        // Noise analysis
    {
        wxString output = m_noiseMeas->GetStringSelection();
        wxString ref = m_noiseRef->GetStringSelection();
        wxString noiseSource = m_noiseSrc->GetStringSelection();

        if( m_noiseFreqStart->IsEmpty() || m_noiseFreqStop->IsEmpty() )
        {
            DisplayError( this, _( "A frequency range must be specified." ) );
            return false;
        }

        if( !ref.IsEmpty() )
            ref = wxS( "," ) + m_circuitModel->GetItemName( std::string( ref.ToUTF8() ) );

        m_simCommand.Printf( ".noise v(%s%s) %s %s %s %s %s",
                             output,
                             ref,
                             noiseSource,
                             scaleToString( m_noiseScale->GetSelection() ),
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

        const wxString    spc = wxS( " " );
        const SPICE_VALUE timeStep( m_transStep->GetValue() );
        const SPICE_VALUE finalTime( m_transFinal->GetValue() );

        SPICE_VALUE startTime( 0 );

        if( !empty( m_transInitial ) )
            startTime = SPICE_VALUE( m_transInitial->GetValue() );

        wxString optionals;

        if( m_useInitialConditions->GetValue() )
            optionals = wxS( "uic" );

        if( !empty( m_transMaxStep ) )
        {
            optionals = SPICE_VALUE( m_transMaxStep->GetValue() ).ToSpiceString() + spc + optionals;
        }
        else if( !optionals.IsEmpty() )
        {
            SPICE_VALUE maxStep = ( finalTime - startTime ) / 50.0;

            if( maxStep > timeStep )
                maxStep = timeStep;

            optionals = maxStep.ToSpiceString() + spc + optionals;
        }

        if( !empty( m_transInitial ) )
            optionals = startTime.ToSpiceString() + spc + optionals;
        else if( !optionals.IsEmpty() )
            optionals = wxS( "0 " ) + optionals;

        m_simCommand.Printf( wxS( ".tran %s %s %s" ), timeStep.ToSpiceString(),
                             finalTime.ToSpiceString(), optionals );
    }
    else if( page == m_pgCustom )       // Custom directives
    {
        m_simCommand = m_customTxt->GetValue();
    }
    else
    {
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

    m_settings->SetFixIncludePaths( m_fixIncludePaths->GetValue() );

    return true;
}


bool DIALOG_SIM_COMMAND::TransferDataToWindow()
{
    /// @todo one day it could interpret the sim command and fill out appropriate fields.
    if( empty( m_customTxt ) )
        loadDirectives();

    m_fixIncludePaths->SetValue( m_settings->GetFixIncludePaths() );

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

    if( m_simCommand.IsEmpty() && !empty( m_customTxt ) )
        parseCommand( m_customTxt->GetValue() );

    return true;
}


void DIALOG_SIM_COMMAND::updateDCSources( wxChar aType, wxChoice* aSource )
{
    wxString prevSelection;

    if( !aSource->IsEmpty() )
        prevSelection = aSource->GetString( aSource->GetSelection() );

    std::set<wxString> sourcesList;
    bool               enableSrcSelection = true;

    if( aType != 'T' )
    {
        for( const SPICE_ITEM& item : m_circuitModel->GetItems() )
        {
            if( ( aType == 'R' && item.model->GetDeviceType() == SIM_MODEL::DEVICE_T::R )
            ||  ( aType == 'V' && item.model->GetDeviceType() == SIM_MODEL::DEVICE_T::V )
            ||  ( aType == 'I' && item.model->GetDeviceType() == SIM_MODEL::DEVICE_T::I ) )
            {
                sourcesList.insert( item.refName );
            }
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


void DIALOG_SIM_COMMAND::parseCommand( const wxString& aCommand )
{
    if( aCommand.IsEmpty() )
        return;

    wxStringTokenizer tokenizer( aCommand, " " );
    wxString          token = tokenizer.GetNextToken().Lower();

    if( token == ".ac" )
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgAC ) );

        token = tokenizer.GetNextToken().Lower();

        for( SCALE_TYPE candidate : { DECADE, OCTAVE, LINEAR } )
        {
            if( scaleToString( candidate ) == token )
            {
                m_acScale->SetSelection( candidate );
                break;
            }
        }

        m_acPointsNumber->SetValue( tokenizer.GetNextToken() );
        m_acFreqStart->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
        m_acFreqStop->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
    }
    else if( token == ".dc" )
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgDC ) );

        SPICE_DC_PARAMS src1, src2;
        src2.m_vincrement = SPICE_VALUE( -1 );

        m_circuitModel->ParseDCCommand( aCommand, &src1, &src2 );

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
    else if( token == ".noise" )
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgNoise ) );

        wxString    output;
        wxString    ref;
        wxString    source;
        wxString    scale;
        SPICE_VALUE pts;
        SPICE_VALUE fStart;
        SPICE_VALUE fStop;

        m_circuitModel->ParseNoiseCommand( aCommand, &output, &ref, &source, &scale, &pts,
                                           &fStart, &fStop );

        m_noiseMeas->SetStringSelection( output );
        m_noiseRef->SetStringSelection( ref );
        m_noiseSrc->SetStringSelection( source );

        for( SCALE_TYPE candidate : { DECADE, OCTAVE, LINEAR } )
        {
            if( scaleToString( candidate ) == scale )
            {
                m_noiseScale->SetSelection( candidate );
                break;
            }
        }

        m_noisePointsNumber->SetValue( pts.ToSpiceString() );
        m_noiseFreqStart->SetValue( fStart.ToSpiceString() );
        m_noiseFreqStop->SetValue( fStop.ToSpiceString() );
    }
    else if( token == ".tran" )
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgTransient ) );

        // If the fields below are empty, it will be caught by the exception handler
        m_transStep->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );
        m_transFinal->SetValue( SPICE_VALUE( tokenizer.GetNextToken() ).ToSpiceString() );

        // Initial time is an optional field
        token = tokenizer.GetNextToken();

        if( !token.IsEmpty() )
            m_transInitial->SetValue( SPICE_VALUE( token ).ToSpiceString() );

        // Max step is an optional field
        token = tokenizer.GetNextToken();

        if( !token.IsEmpty() )
            m_transMaxStep->SetValue( SPICE_VALUE( token ).ToSpiceString() );

        // uic is an optional field
        token = tokenizer.GetNextToken();

        if( token.IsSameAs( wxS( "uic" ) ) )
            m_useInitialConditions->SetValue( true );
    }
    else if( token == ".op" )
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgOP ) );
    }
    else if( !empty( m_customTxt ) )        // Custom directives
    {
        m_simPages->SetSelection( m_simPages->FindPage( m_pgCustom ) );
    }
}


void DIALOG_SIM_COMMAND::onSwapDCSources( wxCommandEvent& event )
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


void DIALOG_SIM_COMMAND::onDCEnableSecondSource( wxCommandEvent& event )
{
    bool   is2ndSrcEnabled = m_dcEnable2->IsChecked();
    wxChar type = getStringSelection( m_dcSourceType2 ).Upper().GetChar( 0 );

    m_dcSourceType2->Enable( is2ndSrcEnabled );
    m_dcSource2->Enable( is2ndSrcEnabled && type != 'T' );
    m_dcStart2->Enable( is2ndSrcEnabled );
    m_dcStop2->Enable( is2ndSrcEnabled );
    m_dcIncr2->Enable( is2ndSrcEnabled );
}


void DIALOG_SIM_COMMAND::updateDCUnits( wxChar aType, wxChoice* aSource,
                                         wxStaticText* aStartValUnit, wxStaticText* aEndValUnit,
                                         wxStaticText* aStepUnit )
{
    wxString unit;

    switch( aType )
    {
    case 'V': unit = wxS( "V" );  break;
    case 'I': unit = wxS( "A" );  break;
    case 'R': unit = wxS( "Ω" );  break;
    case 'T': unit = wxS( "°C" ); break;
    }

    aStartValUnit->SetLabel( unit );
    aEndValUnit->SetLabel( unit );
    aStepUnit->SetLabel( unit );

    m_pgDC->Refresh();
}


void DIALOG_SIM_COMMAND::loadDirectives()
{
    if( m_circuitModel )
        m_customTxt->SetValue( m_circuitModel->GetSchTextSimCommand() );
}


