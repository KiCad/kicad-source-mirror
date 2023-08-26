/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PLUGIN_FILE_DESC_H_
#define PLUGIN_FILE_DESC_H_

#include <vector>
#include <string>
#include <wx/string.h>

/**
* Container that describes file type info
*/
struct PLUGIN_FILE_DESC
{
    wxString                 m_Description;    ///< Description shown in the file picker dialog
    std::vector<std::string> m_FileExtensions; ///< Filter used for file pickers if m_IsFile is true
    std::vector<std::string> m_ExtensionsInDir; ///< In case of folders: extensions of files inside
    bool                     m_IsFile;          ///< Whether the library is a folder or a file

    PLUGIN_FILE_DESC( const wxString& aDescription, const std::vector<std::string>& aFileExtensions,
                      const std::vector<std::string>& aExtsInFolder = {}, bool aIsFile = true ) :
            m_Description( aDescription ),
            m_FileExtensions( aFileExtensions ), m_ExtensionsInDir( aExtsInFolder ),
            m_IsFile( aIsFile )
    {
    }

    PLUGIN_FILE_DESC() : PLUGIN_FILE_DESC( wxEmptyString, {} ) {}

    /**
     * @return translated description + wildcards string for file dialogs.
     */
    wxString FileFilter() const;

    operator bool() const { return !m_Description.empty(); }
};

#endif // PLUGIN_FILE_DESC_H_