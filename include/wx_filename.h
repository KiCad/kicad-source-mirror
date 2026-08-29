/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WX_FILENAME_H
#define WX_FILENAME_H

#include <kicommon.h>
#include <wx/filename.h>

/**
 * Default flags to pass to wxFileName::Normalize().
 *
 * @note wxPATH_NORM_ALL is deprecated in wxWidgets 3.1 and later.  wxPATH_NORM_ENV_VARS
 *       is not included because it has some known issues and we typically use our own
 *       ExpandEnvVarSubstitutions() for handling environment variable expansion.  If
 *       ExpandEnvVarSubstitutions() is not used, logically or wxPATH_NORM_ENV_VARS to
 *       this.
 */
#define FN_NORMALIZE_FLAGS ( wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_ABSOLUTE | \
                             wxPATH_NORM_LONG | wxPATH_NORM_SHORTCUT )

/**
 * A wrapper around a wxFileName which is much more performant with a subset of the API.
 *
 * A wrapper around a wxFileName which avoids expensive calls to wxFileName::SplitPath()
 * and string concatenations by caching the path and filename locally and only resolving
 * the wxFileName when it has to.
 */
class KICOMMON_API WX_FILENAME
{
public:
    WX_FILENAME( const wxString& aPath, const wxString& aFilename );

    void SetFullName( const wxString& aFileNameAndExtension );
    void SetPath( const wxString& aPath );

    wxString GetName() const;
    wxString GetFullName() const;
    wxString GetPath() const;
    wxString GetFullPath() const;

    // Avoid multiple calls to stat() on POSIX kernels.
    long long GetTimestamp();

    // Resolve possible symlink(s) to absolute path
    static void ResolvePossibleSymlinks( wxFileName& aFilename );

    /**
     * Split an untrusted archive entry name into its path components.
     *
     * Rejects rather than sanitizes: an absolute or volume qualified name, or a ".."
     * component anywhere, fails.
     *
     * @param aEntryName is the raw, untrusted name read from the archive.
     * @param aParts is filled with the path components on success.
     * @return true if the name is safe to append to an extraction directory.
     */
    static bool SplitArchiveEntryName( const wxString& aEntryName, wxArrayString& aParts );

    /**
     * Resolve an untrusted archive entry name against the directory it is extracted into.
     * 
     * If this function returns false, don't attempt to extract the given entry.
     * Use the aResult parameter for the final destination to extract to otherwise.
     *
     * @param aDestDir is the directory the archive is being extracted into.
     * @param aEntryName is the raw, untrusted name read from the archive.
     * @param aResult is set to the full path to write to when the call succeeds.
     *
     * @return true if the entry resolves to a path inside @a aDestDir.
     */
    static bool ResolveArchiveEntryPath( const wxString& aDestDir, const wxString& aEntryName,
                                         wxFileName& aResult );

private:
    // Write cached values to the wrapped wxFileName.  MUST be called before using m_fn.
    void resolve();

    wxFileName m_fn;
    wxString   m_path;
    wxString   m_fullName;
};

#endif // WX_FILENAME_H
