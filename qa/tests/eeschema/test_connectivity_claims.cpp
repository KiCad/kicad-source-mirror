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
#include <connectivity/conn_claims.h>

#include <algorithm>
#include <vector>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityClaims )

BOOST_AUTO_TEST_CASE( AliasContainmentCannotCreateComparatorCycles )
{
    SESSION_KEYS       keys;
    const INST_ID      instance = keys.InternInstance( KIID_PATH() );
    std::vector<CLAIM> claims;
    const BUS_ALIASES  aliases{ { "A", { "D0" } }, { "B", { "D1" } }, { "C", { "D0", "D2" } } };

    for( const wxString& name : { wxString( "{A}" ), wxString( "{B}" ), wxString( "{C}" ) } )
    {
        CLAIM claim;
        claim.priority = PRIORITY::LOCAL_LABEL;
        claim.source = { niluuid, instance };
        claim.path = keys.InternName( wxString() );
        claim.name = claim.ncName = claim.fullName = keys.InternName( name );
        claim.schema = std::make_shared<const BUS_SCHEMA>( *BUS_SCHEMA::Parse( name, aliases ) );
        claims.push_back( std::move( claim ) );
    }

    const CLAIM_LESS less{ keys };
    BOOST_CHECK( less( claims[1], claims[0] ) );
    BOOST_CHECK( less( claims[2], claims[0] ) );
    BOOST_CHECK( less( claims[2], claims[1] ) );
    const std::vector<NAME_ID> expected{ claims[0].name, claims[1].name, claims[2].name };
    std::vector<size_t>        order{ 0, 1, 2 };

    do
    {
        std::vector<CLAIM> permuted;

        for( size_t i : order )
            permuted.push_back( claims[i] );

        std::sort( permuted.begin(), permuted.end(),
                   [&]( const CLAIM& a, const CLAIM& b )
                   {
                       return less( b, a );
                   } );

        for( size_t i = 0; i < expected.size(); ++i )
            BOOST_CHECK_EQUAL( permuted[i].name, expected[i] );
    } while( std::next_permutation( order.begin(), order.end() ) );

    claims[0].depth = 1;
    claims[2].depth = 2;
    BOOST_CHECK( less( claims[2], claims[0] ) );
    claims[2].priority = PRIORITY::GLOBAL;
    BOOST_CHECK( less( claims[0], claims[2] ) );
}

BOOST_AUTO_TEST_SUITE_END()
