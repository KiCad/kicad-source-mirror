/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/git_clone_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <qa_utils/git_test_fixture.h>

#include <git2.h>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/filesys.h>


BOOST_AUTO_TEST_SUITE( GitCloneHandler )


// Cloning through KiCad must not leave the team's work tree modified (WIN-45)
BOOST_FIXTURE_TEST_CASE( CloneLeavesWorkTreeClean, KI_TEST::GIT_TEST_FIXTURE )
{
    const wxString sep = wxFileName::GetPathSeparator();
    const wxString sourcePath = TempPath() + sep + wxS( "source" );
    const wxString barePath = TempPath() + sep + wxS( "ctrl-board.git" );
    const wxString clonePath = TempPath() + sep + wxS( "clone" );
    const wxString dataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "common/git/mdw_ctrl_board/" );

    // The campaign's ctrl-board seed keeps its .gitignore under a neutral name in the data directory
    const std::pair<wxString, wxString> files[] = { { wxS( "ctrl-board.kicad_pro" ), wxS( "ctrl-board.kicad_pro" ) },
                                                    { wxS( "gitignore.txt" ), wxS( ".gitignore" ) } };

    for( const auto& [src, dst] : files )
    {
        wxFFile  in( dataDir + src, wxS( "rb" ) );
        wxString contents;

        BOOST_REQUIRE( in.IsOpened() && in.ReadAll( &contents, wxConvISO8859_1 ) );
        KI_TEST::WriteGitFile( sourcePath, dst, std::string( contents.mb_str( wxConvISO8859_1 ) ) );
    }

    {
        KIGIT::GitRepositoryPtr source = KI_TEST::InitGitRepository( sourcePath );
        KI_TEST::CommitAllGitFiles( source.get(), wxS( "Import ctrl-board" ) );

        git_repository*   bare = nullptr;
        git_clone_options options = GIT_CLONE_OPTIONS_INIT;
        options.bare = 1;

        BOOST_REQUIRE_EQUAL( git_clone( &bare, wxFileSystem::FileNameToURL( wxFileName( sourcePath ) ).utf8_str(),
                                        barePath.utf8_str(), &options ),
                             GIT_OK );
        git_repository_free( bare );
    }

    KIGIT_COMMON      common( nullptr );
    GIT_CLONE_HANDLER handler( &common );

    handler.SetRemote( wxFileSystem::FileNameToURL( wxFileName( barePath ) ) );
    handler.SetClonePath( clonePath );

    BOOST_REQUIRE( handler.PerformClone() );
    BOOST_REQUIRE( common.GetRepo() );

    git_status_options statusOptions = GIT_STATUS_OPTIONS_INIT;
    statusOptions.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusOptions.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    git_status_list* statusList = nullptr;
    BOOST_REQUIRE_EQUAL( git_status_list_new( &statusList, common.GetRepo(), &statusOptions ), GIT_OK );
    KIGIT::GitStatusListPtr statusPtr( statusList );

    BOOST_CHECK_EQUAL( git_status_list_entrycount( statusList ), 0 );

    wxFFile  attributes( clonePath + sep + wxS( ".git/info/attributes" ), wxS( "rb" ) );
    wxString attributesText;

    BOOST_REQUIRE( attributes.IsOpened() && attributes.ReadAll( &attributesText ) );
    BOOST_CHECK( attributesText.Contains( wxS( "*.kicad_pcb merge=kicad-pcb" ) ) );
    BOOST_CHECK( attributesText.Contains( wxS( "*.kicad_sch merge=kicad-sch" ) ) );

    git_repository_free( common.GetRepo() );
    common.SetRepo( nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
