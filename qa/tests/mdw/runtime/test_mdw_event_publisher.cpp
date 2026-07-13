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

#include <mdw_runtime/mdw_event_publisher.h>

#include <api/mdw/mdw_envelope.pb.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>

#include <cstdint>
#include <string>
#include <vector>


namespace
{
const std::string TOKEN = "phases";


void report( MDW_EVENT_PUBLISHER& aPublisher, std::uint64_t aDone, std::uint64_t aTotal, const std::string& aPhase )
{
    kiapi::workspace::EventEnvelope event;
    event.set_type( kiapi::workspace::ET_PROGRESS );
    event.mutable_progress()->set_done( aDone );
    event.mutable_progress()->set_total( aTotal );
    event.mutable_progress()->set_phase( aPhase );
    BOOST_REQUIRE( aPublisher.Publish( event, "op" ) );
}


/// A client ingress that waits aTimeoutMs for each report
nng_socket openIngress( const std::string& aUrl, int aTimeoutMs )
{
    nng_socket client = NNG_SOCKET_INITIALIZER;
    BOOST_REQUIRE_EQUAL( nng_rep0_open( &client ), 0 );
    BOOST_REQUIRE_EQUAL( nng_socket_set_ms( client, NNG_OPT_RECVTIMEO, aTimeoutMs ), 0 );
    BOOST_REQUIRE_EQUAL( nng_listen( client, aUrl.c_str(), nullptr, 0 ), 0 );
    return client;
}


/// The next report as "phase done/total", acknowledged when aAnswer is set
std::string receive( nng_socket aClient, bool aAnswer = true )
{
    void*  message = nullptr;
    size_t size = 0;

    BOOST_REQUIRE_EQUAL( nng_recv( aClient, &message, &size, NNG_FLAG_ALLOC ), 0 );

    kiapi::mdw::PublishEvent publish;
    BOOST_REQUIRE( publish.ParseFromArray( message, static_cast<int>( size ) ) );
    nng_free( message, size );

    if( aAnswer )
    {
        kiapi::mdw::PublishEventAck ack;
        ack.set_channel_token( TOKEN );
        ack.set_seq( publish.event().seq() );

        std::string reply = ack.SerializeAsString();
        BOOST_REQUIRE_EQUAL( nng_send( aClient, reply.data(), reply.size(), 0 ), 0 );
    }

    const kiapi::workspace::ProgressNotice& progress = publish.event().progress();
    return progress.phase() + " " + std::to_string( progress.done() ) + "/" + std::to_string( progress.total() );
}
} // namespace


BOOST_AUTO_TEST_SUITE( MdwEventPublisher )


// Reports queued while the client is away keep the last of each phase, so it sees where fetch and checkout ended
BOOST_AUTO_TEST_CASE( SlowClientSeesEachPhaseEnd )
{
    const std::string   url = "inproc://mdw-publisher-phases";
    MDW_EVENT_PUBLISHER publisher( TOKEN );

    BOOST_REQUIRE( publisher.Start( url ) );

    // Nothing listens yet, so every report after the first waits in the queue
    report( publisher, 0, 0, "receive" );
    report( publisher, 4, 8, "receive" );
    report( publisher, 8, 8, "receive" );
    report( publisher, 2, 2, "checkout" );
    report( publisher, 0, 0, "index" );

    nng_socket               client = openIngress( url, 10000 );
    std::vector<std::string> seen;

    while( seen.empty() || !seen.back().starts_with( "index" ) )
        seen.push_back( receive( client ) );

    publisher.Stop();
    nng_close( client );

    // Only the report in flight when the client came may precede the last of each phase
    const std::vector<std::string> ends = { "receive 8/8", "checkout 2/2", "index 0/0" };

    BOOST_REQUIRE_GE( seen.size(), ends.size() );
    BOOST_CHECK_LE( seen.size(), ends.size() + 1 );
    BOOST_CHECK( seen.front().starts_with( "receive" ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( seen.end() - ends.size(), seen.end(), ends.begin(), ends.end() );
}


// A phase's last report that times out on a busy client is sent again, since nothing newer replaces it
BOOST_AUTO_TEST_CASE( UnansweredLastReportIsTriedAgain )
{
    const std::string   url = "inproc://mdw-publisher-retry";
    nng_socket          client = openIngress( url, 5000 );
    MDW_EVENT_PUBLISHER publisher( TOKEN );

    BOOST_REQUIRE( publisher.Start( url ) );
    report( publisher, 2, 2, "checkout" );

    BOOST_CHECK_EQUAL( receive( client, false ), "checkout 2/2" );
    BOOST_CHECK_EQUAL( receive( client, false ), "checkout 2/2" );

    // A third copy would come about 2.2 s after the second, so the retry stops after one
    void*  message = nullptr;
    size_t size = 0;

    BOOST_REQUIRE_EQUAL( nng_socket_set_ms( client, NNG_OPT_RECVTIMEO, 3500 ), 0 );
    BOOST_CHECK_EQUAL( nng_recv( client, &message, &size, NNG_FLAG_ALLOC ), NNG_ETIMEDOUT );

    publisher.Stop();
    nng_close( client );
}


BOOST_AUTO_TEST_SUITE_END()
