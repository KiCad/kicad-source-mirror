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

#ifndef KICAD_ACCESS_FAST_EXIT_H
#define KICAD_ACCESS_FAST_EXIT_H

#include <kicommon.h>

/**
 * Records that a worker inside the Access engine was abandoned.  The flag is process wide and never cleared.
 */
KICOMMON_API void MarkAccessWorkerAbandoned();

/**
 * Ends the process without the normal teardown when an Access engine worker was abandoned, since the Office
 * libraries it is still inside crash when they unload at exit.  Call it after the application's own shutdown and
 * only from the main thread.  Returns at once if no worker was abandoned.
 *
 * The abandoned session's lock file entry stays behind, and ending the process can wait on redirector I/O.
 *
 * @param aExitCode is the code the process would have exited with
 */
KICOMMON_API void ExitFastIfAccessWorkerAbandoned( int aExitCode );

#endif // KICAD_ACCESS_FAST_EXIT_H
