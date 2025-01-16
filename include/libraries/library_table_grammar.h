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

#ifndef LIBRARY_TABLE_GRAMMAR_H
#define LIBRARY_TABLE_GRAMMAR_H

#include <pegtl.hpp>

/*
 * Grammar for a KiCad library stored in s-expression format.  This parser is generic and
 * supports all known types of library tables -- the differences in code between things like
 * symbol and footprint libraries are handled elsewhere in the architecture.
 */

namespace LIBRARY_TABLE_GRAMMAR
{
using namespace tao::pegtl;

namespace KEYWORDS
{
    struct LIB : TAO_PEGTL_STRING( "lib" ) {};
    struct NAME : TAO_PEGTL_STRING( "name" ) {};
    struct TYPE : TAO_PEGTL_STRING( "type" ) {};
    struct URI : TAO_PEGTL_STRING( "uri" ) {};
    struct OPTIONS : TAO_PEGTL_STRING( "options" ) {};
    struct DESCR : TAO_PEGTL_STRING( "descr" ) {};

    struct SYM_LIB_TABLE : TAO_PEGTL_STRING( "sym_lib_table" ) {};
    struct FP_LIB_TABLE : TAO_PEGTL_STRING( "fp_lib_table" ) {};
    struct DESIGN_BLOCK_LIB_TABLE : TAO_PEGTL_STRING( "design_block_lib_table" ) {};
    struct VERSION : TAO_PEGTL_STRING( "version" ) {};
}

struct LPAREN : TAO_PEGTL_STRING( "(" ) {};
struct RPAREN : TAO_PEGTL_STRING( ")" ) {};

// An s-expression identifier token
struct TOKEN : plus< not_one< '(', ')', ' ', '\t', '\n', '\r' > > {};

struct QUOTED_TEXT : if_must< one< '"' >, until< one< '"' > > > {};

// Inner expression
struct SEXPR_CONTENT : star<
        sor< QUOTED_TEXT, TOKEN, seq< LPAREN, must< SEXPR_CONTENT, RPAREN > > >
        > {};

// Outer (top-level) expression
struct SEXPR : if_must< LPAREN, SEXPR_CONTENT, RPAREN > {};

struct TABLE_TYPE : sor<
        KEYWORDS::SYM_LIB_TABLE,
        KEYWORDS::FP_LIB_TABLE,
        KEYWORDS::DESIGN_BLOCK_LIB_TABLE
        > {};

struct LIB_PROPERTY_KEY : sor<
        KEYWORDS::NAME,
        KEYWORDS::TYPE,
        KEYWORDS::URI,
        KEYWORDS::OPTIONS,
        KEYWORDS::DESCR
        > {};

// Allow both quoted and non-quoted (whitespace-free) strings
struct PROPERTY_VALUE : sor< QUOTED_TEXT, TOKEN > {};

// (key value) or (key "value")
struct LIB_PROPERTY : seq<
        LPAREN,
        LIB_PROPERTY_KEY,
        plus< space >,
        PROPERTY_VALUE,
        RPAREN
        > {};

// (version %d)
struct TABLE_VERSION : seq<
        LPAREN,
        KEYWORDS::VERSION,
        plus< space >,
        PROPERTY_VALUE,
        RPAREN
        > {};

// (lib (name ...)(type ...)...)
struct LIB_ROW : if_must<
        pad< LPAREN, space >,
        KEYWORDS::LIB,
        plus< space >,
        plus< pad< LIB_PROPERTY, space > >,
        pad< RPAREN, space>
        > {};

struct LIB_TABLE : if_must<
        LPAREN,
        TABLE_TYPE,
        pad_opt< TABLE_VERSION, space >,
        star< pad< LIB_ROW, space > >,
        pad< RPAREN, space >
        > {};

struct LIB_TABLE_FILE : until< eof, LIB_TABLE > {};

} // namespace LIBRARY_TABLE_GRAMMAR

#endif //LIBRARY_TABLE_GRAMMAR_H
