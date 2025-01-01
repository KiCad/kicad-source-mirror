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

#ifndef KICAD_ASSET_ARCHIVE_H
#define KICAD_ASSET_ARCHIVE_H

#include <kicommon.h>
#include <unordered_map>
#include <vector>

#include <core/wx_stl_compat.h>

/**
 * An asset archive represents a file containing data assets that are loaded from disk and then
 * cached in memory.  For example, a set of bitmap images.
 *
 * The entire contents of the archive will be uncompressed and kept resident in memory in the
 * current implementation, so consider this before reusing this as-is for new use cases.
 */
class KICOMMON_API ASSET_ARCHIVE
{
public:
    ASSET_ARCHIVE( const wxString& aFilePath, bool aLoadNow = true );

    ~ASSET_ARCHIVE() = default;

    bool Load();

    /**
     * Retrieves a file with the given path from the cached archive
     * @param aFilePath is the path within the archive to the requested file
     * @param aDest is the target byte array to copy into
     * @param aMaxLen is the maximum bytes that can be copied into aDest
     * @return the number of bytes copied, or -1 if the given file was not found
     */
    long GetFileContents( const wxString& aFilePath, const unsigned char* aDest, size_t aMaxLen );

    /**
     * Retrieves a pointer to a file with the given path from the cached archive
     * @param aFilePath is the path within the archive to the requested file
     * @param aDest will be set to point to the start of the file if the file was found
     * @return the file size, or -1 if the given file was not found
     */
    long GetFilePointer( const wxString& aFilePath, const unsigned char** aDest );

private:
    struct FILE_INFO
    {
        size_t offset;
        size_t length;
    };

    /// Cache of file info for a given file path
    std::unordered_map<wxString, FILE_INFO> m_fileInfoCache;

    /// The full file contents
    std::vector<unsigned char> m_cache;

    /// Path to the source archive file
    wxString m_filePath;
};

#endif // KICAD_ASSET_ARCHIVE_H
