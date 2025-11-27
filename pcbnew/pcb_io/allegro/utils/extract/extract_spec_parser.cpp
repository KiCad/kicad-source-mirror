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
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::KEYWORDS::SYMBOL>
{
    static void apply0( PARSER_STATE& s ) { s.current_block.Type = IR::BLOCK_TYPE::SYMBOL; }
};


static const std::vector<std::string> k_symbol_field_names = {
    "SYM_TYPE",     "SYM_NAME",     "REFDES",     "SYM_X",      "SYM_Y",
    "SYM_CENTER_X", "SYM_CENTER_Y", "SYM_MIRROR", "SYM_ROTATE", "SYM_LIBRARY_PATH",
};


static bool IsValidField( const std::string& field_name, const std::vector<std::string>& valid_field_names )
{
    return std::find( valid_field_names.begin(), valid_field_names.end(), field_name ) != valid_field_names.end();
}


static bool IsValidFieldForCurrentBlock( const std::string& field_name, const PARSER_STATE& state )
{
    switch( state.current_block.Type )
    {
    case IR::BLOCK_TYPE::SYMBOL: return IsValidField( field_name, k_symbol_field_names );
    }

    return false;
}


/*
 * Check and store a parsed field name in the current block.
 *
 * We do it this way to avoid dozens of nearly identical specializations
 * for each field, but we could deal with this in the grammar if needed.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::FIELD_NAME>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, PARSER_STATE& s )
    {
        if( !IsValidFieldForCurrentBlock( in.string(), s ) )
        {
            throw tao::pegtl::parse_error( fmt::format( "Invalid field '{}' for block type {}", in.string(),
                                                        static_cast<int>( s.current_block.Type ) ),
                                           in.position() );
        }

        s.current_block.Fields.emplace_back( in.string().c_str() );
    }
};


/**
 * When completing a block, store it in the spec model and reset the current block.
 */
template <>
struct EXTRACT_SPEC_PARSER_ACTION<GRAMMAR::ANY_BLOCK>
{
    static void apply0( PARSER_STATE& s )
    {
        wxLogTrace( traceAllegroExtract, "Complete parsing block with %zu fields", s.current_block.Fields.size() );
        s.model.Blocks.emplace_back( std::move( s.current_block ) );
        s.current_block = IR::BLOCK();
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

    return state.model;
}
