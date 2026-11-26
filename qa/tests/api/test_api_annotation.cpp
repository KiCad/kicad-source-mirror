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

#include <boost/test/unit_test.hpp>

#include <api/annotation.h>
#include <api/api_handler.h>
#include <api/internal/annotation.pb.h>

BOOST_AUTO_TEST_SUITE( ApiAnnotation )

BOOST_AUTO_TEST_CASE( InternalGenerateNetlistRoundTrips )
{
    kiapi::internal::GenerateNetlistRequest request;
    request.mutable_document()->set_type( kiapi::common::types::DOCTYPE_SCHEMATIC );

    kiapi::internal::GenerateNetlistResponse response;
    response.set_success( true );
    response.set_netlist( "(export)" );
    response.set_report( "ok" );

    kiapi::internal::GenerateNetlistRequest parsedRequest;
    BOOST_REQUIRE( parsedRequest.ParseFromString( request.SerializeAsString() ) );
    BOOST_REQUIRE_EQUAL( parsedRequest.document().type(),
                         kiapi::common::types::DOCTYPE_SCHEMATIC );

    kiapi::internal::GenerateNetlistResponse parsedResponse;
    BOOST_REQUIRE( parsedResponse.ParseFromString( response.SerializeAsString() ) );
    BOOST_REQUIRE( parsedResponse.success() );
    BOOST_REQUIRE_EQUAL( parsedResponse.netlist(), "(export)" );
}

BOOST_AUTO_TEST_CASE( InternalRequestHandlerDefaultsToUnhandled )
{
    API_HANDLER handler;
    std::string response;

    BOOST_REQUIRE( !handler.HandleInternalApiRequest( "not-a-request", response ) );
    BOOST_REQUIRE( response.empty() );
}

BOOST_AUTO_TEST_SUITE_END()
