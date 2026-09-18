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

#include "mdw_git_test_utils.h"

#include <boost/test/unit_test.hpp>

#include <mdw/mdw_backend.h>
#include <mdw/mdw_backend_supervisor.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_locking.pb.h>
#include <api/mdw/backend_revisions.pb.h>

#include <git/kigit_platform.h>
#include <qa_utils/env_var_utils.h>
#include <qa_utils/git_test_fixture.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/utils.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;


namespace
{

// A repository of project files with a bare origin, served to bob by a backend session
class PROJECT_REPO_FIXTURE : public KI_TEST::GIT_TEST_FIXTURE
{
public:
    PROJECT_REPO_FIXTURE() :
            m_dataDir( wxS( "KICAD_MDW_DATA_DIR" ), TempPath() + wxFileName::GetPathSeparator() + wxS( "data" ) )
    {
        const fs::path temp( TempPath().utf8_string() );
        const fs::path origin = temp / "origin.git";

        m_tree = temp / "work";

        git_repository* bare = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_init( &bare, origin.string().c_str(), 1 ), GIT_OK );
        git_repository_free( bare );

        m_repo =
                KI_TEST::InitGitRepository( wxString::FromUTF8( m_tree.string() ), "main", "carol", "carol@acme.test" );

        git_remote* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_remote_create( &remote, m_repo.get(), "origin", origin.string().c_str() ), GIT_OK );
        git_remote_free( remote );
    }

    ~PROJECT_REPO_FIXTURE()
    {
        if( m_supervisor )
            m_supervisor->Shutdown();
    }

    /// The ctrl-board history in aFolder, where carol's "Place R22 and R23" changes the project file last and bob's
    /// later commits leave it alone. Answers bob's last commit
    std::string CtrlBoard( const std::string& aFolder )
    {
        const std::string at = aFolder.empty() ? std::string() : aFolder + "/";

        for( const char* name : { "ctrl-board.kicad_pro", "ctrl-board.kicad_sch", "pic_sockets.kicad_sch" } )
            Copy( std::string( "common/git/mdw_ctrl_board/" ) + name, at + name );

        Copy( "common/git/mdw_ctrl_board/ctrl-board.kicad_pcb", at + "ctrl-board.kicad_pcb" );

        Commit( { at + "ctrl-board.kicad_pro", at + "ctrl-board.kicad_sch" }, "Place R22 and R23" );
        SetAuthor( "bob" );
        Commit( { at + "ctrl-board.kicad_pcb" }, "Update board to v10 format" );
        return Commit( { at + "pic_sockets.kicad_sch" }, "Add the PIC sockets sheet" );
    }

    void Copy( const std::string& aData, const std::string& aPath )
    {
        fs::create_directories( ( m_tree / aPath ).parent_path() );
        fs::copy_file( fs::path( KI_TEST::GetTestDataRootDir() ) / aData, m_tree / aPath );
    }

    /// Commit aPaths as they stand in the tree, a missing one as removed, and answer the new commit's id
    std::string Commit( std::initializer_list<std::string> aPaths, const char* aMessage )
    {
        {
            git_index* index = nullptr;
            BOOST_REQUIRE_EQUAL( git_repository_index( &index, m_repo.get() ), GIT_OK );
            KIGIT::GitIndexPtr indexPtr( index );

            for( const std::string& path : aPaths )
            {
                BOOST_REQUIRE_EQUAL( fs::exists( m_tree / path ) ? git_index_add_bypath( index, path.c_str() )
                                                                 : git_index_remove_bypath( index, path.c_str() ),
                                     GIT_OK );
            }

            BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
        }

        git_oid oid = KI_TEST::CommitGitIndex( m_repo.get(), wxString::FromUTF8( aMessage ) );
        return git_oid_tostr_s( &oid );
    }

    void SetAuthor( const char* aName )
    {
        git_config* config = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &config, m_repo.get() ), GIT_OK );
        KIGIT::GitConfigPtr configPtr( config );
        BOOST_REQUIRE_EQUAL( git_config_set_string( config, "user.name", aName ), GIT_OK );
    }

    /// Open the project at aProjectFile in a backend session, as for the project the editor has open
    void Launch( const std::string& aProjectFile )
    {
        MDW_BACKEND_MANIFEST manifest;
        manifest.backend_id = wxS( "org.kicad.mdw.git" );
        manifest.executable_path = wxString::FromUTF8( MDW_GIT_EXECUTABLE );

        MDW_BACKEND_CONTEXT context;
        context.workspaceId = wxS( "acme" );
        context.projectPath = wxString::FromUTF8( ( m_tree / aProjectFile ).string() );

        m_supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
        BOOST_REQUIRE( m_supervisor->Launch( 8000 ) );

        m_backend = m_supervisor->Backend();
        BOOST_REQUIRE( m_backend );
    }

    /// The message of the commit or tag aSpec names
    std::string Message( const std::string& aSpec )
    {
        git_object* object = nullptr;
        BOOST_REQUIRE_EQUAL( git_revparse_single( &object, m_repo.get(), aSpec.c_str() ), GIT_OK );
        KIGIT::GitObjectPtr objectPtr( object );

        if( git_object_type( object ) == GIT_OBJECT_TAG )
            return git_tag_message( reinterpret_cast<git_tag*>( object ) );

        return git_commit_message( reinterpret_cast<git_commit*>( object ) );
    }

    ws::ItemInfo Info( const std::string& aItemId )
    {
        mdw::GetItemInfoRequest request;
        mdw::GetItemInfoReply   reply;
        request.set_item_id( aItemId );

        BOOST_REQUIRE( m_backend->CallSync( request, reply, 8000 ).IsOk() );
        return reply.item();
    }

    mdw::GetRevisionHistoryReply History( const std::string& aItemId )
    {
        mdw::GetRevisionHistoryRequest request;
        mdw::GetRevisionHistoryReply   reply;
        request.set_item_id( aItemId );

        BOOST_REQUIRE( m_backend->CallSync( request, reply, 8000 ).IsOk() );
        return reply;
    }

    /// Release the design at the revision the release dialog reads, the located base of aProjectFile
    mdw::CreateBaselineReply Baseline( const std::string& aProjectFile, const std::string& aNote )
    {
        mdw::LocateItemsRequest locate;
        mdw::LocateItemsReply   located;
        locate.add_relative_paths( aProjectFile );

        BOOST_REQUIRE( m_backend->CallSync( locate, located, 8000 ).IsOk() );
        BOOST_REQUIRE_EQUAL( located.items_size(), 1 );

        mdw::CreateBaselineRequest request;
        mdw::CreateBaselineReply   reply;
        request.set_item_id( located.items( 0 ).item().item_id() );
        request.set_revision_id( located.items( 0 ).base_revision_id() );
        request.set_comment( aNote );
        request.set_idempotency_id( "release" );

        BOOST_REQUIRE( m_backend->CallSync( request, reply, 30000 ).IsOk() );
        return reply;
    }

    /// What a release of aProjectFile at its located base would be held by, checking one placed part
    mdw::BaselineHold Hold( const std::string& aProjectFile )
    {
        mdw::LocateItemsRequest locate;
        mdw::LocateItemsReply   located;
        locate.add_relative_paths( aProjectFile );

        BOOST_REQUIRE( m_backend->CallSync( locate, located, 8000 ).IsOk() );
        BOOST_REQUIRE_EQUAL( located.items_size(), 1 );

        mdw::CheckBaselineRequest request;
        mdw::CheckBaselineReply   reply;
        request.set_item_id( located.items( 0 ).item().item_id() );
        request.set_revision_id( located.items( 0 ).base_revision_id() );
        request.add_parts()->set_lib_id( "Device:R" );

        BOOST_REQUIRE( m_backend->CallSync( request, reply, 8000 ).IsOk() );
        BOOST_CHECK_EQUAL( reply.parts_size(), 1 );
        return reply.hold();
    }

    KI_TEST::SCOPED_PROCESS_ENV_VAR         m_dataDir;
    fs::path                                m_tree;
    KIGIT::GitRepositoryPtr                 m_repo;
    std::unique_ptr<MDW_BACKEND_SUPERVISOR> m_supervisor;
    std::shared_ptr<MDW_BACKEND>            m_backend;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitBaseline )


