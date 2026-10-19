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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GIT_EDITOR_SNAPSHOT_H
#define GIT_EDITOR_SNAPSHOT_H

#include <map>

#include <wx/string.h>

#include <frame_type.h>

/// On-disk identity of one file, used to tell whether a git operation rewrote it.
struct FILE_STAMP
{
    long long modified = 0; ///< Modification time, nanoseconds where the platform provides them.
    long long size = 0;

    bool operator==( const FILE_STAMP& aOther ) const { return modified == aOther.modified && size == aOther.size; }

    bool operator!=( const FILE_STAMP& aOther ) const { return !( *this == aOther ); }
};

/**
 * The files making up one open document, each with its on-disk identity. An empty map means
 * the document had no files on disk.
 */
struct DOCUMENT_SNAPSHOT
{
    std::map<wxString, FILE_STAMP> files;
};

/**
 * What the open editors and the project settings looked like on disk before an operation
 * that may rewrite them. A default constructed snapshot holds no entries, so every file found
 * afterwards counts as changed.
 */
struct EDITOR_SNAPSHOT
{
    std::map<FRAME_T, DOCUMENT_SNAPSHOT> editors;
    DOCUMENT_SNAPSHOT                    settings;
};

#endif // GIT_EDITOR_SNAPSHOT_H
