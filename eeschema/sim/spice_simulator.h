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

#ifndef SPICE_SIMULATOR_H
#define SPICE_SIMULATOR_H

#include "sim_types.h"

#include <string>
#include <vector>
#include <complex>

class SPICE_REPORTER;

typedef std::complex<double> COMPLEX;

class SPICE_SIMULATOR
{
public:
    SPICE_SIMULATOR() : m_reporter( NULL ) {}
    virtual ~SPICE_SIMULATOR() {}

    static SPICE_SIMULATOR* CreateInstance( const std::string& aName );

    virtual void Init() = 0;
    virtual bool LoadNetlist( const std::string& aNetlist ) = 0;
    virtual bool Run() = 0;
    virtual bool Stop() = 0;
    virtual bool IsRunning() = 0;
    virtual bool Command( const std::string& aCmd ) = 0;

    ///> Returns X axis name for a given simulation type
    virtual std::string GetXAxis( SIM_TYPE aType ) const = 0;

    virtual void SetReporter( SPICE_REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    virtual std::vector<COMPLEX> GetPlot( const std::string& aName, int aMaxLen = -1 ) = 0;
    virtual std::vector<double> GetRealPlot( const std::string& aName, int aMaxLen = -1 ) = 0;
    virtual std::vector<double> GetImagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;
    virtual std::vector<double> GetMagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;
    virtual std::vector<double> GetPhasePlot( const std::string& aName, int aMaxLen = -1 ) = 0;


protected:
    SPICE_REPORTER* m_reporter;
};

#endif /* SPICE_SIMULATOR_H */
