/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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


#include <markup_parser.h>
#include <sstream>
#include <string_utils.h>

using namespace MARKUP;


std::unique_ptr<NODE> MARKUP_PARSER::Parse()
{
    try
    {
        std::unique_ptr<NODE> root;

        if( mem_in )
            root = parse_tree::parse<MARKUP::grammar, MARKUP::NODE, MARKUP::selector>( *mem_in );
        else
            root = parse_tree::parse<MARKUP::grammar, MARKUP::NODE, MARKUP::selector>( *in );

        return root;
    }
    catch ( tao::pegtl::parse_error& )
    {
        // couldn't parse text item
        // TODO message to user?
        return nullptr;
    }
}


std::string NODE::typeString() const
{
    std::stringstream os;

    if( is_type<MARKUP::subscript>() )                  os << "SUBSCRIPT";
    else if( is_type<MARKUP::superscript>() )           os << "SUPERSCRIPT";
    else if( is_type<MARKUP::overbar>() )               os << "OVERBAR";
    else if( is_type<MARKUP::anyString>() )             os << "ANYSTRING";
    else if( is_type<MARKUP::url>() )                   os << "URL";
    else if( is_type<MARKUP::anyStringWithinBraces>() ) os << "ANYSTRINGWITHINBRACES";
    else                                                os << "other";

    return os.str();
}


wxString NODE::asWxString() const
{
    return From_UTF8( string().c_str() );
}


std::string NODE::asString() const
{
    std::stringstream os;

    os << type;

    if( has_content() )
        os << " \"" << string() << "\"";

    return os.str();
}
