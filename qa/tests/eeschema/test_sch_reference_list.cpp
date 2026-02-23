/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_reference_list.h>
#include <sch_sheet_path.h> // SCH_MULTI_UNIT_REFERENCE_MAP


struct REANNOTATED_REFERENCE
{
    wxString m_KIID;                      ///< KIID of the symbol to reannotate
    wxString m_OriginalRef;               ///< Original Reference Designator (prior to reannotating)
    wxString m_ExpectedRef;               ///< Expected Reference Designator (after reannotating)
    bool     m_IncludeInReannotationList; ///< True if reference is "selected" for reannotation
};


class TEST_SCH_REFERENCE_LIST_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    void loadTestCase( wxString aSchematicRelativePath, std::vector<REANNOTATED_REFERENCE> aRefs );
    void setupRefDesTrackerWithPreloads( const std::vector<std::string>& preloads );
    void validateTrackerIntegration();

    SCH_SYMBOL* getSymbolByKIID( wxString aKIID, SCH_SHEET_PATH* aSymbolPath );

    SCH_REFERENCE_LIST getAdditionalRefs();

    void        checkAnnotation( std::vector<REANNOTATED_REFERENCE> aRefs );

    SCH_REFERENCE_LIST           m_refsToReannotate;
    SCH_MULTI_UNIT_REFERENCE_MAP m_lockedRefs;
};


void TEST_SCH_REFERENCE_LIST_FIXTURE::loadTestCase( wxString aSchematicRelativePath,
                                                    std::vector<REANNOTATED_REFERENCE> aRefs )
{
    m_refsToReannotate.Clear();
    m_lockedRefs.clear();

    LoadSchematic( SchematicQAPath( aSchematicRelativePath ) );

    // Create list of references to reannotate
    for( REANNOTATED_REFERENCE ref : aRefs )
    {
        SCH_SHEET_PATH symbolPath;
        SCH_SYMBOL*    symbol = getSymbolByKIID( ref.m_KIID, &symbolPath );

        //Make sure test case is built properly
        BOOST_REQUIRE_NE( symbol, nullptr );
        BOOST_REQUIRE_EQUAL( symbol->GetRef( &symbolPath, true ), ref.m_OriginalRef );

        if( ref.m_IncludeInReannotationList )
        {
            symbolPath.AppendSymbol( m_refsToReannotate, symbol );
            symbolPath.AppendMultiUnitSymbol( m_lockedRefs, symbol );
        }
    }
}


SCH_SYMBOL* TEST_SCH_REFERENCE_LIST_FIXTURE::getSymbolByKIID( wxString        aKIID,
                                                              SCH_SHEET_PATH* aSymbolPath )
{
    KIID        symKIID( aKIID );
    SCH_ITEM*   foundItem = m_schematic->ResolveItem( symKIID, aSymbolPath );
    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( foundItem );

    return symbol;
};


SCH_REFERENCE_LIST TEST_SCH_REFERENCE_LIST_FIXTURE::getAdditionalRefs()
{
    // Build List of additional references to pass into Annotate()
    SCH_REFERENCE_LIST allRefs, additionalRefs;

    m_schematic->BuildSheetListSortedByPageNumbers().GetSymbols( allRefs );

    for( size_t i = 0; i < allRefs.GetCount(); ++i )
    {
        if( !m_refsToReannotate.Contains( allRefs[i] ) )
            additionalRefs.AddItem( allRefs[i] );
    }

    return additionalRefs;
}


void TEST_SCH_REFERENCE_LIST_FIXTURE::checkAnnotation( std::vector<REANNOTATED_REFERENCE> aRefs )
{
    for( REANNOTATED_REFERENCE ref : aRefs )
    {
        SCH_SHEET_PATH symbolPath;
        SCH_SYMBOL*    symbol = getSymbolByKIID( ref.m_KIID, &symbolPath );

        BOOST_CHECK_EQUAL( symbol->GetRef( &symbolPath, true ), ref.m_ExpectedRef );
    }
}


BOOST_FIXTURE_TEST_SUITE( SchReferenceList, TEST_SCH_REFERENCE_LIST_FIXTURE )


