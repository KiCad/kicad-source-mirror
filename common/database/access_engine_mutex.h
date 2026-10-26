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

#ifndef KICAD_ACCESS_ENGINE_MUTEX_H
#define KICAD_ACCESS_ENGINE_MUTEX_H

#include <chrono>
#include <mutex>
#include <string>


/**
 * The one lock every user of the Microsoft Access engine takes while the engine initializes or unloads.  The ODBC
 * driver and the ACE OLE DB provider load the same Office libraries, and those race when two threads start or
 * stop the engine at once.
 *
 * The lock is defined once, in access_engine_mutex.cpp, and never destroyed so a worker thread still waiting on it
 * at process exit finds it alive.  An abandoned worker can hold it without bound, so no caller waits on it
 * without a deadline.
 */
std::timed_mutex& AccessEngineMutex();

/// @return the engine lock taken before aDeadline, or an unowned lock if the deadline passed first
std::unique_lock<std::timed_mutex> LockAccessEngine( std::chrono::steady_clock::time_point aDeadline );

/// @return the text shown when the engine lock could not be taken in time
std::string AccessEngineBusyMessage();

#endif // KICAD_ACCESS_ENGINE_MUTEX_H
