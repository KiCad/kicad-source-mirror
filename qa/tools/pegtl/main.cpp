/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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


/**
 * A program to test a PEGTL grammar
 */

#include <pegtl.hpp>
#include <pegtl/contrib/analyze.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <pegtl/contrib/parse_tree_to_dot.hpp>
#include <pegtl/contrib/trace.hpp>

#include <filesystem>
#include <iostream>

using namespace tao::pegtl;

//-------------------- Grammar definition ----------------------------------------------------

struct LINE_CONTINUATION : seq<one<'&'>, eol>{}; ///< Any text can span multiple lines using '&'

/**
 * String segment( no line continuation ), with exclusion rules
 */
template <typename... EXCLUSION_RULES>
struct STR_SEGMENT_EXCLUDING : plus<not_at<sor<eolf, LINE_CONTINUATION, EXCLUSION_RULES...>>, any>{};

/**
 * String with optional line continuation and exclusion rules
 */
template <typename... EXCLUSION_RULES>
struct STRING_EXCLUDING : plus<STR_SEGMENT_EXCLUDING<EXCLUSION_RULES...>, opt<LINE_CONTINUATION>> {};

/**
 * Control character with or without preceding whitespace
 */
template <char... CHAR_TO_FIND>
struct spaced_ch : seq<star<space>, one<CHAR_TO_FIND...>>{};

// Definition of "Format"
// # Format <current format number>
struct CURRENT_FORMAT_NUMBER : plus<digit> {};
struct FORMAT : seq<bol, TAO_PEGTL_STRING( "# FORMAT" ), star<space>, CURRENT_FORMAT_NUMBER>{};


// Definition of part
//.<Part name>_[(<Part number>)][:<Part version>][;<Description>]
    // string filters:
struct PART_NAME_FILTER : sor<spaced_ch<'('>, spaced_ch<':'>, spaced_ch<';'>>{};
struct PART_NUMBER_FILTER : one<')'>{};
struct PART_VERSION_FILTER : spaced_ch<';'>{};

    // part elements:
struct PART_NAME : STRING_EXCLUDING<PART_NAME_FILTER> {};
struct PART_NUMBER : STRING_EXCLUDING<PART_NUMBER_FILTER> {};
struct PART_VERSION : STRING_EXCLUDING<PART_VERSION_FILTER> {};
struct PART_DESCRIPTION : STRING_EXCLUDING<> {};

    // the part itself
struct PART :   seq
                <
                    bol,
                    one<'.'>,
                    PART_NAME,
                    opt<seq<spaced_ch<'('>, PART_NUMBER, one<')'>>>,
                    opt<seq<spaced_ch<':'>, PART_VERSION>>,
                    opt<seq<spaced_ch<';'>, PART_DESCRIPTION>>
                >
{};


struct UNMATCHED_CONTENT : STRING_EXCLUDING<FORMAT, PART> {}; //@todo remove once parser is complete


struct GRAMMAR :    plus
                    <
                        sor<FORMAT, PART, UNMATCHED_CONTENT>,
                        opt<eolf>
                    >
{};

//--------------------------Simple selector definition -----------------------------
// disable all parse nodes by default
template< typename PEGTL_RULE > struct SIMPLE_SELECTOR : std::false_type {};
// enable only nodes that match certain rules:
template<> struct SIMPLE_SELECTOR< STR_SEGMENT_EXCLUDING< PART_NAME_FILTER > > : std::true_type {};
template<> struct SIMPLE_SELECTOR< STR_SEGMENT_EXCLUDING< PART_NUMBER_FILTER > > : std::true_type {};
template<> struct SIMPLE_SELECTOR< STR_SEGMENT_EXCLUDING< PART_VERSION_FILTER > > : std::true_type {};
template<> struct SIMPLE_SELECTOR< STR_SEGMENT_EXCLUDING<> > : std::true_type {};
template<> struct SIMPLE_SELECTOR< CURRENT_FORMAT_NUMBER > : std::true_type {};
template<> struct SIMPLE_SELECTOR< FORMAT > : std::true_type {};
template<> struct SIMPLE_SELECTOR< PART > : std::true_type {};
template<> struct SIMPLE_SELECTOR< PART_NAME > : std::true_type {};
template<> struct SIMPLE_SELECTOR< PART_NUMBER > : std::true_type {};
template<> struct SIMPLE_SELECTOR< PART_VERSION > : std::true_type {};
template<> struct SIMPLE_SELECTOR< PART_DESCRIPTION > : std::true_type {};
template<> struct SIMPLE_SELECTOR< UNMATCHED_CONTENT > : std::true_type {}; //@todo remove


//--------------------------Complex selector definition -----------------------------
// Helper function to clean up the tree
struct FOLD_CONTENT : parse_tree::apply<FOLD_CONTENT>
{
    template <typename Node>
    static void transform( Node& n )
    {
        if( n->children.size() == 1 )
        {
            n->children.pop_back();
        }
        else if( n->children.size() != 0 )
        {
            n->remove_content();
        }
    }
};


template <typename PEGTL_RULE>
using COMPLEX_SELECTOR = parse_tree::selector<
        PEGTL_RULE,
        parse_tree::store_content::on<
            STR_SEGMENT_EXCLUDING< PART_NAME_FILTER >,
            STR_SEGMENT_EXCLUDING< PART_NUMBER_FILTER >,
            STR_SEGMENT_EXCLUDING< PART_VERSION_FILTER >,
            STR_SEGMENT_EXCLUDING<>,
            CURRENT_FORMAT_NUMBER,
            UNMATCHED_CONTENT //@todo remove when parser complete. For debugging only
        >,
        parse_tree::remove_content::on<
            FORMAT,
            PART
        >,
        parse_tree::apply< FOLD_CONTENT >::on<
            PART_NAME,
            PART_NUMBER,
            PART_VERSION,
            PART_DESCRIPTION
        >
    >;


int main( int argc, char** argv )
{
    //  .\test_pegtl.exe complex my_file.lib | dot -Tsvg > output.svg; .\output.svg

    if( argc != 3 )
    {
        printf( "usage: %s <complex|simple> <filename>", argv[0] );
        return -1;
    }

    std::string chosen_selector = argv[1];
    std::filesystem::path filepath( argv[2] );

    file_input in( filepath );

    const std::size_t issues = tao::pegtl::analyze<GRAMMAR>();

    if( issues > 0 )
    {
        std::cout << "\n***ERROR***: " << issues << " issues found in the grammar!\n";
        return -1;
    }

    std::unique_ptr<parse_tree::node> root;

    if( chosen_selector == "complex" )
    {
        root = parse_tree::parse<GRAMMAR, COMPLEX_SELECTOR>( in );
    }
    else if( chosen_selector == "simple" )
    {
        root = parse_tree::parse<GRAMMAR, SIMPLE_SELECTOR>( in );
    }
    else
    {
        printf( "Invalid selector '%s' requested. Valid values are: complex or simple. \n",
                argv[1] );
        printf( "usage: %s <complex|simple> <filename>\n", argv[0] );
    }


    if( root )
    {
        parse_tree::print_dot( std::cout, *root );
    }
    else
    {
        std::cout << "\n***ERROR***: No root tree node!\n";

        // lets print out the trace for debugging
        standard_trace<GRAMMAR>( in );
    }

    return 0;
}