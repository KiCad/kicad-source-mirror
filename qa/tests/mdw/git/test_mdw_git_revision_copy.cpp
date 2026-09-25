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

#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_revision_copy.h>
#include <mdw/client/mdw_source.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;


namespace
{

std::string readFile( const fs::path& aPath )
{
    std::ifstream      in( aPath, std::ios::binary );
    std::ostringstream text;

    text << in.rdbuf();
    return text.str();
}


std::vector<fs::path> filesIn( const fs::path& aDir )
{
    std::vector<fs::path> files;

    for( const fs::directory_entry& entry : fs::directory_iterator( aDir ) )
        files.push_back( entry.path() );

    return files;
}


std::pair<MDW_RESULT, MDW_REVISION> saveCopy( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                                              const wxString& aRevisionId, const fs::path& aTarget,
                                              bool aReplace = true )
{
    return MdwSync<MDW_REVISION>(
            [&]( MDW_DONE<MDW_REVISION> aDone )
            {
                return MdwSaveRevisionCopy( aSource, aItem, aRevisionId, wxString::FromUTF8( aTarget.string() ),
                                            aReplace, std::move( aDone ) );
            } );
}


// A bound project whose board file is committed twice
class PROJECT_HISTORY_FIXTURE : public KI_TEST::GIT_TEST_FIXTURE
{
public:
    PROJECT_HISTORY_FIXTURE() :
            m_dataDir( wxS( "KICAD_MDW_DATA_DIR" ), TempPath() + wxFileName::GetPathSeparator() + wxS( "data" ) )
    {
        const fs::path temp( TempPath().utf8_string() );
        const fs::path origin = temp / "origin.git";

        m_tree = temp / "work";
        m_targetDir = temp / "out";
        fs::create_directories( m_targetDir );

        git_repository* bare = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_init( &bare, origin.string().c_str(), 1 ), GIT_OK );
        git_repository_free( bare );

        m_repo = KI_TEST::InitGitRepository( wxString::FromUTF8( m_tree.string() ), "main", "carol",
                                             "carol@acme.test" );

        git_remote* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_remote_create( &remote, m_repo.get(), "origin", origin.string().c_str() ), GIT_OK );
        git_remote_free( remote );

        std::ofstream( m_tree / "board.kicad_pro" ) << "{}";
        std::ofstream( m_tree / "board.kicad_pcb", std::ios::binary ) << "rev1";
        commit( "Add the board" );

        std::ofstream( m_tree / "board.kicad_pcb", std::ios::binary | std::ios::trunc ) << "rev2";
        commit( "Change the board" );

        MDW_BACKEND_CONTEXT context;
        context.workspaceId = wxS( "acme" );
        context.projectPath = wxString::FromUTF8( ( m_tree / "board.kicad_pro" ).string() );

        auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( gitManifest(), context );
        BOOST_REQUIRE( supervisor->Launch( 8000 ) );

        m_session = std::make_shared<MDW_SESSION>(
                MDW_PROJECT_BINDING{ wxS( "org.kicad.mdw.git" ), context.workspaceId, wxEmptyString },
                std::move( supervisor ) );
        m_source = std::make_shared<MDW_SOURCE>( m_session );
        m_item.item_id = wxS( "board.kicad_pcb" );
    }

    /// The board's revisions, newest first
    std::vector<MDW_REVISION> History()
    {
        auto [result, page] = MdwSync<MDW_PAGED<MDW_REVISION>>(
                [&]( MDW_DONE<MDW_PAGED<MDW_REVISION>> aDone )
                {
                    return m_source->History( m_item, MDW_PAGE(), std::move( aDone ) );
                } );

        BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
        return page.rows;
    }

    fs::path                     m_tree;
    fs::path                     m_targetDir;
    std::shared_ptr<MDW_SESSION> m_session;
    std::shared_ptr<MDW_SOURCE>  m_source;
    MDW_ITEM_REF                 m_item;

private:
    void commit( const char* aMessage )
    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, m_repo.get() ), GIT_OK );
        KIGIT::GitIndexPtr indexPtr( index );

        for( const char* path : { "board.kicad_pro", "board.kicad_pcb" } )
            BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, path ), GIT_OK );

        BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
        KI_TEST::CommitGitIndex( m_repo.get(), wxString::FromUTF8( aMessage ) );
    }

    KI_TEST::SCOPED_PROCESS_ENV_VAR m_dataDir;
    KIGIT::GitRepositoryPtr         m_repo;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitRevisionCopy )


// History lists the newest first, so the copy of the older entry is the board as first committed
BOOST_FIXTURE_TEST_CASE( OldRevisionCopyHoldsThatRevision, PROJECT_HISTORY_FIXTURE )
{
    const std::vector<MDW_REVISION> history = History();
    BOOST_REQUIRE_EQUAL( history.size(), 2u );

    const fs::path target = m_targetDir / "board-copy.kicad_pcb";
    auto [result, revision] = saveCopy( m_source, m_item, history[1].revision_id, target );

    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK_EQUAL( readFile( target ), "rev1" );
    BOOST_CHECK( revision.revision_id == history[1].revision_id );
    BOOST_CHECK_EQUAL( filesIn( m_targetDir ).size(), 1u );
}


