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

#include <memory>
#include <vector>

#include <wx/datetime.h>
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


// Restore the backup location on destruction so a thrown BOOST_REQUIRE cannot leak the
// override into later tests.
struct SCOPED_BACKUP_LOCATION_OVERRIDE
{
    explicit SCOPED_BACKUP_LOCATION_OVERRIDE( BACKUP_LOCATION& aLocation ) :
            m_location( aLocation ), m_original( aLocation )
    {
    }

    ~SCOPED_BACKUP_LOCATION_OVERRIDE() { m_location = m_original; }

    BACKUP_LOCATION& m_location;
    BACKUP_LOCATION  m_original;
};


struct SCOPED_BACKUP_FORMAT_OVERRIDE
{
    explicit SCOPED_BACKUP_FORMAT_OVERRIDE( BACKUP_FORMAT& aFormat ) :
            m_format( aFormat ), m_original( aFormat )
    {
    }

    ~SCOPED_BACKUP_FORMAT_OVERRIDE() { m_format = m_original; }

    BACKUP_FORMAT& m_format;
    BACKUP_FORMAT  m_original;
};


// Load a project into the settings manager and unload it on destruction, keeping the global
// active-project state isolated even when a test aborts partway through.
struct SCOPED_PROJECT_LOAD
{
    SCOPED_PROJECT_LOAD( SETTINGS_MANAGER& aMgr, const wxString& aProjectFile ) : m_mgr( aMgr )
    {
        m_mgr.LoadProject( aProjectFile.ToStdString() );
    }

    ~SCOPED_PROJECT_LOAD() { m_mgr.UnloadProject( &m_mgr.Prj(), false ); }

