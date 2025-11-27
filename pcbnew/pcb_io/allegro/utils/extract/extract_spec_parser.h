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


#include <tl/expected.hpp>

#include <wx/string.h>

namespace ALLEGRO
{
namespace EXTRACT_SPEC_PARSER
{
    /**
     * Classes representing the intermediate representation (IR) of an
     * Allegro extract specification.
     */
    namespace IR
    {
        enum class BLOCK_TYPE
        {
            UNKNOWN, // Not read yet?
            SYMBOL,  // Symbol instance
        };

        // A single block in the extract spec is a type and a set of fields in order.
        class BLOCK
        {
        public:
            BLOCK() :
                    Type( BLOCK_TYPE::UNKNOWN )
            {
            }

            BLOCK_TYPE Type;
            // We could be super-typesafe here with a variant for the fields, based on the
            // block type, but this is probably sufficient for now and the parser is handling
            // the expected fields.
            std::vector<std::string> Fields;
        };

        struct SPEC
        {
            // Filename?
            std::vector<BLOCK> Blocks;
        };

    } // namespace IR


    struct PARSER_STATE
    {
        PARSER_STATE() :
                target_string( nullptr )
        {
        }

        std::string* target_string;
        IR::BLOCK    current_block;
        IR::SPEC     model;
    };

    struct PARSE_ERROR
    {
        wxString description;
        size_t   line = 0;
        size_t   column = 0;
    };

    class PARSER
    {
    public:
        PARSER() {}

        tl::expected<IR::SPEC, PARSE_ERROR> ParseBuffer( const std::string& aBuffer );
    };


} // namespace EXTRACT_SPEC_PARSER


} // namespace ALLEGRO
