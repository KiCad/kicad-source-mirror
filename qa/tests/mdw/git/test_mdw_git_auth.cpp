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

#define BOOST_TEST_MODULE MdwGit
#include <boost/test/unit_test.hpp>

#include <mdw/mdw_backend.h>
#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_dirty_tracker.h>
#include <mdw/mdw_event_bus.h>
#include <mdw/mdw_lock_tracker.h>
#include <mdw/mdw_session.h>
#include <mdw/mdw_workspace_service.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_source.h>

#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_locking.pb.h>

#include <git2.h>

#include <qa_utils/env_var_utils.h>
#include <qa_utils/file_utils.h>

#include <wx/filename.h>
#include <wx/init.h>
#include <wx/utils.h>

#include "mdw_git_test_utils.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <thread>

namespace
{

struct PROCESS_TEMP_DIR_FIXTURE
{
    KI_TEST::SCOPED_PROCESS_TEMP_DIR m_dir{ "kicad_qa_mdw_git" };
};


// MDW_FRAMEWORK finds its backends through wxStandardPaths, which asks the app for its traits
struct WX_APP_FIXTURE
{
    wxInitializer m_wx;
};

} // namespace


BOOST_GLOBAL_FIXTURE( PROCESS_TEMP_DIR_FIXTURE );
BOOST_GLOBAL_FIXTURE( WX_APP_FIXTURE );
BOOST_GLOBAL_FIXTURE( GIT_HOST_ISOLATION );
BOOST_GLOBAL_FIXTURE( UI_EVENT_ISOLATION );


BOOST_AUTO_TEST_SUITE( MdwGitAuth )


// A backend that locks but cannot list its locks reports them only as events, so free items must not read as unknown
BOOST_AUTO_TEST_CASE( UnlistedLocksAreKnownFromEvents )
{
    SCOPED_DATA_DIR    dataDir;
    MDW_BACKEND_SERVER server;
    const std::string  url = "ipc://" + ( dataDir.Path() / "control" ).string();

    server.RegisterHandler<kiapi::mdw::Hello, kiapi::mdw::HelloReply>(
            []( const kiapi::mdw::Hello&, kiapi::mdw::HelloReply& aReply )
            {
                aReply.mutable_capabilities()->add_groups( kiapi::workspace::CG_LOCKING );
                return MdwOk();
            } );

    BOOST_REQUIRE( server.Start( url, 1 ) );

    auto backend = std::make_shared<MDW_BACKEND>( url, wxEmptyString );
    BOOST_REQUIRE( backend->Start() );
    BOOST_REQUIRE( backend->Handshake( kiapi::mdw::Hello(), 8000 ) );

    auto                  locks = std::make_shared<MDW_LOCK_TRACKER>();
    MDW_WORKSPACE_SERVICE service( backend, std::make_shared<MDW_EVENT_BUS>(), locks,
                                   std::make_shared<MDW_DIRTY_TRACKER>(), wxS( "project" ), wxS( "carol" ) );

    BOOST_CHECK( service.ListLocks().error == MDW_ERROR_CODE::NOT_SUPPORTED );
    BOOST_CHECK( locks->Get( wxS( "board.kicad_sch" ) ).known );
}


