/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <settings/nested_settings.h>

#include "sim_types.h"

#include <string>
#include <vector>
#include <complex>
#include <memory>

#include <wx/string.h>

class SPICE_REPORTER;

typedef std::complex<double> COMPLEX;


/**
 * Storage for simulator specific settings.
 */
class SPICE_SIMULATOR_SETTINGS : public NESTED_SETTINGS
{
public:
    SPICE_SIMULATOR_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~SPICE_SIMULATOR_SETTINGS() {}

    virtual bool operator==( const SPICE_SIMULATOR_SETTINGS& aRhs ) const = 0;

    bool operator!=( const SPICE_SIMULATOR_SETTINGS& aRhs ) const { return !( *this == aRhs ); }
};


class SPICE_SIMULATOR
{
public:
    SPICE_SIMULATOR() :
        m_reporter( nullptr ),
        m_settings( nullptr )
    {}

    virtual ~SPICE_SIMULATOR() {}

    ///< Create a simulator instance of particular type (currently only ngspice is handled)
    static std::shared_ptr<SPICE_SIMULATOR> CreateInstance( const std::string& aName );

    /*
     * Initialize the simulator using the optional \a aSettings.
     *
     * @param aSettings [in] are the simulator specific settings.  Can be null if no settings need
     *                  to be initialized.
     */
    virtual void Init( const SPICE_SIMULATOR_SETTINGS* aSettings = nullptr ) = 0;

    /**
     * Load a netlist for the simulation.
     *
     * @return True in case of success, false otherwise.
     */
    virtual bool LoadNetlist( const std::string& aNetlist ) = 0;

    /**
     * Execute the simulation with currently loaded netlist.
     *
     * @return True in case of success, false otherwise.
     */
    virtual bool Run() = 0;

    /**
     * Halt the simulation.
     *
     * @return True in case of success, false otherwise.
     */
    virtual bool Stop() = 0;

    /**
     * Check if simulation is running at the moment.
     *
     * @return True if simulation is currently executed.
     */
    virtual bool IsRunning() = 0;

    /**
     * Execute a Spice command as if it was typed into console.
     *
     * @param aCmd is the command to be issued.
     */
    virtual bool Command( const std::string& aCmd ) = 0;

    ///< Return X axis name for a given simulation type
    virtual std::string GetXAxis( SIM_TYPE aType ) const = 0;

    ///< Set a #SPICE_REPORTER object to receive the simulation log.
    virtual void SetReporter( SPICE_REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    /**
     * Return a list with all vectors generated in current simulation.
     *
     * @return List of vector names. ?May not match to the net name elements.
     */
    virtual std::vector<std::string> AllPlots() const = 0;

    /**
     * Return a requested vector with complex values. If the vector is real, then
     * the imaginary part is set to 0 in all values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<COMPLEX> GetPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with real values. If the vector is complex, then
     * the real part is returned.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetRealPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with imaginary values. If the vector is complex, then
     * the imaginary part is returned. If the vector is reql, then only zeroes are returned.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetImagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with magnitude values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetMagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * Return a requested vector with phase values.
     *
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetPhasePlot( const std::string& aName, int aMaxLen = -1 ) = 0;

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
    std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& Settings() { return m_settings; }

    const std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& Settings() const { return m_settings; }

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
    SPICE_REPORTER* m_reporter;

    ///< We don't own this.  We are just borrowing it from the #SCHEMATIC_SETTINGS.
    std::shared_ptr<SPICE_SIMULATOR_SETTINGS> m_settings;
};

#endif /* SPICE_SIMULATOR_H */