// A release records the folder's newest commit, not the project file's older one, and the project reads released
// A later commit outside the project's folder must not move what the project releases
BOOST_FIXTURE_TEST_CASE( DesignBaselineNamesTheTaggedRevisionAndReadsReleased, PROJECT_REPO_FIXTURE )
{
    const std::string project = "ctrl-board/ctrl-board.kicad_pro";
    const std::string newest = CtrlBoard( "ctrl-board" );

    Copy( "common/git/mdw_ctrl_board/ctrl-board.kicad_pro", "amp-board/amp-board.kicad_pro" );
    Commit( { "amp-board/amp-board.kicad_pro" }, "Add amp-board" );
    Launch( project );

    mdw::CreateBaselineReply reply = Baseline( "ctrl-board.kicad_pro", std::string() );

    BOOST_REQUIRE_EQUAL( reply.baseline().parent_revision_ids_size(), 1 );
    BOOST_CHECK_EQUAL( reply.baseline().parent_revision_ids( 0 ), newest );

    ws::ItemInfo released = Info( project );
    BOOST_CHECK_EQUAL( released.revision_id(), newest );
    BOOST_CHECK_EQUAL( released.state().category(), ws::LC_RELEASED );
    BOOST_CHECK_EQUAL( released.revision_label(), "Baseline 1" );

    mdw::GetRevisionHistoryReply history = History( project );
    BOOST_REQUIRE_EQUAL( history.revisions_size(), 3 );
    BOOST_CHECK_EQUAL( history.revisions( 0 ).revision_id(), newest );
    BOOST_CHECK_EQUAL( history.revisions( 0 ).kind(), ws::RK_BASELINE );
    BOOST_CHECK_EQUAL( history.revisions( 0 ).label(), "Baseline 1" );

    fs::remove( m_tree / "ctrl-board/pic_sockets.kicad_sch" );
    const std::string next = Commit( { "ctrl-board/pic_sockets.kicad_sch" }, "Remove the PIC sockets sheet" );

    ws::ItemInfo edited = Info( project );
    BOOST_CHECK_EQUAL( edited.revision_id(), next );
    BOOST_CHECK_EQUAL( edited.state().category(), ws::LC_EDITABLE );
}


