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
 * @file
 * Test suite for SCH_SYMBOL object.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

// Code under test
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <wildcards_and_files_ext.h>


class TEST_SCH_SYMBOL_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    SCH_SYMBOL* GetFirstSymbol()
    {
        if( !m_schematic )
            return nullptr;

        SCH_SCREEN* screen = m_schematic->RootScreen();

        if( !screen )
            return nullptr;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol )
                return symbol;
        }

        return nullptr;
    }

    ///< #SCH_SYMBOL object with no extra data set.
    SCH_SYMBOL m_symbol;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSymbol, TEST_SCH_SYMBOL_FIXTURE )


/**
 * Check that we can get the default properties as expected.
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
}


/**
 * Test the orientation transform changes.
 */
BOOST_AUTO_TEST_CASE( Orientation )
{
    TRANSFORM t = m_symbol.GetTransform();

    m_symbol.SetOrientation( SYM_ORIENT_90 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( SYM_ORIENT_180 );
    t = m_symbol.GetTransform();
    m_symbol.SetTransform( TRANSFORM() );
    m_symbol.SetOrientation( SYM_ORIENT_270 );
    t = m_symbol.GetTransform();
}


/**
 * Test symbol variant handling.
 */
BOOST_AUTO_TEST_CASE( SchSymbolVariantTest )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_CHECK( symbol );

    // Test for an empty (non-existant) variant.
    wxString                          variantName = wxS( "Variant1" );
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( !variant );

    // Test DNP property variant.
    BOOST_CHECK( !symbol->GetDNP() );
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from BOM property variant.
    BOOST_CHECK( !symbol->GetExcludedFromBOM() );
    symbol->SetExcludedFromBOM( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromBOM( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from simulation property variant.
    BOOST_CHECK( !symbol->GetExcludedFromSim() );
    symbol->SetExcludedFromSim( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromSim( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from board property variant.
    BOOST_CHECK( !symbol->GetExcludedFromBoard() );
    symbol->SetExcludedFromBoard( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromBoard( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from position files property variant.
    BOOST_CHECK( !symbol->GetExcludedFromPosFiles() );
    symbol->SetExcludedFromPosFiles( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromPosFiles( &m_schematic->Hierarchy()[0], variantName ) );

    // Test a value field variant change.
    BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0 ) == wxS( "1K" ) );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "10K" ), &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0, variantName ) == wxS( "10K" ) );
    // BOOST_CHECK( symbol->GetFieldText( FIELD_T::VALUE, &m_schematic->Hierarchy()[0], variantName ) == wxS( "10K" ) );
}


/**
 * Test field addition and retrieval by name.
 * Verifies that GetField(wxString) returns null for non-existent fields and that
 * AddField properly creates new fields (fix for issue #22628).
 */
BOOST_AUTO_TEST_CASE( FieldAdditionByName )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString newFieldName = wxS( "Sim.Library" );

    // GetField by name should return null for non-existent field
    const SCH_FIELD* existing = symbol->GetField( newFieldName );
    BOOST_CHECK( existing == nullptr );

    // Create a field to add
    SCH_FIELD newField( symbol, FIELD_T::USER, newFieldName );
    newField.SetText( wxS( "test_model.lib" ) );

    // AddField should add the field and return a valid pointer
    SCH_FIELD* addedField = symbol->AddField( newField );
    BOOST_REQUIRE( addedField != nullptr );

    // After setting the parent, verify the field is correctly configured
    addedField->SetParent( symbol );
    BOOST_CHECK( addedField->GetParent() == symbol );

    // Now GetField should find the newly added field
    const SCH_FIELD* found = symbol->GetField( newFieldName );
    BOOST_REQUIRE( found != nullptr );
    BOOST_CHECK( found->GetText() == wxS( "test_model.lib" ) );
}


/**
 * Test variant description methods.
 */