struct REANNOTATION_CASE
{
    std::string m_caseName;
    wxString    m_SchematicRelativePath;
    int         m_StartNumber;
    std::vector<REANNOTATED_REFERENCE> m_ExpectedReannotations;
};

// Case 1: same value, same libref
// Case 2: same value, different libref
// Case 3: different value, same libref
// Case 4: Not annotated unit to reannotate
// Case 5: Duplicate references
static const std::vector<REANNOTATION_CASE> reannotationCases = {
    { "CASE 1. Rename only selected. All units selected",
      "test_multiunit_reannotate",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U3A", true },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 1. Rename only selected. Only unit B selected (A and C should NOT be reannotated)",
      "test_multiunit_reannotate",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U2B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U99C", false },
      } },
    { "CASE 1. Rename only selected. Only units B and C selected (A should NOT be reannotated)",
      "test_multiunit_reannotate",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 2. Rename only selected. All units selected",
      "test_multiunit_reannotate_2",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U3A", true },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 2. Rename only selected. Only unit B selected (A and C should NOT be reannotated)",
      "test_multiunit_reannotate_2",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U99C", false },
      } },
    { "CASE 2. Rename only selected. Only units B and C selected (A should NOT be reannotated)",
      "test_multiunit_reannotate_2",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 3. Rename only selected. All units selected",
      "test_multiunit_reannotate_3",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U3A", true },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 3. Rename only selected. Only unit B selected (A and C should NOT be reannotated)",
      "test_multiunit_reannotate_3",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U99C", false },
      } },
    { "CASE 3. Rename only selected. Only units B and C selected (A should NOT be reannotated)",
      "test_multiunit_reannotate_3",
      1,
      {
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U99A", "U99A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U99B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U99C", "U3C", true },
      } },
    { "CASE 4 - Not previously annotated (does not get added to multi-unit locked group)",
      "test_multiunit_reannotate_4",
      1,
      {
              { "549455c3-ab6e-454e-94b0-5ca9e521ae0b", "U?B", "U2B", true },
      } },
    { "CASE 5 - Duplicate annotation. 1 selected",
      "test_multiunit_reannotate_5",
      10,
      {
              { "d43a1d25-d37a-467a-8b09-10cf2e2ace09", "U2A", "U2A", false },
              { "cd562bae-2426-44e6-8196-59eee5439809", "U2B", "U2B", false },
              { "3f20a749-efe3-4804-8fef-435caaa8dacb", "U2C", "U2C", false },
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U2A", "U2A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U2B", "U11B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U2C", "U2C", false },
      } },
    { "CASE 5 - Duplicate annotation. 2 selected",
      "test_multiunit_reannotate_5",
      10,
      {
              { "d43a1d25-d37a-467a-8b09-10cf2e2ace09", "U2A", "U2A", false },
              { "cd562bae-2426-44e6-8196-59eee5439809", "U2B", "U11B", true },
              { "3f20a749-efe3-4804-8fef-435caaa8dacb", "U2C", "U2C", false },
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U2A", "U2A", false },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U2B", "U12B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U2C", "U2C", false },
      } },
};

// @todo simplify or refactor this test case.
// Currently it simulates part of SCH_EDIT_FRAME::AnnotateSymbols
BOOST_AUTO_TEST_CASE( Reannotate )
{
    for( const REANNOTATION_CASE& c : reannotationCases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_caseName );

        loadTestCase( c.m_SchematicRelativePath, c.m_ExpectedReannotations );
        m_refsToReannotate.SetRefDesTracker( m_schematic->Settings().m_refDesTracker );
        m_refsToReannotate.RemoveAnnotation();
        m_refsToReannotate.SplitReferences();
        m_refsToReannotate.Annotate( false, 0, c.m_StartNumber, m_lockedRefs, getAdditionalRefs() );
        m_refsToReannotate.UpdateAnnotation();

        checkAnnotation( c.m_ExpectedReannotations );
    }
}


