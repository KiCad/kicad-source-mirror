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

#define BOOST_TEST_MODULE MdwRuntime
#include <boost/test/unit_test.hpp>

#include <mdw_runtime/mdw_backend_server.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>

#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>


BOOST_AUTO_TEST_SUITE( MdwBackendServer )


// A worker's call on its context racing the socket's close must not panic nng_ctx_close. Workers that answer at
// once spend nearly all their time in nng, which is where the stop has to land
BOOST_AUTO_TEST_CASE( StopDuringReplyIsSafe )
{
    kiapi::mdw::MdwRequest request;
    BOOST_REQUIRE( request.mutable_payload()->PackFrom( kiapi::mdw::PingRequest() ) );

    const std::string                  wire = request.SerializeAsString();
    std::mt19937                       random( 2026 );
    std::uniform_int_distribution<int> stopAfterUs( 0, 3000 );
    std::atomic<long>                  replies = 0;

    for( int i = 0; i < 200; ++i )
    {
        const std::string  url = "inproc://mdw-stop-during-reply-" + std::to_string( i );
        MDW_BACKEND_SERVER server;

        server.RegisterHandler<kiapi::mdw::PingRequest, kiapi::mdw::PingReply>(
                []( const kiapi::mdw::PingRequest&, kiapi::mdw::PingReply& )
                {
                    return MdwOk();
                } );

        BOOST_REQUIRE( server.Start( url, 8 ) );

        std::atomic<bool>        sending = true;
        std::vector<nng_socket>  clients( 8, NNG_SOCKET_INITIALIZER );
        std::vector<std::thread> senders;

        for( nng_socket& client : clients )
        {
            BOOST_REQUIRE_EQUAL( nng_req0_open( &client ), 0 );
            nng_socket_set_ms( client, NNG_OPT_SENDTIMEO, 100 );
            nng_socket_set_ms( client, NNG_OPT_RECVTIMEO, 100 );
            BOOST_REQUIRE_EQUAL( nng_dial( client, url.c_str(), nullptr, 0 ), 0 );

            senders.emplace_back(
                    [&sending, &replies, &wire, client]()
                    {
                        while( sending.load() )
                        {
                            void*  reply = nullptr;
                            size_t size = 0;

                            if( nng_send( client, const_cast<char*>( wire.data() ), wire.size(), 0 ) == 0
                                && nng_recv( client, &reply, &size, NNG_FLAG_ALLOC ) == 0 )
                            {
                                nng_free( reply, size );
                                ++replies;
                            }
                        }
                    } );
        }

        std::this_thread::sleep_for( std::chrono::microseconds( stopAfterUs( random ) ) );
        server.Stop();

        sending = false;

        for( std::thread& sender : senders )
            sender.join();

        for( nng_socket client : clients )
            nng_close( client );
    }

    // Workers left idle would make every stop trivially safe
    BOOST_CHECK_GT( replies.load(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
