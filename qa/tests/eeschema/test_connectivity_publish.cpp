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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <connectivity/conn_publish.h>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityPublish )

BOOST_AUTO_TEST_CASE( GroupSuffixPreservesQuotedAndEscapedPrefixes )
{
    const std::vector<std::pair<wxString, wxString>> cases{
        { "MY\\ BUS{A B}", "MY\\ BUS_1{A B}" },
        { "\"MY BUS\"{A B}", "\"MY BUS\"_1{A B}" },
        { "R{slash}W\\ BUS{A}", "R{slash}W\\ BUS_1{A}" },
        { "I^{2}C\\ BUS{A}", "I^{2}C\\ BUS_1{A}" },
        { "G{lt}H{A}", "G{lt}H_1{A}" },
        { "G{return}{A}", "G{return}_1{A}" },
        { "{dblquote}MY BUS{dblquote}{A}", "{dblquote}MY BUS{dblquote}_1{A}" },
        { "G{A}", "G_1{A}" }
    };

    for( const auto& [text, expected] : cases )
    {
        BOOST_TEST_CONTEXT( text )
        {
            const auto schema = BUS_SCHEMA::Parse( text );
            BOOST_REQUIRE( schema );
            BOOST_CHECK_EQUAL( ApplyNameSuffix( text, &*schema, 0 ), text );
            const wxString rendered = ApplyNameSuffix( text, &*schema, 1 );
            BOOST_CHECK_EQUAL( rendered, expected );
            const auto parsed = BUS_SCHEMA::Parse( rendered );
            BOOST_REQUIRE( parsed );
            BOOST_CHECK_EQUAL( parsed->prefix, schema->prefix + "_1" );
            BOOST_REQUIRE_EQUAL( parsed->leaves.size(), schema->leaves.size() );

            for( size_t i = 0; i < schema->leaves.size(); ++i )
            {
                const wxString name = schema->prefix + "_1"
                                      + schema->leaves[i].name.Mid( schema->prefix.length() );
                BOOST_CHECK_EQUAL( parsed->leaves[i].name, name );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
