/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef DIALOG_SIM_COMMAND_H
#define DIALOG_SIM_COMMAND_H

#include "dialog_sim_command_base.h"
#include <netlist_exporter_spice.h>
#include <sim/spice_value.h>

#include <wx/valnum.h>

class SPICE_CIRCUIT_MODEL;
class SPICE_SETTINGS;
class SIMULATOR_FRAME;
class SIM_TAB;


class DIALOG_SIM_COMMAND : public DIALOG_SIM_COMMAND_BASE
{
public:
    DIALOG_SIM_COMMAND( SIMULATOR_FRAME* aParent,
                        std::shared_ptr<SPICE_CIRCUIT_MODEL> aCircuitModel,
                        std::shared_ptr<SPICE_SETTINGS>& aSettings );

    const wxString& GetSimCommand() const
    {
        return m_simCommand;
    }

    void SetSimCommand( const wxString& aCommand )
    {
        parseCommand( aCommand );
        m_simCommand = aCommand;
        refreshUIControls();
    }

    void SetSimOptions( int aOptions )
    {
        m_fixIncludePaths->SetValue( aOptions &
                                     NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS );
        m_saveAllVoltages->SetValue( aOptions & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES );
        m_saveAllCurrents->SetValue( aOptions & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS );
        m_saveAllDissipations->SetValue( aOptions &
                                         NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS );
        m_saveAllEvents->SetValue( aOptions & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS );
    }

    void SetPlotSettings( const SIM_TAB* aSimTab );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    // The default dialog Validate() calls the validators of all widgets.
    // This is not what we want; We want only validators of the selected page
    // of the notebooks. So disable the wxDialog::Validate(), and let our
    // TransferDataFromWindow doing the job.
    virtual bool Validate() override
    {
        return true;
    }

    void ApplySettings( SIM_TAB* aTab );

private:
    enum SCALE_TYPE
    {
        DECADE,
        OCTAVE,
        LINEAR
    };

    /// Generate events to update UI state.
    void refreshUIControls()
    {
        wxQueueEvent( m_dcEnable2, new wxCommandEvent( wxEVT_CHECKBOX ) );
        wxQueueEvent( m_dcSourceType1, new wxCommandEvent( wxEVT_RADIOBOX ) );
        wxQueueEvent( m_dcSourceType2, new wxCommandEvent( wxEVT_RADIOBOX ) );
        wxQueueEvent( m_inputSignalsFilter, new wxCommandEvent( wxEVT_TEXT ) );
    }

    /**
     * Read values from one DC sweep source to form a part of simulation command.
     *
     * @return string of four SPICE values if values are correct, empty string upon error.
     */
    wxString evaluateDCControls( wxChoice* aDcSource, wxTextCtrl* aDcStart, wxTextCtrl* aDcStop,
                                 wxTextCtrl* aDcIncr );

    /**
     * Update DC sweep source with symbols from schematic.
     */
    void updateDCSources( wxChar aType, wxChoice* aSource );

    /**
     * Update units on labels depending on selected source.
     */
    void updateDCUnits( wxChar aType, wxStaticText* aStartValUnit, wxStaticText* aEndValUnit,
                        wxStaticText* aStepUnit );

    virtual void onInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        finishDialogSettings();
    }

    /**
     * Parse a Spice directive.
     *
     * @param aCommand is the directive to be parsed (e.g. ".tran 10n 1000n").
     */
    void parseCommand( const wxString& aCommand );

    void onLoadDirectives( wxCommandEvent& event ) override
    {
        loadDirectives();
    }

    void onDCEnableSecondSource( wxCommandEvent& event ) override;
    void onSwapDCSources( wxCommandEvent& event ) override;
    void onDCSource1Selected( wxCommandEvent& event ) override;
    void onDCSource2Selected( wxCommandEvent& event ) override;

    void OnUpdateUILockY1( wxUpdateUIEvent& event ) override;
    void OnUpdateUILockY2( wxUpdateUIEvent& event ) override;
    void OnUpdateUILockY3( wxUpdateUIEvent& event ) override;

    static wxString scaleToString( int aOption )
    {
        switch( aOption )
        {
        case DECADE: return wxString( "dec" );
        case OCTAVE: return wxString( "oct" );
        case LINEAR: return wxString( "lin" );
        default:     wxFAIL_MSG( "Unhandled scale type" ); return wxEmptyString;
        }
    }

    void OnCommandType( wxCommandEvent& event ) override;
    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFilterText( wxCommandEvent& event ) override;

    void loadDirectives();

private:
    SIMULATOR_FRAME*                     m_simulatorFrame;
    wxString                             m_simCommand;
    std::shared_ptr<SPICE_CIRCUIT_MODEL> m_circuitModel;
    std::shared_ptr<SPICE_SETTINGS>      m_settings;
    SPICE_VALIDATOR                      m_spiceValidator;
    SPICE_VALIDATOR                      m_spiceEmptyValidator;
    wxIntegerValidator<int>              m_posIntValidator;
    std::set<wxString>                   m_fftInputSignals;
};

#endif /* DIALOG_SIM_COMMAND_H */