    SETTINGS_MANAGER& m_mgr;
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


void writeTextFile( const wxString& aPath, const wxString& aContents )
{
    wxFFile f( aPath, wxT( "w" ) );
    BOOST_REQUIRE( f.IsOpened() );
    f.Write( aContents );
    f.Close();
}
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


/**
 * When pcbnew runs standalone with no project loaded, saving a board can land anywhere
 * on the filesystem. CommitFullProjectSnapshot used to recursively walk that directory
 * and call git_status_file on every entry. If the user saved into /tmp (or any directory
 * with millions of files), pcbnew would peg a CPU for hours. Verify that LOCAL_HISTORY
 * refuses to do any work for a directory that is not a real KiCad project.
 */
BOOST_AUTO_TEST_CASE( NoSnapshotWithoutProjectFile )
{
    LIBGIT2_SCOPE libgit;

    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR notAProject( wxS( "kicad_qa_no_project" ) );
    const wxString& path = notAProject.Path();

    // Drop a board file in but no .kicad_pro - this mirrors saving a board to /tmp
    // from standalone pcbnew.
    wxString boardPath = path + wxFileName::GetPathSeparator() + wxS( "stray.kicad_pcb" );
    writeTextFile( boardPath, wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;

    BOOST_CHECK( !history.Init( path ) );
    BOOST_CHECK( !history.CommitFullProjectSnapshot( path, wxS( "PCB Save" ) ) );
    BOOST_CHECK( !history.TagSave( path, wxS( "pcb" ) ) );

    // No .history directory should have been created.
    wxString historyDir = path + wxFileName::GetPathSeparator() + wxS( ".history" );
    BOOST_CHECK( !wxDirExists( historyDir ) );
}


/**
 * CommitFullProjectSnapshot collects files recursively. CommitSnapshot's legacy
 * path derives the project root from aFiles[0], so when the first file ends up
 * in a subdirectory the project root would be misidentified. CommitFullProjectSnapshot
 * must commit using the explicitly supplied project path regardless of what the
 * recursive walk happens to return first.
 */
BOOST_AUTO_TEST_CASE( CommitFullProjectSnapshotHandlesSubdirectories )
{
    LIBGIT2_SCOPE libgit;

    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_subdirs" ) );
    const wxString& path = project.Path();

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "subdirs.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "subdirs.kicad_pcb" ),
                   wxS( "(kicad_pcb (version 20240108))\n" ) );

    // Create a subdirectory whose contents will likely be collected before files at the
    // project root, exercising the subdirectory-first iteration order.
    wxString subDir = path + wxFileName::GetPathSeparator() + wxS( "libs" );
    BOOST_REQUIRE( wxFileName::Mkdir( subDir, 0777, wxPATH_MKDIR_FULL ) );
    writeTextFile( subDir + wxFileName::GetPathSeparator() + wxS( "fp.kicad_mod" ),
                   wxS( "(footprint test)\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.CommitFullProjectSnapshot( path, wxS( "Initial" ) ) );

    // History must have been created at the project root, not at the subdirectory.
    wxString historyDir = path + wxFileName::GetPathSeparator() + wxS( ".history" );
    BOOST_CHECK( wxDirExists( historyDir ) );
    BOOST_CHECK( !wxDirExists( subDir + wxFileName::GetPathSeparator() + wxS( ".history" ) ) );

    wxString headBefore = history.GetHeadHash( path );
    BOOST_REQUIRE( !headBefore.IsEmpty() );

    // Mutate a file in the subdirectory to ensure the next snapshot has work to do.
    writeTextFile( subDir + wxFileName::GetPathSeparator() + wxS( "fp.kicad_mod" ),
                   wxS( "(footprint test (modified))\n" ) );

    // CommitFullProjectSnapshot must commit using the project root, not derive a wrong
    // root from the first file in the recursive collection.
    BOOST_CHECK( history.CommitFullProjectSnapshot( path, wxS( "PCB Save" ) ) );

    wxString headAfter = history.GetHeadHash( path );
    BOOST_REQUIRE( !headAfter.IsEmpty() );
    BOOST_CHECK( headBefore != headAfter );
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

    wxString boardPath =
            projectPath + wxFileName::GetPathSeparator() + wxS( "issue24016.kicad_pcb" );
    writeTextFile( boardPath, wxS( "(kicad_pcb (version 20240108))\n" ) );

    // LOCAL_HISTORY refuses to operate on directories without a project file, so seed
    // the fixture with a minimal one to mirror a real KiCad project layout.
    wxString projectFile =
            projectPath + wxFileName::GetPathSeparator() + wxS( "issue24016.kicad_pro" );
    writeTextFile( projectFile, wxS( "{}\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.CommitFullProjectSnapshot( projectPath, wxS( "Initial" ) ) );

    wxString headHash = history.GetHeadHash( projectPath );
    BOOST_REQUIRE( !headHash.IsEmpty() );

    // Mirror SETTINGS_MANAGER::BackupProject output: a sibling "<name>-backups" directory
    // containing one or more .zip archives.
    wxString backupsDir =
            projectPath + wxFileName::GetPathSeparator() + wxS( "issue24016-backups" );
    BOOST_REQUIRE( wxFileName::Mkdir( backupsDir, 0777, wxPATH_MKDIR_FULL ) );

    wxString zipPath = backupsDir + wxFileName::GetPathSeparator()
                       + wxS( "issue24016-2026-04-22_120000.zip" );
    writeTextFile( zipPath, wxS( "pretend-zip-contents" ) );

    writeTextFile( boardPath, wxS( "(kicad_pcb (version 20240108) (dirty yes))\n" ) );

    BOOST_REQUIRE( history.RestoreCommit( projectPath, headHash, nullptr ) );

    BOOST_CHECK_MESSAGE( wxDirExists( backupsDir ),
                         "zip backups directory must survive RestoreCommit" );
    BOOST_CHECK_MESSAGE( wxFileExists( zipPath ),
                         ".zip archive inside the backups directory must survive RestoreCommit" );
}


/**
 * Regression test: a user reported that opening a project containing a nested project and
 * accepting "restore previous version" deleted the entire nested project. The data-loss path
 * was that LOCAL_HISTORY treated the nested project's files as ordinary subtree contents,
 * captured them in the parent's snapshot, then on restore proposed them for deletion and
 * (because backupCurrentFiles renamed whole top-level entries) wiped the nested directory.
 *
 * The fix: collectProjectFiles() and findFilesToDelete() must skip subtrees that contain
 * their own .kicad_pro file, and the restore confirmation dialog must enter the
 * non-destructive "Keep All Files" path automatically when a nested project is at risk.
 *
 * This test exercises the simple reported scenario - nested project as a top-level subdir.
 */
BOOST_AUTO_TEST_CASE( RestoreCommitPreservesNestedProject )
{
    LIBGIT2_SCOPE libgit;

    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR tempProject( wxS( "kicad_qa_nested_project" ) );
    const wxString& projectPath = tempProject.Path();

    // Parent project (projectA): minimal .kicad_pro plus a board file.
    wxString parentPro = projectPath + wxFileName::GetPathSeparator() + wxS( "projectA.kicad_pro" );
    wxString parentPcb = projectPath + wxFileName::GetPathSeparator() + wxS( "projectA.kicad_pcb" );
    writeTextFile( parentPro, wxS( "{}\n" ) );
    writeTextFile( parentPcb, wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.CommitFullProjectSnapshot( projectPath, wxS( "Initial" ) ) );

    wxString headHash = history.GetHeadHash( projectPath );
    BOOST_REQUIRE( !headHash.IsEmpty() );

    // Now drop a nested project under projectA/. The user's scenario was that this nested
    // project was added AFTER the parent's snapshot was committed - so its files are not in
    // the restored commit, and a naive restore would propose them for deletion.
    wxString nestedDir = projectPath + wxFileName::GetPathSeparator() + wxS( "projectB" );
    BOOST_REQUIRE( wxFileName::Mkdir( nestedDir, 0777, wxPATH_MKDIR_FULL ) );

    wxString nestedPro = nestedDir + wxFileName::GetPathSeparator() + wxS( "projectB.kicad_pro" );
    wxString nestedPcb = nestedDir + wxFileName::GetPathSeparator() + wxS( "projectB.kicad_pcb" );
    wxString nestedSch = nestedDir + wxFileName::GetPathSeparator() + wxS( "projectB.kicad_sch" );
    writeTextFile( nestedPro, wxS( "{ \"nested\": true }\n" ) );
    writeTextFile( nestedPcb, wxS( "(kicad_pcb (version 20240108) (nested yes))\n" ) );
    writeTextFile( nestedSch, wxS( "(kicad_sch (version 20240108))\n" ) );

    // Modify the parent board so the restore actually has work to do.
    writeTextFile( parentPcb, wxS( "(kicad_pcb (version 20240108) (dirty yes))\n" ) );

    BOOST_REQUIRE( history.RestoreCommit( projectPath, headHash, nullptr ) );

    // The nested project's directory and every one of its files MUST survive the restore.
    BOOST_CHECK_MESSAGE( wxDirExists( nestedDir ),
                         "nested project directory must survive RestoreCommit" );
    BOOST_CHECK_MESSAGE( wxFileExists( nestedPro ),
                         "nested .kicad_pro must survive RestoreCommit" );
    BOOST_CHECK_MESSAGE( wxFileExists( nestedPcb ),
                         "nested .kicad_pcb must survive RestoreCommit" );
    BOOST_CHECK_MESSAGE( wxFileExists( nestedSch ),
                         "nested .kicad_sch must survive RestoreCommit" );

    // Parent project files were correctly restored.
    BOOST_CHECK( wxFileExists( parentPro ) );
    BOOST_CHECK( wxFileExists( parentPcb ) );
}


/**
 * The retained backup directory must be timestamped (Windows-safe, no colons) and survive
 * the success path of RestoreCommit so the user can recover any displaced file. Replaces
 * the old behavior where _restore_backup/ was unconditionally rmdir'd on success.
 */
BOOST_AUTO_TEST_CASE( RestoreCommitRetainsTimestampedBackup )
{
    LIBGIT2_SCOPE libgit;

    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR tempProject( wxS( "kicad_qa_retained_backup" ) );
    const wxString& projectPath = tempProject.Path();

    wxString boardPath = projectPath + wxFileName::GetPathSeparator() + wxS( "rb.kicad_pcb" );
    wxString projectFile = projectPath + wxFileName::GetPathSeparator() + wxS( "rb.kicad_pro" );
    writeTextFile( projectFile, wxS( "{}\n" ) );
    writeTextFile( boardPath, wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.CommitFullProjectSnapshot( projectPath, wxS( "Initial" ) ) );

    wxString headHash = history.GetHeadHash( projectPath );
    BOOST_REQUIRE( !headHash.IsEmpty() );

    // Mutate the board so restore has work to do (and produces a backup).
    writeTextFile( boardPath, wxS( "(kicad_pcb (version 20240108) (dirty yes))\n" ) );

    BOOST_REQUIRE( history.RestoreCommit( projectPath, headHash, nullptr ) );

    // Backups land at a SIBLING path (aProjectPath + "_restore_backup_<ts>"), so look in
    // the parent directory of the project. Same convention as the legacy "_restore_backup".
    wxString parentDir = wxFileName( projectPath ).GetPath();
    wxString leafPrefix = wxFileName( projectPath ).GetFullName() + wxS( "_restore_backup_" );

    wxDir dir( parentDir );
    BOOST_REQUIRE( dir.IsOpened() );

    wxString retainedBackup;
    bool     foundLegacyBackup = false;

    wxString name;
    for( bool cont = dir.GetFirst( &name, wxEmptyString, wxDIR_DIRS ); cont;
         cont = dir.GetNext( &name ) )
    {
        if( name.StartsWith( leafPrefix ) )
        {
            retainedBackup = parentDir + wxFileName::GetPathSeparator() + name;

            // No colons - Windows path-safe.
            BOOST_CHECK_MESSAGE( name.Find( ':' ) == wxNOT_FOUND,
                                 "retained backup directory name must not contain ':' "
                                 "(Windows-illegal in path components)" );
        }
        else if( name == wxFileName( projectPath ).GetFullName() + wxS( "_restore_backup" ) )
        {
            foundLegacyBackup = true;
        }
    }

    BOOST_CHECK_MESSAGE(
            !retainedBackup.IsEmpty(),
            "RestoreCommit must retain a timestamped _restore_backup_<ts>/ sibling directory" );
    BOOST_CHECK_MESSAGE(
            !foundLegacyBackup,
            "RestoreCommit must not leave the legacy non-timestamped _restore_backup/ behind" );

    // Clean up the retained backup so the test does not leak files into /tmp.
    if( !retainedBackup.IsEmpty() && wxDirExists( retainedBackup ) )
        wxFileName::Rmdir( retainedBackup, wxPATH_RMDIR_RECURSIVE );
}


/**
 * Snapshots must only commit KiCad managed files and user files in the project dir stay out.
 */
BOOST_AUTO_TEST_CASE( CommitFullProjectSnapshotExcludesNonKiCadFiles )
{
    LIBGIT2_SCOPE libgit;

    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_privacy" ) );
    const wxString& path = project.Path();

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pcb" ),
                   wxS( "(kicad_pcb (version 20240108))\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_sch" ),
                   wxS( "(kicad_sch (version 20240108))\n" ) );

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "passwords.txt" ), wxS( "secret\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "datasheet.pdf" ), wxS( "fake pdf bytes\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "notes.md" ), wxS( "personal notes\n" ) );

    wxString subDir = path + wxFileName::GetPathSeparator() + wxS( "docs" );
    BOOST_REQUIRE( wxFileName::Mkdir( subDir, 0777, wxPATH_MKDIR_FULL ) );
    writeTextFile( subDir + wxFileName::GetPathSeparator() + wxS( "manual.txt" ), wxS( "irrelevant\n" ) );

    LOCAL_HISTORY history;
    BOOST_REQUIRE( history.CommitFullProjectSnapshot( path, wxS( "Close" ) ) );

    wxString        hist = path + wxFileName::GetPathSeparator() + wxS( ".history" );
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, hist.mb_str().data() ), 0 );