BOOST_AUTO_TEST_CASE( VariantDescription )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    wxString variantName = wxS( "TestVariant" );
    wxString description = wxS( "This is a test variant description" );

    // Add a variant
    m_schematic->AddVariant( variantName );
    BOOST_CHECK( m_schematic->GetVariantNames().contains( variantName ) );

    // Set description
    m_schematic->SetVariantDescription( variantName, description );
    BOOST_CHECK_EQUAL( m_schematic->GetVariantDescription( variantName ), description );

    // Empty description for non-existent variant
    BOOST_CHECK( m_schematic->GetVariantDescription( wxS( "NonExistent" ) ).IsEmpty() );

    // Clear description
    m_schematic->SetVariantDescription( variantName, wxEmptyString );
    BOOST_CHECK( m_schematic->GetVariantDescription( variantName ).IsEmpty() );
}


/**
 * Test ${VARIANT} text variable resolution.
 */
BOOST_AUTO_TEST_CASE( TextVariableVARIANT )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    wxString variantName = wxS( "MilitaryGrade" );

    // Add a variant and set it as current
    m_schematic->AddVariant( variantName );
    m_schematic->SetCurrentVariant( variantName );

    // Test that ${VARIANT} resolves to the current variant name
    wxString token = wxS( "VARIANT" );
    bool resolved = m_schematic->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, variantName );

    // Also test VARIANTNAME (legacy/alias)
    token = wxS( "VARIANTNAME" );
    resolved = m_schematic->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, variantName );

    // Empty variant
    m_schematic->SetCurrentVariant( wxEmptyString );
    token = wxS( "VARIANT" );
    resolved = m_schematic->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK( token.IsEmpty() );
}


/**
 * Test ${VARIANT_DESC} text variable resolution.
 */
BOOST_AUTO_TEST_CASE( TextVariableVARIANT_DESC )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    wxString variantName = wxS( "IndustrialVariant" );
    wxString description = wxS( "Industrial temperature range components" );

    // Add a variant with description and set it as current
    m_schematic->AddVariant( variantName );
    m_schematic->SetVariantDescription( variantName, description );
    m_schematic->SetCurrentVariant( variantName );

    // Test that ${VARIANT_DESC} resolves to the variant description
    wxString token = wxS( "VARIANT_DESC" );
    bool resolved = m_schematic->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, description );

    // Test with no description set
    m_schematic->SetVariantDescription( variantName, wxEmptyString );
    token = wxS( "VARIANT_DESC" );
    resolved = m_schematic->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK( token.IsEmpty() );
}


/**
 * Test RenameVariant preserves data.
 */
BOOST_AUTO_TEST_CASE( RenameVariantPreservesData )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString oldName = wxS( "OriginalVariant" );
    wxString newName = wxS( "RenamedVariant" );
    wxString description = wxS( "Original description" );

    // Set up the variant with data
    m_schematic->AddVariant( oldName );
    m_schematic->SetVariantDescription( oldName, description );
    m_schematic->SetCurrentVariant( oldName );

    // Set symbol variant properties
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], oldName );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "100K" ), &m_schematic->Hierarchy()[0], oldName );

    // Verify the data is set
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], oldName ) );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                          false, 0, oldName ), wxS( "100K" ) );

    // Rename the variant
    m_schematic->RenameVariant( oldName, newName );

    // Verify old name is gone
    BOOST_CHECK( !m_schematic->GetVariantNames().contains( oldName ) );

    // Verify new name exists
    BOOST_CHECK( m_schematic->GetVariantNames().contains( newName ) );

    // Verify description was preserved
    BOOST_CHECK_EQUAL( m_schematic->GetVariantDescription( newName ), description );

    // Verify current variant was updated
    BOOST_CHECK_EQUAL( m_schematic->GetCurrentVariant(), newName );

    // Verify symbol variant data was preserved
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], newName ) );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                          false, 0, newName ), wxS( "100K" ) );
}


/**
 * Test CopyVariant creates an independent copy.
 */
