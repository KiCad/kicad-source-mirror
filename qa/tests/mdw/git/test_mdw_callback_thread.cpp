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

#include <mdw/client/mdw_source.h>

#include <qa_utils/git_test_fixture.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>


namespace
{

/// The threads a callback's captures were copied, moved or destroyed on
struct THREAD_LOG
{
    std::mutex                   mutex;
    std::vector<std::thread::id> threads;

    void Note()
    {
        std::lock_guard<std::mutex> lock( mutex );
        threads.push_back( std::this_thread::get_id() );
    }
};


/// Stands in for a capture such as wxWeakRef, which may only be touched on the thread that delivers the callback
struct TRACKER
{
    explicit TRACKER( std::shared_ptr<THREAD_LOG> aLog ) :
            log( std::move( aLog ) )
    {
    }

    TRACKER( const TRACKER& aOther ) :
            log( aOther.log )
    {
        log->Note();
    }

    TRACKER( TRACKER&& aOther ) noexcept :
            log( aOther.log )
    {
        log->Note();
    }

    ~TRACKER() { log->Note(); }

    std::shared_ptr<THREAD_LOG> log;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwCallbackThread )


// The transport answers on its own thread, which must neither copy nor destroy the caller's callback
BOOST_FIXTURE_TEST_CASE( CallbackLivesOnlyWhereItStartsAndIsDelivered, KI_TEST::GIT_TEST_FIXTURE )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR dataDir( wxS( "KICAD_MDW_DATA_DIR" ),
                                             TempPath() + wxFileName::GetPathSeparator() + wxS( "data" ) );

    const wxString          tree = TempPath() + wxFileName::GetPathSeparator() + wxS( "work" );
    KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( tree );

    KI_TEST::WriteGitFile( tree, wxS( "board.kicad_pro" ), "text" );
    KI_TEST::CommitAllGitFiles( repo.get(), wxS( "Add the design" ) );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "acme" );
    context.projectPath = tree + wxFileName::GetPathSeparator() + wxS( "board.kicad_pro" );

    std::shared_ptr<MDW_SESSION> session = gitSession( context );
    auto                         source = std::make_shared<MDW_SOURCE>( session );
    auto                         log = std::make_shared<THREAD_LOG>();
    const std::thread::id        caller = std::this_thread::get_id();
    std::thread::id              deliveredOn;
    MDW_RESULT                   result = MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL );

    {
        // Delivered on the client worker, so the caller can wait here for it
        MDW_SYNC_SCOPE scope;

        MDW_OP op = source->RootFolders( MDW_PAGE(),
                                         [tracker = TRACKER( log ), &deliveredOn,
                                          &result]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_FOLDER>& )
                                         {
                                             deliveredOn = std::this_thread::get_id();
                                             result = aResult;
                                         } );
        op.Wait();
    }

    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE( deliveredOn != caller );

    std::lock_guard<std::mutex> lock( log->mutex );

    BOOST_REQUIRE( !log->threads.empty() );
    BOOST_CHECK( log->threads.back() == deliveredOn );

    BOOST_CHECK( std::all_of( log->threads.begin(), log->threads.end(),
                              [&]( const std::thread::id& aThread )
                              {
                                  return aThread == caller || aThread == deliveredOn;
                              } ) );
}


BOOST_AUTO_TEST_SUITE_END()
