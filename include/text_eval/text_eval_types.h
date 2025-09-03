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

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <fmt/format.h>

namespace calc_parser
{
    using Value = std::variant<double, std::string>;

    template<typename T>
    class Result
    {
    private:
        std::variant<T, std::string> m_data;

    public:
        Result( T aValue ) : m_data( std::move( aValue ) ) {}
        Result( std::string aError ) : m_data( std::move( aError ) ) {}

        auto HasValue() const -> bool { return std::holds_alternative<T>( m_data ); }
        auto HasError() const -> bool { return std::holds_alternative<std::string>( m_data ); }

        auto GetValue() const -> const T& { return std::get<T>( m_data ); }
        auto GetError() const -> const std::string& { return std::get<std::string>( m_data ); }

        explicit operator bool() const { return HasValue(); }
    };

    template<typename T>
    auto MakeError( std::string aMsg ) -> Result<T>
    {
        return Result<T>( std::move( aMsg ) );
    }

    template<typename T>
    auto MakeValue( T aVal ) -> Result<T>
    {
        return Result<T>( std::move( aVal ) );
    }

    class ERROR_COLLECTOR
    {
    private:
        std::vector<std::string> m_errors;
        std::vector<std::string> m_warnings;

    public:
        auto AddError( std::string aError ) -> void
        {
            m_errors.emplace_back( std::move( aError ) );
        }

        auto AddWarning( std::string aWarning ) -> void
        {
            m_warnings.emplace_back( std::move( aWarning ) );
        }

        auto AddSyntaxError( int aLine = -1, int aColumn = -1 ) -> void
        {
            if( aLine >= 0 && aColumn >= 0 )
                AddError( fmt::format( "Syntax error at line {}, column {}", aLine, aColumn ) );
            else
                AddError( "Syntax error in calculation expression" );
        }

        auto AddParseFailure() -> void
        {
            AddError( "Parser failed to parse input" );
        }

        auto HasErrors() const -> bool { return !m_errors.empty(); }
        auto HasWarnings() const -> bool { return !m_warnings.empty(); }
        auto GetErrors() const -> const std::vector<std::string>& { return m_errors; }
        auto GetWarnings() const -> const std::vector<std::string>& { return m_warnings; }

        auto GetAllMessages() const -> std::string
        {
            std::string result;
            for( const auto& error : m_errors )
                result += fmt::format( "Error: {}\n", error );

            for( const auto& warning : m_warnings )
                result += fmt::format( "Warning: {}\n", warning );

            return result;
        }

        auto Clear() -> void
        {
            m_errors.clear();
            m_warnings.clear();
        }
    };

    // Forward declarations for parser-related types
    class DOC;
    class PARSE_CONTEXT;
    class DOC_PROCESSOR;
}
