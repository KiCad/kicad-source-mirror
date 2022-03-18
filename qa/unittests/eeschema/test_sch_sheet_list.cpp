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

#include <qa_utils/wx_utils/unit_test_utils.h>
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

    void loadSchematic( const wxString& aRelativePath );

    wxFileName buildFullPath( const wxString& aRelativePath );

    ///> Schematic to load
    SCHEMATIC m_schematic;

    SCH_PLUGIN* m_pi;

    SETTINGS_MANAGER m_manager;
};


void TEST_SCH_SHEET_LIST_FIXTURE::loadSchematic( const wxString& aRelativePath )
{
    wxFileName fn = buildFullPath( aRelativePath );

    BOOST_TEST_MESSAGE( fn.GetFullPath() );

    wxFileName pro( fn );
    pro.SetExt( ProjectFileExtension );

    m_schematic.Reset();
    m_schematic.CurrentSheet().clear();

    m_manager.LoadProject( pro.GetFullPath() );
    m_manager.Prj().SetElem( PROJECT::ELEM_SCH_SYMBOL_LIBS, nullptr );

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
    sheets.UpdateSheetInstances( m_schematic.RootScreen()->GetSheetInstances() );

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();
}


wxFileName TEST_SCH_SHEET_LIST_FIXTURE::buildFullPath( const wxString& aRelativePath )
{
    wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
    fn.AppendDir( "netlists" );

    wxString path = fn.GetFullPath();
    path += aRelativePath + wxT( "." ) + KiCadSchematicFileExtension;

    return wxFileName( path );
}


BOOST_FIXTURE_TEST_SUITE( SchSheetList, TEST_SCH_SHEET_LIST_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSheetListPageProperties )
{
    loadSchematic( "complex_hierarchy/complex_hierarchy" );

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();

    BOOST_CHECK( sheets.AllSheetPageNumbersEmpty() );

    sheets.SetInitialPageNumbers();

    // The root sheet should now be page 1.
    BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "1" );
    BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "2" );
    BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "3" );
}


BOOST_AUTO_TEST_CASE( TestEditPageNumbersInSharedDesign )
{
    BOOST_TEST_CONTEXT( "Read Sub-Sheet, prior to modification" )
    {
        // Check the Sub Sheet has the expected page numbers
        loadSchematic( "complex_hierarchy_shared/ampli_ht/ampli_ht" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 2 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "i" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "ii" );
    }

    BOOST_TEST_CONTEXT( "Read Root Sheet, prior to modification" )
    {
        // Check the parent sheet has the expected page numbers
        loadSchematic( "complex_hierarchy_shared/complex_hierarchy" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 5 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "1" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "2" );
        BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "3" );
        BOOST_CHECK_EQUAL( sheets.at( 3 ).GetPageNumber(), "4" );
        BOOST_CHECK_EQUAL( sheets.at( 4 ).GetPageNumber(), "5" );
    }

    BOOST_TEST_CONTEXT( "Modify page numbers in root sheet" )
    {
        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        // Amend Page numbers
        sheets.at( 0 ).SetPageNumber( "A" );
        sheets.at( 1 ).SetPageNumber( "B" );
        sheets.at( 2 ).SetPageNumber( "C" );
        sheets.at( 3 ).SetPageNumber( "D" );
        sheets.at( 4 ).SetPageNumber( "E" );

        // Save and reload
        wxString   tempName = "complex_hierarchy_shared/complex_hierarchy_modified";
        wxFileName tempFn = buildFullPath( tempName );
        m_pi->Save( tempFn.GetFullPath(), &m_schematic.Root(), &m_schematic );
        loadSchematic( tempName );

        sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 5 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "A" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "B" );
        BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "C" );
        BOOST_CHECK_EQUAL( sheets.at( 3 ).GetPageNumber(), "D" );
        BOOST_CHECK_EQUAL( sheets.at( 4 ).GetPageNumber(), "E" );

        // Cleanup
        wxRemoveFile( tempFn.GetFullPath() );
    }

    BOOST_TEST_CONTEXT( "Read Sub-Sheet, after modification" )
    {
        // Check the Sub Sheet has the expected page numbers
        // (This should not have been modified after editing the root sheet)
        loadSchematic( "complex_hierarchy_shared/ampli_ht/ampli_ht" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 2 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "i" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "ii" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
