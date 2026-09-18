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

#include "mdw_git_library_fixture.h"
#include "mdw_git_test_utils.h"

#include <mdw_git_remote.h>

#include <mdw/client/mdw_op.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/mdw_part_state.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_lifecycle.pb.h>
#include <api/mdw/backend_revisions.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;


namespace
{

std::map<std::string, ws::Permission> listedCheckIns( MDW_BACKEND& aBackend, const fs::path& aCopy )
{
    mdw::ListLocalChangesRequest request;
    mdw::ListLocalChangesReply   reply;
    request.set_local_root( aCopy.string() );

    MDW_RESULT result = aBackend.CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::map<std::string, ws::Permission> rows;

    for( const mdw::LocalChange& change : reply.changes() )
        rows[change.member().item_id()] = change.checkin();

    return rows;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitPermissions )


// Any SSH stderr classifies as a server refusal, so a transient one must not lock the library
BOOST_AUTO_TEST_CASE( OnlyForbiddenPushesAreRecorded )
{
    KIGIT_ERROR_INFO      error;
    KIGIT_AUTH_ATTEMPT    auth;
    KIGIT_FAILURE_CONTEXT context;

    error.code = GIT_ERROR;
    context.operation = KIGIT_OPERATION::PUSH;
    context.pushRefusal = wxS( "refs/heads/main: rate limit exceeded" );

    const KIGIT_FAILURE refused = KIGIT::DescribeGitFailure( error, auth, context ).kind;

    BOOST_REQUIRE( refused == KIGIT_FAILURE::SERVER_REFUSED );
    BOOST_CHECK( !MDW_GIT_REMOTE::RecordsDenial( refused ) );
    BOOST_CHECK( MDW_GIT_REMOTE::RecordsDenial( KIGIT_FAILURE::PUSH_FORBIDDEN ) );
}


// An SSH sign-in has no realm for the host to word it from, so the classifier's advice is the headline
BOOST_AUTO_TEST_CASE( SshSignInKeepsTheClassifiersAdvice )
{
    MDW_GIT_REMOTE        remote;
    MDW_GIT_FAILURE_FACTS facts;
    KIGIT_ERROR_INFO      error;

    facts.ssh = true;
    facts.attempt.connType = KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH;
    error.code = GIT_EAUTH;

    ws::WorkspaceError noKey = remote.Describe( error, facts, "ssh://git@git.example.com/acme/board.git" );

    BOOST_CHECK_EQUAL( noKey.code(), ws::WE_NOT_AUTHENTICATED );
    BOOST_CHECK_MESSAGE( noKey.message().find( ".pub" ) != std::string::npos, noKey.message() );

    error.code = GIT_ERROR;
    error.message = wxS( "unable to open public key file" );
    facts.attempt.keysTried = { wxS( "id_ed25519" ) };

    ws::WorkspaceError badKey = remote.Describe( error, facts, "ssh://git@git.example.com/acme/board.git" );

    BOOST_CHECK_MESSAGE( badKey.message().find( "private key file" ) != std::string::npos, badKey.message() );

    // An HTTPS server is named by the host from its realm and the user
    facts = MDW_GIT_FAILURE_FACTS();
    facts.attempt.passwordGiven = true;
    error.code = GIT_EAUTH;
    error.message = wxEmptyString;

    ws::WorkspaceError refused = remote.Describe( error, facts, "https://git.example.com/acme/board.git" );

    BOOST_CHECK( refused.message().empty() );
    BOOST_CHECK_EQUAL( refused.auth_challenge().realm(), "https://git.example.com" );
}


// A headline that sends the user to the details must come with some
BOOST_FIXTURE_TEST_CASE( UnknownRemoteIsExplainedInTheDetails, ONE_LIBRARIAN )
{
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, m_alice.Copy().string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    MDW_GIT_REMOTE remote;

    ws::WorkspaceError error = remote.Transfer( repo, "no-such-remote",
                                                []( KIGIT::REMOTE_TRANSPORT& )
                                                {
                                                    return GIT_OK;
                                                } );

    BOOST_CHECK_EQUAL( error.code(), ws::WE_NETWORK );
    BOOST_CHECK_MESSAGE( error.message().find( "no-such-remote" ) != std::string::npos, error.message() );
    BOOST_CHECK_MESSAGE( error.detail().find( "no-such-remote" ) != std::string::npos, error.detail() );
}


// L was never released, so it is checked in. C was, so it is published by revising and releasing
BOOST_FIXTURE_TEST_CASE( CheckInPermissionFollowsReleaseState, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    edit( file, C_FILTERS, "C? C_????_*" );
    MDW_RESULT released = release( *m_alice.backend, file, C, "v1:capacitor" );
    BOOST_REQUIRE_MESSAGE( released.IsOk(), released.message );

    edit( file, "C? C_????_*", "C? Capacitor*" );
    edit( file, L_KEYWORDS, "inductor choke" );

    std::map<std::string, ws::Permission> rows = listedCheckIns( *m_alice.backend, m_alice.Copy() );

    BOOST_REQUIRE_EQUAL( rows.size(), 2u );
    BOOST_CHECK( rows[L].state() == ws::PS_ALLOWED );
    BOOST_CHECK( rows[C].state() == ws::PS_DENIED );
    BOOST_CHECK_EQUAL( rows[C].reason(), "Released; Revise it, then Release" );

    // The backend refuses what it listed as denied, whatever the client sends
    BOOST_CHECK_EQUAL( tryCheckIn( m_alice, C, "alice-direct" ).code(), ws::WE_PERMISSION_DENIED );

    // The claim is how a released part is revised
    claim( m_alice, C );

    rows = listedCheckIns( *m_alice.backend, m_alice.Copy() );
    BOOST_CHECK( rows[C].state() == ws::PS_ALLOWED );

    // The claim check loads claims itself, as a check-in can be the first call after a backend restart
    Reconnect( m_alice );
    BOOST_CHECK_EQUAL( tryCheckIn( m_alice, C, "alice-claimed" ).code(), ws::WE_OK );
}


// A part saved but not checked in has a check-in target, and a never released one offers no state change
BOOST_FIXTURE_TEST_CASE( ListedTargetsOfferWhatTheListingSupports, ONE_LIBRARIAN )
{
    const MDW_TEAM_LIBRARY binding{ m_alice.source, wxString::FromUTF8( LIBRARY ), wxString(),
                                    wxString( m_alice.Copy().string() ) };

    auto [read, page] = MdwSync<MDW_PAGED<MDW_LIBRARY_MEMBER>>(
            [&]( MDW_DONE<MDW_PAGED<MDW_LIBRARY_MEMBER>> aDone )
            {
                return m_alice.source->Libraries().Members( binding.library_id, binding.snapshot, MDW_PAGE(),
                                                             std::move( aDone ) );
            } );

    BOOST_REQUIRE_MESSAGE( read.IsOk(), read.message );

    std::map<wxString, MDW_MEMBER_ENTRY> members;

    for( const MDW_LIBRARY_MEMBER& member : page.rows )
        members[member.name] = MDW_MEMBER_ENTRY{ member, MDW_LOCK_INFO() };

    BOOST_REQUIRE( members.count( wxS( "C" ) ) );

    // C is listed but never released, so it has no state to leave
    MDW_PART_TARGET listed = MdwPartTarget( binding, members, wxS( "C" ), wxString(), wxString() );

    BOOST_CHECK( listed.listed );
    BOOST_CHECK( std::ranges::none_of( listed.ListedActions(),
                                       []( const MDW_PART_ACTION& aAction )
                                       {
                                           return aAction.id == MDW_PART_ACTION::ID::TRANSITION;
                                       } ) );

    MDW_PART_TARGET unlisted = MdwPartTarget( binding, members, wxS( "C_Small" ), wxString(), wxString() );

    BOOST_CHECK( !unlisted.listed );
    BOOST_CHECK_EQUAL( unlisted.item.source, m_alice.source->Info().locator );
    BOOST_CHECK( std::ranges::any_of( unlisted.ListedActions(),
                                      []( const MDW_PART_ACTION& aAction )
                                      {
                                          return aAction.id == MDW_PART_ACTION::ID::CHECK_IN && aAction.Allowed();
                                      } ) );
}


// A push whose lease broke names the changes to fetch, as any other stale push does
BOOST_FIXTURE_TEST_CASE( LeaseBrokenPushNamesGetChanges, ONE_LIBRARIAN )
{
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, m_alice.Copy().string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    GIT_REF_UPDATE update;
    update.localRef = wxS( "refs/heads/main" );
    update.remoteRef = wxS( "refs/heads/main" );
    update.leased = true;
    update.expectedOld = wxString( std::string( 40, 'a' ) );

    MDW_GIT_REMOTE              remote;
    std::vector<GIT_REF_RESULT> results;
    ws::WorkspaceError          error;

    BOOST_REQUIRE( remote.Push( repo, "origin", { update }, results, error ) == RefPushResult::LeaseBroken );
    BOOST_CHECK_EQUAL( error.code(), ws::WE_VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( error.message().find( "Get changes first" ) != std::string::npos, error.message() );
}


BOOST_AUTO_TEST_SUITE_END()