// A refused download must not touch the user's file, nor leave its partial file beside it
BOOST_FIXTURE_TEST_CASE( FailedCopyLeavesTheTargetAlone, PROJECT_HISTORY_FIXTURE )
{
    const fs::path target = m_targetDir / "board-copy.kicad_pcb";

    std::ofstream( target, std::ios::binary ) << "mine";

    auto [result, revision] =
            saveCopy( m_source, m_item, wxS( "0000000000000000000000000000000000000000" ), target );

    BOOST_CHECK( !result.IsOk() );
    BOOST_CHECK_EQUAL( readFile( target ), "mine" );
    BOOST_CHECK_EQUAL( filesIn( m_targetDir ).size(), 1u );
}


// A file another program makes during the download was never offered for replacing, so it is kept
BOOST_FIXTURE_TEST_CASE( CopyKeepsAFileMadeDuringTheDownload, PROJECT_HISTORY_FIXTURE )
{
    const fs::path target = m_targetDir / "board-copy.kicad_pcb";

    std::ofstream( target, std::ios::binary ) << "theirs";

    auto [result, revision] = saveCopy( m_source, m_item, History()[1].revision_id, target, false );

    BOOST_CHECK( !result.IsOk() );
    BOOST_CHECK_EQUAL( readFile( target ), "theirs" );
    BOOST_CHECK_EQUAL( filesIn( m_targetDir ).size(), 1u );
}


// Whichever of the cancel and the download wins, the target is whole and no partial file is left beside it
BOOST_FIXTURE_TEST_CASE( CancelledCopyLeavesNoPartFile, PROJECT_HISTORY_FIXTURE )
{
    const fs::path target = m_targetDir / "board-copy.kicad_pcb";

    std::ofstream( target, std::ios::binary ) << "mine";

    {
        MDW_OP op = MdwSaveRevisionCopy( m_source, m_item, History()[1].revision_id,
                                         wxString::FromUTF8( target.string() ), true, {} );

        op.Cancel();
    }

    // A later round trip, so the backend has dealt with the abandoned request
    History();

    // The detached download removes its part file when its callback runs, which is not ordered with that round trip
    const auto giveUp = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( filesIn( m_targetDir ).size() > 1u && std::chrono::steady_clock::now() < giveUp )
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

    const std::string text = readFile( target );

    BOOST_CHECK_MESSAGE( text == "mine" || text == "rev1", "target holds \"" << text << "\"" );
    BOOST_CHECK_EQUAL( filesIn( m_targetDir ).size(), 1u );
}


// Stopping the client workers drops the copy's cleanup, which leaves its partial file to the sweep, along with
// the staging file a backend killed mid-write would leave beside it
BOOST_FIXTURE_TEST_CASE( SweepRemovesAnUnsettledPartFile, PROJECT_HISTORY_FIXTURE )
{
    const fs::path target = m_targetDir / "board-copy.kicad_pcb";
    const wxString revisionId = History()[1].revision_id;

    std::ofstream( target, std::ios::binary ) << "mine";

    // Later cases need the client workers however this one ends
    struct WORKER_RESTART
    {
        ~WORKER_RESTART() { MdwStartWorkers(); }
    } restart;

    BOOST_REQUIRE( MdwStopWorkers( std::chrono::seconds( 5 ) ) );

    MDW_OP op = MdwSaveRevisionCopy( m_source, m_item, revisionId, wxString::FromUTF8( target.string() ), true, {} );

    // The backend's own staging file ends in .tmp, and only its rename lands the part file
    auto partWritten = [&]() -> std::optional<fs::path>
    {
        for( const fs::path& file : filesIn( m_targetDir ) )
        {
            const std::string name = file.filename().string();

            if( name.find( ".part" ) != std::string::npos && file.extension() != ".tmp" )
                return file;
        }

        return std::nullopt;
    };

    const auto giveUp = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );

    while( !partWritten() && std::chrono::steady_clock::now() < giveUp )
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

    std::optional<fs::path> part = partWritten();
    BOOST_REQUIRE( part );

    // The first as MDW_ReplaceFile names it, the second a name it never writes
    const fs::path staging = part->string() + ".kicad-mdw-1234.tmp";
    const fs::path notes = part->string() + ".kicad-mdw-notes.tmp";

    std::ofstream( staging, std::ios::binary ) << "rev";
    std::ofstream( notes, std::ios::binary ) << "notes";
    BOOST_REQUIRE( fs::exists( staging ) );

    BOOST_CHECK( MdwSweepRevisionCopies( std::chrono::seconds( 5 ) ) );

    BOOST_CHECK_EQUAL( readFile( target ), "mine" );
    BOOST_CHECK( fs::exists( notes ) );
    BOOST_CHECK_EQUAL( filesIn( m_targetDir ).size(), 2u );
}


// Only the member's own text lands in the file, not the library it lives in
BOOST_FIXTURE_TEST_CASE( MemberCopyHoldsOnlyThatMember, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    Publish( wxS( "Import acme-library" ) );

    MDW_GIT_LIBRARIAN librarian = Connect();
    librarian.SyncOk();

    MDW_ITEM_REF member;
    member.item_id = wxString::FromUTF8( L );

    const fs::path target = m_temp.Path() / "member.kicad_sym";
    auto [result, revision] = saveCopy( librarian.source, member, wxEmptyString, target );

    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    const std::string text = readFile( target );

    BOOST_CHECK( text.find( "(symbol \"L\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "(symbol \"C\"" ) == std::string::npos );
}


BOOST_AUTO_TEST_SUITE_END()
