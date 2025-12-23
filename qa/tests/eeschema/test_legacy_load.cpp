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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_legacy_load.cpp
 * Standalone test to load legacy schematic and check references
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <kiid.h>
#include <sch_file_versions.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <sch_sheet_path.h>
#include <trace_helpers.h>


BOOST_AUTO_TEST_SUITE( LegacyLoad )


BOOST_AUTO_TEST_CASE( TestLoadWithLogging )
{
    SETTINGS_MANAGER settingsManager;

    // Create a temporary project
    wxString tempDir = wxStandardPaths::Get().GetTempDir();
    wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT("test_legacy.kicad_pro");

    settingsManager.LoadProject( projectPath.ToStdString() );
    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( nullptr );
    PROJECT* project = &settingsManager.Prj();
    schematic->SetProject( project );

    // Load the legacy hierarchical schematic
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "legacy_hierarchy" );
    fn.SetName( "legacy_hierarchy" );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );
    wxString mainFile = fn.GetFullPath();

    BOOST_TEST_MESSAGE( "=== Loading schematic: " << mainFile.ToStdString() );
    BOOST_REQUIRE( wxFileExists( mainFile ) );

    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET* loadedSheet = nullptr;

    BOOST_CHECK_NO_THROW( loadedSheet = io.LoadSchematicFile( mainFile, schematic.get() ) );
    BOOST_REQUIRE( loadedSheet != nullptr );

    BOOST_TEST_MESSAGE( "=== Setting root sheet" );
    schematic->Reset();
    SCH_SHEET* defaultSheet = schematic->GetTopLevelSheet( 0 );
    schematic->AddTopLevelSheet( loadedSheet );
    schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    BOOST_TEST_MESSAGE( "=== Building hierarchy" );
    schematic->RefreshHierarchy();
    SCH_SHEET_LIST hierarchy = schematic->Hierarchy();

    BOOST_TEST_MESSAGE( "=== Checking for missing symbol instances" );
    hierarchy.CheckForMissingSymbolInstances( project->GetProjectName() );

    BOOST_TEST_MESSAGE( "\n=== Testing symbol reference retrieval ===" );

    // Test getting references for symbols on the first sub-sheet
    if( hierarchy.size() >= 2 )
    {
        const SCH_SHEET_PATH& subSheetPath = hierarchy[1];
        BOOST_TEST_MESSAGE( "Sheet path [1]: " << subSheetPath.PathHumanReadable( false ).ToStdString() );
        BOOST_TEST_MESSAGE( "Sheet path KIID: " << subSheetPath.Path().AsString().ToStdString() );

        if( subSheetPath.LastScreen() )
        {
            int count = 0;
            for( SCH_ITEM* item : subSheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                wxString ref = symbol->GetRef( &subSheetPath, false );

                BOOST_TEST_MESSAGE( "  Symbol " << symbol->m_Uuid.AsString().ToStdString()
                                   << " ref: " << ref.ToStdString() );

                if( ++count >= 5 )
                    break;  // Just test first 5 symbols
            }
        }
    }

    BOOST_TEST_MESSAGE( "=== Test completed ===" );
}


BOOST_AUTO_TEST_SUITE_END()
