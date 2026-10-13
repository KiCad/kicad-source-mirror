/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @file env_paths.h
 * @brief Helper functions to substitute paths with environmental variables.
 */

#ifndef ENV_PATHS_H
#define ENV_PATHS_H

#include <wx/string.h>
#include <settings/environment.h>
#include <kicommon.h>

class wxFileName;
class PROJECT;

/**
 * Normalize a file path to an environmental variable, if possible.
 *
 * @param aFilePath is the full file path (path and file name) to be normalized.
 * @param aEnvVars is an optional map of environmental variables to try substitution with.
 * @param aProject is an optional project, to normalize the file path to the project path.
 * @return Normalized full file path (path and file name) if succeeded or the input path if
 *         the path could not be normalized.
 */
wxString KICOMMON_API NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const PROJECT* aProject );

/**
 * Normalize a file path to an environmental variable, if possible.
 *
 * @param aFilePath is the full file path (path and file name) to be normalized.
 * @param aEnvVars is an optional map of environmental variables to try substitution with.
 * @param aProjectPath is an optional string to normalize the file path to the project path.
 * @return Normalized full file path (path and file name) if succeeded or the input path if
 *         the path could not be normalized.
 */
wxString KICOMMON_API NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const wxString& aProjectPath );

/**
 * Express an absolute path relative to a base directory, stepping out with ".." when needed.
 *
 * The result always uses '/' separators so a path stored in a shared file works on every
 * platform.
 *
 * @param aPath is the absolute file or directory path to convert.
 * @param aBaseDir is the absolute directory the result is relative to.
 * @return the relative path, "." when \a aPath is \a aBaseDir itself, or \a aPath unchanged
 *         when it has no relative form (relative input, empty base or a different volume).
 */
wxString KICOMMON_API MakeRelativePath( const wxString& aPath, const wxString& aBaseDir );

/**
 * Choose how to store a path the user picked for a project.
 *
 * A path inside the project is always made relative to it. Otherwise an environment variable
 * substitution is preferred, then a path climbing out of the project with ".." when
 * \a aAllowParentDirs is set, and finally the absolute path.
 *
 * @param aPath is the absolute file or directory path to store.
 * @param aEnvVars is an optional map of environment variables to try substitution with.
 * @param aProjectPath is the absolute project directory, or empty when there is no project.
 * @param aAllowParentDirs permits a relative path that leaves the project directory.
 */
wxString KICOMMON_API NormalizePathForProject( const wxString& aPath, const ENV_VAR_MAP* aEnvVars,
                                               const wxString& aProjectPath, bool aAllowParentDirs );

/**
 * Search the default paths trying to find one with the requested file.
 *
 * @param aFileName is the name of the searched file. It might be a relative path.
 * @param aEnvVars is an optional map of environmental variables that can contain paths.
 * @param aProject is an optional project, to check the project path.
 * @return Full path (path and file name) if the file was found in one of the paths, otherwise
 *         an empty string.
 */
wxString KICOMMON_API ResolveFile( const wxString& aFileName, const ENV_VAR_MAP* aEnvVars,
                      const PROJECT* aProject );

#endif /* ENV_PATHS_H */
