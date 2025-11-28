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

#pragma once

#include <pegtl.hpp>

/*
 * Grammar for Allegro ASCII "view" .txt files used to specify
 * board DB extraction.
 */

namespace ALLEGRO
{

namespace EXTRACT_SPEC_PARSER
{

    // clang-format off

    namespace GRAMMAR
    {

    using namespace tao::pegtl;
    using eof = tao::pegtl::eof;

    namespace KEYWORDS
    {
        struct END :                TAO_PEGTL_STRING( "END" ) {};
        struct OR :                 TAO_PEGTL_STRING( "OR" ) {};

        struct SYMBOL :             TAO_PEGTL_STRING( "SYMBOL" ) {};
        struct CONNECTIVITY :       TAO_PEGTL_STRING( "CONNECTIVITY" ) {};
    }

    // All of "#"-style comments to end of line, including EOL
    struct HASH_COMMENT     : seq< one<'#'>, until< eol > > {};
    // Trailing whitespace or comment, including EOL
    struct TRAILING         : seq< star< blank >, sor<HASH_COMMENT, eol> > {};

    struct FIELD_NAME       : plus< sor< alnum, one<'_'> > > {};

    struct FIELD_LINE : seq<
        star< blank >,              // Leading whitespace
        not_at< KEYWORDS::END >,    // Not END (that ends a block)
        FIELD_NAME,                 // one of the field keywords
        TRAILING                    // Trailing comment or whitespace
    > {};

    struct EQUALS :     TAO_PEGTL_STRING( "=" ) {};
    struct NOT_EQUALS : TAO_PEGTL_STRING( "!=" ) {};

    struct OPERATOR : sor<
        EQUALS,
        NOT_EQUALS
    > {};

    // Quoted field value (single or double quotes). Content cannot contain the
    // matching quote character. Empty strings are allowed.
    struct SINGLE_QUOTED : seq< one<'\''>, star< not_one<'\''> >, one<'\''> > {};
    struct DOUBLE_QUOTED : seq< one<'"'>, star< not_one<'"'> >, one<'"'> > {};
    struct FIELD_VALUE : sor< SINGLE_QUOTED, DOUBLE_QUOTED > {};


    /**
     * A field line with a condition, e.g.:
     *  FIELD_NAME = 'value'   # comment
     */
    struct CONDITIONED_FIELD_LINE : seq<
        star< blank >,              // Leading whitespace
        not_at< KEYWORDS::END >,    // Not END (that ends a block)
        FIELD_NAME,                 // one of the field keywords
        star< blank >,
        OPERATOR,
        star< blank >,
        FIELD_VALUE,
        TRAILING                    // Trailing comment or whitespace
    > {};

    struct OR_LINE : seq<
        star< blank >,              // Leading whitespace
        not_at< KEYWORDS::END >,    // Not END (that ends a block)
        KEYWORDS::OR,
        TRAILING                    // Trailing comment or whitespace
    > {};

    /*
    * Generic block structure:
    * <HEADER>
    *    <FIELD_NAME_1>
    *    <FIELD_NAME_2>
    *    ...
    * END
    */
    template<typename Header>
    struct GENERIC_BLOCK : seq<
        Header, TRAILING,           // HEADER line
        star< sor<
            OR_LINE,
            CONDITIONED_FIELD_LINE,
            FIELD_LINE,
            TRAILING
        > >,
        star< blank >,
        KEYWORDS::END, TRAILING     // END line
    > {};

    // Individual blocks are defined as specializations of GENERIC_BLOCK
    // with the appropriate header keyword
    // (we could list all suitable field keywords here if we wanted to be more strict)
    using SYMBOL_BLOCK = GENERIC_BLOCK<KEYWORDS::SYMBOL>;
    using CONNECTIVITY_BLOCK = GENERIC_BLOCK<KEYWORDS::CONNECTIVITY>;

    struct ANY_BLOCK : sor<
        SYMBOL_BLOCK,
        CONNECTIVITY_BLOCK
    > {};

    // The overall extract spec file consists of zero or more blocks
    // separated by blank/comment lines, ending with EOF
    struct EXTRACT_SPEC : seq<
        star< sor<
            ANY_BLOCK,
            TRAILING
        > >,
        eof
    > {};


    } // namespace GRAMMAR

    // clang-format on

} // namespace EXTRACT_SPEC_PARSER

} // namespace ALLEGRO
