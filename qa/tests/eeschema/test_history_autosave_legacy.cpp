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

#include <boost/test/unit_test.hpp>
#include <local_history.h>
#include <pgm_base.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <vector>

#include <wx/filename.h>
#include <wx/stdpaths.h>

BOOST_AUTO_TEST_SUITE( SchHistoryAutosave )

template <typename T>
struct SCOPED_SETTING_OVERRIDE
{
    SCOPED_SETTING_OVERRIDE( T& aRef, T aValue ) : m_ref( aRef ), m_original( aRef ) { aRef = aValue; }
    ~SCOPED_SETTING_OVERRIDE() { m_ref = m_original; }

    T& m_ref;
    T  m_original;
};


struct HISTORY_AUTOSAVE_FIXTURE
{
    HISTORY_AUTOSAVE_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "legacy_autosave.kicad_pro" );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();
    }

    ~HISTORY_AUTOSAVE_FIXTURE()
    {
        // Clean up the entire .history directory under the project
        wxString historyPath = m_settingsManager.Prj().GetProjectPath() + wxS( ".history" );
        wxFileName::Rmdir( historyPath, wxPATH_RMDIR_RECURSIVE );
    }

    SETTINGS_MANAGER m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

BOOST_FIXTURE_TEST_CASE( SavesLegacySheetIntoHistoryPath, HISTORY_AUTOSAVE_FIXTURE )
{
    std::vector<SCH_SHEET*> sheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !sheets.empty() );

    SCH_SHEET* sheet = sheets[0];
    BOOST_REQUIRE( sheet->GetScreen() != nullptr );

    sheet->SetFileName( wxS( "legacy/legacy.sch" ) );
    sheet->GetScreen()->SetFileName( wxS( "legacy/legacy.sch" ) );

    std::vector<HISTORY_FILE_DATA> fileData;
    m_schematic->SaveToHistory( m_settingsManager.Prj().GetProjectPath(), fileData );

    BOOST_REQUIRE_EQUAL( fileData.size(), 1 );

    // SaveToHistory now emits project-relative paths -- the dispatcher joins them with
    // the storage root (.history/ or the user data dir) at write time.
    wxFileName saved( fileData[0].relativePath );
    BOOST_CHECK_EQUAL( saved.GetExt(), FILEEXT::LegacySchematicFileExtension );
    BOOST_CHECK( saved.GetPath().Contains( wxS( "legacy" ) ) );
    BOOST_CHECK( !wxFileName( fileData[0].relativePath ).IsAbsolute() );
    BOOST_CHECK( !fileData[0].content.empty() );
    BOOST_CHECK( fileData[0].prettify );
}

// A clean sheet must still be captured while local history is active (backups on, format
// incremental).  The manual-save flow clears the dirty flag before the saver runs, so
// filtering on it here would drop the whole snapshot -- the regression that left .history
// stale for users on the Zip backup format (issue 24773).
BOOST_FIXTURE_TEST_CASE( CleanSheetStillSavedWhenLocalHistoryEnabled, HISTORY_AUTOSAVE_FIXTURE )
{
    COMMON_SETTINGS* cs = Pgm().GetCommonSettings();
    BOOST_REQUIRE( cs );

    SCOPED_SETTING_OVERRIDE<bool>          backupOn( cs->m_Backup.enabled, true );
    SCOPED_SETTING_OVERRIDE<BACKUP_FORMAT> incrementalFormat( cs->m_Backup.format,
                                                             BACKUP_FORMAT::INCREMENTAL );

    std::vector<SCH_SHEET*> sheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !sheets.empty() );

    SCH_SHEET* sheet = sheets[0];
    BOOST_REQUIRE( sheet->GetScreen() != nullptr );

    sheet->SetFileName( wxS( "clean/clean.kicad_sch" ) );
    sheet->GetScreen()->SetFileName( wxS( "clean/clean.kicad_sch" ) );
    sheet->GetScreen()->SetContentModified( false );

    std::vector<HISTORY_FILE_DATA> fileData;
    m_schematic->SaveToHistory( m_settingsManager.Prj().GetProjectPath(), fileData );

    BOOST_CHECK_EQUAL( fileData.size(), 1 );
}

BOOST_AUTO_TEST_SUITE_END()
