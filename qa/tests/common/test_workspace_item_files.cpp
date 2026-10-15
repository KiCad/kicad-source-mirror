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

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialogs/git/workspace_item_files.h>
#include <git/kicad_git_memory.h>
#include <git/project_git_utils.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>

#include <filesystem>
#include <fstream>


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


// A copy of an old revision is named for the file and the revision, so it never lands on the working file
BOOST_AUTO_TEST_CASE( CopyNameCarriesTheRevisionLabel )
{
    MDW_REVISION rev;
    rev.label = wxS( "Baseline 3" );
    BOOST_CHECK_EQUAL( MdwRevisionCopyName( wxS( "board.kicad_pcb" ), wxEmptyString, rev ),
                       wxS( "board-Baseline_3.kicad_pcb" ) );
}


BOOST_AUTO_TEST_CASE( CopyNameOfAMemberUsesTheMember )
{
    MDW_REVISION rev;
    rev.label = wxS( "3" );
    BOOST_CHECK_EQUAL( MdwRevisionCopyName( wxS( "opamps.kicad_sym" ), wxS( "LM358" ), rev ),
                       wxS( "LM358-3.kicad_sym" ) );

    // A member name is backend text, so a separator must not name another folder
    BOOST_CHECK_EQUAL( MdwRevisionCopyName( wxS( "opamps.kicad_sym" ), wxS( "OPA/B" ), rev ),
                       wxS( "OPA_B-3.kicad_sym" ) );
}


BOOST_AUTO_TEST_CASE( CopyNameWithoutALabelUsesTheShortId )
{
    MDW_REVISION rev;
    rev.revision_id = wxS( "a1b2c3d4e5f6" );
    BOOST_CHECK_EQUAL( MdwRevisionCopyName( wxS( "board.kicad_pcb" ), wxEmptyString, rev ),
                       wxS( "board-a1b2c3d.kicad_pcb" ) );
}


BOOST_AUTO_TEST_CASE( LabelWithPathCharactersStaysOneName )
{
    MDW_REVISION rev;
    rev.label = wxS( "A/1: x" );
    BOOST_CHECK_EQUAL( MdwRevisionCopyName( wxS( "board.kicad_pcb" ), wxEmptyString, rev ),
                       wxS( "board-A_1__x.kicad_pcb" ) );
}


BOOST_AUTO_TEST_CASE( CopyOntoTheCheckoutIsRefused )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-copy" ) );
    const wxString                checkout = temp.ChildPathStr( wxS( "board.kicad_pcb" ) );
    const WORKSPACE_WORKING_FILES working( checkout );

    BOOST_CHECK( !MdwCopyTargetRefusal( checkout, working ).IsEmpty() );
    BOOST_CHECK( !MdwCopyTargetRefusal( temp.PathStr() + wxS( "/./board.kicad_pcb" ), working ).IsEmpty() );
    BOOST_CHECK( MdwCopyTargetRefusal( temp.ChildPathStr( wxS( "board-3.kicad_pcb" ) ), working ).IsEmpty() );
}


// After a rename on the server the user still edits the file recorded at check out, which Check In uploads
BOOST_AUTO_TEST_CASE( CopyOntoTheRecordedCheckoutIsRefused )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-copy" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxS( "1841/amp-board.kicad_sch" );
    item.relative_path = wxS( "amp-board-rev-b.kicad_sch" );

    const std::map<wxString, wxString> recorded = { { wxS( "amp-board.kicad_sch" ), item.item_id } };
    const WORKSPACE_WORKING_FILES      working = WorkspaceWorkingFiles( recorded, temp.PathStr(), item );

    const wxString edited = temp.ChildPathStr( wxS( "amp-board.kicad_sch" ) );

    // Compare lists the edited file as the Working copy
    BOOST_CHECK( wxFileName( working.edited ).SameAs( wxFileName( edited ) ) );

    BOOST_CHECK( !MdwCopyTargetRefusal( edited, working ).IsEmpty() );
    BOOST_CHECK( !MdwCopyTargetRefusal( temp.ChildPathStr( wxS( "amp-board-rev-b.kicad_sch" ) ), working ).IsEmpty() );
    BOOST_CHECK( MdwCopyTargetRefusal( temp.ChildPathStr( wxS( "amp-board-v2.kicad_sch" ) ), working ).IsEmpty() );
}


