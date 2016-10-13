/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#ifndef NGSPICE_H
#define NGSPICE_H

#include "spice_simulator.h"

#include <ngspice/sharedspice.h>

class wxDynamicLibrary;

class NGSPICE : public SPICE_SIMULATOR {

public:
    NGSPICE();
    virtual ~NGSPICE();

    ///> @copydoc SPICE_SIMULATOR::Init()
    void Init() override;

    ///> @copydoc SPICE_SIMULATOR::LoadNetlist()
    bool LoadNetlist( const std::string& aNetlist ) override;

    ///> @copydoc SPICE_SIMULATOR::Run()
    bool Run() override;

    ///> @copydoc SPICE_SIMULATOR::Stop()
    bool Stop() override;

    ///> @copydoc SPICE_SIMULATOR::IsRunning()
    bool IsRunning() override;

    ///> @copydoc SPICE_SIMULATOR::Command()
    bool Command( const std::string& aCmd ) override;

    ///> @copydoc SPICE_SIMULATOR::GetXAxis()
    std::string GetXAxis( SIM_TYPE aType ) const override;

    ///> @copydoc SPICE_SIMULATOR::GetPlot()
    std::vector<COMPLEX> GetPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///> @copydoc SPICE_SIMULATOR::GetRealPlot()
    std::vector<double> GetRealPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///> @copydoc SPICE_SIMULATOR::GetImagPlot()
    std::vector<double> GetImagPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///> @copydoc SPICE_SIMULATOR::GetMagPlot()
    std::vector<double> GetMagPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///> @copydoc SPICE_SIMULATOR::GetPhasePlot()
    std::vector<double> GetPhasePlot( const std::string& aName, int aMaxLen = -1 ) override;

private:
    void init();

    ///> Executes commands from a file
    bool loadSpinit( const std::string& aFileName );

    ///> Checks a few different locations for codemodel files and returns one
    ///> if it exists
    std::string findCmPath() const;

    ///> Loads codemodel files from a directory
    bool loadCodemodels( const std::string& aPath );

    // Callback functions
    static int cbSendChar( char* what, int id, void* user );
    static int cbSendStat( char* what, int id, void* user );
    static int cbBGThreadRunning( bool is_running, int id, void* user );
    static int cbControlledExit( int status, bool immediate, bool exit_upon_quit, int id, void* user );

    void dump();

    ///> NGspice should be initialized only once
    static bool m_initialized;
};

#endif /* NGSPICE_H */
