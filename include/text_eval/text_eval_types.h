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

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <fmt/format.h>

namespace calc_parser
{
    using VALUE = std::variant<double, std::string>;

    template<typename T>
    class RESULT
    {
    public:
        RESULT( T aValue ) : m_data( std::move( aValue ) ) {}
        RESULT( std::string aError ) : m_data( std::move( aError ) ) {}

        bool HasValue() const { return std::holds_alternative<T>( m_data ); }
        bool HasError() const { return std::holds_alternative<std::string>( m_data ); }

        const T& GetValue() const { return std::get<T>( m_data ); }
        const std::string& GetError() const { return std::get<std::string>( m_data ); }

        explicit operator bool() const { return HasValue(); }

    private:
        std::variant<T, std::string> m_data;
    };

    template<typename T>
    RESULT<T> MakeError( std::string aMsg )
    {
        return RESULT<T>( std::move( aMsg ) );
    }

    template<typename T>
    RESULT<T> MakeValue( T aVal )
    {
        return RESULT<T>( std::move( aVal ) );
    }

    class ERROR_COLLECTOR
    {
    public:
        void AddError( std::string aError )
        {
            m_errors.emplace_back( std::move( aError ) );
        }

        void AddWarning( std::string aWarning )
        {
            m_warnings.emplace_back( std::move( aWarning ) );
        }

        void AddSyntaxError( int aLine = -1, int aColumn = -1 )
        {
            if( aLine >= 0 && aColumn >= 0 )
                AddError( fmt::format( "Syntax error at line {}, column {}", aLine, aColumn ) );
            else
                AddError( "Syntax error in calculation expression" );
        }

        void AddParseFailure()
        {
            AddError( "Parser failed to parse input" );
        }

        bool HasErrors() const { return !m_errors.empty(); }
        bool HasWarnings() const { return !m_warnings.empty(); }
        const std::vector<std::string>& GetErrors() const  { return m_errors; }
        const std::vector<std::string>& GetWarnings() const { return m_warnings; }

        std::string GetAllMessages() const
        {
            std::string result;

            for( const std::string& error : m_errors )
                result += fmt::format( "Error: {}\n", error );

            for( const std::string& warning : m_warnings )
                result += fmt::format( "Warning: {}\n", warning );

            return result;
        }

        void Clear()
        {
            m_errors.clear();
            m_warnings.clear();
        }

    private:
        std::vector<std::string> m_errors;
        std::vector<std::string> m_warnings;
    };

    // Forward declarations for parser-related types
    class DOC;
    class PARSE_CONTEXT;
    class DOC_PROCESSOR;
}