// Git names files from the repository root, so one outside the project has no checkout path and is still edited
BOOST_AUTO_TEST_CASE( CopyOntoARepositoryFileOutsideTheProjectIsRefused )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-copy" ) );
    const wxString           design = temp.ChildPathStr( wxS( "design" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxS( "notes.md" );

    const WORKSPACE_WORKING_FILES working = WorkspaceWorkingFiles( {}, design, item, temp.PathStr() );
    const wxString                edited = temp.ChildPathStr( wxS( "notes.md" ) );

    BOOST_CHECK( !MdwCopyTargetRefusal( edited, working ).IsEmpty() );
    BOOST_CHECK( MdwCopyTargetRefusal( temp.ChildPathStr( wxS( "notes-3.md" ) ), working ).IsEmpty() );

    // The project's read of local changes leaves it out, so its state stays unknown rather than unchanged
    BOOST_CHECK( working.edited.IsEmpty() );

    // An id that would leave the tree protects nothing outside it
    item.item_id = wxS( "../notes.md" );
    BOOST_CHECK( WorkspaceWorkingFiles( {}, design, item, temp.PathStr() ).All().empty() );
}


// With Git integration off the repository may go unfound, and the file outside the project is still edited
BOOST_AUTO_TEST_CASE( CopyOverAnExistingFileIsRefusedWithoutTheRepository )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-copy" ) );
    const wxString           edited = temp.ChildPathStr( wxS( "notes.md" ) );

    MDW_ITEM_INFO item;
    item.item_id = wxS( "notes.md" );

    const WORKSPACE_WORKING_FILES working =
            WorkspaceWorkingFiles( {}, temp.ChildPathStr( wxS( "design" ) ), item, wxString() );

    std::ofstream( temp.Path() / "notes.md" ) << "mine";

    BOOST_CHECK( !MdwCopyTargetRefusal( edited, working ).IsEmpty() );
    BOOST_CHECK( MdwCopyTargetRefusal( temp.ChildPathStr( wxS( "notes-3.md" ) ), working ).IsEmpty() );

    // A checkout recorded in another file leaves the repository original just as unfound
    const WORKSPACE_WORKING_FILES recorded = WorkspaceWorkingFiles( { { wxS( "notes-mine.md" ), item.item_id } },
                                                                    temp.ChildPathStr( wxS( "design" ) ), item,
                                                                    wxString() );

    BOOST_CHECK( !MdwCopyTargetRefusal( edited, recorded ).IsEmpty() );
}


// The check must name the file the writer opens, which follows the link before or after ".." as the platform does
BOOST_AUTO_TEST_CASE( CopyThroughASymlinkedFolderIsRefused )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-copy" ) );
    const std::filesystem::path project = temp.Path() / "project";
    const std::filesystem::path exports = temp.Path() / "exports";
    std::error_code             ec;

    std::filesystem::create_directories( project / "sub" );
    std::filesystem::create_directories( exports );
    std::ofstream( project / "board.kicad_pcb" ) << "mine";

#ifdef _WIN32
    // Win32 drops "link\.." before following the link, so only a path through the link reaches the checkout
    std::filesystem::create_directory_symlink( project, exports / "link", ec );
    const wxString target = FromFsPath( exports / "link" / "board.kicad_pcb" );
#else
    std::filesystem::create_directory_symlink( project / "sub", exports / "link", ec );
    const wxString target = FromFsPath( exports / "link" ) + wxS( "/../board.kicad_pcb" );
#endif

    // Windows needs a privilege to make symbolic links
    if( ec )
    {
        BOOST_TEST_MESSAGE( "No symbolic link, skipped: " << ec.message() );
        return;
    }

    const WORKSPACE_WORKING_FILES working( FromFsPath( project / "board.kicad_pcb" ) );

    BOOST_CHECK( !MdwCopyTargetRefusal( target, working ).IsEmpty() );
}


#ifndef _WIN32
// A folder that cannot be searched says nothing about the file in it, so the copy must not go ahead
BOOST_AUTO_TEST_CASE( CopyThatCannotBeCheckedIsRefused )
{
    KI_TEST::SCOPED_TEMP_DIR    temp( wxS( "mdw-copy" ) );
    const std::filesystem::path locked = temp.Path() / "locked";

    std::filesystem::create_directories( locked );
    std::ofstream( temp.Path() / "board.kicad_pcb" ) << "mine";
    std::filesystem::permissions( locked, std::filesystem::perms::none );

    std::error_code ec;
    const bool      searchable = std::filesystem::exists( locked / "board.kicad_pcb", ec ) || !ec;
    const wxString  refusal =
            MdwCopyTargetRefusal( FromFsPath( locked / "board.kicad_pcb" ),
                                  WORKSPACE_WORKING_FILES( temp.ChildPathStr( wxS( "board.kicad_pcb" ) ) ) );

    std::filesystem::permissions( locked, std::filesystem::perms::owner_all );

    // Root searches any folder, so there is no error to refuse on
    if( searchable )
    {
        BOOST_TEST_MESSAGE( "Folder still searchable, skipped" );
        return;
    }

    BOOST_CHECK( !refusal.IsEmpty() );
}
#endif


BOOST_AUTO_TEST_SUITE_END()
