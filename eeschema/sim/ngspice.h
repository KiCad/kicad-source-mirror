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

#ifndef NGSPICE_H
#define NGSPICE_H

#include "sharedspice.h"
#include "spice_simulator.h"

class wxDynamicLibrary;

class NGSPICE : public SPICE_SIMULATOR {

public:
    NGSPICE();
    virtual ~NGSPICE();

    void Init();
    bool LoadNetlist( const std::string& aNetlist );
    bool Run();
    bool Command( const std::string& aCmd );

    std::string GetConsole() const;

    const std::vector<double> GetPlot( const std::string& aName, int aMaxLen = -1 );

    void dump();

private:
    typedef void (*ngSpice_Init)( SendChar*, SendStat*, ControlledExit*,
                                    SendData*, SendInitData*, BGThreadRunning*, void* );

    // ngspice library functions
    typedef int (*ngSpice_Circ)(char** circarray);
    typedef int (*ngSpice_Command)(char* command);
    typedef pvector_info (*ngGet_Vec_Info)(char* vecname);
    typedef char** (*ngSpice_AllVecs)(char* plotname);
    typedef char** (*ngSpice_AllPlots)(void);

    ngSpice_Init m_ngSpice_Init;
    ngSpice_Circ m_ngSpice_Circ;
    ngSpice_Command m_ngSpice_Command;
    ngGet_Vec_Info m_ngGet_Vec_Info;
    ngSpice_AllPlots m_ngSpice_AllPlots;
    ngSpice_AllVecs m_ngSpice_AllVecs;

    wxDynamicLibrary* m_dll;

    static int cbSendChar( char* what, int id, void* user );
    static int cbSendStat( char* what, int id, void* user );
};

#endif /* NGSPICE_H */
