/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_issue22576_subsheet_standalone.cpp
 *
 * Test for issue #22576: Schematic Sheet Appears different if opened from
 * Project Files vs. Schematic Hierarchy.
 *
 * When a subsheet file is opened directly as a standalone project (e.g., from
 * File Browser or Project Files panel), symbols that only have instance data
 * from the parent hierarchy should still display their references correctly
 * instead of showing unannotated references (e.g., "R?" instead of "R202").
 *
 * The fix ensures CheckForMissingSymbolInstances uses existing instance data
 * as a fallback when creating instances for a new project context.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ISSUE22576_FIXTURE
{
    ISSUE22576_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that CheckForMissingSymbolInstances uses existing instance data as fallback.
 *
 * When a schematic file with existing instances is opened in a different project context
 * (simulating opening a subsheet directly from File Browser), symbols that have instance
 * data from other projects should use that data instead of creating unannotated references.
 */
BOOST_FIXTURE_TEST_CASE( Issue22576MissingInstancesFallback, ISSUE22576_FIXTURE )
{
    LOCALE_IO dummy;

    // Create a minimal schematic with a symbol that has existing instance data
    m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    m_schematic->Reset();

    SCH_SHEET* rootSheet = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
    rootSheet->SetScreen( screen );

    // Create a symbol
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetLibId( LIB_ID( wxT( "Device" ), wxT( "R" ) ) );
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    symbol->GetField( FIELD_T::REFERENCE )->SetText( wxT( "R202" ) );
    symbol->SetPrefix( wxT( "R" ) );
    screen->Append( symbol );

    // Add an instance for a "different" project path (simulating the parent hierarchy)
    // This is what would exist if the file was saved as part of a hierarchical project
    KIID_PATH existingPath;
    existingPath.push_back( KIID() );  // Some other root sheet UUID
    existingPath.push_back( KIID() );  // This sheet's UUID in that hierarchy

    SCH_SYMBOL_INSTANCE existingInstance;
    existingInstance.m_Path = existingPath;
    existingInstance.m_Reference = wxT( "R202" );
    existingInstance.m_Unit = 1;
    existingInstance.m_ProjectName = wxT( "ParentProject" );
    symbol->AddHierarchicalReference( existingInstance );

    // Set up the schematic
    m_schematic->AddTopLevelSheet( rootSheet );

    // Now simulate opening as a standalone project
    // This creates a NEW path that doesn't match the existing instance
    SCH_SHEET_PATH newPath;
    newPath.push_back( rootSheet );
    newPath.Rehash();

    // Before the fix, CheckForMissingSymbolInstances would set unannotated reference "R?"
    // After the fix, it should use the existing instance data "R202"
    wxString newProjectName = wxT( "StandaloneProject" );
    newPath.CheckForMissingSymbolInstances( newProjectName );

    // Verify the symbol now has an instance for the new path with the correct reference
    wxString ref = symbol->GetRef( &newPath );

    BOOST_TEST_MESSAGE( "Reference after CheckForMissingSymbolInstances: " << ref );

    // The reference should be "R202" (from existing instance), NOT "R?" (unannotated)
    BOOST_CHECK_MESSAGE( ref == wxT( "R202" ),
                         "Symbol reference should use existing instance data, not unannotated" );
    BOOST_CHECK_MESSAGE( !ref.EndsWith( wxT( "?" ) ),
                         "Symbol reference should NOT be unannotated" );
}


/**
 * Test that symbols with no existing instances fall back to field text.
 *
 * When a symbol has no instance data at all, CheckForMissingSymbolInstances
 * should fall back to using the REFERENCE field text.
 */
BOOST_FIXTURE_TEST_CASE( Issue22576NoInstancesFallback, ISSUE22576_FIXTURE )
{
    LOCALE_IO dummy;

    m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    m_schematic->Reset();

    SCH_SHEET* rootSheet = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
    rootSheet->SetScreen( screen );

    // Create a symbol with NO instances, only field text
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetLibId( LIB_ID( wxT( "Device" ), wxT( "C" ) ) );
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    symbol->GetField( FIELD_T::REFERENCE )->SetText( wxT( "C101" ) );
    symbol->SetPrefix( wxT( "C" ) );
    screen->Append( symbol );

    // Verify no instances exist
    BOOST_REQUIRE( symbol->GetInstances().empty() );

    m_schematic->AddTopLevelSheet( rootSheet );

    SCH_SHEET_PATH newPath;
    newPath.push_back( rootSheet );
    newPath.Rehash();

    wxString newProjectName = wxT( "NewProject" );
    newPath.CheckForMissingSymbolInstances( newProjectName );

    wxString ref = symbol->GetRef( &newPath );

    BOOST_TEST_MESSAGE( "Reference after CheckForMissingSymbolInstances (no prior instances): " << ref );

    // Should use the field text "C101"
    BOOST_CHECK_MESSAGE( ref == wxT( "C101" ),
                         "Symbol reference should use field text when no instances exist" );
}


/**
 * Test that multiple symbols all get proper fallback references.
 *
 * When multiple symbols exist and only some have instances for the current path,
 * each should get the appropriate reference from existing instances or field text.
 */
BOOST_FIXTURE_TEST_CASE( Issue22576MultipleSymbolsFallback, ISSUE22576_FIXTURE )
{
    LOCALE_IO dummy;

    m_schematic = std::make_unique<SCHEMATIC>( nullptr );
    m_schematic->Reset();

    SCH_SHEET* rootSheet = new SCH_SHEET( m_schematic.get() );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
    rootSheet->SetScreen( screen );

    // Create the "current" path first
    m_schematic->AddTopLevelSheet( rootSheet );

    SCH_SHEET_PATH currentPath;
    currentPath.push_back( rootSheet );
    currentPath.Rehash();

    // Path for "other project" instances
    KIID_PATH otherPath;
    otherPath.push_back( KIID() );
    otherPath.push_back( KIID() );

    // Symbol 1: Has instance for other path only (should use that instance's ref)
    SCH_SYMBOL* symbol1 = new SCH_SYMBOL();
    symbol1->SetLibId( LIB_ID( wxT( "Device" ), wxT( "R" ) ) );
    symbol1->SetPosition( VECTOR2I( 0, 0 ) );
    symbol1->GetField( FIELD_T::REFERENCE )->SetText( wxT( "R?" ) );  // Field is unannotated
    symbol1->SetPrefix( wxT( "R" ) );

    SCH_SYMBOL_INSTANCE inst1;
    inst1.m_Path = otherPath;
    inst1.m_Reference = wxT( "R202" );
    inst1.m_Unit = 1;
    inst1.m_ProjectName = wxT( "OtherProject" );
    symbol1->AddHierarchicalReference( inst1 );
    screen->Append( symbol1 );

    // Symbol 2: Has instance for current path (should keep it)
    SCH_SYMBOL* symbol2 = new SCH_SYMBOL();
    symbol2->SetLibId( LIB_ID( wxT( "Device" ), wxT( "C" ) ) );
    symbol2->SetPosition( VECTOR2I( 1000, 0 ) );
    symbol2->GetField( FIELD_T::REFERENCE )->SetText( wxT( "C101" ) );
    symbol2->SetPrefix( wxT( "C" ) );
    symbol2->SetRef( &currentPath, wxT( "C101" ) );
    screen->Append( symbol2 );

    // Symbol 3: No instances at all (should use field text)
    SCH_SYMBOL* symbol3 = new SCH_SYMBOL();
    symbol3->SetLibId( LIB_ID( wxT( "Device" ), wxT( "L" ) ) );
    symbol3->SetPosition( VECTOR2I( 2000, 0 ) );
    symbol3->GetField( FIELD_T::REFERENCE )->SetText( wxT( "L1" ) );
    symbol3->SetPrefix( wxT( "L" ) );
    screen->Append( symbol3 );

    wxString projectName = wxT( "CurrentProject" );
    currentPath.CheckForMissingSymbolInstances( projectName );

    // Verify each symbol
    wxString ref1 = symbol1->GetRef( &currentPath );
    wxString ref2 = symbol2->GetRef( &currentPath );
    wxString ref3 = symbol3->GetRef( &currentPath );

    BOOST_TEST_MESSAGE( "Symbol1 (other instance only): " << ref1 );
    BOOST_TEST_MESSAGE( "Symbol2 (current instance): " << ref2 );
    BOOST_TEST_MESSAGE( "Symbol3 (no instances): " << ref3 );

    BOOST_CHECK_MESSAGE( ref1 == wxT( "R202" ),
                         "Symbol with other-path instance should use that reference" );
    BOOST_CHECK_MESSAGE( ref2 == wxT( "C101" ),
                         "Symbol with current-path instance should keep its reference" );
    BOOST_CHECK_MESSAGE( ref3 == wxT( "L1" ),
                         "Symbol with no instances should use field text" );
}