BOOST_AUTO_TEST_CASE( CopyVariantCreatesIndependentCopy )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString sourceVariant = wxS( "SourceVariant" );
    wxString copyVariant = wxS( "CopiedVariant" );
    wxString description = wxS( "Source description" );

    // Set up the source variant
    m_schematic->AddVariant( sourceVariant );
    m_schematic->SetVariantDescription( sourceVariant, description );

    // Set symbol variant properties for source
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], sourceVariant );
    symbol->SetExcludedFromBOM( true, &m_schematic->Hierarchy()[0], sourceVariant );
    symbol->SetExcludedFromBoard( true, &m_schematic->Hierarchy()[0], sourceVariant );
    symbol->SetExcludedFromPosFiles( true, &m_schematic->Hierarchy()[0], sourceVariant );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "47K" ), &m_schematic->Hierarchy()[0], sourceVariant );

    // Copy the variant
    m_schematic->CopyVariant( sourceVariant, copyVariant );

    // Verify both variants exist
    BOOST_CHECK( m_schematic->GetVariantNames().contains( sourceVariant ) );
    BOOST_CHECK( m_schematic->GetVariantNames().contains( copyVariant ) );

    // Verify description was copied
    BOOST_CHECK_EQUAL( m_schematic->GetVariantDescription( copyVariant ), description );

    // Verify symbol properties were copied
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], copyVariant ) );
    BOOST_CHECK( symbol->GetExcludedFromBOM( &m_schematic->Hierarchy()[0], copyVariant ) );
    BOOST_CHECK( symbol->GetExcludedFromBoard( &m_schematic->Hierarchy()[0], copyVariant ) );
    BOOST_CHECK( symbol->GetExcludedFromPosFiles( &m_schematic->Hierarchy()[0], copyVariant ) );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                          false, 0, copyVariant ), wxS( "47K" ) );

    // Modify the copy and verify source is unchanged
    symbol->SetDNP( false, &m_schematic->Hierarchy()[0], copyVariant );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "100K" ), &m_schematic->Hierarchy()[0], copyVariant );

    // Source should still have original values
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], sourceVariant ) );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                          false, 0, sourceVariant ), wxS( "47K" ) );

    // Copy should have modified values
    BOOST_CHECK( !symbol->GetDNP( &m_schematic->Hierarchy()[0], copyVariant ) );
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                          false, 0, copyVariant ), wxS( "100K" ) );
}


/**
 * Test variant field value difference detection.
 */
BOOST_AUTO_TEST_CASE( VariantFieldDifferenceDetection )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName1 = wxS( "DiffTestVariant1" );
    wxString variantName2 = wxS( "DiffTestVariant2" );

    // Get the default value
    wxString defaultValue = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                               false, 0 );
    BOOST_CHECK_EQUAL( defaultValue, wxS( "1K" ) );

    // Add a variant and set a different value
    m_schematic->AddVariant( variantName1 );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "2.2K" ), &m_schematic->Hierarchy()[0], variantName1 );

    // Get variant value
    wxString variantValue = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                               false, 0, variantName1 );
    BOOST_CHECK_EQUAL( variantValue, wxS( "2.2K" ) );

    // Verify values differ
    BOOST_CHECK( defaultValue != variantValue );

    // Add another variant with same value as default to verify equality detection
    m_schematic->AddVariant( variantName2 );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "1K" ), &m_schematic->Hierarchy()[0], variantName2 );
    wxString variantValue2 = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                                false, 0, variantName2 );

    // Second variant should have the same value as default
    BOOST_CHECK_EQUAL( defaultValue, variantValue2 );
}


/**
 * Test variant DNP filtering for BOM.
 */