    git_oid head_oid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head_oid, repo, "HEAD" ), 0 );

    git_commit* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &head_oid ), 0 );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), 0 );

    std::vector<std::string> committedPaths;
    git_tree_walk(
            tree, GIT_TREEWALK_PRE,
            []( const char* root, const git_tree_entry* entry, void* payload ) -> int
            {
                auto* paths = static_cast<std::vector<std::string>*>( payload );

                if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB )
                    paths->push_back( std::string( root ) + git_tree_entry_name( entry ) );

                return 0;
            },
            &committedPaths );

    git_tree_free( tree );
    git_commit_free( head );
    git_repository_free( repo );

    auto contains = [&]( const std::string& s )
    {
        return std::find( committedPaths.begin(), committedPaths.end(), s ) != committedPaths.end();
    };

    BOOST_CHECK_MESSAGE( contains( "p.kicad_pro" ), "kicad_pro must be committed" );
    BOOST_CHECK_MESSAGE( contains( "p.kicad_pcb" ), "kicad_pcb must be committed" );
    BOOST_CHECK_MESSAGE( contains( "p.kicad_sch" ), "kicad_sch must be committed" );

    BOOST_CHECK_MESSAGE( !contains( "passwords.txt" ), "passwords.txt must NOT appear in history" );
    BOOST_CHECK_MESSAGE( !contains( "datasheet.pdf" ), "datasheet.pdf must NOT appear in history" );
    BOOST_CHECK_MESSAGE( !contains( "notes.md" ), "notes.md must NOT appear in history" );
    BOOST_CHECK_MESSAGE( !contains( "docs/manual.txt" ), "subdirectory user content must NOT appear in history" );
}