struct DUPLICATE_REANNOTATION_CASE
{
    std::string                        m_caseName;
    wxString                           m_SchematicRelativePath;
    std::vector<REANNOTATED_REFERENCE> m_ExpectedReannotations;
};


static const std::vector<DUPLICATE_REANNOTATION_CASE> reannotateDuplicatesCases = {
    { "Reannotate Duplicates. Simple case",
      "test_multiunit_reannotate_5",
      {
              { "d43a1d25-d37a-467a-8b09-10cf2e2ace09", "U2A", "U2A", false },
              { "cd562bae-2426-44e6-8196-59eee5439809", "U2B", "U2B", false },
              { "3f20a749-efe3-4804-8fef-435caaa8dacb", "U2C", "U2C", false },
              { "cf058f25-2bad-4c49-a0c4-f059825c427f", "U2A", "U3A", true },
              { "e6c8127f-e282-4128-8744-05f7893bc3ec", "U2B", "U3B", true },
              { "db066797-b21c-4c1c-9591-8c7c549f8087", "U2C", "U3C", true },
      } },
};


BOOST_AUTO_TEST_CASE( ReannotateDuplicates )
{
    for( const DUPLICATE_REANNOTATION_CASE& c : reannotateDuplicatesCases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_caseName );

        loadTestCase( c.m_SchematicRelativePath, c.m_ExpectedReannotations );

        m_refsToReannotate.SetRefDesTracker( m_schematic->Settings().m_refDesTracker );
        m_refsToReannotate.ReannotateDuplicates( getAdditionalRefs(), INCREMENTAL_BY_REF );
        m_refsToReannotate.UpdateAnnotation();

        checkAnnotation( c.m_ExpectedReannotations );
    }
}


BOOST_AUTO_TEST_CASE( ReferenceListDoesNotMutateEmptyValue )
{
    loadTestCase( "test_multiunit_reannotate", {} );

    SCH_SHEET_PATH sheetPath = m_schematic->CurrentSheet();
    SCH_SYMBOL*    symbol = nullptr;

    for( SCH_ITEM* item : sheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        symbol = static_cast<SCH_SYMBOL*>( item );
        break;
    }

    BOOST_REQUIRE( symbol != nullptr );

    symbol->SetValueFieldText( wxEmptyString );
    BOOST_REQUIRE( symbol->GetValue( false, &sheetPath, false ).IsEmpty() );

    SCH_REFERENCE_LIST refs;
    sheetPath.AppendSymbol( refs, symbol );

    BOOST_CHECK( symbol->GetValue( false, &sheetPath, false ).IsEmpty() );
    BOOST_REQUIRE_EQUAL( refs.GetCount(), 1 );
    BOOST_CHECK_EQUAL( refs[0].GetValue(), wxT( "~" ) );
}


/**
 * Test for issue #23183: multi-unit symbols with the same value should maintain
 * unit grouping when reannotated with reset. When annotations are cleared before
 * building locked groups (old design block placement behavior), the locked groups
 * are empty and units from different symbols can get mixed during annotation.
 * The fix preserves original refs for locked group construction, then resets.
 */
