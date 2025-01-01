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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STRING_ANY_MAP_H_
#define STRING_ANY_MAP_H_

#include <string>
#include <map>
#include <optional>

#include <wx/any.h>


/**
 * A name/value tuple with unique names and wxAny values.  The names
 * may be iterated alphabetically.
 */
class STRING_ANY_MAP : public std::map<std::string, wxAny>
{
    double m_iuScale;

public:

    STRING_ANY_MAP( double aIUScale = 1.0 ) : m_iuScale( aIUScale ) {}

    template <typename T>
    bool get_to( const std::string& aKey, T& aVar ) const
    {
        if( !contains( aKey ) )
            return false;

        return at( aKey ).GetAs( &aVar );
    }

    template <typename T>
    bool get_to_iu( const std::string& aKey, T& aVar ) const
    {
        if( !contains( aKey ) )
            return false;

        const wxAny& value = at( aKey );

        if( value.CheckType<double>() || value.CheckType<int>() || value.CheckType<long>()
            || value.CheckType<long long>() )
        {
            double number;

            if( !value.GetAs( &number ) )
                return false;

            number *= m_iuScale;
            aVar = number;
        }
        else
        {
            if( !value.GetAs( &aVar ) )
                return false;
        }

        return true;
    }

    template <typename T>
    void set( const std::string& aKey, const T& aVar )
    {
        emplace( aKey, aVar );
    }

    template <typename T>
    void set_iu( const std::string& aKey, const T& aVar)
    {
        emplace( aKey, aVar / m_iuScale );
    }

    bool contains( const std::string& aKey ) const
    { //
        return find( aKey ) != end();
    }

    template <typename T>
    std::optional<T> get_opt( const std::string& aKey ) const
    {
        if( contains( aKey ) )
        {
            T val;

            if( !at( aKey ).GetAs( &val ) )
                return std::nullopt;

            return val;
        }

        return std::nullopt;
    }
};


#endif // STRING_ANY_MAP_H_