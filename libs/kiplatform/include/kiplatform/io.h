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

#ifndef KIPLATFORM_IO_H_
#define KIPLATFORM_IO_H_

#include <stdio.h>
#include <cstddef>

class wxString;
class wxFileName;

namespace KIPLATFORM
{
namespace IO
{
    /**
     * Buffer size for file I/O operations on cloud-synced folders.
     *
     * Cloud sync services like Google Drive, OneDrive, and Dropbox can report stale file sizes
     * during seek operations immediately after writing. Using a 512KB buffer reduces the number
     * of I/O operations and allows the cloud sync driver to flush data more reliably before
     * subsequent reads. This value was determined empirically to eliminate sync issues.
     */
    static constexpr size_t CLOUD_SYNC_BUFFER_SIZE = 512 * 1024;
    /**
     * Opens the file like fopen but sets flags (if available) for sequential read hinting.
     * Only use this variant of fopen if the file is truely going to be read sequentially only
     * otherwise you may encounter performance penalities.
     *
     * Windows in particular is a little ulgy to set the sequential scan flag compared
     * to say linux and it's posix_fadvise
     */
    FILE* SeqFOpen( const wxString& aPath, const wxString& mode );

    /**
     * Duplicates the file security data from one file to another ensuring that they are
     * the same between both.  This assumes that the user has permission to set #aDest
     * @return true if the process was successful
     */
    bool DuplicatePermissions( const wxString& aSrc, const wxString& aDest );

    /**
     * Ensures that a file has write permissions.
     * This is useful after copying files that may have been read-only.
     * @param aFilePath path to the file to make writeable
     * @return true if the process was successful
     */
    bool MakeWriteable( const wxString& aFilePath );

    /**
    * Helper function to determine the status of the 'Hidden' file attribute.
    * @return true if the file attribut is set.
    */
    bool IsFileHidden( const wxString& aFileName );

    /**
     * Adjusts a filename to be a long path compatible.
     * This is a no-op on non-Windows platforms.
     */
    void LongPathAdjustment( wxFileName& aFilename );

    /**
     * Computes a hash of modification times and sizes for files matching a pattern.
     * Used for cache invalidation by detecting changes to library directories.
     *
     * @param aDirPath  Directory to search
     * @param aFilespec Wildcard pattern to match (e.g. "*.kicad_mod")
     * @return Hash value that changes when any matching file is modified
     */
    long long TimestampDir( const wxString& aDirPath, const wxString& aFilespec );
} // namespace IO
} // namespace KIPLATFORM

#endif // KIPLATFORM_IO_H_