// Only a release on the remote counts, yet a teammate whose clone took the tag but not its mirror reads it released
BOOST_FIXTURE_TEST_CASE( TeammateReadsAPulledBaselineReleased, PROJECT_REPO_FIXTURE )
{
    const std::string project = "ctrl-board/ctrl-board.kicad_pro";
    const std::string newest = CtrlBoard( "ctrl-board" );
    const fs::path    origin = fs::path( TempPath().utf8_string() ) / "origin.git";
    const fs::path    teammate = fs::path( TempPath().utf8_string() ) / "teammate";

    Launch( project );
    Baseline( "ctrl-board.kicad_pro", std::string() );
    m_supervisor->Shutdown();

    git_repository*   clone = nullptr;
    git_clone_options options = GIT_CLONE_OPTIONS_INIT;

    BOOST_REQUIRE_EQUAL( git_clone( &clone, m_tree.string().c_str(), teammate.string().c_str(), &options ), GIT_OK );
    KIGIT::GitRepositoryPtr clonePtr( clone );
    BOOST_REQUIRE_EQUAL( git_remote_set_url( clone, "origin", origin.string().c_str() ), GIT_OK );

    m_tree = teammate;
    Launch( project );

    ws::ItemInfo released = Info( project );
    BOOST_CHECK_EQUAL( released.revision_id(), newest );
    BOOST_CHECK_EQUAL( released.state().category(), ws::LC_RELEASED );
    BOOST_CHECK_EQUAL( released.revision_label(), "Baseline 1" );
}


