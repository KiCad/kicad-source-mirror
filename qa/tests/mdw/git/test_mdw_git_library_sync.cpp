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
#include "mdw_git_repo.h"
#include "mdw_git_test_utils.h"

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>
#include <api/mdw/mdw_envelope.pb.h>
#include <api/workspace/workspace_types.pb.h>
#include <git/kigit_failure.h>
#include <kinng.h>
#include <lib_id.h>
#include <mdw/client/mdw_call.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_op_state.h>
#include <mdw/client/mdw_source.h>
#include <widgets/mdw_member_index.h>

#include <wx/app.h>

#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;


namespace
{

/// The reflog message of the copy's last branch move, "checkout" for a fast-forward and "update" for a merge
std::string lastMove( const fs::path& aCopy )
{
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, aCopy.string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_reflog* reflog = nullptr;
    BOOST_REQUIRE_EQUAL( git_reflog_read( &reflog, repo, "refs/heads/main" ), GIT_OK );

    const git_reflog_entry* entry = git_reflog_entry_byindex( reflog, 0 );
    const char*             message = entry ? git_reflog_entry_message( entry ) : nullptr;
    const std::string       move = message ? message : "";

    git_reflog_free( reflog );
    return move;
}


/// Later tests need the client workers, which only a new framework brings back, so a case that stops them
/// restarts them however it ends
struct WORKER_RESTART
{
    ~WORKER_RESTART()
    {
        SCOPED_PGM program;
        BOOST_CHECK( Pgm().StartMdwFramework() );
    }
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitLibrarySync )


// A sync already waiting when the client workers stop fails at once instead of waiting forever
BOOST_AUTO_TEST_CASE( SyncEndsOnceWorkersStop )
{
    auto                    answer = std::make_shared<std::promise<MDW_RESULT>>();
    auto                    started = std::make_shared<std::promise<void>>();
    std::future<MDW_RESULT> outcome = answer->get_future();
    std::future<void>       waiting = started->get_future();

    // The op never completes, so MdwSync waits from the moment this returns until the workers stop
    std::function<MDW_OP( MDW_DONE<int> )> start = [started]( MDW_DONE<int> )
    {
        started->set_value();
        return MDW_OP( std::make_shared<MDW_OP_STATE>() );
    };

    // Detached, so a sync that never ends fails this case rather than hanging the suite
    std::thread(
            [answer, start]()
            {
                answer->set_value( MdwSync<int>( start ).first );
            } )
            .detach();

    WORKER_RESTART restart;

    BOOST_REQUIRE( waiting.wait_for( std::chrono::seconds( 10 ) ) == std::future_status::ready );
    BOOST_REQUIRE( MdwStopWorkers( std::chrono::seconds( 5 ) ) );
    BOOST_REQUIRE( outcome.wait_for( std::chrono::seconds( 10 ) ) == std::future_status::ready );
    BOOST_CHECK( outcome.get().error == MDW_ERROR_CODE::CANCELLED );
}


// A completion still queued on the UI when the workers stop never runs once the wait on it has ended
BOOST_AUTO_TEST_CASE( StoppedWaitSuppressesLateCallback )
{
    BOOST_REQUIRE( wxTheApp );

    // A completion an earlier case left would run here and pass for this one's, which UI_EVENT_ISOLATION prevents
    BOOST_REQUIRE( !wxTheApp->HasPendingEvents() );

    WORKER_RESTART restart;

    BOOST_REQUIRE( MdwStopWorkers( std::chrono::seconds( 5 ) ) );

    auto state = std::make_shared<MDW_OP_STATE>();
    bool ran = false;

    state->Complete(
            [&ran]()
            {
                ran = true;
            } );

    // Off the UI thread, which delivers this completion
    std::thread(
            [state]()
            {
                state->Wait();
            } )
            .join();

    wxTheApp->ProcessPendingEvents();

    BOOST_CHECK( !ran );
}


// Get Library Updates relies on this path, the only one that brings upstream commits into a copy
BOOST_FIXTURE_TEST_CASE( SyncPicksUpUpstreamCommit, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path    symdir = "connectors.kicad_symdir";
    const std::string added = "CONN_2.kicad_sym";

    fs::create_directories( Source() / symdir );

    for( const fs::directory_entry& entry : fs::directory_iterator( Seed() / symdir ) )
    {
        if( entry.path().filename() != added )
            fs::copy_file( entry.path(), Source() / symdir / entry.path().filename() );
    }

    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    const fs::path copy = librarian.Copy();

    MDW_LIBRARY_SYNC_RESULT materialized = librarian.SyncOk();
    BOOST_CHECK( materialized.filesChanged );
    BOOST_REQUIRE( fs::exists( copy / symdir / "CONN_1.kicad_sym" ) );
    BOOST_REQUIRE( !fs::exists( copy / symdir / added ) );

    // Nothing upstream moved, so no library needs reloading
    MDW_LIBRARY_SYNC_RESULT unchanged = librarian.SyncOk();
    BOOST_CHECK( unchanged.changes.empty() );
    BOOST_CHECK( !unchanged.filesChanged );

    fs::copy_file( Seed() / symdir / added, Source() / symdir / added );
    Publish( wxS( "Add CONN_2" ) );

    MDW_LIBRARY_SYNC_RESULT updated = librarian.SyncOk();
    BOOST_CHECK( updated.conflicts.empty() );
    BOOST_CHECK( updated.filesChanged );
    BOOST_REQUIRE_EQUAL( updated.changes.size(), 1u );
    BOOST_CHECK_EQUAL( updated.changes[0].member.item_id, wxS( "connectors.kicad_symdir#CONN_2" ) );
    BOOST_CHECK( updated.changes[0].change == MDW_CHANGE_TYPE::ADDED );
    BOOST_CHECK_EQUAL( StoredDigest( copy, symdir / added, copy / symdir / added ),
                       StoredDigest( copy, symdir / added, Seed() / symdir / added ) );

    // Nothing was edited here, so the copy moved by checkout
    BOOST_CHECK_EQUAL( lastMove( copy ), "checkout" );
    BOOST_CHECK( std::any_of( updated.copy.members.begin(), updated.copy.members.end(),
                              []( const MDW_COPY_MEMBER& aMember )
                              {
                                  return aMember.name == wxS( "CONN_2" );
                              } ) );
}


// A global core.autocrlf or attributes file would have the copy differ in every line from the LF every save writes
BOOST_FIXTURE_TEST_CASE( CopyHoldsTheCommittedBytes, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path home = m_temp.Path() / "home";

    fs::create_directories( home );
    std::ofstream( home / "attributes" ) << "*.kicad_sym text eol=crlf\n";
    std::ofstream( home / ".gitconfig" ) << "[core]\n\tautocrlf = true\n\tattributesfile = "
                                         << ( home / "attributes" ).generic_string() << "\n";

    // The backend process reads its global config from the environment it inherits
    KI_TEST::SCOPED_PROCESS_ENV_VAR homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR xdgEnv( wxS( "XDG_CONFIG_HOME" ), wxString::FromUTF8( home.string() ) );

    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();
    BOOST_CHECK( ReadFile( librarian.Copy() / LIBRARY ) == Upstream( LIBRARY ) );

    edit( Source() / LIBRARY, C_FILTERS, "C_*" );
    Publish( wxS( "Narrow C's filters" ) );
    librarian.SyncOk();
    BOOST_CHECK( ReadFile( librarian.Copy() / LIBRARY ) == Upstream( LIBRARY ) );
}


// A level attached above the copy's own config may be shared, so the copy's settings stay out of it
BOOST_FIXTURE_TEST_CASE( CommittedBytesStayInTheCopysConfig, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path    root = m_temp.Path() / "copy";
    const fs::path    app = m_temp.Path() / "kicad-longpaths.gitconfig";
    const std::string longPaths = "[core]\n\tlongpaths = true\n";
    git_repository*   repo = nullptr;

    std::ofstream( app, std::ios::binary ) << longPaths;
    BOOST_REQUIRE_EQUAL( git_repository_init( &repo, root.string().c_str(), false ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_config* config = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo ), GIT_OK );
    KIGIT::GitConfigPtr configPtr( config );

    BOOST_REQUIRE_EQUAL( git_config_add_file_ondisk( config, app.string().c_str(), GIT_CONFIG_LEVEL_APP, repo, 0 ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( MdwGitCommittedBytes( repo ), GIT_OK );

    BOOST_CHECK( ReadFile( app ) == longPaths );
    BOOST_CHECK( ReadFile( root / ".git" / "config" ).find( "eol = lf" ) != std::string::npos );
}


// A copy cloned while the global autocrlf applied is written back as committed, but for a file edited since
BOOST_FIXTURE_TEST_CASE( OlderCopyTakesTheCommittedBytes, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path    home = m_temp.Path() / "home";
    const std::string ics = "ics.kicad_sym";

    fs::create_directories( home );
    std::ofstream( home / ".gitconfig" ) << "[core]\n\tautocrlf = true\n";

    KI_TEST::SCOPED_PROCESS_ENV_VAR homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR xdgEnv( wxS( "XDG_CONFIG_HOME" ), wxString::FromUTF8( home.string() ) );

    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    fs::copy_file( Seed() / ics, Source() / ics );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    // As a clone made before the copy kept its own settings, whose checkout the global autocrlf converted
    {
        git_repository* repo = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &repo, librarian.Copy().string().c_str() ), GIT_OK );
        KIGIT::GitRepositoryPtr repoPtr( repo );

        git_config* config = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo ), GIT_OK );
        KIGIT::GitConfigPtr configPtr( config );

