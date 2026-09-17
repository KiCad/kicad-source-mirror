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

#include <mdw/client/mdw_library_sources.h>
#include <mdw/mdw_framework.h>

#include <qa_utils/env_var_utils.h>

#include "mdw_git_test_utils.h"

#include <json_common.h>

#include <wx/process.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <thread>


BOOST_AUTO_TEST_SUITE( MdwFrameworkLifecycle )


// Turning integration off stops every backend, even one a leaseholder is still using
BOOST_AUTO_TEST_CASE( DisablingIntegrationStopsBackends )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  dir = dataDir.Path();
    KI_TEST::SCOPED_PROCESS_ENV_VAR pidFile( wxS( "QA_MDW_PID_FILE" ), wxString::FromUTF8( ( dir / "pid" ).string() ) );
    SCOPED_PGM                      program;

    Pgm().SetMdwEnabled( true );
    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();
    BOOST_REQUIRE( framework );

    offerRealmBackend( *framework, dataDir );

    BOOST_REQUIRE( framework->OpenSession( wxString::FromUTF8( ( dir / "board.kicad_pro" ).string() ), realmBinding(),
                                           8000 ) );

    long pid = 0;
    std::ifstream( dir / "pid" ) >> pid;
    BOOST_REQUIRE( pid > 0 && wxProcess::Exists( pid ) );

    // Returned only once the backend is gone, so a stop that drains leases before stopping backends never sees it
    std::atomic<bool> stoppedWhileLeased = false;
    std::thread       holder(
            [&stoppedWhileLeased, pid, lease = Pgm().LeaseMdwFramework()]()
            {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

                while( wxProcess::Exists( pid ) && std::chrono::steady_clock::now() < deadline )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

                stoppedWhileLeased = !wxProcess::Exists( pid );
            } );

    Pgm().SetMdwEnabled( false );
    holder.join();

    BOOST_CHECK( !Pgm().MdwFrameworkOrNull() );
    BOOST_CHECK( stoppedWhileLeased );

    // Later tests need the client workers, which only a new framework brings back
    Pgm().SetMdwEnabled( true );
    BOOST_CHECK( Pgm().MdwFrameworkOrNull() );
}


namespace
{

// Disabling integration must interrupt the hanging call, whose own timeout is 40 s
// A stopped backend's exit takes a few seconds
void disablingEndsAHangingCall( const std::function<void( MDW_FRAMEWORK&, const wxString&, const fs::path& )>& aHang )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  dir = dataDir.Path();
    const wxString                  projectPath = wxString::FromUTF8( ( dir / "board.kicad_pro" ).string() );
    KI_TEST::SCOPED_PROCESS_ENV_VAR hang( wxS( "QA_MDW_HANG_FILE" ), wxString::FromUTF8( ( dir / "hang" ).string() ) );
    SCOPED_PGM                      program;

    Pgm().SetMdwEnabled( true );
    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();
    BOOST_REQUIRE( framework );

    offerRealmBackend( *framework, dataDir );
    aHang( *framework, projectPath, dir );

    const auto reached = std::chrono::steady_clock::now() + std::chrono::seconds( 20 );

    while( !fs::exists( dir / "hang" ) && std::chrono::steady_clock::now() < reached )
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

    BOOST_REQUIRE( fs::exists( dir / "hang" ) );

    const auto start = std::chrono::steady_clock::now();

    Pgm().SetMdwEnabled( false );

    BOOST_CHECK_LT( std::chrono::duration_cast<std::chrono::seconds>( std::chrono::steady_clock::now() - start )
                            .count(),
                    20 );

    Pgm().SetMdwEnabled( true );
}

} // namespace


// Turning integration off while a login waits on a server that never answers must not wait the login out
BOOST_AUTO_TEST_CASE( DisablingIntegrationEndsLoginInFlight )
{
    disablingEndsAHangingCall(
            []( MDW_FRAMEWORK& aFramework, const wxString& aProjectPath, const fs::path& )
            {
                aFramework.EnsureSessionAsync( aProjectPath, realmBinding(), 40000 );
            } );
}


// An unreachable server is probed back to back, and turning integration off must not wait a probe out
BOOST_AUTO_TEST_CASE( DisablingIntegrationEndsProbeInFlight )
{
    disablingEndsAHangingCall(
            []( MDW_FRAMEWORK& aFramework, const wxString& aProjectPath, const fs::path& aDir )
            {
                // The login times out, which leaves the session offline and due a probe
                BOOST_REQUIRE( aFramework.OpenSession( aProjectPath, realmBinding(), 1000 ) );

                fs::remove( aDir / "hang" );
                aFramework.WatchBackends( 50, 40000 );
            } );
}


// A Reconnect asked for while offline waits on the server, and turning integration off must not wait it out
BOOST_AUTO_TEST_CASE( DisablingIntegrationEndsReconnectInFlight )
{
    disablingEndsAHangingCall(
            []( MDW_FRAMEWORK& aFramework, const wxString& aProjectPath, const fs::path& aDir )
            {
                // The login times out, and no probe runs, so only the Reconnect reaches the server
                BOOST_REQUIRE( aFramework.OpenSession( aProjectPath, realmBinding(), 1000 ) );

                aFramework.WatchBackends( 0, 0 );
                fs::remove( aDir / "hang" );
                aFramework.ReconnectAsync( aProjectPath, MDW_CONNECTION_ANSWER(), 40000 );
            } );
}


// Closing a session, as a binding change or Disconnect here does, must not start the framework the user turned off
BOOST_AUTO_TEST_CASE( ClosingASessionLeavesIntegrationOff )
{
    SCOPED_DATA_DIR dataDir;
    SCOPED_PGM      program;

    BOOST_REQUIRE( !Pgm().MdwEnabled() );

    MDW_SETUP::CloseProjectSession( wxString::FromUTF8(
            ( dataDir.Path() / "board.kicad_pro" ).string() ) );

    BOOST_CHECK( !Pgm().LeaseMdwFramework( false ) );

    Pgm().SetMdwEnabled( true );
}


// A standalone editor hosts no framework, yet must still guard the team libraries of a user who turned it on
BOOST_AUTO_TEST_CASE( StandaloneEditorsKeepTeamLibraryGuards )
{
    SCOPED_PGM program( false );

    Pgm().SetMdwEnabled( true );

    BOOST_CHECK( !Pgm().MdwFrameworkOrNull() );
    BOOST_CHECK( MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() );
}


BOOST_AUTO_TEST_SUITE_END()
