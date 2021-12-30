/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

using namespace MARKUP;


MARKUP::MARKUP_NODE MARKUP_PARSER::Parse()
{
    //string_input<> in( source, "from_input" );
    auto root = parse_tree::parse<MARKUP::grammar, MARKUP::NODE, MARKUP::selector>( in );
    return root;
}


std::ostream& operator<<( std::ostream& os, const MARKUP_NODE& node )
{
    os << "<";

    if( !node->is_root() )
        os << node->asString();

    for( const auto& child : node->children )
        os << " " << child;

    os << ">";

    return os;
}


std::string NODE::typeString() const
{
    std::stringstream os;

    if( is<MARKUP::subscript>() )                  os << "SUBSCRIPT";
    else if( is<MARKUP::superscript>() )           os << "SUPERSCRIPT";
    else if( is<MARKUP::anyString>() )             os << "ANYSTRING";
    else if( is<MARKUP::anyStringWithinBraces>() ) os << "ANYSTRINGWITHINBRACES";
    else if( is<MARKUP::varNamespaceName>() )      os << "VARNAMESPACENAME";
    else if( is<MARKUP::varName>() )               os << "VARNAME";
    else                                           os << "other";

    return os.str();
}


std::string NODE::asString() const
{
    std::stringstream os;

    os << name(); // << "{" << typeString() << "}";

    if( has_content() )
        os << " \"" << string() << "\"";

    return os.str();
}