BOOST_AUTO_TEST_CASE( VariantDNPFiltering )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "DNPVariant" );

    // By default, symbol should not be DNP
    BOOST_CHECK( !symbol->GetDNP() );
    BOOST_CHECK( !symbol->GetDNP( &m_schematic->Hierarchy()[0], wxEmptyString ) );

    // Add a variant where symbol is DNP
    m_schematic->AddVariant( variantName );
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], variantName );

    // Default should still not be DNP
    BOOST_CHECK( !symbol->GetDNP() );
    BOOST_CHECK( !symbol->GetDNP( &m_schematic->Hierarchy()[0], wxEmptyString ) );

    // Variant should be DNP
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], variantName ) );

    // Test exclude from BOM as well
    BOOST_CHECK( !symbol->GetExcludedFromBOM() );
    symbol->SetExcludedFromBOM( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromBOM( &m_schematic->Hierarchy()[0], variantName ) );
    BOOST_CHECK( !symbol->GetExcludedFromBOM( &m_schematic->Hierarchy()[0], wxEmptyString ) );

    // Test exclude from board as well
    BOOST_CHECK( !symbol->GetExcludedFromBoard() );
    symbol->SetExcludedFromBoard( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromBoard( &m_schematic->Hierarchy()[0], variantName ) );
    BOOST_CHECK( !symbol->GetExcludedFromBoard( &m_schematic->Hierarchy()[0], wxEmptyString ) );

    // Test exclude from position files as well
    BOOST_CHECK( !symbol->GetExcludedFromPosFiles() );
    symbol->SetExcludedFromPosFiles( true, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( symbol->GetExcludedFromPosFiles( &m_schematic->Hierarchy()[0], variantName ) );
    BOOST_CHECK( !symbol->GetExcludedFromPosFiles( &m_schematic->Hierarchy()[0], wxEmptyString ) );
}


/**
 * Test GetVariantNamesForUI returns properly formatted array.
 */
BOOST_AUTO_TEST_CASE( GetVariantNamesForUI )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    // Add some variants
    m_schematic->AddVariant( wxS( "Zebra" ) );
    m_schematic->AddVariant( wxS( "Alpha" ) );
    m_schematic->AddVariant( wxS( "Beta" ) );

    wxArrayString variantNames = m_schematic->GetVariantNamesForUI();

    // Should have at least 4 items (default + 3 variants)
    BOOST_CHECK( variantNames.GetCount() >= 4 );

    // First item should be the default placeholder
    // The actual string may vary but should represent "default"
    BOOST_CHECK( !variantNames[0].IsEmpty() );

    // Remaining items should be sorted
    // Alpha, Beta, Zebra
    bool foundAlpha = false;
    bool foundBeta = false;
    bool foundZebra = false;

    for( size_t i = 1; i < variantNames.GetCount(); i++ )
    {
        if( variantNames[i] == wxS( "Alpha" ) )
            foundAlpha = true;
        else if( variantNames[i] == wxS( "Beta" ) )
            foundBeta = true;
        else if( variantNames[i] == wxS( "Zebra" ) )
            foundZebra = true;
    }

    BOOST_CHECK( foundAlpha );
    BOOST_CHECK( foundBeta );
    BOOST_CHECK( foundZebra );
}


/**
 * Test that SetValueFieldText correctly persists variant field values.
 * This tests the fix for a bug where variant values were not saved because
 * GetVariant returned a copy instead of modifying the instance directly.
 */
BOOST_AUTO_TEST_CASE( SetValueFieldTextPersistsVariantValue )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "PersistenceTest" );
    wxString newValue = wxS( "4.7K" );
    wxString defaultValue = symbol->GetField( FIELD_T::VALUE )->GetText();

    // Add the variant
    m_schematic->AddVariant( variantName );

    // Set value using SetValueFieldText (the function that had the bug)
    symbol->SetValueFieldText( newValue, &m_schematic->Hierarchy()[0], variantName );

    // Verify that the variant value was actually saved by reading it back
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );
    BOOST_REQUIRE( variant.has_value() );

    wxString fieldName = symbol->GetField( FIELD_T::VALUE )->GetName();
    BOOST_CHECK( variant->m_Fields.contains( fieldName ) );
    BOOST_CHECK_EQUAL( variant->m_Fields.at( fieldName ), newValue );

    // Verify through GetValue method as well
    wxString retrievedValue = symbol->GetValue( false, &m_schematic->Hierarchy()[0], false, variantName );
    BOOST_CHECK_EQUAL( retrievedValue, newValue );

    // Verify default value is unchanged
    wxString retrievedDefault = symbol->GetValue( false, &m_schematic->Hierarchy()[0], false, wxEmptyString );
    BOOST_CHECK_EQUAL( retrievedDefault, defaultValue );
}


