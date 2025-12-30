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

#include <boost/test/unit_test.hpp>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <vector>

#include <wx/filename.h>
#include <wx/stdpaths.h>

BOOST_AUTO_TEST_SUITE( SchHistoryAutosave )

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

    std::vector<wxString> savedFiles;
    m_schematic->SaveToHistory( m_settingsManager.Prj().GetProjectPath(), savedFiles );

    BOOST_REQUIRE_EQUAL( savedFiles.size(), 1 );

    wxFileName saved( savedFiles[0] );
    BOOST_CHECK( saved.FileExists() );
    BOOST_CHECK_EQUAL( saved.GetExt(), FILEEXT::LegacySchematicFileExtension );
    BOOST_CHECK( saved.GetPath().Contains( wxS( "legacy" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()
