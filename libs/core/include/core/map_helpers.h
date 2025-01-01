/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MAP_HELPERS_H_
#define MAP_HELPERS_H_

#include <optional>
#include <map>
#include <wx/string.h>


template <typename V>
inline std::optional<V> get_opt( const std::map<wxString, V>& aMap, const wxString& aKey )
{
    auto it = aMap.find( aKey );

    if( it == aMap.end() )
        return std::nullopt;

    return it->second;
}


template <typename V>
inline std::optional<V> get_opt( const std::map<wxString, V>& aMap, const char* aKey )
{
    return get_opt( aMap, wxString::FromUTF8( aKey ) );
}


template <typename K, typename V>
inline std::optional<V> get_opt( const std::map<K, V>& aMap, const K& aKey )
{
    auto it = aMap.find( aKey );

    if( it == aMap.end() )
        return std::nullopt;

    return it->second;
}


inline wxString get_def( const std::map<wxString, wxString>& aMap, const char* aKey,
                         const char* aDefval = "" )
{
    typename std::map<wxString, wxString>::const_iterator it =
            aMap.find( wxString::FromUTF8( aKey ) );
    if( it == aMap.end() )
    {
        return wxString::FromUTF8( aDefval );
    }
    else
    {
        return it->second;
    }
}


inline wxString get_def( const std::map<wxString, wxString>& aMap, const char* aKey,
                         const wxString& aDefval = wxString() )
{
    typename std::map<wxString, wxString>::const_iterator it =
            aMap.find( wxString::FromUTF8( aKey ) );
    if( it == aMap.end() )
    {
        return aDefval;
    }
    else
    {
        return it->second;
    }
}


inline wxString get_def( const std::map<wxString, wxString>& aMap, const wxString& aKey,
                         const wxString& aDefval = wxString() )
{
    typename std::map<wxString, wxString>::const_iterator it = aMap.find( aKey );
    if( it == aMap.end() )
    {
        return aDefval;
    }
    else
    {
        return it->second;
    }
}


template <typename K, typename V>
inline V get_def( const std::map<K, V>& aMap, const K& aKey, const V& aDefval = V() )
{
    typename std::map<K, V>::const_iterator it = aMap.find( aKey );
    if( it == aMap.end() )
    {
        return aDefval;
    }
    else
    {
        return it->second;
    }
}


#endif // MAP_HELPERS_H_
