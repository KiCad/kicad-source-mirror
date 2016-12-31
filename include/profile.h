/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file profile.h:
 * @brief Simple profiling functions for measuring code execution time.
 */

#ifndef __TPROFILE_H
#define __TPROFILE_H

#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>

class PROF_COUNTER
{
public:
    PROF_COUNTER( const std::string& name, bool autostart = true ) :
        m_name( name ),
        m_running( false )
    {
        if( autostart )
            start();
    }

    void start()
    {
        m_running = true;
        starttime = std::chrono::system_clock::now();
    }

    void stop()
    {
        if( !m_running )
            return;

        stoptime = std::chrono::system_clock::now();
    }

    void show()
    {
        time_point display_stoptime;

        if( m_running )
            display_stoptime = std::chrono::system_clock::now();
        else
            display_stoptime = stoptime;

        std::chrono::duration<double, std::milli> d = display_stoptime - starttime;
        std::cerr << m_name << " took " << std::setprecision(1) << d.count() << "milliseconds." << std::endl;
    }

    double msecs() const
    {
        std::chrono::duration<double, std::milli> d = stoptime - starttime;
        return d.count();
    }

private:
    std::string m_name;
    bool m_running;

    typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

    time_point starttime, stoptime;
};


#endif