        git_config* local = nullptr;
        BOOST_REQUIRE_EQUAL( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ), GIT_OK );
        KIGIT::GitConfigPtr localPtr( local );

        for( const char* key : { "core.autocrlf", "core.eol", "core.attributesfile" } )
            BOOST_REQUIRE_EQUAL( git_config_delete_entry( local, key ), GIT_OK );
    }

    auto crlf = []( std::string aText )
    {
        for( size_t at = aText.find( '\n' ); at != std::string::npos; at = aText.find( '\n', at + 2 ) )
            aText.replace( at, 1, "\r\n" );

        return aText;
    };

    std::string  edited = Upstream( LIBRARY );
    const size_t at = edited.find( "\"" + C_FILTERS + "\"" );

    BOOST_REQUIRE( at != std::string::npos );
    edited = crlf( edited.replace( at, C_FILTERS.size() + 2, "\"C_*\"" ) );

    std::ofstream( librarian.Copy() / LIBRARY, std::ios::binary | std::ios::trunc ) << edited;
    std::ofstream( librarian.Copy() / ics, std::ios::binary | std::ios::trunc ) << crlf( Upstream( ics ) );

    librarian.SyncOk();
    BOOST_CHECK( ReadFile( librarian.Copy() / ics ) == Upstream( ics ) );
    BOOST_CHECK( ReadFile( librarian.Copy() / LIBRARY ) == edited );
}


