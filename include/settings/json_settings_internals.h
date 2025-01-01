/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_JSON_SETTINGS_INTERNALS_H
#define KICAD_JSON_SETTINGS_INTERNALS_H

#include <json_common.h>

class KICOMMON_API JSON_SETTINGS_INTERNALS : public nlohmann::json
{
    friend class JSON_SETTINGS;

public:
    JSON_SETTINGS_INTERNALS() :
            nlohmann::json()
    {}

    /**
     * Builds a JSON pointer based on a given string
     * @param aPath is the path in the form "key1.key2.key3"
     * @return a JSON pointer that can be used to index into a JSON object
     */
    static nlohmann::json::json_pointer PointerFromString( std::string aPath );

    template<typename ValueType>
    void SetFromString( const std::string& aPath, ValueType aVal )
    {
        // Calls the overload below, which will convert from dotted string to JSON pointer
        ( *this )[aPath] = std::move( aVal );
    }

    template<typename ValueType>
    ValueType Get( const std::string& aPath ) const
    {
        return at( PointerFromString( aPath ) ).get<ValueType>();
    }

    nlohmann::json& At( const std::string& aPath )
    {
        return at( PointerFromString( aPath ) );
    }

    nlohmann::json& operator[]( const nlohmann::json::json_pointer& aPointer )
    {
        return nlohmann::json::operator[]( aPointer );
    }

    nlohmann::json& operator[]( const std::string& aPath )
    {
        return nlohmann::json::operator[]( PointerFromString( aPath ) );
    }

    void CloneFrom( const JSON_SETTINGS_INTERNALS& aOther )
    {
        nlohmann::json::json_pointer root( "" );
        this->nlohmann::json::operator[]( root ) = aOther.nlohmann::json::operator[]( root );
    }

private:

    nlohmann::json m_original;
};

#endif // KICAD_JSON_SETTINGS_INTERNALS_H
