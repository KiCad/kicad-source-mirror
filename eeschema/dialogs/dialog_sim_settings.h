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

#ifndef DIALOG_SIM_SETTINGS_BASE_H
#define DIALOG_SIM_SETTINGS_BASE_H

#include "dialog_sim_settings_base.h"
#include <sim/spice_value.h>

#include <wx/valnum.h>

class NETLIST_EXPORTER_PSPICE_SIM;

class DIALOG_SIM_SETTINGS : public DIALOG_SIM_SETTINGS_BASE
{
public:
    DIALOG_SIM_SETTINGS( wxWindow* aParent );

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
    // of the notbooks. So disable the wxDialog::Validate(), and let our
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

    virtual void onInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    /**
     * @brief Parses a Spice directive.
     * @param aCommand is the directive to be parsed (e.g. ".tran 10n 1000n").
     * @return bool if the directive was parsed correctly.
     */
    bool parseCommand( const wxString& aCommand );

    void onLoadDirectives( wxCommandEvent& event ) override
    {
        loadDirectives();
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

    SPICE_VALIDATOR m_spiceValidator;
    SPICE_VALIDATOR m_spiceEmptyValidator;
    wxIntegerValidator<int> m_posIntValidator;
};

#endif /* DIALOG_SIM_SETTINGS_BASE_H */
