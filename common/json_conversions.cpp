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

#include <json_common.h>
#include <json_conversions.h>

// Specializations to allow directly reading/writing wxStrings from JSON
void to_json( nlohmann::json& aJson, const wxString& aString )
{
    aJson = aString.ToUTF8();
}


void from_json( const nlohmann::json& aJson, wxString& aString )
{
    aString = wxString( aJson.get<std::string>().c_str(), wxConvUTF8 );
}
