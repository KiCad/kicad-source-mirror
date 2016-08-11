/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_SIMULATOR_H
#define SPICE_SIMULATOR_H

#include <string>
#include <vector>

class REPORTER;

enum SIM_TRACE_TYPE
{
    SIM_AC_MAG = 0x1,
    SIM_AC_PHASE = 0x2,
    SIM_TR_VOLTAGE = 0x4,
    SIM_TR_CURRENT = 0x8,
    SIM_TR_FFT = 0x10
};

class SPICE_SIMULATOR {

public:
    SPICE_SIMULATOR() : m_consoleReporter( NULL ) {}
    virtual ~SPICE_SIMULATOR() {}

    static SPICE_SIMULATOR* CreateInstance( const std::string& aName );

    virtual void Init() = 0;
    virtual bool LoadNetlist( const std::string& aNetlist ) = 0;
    virtual bool Run() = 0;
    virtual bool Command( const std::string& aCmd ) = 0;

    virtual void SetConsoleReporter( REPORTER* aReporter )
    {
        m_consoleReporter = aReporter;
    }

    virtual const std::vector<double> GetPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

protected:
    REPORTER* m_consoleReporter;
};

#endif /* SPICE_SIMULATOR_H */
