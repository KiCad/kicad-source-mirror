/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file
 * Test suite for MARKUP_PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <markup_parser.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( MarkupParser )

void nodeToString( std::unique_ptr<MARKUP::NODE>& aNode, std::string& aStringToPopulate )
{
    aStringToPopulate += " {";

    if( aNode->isOverbar() )
        aStringToPopulate += "OVER";
    if( aNode->isSubscript() )
        aStringToPopulate += "SUB";
    if( aNode->isSuperscript() )
        aStringToPopulate += "SUP";

    if( aNode->has_content() )
        aStringToPopulate += "'" + aNode->string() + "'";

    for( auto& c : aNode->children )
        nodeToString( c, aStringToPopulate );

    aStringToPopulate += "} ";
}

struct PARSE_CASE
{
    std::string Input;
    std::string ExpectedResult;
};

/**
 * Test the #Parse method.
 */
BOOST_AUTO_TEST_CASE( Parse )
{

    std::vector<PARSE_CASE> cases =
    {
        {
            "A normal string",
            " { {'A normal string'} } "
        },
        {
            "_{A subscript String}",
            " { {SUB {'A subscript String'} } } "
        },
        {
            "^{A superscript String}",
            " { {SUP {'A superscript String'} } } "
        },
        {
            "~{An overbar String}",
            " { {OVER {'An overbar String'} } } "
        },
        {
            "~{An incomplete markup",
            " { {'~{An incomplete markup'} } "
        },
        {
            "A string ~{overbar}",
            " { {'A string '}  {OVER {'overbar'} } } "
        },
        {
            "A string ~{incomplete markup",
            " { {'A string ~{incomplete markup'} } "
        },
        {
            "A string ~{overbar} ~{incomplete markup",
            " { {'A string '}  {OVER {'overbar'} }  {' ~{incomplete markup'} } "
        },
        {   "A string ~{incomplete markup ~{overbar}",
            " { {'A string ~{incomplete markup '}  {OVER {'overbar'} } } "
        }
    };

    for( auto& c : cases )
    {
        BOOST_TEST_INFO_SCOPE( c.Input );
        MARKUP::MARKUP_PARSER parser( c.Input );

        std::unique_ptr<MARKUP::NODE> rootNode = parser.Parse();
        BOOST_REQUIRE( rootNode );

        std::string result;
        nodeToString( rootNode, result );

        BOOST_CHECK_EQUAL( result, c.ExpectedResult );

        // Uncomment for testing / generating test cases:
        // printf( "%s\n", result.c_str() );
    }

}


BOOST_AUTO_TEST_SUITE_END()
