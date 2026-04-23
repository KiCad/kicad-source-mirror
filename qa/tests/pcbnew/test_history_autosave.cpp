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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <local_history.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>

#include <git2.h>

#include <vector>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


namespace
{
// single_top handles libgit2 init in production; the QA harness does not, so tests driving
// LOCAL_HISTORY must manage it themselves.
struct LIBGIT2_SCOPE
{
    LIBGIT2_SCOPE() { git_libgit2_init(); }
    ~LIBGIT2_SCOPE() { git_libgit2_shutdown(); }
};


struct SCOPED_BOOL_OVERRIDE
{
    explicit SCOPED_BOOL_OVERRIDE( bool& aFlag ) : m_flag( aFlag ), m_original( aFlag ) {}
    ~SCOPED_BOOL_OVERRIDE() { m_flag = m_original; }

    bool& m_flag;
    bool  m_original;
};


// Recursively remove the directory on destruction so test failures (which throw out of
// BOOST_REQUIRE) do not leak temp directories.
struct SCOPED_TEMP_DIR
{
    explicit SCOPED_TEMP_DIR( const wxString& aPrefix )
    {
        wxString base = wxStandardPaths::Get().GetTempDir();
        m_path = base + wxFileName::GetPathSeparator() + aPrefix
                 + wxString::Format( wxS( "_%lu_%ld" ),
                                     static_cast<unsigned long>( ::wxGetProcessId() ),
                                     static_cast<long>( wxDateTime::UNow().GetTicks() ) );
        wxFileName::Mkdir( m_path, 0777, wxPATH_MKDIR_FULL );
    }

    ~SCOPED_TEMP_DIR()
    {
        if( !m_path.IsEmpty() && wxDirExists( m_path ) )
            wxFileName::Rmdir( m_path, wxPATH_RMDIR_RECURSIVE );
    }

    const wxString& Path() const { return m_path; }

    wxString m_path;
};
}  // namespace


BOOST_AUTO_TEST_SUITE( PcbHistoryAutosave )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23737
 *
 * After importing a non-KiCad board, BOARD::SaveToHistory could be invoked from the autosave
 * timer while the board's project pointer was transiently null (between unloading the previous
 * project and linking the new one). The function then dereferenced GetProject() and crashed
 * with EXCEPTION_ACCESS_VIOLATION_READ. SaveToHistory must tolerate a null project and bail
 * out cleanly without crashing.
 */
BOOST_AUTO_TEST_CASE( SaveToHistoryWithNullProjectDoesNotCrash )
{
    BOARD                          board;
    std::vector<HISTORY_FILE_DATA> fileData;

    BOOST_REQUIRE( board.GetProject() == nullptr );

    BOOST_CHECK_NO_THROW( board.SaveToHistory( wxS( "/tmp/anywhere" ), fileData ) );
    BOOST_CHECK( fileData.empty() );
}


/**
 * SaveToHistory with a real project but no board filename should not produce any file data
 * (the board is unsaved). It must also not crash.
 */
BOOST_AUTO_TEST_CASE( SaveToHistoryUnsavedBoardProducesNothing )
{
    SETTINGS_MANAGER mgr;

    wxString tempDir = wxStandardPaths::Get().GetTempDir();
    wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxS( "pcb_autosave.kicad_pro" );

    mgr.LoadProject( projectPath.ToStdString() );

    BOARD board;
    board.SetProject( &mgr.Prj() );

    std::vector<HISTORY_FILE_DATA> fileData;

    BOOST_REQUIRE( board.GetFileName().IsEmpty() );
    BOOST_CHECK_NO_THROW( board.SaveToHistory( mgr.Prj().GetProjectPath(), fileData ) );
    BOOST_CHECK( fileData.empty() );

    // Detach project before BOARD destruction so design settings ownership unwinds cleanly
    board.ClearProject();
    mgr.UnloadProject( &mgr.Prj(), false );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24016
BOOST_AUTO_TEST_CASE( RestoreCommitPreservesZipBackupsDirectory )
{
    LIBGIT2_SCOPE libgit;

    // LOCAL_HISTORY early-exits when backups are disabled.
    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR tempProject( wxS( "kicad_qa_issue24016" ) );
    const wxString& projectPath = tempProject.Path();

    auto writeFile = []( const wxString& aPath, const wxString& aContents )
    {
        wxFFile f( aPath, wxT( "w" ) );
        BOOST_REQUIRE( f.IsOpened() );
        f.Write( aContents );
        f.Close();
    };

    wxString boardPath =
            projectPath + wxFileName::GetPathSeparator() + wxS( "issue24016.kicad_pcb" );
    writeFile( boardPath, wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.Init( projectPath ) );

    wxString headHash = history.GetHeadHash( projectPath );
    BOOST_REQUIRE( !headHash.IsEmpty() );

    // Mirror SETTINGS_MANAGER::BackupProject output: a sibling "<name>-backups" directory
    // containing one or more .zip archives.
    wxString backupsDir =
            projectPath + wxFileName::GetPathSeparator() + wxS( "issue24016-backups" );
    BOOST_REQUIRE( wxFileName::Mkdir( backupsDir, 0777, wxPATH_MKDIR_FULL ) );

    wxString zipPath = backupsDir + wxFileName::GetPathSeparator()
                       + wxS( "issue24016-2026-04-22_120000.zip" );
    writeFile( zipPath, wxS( "pretend-zip-contents" ) );

    writeFile( boardPath, wxS( "(kicad_pcb (version 20240108) (dirty yes))\n" ) );

    BOOST_REQUIRE( history.RestoreCommit( projectPath, headHash, nullptr ) );

    BOOST_CHECK_MESSAGE( wxDirExists( backupsDir ),
                         "zip backups directory must survive RestoreCommit" );
    BOOST_CHECK_MESSAGE( wxFileExists( zipPath ),
                         ".zip archive inside the backups directory must survive RestoreCommit" );
}


BOOST_AUTO_TEST_SUITE_END()