// An upload backend can refuse a stale check in only if it learns which revision the file was edited from
BOOST_AUTO_TEST_CASE( SourceCheckinSendsItsBase )
{
    MDW_BACKEND_MANIFEST manifest;
    MDW_BACKEND_CONTEXT  context;

    manifest.backend_id = wxS( "org.kicad.mdw.qa.realm" );
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );
    context.workspaceId = wxS( "realm" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    auto session = std::make_shared<MDW_SESSION>(
            MDW_PROJECT_BINDING{ manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );
    auto source = std::make_shared<MDW_SOURCE>( session );

    MDW_CHECKIN_REQUEST request;
    request.item_id = wxS( "1898/amp-board.kicad_sch" );
    request.local_path = wxS( "amp-board.kicad_sch" );
    request.base_revision_id = wxS( "7001" );

    auto [result, revision] = MdwSync<MDW_REVISION>(
            [&]( MDW_DONE<MDW_REVISION> aDone )
            {
                return source->Checkin( request, std::move( aDone ) );
            } );

    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( revision.label == wxS( "7001" ) );
}


// A reply that names no path wrote where the checkout asked, so the caller still learns the working copy
BOOST_AUTO_TEST_CASE( SourceCheckoutNamesItsPathWhenTheReplyOmitsIt )
{
    MDW_BACKEND_MANIFEST manifest;
    MDW_BACKEND_CONTEXT  context;

    manifest.backend_id = wxS( "org.kicad.mdw.qa.realm" );
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );
    context.workspaceId = wxS( "realm" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    auto session = std::make_shared<MDW_SESSION>(
            MDW_PROJECT_BINDING{ manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );
    auto source = std::make_shared<MDW_SOURCE>( session );

    MDW_CHECKOUT_REQUEST request;
    request.item_id = wxS( "1898/amp-board.kicad_sch" );
    request.local_path = wxS( "amp-board.kicad_sch" );

    auto [result, checkedOut] = MdwSync<MDW_CHECKOUT_RESULT>(
            [&]( MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
            {
                return source->Checkout( request, std::move( aDone ) );
            } );

    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( checkedOut.local_path == request.local_path );
}


// A local source sends no login form, so only the server check can sign its session in
BOOST_AUTO_TEST_CASE( MissingLocalSourceIsNotSignedIn )
{
    SCOPED_DATA_DIR     dataDir;
    MDW_BACKEND_CONTEXT library;
    library.workspaceId = wxS( "library" );
    library.locator = wxString::FromUTF8( ( dataDir.Path() / "missing" ).string() );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( gitManifest(), library );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );
    BOOST_REQUIRE( !supervisor->Backend()->AuthChallenge() );

    MDW_PROJECT_BINDING binding{ wxS( "org.kicad.mdw.git" ), library.workspaceId, library.locator };
    MDW_SESSION         session( binding, std::move( supervisor ) );

    BOOST_CHECK( session.Authenticate( MDW_TYPED_CREDENTIALS(), 8000 ).error == MDW_ERROR_CODE::NETWORK );
    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::OFFLINE );
}


// A backend too old to check its server names no auth form and stays stopped, whatever its other calls answer
BOOST_AUTO_TEST_CASE( StaleBackendStaysStopped )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR stale( wxS( "QA_MDW_STALE" ), wxS( "1" ) );
    SCOPED_DATA_DIR                 dataDir;
    MDW_BACKEND_MANIFEST            manifest;
    manifest.backend_id = realmBinding().backend_id;
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "stale" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_SESSION session( { manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );

    BOOST_CHECK( !session.Backend()->AuthChallenge() );
    BOOST_REQUIRE( session.Reconnect( 8000 ).error == MDW_ERROR_CODE::PROTOCOL );
    BOOST_REQUIRE( session.Status().phase == MDW_CONNECTION::BACKEND_STOPPED );

    const MDW_RESULT tooOld = session.Status().reason;

    kiapi::mdw::AuthenticateRequest request;
    kiapi::mdw::AuthenticateReply   reply;
    BOOST_REQUIRE( session.Backend()->CallSync( request, reply, 8000 ).error == MDW_ERROR_CODE::NETWORK );

    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::BACKEND_STOPPED );
    BOOST_CHECK( session.Status().reason.error == MDW_ERROR_CODE::PROTOCOL );
    BOOST_CHECK( session.Status().reason.message == tooOld.message );
}