/**
 * Idle autosave on a fresh project (saver output == disk) must not create an untagged
 * HEAD.  Real edits (saver output != disk) must commit.
 */
BOOST_AUTO_TEST_CASE( FirstAutosaveSkipsCommitWhenStagedMatchesDisk )
{
    LIBGIT2_SCOPE libgit;

    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_first_idle_autosave" ) );
    const wxString& path = project.Path();

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pcb" ),
                   wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;

    // Mutating this between calls simulates the user editing the in-memory document.
    std::string inMemoryContent = "(kicad_pcb (version 20240108))\n";

    auto saver = [&inMemoryContent]( const wxString&, std::vector<HISTORY_FILE_DATA>& aFileData )
    {
        HISTORY_FILE_DATA entry;
        entry.relativePath = wxS( "p.kicad_pcb" );
        entry.content = inMemoryContent;
        aFileData.push_back( std::move( entry ) );
    };

    history.RegisterSaver( &history, saver );

    // Saver output matches disk, must skip (autosave path: empty tagFileType).
    BOOST_REQUIRE( history.RunRegisteredSaversAndCommit( path, wxS( "Autosave" ), wxEmptyString ) );
    history.WaitForPendingSave();

    wxString histDir = path + wxFileName::GetPathSeparator() + wxS( ".history" );
    BOOST_CHECK( wxDirExists( histDir ) );

    wxString head = history.GetHeadHash( path );
    BOOST_CHECK_MESSAGE( head.IsEmpty(), "no untagged HEAD should exist after an idle first save" );

    // Real edit, must commit.
    inMemoryContent = "(kicad_pcb (version 20240108) (edited yes))\n";

    BOOST_REQUIRE( history.RunRegisteredSaversAndCommit( path, wxS( "Autosave" ), wxEmptyString ) );
    history.WaitForPendingSave();

    head = history.GetHeadHash( path );
    BOOST_CHECK_MESSAGE( !head.IsEmpty(), "in-memory edits diverging from disk must produce a commit" );

    history.UnregisterSaver( &history );
}


