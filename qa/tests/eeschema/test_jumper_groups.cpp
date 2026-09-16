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

#include <boost/test/tools/old/interface.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <vector>

#include <string_utils.h>
#include <jumper_group.h>
#include <lib_symbol.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>


/**
 * Boost.Test print helper for jumper group collections.
 */
std::ostream& boost_test_print_type( std::ostream& aStream, const JUMPER_GROUP_SET& aGroups )
{
    if( aGroups.IsEmpty() )
        return aStream << "<no groups>";


    std::vector<JUMPER_GROUP> groups = aGroups.GetAll();

    aStream << groups.size() << " group(s): ";
    for( size_t ii = 0; ii < groups.size(); ++ii )
    {
        if( ii > 0 )
            aStream << " ";

        aStream << "(";

        for( const wxString& name : groups[ii].GetNames() )
            aStream << "\"" << name << "\" ";

        aStream << ")";
    }

    return aStream;
}


BOOST_AUTO_TEST_SUITE( ParseJumperGroupSexprs )


struct PARSED_JUMPER_TEST_CASE
{
    std::string                     input;
    std::vector<std::set<wxString>> expected_groups;
};


BOOST_AUTO_TEST_CASE( WellFormedJumperPinGroupsAreParsed )
{
    const std::vector<PARSED_JUMPER_TEST_CASE> valid_jumper_groups = {
        // Nothing at all
        // I.e. just `(jumper_pin_groups)`
        {
                "",
                {},
        },
        // Empty groups list
        {
                "()",
                {},
        },
        // A group whose only name is blank contributes nothing
        {
                "(\"\")",
                {},
        },
        // A single group with a single pin
        {
                "(\"1\")",
                { { "1" } },
        },
        // A single group with multiple pins
        {
                "(\"1\" \"2\" \"3\")",
                { { "1", "2", "3" } },
        },
        // Multiple groups with a single pin each
        {
                "(\"1\") (\"2\") (\"3\")",
                { { "1" }, { "2" }, { "3" } },
        },
        // Multiple groups with multiple pins each
        {
                "(\"1\" \"2\") (\"3\" \"4\") (\"5\" \"6\")",
                { { "1", "2" }, { "3", "4" }, { "5", "6" } },
        },
    };

    for( const PARSED_JUMPER_TEST_CASE& test_case : valid_jumper_groups )
    {
        BOOST_TEST_CONTEXT( "Testing well-formed jumper pin group: " << test_case.input )
        {
            std::string good = "(symbol \"Good\" (jumper_pin_groups " + test_case.input + "))";

            std::vector<LIB_SYMBOL*> loaded = SCH_IO_KICAD_SEXPR::ParseLibSymbols( good, "good jumper groups" );

            BOOST_REQUIRE_EQUAL( loaded.size(), 1u );
            LIB_SYMBOL* const symbol = loaded.front();
            BOOST_REQUIRE( symbol );

            // Build the expected groups from the test case data
            JUMPER_GROUP_SET expected_groups;
            for( const std::set<wxString>& groupNames : test_case.expected_groups )
                expected_groups.Add( std::move( groupNames ) );

            // BOOST_TEST == doesn't print nicely
            BOOST_CHECK_EQUAL( symbol->JumperPinGroups(), expected_groups );
        }
    }
}


BOOST_AUTO_TEST_CASE( MalformedJumperPinGroupsAreRejected )
{
    const std::vector<std::string> throwing_jumper_groups = {
        // A literal string must be rejected, even if empty
        "\"\"",
        "\"A\"", // Sentry KICAD-1A3H
        // Literal non-strings
        "42",
        "42.42",
        // Groups of non-strings
        "(42)",
        "(42.42)",
        "(42 42)",
        "((42 42))",
        "(bad_keyword 42)",
    };

    for( const std::string& bad_jumper_group : throwing_jumper_groups )
    {
        BOOST_TEST_CONTEXT( "Testing malformed jumper pin group: " << bad_jumper_group )
        {
            std::string bad = "(symbol \"Bad\" (jumper_pin_groups " + bad_jumper_group + "))";

            BOOST_CHECK_THROW( SCH_IO_KICAD_SEXPR::ParseLibSymbols( bad, "bad jumper groups" ), IO_ERROR );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
