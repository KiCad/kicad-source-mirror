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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "utils/extract/extract_spec_parser.h"
#include "utils/extract/extract_spec_grammar.h"
#include "utils/extract/data_dictionary.h"

#include <set>

#include <fmt/format.h>

#include <wx/log.h>
#include <wx/translation.h>


using namespace ALLEGRO::EXTRACT_SPEC_PARSER;


static const char* traceAllegroExtract = "ALLEGRO_EXTRACT";


template <typename Rule>
struct EXTRACT_SPEC_PARSER_ACTION : tao::pegtl::nothing<Rule>
{
};


template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::HEADER_KEYWORD>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, PARSER_STATE& s )
    {
        wxLogTrace( traceAllegroExtract, "Parsing header keyword: '%s'", in.string().c_str() );

        // clang-format off
        static const std::unordered_map<std::string, IR::VIEW_TYPE> k_keyword_to_view_type = {
            { "SYMBOL",         IR::VIEW_TYPE::SYMBOL },
            { "CONNECTIVITY",   IR::VIEW_TYPE::CONNECTIVITY },
            { "COMPONENT_PIN",  IR::VIEW_TYPE::COMPONENT_PIN },
            { "COMPONENT",      IR::VIEW_TYPE::COMPONENT },
            { "FUNCTION",       IR::VIEW_TYPE::FUNCTION },
            { "LOGICAL_PIN",    IR::VIEW_TYPE::LOGICAL_PIN },
            { "NET",            IR::VIEW_TYPE::NET },
            { "COMPOSITE_PAD",  IR::VIEW_TYPE::COMPOSITE_PAD },
            { "GEOMETRY",       IR::VIEW_TYPE::GEOMETRY },
            { "FULL_GEOMETRY",  IR::VIEW_TYPE::FULL_GEOMETRY },
            { "BOARD",          IR::VIEW_TYPE::BOARD },
            { "LAYER",          IR::VIEW_TYPE::LAYER },
            { "RAT_PIN",        IR::VIEW_TYPE::RAT_PIN },
        };
        // clang-format on

        auto it = k_keyword_to_view_type.find( in.string() );
        if( it == k_keyword_to_view_type.end() )
        {
            throw tao::pegtl::parse_error(
                fmt::format( "Unknown header keyword '{}'", in.string() ), in.position() );
        }
        s.CurrentBlock.ViewType = it->second;
        s.CurrentBlock.Fields.clear();
        s.CurrentBlock.OrConditions.clear();
    }
};


/*
 * Check and store a parsed field name in the current block.
 *
 * We do it this way to avoid hundreds of keywords and complex logic
 * in the grammar.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::FIELD_NAME>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, PARSER_STATE& s )
    {
        // It's not clear to me that we can easily detect valid/invalid fields
        // for the current view here, as there are hundreds of fields, and also
        // prefixes and maybe also properties.

        // if( !ALLEGRO::IsValidFieldForView( in.string(), s.CurrentBlock.ViewType ) )
        // {
        //     throw tao::pegtl::parse_error( fmt::format( "Invalid field '{}' for block type {}", in.string(),
        //                                                 static_cast<int>( s.CurrentBlock.ViewType ) ),
        //                                    in.position() );
        // }

        s.CurrentField = in.string();
    }
};


/**
 * Store the parsed field value in the current block.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::FIELD_VALUE>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, PARSER_STATE& s )
    {
        wxString value = wxString::FromUTF8( in.string().c_str() );

        // Strip quotes if present, which may result in empty string
        if( value.StartsWith( "'" ) && value.EndsWith( "'" ) ||
            value.StartsWith( "\"" ) && value.EndsWith( "\"" ) )
        {
            value = value.Mid( 1, value.Length() - 2 );
        }

        s.CurrentConditionValue = value;
    }
};

template<>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::EQUALS>
{
    template<typename ActionInput>
    static void apply(const ActionInput&, PARSER_STATE& st)
    {
        st.CurrentConditionEquals = true;
    }
};


template<>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::NOT_EQUALS>
{
    template<typename ActionInput>
    static void apply(const ActionInput&, PARSER_STATE& st)
    {
        st.CurrentConditionEquals = false;
    }
};


/**
 * When completing a simple field line, just store the field name.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::FIELD_LINE>
{
    static void apply0( PARSER_STATE& s )
    {
        s.CurrentBlock.Fields.emplace_back( s.CurrentField );
    }
};


template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::CONDITIONED_FIELD_LINE>
{
    static void apply0( PARSER_STATE& s )
    {
        IR::CONDITION cond{ s.CurrentField, s.CurrentConditionEquals, s.CurrentConditionValue };

        // Start a new AND condition set if needed
        if( s.CurrentBlock.OrConditions.empty() )
        {
            s.CurrentBlock.OrConditions.emplace_back();
        }

        s.CurrentBlock.OrConditions.back().emplace_back( std::move( cond ) );
    }
};

/**
 * When encountering an OR, start a new AND condition set.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::OR_LINE>
{
    static void apply0( PARSER_STATE& s )
    {
        s.CurrentBlock.OrConditions.emplace_back();
    }
};


/**
 * When completing a block, store it in the spec model and reset the current block.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::VIEW_BLOCK>
{
    static void apply0( PARSER_STATE& s )
    {
        wxLogTrace( traceAllegroExtract, "Complete parsing block with %zu fields and %zu OR conditions",
                    s.CurrentBlock.Fields.size(), s.CurrentBlock.OrConditions.size() );
        s.Model.Blocks.emplace_back( std::move( s.CurrentBlock ) );
        s.CurrentBlock = IR::BLOCK();
    }
};

// #include <pegtl/contrib/trace.hpp>

tl::expected<IR::SPEC, PARSE_ERROR> PARSER::ParseBuffer( const std::string& aBuffer )
{
    tao::pegtl::memory_input in( aBuffer, "" );
    PARSER_STATE             state;
    wxLogTrace( traceAllegroExtract, "PARSER::Parse from string buffer" );

    try
    {
        // complete_trace
        if( !tao::pegtl::parse<tao::pegtl::must<GRAMMAR::EXTRACT_SPEC>, EXTRACT_SPEC_PARSER_ACTION>( in, state ) )
        {
            wxLogTrace( traceAllegroExtract, "Parsing failed without throwing" );

            return tl::unexpected(
                    PARSE_ERROR( { .description = _( "An unexpected error occurred while reading library table" ) } ) );
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        const auto& p = e.positions().front();
        std::string msg = fmt::format( "Error at line {}, column {}:\n{}\n{:>{}}\n{}", p.line, p.column,
                                       in.line_at( p ), "^", p.column, e.message() );

        wxLogTrace( traceAllegroExtract, "%s", msg.c_str() );

        return tl::unexpected( PARSE_ERROR(
                { .description = wxString::Format( _( "Syntax error at line %zu, column %zu" ), p.line, p.column ),
                  .line = p.line,
                  .column = p.column } ) );
    }

    return state.Model;
}
