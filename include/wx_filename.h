/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_FILENAME_H
#define WX_FILENAME_H

#include <wx/filename.h>

/**
 * A wrapper around a wxFileName which is much more performant with a subset of the API.
 *
 * A wrapper around a wxFileName which avoids expensive calls to wxFileName::SplitPath()
 * and string concatenations by caching the path and filename locally and only resolving
 * the wxFileName when it has to.
 */
class WX_FILENAME
{
public:
    WX_FILENAME( const wxString& aPath, const wxString& aFilename );

    void SetFullName( const wxString& aFileNameAndExtension );

    wxString GetName() const;
    wxString GetFullName() const;
    wxString GetPath() const;
    wxString GetFullPath() const;

    // Avoid multiple calls to stat() on POSIX kernels.
    long long GetTimestamp();

    // Resolve possible symlink(s) to absolute path
    static void ResolvePossibleSymlinks( wxFileName& aFilename );

private:
    // Write cached values to the wrapped wxFileName.  MUST be called before using m_fn.
    void resolve();

    wxFileName m_fn;
    wxString   m_path;
    wxString   m_fullName;
};

#endif // WX_FILENAME_H