// A release counts only while its tag is on the remote, though this clone still holds the tag
BOOST_FIXTURE_TEST_CASE( WithdrawnBaselineReadsEditable, PROJECT_REPO_FIXTURE )
{
    const std::string project = "ctrl-board/ctrl-board.kicad_pro";
    const fs::path    origin = fs::path( TempPath().utf8_string() ) / "origin.git";

    CtrlBoard( "ctrl-board" );
    Launch( project );
    Baseline( "ctrl-board.kicad_pro", std::string() );
    BOOST_REQUIRE_EQUAL( Info( project ).state().category(), ws::LC_RELEASED );
    m_supervisor->Shutdown();

    git_repository* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open_bare( &remote, origin.string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr remotePtr( remote );
    BOOST_REQUIRE_EQUAL( git_reference_remove( remote, "refs/tags/kicad/design-rel/1" ), GIT_OK );

    // A fresh backend, so its first read fetches and prunes the mirror
    Launch( project );
    BOOST_CHECK_EQUAL( Info( project ).state().category(), ws::LC_EDITABLE );
}


// The poll's own mirror fetch prunes a withdrawn release
BOOST_FIXTURE_TEST_CASE( PollWithdrawsABaseline, PROJECT_REPO_FIXTURE )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR interval( wxS( "KICAD_MDW_POLL_INTERVAL_MS" ), wxS( "100" ) );
    const std::string               project = "ctrl-board/ctrl-board.kicad_pro";
    const fs::path                  origin = fs::path( TempPath().utf8_string() ) / "origin.git";

    CtrlBoard( "ctrl-board" );
    Launch( project );
    Baseline( "ctrl-board.kicad_pro", std::string() );
    BOOST_REQUIRE_EQUAL( Info( project ).state().category(), ws::LC_RELEASED );

    git_repository* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open_bare( &remote, origin.string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr remotePtr( remote );
    BOOST_REQUIRE_EQUAL( git_reference_remove( remote, "refs/tags/kicad/design-rel/1" ), GIT_OK );

    // Claims were read once already, so only a pass can bring the withdrawal
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    int        category = ws::LC_RELEASED;

    while( category != ws::LC_EDITABLE && std::chrono::steady_clock::now() < limit )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        category = Info( project ).state().category();
    }

    BOOST_CHECK_EQUAL( category, ws::LC_EDITABLE );
}


// Design releases are numbered across the repository, so another project's release must not mark this one
BOOST_FIXTURE_TEST_CASE( AnotherProjectsBaselineLeavesThisOneEditable, PROJECT_REPO_FIXTURE )
{
    CtrlBoard( std::string() );
    Copy( "common/git/mdw_ctrl_board/ctrl-board.kicad_pro", "amp-board.kicad_pro" );
    Commit( { "amp-board.kicad_pro" }, "Add amp-board" );
    Launch( "ctrl-board.kicad_pro" );

    Baseline( "ctrl-board.kicad_pro", std::string() );
    BOOST_REQUIRE_EQUAL( Info( "ctrl-board.kicad_pro" ).state().category(), ws::LC_RELEASED );

    ws::ItemInfo other = Info( "amp-board.kicad_pro" );
    BOOST_CHECK_EQUAL( other.state().category(), ws::LC_EDITABLE );
    BOOST_CHECK_EQUAL( other.revision_label(), "" );

    mdw::GetRevisionHistoryReply history = History( "amp-board.kicad_pro" );

    for( const ws::Revision& revision : history.revisions() )
        BOOST_CHECK_NE( revision.kind(), ws::RK_BASELINE );
}


// A clone of an empty remote has its parts checked before any commit, and only its first commit holds it
BOOST_FIXTURE_TEST_CASE( UnbornDesignIsHeldOnlyByItsFirstCommit, PROJECT_REPO_FIXTURE )
{
    Copy( "common/git/mdw_ctrl_board/ctrl-board.kicad_pro", "ctrl-board.kicad_pro" );
    Copy( "common/git/mdw_ctrl_board/ctrl-board.kicad_sch", "ctrl-board.kicad_sch" );
    Launch( "ctrl-board.kicad_pro" );

    // Untracked, so the check names no item
    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_UNCOMMITTED );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, m_repo.get() ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "ctrl-board.kicad_pro" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_UNCOMMITTED );
}