/**
 * A saver tied to a document's lifetime token must be skipped, and dropped, once the document is
 * destroyed.  The autosave timer is shared across editor frames, so a saver that outlives its
 * BOARD/SCHEMATIC would serialize freed memory -- the autosave-saver use-after-free behind the
 * crashes in EDA_BASE_FRAME::doAutoSave (Sentry KICAD-159V PCB, KICAD-17F5 SCH).
 */
BOOST_AUTO_TEST_CASE( SaverSkippedAfterOwningDocumentDestroyed )
{
    LIBGIT2_SCOPE libgit;

    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_saver_lifetime" ) );
    const wxString&  path = project.Path();
    const wxString   sep = wxFileName::GetPathSeparator();

    writeTextFile( path + sep + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + sep + wxS( "p.kicad_pcb" ), wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;

    // The saver only touches this heap counter, so it stays safe to invoke after the board is gone;
    // gating it on the board's token is the behaviour under test.
    auto                   runCount = std::make_shared<int>( 0 );
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    history.RegisterSaver( board.get(),
            [runCount]( const wxString&, std::vector<HISTORY_FILE_DATA>& aFileData )
            {
                ++( *runCount );

                HISTORY_FILE_DATA entry;
                entry.relativePath = wxS( "p.kicad_pcb" );
                entry.content = "(kicad_pcb (version 20240108) (edited yes))\n";
                aFileData.push_back( std::move( entry ) );
            },
            board->GetHistoryLifetimeToken() );

    // Positive control: while the board is alive the saver runs.
    history.RunRegisteredSaversAndCommit( path, wxS( "Autosave" ), wxEmptyString );
    history.WaitForPendingSave();
    BOOST_CHECK_EQUAL( *runCount, 1 );

    // Destroy the document; its expired token must make both runners skip and drop the saver.
    board.reset();

    history.RunRegisteredSaversAndCommit( path, wxS( "Autosave" ), wxEmptyString );
    history.WaitForPendingSave();
    BOOST_CHECK_MESSAGE( *runCount == 1, "commit runner invoked a saver whose board was destroyed" );

    history.RunRegisteredSaversAsAutosaveFiles( path );
    BOOST_CHECK_MESSAGE( *runCount == 1, "autosave-file runner invoked a saver whose board was destroyed" );
}


/**
 * Manual save on a fresh project (saver output == disk) must still commit, so the user's
 * first explicit save shows up in the history dialog.  Counterpart to
 * FirstAutosaveSkipsCommitWhenStagedMatchesDisk.
 */
BOOST_AUTO_TEST_CASE( FirstManualSaveAlwaysCommitsOnFreshProject )
{
    LIBGIT2_SCOPE libgit;

    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_first_manual_save" ) );
    const wxString& path = project.Path();

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pcb" ),
                   wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;

    std::string inMemoryContent = "(kicad_pcb (version 20240108))\n";

    auto saver = [&inMemoryContent]( const wxString&, std::vector<HISTORY_FILE_DATA>& aFileData )
    {
        HISTORY_FILE_DATA entry;
        entry.relativePath = wxS( "p.kicad_pcb" );
        entry.content = inMemoryContent;
        aFileData.push_back( std::move( entry ) );
    };

    history.RegisterSaver( &history, saver );

    // Manual save (non-empty tagFileType) on fresh project: commit even when staged matches disk.
    BOOST_REQUIRE( history.RunRegisteredSaversAndCommit( path, wxS( "Manual Save" ), wxS( "pcb" ) ) );

    wxString head = history.GetHeadHash( path );
    BOOST_CHECK_MESSAGE( !head.IsEmpty(), "manual save on a fresh project must commit even when staged matches disk" );

    history.UnregisterSaver( &history );
}


// With backups enabled but the format set to Zip, autosave uses legacy recovery files, so the
// incremental git-commit path must be a no-op rather than extending a history the user switched
// off (issue 24773).
BOOST_AUTO_TEST_CASE( ZipFormatSkipsIncrementalAutosave )
{
    LIBGIT2_SCOPE libgit;

    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    BACKUP_FORMAT& format = Pgm().GetCommonSettings()->m_Backup.format;
    SCOPED_BACKUP_FORMAT_OVERRIDE restoreFormat( format );
    format = BACKUP_FORMAT::ZIP;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_zip_skips_incremental" ) );
    const wxString& path = project.Path();

    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );
    writeTextFile( path + wxFileName::GetPathSeparator() + wxS( "p.kicad_pcb" ),
                   wxS( "(kicad_pcb (version 20240108))\n" ) );

    LOCAL_HISTORY history;

    auto saver = []( const wxString&, std::vector<HISTORY_FILE_DATA>& aFileData )
    {
        HISTORY_FILE_DATA entry;
        entry.relativePath = wxS( "p.kicad_pcb" );
        entry.content = "(kicad_pcb (version 20240108) (edited yes))\n";
        aFileData.push_back( std::move( entry ) );
    };

    history.RegisterSaver( &history, saver );

    BOOST_REQUIRE( history.RunRegisteredSaversAndCommit( path, wxS( "Autosave" ), wxEmptyString ) );
    history.WaitForPendingSave();

    BOOST_CHECK_MESSAGE( history.GetHeadHash( path ).IsEmpty(),
                         "zip backup format must not create incremental autosave commits" );

    history.UnregisterSaver( &history );
}