BOOST_AUTO_TEST_CASE( ReannotateSameValueMultiUnitPreservesGrouping )
{
    // Load a schematic with two 3-unit LM2903 (U1, U2), same value, interleaved X positions.
    // Sort by X gives: U1A, U2B, U1B, U2A, U1C, U2C
    // Without locked groups this would mix units across the two symbols.
    LoadSchematic( SchematicQAPath( "test_multiunit_reannotate_same_value" ) );

    SCH_SHEET_PATH sheetPath = m_schematic->CurrentSheet();

    // Build a map from UUID to symbol for verification
    std::map<wxString, SCH_SYMBOL*> symbolsByUUID;

    for( SCH_ITEM* item : sheetPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        symbolsByUUID[ sym->m_Uuid.AsString() ] = sym;
    }

    BOOST_REQUIRE_EQUAL( symbolsByUUID.size(), 6u );

    // The six symbol UUIDs (from our test schematic)
    wxString U1_unit1 = "aa000001-0000-0000-0000-000000000001";
    wxString U1_unit2 = "aa000001-0000-0000-0000-000000000002";
    wxString U1_unit3 = "aa000001-0000-0000-0000-000000000003";
    wxString U2_unit1 = "bb000002-0000-0000-0000-000000000001";
    wxString U2_unit2 = "bb000002-0000-0000-0000-000000000002";
    wxString U2_unit3 = "bb000002-0000-0000-0000-000000000003";

    // Verify initial annotations
    BOOST_CHECK_EQUAL( symbolsByUUID[U1_unit1]->GetRef( &sheetPath, true ), "U1A" );
    BOOST_CHECK_EQUAL( symbolsByUUID[U1_unit2]->GetRef( &sheetPath, true ), "U1B" );
    BOOST_CHECK_EQUAL( symbolsByUUID[U1_unit3]->GetRef( &sheetPath, true ), "U1C" );
    BOOST_CHECK_EQUAL( symbolsByUUID[U2_unit1]->GetRef( &sheetPath, true ), "U2A" );
    BOOST_CHECK_EQUAL( symbolsByUUID[U2_unit2]->GetRef( &sheetPath, true ), "U2B" );
    BOOST_CHECK_EQUAL( symbolsByUUID[U2_unit3]->GetRef( &sheetPath, true ), "U2C" );

    // Simulate the FIXED design block placement flow:
    // Build locked groups BEFORE clearing, then use aResetAnnotation=true.
    // This keeps multi-unit groups intact.
    SCH_REFERENCE_LIST references;
    SCH_MULTI_UNIT_REFERENCE_MAP lockedSymbols;

    for( auto& [uuid, sym] : symbolsByUUID )
    {
        sheetPath.AppendSymbol( references, sym );
        sheetPath.AppendMultiUnitSymbol( lockedSymbols, sym );
    }

    BOOST_REQUIRE_EQUAL( references.GetCount(), 6u );

    // Locked groups should have entries for U1 and U2 (3 units each)
    BOOST_REQUIRE_EQUAL( lockedSymbols.size(), 2u );
    BOOST_CHECK( lockedSymbols.count( "U1" ) );
    BOOST_CHECK( lockedSymbols.count( "U2" ) );
    BOOST_CHECK_EQUAL( lockedSymbols["U1"].GetCount(), 3u );
    BOOST_CHECK_EQUAL( lockedSymbols["U2"].GetCount(), 3u );

    // Mark as needing reannotation (equivalent to aResetAnnotation=true path)
    references.RemoveAnnotation();
    references.SetRefDesTracker( m_schematic->Settings().m_refDesTracker );
    references.SplitReferences();
    references.Annotate( false, 0, 1, lockedSymbols, SCH_REFERENCE_LIST() );
    references.UpdateAnnotation();

    // All units of the same original symbol should share one reference designator.
    // The exact number (U1 vs U2) depends on sort order, but the grouping must be preserved.
    wxString u1a_ref = symbolsByUUID[U1_unit1]->GetRef( &sheetPath, false );
    wxString u1b_ref = symbolsByUUID[U1_unit2]->GetRef( &sheetPath, false );
    wxString u1c_ref = symbolsByUUID[U1_unit3]->GetRef( &sheetPath, false );
    wxString u2a_ref = symbolsByUUID[U2_unit1]->GetRef( &sheetPath, false );
    wxString u2b_ref = symbolsByUUID[U2_unit2]->GetRef( &sheetPath, false );
    wxString u2c_ref = symbolsByUUID[U2_unit3]->GetRef( &sheetPath, false );

    // All three units of symbol "A" (originally U1) must share the same ref number
    BOOST_CHECK_EQUAL( u1a_ref, u1b_ref );
    BOOST_CHECK_EQUAL( u1b_ref, u1c_ref );

    // All three units of symbol "B" (originally U2) must share the same ref number
    BOOST_CHECK_EQUAL( u2a_ref, u2b_ref );
    BOOST_CHECK_EQUAL( u2b_ref, u2c_ref );

    // The two symbols must have different reference designators
    BOOST_CHECK_NE( u1a_ref, u2a_ref );
}


BOOST_AUTO_TEST_SUITE_END()
