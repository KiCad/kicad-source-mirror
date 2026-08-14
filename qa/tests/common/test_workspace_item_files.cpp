/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/git/workspace_item_files.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>


BOOST_AUTO_TEST_SUITE( WorkspaceItemFiles )


// An Odoo schematic whose display path is not where it checks out to
BOOST_AUTO_TEST_CASE( CheckoutPathIgnoresDisplayPath )
{
    wxFileName project = wxFileName::DirName( wxFileName::GetTempDir() );
    project.AppendDir( wxT( "amp-odoo" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxT( "1841/amp-board.kicad_sch" );
    item.path_hint = wxT( "ACME MAC / Projects / PCBA-AMP-001 / amp-board.kicad_sch" );

    BOOST_CHECK( WorkspaceCheckoutPath( project.GetPath(), item ).IsEmpty() );

    item.relative_path = wxT( "amp-board.kicad_sch" );

    BOOST_CHECK_EQUAL( WorkspaceCheckoutPath( project.GetPath(), item ),
                       wxFileName( project.GetPath(), wxT( "amp-board.kicad_sch" ) ).GetFullPath() );

    item.relative_path = wxT( "../amp-board.kicad_sch" );

    BOOST_CHECK( WorkspaceCheckoutPath( project.GetPath(), item ).IsEmpty() );
}


// A .kicad_prl uploaded through Odoo's web pages would replace this copy's checkout record
BOOST_AUTO_TEST_CASE( CheckoutPathRefusesLocalSettings )
{
    wxFileName project = wxFileName::DirName( wxFileName::GetTempDir() );
    project.AppendDir( wxT( "amp-odoo" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxT( "1841/amp-board.kicad_prl" );
    item.relative_path = wxT( "amp-board.kicad_prl" );

    BOOST_CHECK( WorkspaceCheckoutPath( project.GetPath(), item ).IsEmpty() );

    item.relative_path = wxT( "amp-board.KICAD_PRL" );

    BOOST_CHECK( WorkspaceCheckoutPath( project.GetPath(), item ).IsEmpty() );
}


// Two Odoo products may both hold an amp-board.kicad_sch, and only one can be the file at the project root
BOOST_AUTO_TEST_CASE( CheckinPathSkipsAnotherItemsFile )
{
    wxFileName project = wxFileName::DirName( wxFileName::GetTempDir() );
    project.AppendDir( wxT( "amp-odoo" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxT( "1841/amp-board.kicad_sch" );
    item.relative_path = wxT( "amp-board.kicad_sch" );

    std::map<wxString, wxString> recorded = { { wxT( "amp-board.kicad_sch" ), wxT( "1907/amp-board.kicad_sch" ) } };

    BOOST_CHECK( WorkspaceCheckinPath( recorded, project.GetPath(), item ).IsEmpty() );
}


// Odoo's attachment id is no version number, its label is
BOOST_AUTO_TEST_CASE( RevisionTextPrefersLabel )
{
    MDW_REVISION revision;
    revision.revision_id = wxT( "28140" );
    revision.label = wxT( "3" );

    BOOST_CHECK_EQUAL( WorkspaceRevisionText( revision ), wxT( "3" ) );

    // Without a label the id shows whole, as for a git commit
    revision.revision_id = wxT( "09e9d5090bbab23653af5358784909af9d37a085" );
    revision.label.clear();

    BOOST_CHECK_EQUAL( WorkspaceRevisionText( revision ), revision.revision_id );
}


// A folder is named with its parents, never by its id
BOOST_AUTO_TEST_CASE( FolderNameJoinsParents )
{
    MDW_FOLDER root;
    root.folder_id = wxT( "9" );
    root.display_name = wxT( "ACME MAC" );

    MDW_FOLDER projects;
    projects.folder_id = wxT( "27" );
    projects.display_name = wxT( "Projects" );

    wxString rootName = WorkspaceFolderName( wxEmptyString, root );

    BOOST_CHECK_EQUAL( rootName, wxT( "ACME MAC" ) );
    BOOST_CHECK_EQUAL( WorkspaceFolderName( rootName, projects ), wxT( "ACME MAC / Projects" ) );
}


// Keep Both saves the stale file it refuses beside the original, and an earlier copy must survive a second refusal
BOOST_AUTO_TEST_CASE( KeepBothNeverReplacesACopy )
{
    // The transient pathspec is built on first use and needs libgit2 up when no git suite ran first
    KIGIT::LIBGIT2_SCOPE libgit2;

    const wxString seed = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() )
                          + wxT( "/common/git/mdw_ctrl_board/ctrl-board.kicad_sch" );
    wxFileName     dir = wxFileName::DirName( wxFileName::GetTempDir() );

    dir.AppendDir( wxString::Format( wxT( "qa_keep_both_%lu" ), wxGetProcessId() ) );
    BOOST_REQUIRE( dir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    const wxString file = wxFileName( dir.GetPath(), wxT( "amp-board.kicad_sch" ) ).GetFullPath();
    const wxString first = WorkspaceKeepBothPath( file );

    BOOST_CHECK_EQUAL( first, wxFileName( dir.GetPath(), wxT( "amp-board.kicad_sch.mine" ) ).GetFullPath() );
    BOOST_REQUIRE( wxCopyFile( seed, first ) );

    const wxString second = WorkspaceKeepBothPath( file );

    BOOST_CHECK_EQUAL( second, wxFileName( dir.GetPath(), wxT( "amp-board.kicad_sch.mine.2" ) ).GetFullPath() );

    // A copy that read as a design would be published by the next Commit All
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( first ) ) );
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( second ) ) );

    wxFileName::Rmdir( dir.GetPath(), wxPATH_RMDIR_RECURSIVE );
}


// The Compare header names the file and its version, not the item or revision id
BOOST_AUTO_TEST_CASE( VersionLabelNamesTheFile )
{
    MDW_ITEM_REF item;
    item.item_id = wxT( "2183/amp-board.kicad_sch" );

    MDW_REVISION revision;
    revision.revision_id = wxT( "28439" );
    revision.label = wxT( "1" );

    BOOST_CHECK_EQUAL( MdwVersionSideLabel( item, revision ), wxT( "amp-board.kicad_sch v1" ) );

    // The hint is a display path, and the file is its last part
    item.path_hint = wxT( "ACME MAC / Projects / PCBA-AMP-001 / amp-board.kicad_sch" );

    BOOST_CHECK_EQUAL( MdwVersionSideLabel( item, revision ), wxT( "amp-board.kicad_sch v1" ) );

    revision.kind = MDW_REVISION_KIND::RELEASE;
    revision.label = wxT( "3" );

    BOOST_CHECK_EQUAL( MdwVersionSideLabel( item, revision ), wxT( "amp-board.kicad_sch Rev 3" ) );
}


BOOST_AUTO_TEST_SUITE_END()
