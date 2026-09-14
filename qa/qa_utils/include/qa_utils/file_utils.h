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

#pragma once

#include <filesystem>

#include <wx/string.h>

class PROJECT;
class SETTINGS_MANAGER;


namespace KI_TEST
{

/*
 * A uniquely-named temporary directory removed on destruction.
 */
class SCOPED_TEMP_DIR
{
public:
    SCOPED_TEMP_DIR( const wxString& aPrefix );

    ~SCOPED_TEMP_DIR();

    /// Get the path to the temporary directory as a std::filesystem::path.
    const std::filesystem::path& Path() const { return m_path; }

    /// Get the path to the temporary directory as a wxString.
    wxString PathStr() const { return wxString::FromUTF8( m_path.string() ); }

private:
    std::filesystem::path m_path;
};


/*
 * A project loaded from its own temporary directory.
 *
 * The project is unloaded before the directory is removed so its lock file is released while the
 * file still exists.  Declare it ahead of any SCHEMATIC that refers to the project.
 */
class SCOPED_TEMP_PROJECT
{
public:
    SCOPED_TEMP_PROJECT( SETTINGS_MANAGER& aManager, const wxString& aPrefix, const wxString& aName );

    ~SCOPED_TEMP_PROJECT();

    SCOPED_TEMP_PROJECT( const SCOPED_TEMP_PROJECT& ) = delete;
    SCOPED_TEMP_PROJECT& operator=( const SCOPED_TEMP_PROJECT& ) = delete;

    PROJECT& Project() const { return *m_project; }

    /// Get the path to the temporary directory as a wxString.
    wxString DirStr() const { return m_dir.PathStr(); }

private:
    SCOPED_TEMP_DIR   m_dir;
    SETTINGS_MANAGER& m_manager;
    PROJECT*          m_project;
};

} // namespace KI_TEST