// The release dialog reads readiness from the same check the release refuses on
BOOST_FIXTURE_TEST_CASE( CheckReportsTheHoldReleaseRefusesOn, PROJECT_REPO_FIXTURE )
{
    const std::string head = CtrlBoard( std::string() );
    Launch( "ctrl-board.kicad_pro" );

    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_NONE );

    fs::remove( m_tree / "pic_sockets.kicad_sch" );
    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_UNCOMMITTED );
    Copy( "common/git/mdw_ctrl_board/pic_sockets.kicad_sch", "pic_sockets.kicad_sch" );

    git_oid oid;
    BOOST_REQUIRE_EQUAL( git_oid_fromstr( &oid, head.c_str() ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_repository_set_head_detached( m_repo.get(), &oid ), GIT_OK );
    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_DETACHED );
    BOOST_REQUIRE_EQUAL( git_repository_set_head( m_repo.get(), "refs/heads/main" ), GIT_OK );

    BOOST_REQUIRE_EQUAL( git_remote_delete( m_repo.get(), "origin" ), GIT_OK );
    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_NO_REMOTE );
}


// A local history repository inside the project is a transient nested repository, not an uncommitted change
BOOST_FIXTURE_TEST_CASE( LocalHistoryDoesNotHoldARelease, PROJECT_REPO_FIXTURE )
{
    CtrlBoard( "ctrl-board" );

    git_repository* history = nullptr;
    const std::string historyPath = ( m_tree / "ctrl-board" / ".history" ).string();
    BOOST_REQUIRE_EQUAL( git_repository_init( &history, historyPath.c_str(), 0 ), GIT_OK );
    git_repository_free( history );

    // Local history leaves its own exclusions file inside, which is what makes git report the folder
    std::ofstream( ( m_tree / "ctrl-board" / ".history" / ".gitignore" ).string() ) << "fp-info-cache\n";

    Launch( "ctrl-board/ctrl-board.kicad_pro" );

    BOOST_CHECK_EQUAL( Hold( "ctrl-board.kicad_pro" ), mdw::HOLD_NONE );
}


// A claim or a release reads as a sentence in any git viewer, and the backend still reads its record back
// The note holds a "---" line, where git's own trailer parser stops looking
BOOST_FIXTURE_TEST_CASE( RecordMessageRoundTrips, PROJECT_REPO_FIXTURE )
{
    CtrlBoard( std::string() );
    Launch( "ctrl-board.kicad_pro" );

    mdw::LockRequest lock;
    mdw::LockReply   locked;
    lock.set_item_id( "ctrl-board.kicad_pcb" );
    lock.set_idempotency_id( "claim" );

    BOOST_REQUIRE( m_backend->CallSync( lock, locked, 30000 ).IsOk() );

    const std::string claim = "bob is editing ctrl-board.kicad_pcb\n\n";
    BOOST_CHECK_EQUAL( Message( "refs/kicad/checkouts/ctrl-board%2Ekicad_pcb" ).substr( 0, claim.size() ), claim );

    mdw::ListLocksRequest listRequest;
    mdw::ListLocksReply   locks;

    BOOST_REQUIRE( m_backend->CallSync( listRequest, locks, 30000 ).IsOk() );
    BOOST_REQUIRE_EQUAL( locks.locks_size(), 1 );
    BOOST_CHECK_EQUAL( locks.locks( 0 ).item_id(), "ctrl-board.kicad_pcb" );

    const std::string note = "First boards for the enclosure fit check\n---\nR23 is rotated by hand";

    Baseline( "ctrl-board.kicad_pro", note );

    const std::string release = "Baseline 1 of ctrl-board\n\n" + note + "\n\n";
    BOOST_CHECK_EQUAL( Message( "kicad/design-rel/1" ).substr( 0, release.size() ), release );
    BOOST_CHECK_EQUAL( Info( "ctrl-board.kicad_pro" ).revision_label(), "Baseline 1" );
}


