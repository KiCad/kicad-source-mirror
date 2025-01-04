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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PROJECT_ARCHIVER_H
#define KICAD_PROJECT_ARCHIVER_H

#include <wx/string.h>
#include <kicommon.h>


class PROJECT;
class REPORTER;
class SETTINGS_MANAGER;


class KICOMMON_API PROJECT_ARCHIVER
{
public:
    PROJECT_ARCHIVER();

    ~PROJECT_ARCHIVER() = default;

    /**
     * Compare the CRCs of all the files in zip archive to determine whether the archives are
     * identical.
     *
     * @param aZipFileA is the full path to the first zip.
     * @param aZipFileB is the full path to the second zip.
     * @param aReporter is used to report status.
     * @return true if the archives are identical.
     */
    static bool AreZipArchivesIdentical( const wxString& aZipFileA, const wxString& aZipFileB,
                                         REPORTER& aReporter );

    /**
     * Create an archive of the project.
     *
     * @param aSrcFile is the full path to the project to be archived.
     * @param aDestFile is the full path to the zip file to be created.
     * @param aReporter is used to report status.
     * @param aVerbose controls the verbosity of reported status messages.
     * @param aIncludeExtraFiles if true will archive legacy and output files.
     * @return true if the archive was created successfully.
     */
    static bool Archive( const wxString& aSrcDir, const wxString& aDestFile, REPORTER& aReporter,
                         bool aVerbose = true, bool aIncludeExtraFiles = false );

    /**
     * Extract an archive of the current project over existing files.
     *
     * @warning This will overwrite files in the project directory.  Use with care.  The caller is
     * responsible for doing any reloading of state after taking this action.
     *
     * @param aSrcFile is the full path to the archive to extract.
     * @param aDestDir is the target directory to unarchive to.
     * @param aReporter is used to report status.
     * @return true if the archive was created successfully.
     */
    static bool Unarchive( const wxString& aSrcFile, const wxString& aDestDir,
                           REPORTER& aReporter );
};

#endif // KICAD_PROJECT_ARCHIVER_H
