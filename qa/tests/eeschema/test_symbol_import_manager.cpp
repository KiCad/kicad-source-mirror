/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file test_symbol_import_manager.cpp
 * Test suite for SYMBOL_IMPORT_MANAGER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <symbol_import_manager.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/sch_io.h>
#include <ki_exception.h>
#include <lib_symbol.h>
#include <wx/filename.h>


class TEST_SYMBOL_IMPORT_MANAGER_FIXTURE
{
public:
    TEST_SYMBOL_IMPORT_MANAGER_FIXTURE()
    {
        // Get the path to our test libraries
        wxFileName kicadSymLib( KI_TEST::GetTestDataRootDir() );
        kicadSymLib.AppendDir( "eeschema" );
        kicadSymLib.AppendDir( "libs" );
        kicadSymLib.SetFullName( "4xxx.kicad_sym" );
        m_kicadSymLibPath = kicadSymLib.GetFullPath();

        wxFileName legacyLib( KI_TEST::GetTestDataRootDir() );
        legacyLib.AppendDir( "eeschema" );
        legacyLib.AppendDir( "libs" );
        legacyLib.SetFullName( "4xxx.lib" );
        m_legacyLibPath = legacyLib.GetFullPath();
    }

    /**
     * Load symbols from a KiCad symbol library into the manager.
     */
    bool LoadKiCadLibraryHelper( SYMBOL_IMPORT_MANAGER& aManager )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

        if( !pi )
            return false;

        wxArrayString symbolNames;

        try
        {
            pi->EnumerateSymbolLib( symbolNames, m_kicadSymLibPath );
        }
        catch( const IO_ERROR& )
        {
            return false;
        }

        for( const wxString& name : symbolNames )
        {
            try
            {
                LIB_SYMBOL* sym = pi->LoadSymbol( m_kicadSymLibPath, name );

                if( sym )
                {
                    wxString parentName = sym->GetParentName();
                    bool isPower = sym->IsPower();

                    // Don't pass symbol pointer to manager - LoadSymbol returns
                    // a cached pointer, not a copy. Don't delete - cache owns it.
                    aManager.AddSymbol( name, parentName, isPower, nullptr );
                }
            }
            catch( const IO_ERROR& )
            {
                // Add without full symbol data
                aManager.AddSymbol( name, wxEmptyString, false, nullptr );
            }
        }

        aManager.BuildDependencyMaps();
        return true;
    }

    /**
     * Load symbols from a legacy symbol library into the manager.
     */
    bool LoadLegacyLibraryHelper( SYMBOL_IMPORT_MANAGER& aManager )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

        if( !pi )
            return false;

        wxArrayString symbolNames;

        try
        {
            pi->EnumerateSymbolLib( symbolNames, m_legacyLibPath );
        }
        catch( const IO_ERROR& )
        {
            return false;
        }

        for( const wxString& name : symbolNames )
        {
            try
            {
                LIB_SYMBOL* sym = pi->LoadSymbol( m_legacyLibPath, name );

                if( sym )
                {
                    wxString parentName = sym->GetParentName();
                    bool isPower = sym->IsPower();

                    // Don't pass symbol pointer to manager - LoadSymbol returns
                    // a cached pointer, not a copy. Don't delete - cache owns it.
                    aManager.AddSymbol( name, parentName, isPower, nullptr );
                }
            }
            catch( const IO_ERROR& )
            {
                aManager.AddSymbol( name, wxEmptyString, false, nullptr );
            }
        }

        aManager.BuildDependencyMaps();
        return true;
    }

protected:
    wxString m_kicadSymLibPath;
    wxString m_legacyLibPath;
};


BOOST_FIXTURE_TEST_SUITE( SymbolImportManager, TEST_SYMBOL_IMPORT_MANAGER_FIXTURE )


/**
 * Test loading symbols from a KiCad symbol library.
 */
BOOST_AUTO_TEST_CASE( LoadKiCadSymbolLibrary )
{
    SYMBOL_IMPORT_MANAGER manager;

    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );
    BOOST_CHECK_GT( manager.GetSymbolCount(), 0 );

    // Should have 51 symbols based on our test data
    BOOST_CHECK_EQUAL( manager.GetSymbolCount(), 51 );
}