/**
 * Test that SetFieldText works consistently with SetValueFieldText for the VALUE field.
 */
BOOST_AUTO_TEST_CASE( SetFieldTextAndSetValueFieldTextConsistency )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName1 = wxS( "MethodTest1" );
    wxString variantName2 = wxS( "MethodTest2" );
    wxString value1 = wxS( "10K" );
    wxString value2 = wxS( "22K" );
    wxString fieldName = symbol->GetField( FIELD_T::VALUE )->GetName();

    m_schematic->AddVariant( variantName1 );
    m_schematic->AddVariant( variantName2 );

    // Set variant 1 using SetValueFieldText
    symbol->SetValueFieldText( value1, &m_schematic->Hierarchy()[0], variantName1 );

    // Set variant 2 using SetFieldText
    symbol->SetFieldText( fieldName, value2, &m_schematic->Hierarchy()[0], variantName2 );

    // Both methods should produce the same result structure
    std::optional<SCH_SYMBOL_VARIANT> variant1 = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName1 );
    std::optional<SCH_SYMBOL_VARIANT> variant2 = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName2 );

    BOOST_REQUIRE( variant1.has_value() );
    BOOST_REQUIRE( variant2.has_value() );

    BOOST_CHECK( variant1->m_Fields.contains( fieldName ) );
    BOOST_CHECK( variant2->m_Fields.contains( fieldName ) );

    BOOST_CHECK_EQUAL( variant1->m_Fields.at( fieldName ), value1 );
    BOOST_CHECK_EQUAL( variant2->m_Fields.at( fieldName ), value2 );

    // Verify through GetValue
    BOOST_CHECK_EQUAL( symbol->GetValue( false, &m_schematic->Hierarchy()[0], false, variantName1 ), value1 );
    BOOST_CHECK_EQUAL( symbol->GetValue( false, &m_schematic->Hierarchy()[0], false, variantName2 ), value2 );

    // Verify through GetFieldText
    BOOST_CHECK_EQUAL( symbol->GetFieldText( fieldName, &m_schematic->Hierarchy()[0], variantName1 ), value1 );
    BOOST_CHECK_EQUAL( symbol->GetFieldText( fieldName, &m_schematic->Hierarchy()[0], variantName2 ), value2 );
}


/**
 * Test variant name handling in eeschema.
 * Note: Unlike PCB, eeschema variant lookups are currently case-sensitive.
 */
BOOST_AUTO_TEST_CASE( VariantNameHandling )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "ProductionVariant" );

    // Add variant with specific casing
    m_schematic->AddVariant( variantName );

    // Set DNP on variant
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], variantName );

    // Verify with exact case - this should work
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], wxS( "ProductionVariant" ) ) );

    // Verify variant exists
    BOOST_CHECK( m_schematic->GetVariantNames().contains( wxS( "ProductionVariant" ) ) );

    // Set and get current variant
    m_schematic->SetCurrentVariant( variantName );
    BOOST_CHECK_EQUAL( m_schematic->GetCurrentVariant(), variantName );

    // Unknown variant should return base DNP (false)
    BOOST_CHECK( !symbol->GetDNP( &m_schematic->Hierarchy()[0], wxS( "NonExistentVariant" ) ) );
}


/**
 * Test variant deletion clears symbol variant data.
 */
