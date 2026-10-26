/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "access_engine_mutex.h"

#include <wx/intl.h>
#include <wx/string.h>


std::timed_mutex& AccessEngineMutex()
{
    static auto* s_mutex = new std::timed_mutex;

    return *s_mutex;
}


std::unique_lock<std::timed_mutex> LockAccessEngine( std::chrono::steady_clock::time_point aDeadline )
{
    return std::unique_lock<std::timed_mutex>( AccessEngineMutex(), aDeadline );
}


std::string AccessEngineBusyMessage()
{
    return _( "The Access database engine is busy with another library" ).utf8_string();
}