/**
 * Test loading symbols from a legacy symbol library.
 */
BOOST_AUTO_TEST_CASE( LoadLegacyLibrary )
{
    SYMBOL_IMPORT_MANAGER manager;

    BOOST_REQUIRE( LoadLegacyLibraryHelper( manager ) );
    BOOST_CHECK_GT( manager.GetSymbolCount(), 0 );

    // Should have 48 symbols based on our test data (44 DEF + 4 ALIAS entries)
    BOOST_CHECK_EQUAL( manager.GetSymbolCount(), 48 );
}


/**
 * Test dependency map building for KiCad symbols.
 */
BOOST_AUTO_TEST_CASE( DependencyMapKiCadSym )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Test known derived symbols from 4xxx.kicad_sym:
    // 14528, 14538, 4528 extend 4538
    // 4066 extends 4016

    // Check IsDerived
    BOOST_CHECK( manager.IsDerived( "14528" ) );
    BOOST_CHECK( manager.IsDerived( "14538" ) );
    BOOST_CHECK( manager.IsDerived( "4528" ) );
    BOOST_CHECK( manager.IsDerived( "4066" ) );

    // Check that base symbols are not derived
    BOOST_CHECK( !manager.IsDerived( "4538" ) );
    BOOST_CHECK( !manager.IsDerived( "4016" ) );
    BOOST_CHECK( !manager.IsDerived( "4001" ) );

    // Check GetParent
    BOOST_CHECK_EQUAL( manager.GetParent( "14528" ), "4538" );
    BOOST_CHECK_EQUAL( manager.GetParent( "14538" ), "4538" );
    BOOST_CHECK_EQUAL( manager.GetParent( "4528" ), "4538" );
    BOOST_CHECK_EQUAL( manager.GetParent( "4066" ), "4016" );
    BOOST_CHECK_EQUAL( manager.GetParent( "4538" ), "" );

    // Check GetDirectDerivatives
    std::vector<wxString> derivatives = manager.GetDirectDerivatives( "4538" );
    BOOST_CHECK_EQUAL( derivatives.size(), 3 );

    // Check 4016 has one derivative
    derivatives = manager.GetDirectDerivatives( "4016" );
    BOOST_CHECK_EQUAL( derivatives.size(), 1 );
    BOOST_CHECK_EQUAL( derivatives[0], "4066" );
}


/**
 * Test GetAncestors with single-level inheritance.
 */
BOOST_AUTO_TEST_CASE( GetAncestorsSingleLevel )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // 4066 extends 4016 (single level)
    std::set<wxString> ancestors = manager.GetAncestors( "4066" );
    BOOST_CHECK_EQUAL( ancestors.size(), 1 );
    BOOST_CHECK( ancestors.count( "4016" ) == 1 );

    // 4016 has no ancestors (is root)
    ancestors = manager.GetAncestors( "4016" );
    BOOST_CHECK_EQUAL( ancestors.size(), 0 );
}


/**
 * Test GetDescendants.
 */
BOOST_AUTO_TEST_CASE( GetDescendants )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // 4538 has 3 direct derivatives: 14528, 14538, 4528
    std::set<wxString> descendants = manager.GetDescendants( "4538" );
    BOOST_CHECK_EQUAL( descendants.size(), 3 );
    BOOST_CHECK( descendants.count( "14528" ) == 1 );
    BOOST_CHECK( descendants.count( "14538" ) == 1 );
    BOOST_CHECK( descendants.count( "4528" ) == 1 );

    // 4016 has 1 descendant: 4066
    descendants = manager.GetDescendants( "4016" );
    BOOST_CHECK_EQUAL( descendants.size(), 1 );
    BOOST_CHECK( descendants.count( "4066" ) == 1 );

    // A leaf symbol has no descendants
    descendants = manager.GetDescendants( "4066" );
    BOOST_CHECK_EQUAL( descendants.size(), 0 );
}


/**
 * Test symbol selection auto-selects ancestors.
 */
