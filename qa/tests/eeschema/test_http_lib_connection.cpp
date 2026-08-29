/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <chrono>
#include <thread>

#include <http_lib/http_lib_connection.h>
#include <http_lib/http_lib_settings.h>

#include "http_lib_fake_server.h"

BOOST_AUTO_TEST_SUITE( HttpLibConnection )


static const std::string RootUrl = "http://fake.test/v1/";


static HTTP_LIB_SOURCE makeTestSource()
{
    HTTP_LIB_SOURCE source;
    source.type = HTTP_LIB_SOURCE_TYPE::REST_API;
    source.root_url = RootUrl;
    return source;
}


static HTTP_LIB_CONNECTION connectToStandardLibrary( HTTP_LIB_FAKE_SERVER& aServer )
{
    HTTP_LIB_SOURCE source = makeTestSource();
    aServer.InstallStandardLibrary( source.root_url );

    return HTTP_LIB_CONNECTION( source, true, aServer.MakeRetriever() );
}


BOOST_AUTO_TEST_CASE( ValidEndpointSyncsCategories )
{
    HTTP_LIB_FAKE_SERVER server;
    HTTP_LIB_CONNECTION  conn = connectToStandardLibrary( server );

    BOOST_CHECK( conn.IsValidEndpoint() );

    BOOST_REQUIRE_EQUAL( conn.getCategories().size(), 2u );
    BOOST_CHECK_EQUAL( conn.getCategories()[0].name, "Resistors" );
    BOOST_CHECK_EQUAL( conn.getCategoryDescription( "Resistors" ), "Resistors" );
    BOOST_CHECK_EQUAL( conn.getCategoryDescription( "Capacitors" ), "Capacitors" );
}


