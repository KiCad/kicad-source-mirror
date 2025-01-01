/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Sylwester Kocjan <s.kocjan@o2.pl>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <mutex>
#include <memory>

class SPICE_SIMULATOR;
class REPORTER;

class SIMULATION_MODEL
{
public:
    virtual ~SIMULATION_MODEL() {};
};


class SIMULATOR
{
public:
    SIMULATOR() :
        m_simModel( nullptr )
    {}

    virtual ~SIMULATOR() {}

    ///< Create a simulator instance of particular type (currently only ngspice is handled)
    static std::shared_ptr<SPICE_SIMULATOR> CreateInstance( const std::string& aName );

    /*
     * @return mutex for exclusive access to the simulator.
     */
    std::mutex& GetMutex()
    {
        return m_mutex;
    }

    /**
     * Point out the model that will be used in future simulations.
     *
     * @return True in case of success, false otherwise.
     */
    virtual bool Attach( const std::shared_ptr<SIMULATION_MODEL>& aModel,
                         const wxString& aSimCommand, unsigned aSimOptions,
                         const wxString& aInputPath, REPORTER& aReporter )
    {
        m_simModel = aModel;
        return true;
    }

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
     * Cleans simulation data (i.e. all vectors)
     *
     */
    virtual void Clean() = 0;

protected:
    ///< Model that should be simulated.
    std::shared_ptr<SIMULATION_MODEL> m_simModel;

private:
    ///< For interprocess synchronisation.
    std::mutex m_mutex;
};

