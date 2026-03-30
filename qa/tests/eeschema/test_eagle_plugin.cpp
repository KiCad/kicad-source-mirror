/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <core/ignore.h>
#include <kiway.h>
#include <pgm_base.h>
#include <sch_io/sch_io.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <libraries/library_manager.h>

#include "eeschema_test_utils.h"

/**
 * Checks that the SCH_IO manager finds the Eagle plugin
 */
BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


/**
 * Get a schematic file from the test data eagle subdir
 */
static wxFileName getEagleTestSchematic( const wxString& sch_file )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "io" );
    fn.AppendDir( "eagle" );
    fn.SetFullName( sch_file );

    return fn;
}


/**
 * Verify that a multi-page Eagle import produces the correct top-level sheet hierarchy.
 *
 * Regression test for issue #23645: Eagle import was inserting a spurious empty default
 * sheet at index 0 in m_topLevelSheets, causing GetTopLevelSheet(0) to return an empty
 * sheet that then received the project filename.  On save, only that empty sheet was
 * written; on re-open all components were missing.
 */
BOOST_AUTO_TEST_CASE( ImportHierarchy )
{
    const wxFileName eagleFn = getEagleTestSchematic( "eagle-import-testfile.sch" );
    BOOST_REQUIRE( wxFileExists( eagleFn.GetFullPath() ) );

    // Eagle import writes a .kicad_sym library to the project directory.  A temp dir
    // is used so the generated files don't pollute the source tree.
    wxString tempDir = wxStandardPaths::Get().GetTempDir();
    wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "eagle_test.kicad_pro" );

    // Eagle import uses Pgm().GetLibraryManager() for the symbol library adapter, so we
    // must load the project through the global settings manager and set up project tables.
    Pgm().GetSettingsManager().LoadProject( projectPath );
    PROJECT& project = Pgm().GetSettingsManager().Prj();
    Pgm().GetLibraryManager().LoadProjectTables( project.GetProjectDirectory() );

    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( &project );
    schematic->Reset();

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
    BOOST_REQUIRE( pi.get() != nullptr );

    SCH_SHEET* loadedSheet = nullptr;

    try
    {
        loadedSheet = pi->LoadSchematicFile( eagleFn.GetFullPath(), schematic.get() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( std::string( "LoadSchematicFile threw: " ) + e.what() );
    }
    catch( ... )
    {
        BOOST_FAIL( "LoadSchematicFile threw unknown exception" );
    }

    BOOST_REQUIRE( loadedSheet != nullptr );

    // The returned sheet must be the virtual root (niluuid).
    BOOST_CHECK( loadedSheet->m_Uuid == niluuid );

    const std::vector<SCH_SHEET*>& topSheets = schematic->GetTopLevelSheets();

    // The test file has exactly 2 Eagle pages; there must be no spurious default sheet.
    BOOST_CHECK_EQUAL( topSheets.size(), 2 );

    for( SCH_SHEET* sheet : topSheets )
    {
        // No top-level sheet should be the virtual root.
        BOOST_CHECK( sheet != nullptr );
        BOOST_CHECK( sheet->m_Uuid != niluuid );

        // Every page must have a screen with a non-empty filename so SaveProject can
        // write it to disk.
        BOOST_REQUIRE( sheet->GetScreen() != nullptr );
        BOOST_CHECK( !sheet->GetScreen()->GetFileName().IsEmpty() );
    }

    // Verify the hierarchy paths match the symbol instance paths so PCB update works.
    // Each symbol's hierarchical reference must have a path matching one of the top-level
    // sheet paths in the hierarchy.
    SCH_SHEET_LIST hierarchy = schematic->BuildSheetListSortedByPageNumbers();
    BOOST_CHECK_EQUAL( hierarchy.size(), 2 );

    std::set<KIID_PATH> hierPaths;

    for( const SCH_SHEET_PATH& path : hierarchy )
        hierPaths.insert( path.Path() );

    int totalSymbols = 0;
    int orphanedSymbols = 0;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( const EDA_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            const SCH_SYMBOL* sym = static_cast<const SCH_SYMBOL*>( item );
            ++totalSymbols;

            bool foundMatch = false;

            for( const SCH_SYMBOL_INSTANCE& inst : sym->GetInstances() )
            {
                if( hierPaths.count( inst.m_Path ) )
                {
                    foundMatch = true;
                    break;
                }
            }

            if( !foundMatch )
                ++orphanedSymbols;
        }
    }

    BOOST_CHECK_GT( totalSymbols, 0 );
    BOOST_CHECK_EQUAL( orphanedSymbols, 0 );
}
