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

#include <dialogs/git/workspace_item_files.h>
#include <dialogs/git/workspace_listing.h>
#include <mdw/client/mdw_source.h>

#include <qa_utils/git_test_fixture.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <set>


BOOST_AUTO_TEST_SUITE( MdwGitWorkspaceListing )


// The git root folder has an empty id, so asking for its children must not hand back the root again
BOOST_FIXTURE_TEST_CASE( ListingWalksNestedFoldersOnce, KI_TEST::GIT_TEST_FIXTURE )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR dataDir( wxS( "KICAD_MDW_DATA_DIR" ),
                                             TempPath() + wxFileName::GetPathSeparator() + wxS( "data" ) );

    const wxString          tree = TempPath() + wxFileName::GetPathSeparator() + wxS( "work" );
    KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( tree );

    for( const wxString& path : { wxS( "board.kicad_pro" ), wxS( "notes.txt" ), wxS( "a/top.kicad_sch" ),
                                  wxS( "a/b/file.kicad_sch" ), wxS( "c/readme.txt" ) } )
    {
        KI_TEST::WriteGitFile( tree, path, "text" );
    }

    KI_TEST::CommitAllGitFiles( repo.get(), wxS( "Add the design" ) );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "acme" );
    context.projectPath = tree + wxFileName::GetPathSeparator() + wxS( "board.kicad_pro" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( gitManifest(), context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    auto session = std::make_shared<MDW_SESSION>(
            MDW_PROJECT_BINDING{ wxS( "org.kicad.mdw.git" ), context.workspaceId, wxEmptyString },
            std::move( supervisor ) );
    auto source = std::make_shared<MDW_SOURCE>( session );

    // A regression walks forever, so give up and stop it rather than hang the suite
    auto                           stop = std::make_shared<std::atomic<bool>>( false );
    std::future<WORKSPACE_LISTING> walk = std::async( std::launch::async,
                                                      [&]()
                                                      {
                                                          return ReadWorkspaceListing( source, stop );
                                                      } );

    bool finished = walk.wait_for( std::chrono::seconds( 60 ) ) == std::future_status::ready;
    stop->store( true );

    WORKSPACE_LISTING listing = walk.get();

    BOOST_REQUIRE_MESSAGE( finished, "the walk did not finish" );
    BOOST_REQUIRE_MESSAGE( listing.status.IsOk(), listing.status.message );

    std::map<wxString, wxString> parents;

    for( const MDW_FOLDER& folder : listing.folders )
        BOOST_CHECK_MESSAGE( parents.emplace( folder.folder_id, folder.parent_folder_id ).second, folder.folder_id );

    const std::map<wxString, wxString> expectedParents = { { wxS( "a" ), wxEmptyString },
                                                           { wxS( "a/b" ), wxS( "a" ) },
                                                           { wxS( "c" ), wxEmptyString } };

    BOOST_CHECK( parents == expectedParents );
    BOOST_CHECK_EQUAL( listing.folderNames[wxS( "a/b" )], wxS( "a / b" ) );

    std::set<wxString> items;

    for( const MDW_ITEM_INFO& item : listing.items )
        BOOST_CHECK_MESSAGE( items.insert( item.item_id ).second, item.item_id );

    const std::set<wxString> expectedItems = { wxS( "board.kicad_pro" ), wxS( "notes.txt" ), wxS( "a/top.kicad_sch" ),
                                               wxS( "a/b/file.kicad_sch" ), wxS( "c/readme.txt" ) };

    BOOST_CHECK( items == expectedItems );
}


// Open and Show in Folder resolve an item's relative path against the project, which may sit below the repository
BOOST_FIXTURE_TEST_CASE( RelativePathIsUnderTheProject, KI_TEST::GIT_TEST_FIXTURE )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR dataDir( wxS( "KICAD_MDW_DATA_DIR" ),
                                             TempPath() + wxFileName::GetPathSeparator() + wxS( "data" ) );

    const wxString          tree = TempPath() + wxFileName::GetPathSeparator() + wxS( "work" );
    const wxString          design = tree + wxFileName::GetPathSeparator() + wxS( "design" );
    KIGIT::GitRepositoryPtr repo = KI_TEST::InitGitRepository( tree );

    for( const wxString& path : { wxS( "notes.md" ), wxS( "design/board.kicad_pro" ), wxS( "design/notes.md" ) } )
        KI_TEST::WriteGitFile( tree, path, "text" );

#ifndef __WINDOWS__
    // A legal POSIX name, which a local path would read as design/a/b.md
    KI_TEST::WriteGitFile( tree, wxS( "design/a\\b.md" ), "text" );
    KI_TEST::WriteGitFile( tree, wxS( "design/a/b.md" ), "text" );
#endif

    KI_TEST::CommitAllGitFiles( repo.get(), wxS( "Add the design" ) );

    MDW_BACKEND_CONTEXT context;
    context.workspaceId = wxS( "acme" );
    context.projectPath = design + wxFileName::GetPathSeparator() + wxS( "board.kicad_pro" );

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( gitManifest(), context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    auto session = std::make_shared<MDW_SESSION>(
            MDW_PROJECT_BINDING{ wxS( "org.kicad.mdw.git" ), context.workspaceId, wxEmptyString },
            std::move( supervisor ) );
    auto source = std::make_shared<MDW_SOURCE>( session );

    WORKSPACE_LISTING listing = ReadWorkspaceListing( source, nullptr );

    BOOST_REQUIRE_MESSAGE( listing.status.IsOk(), listing.status.message );

    std::map<wxString, MDW_ITEM_INFO> items;

    for( const MDW_ITEM_INFO& item : listing.items )
        items[item.item_id] = item;

    BOOST_REQUIRE( items.count( wxS( "design/notes.md" ) ) && items.count( wxS( "notes.md" ) ) );

    BOOST_CHECK_EQUAL( items[wxS( "design/notes.md" )].relative_path, wxS( "notes.md" ) );
    BOOST_CHECK( wxFileName( WorkspaceCheckoutPath( design, items[wxS( "design/notes.md" )] ) )
                         .SameAs( wxFileName( design, wxS( "notes.md" ) ) ) );

    // The repository root's copy is outside the project, so it has no file there to open
    BOOST_CHECK( items[wxS( "notes.md" )].relative_path.IsEmpty() );
    BOOST_CHECK( WorkspaceCheckoutPath( design, items[wxS( "notes.md" )] ).IsEmpty() );

#ifndef __WINDOWS__
    BOOST_REQUIRE( items.count( wxS( "design/a\\b.md" ) ) );
    BOOST_CHECK( items[wxS( "design/a\\b.md" )].relative_path.IsEmpty() );
    BOOST_CHECK( WorkspaceCheckoutPath( design, items[wxS( "design/a\\b.md" )] ).IsEmpty() );
#endif
}


// A page closed before its walk began must not send the backend a single request
BOOST_AUTO_TEST_CASE( CancelledListingReadsNothing )
{
    auto source = std::make_shared<MDW_SOURCE>( std::weak_ptr<MDW_SESSION>() );
    auto cancelled = std::make_shared<std::atomic<bool>>( true );

    WORKSPACE_LISTING listing = ReadWorkspaceListing( source, cancelled );

    BOOST_CHECK( listing.status.error == MDW_ERROR_CODE::CANCELLED );
    BOOST_CHECK( listing.folders.empty() );
    BOOST_CHECK( listing.items.empty() );
}


// A backend may leave an item's folder empty, and the tree would then show it at the top
BOOST_AUTO_TEST_CASE( ItemTakesTheFolderItWasListedIn )
{
    WORKSPACE_LISTING_READS reads;

    reads.rootFolders = []( std::vector<MDW_FOLDER>& aOut )
    {
        MDW_FOLDER folder;
        folder.folder_id = wxS( "f1" );
        folder.display_name = wxS( "parts" );
        aOut.push_back( folder );
        return MDW_RESULT::Ok();
    };

    reads.childFolders = []( const wxString&, std::vector<MDW_FOLDER>& )
    {
        return MDW_RESULT::Ok();
    };

    reads.items = []( const wxString& aFolderId, std::vector<MDW_ITEM_INFO>& aOut )
    {
        MDW_ITEM_INFO item;
        item.item_id = aFolderId + wxS( "/item" );
        aOut.push_back( item );
        return MDW_RESULT::Ok();
    };

    WORKSPACE_LISTING listing = ReadWorkspaceListing( reads );

    BOOST_REQUIRE_MESSAGE( listing.status.IsOk(), listing.status.message );
    BOOST_REQUIRE_EQUAL( listing.items.size(), 1u );
    BOOST_CHECK_EQUAL( listing.items[0].folder_id, wxS( "f1" ) );
}


BOOST_AUTO_TEST_SUITE_END()
