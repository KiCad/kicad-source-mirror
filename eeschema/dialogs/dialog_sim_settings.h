/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_SIM_SETTINGS_BASE_H
#define DIALOG_SIM_SETTINGS_BASE_H

#include "dialog_sim_settings_base.h"
#include <sim/spice_value.h>

#include <wx/valnum.h>

class NETLIST_EXPORTER_PSPICE_SIM;
class SPICE_SIMULATOR_SETTINGS;


class DIALOG_SIM_SETTINGS : public DIALOG_SIM_SETTINGS_BASE
{
public:
    DIALOG_SIM_SETTINGS( wxWindow* aParent, std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& aSettings );

    const wxString& GetSimCommand() const
    {
        return m_simCommand;
    }

    bool SetSimCommand( const wxString& aCommand )
    {
        bool res = parseCommand( aCommand );

        if( res )
            m_simCommand = aCommand;

        return res;
    }

    int GetNetlistOptions() const
    {
        return m_netlistOpts;
    }

    void SetNetlistExporter( NETLIST_EXPORTER_PSPICE_SIM* aExporter )
    {
        m_exporter = aExporter;
    }

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

    int ShowModal() override;

private:
    enum SCALE_TYPE
    {
        DECADE,
        OCTAVE,
        LINEAR
    };

    ///< Generate events to update UI state.
    void refreshUIControls()
    {
        wxQueueEvent( m_dcEnable2, new wxCommandEvent( wxEVT_CHECKBOX ) );
        wxQueueEvent( m_dcSourceType1, new wxCommandEvent( wxEVT_RADIOBOX ) );
        wxQueueEvent( m_dcSourceType2, new wxCommandEvent( wxEVT_RADIOBOX ) );
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
    void updateDCUnits( wxChar aType, wxChoice* aSource, wxStaticText* aStartValUnit,
                        wxStaticText* aEndValUnit, wxStaticText* aStepUnit );

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
     * @return true if the directive was parsed correctly.
     */
    bool parseCommand( const wxString& aCommand );

    void onLoadDirectives( wxCommandEvent& event ) override
    {
        loadDirectives();
    }

    void onDCEnableSecondSource( wxCommandEvent& event ) override;
    void onSwapDCSources( wxCommandEvent& event ) override;
    void onDCSource1Selected( wxCommandEvent& event ) override
    {
        wxChar type =
                m_dcSourceType1->GetString( m_dcSourceType1->GetSelection() ).Upper().GetChar( 0 );
        updateDCSources( type, m_dcSource1 );
        updateDCUnits( type, m_dcSource1, m_src1DCStartValUnit, m_src1DCEndValUnit,
                       m_src1DCStepUnit );
    }

    void onDCSource2Selected( wxCommandEvent& event ) override
    {
        wxChar type =
                m_dcSourceType2->GetString( m_dcSourceType2->GetSelection() ).Upper().GetChar( 0 );
        updateDCSources( type, m_dcSource2 );
        updateDCUnits( type, m_dcSource2, m_src2DCStartValUnit, m_src2DCEndValUnit,
                       m_src2DCStepUnit );
    }

    static wxString scaleToString( int aOption )
    {
        switch( aOption )
        {
        case DECADE:
            return wxString( "dec" );

        case OCTAVE:
            return wxString( "oct" );

        case LINEAR:
            return wxString( "lin" );
        }

        wxASSERT_MSG( false, "Unhandled scale type" );

        return wxEmptyString;
    }

    void loadDirectives();
    void updateNetlistOpts();

    wxString m_simCommand;
    int m_netlistOpts;
    NETLIST_EXPORTER_PSPICE_SIM* m_exporter;
    std::shared_ptr<SPICE_SIMULATOR_SETTINGS> m_settings;
    SPICE_VALIDATOR m_spiceValidator;
    SPICE_VALIDATOR m_spiceEmptyValidator;
    wxIntegerValidator<int> m_posIntValidator;
};

#endif /* DIALOG_SIM_SETTINGS_BASE_H */
