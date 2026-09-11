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

} // namespace KI_TEST
