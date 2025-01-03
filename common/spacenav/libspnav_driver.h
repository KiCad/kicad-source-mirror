/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_LIBSPNAV_DRIVER_H
#define KICAD_LIBSPNAV_DRIVER_H

#ifdef __linux__
#include <spnav.h>
#endif

#include <kicommon.h>
#include <mutex>
#include "spacenav_driver.h"

// Implementation of the SPACENAV_DRIVER using libspnav (spacenavd)
class KICOMMON_API LIBSPNAV_DRIVER : public SPACENAV_DRIVER
{
public:
    LIBSPNAV_DRIVER();
    ~LIBSPNAV_DRIVER();

    bool Connect() override;
    void Disconnect() override;
    void Poll() override;

private:
    // Shared connection management
    static std::mutex s_mutex;
    static int s_connection_count;
    static bool s_spnav_connected;

    bool m_client_connected = false;
};

#endif // KICAD_LIBSPNAV_DRIVER_H