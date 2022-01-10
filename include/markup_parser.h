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

#ifndef MARKUP_PARSER_H
#define MARKUP_PARSER_H

#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <iostream>
#include <string>
#include <utf8.h>


namespace MARKUP
{
using namespace tao::pegtl;

struct subscript;
struct superscript;
struct overbar;

struct NODE : parse_tree::basic_node<NODE>
{
    std::string asString() const;

    std::string typeString() const;

    bool isOverbar() const     { return is_type<MARKUP::overbar>(); }
    bool isSubscript() const   { return is_type<MARKUP::subscript>(); }
    bool isSuperscript() const { return is_type<MARKUP::superscript>(); }
};

struct varPrefix : string<'$', '{'> {};

struct subPrefix : string<'_', '{'> {};

struct supPrefix : string<'^', '{'> {};

struct tildePrefix : string<'~', '{'> {};

struct closeBrace : string<'}'> {};

struct varName : plus<sor<identifier_other, string<' '>>> {};

struct varNamespaceName : plus<identifier> {};

struct varNamespace : seq<varNamespaceName, string<':'>> {};

struct variable : seq<varPrefix, opt<varNamespace>, varName, closeBrace> {};

/**
 * anyString =
 * a run of characters that do not start a command sequence, or if they do, they do not start
 * a complete command prefix (command char + open brace)
 */
struct anyString : plus<sor<utf8::not_one<'~', '$', '_', '^'>,
                            seq<not_at<subPrefix>, string<'_'>>,
                            seq<not_at<supPrefix>, string<'^'>>,
                            seq<not_at<tildePrefix>, string<'~'>>>> {};

struct prefixedSuperscript : seq<supPrefix, superscript> {};

struct prefixedSubscript : seq<subPrefix, subscript> {};

struct prefixedOverbar : seq<tildePrefix, overbar> {};

struct anyStringWithinBraces : plus<sor<utf8::not_one<'~', '$', '_', '^', '}'>>> {};

struct superscript : until<closeBrace, sor<variable, anyStringWithinBraces>> {};

struct subscript : until<closeBrace, sor<variable, anyStringWithinBraces>> {};

struct overbar : until<closeBrace, sor<variable, anyStringWithinBraces>> {};

/**
 * Finally, the full grammar
 */
struct grammar : star<sor<variable,
                          prefixedSubscript,
                          prefixedSuperscript,
                          prefixedOverbar,
                          anyString>> {};

template <typename Rule>
using selector = parse_tree::selector< Rule,
                                       parse_tree::store_content::on<varNamespaceName,
                                                                     varName,
                                                                     anyString,
                                                                     anyStringWithinBraces>,
                                       parse_tree::discard_empty::on<superscript,
                                                                     subscript,
                                                                     overbar>>;

class MARKUP_PARSER
{
public:
    MARKUP_PARSER( const std::string& source ) :
            in( source, "from_input" )
    {}

    std::unique_ptr<NODE> Parse();

private:
    string_input<> in;
};

} // namespace MARKUP


#endif //MARKUP_PARSER_H
