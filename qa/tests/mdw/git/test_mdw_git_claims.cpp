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

#include "mdw_git_checkouts.h"
#include "mdw_git_remote.h"
#include "mdw_git_repo.h"
#include "mdw_git_test_utils.h"

#include <qa_utils/git_test_fixture.h>

#include <git2.h>
#include <json_common.h>

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
namespace ws = kiapi::workspace;

using RESULT = MDW_GIT_CHECKOUTS::RESULT;


namespace
{

// One clone with a bare origin, opened by each backend sharing it through a handle of its own
class SHARED_CLONE_FIXTURE : public KI_TEST::GIT_TEST_FIXTURE
{
public:
    SHARED_CLONE_FIXTURE()
    {
        const fs::path temp( TempPath().utf8_string() );
        const fs::path origin = temp / "origin.git";
        const fs::path tree = temp / "work";

        git_repository* bare = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_init( &bare, origin.string().c_str(), 1 ), GIT_OK );
        git_repository_free( bare );

        m_repo = KI_TEST::InitGitRepository( wxString::FromUTF8( tree.string() ), "main", "carol", "carol@acme.test" );

        git_remote* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_remote_create( &remote, m_repo.get(), "origin", origin.string().c_str() ), GIT_OK );
        git_remote_free( remote );

        m_other = Open( tree );
        m_poll = Open( tree );
    }

    static KIGIT::GitRepositoryPtr Open( const fs::path& aPath )
    {
        git_repository* repo = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &repo, aPath.string().c_str() ), GIT_OK );
        return KIGIT::GitRepositoryPtr( repo );
    }

    std::vector<std::string> Refs()
    {
        std::vector<std::string> names;
        git_strarray             list{};

        BOOST_REQUIRE_EQUAL( git_reference_list( &list, m_repo.get() ), GIT_OK );
        names.assign( list.strings, list.strings + list.count );
        git_strarray_dispose( &list );
        return names;
    }

    KIGIT::GitRepositoryPtr m_repo;
    KIGIT::GitRepositoryPtr m_other;
    KIGIT::GitRepositoryPtr m_poll;
};


} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitClaims )


// Another backend's pass on the same clone landing inside this one's must not drop the claims this one tracks
BOOST_FIXTURE_TEST_CASE( PassBesideAnotherKeepsEveryClaim, SHARED_CLONE_FIXTURE )
{
    MDW_GIT_REMOTE    remote;
    MDW_GIT_CHECKOUTS mine( m_repo.get(), remote, "origin" );
    MDW_GIT_CHECKOUTS other( m_other.get(), remote, "origin" );

    BOOST_REQUIRE( mine.Take( "board.kicad_pcb", { "carol@http://acme.test", "carol", "carol@acme.test" } )
                   == RESULT::OK );
    BOOST_REQUIRE( mine.Refresh() == RESULT::OK );
    BOOST_REQUIRE( mine.Find( "board.kicad_pcb" ) );

    std::vector<MDW_GIT_CHECKOUTS::CHANGE> changes;

    mine.SetChangeObserver(
            [&changes]( const std::vector<MDW_GIT_CHECKOUTS::CHANGE>& aChanges )
            {
                changes.insert( changes.end(), aChanges.begin(), aChanges.end() );
            } );

    const std::vector<std::string> refs = Refs();
    MDW_GIT_CHECKOUTS::STAGE       stage = mine.Stage( {} );
    ws::WorkspaceError             fetched = mine.Fetch( m_poll.get(), stage );

    BOOST_REQUIRE_EQUAL( fetched.code(), ws::WE_OK );
    BOOST_CHECK( Refs() == refs );

    MDW_GIT_CHECKOUTS::STAGE beside = other.Stage( {} );
    ws::WorkspaceError       otherFetched = other.Fetch( m_other.get(), beside );

    BOOST_REQUIRE( other.Apply( otherFetched, beside ) == RESULT::OK );

    BOOST_CHECK( mine.Apply( fetched, stage ) == RESULT::OK );
    BOOST_CHECK( mine.Find( "board.kicad_pcb" ) );
    BOOST_CHECK_EQUAL( changes.size(), 0u );
}


