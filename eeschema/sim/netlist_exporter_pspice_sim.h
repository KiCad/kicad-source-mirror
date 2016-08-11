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

#ifndef NETLIST_EXPORTER_PSPICE_SIM_H
#define NETLIST_EXPORTER_PSPICE_SIM_H

#include <netlist_exporters/netlist_exporter_pspice.h>
#include <vector>

#include "sim_types.h"

/// Special netlist exporter flavor that allows to override simulation commands
class NETLIST_EXPORTER_PSPICE_SIM : public NETLIST_EXPORTER_PSPICE
{
public:
    NETLIST_EXPORTER_PSPICE_SIM( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs,
            SEARCH_STACK* aPaths = NULL ) :
        NETLIST_EXPORTER_PSPICE( aMasterList, aLibs, aPaths )
    {
    }

    /**
     * @brief Returns name of Spice dataset for a specific plot.
     * @param aName is name of the measured net or device
     * @param aType describes the type of expected plot
     * @param aParam is an optional parameter for devices, if absent it will return current (only
     * for passive devices).
     * @return Empty string if query is invalid, otherwise a plot name that
     * can be requested from the simulator.
     */
    wxString GetSpiceVector( const wxString& aName, SIM_PLOT_TYPE aType,
            const wxString& aParam = wxEmptyString ) const;

    /**
     * @brief Returns name of Spice device corresponding to a schematic component.
     * @param aComponent is the component reference.
     * @return Spice device name or empty string if there is no such component in the netlist.
     */
    wxString GetSpiceDevice( const wxString& aComponent ) const;

    /**
     * @brief Returns a list of currents that can be probed in a Spice primitive.
     */
    static const std::vector<wxString>& GetCurrents( SPICE_PRIMITIVE aPrimitive );

    /**
     * @brief Overrides the simulation command directive.
     */
    void SetSimCommand( const wxString& aCmd )
    {
        m_simCommand = aCmd;
    }

    /**
     * @brief Returns the simulation command directive.
     */
    const wxString& GetSimCommand() const
    {
        return m_simCommand;
    }

    /**
     * @brief Clears the simulation command directive.
     */
    void ClearSimCommand()
    {
        m_simCommand.Clear();
    }

    /**
     * @brief Returns simulation type basing on the simulation command directives.
     * Simulation directives set using SetSimCommand() have priority over the ones placed in
     * schematic sheets.
     */
    SIM_TYPE GetSimType();

    /**
     * @brief Returns simulation command directives placed in schematic sheets (if any).
     */
    wxString GetSheetSimCommand();

    /**
     * @brief Determines if a directive is a simulation command.
     */
    static bool IsSimCommand( const wxString& aCmd )
    {
        return CommandToSimType( aCmd ) != ST_UNKNOWN;
    }

    /**
     * @brief Returns simulation type basing on a simulation command directive.
     */
    static SIM_TYPE CommandToSimType( const wxString& aCmd );

protected:
    void writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const override;

private:

    ///> Custom simulation command (has priority over the schematic sheet simulation commands)
    wxString m_simCommand;
};

#endif /* NETLIST_EXPORTER_PSPICE_SIM_H */