// The Zip backup format must reliably write legacy recovery files on autosave so a crash does
// not lose work between manual saves (issue 24773).
BOOST_AUTO_TEST_CASE( ZipFormatWritesRecoveryFiles )
{
    bool& backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    BACKUP_FORMAT& format = Pgm().GetCommonSettings()->m_Backup.format;
    SCOPED_BACKUP_FORMAT_OVERRIDE restoreFormat( format );
    format = BACKUP_FORMAT::ZIP;

    BACKUP_LOCATION& location = Pgm().GetCommonSettings()->m_Backup.location;
    SCOPED_BACKUP_LOCATION_OVERRIDE restoreLocation( location );
    location = BACKUP_LOCATION::PROJECT_DIR;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_zip_recovery_files" ) );
    const wxString& path = project.Path();
    const wxString  sep = wxFileName::GetPathSeparator();

    writeTextFile( path + sep + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );

    SETTINGS_MANAGER&   mgr = Pgm().GetSettingsManager();
    SCOPED_PROJECT_LOAD loadedProject( mgr, path + sep + wxS( "p.kicad_pro" ) );

    LOCAL_HISTORY history;

    auto saver = []( const wxString&, std::vector<HISTORY_FILE_DATA>& aFileData )
    {
        HISTORY_FILE_DATA entry;
        entry.relativePath = wxS( "p.kicad_pcb" );
        entry.content = "(kicad_pcb (version 20240108) (edited yes))\n";
        aFileData.push_back( std::move( entry ) );
    };

    history.RegisterSaver( &history, saver );

    BOOST_REQUIRE( history.RunRegisteredSaversAsAutosaveFiles( path ) );

    wxString autosavePath = path + sep + wxS( "_autosave-p.kicad_pcb" );
    BOOST_CHECK_MESSAGE( wxFileExists( autosavePath ),
                         "zip backup format must write autosave recovery files" );

    history.UnregisterSaver( &history );
}