BOOST_AUTO_TEST_CASE( VariantDeletionCascade )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "DeleteMe" );

    // Add variant and set properties
    m_schematic->AddVariant( variantName );
    symbol->SetDNP( true, &m_schematic->Hierarchy()[0], variantName );
    symbol->GetField( FIELD_T::VALUE )->SetText( wxS( "DeletedValue" ), &m_schematic->Hierarchy()[0], variantName );

    // Verify data is set
    BOOST_CHECK( symbol->GetDNP( &m_schematic->Hierarchy()[0], variantName ) );
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK( variant.has_value() );

    // Delete the variant
    m_schematic->DeleteVariant( variantName );

    // Variant should no longer exist at schematic level
    BOOST_CHECK( !m_schematic->GetVariantNames().contains( variantName ) );

    // After deletion, querying DNP for non-existent variant should return base value
    BOOST_CHECK( !symbol->GetDNP( &m_schematic->Hierarchy()[0], variantName ) );
}


/**
 * Test variant field values with unicode and special characters.
 */
BOOST_AUTO_TEST_CASE( VariantFieldUnicodeAndSpecialChars )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "UnicodeTest" );
    m_schematic->AddVariant( variantName );

    // Test Unicode characters in field values
    wxString unicodeValue = wxS( "1kΩ ±5% 日本語" );
    symbol->GetField( FIELD_T::VALUE )->SetText( unicodeValue, &m_schematic->Hierarchy()[0], variantName );

    wxString retrieved = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                            false, 0, variantName );
    BOOST_CHECK_EQUAL( retrieved, unicodeValue );

    // Test special characters
    wxString specialChars = wxS( "R<1K>\"test\"'value'" );
    symbol->GetField( FIELD_T::VALUE )->SetText( specialChars, &m_schematic->Hierarchy()[0], variantName );

    retrieved = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0, variantName );
    BOOST_CHECK_EQUAL( retrieved, specialChars );

    // Test empty string
    symbol->GetField( FIELD_T::VALUE )->SetText( wxEmptyString, &m_schematic->Hierarchy()[0], variantName );
    retrieved = symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0],
                                                                   false, 0, variantName );
    BOOST_CHECK( retrieved.IsEmpty() );

    // Test description with unicode
    wxString unicodeDesc = wxS( "Variante für Produktion — 测试" );
    m_schematic->SetVariantDescription( variantName, unicodeDesc );
    BOOST_CHECK_EQUAL( m_schematic->GetVariantDescription( variantName ), unicodeDesc );
}


/**
 * Test variant-specific field dereferencing via ${REF:FIELD:VARIANT} syntax.
 */
BOOST_AUTO_TEST_CASE( VariantSpecificFieldDereferencing )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "MilitaryGrade" );
    wxString variantValue = wxS( "1K-MIL" );
    wxString defaultValue = wxS( "1K" );

    // Create a variant with a different value field
    m_schematic->AddVariant( variantName );
    symbol->GetField( FIELD_T::VALUE )->SetText( variantValue, &m_schematic->Hierarchy()[0], variantName );

    // Verify default value
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetShownText( &m_schematic->Hierarchy()[0], false, 0 ),
                       defaultValue );

    // Get the symbol's reference (R1) for cross-reference testing
    wxString symbolRef = symbol->GetRef( &m_schematic->Hierarchy()[0], false );

    // Test 1: ResolveCrossReference with variant syntax - should return variant-specific value
    wxString token = symbolRef + wxS( ":VALUE:" ) + variantName;
    bool resolved = m_schematic->ResolveCrossReference( &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, variantValue );

    // Test 2: ResolveCrossReference without variant - should return default value
    token = symbolRef + wxS( ":VALUE" );
    resolved = m_schematic->ResolveCrossReference( &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, defaultValue );

    // Test 3: ResolveCrossReference with non-existent variant - should return default value
    token = symbolRef + wxS( ":VALUE:NonExistentVariant" );
    resolved = m_schematic->ResolveCrossReference( &token, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, defaultValue );

    // Test 4: ResolveTextVar with variant parameter directly
    token = wxS( "VALUE" );
    resolved = symbol->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, variantName, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, variantValue );

    // Test 5: ResolveTextVar with empty variant - should return default
    token = wxS( "VALUE" );
    resolved = symbol->ResolveTextVar( &m_schematic->Hierarchy()[0], &token, wxEmptyString, 0 );
    BOOST_CHECK( resolved );
    BOOST_CHECK_EQUAL( token, defaultValue );
}