// A source under prefix 'acme-' names 'acme-connectors:CONN_1', since no table row answers to 'connectors'
BOOST_FIXTURE_TEST_CASE( LocalLibIdAppliesSourcePrefix, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path symdir = "connectors.kicad_symdir";

    fs::create_directories( Source() / symdir );
    fs::copy_file( Seed() / symdir / "CONN_1.kicad_sym", Source() / symdir / "CONN_1.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );

    MDW_GIT_LIBRARIAN librarian = Connect( wxS( "copies" ), wxS( "acme-" ) );

    MDW_LIBRARY_SYNC_RESULT synced = librarian.SyncOk();

    auto member = std::find_if( synced.copy.members.begin(), synced.copy.members.end(),
                                []( const MDW_COPY_MEMBER& aMember )
                                {
                                    return aMember.name == wxS( "CONN_1" );
                                } );

    BOOST_REQUIRE( member != synced.copy.members.end() );

    // The item id alone names the member, whatever hint the source gives
    MDW_ITEM_REF part;
    part.item_id = member->member_id;
    part.path_hint = wxS( "connectors:CONN_1" );
    part.source = librarian.config.locator;

    std::optional<LIB_ID> id = librarian.sources->LocalLibId( MDW_LIBRARY_TYPE::SYMBOL, part );

    BOOST_REQUIRE( id );
    BOOST_CHECK_EQUAL( id->Format().wx_str(), wxS( "acme-connectors:CONN_1" ) );

    // A member of another source with the same id is not this source's
    part.source = wxS( "https://git.example.com/other-library.git" );
    BOOST_CHECK( !librarian.sources->LocalLibId( MDW_LIBRARY_TYPE::SYMBOL, part ) );

    part.source = librarian.config.locator;
    BOOST_CHECK( !librarian.sources->LocalLibId( MDW_LIBRARY_TYPE::FOOTPRINT, part ) );

    part.item_id = wxS( "connectors.kicad_symdir#missing" );
    BOOST_CHECK( !librarian.sources->LocalLibId( MDW_LIBRARY_TYPE::SYMBOL, part ) );

    part.item_id.clear();
    BOOST_CHECK( !librarian.sources->LocalLibId( MDW_LIBRARY_TYPE::SYMBOL, part ) );
}


// A symbol file added here and never checked in is an edit, which only the merge keeps
BOOST_FIXTURE_TEST_CASE( UntrackedSymbolFileTakesTheMerge, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path symdir = "connectors.kicad_symdir";

    fs::create_directories( Source() / symdir );

    fs::copy_file( Seed() / symdir / "CONN_1.kicad_sym", Source() / symdir / "CONN_1.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    const fs::path local = symdir / "MOUNTING_HOLE.kicad_sym";

    fs::copy_file( Seed() / local, librarian.Copy() / local );
    fs::copy_file( Seed() / symdir / "CONN_2.kicad_sym", Source() / symdir / "CONN_2.kicad_sym" );
    Publish( wxS( "Add CONN_2" ) );

    librarian.SyncOk();
    BOOST_CHECK_EQUAL( lastMove( librarian.Copy() ), "update" );
    BOOST_CHECK( fs::exists( librarian.Copy() / symdir / "CONN_2.kicad_sym" ) );
    BOOST_CHECK( ReadFile( librarian.Copy() / local ) == ReadFile( Seed() / local ) );
}


// No status lists an ignored file, so only the checkout can refuse to overwrite it
BOOST_FIXTURE_TEST_CASE( FastForwardKeepsIgnoredFile, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path symdir = "connectors.kicad_symdir";

    fs::create_directories( Source() / symdir );
    fs::copy_file( Seed() / symdir / "CONN_1.kicad_sym", Source() / symdir / "CONN_1.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    const fs::path mine = librarian.Copy() / symdir / "CONN_2.kicad_sym";

    std::ofstream( librarian.Copy() / ".git/info/exclude", std::ios::app )
            << "/connectors.kicad_symdir/CONN_2.kicad_sym\n";
    fs::copy_file( Seed() / symdir / "MOUNTING_HOLE.kicad_sym", mine );
    fs::copy_file( Seed() / symdir / "CONN_2.kicad_sym", Source() / symdir / "CONN_2.kicad_sym" );
    Publish( wxS( "Add CONN_2" ) );

    MDW_LIBRARY_SYNC_RESULT refused = librarian.Sync();
    BOOST_CHECK( refused.result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( refused.result.message.Contains( wxS( "'connectors.kicad_symdir/CONN_2.kicad_sym'" ) ),
                         refused.result.message );
    BOOST_CHECK( ReadFile( mine ) == ReadFile( Seed() / symdir / "MOUNTING_HOLE.kicad_sym" ) );
}


// A checkout whose branch could not move must not leave the index staged ahead of HEAD
BOOST_FIXTURE_TEST_CASE( UpdateAfterLockedBranchMerges, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path symdir = "connectors.kicad_symdir";

    fs::create_directories( Source() / symdir );
    fs::copy_file( Seed() / symdir / "CONN_1.kicad_sym", Source() / symdir / "CONN_1.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    fs::copy_file( Seed() / symdir / "CONN_2.kicad_sym", Source() / symdir / "CONN_2.kicad_sym" );
    Publish( wxS( "Add CONN_2" ) );

    const fs::path lock = librarian.Copy() / ".git/refs/heads/main.lock";

    std::ofstream( lock ) << "locked\n";
    BOOST_REQUIRE( !librarian.Sync().result.IsOk() );
    fs::remove( lock );

    MDW_LIBRARY_SYNC_RESULT updated = librarian.SyncOk();
    BOOST_CHECK( updated.conflicts.empty() );

    const fs::path added = symdir / "CONN_2.kicad_sym";

    BOOST_CHECK_EQUAL( StoredDigest( librarian.Copy(), added, librarian.Copy() / added ),
                       StoredDigest( librarian.Copy(), added, Seed() / added ) );
}


// A copy moved aside leaves its index and rows behind, which must not read as fetched
BOOST_FIXTURE_TEST_CASE( StatusTreatsMissingCopyAsNotFetched, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    std::mutex              mutex;
    std::condition_variable refreshed;
    bool                    locksRead = false;

    MDW_SUBSCRIPTION locks = librarian.session->EventBus().Subscribe(
            [&]( const MDW_EVENT& aEvent )
            {
                std::lock_guard<std::mutex> lock( mutex );

                locksRead = locksRead || aEvent.type == MDW_EVENT_TYPE::LOCKS_CHANGED;
                refreshed.notify_all();
            } );

    MDW_LIBRARY_SYNC_RESULT materialized = librarian.SyncOk();

    // A fresh copy's claims are fetched after the sync returns, and on Windows that fetch's writes fail the removal
    {
        std::unique_lock<std::mutex> lock( mutex );
        BOOST_REQUIRE( refreshed.wait_for( lock, std::chrono::seconds( 10 ),
                                           [&]()
                                           {
                                               return locksRead;
                                           } ) );
    }

    const wxString            marker = MDW_LIBRARY_SOURCES::Marker( librarian.config );
    MDW_LIBRARY_REGISTRATIONS registered;

    for( const MDW_COPY_LIBRARY& library : materialized.copy.libraries )
    {
        registered.insert( { library.type, library.nickname, marker,
                             MDW_LIBRARY_SOURCES::ComparablePath( materialized.copy.local_root + wxFILE_SEP_PATH
                                                                  + library.relative_path ) } );
    }

    MDW_LIBRARY_REQUIREMENT need;
    need.backend_id = librarian.config.backend_id;
    need.locator = librarian.config.locator;
    need.name = librarian.config.name;
    need.prefix = librarian.config.prefix;

    BOOST_REQUIRE( librarian.sources->Status( { need }, true, registered ).notice == MDW_LIBRARY_NOTICE::NONE );

    fs::remove_all( librarian.Copy() );

    BOOST_CHECK( librarian.sources->Status( { need }, true, registered ).notice == MDW_LIBRARY_NOTICE::NOT_FETCHED );
}


// An update that finds nothing new still answers the whole membership, so no member is listed again
BOOST_FIXTURE_TEST_CASE( UpdateOfCurrentCopyListsMembers, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    auto names = []( const MDW_LIBRARY_COPY& aCopy )
    {
        std::set<wxString> out;

        for( const MDW_COPY_MEMBER& member : aCopy.members )
            out.insert( member.name );

        return out;
    };

    const std::set<wxString> passives = { wxS( "C" ), wxS( "L" ) };

    MDW_LIBRARY_SYNC_RESULT materialized = librarian.SyncOk();
    BOOST_CHECK( names( materialized.copy ) == passives );

    // A file in no library is no edit to merge
    fs::copy_file( Seed() / "README.md", librarian.Copy() / "README.md" );

    const fs::path    gitIndex = librarian.Copy() / ".git" / "index";
    const std::string staged = ReadFile( gitIndex );

    mdw::UpdateSharedLibrariesRequest request;
    mdw::UpdateSharedLibrariesReply   reply;
    request.set_local_root( librarian.Copy().string() );

    MDW_RESULT result = librarian.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( reply.changes().empty() );
    BOOST_CHECK( reply.conflicts().empty() );
    BOOST_CHECK_EQUAL( reply.snapshot().token(), materialized.copy.snapshot.token.utf8_string() );

    // Only the merge rewrites the git index, so an untouched one shows the update skipped it
    BOOST_CHECK( ReadFile( gitIndex ) == staged );

    std::set<std::string> members;

    for( const mdw::LibraryMember& member : reply.members() )
    {
        BOOST_CHECK_EQUAL( member.library_id(), "passives.kicad_sym" );
        BOOST_CHECK( !member.version().empty() );
        members.insert( member.name() );
    }

    BOOST_CHECK( members == std::set<std::string>( { "C", "L" } ) );

    MDW_LIBRARY_SYNC_RESULT updated = librarian.SyncOk();
    BOOST_CHECK( names( updated.copy ) == passives );
}


// A copy whose index was lost, as an interrupted set-up leaves it, is adopted rather than refused as foreign
BOOST_FIXTURE_TEST_CASE( SyncAdoptsItsUnindexedCopy, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    // A folder the backend did not write may be someone's work, so it is refused untouched
    fs::create_directories( librarian.Copy() );
    fs::copy_file( Seed() / "README.md", librarian.Copy() / "README.md" );

    MDW_LIBRARY_SYNC_RESULT refused = librarian.Sync();
    BOOST_CHECK( refused.result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK( !fs::exists( librarian.Copy() / ".git" ) );

    fs::remove_all( librarian.Copy() );

    librarian.SyncOk();
    BOOST_REQUIRE( librarian.sources->DropCopy( MDW_LIBRARY_SOURCES::Marker( librarian.config ) ).IsOk() );

    MDW_LIBRARY_SYNC_RESULT adopted = librarian.SyncOk();
    BOOST_CHECK( !adopted.created );

    std::set<wxString> names;

    for( const MDW_COPY_MEMBER& member : adopted.copy.members )
        names.insert( member.name );

    BOOST_CHECK( names == std::set<wxString>( { wxS( "C" ), wxS( "L" ) } ) );
}


// A set-up that fails after its backend writes the copy leaves a folder no index vouches for
BOOST_FIXTURE_TEST_CASE( SyncAdoptsCopyOfFailedSetUp, MDW_GIT_LIBRARY_FIXTURE )
{
    const std::string passives = ReadFile( Seed() / "passives.kicad_sym" );

    // Held twice, no symbol can be told from its copy, so the fresh copy cannot be listed
    std::ofstream( Source() / "passives.kicad_sym", std::ios::binary ) << passives << passives;
    Publish( wxS( "Import acme-library twice" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    BOOST_REQUIRE( !librarian.Sync().result.IsOk() );

    std::ofstream( Source() / "passives.kicad_sym", std::ios::binary | std::ios::trunc ) << passives;
    Publish( wxS( "Import acme-library once" ) );

    MDW_LIBRARY_SYNC_RESULT adopted = librarian.SyncOk();
    BOOST_CHECK( !adopted.created );
    BOOST_CHECK_EQUAL( adopted.copy.members.size(), 2u );
}


// A fresh copy tells the client how far its fetch and checkout got, so a long wait shows more than a spinner
BOOST_FIXTURE_TEST_CASE( MaterializeReportsProgress, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    std::mutex                mutex;
    std::condition_variable   heard;
    std::vector<MDW_PROGRESS> reports;

    auto sink = [&]( const MDW_PROGRESS& aProgress )
    {
        std::lock_guard<std::mutex> lock( mutex );
        reports.push_back( aProgress );
        heard.notify_all();
    };

    // As a sync's op does, and held to the end, since a report may reach the client just after the reply
    MDW_PROGRESS_SCOPE    scope( sink );
    MDW_TRACKED_OP        op = MdwBeginOperation( *librarian.source );
    std::shared_ptr<void> watch = op.WatchProgress();

    // A watch holds no object, so it is told from none by its owner count
    BOOST_REQUIRE( watch.use_count() > 0 );

    mdw::MaterializeSharedLibrariesRequest request;
    mdw::MaterializeSharedLibrariesReply   reply;
    request.set_local_root( librarian.Copy().string() );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    MDW_RESULT result = librarian.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::unique_lock<std::mutex> lock( mutex );

    BOOST_REQUIRE( heard.wait_for( lock, std::chrono::seconds( 10 ),
                                   [&]()
                                   {
                                       return std::any_of( reports.begin(), reports.end(),
                                                           []( const MDW_PROGRESS& aReport )
                                                           {
                                                               return aReport.total > 0;
                                                           } );
                                   } ) );

    for( const MDW_PROGRESS& report : reports )
    {
        BOOST_CHECK( report.phase == wxS( "receive" ) || report.phase == wxS( "checkout" )
                     || report.phase == wxS( "index" ) );

        if( report.total > 0 )
            BOOST_CHECK_LE( report.done, report.total );
    }
}


// Releasing the watch lets its owner free what the sink captures, so a sink call already running must end first
BOOST_FIXTURE_TEST_CASE( ReleasingAProgressWatchWaitsForItsSink, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    std::mutex              mutex;
    std::condition_variable changed;
    bool                    entered = false;
    bool                    letGo = false;
    bool                    releasing = false;
    bool                    released = false;
    int                     callsAfterRelease = 0;

    // The first report holds the ingress thread until the test lets it go
    auto sink = [&]( const MDW_PROGRESS& )
    {
        std::unique_lock<std::mutex> lock( mutex );

        if( released )
            callsAfterRelease++;

        if( entered )
            return;

        entered = true;
        changed.notify_all();
        changed.wait_for( lock, std::chrono::seconds( 30 ),
                          [&]()
                          {
                              return letGo;
                          } );
    };

    MDW_PROGRESS_SCOPE    scope( sink );
    MDW_TRACKED_OP        op = MdwBeginOperation( *librarian.source );
    std::shared_ptr<void> watch = op.WatchProgress();
    BOOST_REQUIRE( watch.use_count() > 0 );

    mdw::MaterializeSharedLibrariesRequest request;
    mdw::MaterializeSharedLibrariesReply   reply;
    request.set_local_root( librarian.Copy().string() );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    std::future<MDW_RESULT> call = std::async( std::launch::async,
                                               [&]()
                                               {
                                                   return librarian.backend->CallSync( request, reply, 30000 );
                                               } );

    bool sinkEntered = false;

    {
        std::unique_lock<std::mutex> lock( mutex );
        sinkEntered = changed.wait_for( lock, std::chrono::seconds( 10 ),
                                        [&]()
                                        {
                                            return entered;
                                        } );
    }

    bool releaserStarted = false;
    bool releasedEarly = false;

    if( sinkEntered )
    {
        std::thread releaser(
                [&]()
                {
                    {
                        std::lock_guard<std::mutex> lock( mutex );
                        releasing = true;
                        changed.notify_all();
                    }

                    watch.reset();

                    std::lock_guard<std::mutex> lock( mutex );
                    released = true;
                    changed.notify_all();
                } );

        {
            std::unique_lock<std::mutex> lock( mutex );

            // The window opens only once the releaser is at the release, so a slow thread start cannot pass it
            releaserStarted = changed.wait_for( lock, std::chrono::seconds( 10 ),
                                                [&]()
                                                {
                                                    return releasing;
                                                } );

            releasedEarly = changed.wait_for( lock, std::chrono::milliseconds( 500 ),
                                              [&]()
                                              {
                                                  return released;
                                              } );
            letGo = true;
            changed.notify_all();
        }

        releaser.join();
    }

    MDW_RESULT result = call.get();

    BOOST_REQUIRE( sinkEntered );
    BOOST_CHECK_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( releaserStarted );
    BOOST_CHECK( !releasedEarly );
    BOOST_CHECK( released );
    BOOST_CHECK_EQUAL( callsAfterRelease, 0 );
}

// A project closed during a library transfer closes its session then, not later on the transport's own
// completion thread, which cannot join itself to stop that transport
BOOST_FIXTURE_TEST_CASE( SessionDroppedDuringTransferClosesAtOnce, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();
    const fs::path    copy = librarian.Copy();

    auto                    answer = std::make_shared<std::promise<MDW_RESULT>>();
    std::future<MDW_RESULT> outcome = answer->get_future();

    // The UI thread delivers each completion, so this one cannot wait on the op and runs the queue instead
    auto answered = [&]( std::chrono::milliseconds aTimeout )
    {
        const auto until = std::chrono::steady_clock::now() + aTimeout;

        while( true )
        {
            wxTheApp->ProcessPendingEvents();

            if( outcome.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready )
                return true;

            if( std::chrono::steady_clock::now() >= until )
                return false;

            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
    };

    MDW_OP transfer = librarian.source->Libraries().Materialize( wxString::FromUTF8( copy.string() ),
                                                                 wxEmptyString,
                                                                 [answer]( const MDW_RESULT& aResult,
                                                                           const MDW_LIBRARY_LIST& )
                                                                 {
                                                                     answer->set_value( aResult );
                                                                 } );

    // A transfer already answered has freed its closure, so the session would expire regardless
    BOOST_REQUIRE( !answered( std::chrono::milliseconds( 0 ) ) );

    std::weak_ptr<MDW_SESSION> session = librarian.session;
    librarian.source.reset();
    librarian.backend.reset();
    librarian.session.reset();

    BOOST_CHECK( session.expired() );
    BOOST_REQUIRE( answered( std::chrono::seconds( 30 ) ) );

    // The session's Bye lets a running transfer finish, so it may succeed, but only with the copy in place
    BOOST_CHECK( !outcome.get().IsOk() || fs::exists( copy / "passives.kicad_sym" ) );
}


// A teammate's check-in is announced to the copy's session, which leaves the copy alone until asked
BOOST_FIXTURE_TEST_CASE( PollerAnnouncesUpstreamLibraryCommit, MDW_GIT_LIBRARY_FIXTURE )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR interval( wxS( "KICAD_MDW_POLL_INTERVAL_MS" ), wxS( "100" ) );
    const fs::path                  symdir = "connectors.kicad_symdir";
    const fs::path                  added = symdir / "CONN_2.kicad_sym";

    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();
    MDW_EVENT_BUS&    bus = librarian.session->EventBus();

    std::mutex              mutex;
    std::condition_variable arrived;
    bool                    stalled = true;
    int                     announced = 0;

    // Stall delivery behind a first event as load does, so the sync's own announcement is still queued when the
    // watch begins
    MDW_SUBSCRIPTION stall = bus.Subscribe(
            [&]( const MDW_EVENT& )
            {
                std::unique_lock<std::mutex> lock( mutex );

                arrived.wait_for( lock, std::chrono::seconds( 10 ),
                                  [&]()
                                  {
                                      return !stalled;
                                  } );
            } );

    bus.Publish( MDW_EVENT() );

    librarian.SyncOk();

    fs::create_directories( Source() / symdir );
    fs::copy_file( Seed() / added, Source() / added );
    Publish( wxS( "Add CONN_2" ) );

    MDW_SUBSCRIPTION events = bus.Subscribe(
            [&]( const MDW_EVENT& aEvent )
            {
                std::lock_guard<std::mutex> lock( mutex );

                // The sync announces its own copy too, and only the poller's announcement counts
                if( aEvent.type == MDW_EVENT_TYPE::LIBRARY_CHANGED && !aEvent.from_this_client )
                    ++announced;

                arrived.notify_all();
            } );

    std::unique_lock<std::mutex> lock( mutex );
    stalled = false;
    arrived.notify_all();

    BOOST_REQUIRE( arrived.wait_for( lock, std::chrono::seconds( 10 ),
                                     [&]()
                                     {
                                         return announced > 0;
                                     } ) );

    // Ten passes more, each of which finds the same tip
    BOOST_CHECK( !arrived.wait_for( lock, std::chrono::seconds( 1 ),
                                    [&]()
                                    {
                                        return announced > 1;
                                    } ) );

    BOOST_CHECK( !fs::exists( librarian.Copy() / added ) );
}


// Priming lists a library nobody has asked about, so the first reader of its facts finds them
BOOST_FIXTURE_TEST_CASE( PrimeListsALibraryBeforeAnyoneAsks, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / "passives.kicad_sym", Source() / "passives.kicad_sym" );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    MDW_MEMBER_INDEX index(
            [&]( const wxString& )
            {
                return librarian.source;
            },
            []( wxEvtHandler*, std::function<void( const wxString& )> )
            {
                return MDW_SUBSCRIPTION();
            } );

    MDW_TEAM_LIBRARY binding;
    binding.source = librarian.source;
    binding.library_id = wxS( "passives.kicad_sym" );
    binding.local_root = wxString::FromUTF8( librarian.Copy().string() );

    bool heard = false;
    auto token = index.Listen(
            [&heard]( const MDW_MEMBER_CHANGES& )
            {
                heard = true;
            } );

    index.Prime( { binding } );

    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !heard && std::chrono::steady_clock::now() < until )
    {
        wxTheApp->ProcessPendingEvents();
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    BOOST_REQUIRE( heard );

    // Listed already, so the first read finds the members rather than starting a listing
    BOOST_CHECK( index.Find( binding, wxS( "L" ) ) );
}


// A library check-in carries the author preference, not the host's git identity or a stock name
BOOST_AUTO_TEST_CASE( CheckInCarriesTheAuthorKiCadNames )
{
    COMMON_SETTINGS::GIT prefs{};
    prefs.useDefaultAuthor = false;
    prefs.authorName = wxS( "Alice Librarian" );
    prefs.authorEmail = wxS( "alice@acme.test" );
    SCOPED_AUTHOR_PREFERENCE preference( prefs );

    for( bool hostIdentity : { true, false } )
    {
        BOOST_TEST_CONTEXT( ( hostIdentity ? "with" : "without" ) << " a global git identity" )
        {
            MDW_GIT_LIBRARY_FIXTURE fixture;
            const fs::path          home = fixture.m_temp.Path() / "home";

            fs::create_directories( home );

            if( hostIdentity )
                std::ofstream( home / ".gitconfig" ) << "[user]\n\tname = Host User\n\temail = host@example.com\n";

            // The backend process reads its global config from the environment it inherits
            KI_TEST::SCOPED_PROCESS_ENV_VAR homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );
            KI_TEST::SCOPED_PROCESS_ENV_VAR xdgEnv( wxS( "XDG_CONFIG_HOME" ), wxString::FromUTF8( home.string() ) );
            KI_TEST::SCOPED_GIT_CONFIG_SEARCH_PATH globalConfig( home.string() );

            fs::copy_file( fixture.Seed() / "passives.kicad_sym", fixture.Source() / "passives.kicad_sym" );
            fixture.Publish( wxS( "Import acme-library" ) );
            MDW_GIT_LIBRARIAN librarian = fixture.Connect();

            librarian.SyncOk();

            const fs::path file = librarian.Copy() / "passives.kicad_sym";
            std::string    text = MDW_GIT_LIBRARY_FIXTURE::ReadFile( file );
            const size_t   at = text.find( "\"Inductor\"" );

            BOOST_REQUIRE( at != std::string::npos );
            text.replace( at, 10, "\"Inductor, 10uH\"" );
            std::ofstream( file, std::ios::binary | std::ios::trunc ) << text;

            mdw::CheckInSharedLibraryRequest request;
            mdw::CheckInSharedLibraryReply   reply;
            request.set_local_root( librarian.Copy().string() );
            request.add_item_ids( "passives.kicad_sym#L" );
            request.set_comment( "Name the inductance" );
            request.set_idempotency_id( "inductor" );

            MDW_RESULT result = librarian.backend->CallSync( request, reply, 30000 );
            BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
            BOOST_REQUIRE_EQUAL( reply.results_size(), 1 );
            BOOST_REQUIRE_MESSAGE( !reply.results( 0 ).has_error(), reply.results( 0 ).error().message() );

            git_object* object = nullptr;
            BOOST_REQUIRE_EQUAL( git_revparse_single( &object, fixture.m_upstreamRepo.get(), "main" ), GIT_OK );
            KIGIT::GitObjectPtr objectPtr( object );
            const git_commit*   commit = reinterpret_cast<const git_commit*>( object );

            BOOST_CHECK_EQUAL( git_commit_message( commit ), "Name the inductance" );
            BOOST_CHECK_EQUAL( git_commit_author( commit )->name, "Alice Librarian" );
            BOOST_CHECK_EQUAL( git_commit_author( commit )->email, "alice@acme.test" );
        }
    }
}


// A blank user.name in a copy falls back to the Hello author, since a signature refuses it
BOOST_FIXTURE_TEST_CASE( BlankRepositoryAuthorFallsBackToHello, MDW_GIT_LIBRARY_FIXTURE )
{
    KI_TEST::SCOPED_GIT_CONFIG_SEARCH_PATH globalConfig( m_temp.CreateChildDirStr( wxS( "home" ) ).utf8_string() );
    KIGIT::GitRepositoryPtr                repo =
            KI_TEST::InitGitRepository( wxString::FromUTF8( Source().string() ), "main", " ", "  " );

    kiapi::workspace::Account hello;
    hello.set_display_name( "Alice Librarian" );
    hello.set_email( "alice@acme.test" );

    const GIT_IDENTITY author = MdwGitIdentity( repo.get(), hello, wxString() );

    BOOST_CHECK_EQUAL( author.name, "Alice Librarian" );
    BOOST_CHECK_EQUAL( author.email, "alice@acme.test" );
}


// A set-up leaves nothing in team-libraries but the copy and the .kicad folder the client keeps there
BOOST_FIXTURE_TEST_CASE( SetUpLeavesOnlyTheCopyInTeamLibraries, ONE_LIBRARIAN )
{
    std::set<fs::path> left;

    for( const fs::directory_entry& entry : fs::directory_iterator( m_alice.Copy().parent_path() ) )
        left.insert( entry.path().filename() );

    BOOST_CHECK( left == std::set<fs::path>( { m_alice.Copy().filename(), ".kicad" } ) );
}


// A missing remote reads as no repository, and listing changes once it is back asks it again rather than waiting
BOOST_FIXTURE_TEST_CASE( ListingAsksAReturnedRemoteAgain, ONE_LIBRARIAN )
{
    const fs::path remote = m_temp.Path() / "acme-library.git";
    const fs::path away = m_temp.Path() / "away.git";

    // Windows will not rename a folder the fixture still holds open
    m_upstreamRepo.reset();
    fs::rename( remote, away );
    m_alice.session->Probe( 30000 );

    const MDW_CONNECTION_STATUS offline = m_alice.session->Status();
    BOOST_REQUIRE( offline.phase == MDW_CONNECTION::OFFLINE );
    BOOST_CHECK_EQUAL( offline.reason.message, KIGIT::NoRepositoryAt( m_alice.config.locator ) );

    fs::rename( away, remote );

    const MDW_LIBRARY_CHANGES_RESULT listed = MDW_SETUP::LibraryChangesFor(
            m_alice.source, *m_alice.sources->Copy( MDW_LIBRARY_SOURCES::Marker( m_alice.config ) ),
            KIGIT::LIBRARY_DIFF() );

    BOOST_CHECK_MESSAGE( listed.result.IsOk(), listed.result.message );
}


// A backend that died while idle is respawned under the first sync sent to it, which then still succeeds
BOOST_FIXTURE_TEST_CASE( SyncSurvivesARespawnedBackend, ONE_LIBRARIAN )
{
    SCOPED_DATA_DIR dataDir;
    SCOPED_PGM      program;

    Pgm().SetMdwEnabled( true );
    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();
    BOOST_REQUIRE( framework );

    // The watch starts only once the sync waits on the dead backend, so the respawn always comes under it
    framework->WatchBackends( 0, 0 );
    offerBackend( *framework, dataDir, "org.kicad.mdw.git", MDW_GIT_EXECUTABLE );

    // Only a session the framework holds is respawned, and two backends must not share the copy
    m_alice.source.reset();
    m_alice.backend.reset();
    m_alice.session.reset();

    const MDW_LIBRARY_SOURCE_CONFIG& config = m_alice.config;
    std::shared_ptr<MDW_SESSION>     session = framework->OpenSession(
            m_alice.sources->SessionKey( config ), { config.backend_id, config.name, config.locator }, 8000 );
    BOOST_REQUIRE( session );

    auto source = std::make_shared<MDW_SOURCE>( session );

    // Asked to exit, the backend leaves its supervisor to find it gone, as after a kill
    std::shared_ptr<MDW_BACKEND> dying = session->Backend();
    dying->Bye( 1000 );

    const auto exited = std::chrono::steady_clock::now() + std::chrono::seconds( 20 );

    while( dying->Ping( 200 ) && std::chrono::steady_clock::now() < exited )
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

    BOOST_REQUIRE( !dying->Ping( 200 ) );

    // Listens where the dead backend did, so the sync is known to have reached the old generation
    KINNG_REQUEST_SERVER standIn( session->Supervisor()->IpcUrl( wxS( "control.sock" ) ).utf8_string() );
    std::promise<void>   reached;
    std::future<void>    submitted = reached.get_future();

    standIn.SetCallback(
            [&]( std::string* aMessage )
            {
                kiapi::mdw::MdwRequest envelope;
                envelope.ParseFromString( *aMessage );

                const google::protobuf::Any& payload = envelope.payload();

                // The pings that found the backend gone leave cancels queued, and the sync must get past them
                if( payload.Is<kiapi::mdw::PingRequest>() || payload.Is<kiapi::mdw::CancelRequestRequest>()
                    || payload.Is<kiapi::mdw::ByeRequest>() )
                {
                    standIn.Reply( std::string() );
                    return;
                }

                // Never answered, so nothing else reaches here until the respawn fails it
                reached.set_value();
            } );

    BOOST_REQUIRE( standIn.Start() );

    auto sync = [&]()
    {
        return MDW_SETUP::SyncLibrarySource( *m_alice.sources, source, config, {}, {} );
    };

    std::future<MDW_LIBRARY_SYNC_RESULT> synced = std::async( std::launch::async, sync );

    const bool sent = submitted.wait_for( std::chrono::seconds( 30 ) ) == std::future_status::ready;
    const bool pending = synced.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::timeout;

    framework->WatchBackends( 50, 8000 );

    const bool answered = synced.wait_for( std::chrono::seconds( 60 ) ) == std::future_status::ready;

    // Stopping the framework fails a call still waiting, so the future can always be collected
    if( !answered )
    {
        Pgm().SetMdwEnabled( false );
        Pgm().SetMdwEnabled( true );
    }

    BOOST_REQUIRE( sent );
    BOOST_REQUIRE( pending );
    BOOST_REQUIRE( answered );

    const MDW_LIBRARY_SYNC_RESULT result = synced.get();
    BOOST_CHECK_MESSAGE( result.result.IsOk(), result.result.message );
}

BOOST_AUTO_TEST_SUITE_END()
