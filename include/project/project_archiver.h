/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PROJECT_ARCHIVER_H
#define KICAD_PROJECT_ARCHIVER_H

#include <wx/string.h>


class PROJECT;
class REPORTER;
class SETTINGS_MANAGER;


class PROJECT_ARCHIVER
{
public:
    PROJECT_ARCHIVER();

    ~PROJECT_ARCHIVER() = default;

    /**
     * Creates an archive of the project
     * @param aSrcFile is the full path to the project to be archived
     * @param aDestFile is the full path to the zip file to be created
     * @param aReporter is used to report status
     * @param aVerbose controls the verbosity of reported status messages
     * @param aIncludeExtraFiles if true will archive legacy and output files
     * @return true if the archive was created successfully
     */
    bool Archive( const wxString& aSrcDir, const wxString& aDestFile, REPORTER& aReporter,
                  bool aVerbose = true, bool aIncludeExtraFiles = false );

    /**
     * Extracts an archive of the current project over existing files
     * Warning: this will overwrite files in the project directory.  Use with care.  The caller is
     * responsible for doing any reloading of state after taking this action.
     * @param aSrcFile is the full path to the archive to extract
     * @param aDestDir is the target directory to unarchive to
     * @param aReporter is used to report status
     * @return true if the archive was created successfully
     */
    bool Unarchive( const wxString& aSrcFile, const wxString& aDestDir, REPORTER& aReporter );
};

#endif // KICAD_PROJECT_ARCHIVER_H