// A content-identical autosave with a newer mtime (cloud-sync touch) must not be flagged
// stale, or recovery prompts fire on every open with nothing to restore (issue 24126).
BOOST_AUTO_TEST_CASE( CloudSyncTouchedAutosaveFalselyFlaggedStale )
{
    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    BACKUP_LOCATION& location = Pgm().GetCommonSettings()->m_Backup.location;
    SCOPED_BACKUP_LOCATION_OVERRIDE restoreLocation( location );
    location = BACKUP_LOCATION::PROJECT_DIR;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_cloudsync_autosave" ) );
    const wxString& path = project.Path();
    const wxString  sep = wxFileName::GetPathSeparator();

    // FindStaleAutosaveFiles resolves the autosave root through the active project, so it must
    // be loaded; the .kicad_pro must exist on disk first for LoadProject to succeed.
    writeTextFile( path + sep + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );

    SETTINGS_MANAGER&   mgr = Pgm().GetSettingsManager();
    SCOPED_PROJECT_LOAD loadedProject( mgr, path + sep + wxS( "p.kicad_pro" ) );

    const wxString sourcePath = path + sep + wxS( "p.kicad_pcb" );
    const wxString autosavePath = path + sep + wxS( "_autosave-p.kicad_pcb" );
    const wxString boardContent = wxS( "(kicad_pcb (version 20240108))\n" );

    // Byte-identical source and "_autosave-" companion, mirroring a clean save.
    writeTextFile( sourcePath, boardContent );
    writeTextFile( autosavePath, boardContent );

    LOCAL_HISTORY history;

    // Cloud-sync touch gives the identical autosave a strictly newer mtime.
    wxDateTime srcMtime = wxFileName( sourcePath ).GetModificationTime();
    wxDateTime newerMtime = srcMtime + wxTimeSpan::Seconds( 60 );
    BOOST_REQUIRE( wxFileName( autosavePath ).SetTimes( &newerMtime, &newerMtime, nullptr ) );

    std::vector<wxString> exts{ wxS( "kicad_pcb" ) };
    auto                  stale = history.FindStaleAutosaveFiles( path, exts );

    BOOST_CHECK_MESSAGE( stale.empty(),
                         "Content-identical autosave with newer mtime was flagged stale, "
                         "triggering a spurious recovery prompt (issue 24126)" );
}