// The lock a backend reports names the signed-in account and the author through the holder a claim records
BOOST_FIXTURE_TEST_CASE( LockInfoNamesTheHolderOwnerAndAuthor, PROJECT_REPO_FIXTURE )
{
    CtrlBoard( std::string() );
    Launch( "ctrl-board.kicad_pro" );

    mdw::AuthenticateRequest signIn;
    mdw::AuthenticateReply   signedIn;
    ( *signIn.mutable_credentials()->mutable_fields() )["username"] = "dave";
    ( *signIn.mutable_credentials()->mutable_fields() )["password"] = "dave-pw";

    MDW_RESULT auth = m_backend->CallSync( signIn, signedIn, 30000 );
    BOOST_REQUIRE_MESSAGE( auth.IsOk(), auth.message.ToStdString() );

    mdw::LockRequest lock;
    mdw::LockReply   locked;
    lock.set_item_id( "ctrl-board.kicad_pcb" );
    lock.set_idempotency_id( "claim" );

    BOOST_REQUIRE( m_backend->CallSync( lock, locked, 30000 ).IsOk() );

    mdw::GetLockInfoRequest query;
    mdw::GetLockInfoReply   held;
    query.set_item_id( "ctrl-board.kicad_pcb" );

    BOOST_REQUIRE( m_backend->CallSync( query, held, 30000 ).IsOk() );
    BOOST_REQUIRE( held.lock().is_locked() );
    BOOST_CHECK( held.lock().locked_by_me() );

    // The remote is a local path with no realm, so the owner is the login alone, not the author's email
    BOOST_CHECK_EQUAL( held.lock().owner_id(), "dave" );
    BOOST_CHECK_EQUAL( held.lock().owner_display_name(), "bob" );
}


// A claim records the author preference, not the host's git identity
BOOST_FIXTURE_TEST_CASE( ClaimCarriesKiCadAuthorPreference, PROJECT_REPO_FIXTURE )
{
    CtrlBoard( std::string() );

    // The repository names no author, so the global config and the preference compete
    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, m_repo.get() ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );
    git_config*         local = nullptr;
    BOOST_REQUIRE_EQUAL( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ), GIT_OK );
    KIGIT::GitConfigPtr localPtr( local );
    BOOST_REQUIRE_EQUAL( git_config_delete_entry( local, "user.name" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_delete_entry( local, "user.email" ), GIT_OK );

    const fs::path home = m_tree.parent_path() / "home";
    fs::create_directories( home );
    std::ofstream( home / ".gitconfig" ) << "[user]\n\tname = Host User\n\temail = host@example.com\n";

    // The backend process reads its global config from the environment it inherits
    KI_TEST::SCOPED_PROCESS_ENV_VAR        homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR        xdgEnv( wxS( "XDG_CONFIG_HOME" ), wxString::FromUTF8( home.string() ) );
    KI_TEST::SCOPED_GIT_CONFIG_SEARCH_PATH globalConfig( home.string() );

    COMMON_SETTINGS::GIT prefs{};
    prefs.useDefaultAuthor = false;
    prefs.authorName = wxS( "Carol Designer" );
    prefs.authorEmail = wxS( "carol@acme.test" );
    SCOPED_AUTHOR_PREFERENCE preference( prefs );

    Launch( "ctrl-board.kicad_pro" );

    mdw::LockRequest lock;
    mdw::LockReply   locked;
    lock.set_item_id( "ctrl-board.kicad_pcb" );
    lock.set_idempotency_id( "claim" );

    MDW_RESULT result = m_backend->CallSync( lock, locked, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message.ToStdString() );

    git_object* object = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &object, m_repo.get(), "refs/kicad/checkouts/ctrl-board%2Ekicad_pcb" ),
                         GIT_OK );
    KIGIT::GitObjectPtr objectPtr( object );

    git_object* peeled = nullptr;
    BOOST_REQUIRE_EQUAL( git_object_peel( &peeled, object, GIT_OBJECT_COMMIT ), GIT_OK );
    KIGIT::GitObjectPtr peeledPtr( peeled );
    const git_commit*   commit = reinterpret_cast<const git_commit*>( peeled );

    BOOST_CHECK_EQUAL( git_commit_author( commit )->name, "Carol Designer" );
    BOOST_CHECK_EQUAL( git_commit_author( commit )->email, "carol@acme.test" );
}


BOOST_AUTO_TEST_SUITE_END()
