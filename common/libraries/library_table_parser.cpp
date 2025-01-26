/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <fmt/format.h>
#include <pegtl/contrib/parse_tree.hpp>
#include <wx/log.h>

#include <libraries/library_table_parser.h>
#include <libraries/library_table_grammar.h>
#include <trace_helpers.h>
#include <boost/locale/boundary/types.hpp>


using namespace LIBRARY_TABLE_GRAMMAR;


struct LIBRARY_TABLE_PARSER_STATE
{
    LIBRARY_TABLE_PARSER_STATE() :
        target_string( nullptr ),
        current_row_model( {} ),
        model( {} )
    {}

    std::string* target_string;
    LIBRARY_TABLE_ROW_IR current_row_model;
    LIBRARY_TABLE_IR model;
};


template <typename Rule>
struct LIBRARY_TABLE_PARSER_ACTION : tao::pegtl::nothing<Rule>
{
};


template <>
struct LIBRARY_TABLE_PARSER_ACTION<KEYWORDS::SYM_LIB_TABLE>
{
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )
    {
        s.model.type = LIBRARY_TABLE_TYPE::SYMBOL;
    }
};


template <>
struct LIBRARY_TABLE_PARSER_ACTION<KEYWORDS::FP_LIB_TABLE>
{
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )
    {
        s.model.type = LIBRARY_TABLE_TYPE::FOOTPRINT;
    }
};


template <>
struct LIBRARY_TABLE_PARSER_ACTION<KEYWORDS::DESIGN_BLOCK_LIB_TABLE>
{
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )
    {
        s.model.type = LIBRARY_TABLE_TYPE::DESIGN_BLOCK;
    }
};

template <>
struct LIBRARY_TABLE_PARSER_ACTION<TOKEN>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, LIBRARY_TABLE_PARSER_STATE& s )
    {
        wxCHECK2( s.target_string, return );
        *s.target_string = in.string();
    }
};


template <>
struct LIBRARY_TABLE_PARSER_ACTION<QUOTED_TEXT>
{
    template <typename ActionInput>
    static void apply( const ActionInput& in, LIBRARY_TABLE_PARSER_STATE& s )
    {
        wxCHECK2( s.target_string, return );
        wxCHECK2( in.string().size() >= 2, return );
        *s.target_string = in.string().substr( 1, in.string().size() - 2 );
    }
};


#define DEFINE_STRING_ACTION( Rule, StateVariable )                                              \
template <>                                                                                      \
struct LIBRARY_TABLE_PARSER_ACTION<Rule>                                                         \
{                                                                                                \
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )                                          \
    {                                                                                            \
        s.target_string = &s.StateVariable;                                                      \
    }                                                                                            \
}                                                                                                \

DEFINE_STRING_ACTION( KEYWORDS::VERSION, model.version );
DEFINE_STRING_ACTION( KEYWORDS::NAME, current_row_model.nickname );
DEFINE_STRING_ACTION( KEYWORDS::TYPE, current_row_model.type );
DEFINE_STRING_ACTION( KEYWORDS::URI, current_row_model.uri );
DEFINE_STRING_ACTION( KEYWORDS::OPTIONS, current_row_model.options );
DEFINE_STRING_ACTION( KEYWORDS::DESCR, current_row_model.description );

// Handle (hidden), (disabled)
#define DEFINE_FLAG_ACTION( Rule, StateVariable )                                                \
template <>                                                                                      \
struct LIBRARY_TABLE_PARSER_ACTION<Rule>                                                         \
{                                                                                                \
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )                                          \
    {                                                                                            \
        s.StateVariable = true;                                                                  \
    }                                                                                            \
}                                                                                                \

DEFINE_FLAG_ACTION( HIDDEN_MARKER, current_row_model.hidden );
DEFINE_FLAG_ACTION( DISABLED_MARKER, current_row_model.disabled );


template <>
struct LIBRARY_TABLE_PARSER_ACTION<LIB_ROW>
{
    static void apply0( LIBRARY_TABLE_PARSER_STATE& s )
    {
        s.model.rows.emplace_back( s.current_row_model );
        s.current_row_model = LIBRARY_TABLE_ROW_IR();
    }
};


LIBRARY_TABLE_PARSER::LIBRARY_TABLE_PARSER()
{
}


tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> LIBRARY_TABLE_PARSER::Parse(
        const std::filesystem::path& aPath )
{
    file_input in( aPath );
    LIBRARY_TABLE_PARSER_STATE state;
    wxLogTrace( traceLibraries, "LIBRARY_TABLE_PARSER::Parse %s", aPath.string().c_str() );

    try
    {
        if( !parse<LIB_TABLE_FILE, LIBRARY_TABLE_PARSER_ACTION>( in, state ) )
        {
            wxLogTrace( traceLibraries, "Parsing failed without throwing" );
            wxString msg =
                wxString::Format( _( "An unexpected error occurred while reading library table %s "),
                                  aPath.string().c_str() );
            return tl::unexpected( LIBRARY_PARSE_ERROR( { .description = msg } ) );
        }
    }
    catch( const parse_error& e )
    {
        const auto& p = e.positions().front();
        std::string msg = fmt::format( "Error at line {}, column {}:\n{}\n{:>{}}\n{}",
                                       p.line, p.column, in.line_at( p ), "^", p.column,
                                       e.message() );

        wxLogTrace( traceLibraries, "%s", msg.c_str() );

        wxString description = wxString::Format( _( "Syntax error at line %zu, column %zu" ),
                                                 p.line, p.column );

        return tl::unexpected( LIBRARY_PARSE_ERROR( {
            .description = description,
            .line = p.line,
            .column = p.column
        } ) );
    }

    return state.model;
}