// Counterpart to the above: an autosave whose content genuinely differs (a real unsaved edit)
// must still be flagged stale so recovery continues to fire.
BOOST_AUTO_TEST_CASE( DivergentAutosaveStillFlaggedStale )
{
    bool&                backupEnabled = Pgm().GetCommonSettings()->m_Backup.enabled;
    SCOPED_BOOL_OVERRIDE restoreBackupFlag( backupEnabled );
    backupEnabled = true;

    BACKUP_LOCATION& location = Pgm().GetCommonSettings()->m_Backup.location;
    SCOPED_BACKUP_LOCATION_OVERRIDE restoreLocation( location );
    location = BACKUP_LOCATION::PROJECT_DIR;

    SCOPED_TEMP_DIR project( wxS( "kicad_qa_divergent_autosave" ) );
    const wxString& path = project.Path();
    const wxString  sep = wxFileName::GetPathSeparator();

    writeTextFile( path + sep + wxS( "p.kicad_pro" ), wxS( "{}\n" ) );

    SETTINGS_MANAGER&   mgr = Pgm().GetSettingsManager();
    SCOPED_PROJECT_LOAD loadedProject( mgr, path + sep + wxS( "p.kicad_pro" ) );

    const wxString sourcePath = path + sep + wxS( "p.kicad_pcb" );
    const wxString autosavePath = path + sep + wxS( "_autosave-p.kicad_pcb" );

    // Autosave content diverges from the source, a genuine unsaved edit to recover.
    writeTextFile( sourcePath, wxS( "(kicad_pcb (version 20240108))\n" ) );
    writeTextFile( autosavePath, wxS( "(kicad_pcb (version 20240108) (edited yes))\n" ) );

    LOCAL_HISTORY history;

    wxDateTime srcMtime = wxFileName( sourcePath ).GetModificationTime();
    wxDateTime newerMtime = srcMtime + wxTimeSpan::Seconds( 60 );
    BOOST_REQUIRE( wxFileName( autosavePath ).SetTimes( &newerMtime, &newerMtime, nullptr ) );

    std::vector<wxString> exts{ wxS( "kicad_pcb" ) };
    auto                  stale = history.FindStaleAutosaveFiles( path, exts );

    BOOST_CHECK_MESSAGE( stale.size() == 1,
                         "Genuinely divergent autosave must still be flagged stale for recovery" );
}


BOOST_AUTO_TEST_SUITE_END()