BOOST_AUTO_TEST_CASE( SelectAutoSelectsAncestors )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Select a derived symbol
    manager.SetSymbolSelected( "4066", true );

    // Check the derived symbol is checked
    const SYMBOL_IMPORT_INFO* info4066 = manager.GetSymbolInfo( "4066" );
    BOOST_REQUIRE( info4066 != nullptr );
    BOOST_CHECK( info4066->m_checked );
    BOOST_CHECK( !info4066->m_autoSelected );

    // Check the parent is auto-selected
    const SYMBOL_IMPORT_INFO* info4016 = manager.GetSymbolInfo( "4016" );
    BOOST_REQUIRE( info4016 != nullptr );
    BOOST_CHECK( !info4016->m_checked );
    BOOST_CHECK( info4016->m_autoSelected );

    // Both should be in the import list
    std::vector<wxString> toImport = manager.GetSymbolsToImport();
    BOOST_CHECK_EQUAL( toImport.size(), 2 );
}


/**
 * Test that selecting a parent does not auto-select descendants.
 */
BOOST_AUTO_TEST_CASE( SelectParentDoesNotSelectDescendants )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Select a parent symbol
    manager.SetSymbolSelected( "4538", true );

    // Check the parent is checked
    const SYMBOL_IMPORT_INFO* info4538 = manager.GetSymbolInfo( "4538" );
    BOOST_REQUIRE( info4538 != nullptr );
    BOOST_CHECK( info4538->m_checked );

    // Check descendants are NOT selected
    const SYMBOL_IMPORT_INFO* info14528 = manager.GetSymbolInfo( "14528" );
    BOOST_REQUIRE( info14528 != nullptr );
    BOOST_CHECK( !info14528->m_checked );
    BOOST_CHECK( !info14528->m_autoSelected );

    // Only the parent should be in the import list
    std::vector<wxString> toImport = manager.GetSymbolsToImport();
    BOOST_CHECK_EQUAL( toImport.size(), 1 );
}


/**
 * Test GetSelectedDescendants identifies which descendants will be orphaned.
 */
BOOST_AUTO_TEST_CASE( GetSelectedDescendants )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Select multiple derivatives of 4538
    manager.SetSymbolSelected( "14528", true );
    manager.SetSymbolSelected( "4528", true );

    // 4538 should be auto-selected
    const SYMBOL_IMPORT_INFO* info4538 = manager.GetSymbolInfo( "4538" );
    BOOST_REQUIRE( info4538 != nullptr );
    BOOST_CHECK( info4538->m_autoSelected );

    // GetSelectedDescendants should return the two selected derivatives
    std::vector<wxString> selectedDesc = manager.GetSelectedDescendants( "4538" );
    BOOST_CHECK_EQUAL( selectedDesc.size(), 2 );
}


/**
 * Test DeselectWithDescendants cascades deselection.
 */
BOOST_AUTO_TEST_CASE( DeselectWithDescendants )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Select parent and all derivatives
    manager.SetSymbolSelected( "4538", true );
    manager.SetSymbolSelected( "14528", true );
    manager.SetSymbolSelected( "14538", true );
    manager.SetSymbolSelected( "4528", true );

    // Verify all are selected
    BOOST_CHECK_EQUAL( manager.GetSymbolsToImport().size(), 4 );

    // Deselect parent with descendants
    manager.DeselectWithDescendants( "4538" );

    // All should be deselected
    BOOST_CHECK_EQUAL( manager.GetSymbolsToImport().size(), 0 );
}


/**
 * Test SelectAll selects all symbols.
 */
BOOST_AUTO_TEST_CASE( SelectAll )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    manager.SelectAll();

    // All symbols should be in import list
    BOOST_CHECK_EQUAL( manager.GetSymbolsToImport().size(), manager.GetSymbolCount() );

    // Manual count should match total (no auto-selections needed when all selected)
    BOOST_CHECK_EQUAL( manager.GetManualSelectionCount(), (int) manager.GetSymbolCount() );
}


/**
 * Test SelectAll with filter.
 */
BOOST_AUTO_TEST_CASE( SelectAllWithFilter )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Select only symbols starting with "40"
    manager.SelectAll( []( const wxString& name ) { return name.StartsWith( "40" ); } );

    // Should have some but not all symbols selected
    int selectedCount = manager.GetManualSelectionCount() + manager.GetAutoSelectionCount();
    BOOST_CHECK_GT( selectedCount, 0 );
    BOOST_CHECK_LT( selectedCount, (int) manager.GetSymbolCount() );
}


