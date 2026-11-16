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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/kigit_failure.h>

BOOST_AUTO_TEST_SUITE( KigitFailure )


BOOST_AUTO_TEST_CASE( LocalNetworkHostRanges )
{
    for( const wxString host : { "192.168.1.26", "10.0.0.1", "172.16.0.9", "172.31.255.1", "169.254.7.7", "fd12:3456::1",
                                 "fc00::2", "fe80::1%en0", "febf::1", "nas.local", "NAS.Local" } )
    {
        BOOST_CHECK_MESSAGE( KIGIT::IsLocalNetworkHost( host ), host << " is on the local network" );
    }

    for( const wxString host : { "gitlab.com", "8.8.8.8", "172.15.0.1", "172.32.0.1", "192.169.1.1", "169.253.0.1",
                                 "2001:db8::1", "fec0::1", "::1", "local", "10.0.0", "10.0.0.1.5", "300.1.1.1", "" } )
    {
        BOOST_CHECK_MESSAGE( !KIGIT::IsLocalNetworkHost( host ), host << " is not on the local network" );
    }
}


#ifdef __APPLE__
static wxString unreachableSummary( const wxString& aUrl )
{
    KIGIT_ERROR_INFO error;
    error.message = wxS( "failed to connect to host: Connection refused" );

    KIGIT_FAILURE_CONTEXT context;
    context.url = aUrl;

    const KIGIT_FAILURE_REPORT report = KIGIT::DescribeGitFailure( error, {}, context );

    BOOST_REQUIRE( report.kind == KIGIT_FAILURE::HOST_UNREACHABLE );
    return report.summary;
}


BOOST_AUTO_TEST_CASE( PrivateHostUnreachableOnMacNamesLocalNetworkSetting )
{
    BOOST_CHECK(
            unreachableSummary( wxS( "http://192.168.1.26:3000/team/repo.git" ) ).Contains( wxS( "Local Network" ) ) );
    BOOST_CHECK( unreachableSummary( wxS( "ssh://git@nas.local/repo.git" ) ).Contains( wxS( "Local Network" ) ) );
    BOOST_CHECK( !unreachableSummary( wxS( "https://gitlab.com/team/repo.git" ) ).Contains( wxS( "Local Network" ) ) );
}
#endif


BOOST_AUTO_TEST_SUITE_END()