// A call sent while the server was away must not hide a later check that finds it back, however late it fails
BOOST_AUTO_TEST_CASE( LateFailureOfEarlierCallLeavesCheckOnline )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  listing = dataDir.Path() / "listing";
    KI_TEST::SCOPED_PROCESS_ENV_VAR late( wxS( "QA_MDW_LATE_FAILURE" ), wxString::FromUTF8( listing.string() ) );
    MDW_BACKEND_MANIFEST            manifest;
    manifest.backend_id = realmBinding().backend_id;
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "late" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_SESSION session( { manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );

    MDW_TYPED_CREDENTIALS login;
    login.fields[wxS( "password" )] = wxS( "carol-pw" );

    BOOST_REQUIRE( session.Authenticate( login, 8000 ).IsOk() );

    // Shared, so a failed assertion below cannot leave the reply a destroyed promise to answer
    auto                         failed = std::make_shared<std::promise<MDW_ERROR_CODE>>();
    std::future<MDW_ERROR_CODE>  failure = failed->get_future();
    kiapi::mdw::ListLocksRequest request;

    session.Backend()->Call<kiapi::mdw::ListLocksRequest, kiapi::mdw::ListLocksReply>(
            request, 30000,
            [failed]( const MDW_RESULT& aResult, const kiapi::mdw::ListLocksReply& )
            {
                failed->set_value( aResult.error );
            } );

    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !fs::exists( listing ) && std::chrono::steady_clock::now() < until )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    BOOST_REQUIRE( fs::exists( listing ) );

    session.Probe( 8000 );
    BOOST_REQUIRE( failure.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );

    BOOST_CHECK( failure.get() == MDW_ERROR_CODE::NETWORK );
    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::ONLINE );
}



// A refusal of a call sent before the server check that found the login working is older news
BOOST_AUTO_TEST_CASE( LateRefusalOfEarlierCallLeavesCheckOnline )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  listing = dataDir.Path() / "listing";
    const fs::path                  release = dataDir.Path() / "release";
    KI_TEST::SCOPED_PROCESS_ENV_VAR late( wxS( "QA_MDW_LATE_FAILURE" ), wxString::FromUTF8( listing.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR refusal( wxS( "QA_MDW_LATE_REFUSAL" ), wxString::FromUTF8( release.string() ) );
    MDW_BACKEND_MANIFEST            manifest;
    manifest.backend_id = realmBinding().backend_id;
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "refused" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_SESSION session( { manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );

    MDW_TYPED_CREDENTIALS login;
    login.fields[wxS( "password" )] = wxS( "carol-pw" );

    BOOST_REQUIRE( session.Authenticate( login, 8000 ).IsOk() );

    auto                         refused = std::make_shared<std::promise<MDW_ERROR_CODE>>();
    std::future<MDW_ERROR_CODE>  refusalCode = refused->get_future();
    kiapi::mdw::ListLocksRequest request;

    session.Backend()->Call<kiapi::mdw::ListLocksRequest, kiapi::mdw::ListLocksReply>(
            request, 30000,
            [refused]( const MDW_RESULT& aResult, const kiapi::mdw::ListLocksReply& )
            {
                refused->set_value( aResult.error );
            } );

    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !fs::exists( listing ) && std::chrono::steady_clock::now() < until )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    BOOST_REQUIRE( fs::exists( listing ) );

    session.Probe( 8000 );
    BOOST_REQUIRE( session.Status().phase == MDW_CONNECTION::ONLINE );

    std::ofstream( release ) << "refuse";
    BOOST_REQUIRE( refusalCode.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );

    BOOST_CHECK( refusalCode.get() == MDW_ERROR_CODE::NOT_AUTHENTICATED );
    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::ONLINE );
}