// A request's fetch overtaking a poll between its Stage and Apply must not bring back a released claim
BOOST_FIXTURE_TEST_CASE( OvertakenPassBringsBackNoReleasedClaim, SHARED_CLONE_FIXTURE )
{
    const fs::path          temp( TempPath().utf8_string() );
    const std::string       originPath = ( temp / "origin.git" ).string();
    KIGIT::GitRepositoryPtr teammateRepo = KI_TEST::InitGitRepository( wxString::FromUTF8( ( temp / "dave" ).string() ),
                                                                       "main", "dave", "dave@acme.test" );
    git_remote*             origin = nullptr;

    BOOST_REQUIRE_EQUAL( git_remote_create( &origin, teammateRepo.get(), "origin", originPath.c_str() ), GIT_OK );
    git_remote_free( origin );

    MDW_GIT_REMOTE    remote;
    MDW_GIT_CHECKOUTS mine( m_repo.get(), remote, "origin" );
    MDW_GIT_CHECKOUTS teammate( teammateRepo.get(), remote, "origin" );

    MDW_GIT_CHECKOUTS::STAGE stage = mine.Stage( {} );

    BOOST_REQUIRE( teammate.Take( "board.kicad_pcb", { "dave@http://acme.test", "dave", "dave@acme.test" } )
                   == RESULT::OK );

    ws::WorkspaceError fetched = mine.Fetch( m_poll.get(), stage );

    BOOST_REQUIRE_EQUAL( fetched.code(), ws::WE_OK );
    BOOST_REQUIRE( teammate.Give( "board.kicad_pcb" ) == RESULT::OK );
    BOOST_REQUIRE( mine.Refresh() == RESULT::OK );

    BOOST_CHECK( mine.Apply( fetched, stage ) == RESULT::OK );
    BOOST_CHECK( !mine.Find( "board.kicad_pcb" ) );
}


// The release confirmed dave's claim, and erin claiming the file while the reason was asked must keep hers
BOOST_FIXTURE_TEST_CASE( ForcedReleaseRefusesAReplacedClaim, SHARED_CLONE_FIXTURE )
{
    const fs::path    temp( TempPath().utf8_string() );
    const std::string originPath = ( temp / "origin.git" ).string();

    auto teammate = [&]( const std::string& aName )
    {
        const std::string       email = aName + "@acme.test";
        KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( wxString::FromUTF8( ( temp / aName ).string() ),
                                                                   "main", aName.c_str(), email.c_str() );
        git_remote*             origin = nullptr;

        BOOST_REQUIRE_EQUAL( git_remote_create( &origin, repo.get(), "origin", originPath.c_str() ), GIT_OK );
        git_remote_free( origin );
        return repo;
    };

    KIGIT::GitRepositoryPtr daveRepo = teammate( "dave" );
    KIGIT::GitRepositoryPtr erinRepo = teammate( "erin" );

    MDW_GIT_REMOTE    remote;
    MDW_GIT_CHECKOUTS mine( m_repo.get(), remote, "origin" );
    MDW_GIT_CHECKOUTS dave( daveRepo.get(), remote, "origin" );
    MDW_GIT_CHECKOUTS erin( erinRepo.get(), remote, "origin" );

    BOOST_REQUIRE( dave.Take( "board.kicad_pcb", { "dave@http://acme.test", "dave", "dave@acme.test" } )
                   == RESULT::OK );
    BOOST_REQUIRE( mine.Refresh() == RESULT::OK );

    std::optional<MDW_GIT_CLAIM> shown = mine.Find( "board.kicad_pcb" );

    BOOST_REQUIRE( shown );

    BOOST_REQUIRE( dave.Give( "board.kicad_pcb" ) == RESULT::OK );
    BOOST_REQUIRE( erin.Take( "board.kicad_pcb", { "erin@http://acme.test", "erin", "erin@acme.test" } )
                   == RESULT::OK );

    MDW_GIT_CHECKOUTS::RELEASE release{ "carol", "carol@acme.test", "Leaving for the day", "carol@http://acme.test" };
    release.confirmed = std::make_pair( shown->OwnerId(), shown->since );

    BOOST_CHECK( mine.Give( "board.kicad_pcb", release ) == RESULT::REPLACED );

    BOOST_REQUIRE( erin.Refresh() == RESULT::OK );
    std::optional<MDW_GIT_CLAIM> kept = erin.Find( "board.kicad_pcb" );

    BOOST_REQUIRE( kept );
    BOOST_CHECK_EQUAL( kept->user, "erin" );
}


