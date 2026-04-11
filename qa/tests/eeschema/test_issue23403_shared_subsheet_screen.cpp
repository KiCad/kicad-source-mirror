/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

/**
 * @file test_issue23403_shared_subsheet_screen.cpp
 *
 * Test for issue #23403: Top level schematics with shared sheets is broken.
 *
 * When multiple top-level sheets reference the same sub-sheet file, they must
 * share the same SCH_SCREEN object. Without this, edits made through one path
 * are invisible through the other, and saving overwrites earlier changes.
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <locale_io.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


struct ISSUE23403_FIXTURE
{
    ISSUE23403_FIXTURE() :
            m_settingsManager()
    {
        wxFileName projectPath( KI_TEST::GetEeschemaTestDataDir() );
        projectPath.AppendDir( "issue23403" );
        projectPath.SetName( "issue23403" );
        projectPath.SetExt( FILEEXT::ProjectFileExtension );

        m_settingsManager.LoadProject( projectPath.GetFullPath().ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~ISSUE23403_FIXTURE()
    {
        m_schematic.reset();
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT*                   m_project;
};


static SCH_SCREEN* findSubSheetScreen( SCH_SHEET* aTopSheet, const wxString& aSubFileName )
{
    if( !aTopSheet || !aTopSheet->GetScreen() )
        return nullptr;

    for( SCH_ITEM* item : aTopSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

        if( subSheet->GetScreen()
            && subSheet->GetScreen()->GetFileName().EndsWith( aSubFileName ) )
        {
            return subSheet->GetScreen();
        }
    }

    return nullptr;
}


BOOST_FIXTURE_TEST_SUITE( Issue23403SharedSubsheetScreen, ISSUE23403_FIXTURE )


/**
 * When two top-level sheets both reference the same sub-sheet file, loading them
 * through the same SCH_IO instance must produce shared SCH_SCREEN pointers.
 */
BOOST_AUTO_TEST_CASE( SharedSubSheetScreenReuse )
{
    LOCALE_IO dummy;

    wxFileName dataDir( KI_TEST::GetEeschemaTestDataDir() );
    dataDir.AppendDir( "issue23403" );

    wxFileName rootFile( dataDir.GetPath(), "issue23403", FILEEXT::KiCadSchematicFileExtension );
    wxFileName topFile( dataDir.GetPath(), "top_level_sheet_1",
                        FILEEXT::KiCadSchematicFileExtension );

    BOOST_REQUIRE_MESSAGE( rootFile.FileExists(),
                           "Test data missing: " + rootFile.GetFullPath().ToStdString() );
    BOOST_REQUIRE_MESSAGE( topFile.FileExists(),
                           "Test data missing: " + topFile.GetFullPath().ToStdString() );

    SCH_IO_KICAD_SEXPR io;

    SCH_SHEET* sheet1 = io.LoadSchematicFile( rootFile.GetFullPath(), m_schematic.get() );
    BOOST_REQUIRE( sheet1 != nullptr );

    SCH_SHEET* sheet2 = io.LoadSchematicFile( topFile.GetFullPath(), m_schematic.get() );
    BOOST_REQUIRE( sheet2 != nullptr );

    SCH_SCREEN* screen1 = findSubSheetScreen( sheet1, wxT( "shared1.kicad_sch" ) );
    SCH_SCREEN* screen2 = findSubSheetScreen( sheet2, wxT( "shared1.kicad_sch" ) );

    BOOST_REQUIRE_MESSAGE( screen1 != nullptr,
                           "First top-level sheet should contain shared1 sub-sheet" );
    BOOST_REQUIRE_MESSAGE( screen2 != nullptr,
                           "Second top-level sheet should contain shared1 sub-sheet" );

    BOOST_CHECK_MESSAGE( screen1 == screen2,
                         "Both top-level sheets must share the same SCH_SCREEN for shared1" );

    BOOST_CHECK_MESSAGE( screen1->GetRefCount() >= 2,
                         "Shared screen ref count should be at least 2, got "
                                 + std::to_string( screen1->GetRefCount() ) );

    delete sheet1;
    delete sheet2;
}


BOOST_AUTO_TEST_SUITE_END()
