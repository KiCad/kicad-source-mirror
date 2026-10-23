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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <database/database_value.h>

#include <fmt/format.h>


std::string NormalizeDecimalText( const std::string& aText )
{
    const size_t signLength = ( !aText.empty() && ( aText[0] == '-' || aText[0] == '+' ) ) ? 1 : 0;
    const size_t point = aText.find( '.' );

    if( point == std::string::npos )
        return aText;

    const std::string whole = aText.substr( signLength, point - signLength );
    std::string       fraction = aText.substr( point + 1 );

    if( whole.empty() && fraction.empty() )
        return aText;

    if( whole.find_first_not_of( "0123456789" ) != std::string::npos
        || fraction.find_first_not_of( "0123456789" ) != std::string::npos )
    {
        return aText;
    }

    fraction.erase( fraction.find_last_not_of( '0' ) + 1 );

    if( fraction.empty() && whole.find_first_not_of( '0' ) == std::string::npos )
        return "0";

    std::string result = aText.substr( 0, signLength ) + ( whole.empty() ? "0" : whole );

    if( !fraction.empty() )
        result += "." + fraction;

    return result;
}


std::string DatabaseValueToString( const DB_VALUE& aValue )
{
    struct VISITOR
    {
        std::string operator()( std::monostate ) const { return std::string(); }
        std::string operator()( const std::string& aString ) const { return aString; }
        std::string operator()( int64_t aInt ) const { return std::to_string( aInt ); }
        std::string operator()( bool aBool ) const { return aBool ? "1" : "0"; }

        std::string operator()( double aDouble ) const
        {
            // Negative zero must match the "0" that NormalizeDecimalText gives for "-0.000"
            return aDouble == 0.0 ? std::string( "0" ) : fmt::format( "{}", aDouble );
        }
    };

    return std::visit( VISITOR(), aValue );
}