/**
 * Test footprint field variant support.
 * Verifies that editing the footprint field when a variant is active creates a variant-specific
 * override instead of modifying the base footprint value.
 */
BOOST_AUTO_TEST_CASE( FootprintFieldVariantSupport )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "FootprintVariant" );
    wxString baseFootprint = wxS( "Resistor_SMD:R_0805_2012Metric" );
    wxString variantFootprint = wxS( "Resistor_SMD:R_0402_1005Metric" );
    wxString fieldName = symbol->GetField( FIELD_T::FOOTPRINT )->GetName();

    // Set a base footprint first
    symbol->SetFootprintFieldText( baseFootprint );
    BOOST_CHECK_EQUAL( symbol->GetFootprintFieldText( false, nullptr, false ), baseFootprint );

    // Add the variant
    m_schematic->AddVariant( variantName );

    // Set a different footprint for the variant using SetFieldText
    symbol->SetFieldText( fieldName, variantFootprint, &m_schematic->Hierarchy()[0], variantName );

    // Verify base footprint is unchanged
    wxString retrievedBase = symbol->GetFootprintFieldText( false, nullptr, false );
    BOOST_CHECK_EQUAL( retrievedBase, baseFootprint );

    // Verify GetFieldText with no variant returns base footprint
    wxString retrievedDefault = symbol->GetFieldText( fieldName, &m_schematic->Hierarchy()[0], wxEmptyString );
    BOOST_CHECK_EQUAL( retrievedDefault, baseFootprint );

    // Verify GetFieldText with variant returns variant-specific footprint
    wxString retrievedVariant = symbol->GetFieldText( fieldName, &m_schematic->Hierarchy()[0], variantName );
    BOOST_CHECK_EQUAL( retrievedVariant, variantFootprint );

    // Verify that the variant data structure contains the footprint override
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );
    BOOST_REQUIRE( variant.has_value() );
    BOOST_CHECK( variant->m_Fields.contains( fieldName ) );
    BOOST_CHECK_EQUAL( variant->m_Fields.at( fieldName ), variantFootprint );
}


/**
 * Test that setting footprint to same value as base when variant is active does not create an
 * unnecessary override entry.
 */
BOOST_AUTO_TEST_CASE( FootprintFieldVariantNoOpWhenSame )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( wxS( "variant_test" ) );
    fn.SetName( wxS( "variant_test" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );
    BOOST_REQUIRE( m_schematic );

    SCH_SYMBOL* symbol = GetFirstSymbol();
    BOOST_REQUIRE( symbol );

    wxString variantName = wxS( "NoOpTest" );
    wxString baseFootprint = wxS( "Resistor_SMD:R_0805_2012Metric" );
    wxString fieldName = symbol->GetField( FIELD_T::FOOTPRINT )->GetName();

    // Set a base footprint first
    symbol->SetFootprintFieldText( baseFootprint );

    // Add the variant
    m_schematic->AddVariant( variantName );

    // Set the same footprint value for the variant
    symbol->SetFieldText( fieldName, baseFootprint, &m_schematic->Hierarchy()[0], variantName );

    // Verify that no variant override was created since the value is the same
    std::optional<SCH_SYMBOL_VARIANT> variant = symbol->GetVariant( m_schematic->Hierarchy()[0], variantName );

    // Either no variant was created, or it exists but doesn't have a footprint override
    if( variant.has_value() )
    {
        BOOST_CHECK( !variant->m_Fields.contains( fieldName ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
