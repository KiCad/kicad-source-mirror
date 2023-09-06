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

#ifndef JSON_SERIALIZERS_H_
#define JSON_SERIALIZERS_H_

#include <nlohmann/json.hpp>
#include <wx/string.h>
#include <optional>


namespace nlohmann
{
template <>
struct adl_serializer<wxString>
{
    static void from_json( const json& j, wxString& s )
    {
        s = wxString::FromUTF8( j.get<std::string>().c_str() );
    }

    static void to_json( json& j, const wxString& s ) { j = s.ToUTF8(); }
};

template <typename T>
struct adl_serializer<std::optional<T>>
{
    static void from_json( const json& j, std::optional<T>& opt )
    {
        if( j.is_null() )
        {
            opt = std::nullopt;
        }
        else
        {
            opt = j.template get<T>();
        }
    }

    static void to_json( json& j, const std::optional<T>& opt )
    {
        if( opt.has_value() )
        {
            j = *opt;
        }
        else
        {
            j = nullptr;
        }
    }
};
} // namespace nlohmann


#endif // JSON_SERIALIZERS_H_