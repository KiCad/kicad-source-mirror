/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef SPICE_SIMULATOR_H
#define SPICE_SIMULATOR_H

#include "sim_types.h"
#include "spice_settings.h"
#include "simulator.h"

#include <mutex>
#include <string>
#include <vector>
#include <complex>
#include <memory>

#include <wx/string.h>

class SIMULATOR_REPORTER;

typedef std::complex<double> COMPLEX;


class SPICE_SIMULATOR : public SIMULATOR
{
public:
    SPICE_SIMULATOR() :
        SIMULATOR(),
        m_reporter( nullptr ),
        m_settings( nullptr )
    {}

    virtual ~SPICE_SIMULATOR() {}

    /*
     * Initialize the simulator using the optional \a aSettings.
     *
     * @param aSettings [in] are the simulator specific settings.  Can be null if no settings need
     *                  to be initialized.
     */
    virtual void Init( const SPICE_SETTINGS* aSettings = nullptr ) = 0;

    /**
     * Load a netlist for the simulation.
     *
     * @return True in case of success, false otherwise.
     */
    virtual bool LoadNetlist( const std::string& aNetlist ) = 0;

    /**
     * Execute a Spice command as if it was typed into console.
     *
     * @param aCmd is the command to be issued.
     */
    virtual bool Command( const std::string& aCmd ) = 0;

    ///< Return X axis name for a given simulation type
    virtual wxString GetXAxis( SIM_TYPE aType ) const = 0;

    ///< Set a #SIMULATOR_REPORTER object to receive the simulation log.
    virtual void SetReporter( SIMULATOR_REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    /**
     * @return the current simulation plot name (tran1, tran2, etc.)
     */
    virtual wxString CurrentPlotName() const = 0;

    /**
     * @return list of simulation vector (signal) names.
     */
    virtual std::vector<std::string> AllVectors() const = 0;

    /**
     * Return a requested vector with complex values. If the vector is real, then
     * the imaginary part is set to 0 in all values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<COMPLEX> GetComplexVector( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with real values. If the vector is complex, then
     * the real part is returned.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetRealVector( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with imaginary values. If the vector is complex, then
     * the imaginary part is returned. If the vector is reql, then only zeroes are returned.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetImaginaryVector( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with magnitude values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetGainVector( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with phase values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetPhaseVector( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return current SPICE netlist used by the simulator.
     *
     * @return The netlist.
     */
    virtual const std::string GetNetlist() const = 0;

    /**
     * @return a list of simulator setting command strings.
     */
    virtual std::vector<std::string> GetSettingCommands() const = 0;

    /**
     * Return the simulator configuration settings.
     *
     * @return the simulator specific settings.
     */
    std::shared_ptr<SPICE_SETTINGS>& Settings() { return m_settings; }

    const std::shared_ptr<SPICE_SETTINGS>& Settings() const { return m_settings; }

    /**
     * Return a string with simulation name based on enum.
     *
     * @param aType is the enum describing simulation type
     * @param aShortName if true  - return is in format "TRAN", "OP".
     *                   if false - return is in format "Transient", "Operating Point".
     * @return String with requested name as described above.
     */
    static wxString TypeToName( SIM_TYPE aType, bool aShortName );

protected:
    ///< Reporter object to receive simulation log.
    SIMULATOR_REPORTER* m_reporter;

    ///< We don't own this.  We are just borrowing it from the #SCHEMATIC_SETTINGS.
    std::shared_ptr<SPICE_SETTINGS> m_settings;
};

#endif /* SPICE_SIMULATOR_H */
