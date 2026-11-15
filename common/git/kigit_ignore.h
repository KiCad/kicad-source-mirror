/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KIGIT_IGNORE_H_
#define KIGIT_IGNORE_H_

#include <import_export.h>

#include <vector>

#include <wx/filename.h>
#include <wx/string.h>

class KIGIT_COMMON;

namespace KIGIT
{

/// What an ignore rule for one path would achieve.
struct IGNORE_STATE
{
    /// Git's rules already cover the path.
    bool ignored = false;

    /// The path, or a file inside it, is in the index, so a new rule would do nothing.
    bool tracked = false;
};


/// The ignore file that covers a project. It sits beside the project rather than at the
/// repository root, so a project inside a larger repository ignores only its own files.
APIEXPORT wxFileName ProjectIgnoreFile( const wxString& aProjectPath );

/**
 * The rule that ignores one path and nothing else.
 *
 * @param aProjectRelPath Path relative to the project folder, with forward slashes.
 * @param aIsDirectory Restricts the rule to a directory of that name.
 */
APIEXPORT wxString PathIgnoreRule( const wxString& aProjectRelPath, bool aIsDirectory );

/// The rule that ignores every file sharing a path's extension, or nothing when it has none.
APIEXPORT wxString ExtensionIgnoreRule( const wxString& aProjectRelPath );

/// The rules an ignore file holds, in order, without its comments and blank lines.
APIEXPORT std::vector<wxString> ReadIgnoreRules( const wxFileName& aIgnoreFile );

/// Append rules to an ignore file, creating it with aHeader as a comment when it is not there.
/// A rule the file already holds is not repeated. False leaves the reason in aError.
APIEXPORT bool AddIgnoreRules( const wxFileName& aIgnoreFile, const std::vector<wxString>& aRules,
                               const wxString& aHeader, wxString& aError );

/// Remove rules from an ignore file. Only a line holding the rule itself goes, because a rule
/// covering more than the path picked would stop ignoring everything else it covers.
APIEXPORT bool RemoveIgnoreRules( const wxFileName& aIgnoreFile, const std::vector<wxString>& aRules,
                                  wxString& aError );

/**
 * What git's rules and its index say about a project path, so the user is offered only what
 * would take effect. The caller holds the session's action lock.
 *
 * @param aProjectRelPath Path relative to the project folder, with forward slashes.
 * @param aIsDirectory Whether the path names a directory.
 */
APIEXPORT IGNORE_STATE IgnoreState( KIGIT_COMMON& aCommon, const wxString& aProjectRelPath, bool aIsDirectory );

} // namespace KIGIT

#endif // KIGIT_IGNORE_H_
