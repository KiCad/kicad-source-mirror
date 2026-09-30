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

#pragma once

#include <wx/string.h>

class BOARD;
class JOB_EXPORT_PCB_ODB;
class PROGRESS_REPORTER;
class REPORTER;
class wxFileName;


/// The directory or archive file the job writes, with symlinks resolved
wxFileName ResolveOdbOutputPath( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard );

/// Generate the ODB++ tree or archive, replacing an existing archive or directory product
/// @return true if the output was written
bool GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter = nullptr,
                         REPORTER* aReporter = nullptr );