BOOST_AUTO_TEST_CASE( EmptyResponseIsInvalid )
{
    HTTP_LIB_FAKE_SERVER server;
    server.SetResponse( RootUrl, { 200, "" } );

    HTTP_LIB_CONNECTION conn( makeTestSource(), true, server.MakeRetriever() );

    BOOST_CHECK( !conn.IsValidEndpoint() );
    BOOST_CHECK( conn.GetLastError().find( "empty response" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( Non200ResponseIsInvalid )
{
    HTTP_LIB_FAKE_SERVER server;
    server.SetResponse( RootUrl, { 500, "" } );

    HTTP_LIB_CONNECTION conn( makeTestSource(), true, server.MakeRetriever() );

    BOOST_CHECK( !conn.IsValidEndpoint() );
    BOOST_CHECK( conn.GetLastError().find( "error code" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( MalformedJsonIsInvalid )
{
    HTTP_LIB_FAKE_SERVER server;
    server.SetResponse( RootUrl, { 200, "not json" } );

    HTTP_LIB_CONNECTION conn( makeTestSource(), true, server.MakeRetriever() );

    BOOST_CHECK( !conn.IsValidEndpoint() );
    BOOST_CHECK( !conn.GetLastError().empty() );
}


BOOST_AUTO_TEST_CASE( TransportFailureIsInvalid )
{
    HTTP_LIB_CONNECTION conn(
            makeTestSource(), true,
            []( const std::string&, int& aStatusCode, std::string&, std::string& aError )
            {
                aStatusCode = 0;
                aError = "connection refused";
                return false;
            } );

    BOOST_CHECK( !conn.IsValidEndpoint() );
    BOOST_CHECK_EQUAL( conn.GetLastError(), "connection refused" );
}


BOOST_AUTO_TEST_CASE( SelectAllPopulatesPartsAndCache )
{
    HTTP_LIB_FAKE_SERVER server;
    HTTP_LIB_CONNECTION  conn = connectToStandardLibrary( server );

    HTTP_LIB_CATEGORY category;
    category.id = "1";

    std::vector<HTTP_LIB_PART> parts;

    BOOST_CHECK( conn.SelectAll( category, parts ) );
    BOOST_REQUIRE_EQUAL( parts.size(), 2u );
    BOOST_CHECK_EQUAL( parts[0].name, "R_10K" );
    BOOST_CHECK( !parts[0].detailsLoaded );

    std::string partId;
    std::string categoryId;
    BOOST_CHECK( conn.GetCachedPartRelation( "R_10K", partId, categoryId ) );
    BOOST_CHECK_EQUAL( partId, "res-10k" );
    BOOST_CHECK_EQUAL( categoryId, "1" );
}


BOOST_AUTO_TEST_CASE( SelectOneFetchesAndCaches )
{
    HTTP_LIB_FAKE_SERVER server;
    HTTP_LIB_CONNECTION  conn = connectToStandardLibrary( server );

    HTTP_LIB_PART part;

    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK_EQUAL( part.name, "R_10K" );
    BOOST_CHECK( part.detailsLoaded );

    bool foundFootprint = false;

    for( const auto& [name, properties] : part.fields )
    {
        if( name == "footprint" )
        {
            foundFootprint = true;
            BOOST_CHECK_EQUAL( std::get<0>( properties ), "Resistor_SMD:R_0603_1608Metric" );
        }
    }

    BOOST_CHECK( foundFootprint );

    // The second lookup must be served from the part cache without another request.
    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK_EQUAL( server.RequestCount( RootUrl + "parts/res-10k.json" ), 1 );
}


BOOST_AUTO_TEST_CASE( SelectOneRefetchesWhenCacheExpired )
{
    HTTP_LIB_FAKE_SERVER server;
    server.InstallStandardLibrary( RootUrl );

    HTTP_LIB_SOURCE source = makeTestSource();
    source.timeout_parts = 0;

    HTTP_LIB_CONNECTION conn( source, true, server.MakeRetriever() );

    HTTP_LIB_PART part;

    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );

    BOOST_CHECK_EQUAL( server.RequestCount( RootUrl + "parts/res-10k.json" ), 2 );
}


BOOST_AUTO_TEST_CASE( SelectOneReports404 )
{
    HTTP_LIB_FAKE_SERVER server;
    server.InstallStandardLibrary( RootUrl );
    server.SetResponse( RootUrl + "parts/missing.json", { 404, "" } );

    HTTP_LIB_CONNECTION conn( makeTestSource(), true, server.MakeRetriever() );

    HTTP_LIB_PART part;
    BOOST_CHECK( !conn.SelectOne( "missing", part ) );
    BOOST_CHECK( conn.GetLastError().find( "404" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( SelectAllRejectsNonJsonBody )
{
    HTTP_LIB_FAKE_SERVER server;
    server.InstallStandardLibrary( RootUrl );
    server.SetResponse( RootUrl + "parts/category/9.json", { 200, "not json" } );

    HTTP_LIB_CONNECTION conn( makeTestSource(), true, server.MakeRetriever() );

    HTTP_LIB_CATEGORY category;
    category.id = "9";

    std::vector<HTTP_LIB_PART> parts;
    BOOST_CHECK( !conn.SelectAll( category, parts ) );
}


BOOST_AUTO_TEST_CASE( SelectOneFetchCacheLifetime )
{
    HTTP_LIB_FAKE_SERVER server;
    server.InstallStandardLibrary( RootUrl );

    HTTP_LIB_SOURCE source = makeTestSource();
    source.timeout_parts = 1;

    HTTP_LIB_CONNECTION conn( source, true, server.MakeRetriever() );

    HTTP_LIB_PART part;

    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK_EQUAL( server.RequestCount( RootUrl + "parts/res-10k.json" ), 1 );

    // An immediate repeat must be served from cache.
    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK_EQUAL( server.RequestCount( RootUrl + "parts/res-10k.json" ), 1 );

    // Once the cache lifetime has elapsed the record must be re-fetched.
    std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );

    BOOST_CHECK( conn.SelectOne( "res-10k", part ) );
    BOOST_CHECK_EQUAL( server.RequestCount( RootUrl + "parts/res-10k.json" ), 2 );
}


BOOST_AUTO_TEST_SUITE_END()

