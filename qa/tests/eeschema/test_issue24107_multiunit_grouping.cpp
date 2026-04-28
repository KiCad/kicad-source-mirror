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
 */

/**
 * @file test_issue24107_multiunit_grouping.cpp
 * Test for issue #24107: Unexpected multi-unit symbol grouping behavior
 *
 * Pre-annotation, when units of one multi-unit library symbol (e.g. AD8620, three units)
 * are placed and then a different multi-unit library symbol (e.g. OPA1664, four units) is
 * placed with the same reference prefix ("U?"), the second symbol must NOT be treated as a
 * continuation of the first symbol's units.
 *
 * GetUnplacedUnitsForSymbol previously returned the missing units of the merged
 * "U? prefix" pseudo-component, which made an OPA1664 placed after three AD8620 units jump
 * to unit D (4) instead of starting at unit A (1).
 */

#include <boost/test/unit_test.hpp>

#include <lib_id.h>
#include <lib_symbol.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tools/sch_tool_utils.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct ISSUE24107_FIXTURE
{
    ISSUE24107_FIXTURE() : m_settingsManager()
    {
        wxString tempDir     = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_issue24107.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project   = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
        m_schematic->CreateDefaultScreens();
    }

    ~ISSUE24107_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    /**
     * Build a fresh multi-unit library symbol with the given lib name, value and unit count.
     */
    std::shared_ptr<LIB_SYMBOL> makeMultiUnitLib( const wxString& aLibItemName, int aUnitCount,
                                                  const wxString& aValue )
    {
        auto lib = std::make_shared<LIB_SYMBOL>( aLibItemName );
        lib->SetUnitCount( aUnitCount, false );
        lib->GetReferenceField().SetText( wxT( "U" ) );
        lib->GetValueField().SetText( aValue );

        LIB_ID id;
        id.SetLibNickname( wxT( "TestLib" ) );
        id.SetLibItemName( aLibItemName );
        lib->SetLibId( id );

        return lib;
    }

    /**
     * Place a single unit of a multi-unit symbol in the schematic.
     * The symbol is left in the unannotated ("U?") state.
     */
    SCH_SYMBOL* placeUnit( const std::shared_ptr<LIB_SYMBOL>& aLib, int aUnit,
                           const VECTOR2I& aPos )
    {
        SCH_SHEET*           rootSheet = m_schematic->GetTopLevelSheets()[0];
        SCH_SHEET_PATH       sheetPath;
        sheetPath.push_back( rootSheet );

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *aLib, aLib->GetLibId(), &sheetPath, aUnit, 0,
                                             aPos, m_schematic.get() );
        symbol->SetUnit( aUnit );
        symbol->SetUnitSelection( &sheetPath, aUnit );

        rootSheet->GetScreen()->Append( symbol );

        return symbol;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT*                   m_project;
    std::vector<wxString>      m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue24107MultiUnitGrouping, ISSUE24107_FIXTURE )


/**
 * Reproduces the user-reported flow.
 *
 *  - Place AD8620 units 1, 2, 3 (three-unit op-amp), unannotated.
 *  - Place a fresh OPA1664 (four-unit op-amp) at unit 1.
 *  - Ask for the unplaced units of that OPA1664.
 *
 * Before the fix the function reports {} (because U?A/U?B/U?C of AD8620 are mistakenly
 * counted as occupying OPA1664's first three units, and U?A of OPA1664 covers unit 4).
 * After the fix the function must report units 2, 3, 4 as unplaced for the OPA1664
 * placement, because those units of OPA1664 itself have not yet been placed.
 */
BOOST_AUTO_TEST_CASE( UnplacedUnitsAreScopedToLibraryIdentity )
{
    auto ad8620Lib  = makeMultiUnitLib( wxT( "AD8620" ), 3, wxT( "AD8620" ) );
    auto opa1664Lib = makeMultiUnitLib( wxT( "OPA1664" ), 4, wxT( "OPA1664" ) );

    placeUnit( ad8620Lib, 1, VECTOR2I( 0, 0 ) );
    placeUnit( ad8620Lib, 2, VECTOR2I( 1000000, 0 ) );
    placeUnit( ad8620Lib, 3, VECTOR2I( 2000000, 0 ) );

    SCH_SYMBOL* opa = placeUnit( opa1664Lib, 1, VECTOR2I( 0, 1000000 ) );

    std::set<int> missing = GetUnplacedUnitsForSymbol( *opa );

    // Units 2, 3 and 4 of OPA1664 itself have not been placed.
    BOOST_CHECK_EQUAL( missing.size(), 3 );
    BOOST_CHECK( missing.count( 1 ) == 0 );
    BOOST_CHECK( missing.count( 2 ) == 1 );
    BOOST_CHECK( missing.count( 3 ) == 1 );
    BOOST_CHECK( missing.count( 4 ) == 1 );
}


/**
 * When a fresh OPA1664 is placed first, its own unit 1 should be marked as occupied while the
 * remaining units (2, 3, 4) should be reported as unplaced. AD8620 units sitting on the same
 * sheet must not affect the result.
 */
