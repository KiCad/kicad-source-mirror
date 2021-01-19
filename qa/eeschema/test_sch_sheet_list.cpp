/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unit_test_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


class TEST_SCH_SHEET_LIST_FIXTURE
{
public:
    TEST_SCH_SHEET_LIST_FIXTURE() :
            m_schematic( nullptr ),
            m_manager( true )
    {
        m_pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    }

    virtual ~TEST_SCH_SHEET_LIST_FIXTURE()
    {
        m_schematic.Reset();
        delete m_pi;
    }

    void loadSchematic( const wxString& aBaseName );

    ///> Schematic to load
    SCHEMATIC m_schematic;

    SCH_PLUGIN* m_pi;

    SETTINGS_MANAGER m_manager;
};


void TEST_SCH_SHEET_LIST_FIXTURE::loadSchematic( const wxString& aBaseName )
{
    wxFileName fn = KI_TEST::GetEeschemaTestDataDir();

    fn.AppendDir( "netlists" );
    fn.AppendDir( aBaseName );
    fn.SetName( aBaseName );
    fn.SetExt( KiCadSchematicFileExtension );

    BOOST_TEST_MESSAGE( fn.GetFullPath() );

    wxFileName pro( fn );
    pro.SetExt( ProjectFileExtension );

    m_manager.LoadProject( pro.GetFullPath() );

    m_manager.Prj().SetElem( PROJECT::ELEM_SCH_PART_LIBS, nullptr );

    m_schematic.Reset();
    m_schematic.SetProject( &m_manager.Prj() );
    m_schematic.SetRoot( m_pi->Load( fn.GetFullPath(), &m_schematic ) );

    BOOST_REQUIRE_EQUAL( m_pi->GetError().IsEmpty(), true );

    m_schematic.CurrentSheet().push_back( &m_schematic.Root() );

    SCH_SCREENS screens( m_schematic.Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstances( m_schematic.RootScreen()->GetSymbolInstances() );

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();
}


BOOST_FIXTURE_TEST_SUITE( SchSheetList, TEST_SCH_SHEET_LIST_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSheetListPageProperties )
{
    loadSchematic( "complex_hierarchy" );

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();

    BOOST_CHECK( sheets.AllSheetPageNumbersEmpty() );

    sheets.SetInitialPageNumbers();

    // The root sheet should now be page 1.
    BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "1" );
    BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "2" );
    BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "3" );
}


BOOST_AUTO_TEST_SUITE_END()