/**
 * Test DeselectAll clears all selections.
 */
BOOST_AUTO_TEST_CASE( DeselectAll )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    manager.SelectAll();
    BOOST_CHECK_GT( manager.GetSymbolsToImport().size(), 0 );

    manager.DeselectAll();
    BOOST_CHECK_EQUAL( manager.GetSymbolsToImport().size(), 0 );
}


/**
 * Test conflict detection with CheckExistingSymbols.
 */
BOOST_AUTO_TEST_CASE( ConflictDetection )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Simulate that some symbols exist in destination
    std::set<wxString> existingSymbols = { "4001", "4011", "4538" };
    manager.CheckExistingSymbols(
            [&existingSymbols]( const wxString& name ) { return existingSymbols.count( name ) > 0; } );

    // Check the flags are set correctly
    const SYMBOL_IMPORT_INFO* info4001 = manager.GetSymbolInfo( "4001" );
    BOOST_REQUIRE( info4001 != nullptr );
    BOOST_CHECK( info4001->m_existsInDest );

    const SYMBOL_IMPORT_INFO* info4002 = manager.GetSymbolInfo( "4002" );
    BOOST_REQUIRE( info4002 != nullptr );
    BOOST_CHECK( !info4002->m_existsInDest );

    // Select some symbols including conflicting ones
    manager.SetSymbolSelected( "4001", true );
    manager.SetSymbolSelected( "4002", true );

    // Check GetConflicts
    std::vector<wxString> conflicts = manager.GetConflicts();
    BOOST_CHECK_EQUAL( conflicts.size(), 1 );
    BOOST_CHECK_EQUAL( conflicts[0], "4001" );
}


/**
 * Test MatchesFilter for symbol name filtering.
 */
BOOST_AUTO_TEST_CASE( FilterMatching )
{
    // Case-insensitive contains matching
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "40" ) );
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "01" ) );
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "4001" ) );
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "CD4001", "cd" ) );
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "CD4001", "CD" ) );

    // Non-matching
    BOOST_CHECK( !SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "xyz" ) );
    BOOST_CHECK( !SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "4002" ) );

    // Empty filter matches everything
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "4001", "" ) );
    BOOST_CHECK( SYMBOL_IMPORT_MANAGER::MatchesFilter( "anything", "" ) );
}


/**
 * Test selection counts.
 */
BOOST_AUTO_TEST_CASE( SelectionCounts )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    // Initially no selections
    BOOST_CHECK_EQUAL( manager.GetManualSelectionCount(), 0 );
    BOOST_CHECK_EQUAL( manager.GetAutoSelectionCount(), 0 );

    // Select a derived symbol (should auto-select 1 ancestor)
    manager.SetSymbolSelected( "4066", true );
    BOOST_CHECK_EQUAL( manager.GetManualSelectionCount(), 1 );
    BOOST_CHECK_EQUAL( manager.GetAutoSelectionCount(), 1 );

    // Select another derived symbol with same parent (no new auto-selections)
    // (4066's parent is 4016, not related to 4538)
    // Select 14528 which extends 4538
    manager.SetSymbolSelected( "14528", true );
    BOOST_CHECK_EQUAL( manager.GetManualSelectionCount(), 2 );
    BOOST_CHECK_EQUAL( manager.GetAutoSelectionCount(), 2 );  // 4016 and 4538

    // Select the auto-selected parent manually (should decrease auto count)
    manager.SetSymbolSelected( "4538", true );
    BOOST_CHECK_EQUAL( manager.GetManualSelectionCount(), 3 );
    BOOST_CHECK_EQUAL( manager.GetAutoSelectionCount(), 1 );  // Only 4016 now
}


/**
 * Test Clear resets all state.
 */
BOOST_AUTO_TEST_CASE( ClearResetsState )
{
    SYMBOL_IMPORT_MANAGER manager;
    BOOST_REQUIRE( LoadKiCadLibraryHelper( manager ) );

    manager.SelectAll();
    BOOST_CHECK_GT( manager.GetSymbolCount(), 0 );

    manager.Clear();

    BOOST_CHECK_EQUAL( manager.GetSymbolCount(), 0 );
    BOOST_CHECK_EQUAL( manager.GetSymbolsToImport().size(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