BOOST_AUTO_TEST_CASE( UnannotatedUnitsFromOtherLibraryAreIgnored )
{
    auto ad8620Lib  = makeMultiUnitLib( wxT( "AD8620" ), 3, wxT( "AD8620" ) );
    auto opa1664Lib = makeMultiUnitLib( wxT( "OPA1664" ), 4, wxT( "OPA1664" ) );

    SCH_SYMBOL* opaA = placeUnit( opa1664Lib, 1, VECTOR2I( 0, 0 ) );

    placeUnit( ad8620Lib, 1, VECTOR2I( 0, 1000000 ) );
    placeUnit( ad8620Lib, 2, VECTOR2I( 1000000, 1000000 ) );

    std::set<int> missing = GetUnplacedUnitsForSymbol( *opaA );

    BOOST_CHECK_EQUAL( missing.size(), 3 );
    BOOST_CHECK( missing.count( 1 ) == 0 );
    BOOST_CHECK( missing.count( 2 ) == 1 );
    BOOST_CHECK( missing.count( 3 ) == 1 );
    BOOST_CHECK( missing.count( 4 ) == 1 );
}


/**
 * Two different unannotated multi-unit symbols sharing the same reference prefix and unit
 * number must not be aliased. AD8620 unit 2 is occupied for AD8620 itself, regardless of
 * which OPA1664 units are present.
 */
BOOST_AUTO_TEST_CASE( SameLibraryUnitsStillCount )
{
    auto ad8620Lib  = makeMultiUnitLib( wxT( "AD8620" ), 3, wxT( "AD8620" ) );
    auto opa1664Lib = makeMultiUnitLib( wxT( "OPA1664" ), 4, wxT( "OPA1664" ) );

    SCH_SYMBOL* adA = placeUnit( ad8620Lib, 1, VECTOR2I( 0, 0 ) );
    placeUnit( ad8620Lib, 2, VECTOR2I( 1000000, 0 ) );
    placeUnit( opa1664Lib, 1, VECTOR2I( 0, 1000000 ) );

    std::set<int> missing = GetUnplacedUnitsForSymbol( *adA );

    // Only AD8620 unit 3 remains unplaced.
    BOOST_CHECK_EQUAL( missing.size(), 1 );
    BOOST_CHECK( missing.count( 3 ) == 1 );
}


/**
 * Direct coverage for the helper consumed by SCH_DRAWING_TOOLS::PlaceSymbol() in
 * placeAllUnits mode. The helper must agree with the ref/lib-id-based identity check used by
 * GetUnplacedUnitsForSymbol(), but operates on a pre-built SCH_REFERENCE_LIST rather than
 * walking the hierarchy.
 */
BOOST_AUTO_TEST_CASE( IsUnannotatedUnitOccupiedRespectsLibraryIdentity )
{
    auto ad8620Lib  = makeMultiUnitLib( wxT( "AD8620" ), 3, wxT( "AD8620" ) );
    auto opa1664Lib = makeMultiUnitLib( wxT( "OPA1664" ), 4, wxT( "OPA1664" ) );

    placeUnit( ad8620Lib, 1, VECTOR2I( 0, 0 ) );
    placeUnit( ad8620Lib, 2, VECTOR2I( 1000000, 0 ) );
    placeUnit( ad8620Lib, 3, VECTOR2I( 2000000, 0 ) );

    SCH_SHEET_LIST     hierarchy = m_schematic->Hierarchy();
    SCH_REFERENCE_LIST refs;
    hierarchy.GetSymbols( refs, SYMBOL_FILTER_ALL );

    const wxString unannotatedRef = wxT( "U?" );

    // OPA1664: every unit must report unoccupied even though the reference string matches
    // AD8620 placements 1..3.
    BOOST_CHECK( !IsUnannotatedUnitOccupied( refs, unannotatedRef, opa1664Lib->GetLibId(), 1 ) );
    BOOST_CHECK( !IsUnannotatedUnitOccupied( refs, unannotatedRef, opa1664Lib->GetLibId(), 2 ) );
    BOOST_CHECK( !IsUnannotatedUnitOccupied( refs, unannotatedRef, opa1664Lib->GetLibId(), 3 ) );
    BOOST_CHECK( !IsUnannotatedUnitOccupied( refs, unannotatedRef, opa1664Lib->GetLibId(), 4 ) );

    // AD8620: units 1..3 must report occupied.
    BOOST_CHECK( IsUnannotatedUnitOccupied( refs, unannotatedRef, ad8620Lib->GetLibId(), 1 ) );
    BOOST_CHECK( IsUnannotatedUnitOccupied( refs, unannotatedRef, ad8620Lib->GetLibId(), 2 ) );
    BOOST_CHECK( IsUnannotatedUnitOccupied( refs, unannotatedRef, ad8620Lib->GetLibId(), 3 ) );
}


BOOST_AUTO_TEST_SUITE_END()