// A refusal of a call sent after a server check began is newer than the check's success, so the prompt stays
BOOST_AUTO_TEST_CASE( LateSuccessOfEarlierCheckLeavesRefusalShown )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  listing = dataDir.Path() / "listing";
    const fs::path                  release = dataDir.Path() / "release";
    const fs::path                  hold = dataDir.Path() / "hold";
    KI_TEST::SCOPED_PROCESS_ENV_VAR late( wxS( "QA_MDW_LATE_FAILURE" ), wxString::FromUTF8( listing.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR refusal( wxS( "QA_MDW_LATE_REFUSAL" ), wxString::FromUTF8( release.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR held( wxS( "QA_MDW_CHECK_HOLD" ), wxString::FromUTF8( hold.string() ) );
    MDW_BACKEND_MANIFEST            manifest;
    manifest.backend_id = realmBinding().backend_id;
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "held" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_SESSION session( { manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );

    MDW_TYPED_CREDENTIALS login;
    login.fields[wxS( "password" )] = wxS( "carol-pw" );

    BOOST_REQUIRE( session.Authenticate( login, 8000 ).IsOk() );

    std::future<void> probed = std::async( std::launch::async,
                                           [&session]()
                                           {
                                               session.Probe( 8000 );
                                           } );

    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !fs::exists( hold ) && std::chrono::steady_clock::now() < until )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    BOOST_REQUIRE( fs::exists( hold ) );

    std::ofstream( release ) << "refuse";

    auto                         refused = std::make_shared<std::promise<MDW_ERROR_CODE>>();
    std::future<MDW_ERROR_CODE>  refusalCode = refused->get_future();
    kiapi::mdw::ListLocksRequest request;

    session.Backend()->Call<kiapi::mdw::ListLocksRequest, kiapi::mdw::ListLocksReply>(
            request, 30000,
            [refused]( const MDW_RESULT& aResult, const kiapi::mdw::ListLocksReply& )
            {
                refused->set_value( aResult.error );
            } );

    BOOST_REQUIRE( refusalCode.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );
    BOOST_REQUIRE( refusalCode.get() == MDW_ERROR_CODE::NOT_AUTHENTICATED );

    fs::remove( hold );
    BOOST_REQUIRE( probed.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );

    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::SIGN_IN_NEEDED );
}


// A probe's login is newer than a call sent after its check, so that call's failure must not hide the login
BOOST_AUTO_TEST_CASE( FailureSentBeforeProbeLoginLeavesLoginOnline )
{
    SCOPED_DATA_DIR                 dataDir;
    const fs::path                  checking = dataDir.Path() / "checking";
    KI_TEST::SCOPED_PROCESS_ENV_VAR race( wxS( "QA_MDW_RELOGIN_RACE" ), wxString::FromUTF8( checking.string() ) );
    MDW_BACKEND_MANIFEST            manifest;
    manifest.backend_id = realmBinding().backend_id;
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "relogin" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_SESSION       session( { manifest.backend_id, context.workspaceId, wxEmptyString }, std::move( supervisor ) );
    std::atomic<bool> stored = false;

    session.SetCredentialSource(
            [&stored]()
            {
                MDW_TYPED_CREDENTIALS login;

                if( stored )
                    login.fields[wxS( "password" )] = wxS( "carol-pw" );

                return login;
            } );

    session.Reconnect( 8000 );
    BOOST_REQUIRE( session.Status().phase == MDW_CONNECTION::SIGN_IN_NEEDED );

    stored = true;
    BOOST_REQUIRE( session.StoredLoginChanged() );

    std::future<void> probed = std::async( std::launch::async,
                                           [&session]()
                                           {
                                               session.Probe( 8000 );
                                           } );

    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !fs::exists( checking ) && std::chrono::steady_clock::now() < until )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    BOOST_REQUIRE( fs::exists( checking ) );

    auto                         failed = std::make_shared<std::promise<MDW_ERROR_CODE>>();
    std::future<MDW_ERROR_CODE>  failure = failed->get_future();
    kiapi::mdw::ListLocksRequest request;

    session.Backend()->Call<kiapi::mdw::ListLocksRequest, kiapi::mdw::ListLocksReply>(
            request, 30000,
            [failed]( const MDW_RESULT& aResult, const kiapi::mdw::ListLocksReply& )
            {
                failed->set_value( aResult.error );
            } );

    BOOST_REQUIRE( probed.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );
    BOOST_REQUIRE( failure.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready );

    BOOST_CHECK( failure.get() == MDW_ERROR_CODE::NETWORK );
    BOOST_CHECK( session.Status().phase == MDW_CONNECTION::ONLINE );
}


BOOST_AUTO_TEST_SUITE_END()