// The signed-in account owns a claim, since the editable author name would let anyone claim as any name
BOOST_FIXTURE_TEST_CASE( ClaimOwnerIsTheSignedInAccount, SHARED_CLONE_FIXTURE )
{
    ws::Account hello;
    hello.set_display_name( "Robert QA" );
    hello.set_email( "robert.qa@acme.test" );

    const GIT_IDENTITY   author = MdwGitIdentity( nullptr, hello, wxString() );
    const MDW_GIT_HOLDER holder = MdwGitHolder( author, "bob", "http://192.0.2.26:3301/acme/board.git" );

    BOOST_CHECK_EQUAL( holder.owner, "bob@http://192.0.2.26:3301" );
    BOOST_CHECK_EQUAL( holder.user, "Robert QA" );

    MDW_GIT_REMOTE    remote;
    MDW_GIT_CHECKOUTS mine( m_repo.get(), remote, "origin" );

    BOOST_REQUIRE( mine.Take( "board.kicad_pcb", holder ) == RESULT::OK );

    std::optional<MDW_GIT_CLAIM> claim = mine.Find( "board.kicad_pcb" );

    BOOST_REQUIRE( claim );
    BOOST_CHECK_EQUAL( claim->owner, "bob@http://192.0.2.26:3301" );
    BOOST_CHECK_EQUAL( claim->user, "Robert QA" );
}


// A claim from before owners, naming only an email, still matches its holder
BOOST_FIXTURE_TEST_CASE( ClaimByLegacyEmailOwnerStillMatches, SHARED_CLONE_FIXTURE )
{
    // A legacy record with no owner key, on the ref a fetch would track
    nlohmann::json record = { { "user", "Bob Senior" },
                              { "email", "bob@acme.test" },
                              { "item_id", "board.kicad_pcb" } };

    git_treebuilder* builder = nullptr;
    git_oid          treeId;
    BOOST_REQUIRE_EQUAL( git_treebuilder_new( &builder, m_repo.get(), nullptr ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_treebuilder_write( &treeId, builder ), GIT_OK );
    git_treebuilder_free( builder );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, m_repo.get(), &treeId ), GIT_OK );

    git_signature* signature = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &signature, "Bob Senior", "bob@acme.test" ), GIT_OK );

    const std::string message = MdwGitRecordMessage( "Bob Senior is editing board.kicad_pcb", std::string(), record );
    git_oid           oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &oid, m_repo.get(), nullptr, signature, signature, nullptr,
                                            message.c_str(), tree, 0, nullptr ),
                         GIT_OK );
    git_tree_free( tree );
    git_signature_free( signature );

    git_reference* ref = nullptr;
    const char* tracking = "refs/kicad-remote/origin/checkouts/board%2Ekicad_pcb";
    BOOST_REQUIRE_EQUAL( git_reference_create( &ref, m_repo.get(), tracking, &oid, 1, nullptr ), GIT_OK );
    git_reference_free( ref );

    MDW_GIT_REMOTE    remote;
    MDW_GIT_CHECKOUTS reader( m_repo.get(), remote, "origin" );

    std::optional<MDW_GIT_CLAIM> claim = reader.Find( "board.kicad_pcb" );

    BOOST_REQUIRE( claim );
    BOOST_CHECK( claim->owner.empty() );

    const MDW_GIT_HOLDER bob{ "bob@http://acme.test", "Robert QA", "bob@acme.test" };
    const MDW_GIT_HOLDER alice{ "alice@http://acme.test", "Alice", "alice@acme.test" };

    BOOST_CHECK( bob.Is( claim->owner, claim->email ) );
    BOOST_CHECK( !alice.Is( claim->owner, claim->email ) );

    // An owner names the account, so a shared email address is no longer enough
    BOOST_CHECK( !alice.Is( "bob@http://acme.test", "alice@acme.test" ) );
    BOOST_CHECK( bob.Is( "bob@http://acme.test", "other@acme.test" ) );
}


// A claim recorded where no login was known names its holder by email, as does a reader with no login yet
BOOST_AUTO_TEST_CASE( ClaimByEmailOwnerMatchesTheSameAccountWithALogin )
{
    const MDW_GIT_HOLDER signedIn{ "bob@http://acme.test", "Bob", "bob@acme.test" };
    const MDW_GIT_HOLDER noLogin{ "bob@acme.test", "Bob", "bob@acme.test" };

    BOOST_CHECK( signedIn.Is( "bob@acme.test", "bob@acme.test" ) );
    BOOST_CHECK( noLogin.Is( "bob@http://acme.test", "bob@acme.test" ) );
    BOOST_CHECK( !signedIn.Is( "alice@acme.test", "alice@acme.test" ) );
    BOOST_CHECK( !noLogin.Is( "alice@http://acme.test", "alice@acme.test" ) );
}


BOOST_AUTO_TEST_SUITE_END()
