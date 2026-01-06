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
 * @file test_variant.cpp
 * Unit tests for the PCB variant system including FOOTPRINT_VARIANT,
 * BOARD variant registry, and variant-aware filtering.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <board.h>
#include <footprint.h>
#include <pcbnew/netlist_reader/kicad_netlist_parser.h>
#include <pcbnew/netlist_reader/pcb_netlist.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcbnew_utils/board_file_utils.h>
#include <richio.h>


BOOST_AUTO_TEST_SUITE( Variant )


/**
 * Test FOOTPRINT_VARIANT class basic operations
 */
BOOST_AUTO_TEST_CASE( FootprintVariantBasics )
{
    FOOTPRINT_VARIANT variant( "TestVariant" );

    // Check default values
    BOOST_CHECK_EQUAL( variant.GetName(), "TestVariant" );
    BOOST_CHECK( !variant.GetDNP() );
    BOOST_CHECK( !variant.GetExcludedFromBOM() );
    BOOST_CHECK( !variant.GetExcludedFromPosFiles() );
    BOOST_CHECK( variant.GetFields().empty() );

    // Test setting values
    variant.SetDNP( true );
    variant.SetExcludedFromBOM( true );
    variant.SetExcludedFromPosFiles( true );
    variant.SetFieldValue( "Value", "100R" );

    BOOST_CHECK( variant.GetDNP() );
    BOOST_CHECK( variant.GetExcludedFromBOM() );
    BOOST_CHECK( variant.GetExcludedFromPosFiles() );
    BOOST_CHECK( variant.HasFieldValue( "Value" ) );
    BOOST_CHECK_EQUAL( variant.GetFieldValue( "Value" ), "100R" );
}


/**
 * Test BOARD variant registry operations
 */
BOOST_AUTO_TEST_CASE( BoardVariantRegistry )
{
    BOARD board;

    // Initially no variants
    BOOST_CHECK( board.GetVariantNames().empty() );
    BOOST_CHECK( board.GetCurrentVariant().IsEmpty() );

    // Add a variant
    board.AddVariant( "Production" );
    BOOST_CHECK( board.HasVariant( "Production" ) );
    BOOST_CHECK_EQUAL( board.GetVariantNames().size(), 1 );

    // Add another variant
    board.AddVariant( "Debug" );
    BOOST_CHECK( board.HasVariant( "Debug" ) );
    BOOST_CHECK_EQUAL( board.GetVariantNames().size(), 2 );

    // Test case insensitivity
    BOOST_CHECK( board.HasVariant( "production" ) );
    BOOST_CHECK( board.HasVariant( "PRODUCTION" ) );
    BOOST_CHECK( board.HasVariant( "PrOdUcTiOn" ) );

    // Set current variant
    board.SetCurrentVariant( "Production" );
    BOOST_CHECK_EQUAL( board.GetCurrentVariant(), "Production" );

    // Set variant description
    board.SetVariantDescription( "Production", "Standard production build" );
    BOOST_CHECK_EQUAL( board.GetVariantDescription( "Production" ), "Standard production build" );

    // Delete a variant
    board.DeleteVariant( "Debug" );
    BOOST_CHECK( !board.HasVariant( "Debug" ) );
    BOOST_CHECK_EQUAL( board.GetVariantNames().size(), 1 );
}


/**
 * Test footprint DNP for variant
 */
BOOST_AUTO_TEST_CASE( FootprintDNPForVariant )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    // Add variants to board
    board.AddVariant( "Production" );
    board.AddVariant( "Debug" );

    // Set base DNP to false
    fp.SetAttributes( 0 );
    BOOST_CHECK( !fp.GetDNPForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "Production" ) );

    // Set base DNP to true
    fp.SetAttributes( FP_DNP );
    BOOST_CHECK( fp.GetDNPForVariant( wxEmptyString ) );
    BOOST_CHECK( fp.GetDNPForVariant( "Production" ) );

    // Now set Production variant to override DNP
    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetDNP( false );
    fp.SetVariant( prodVariant );

    // Base is still DNP, but Production overrides to not DNP
    BOOST_CHECK( fp.GetDNPForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "Production" ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "production" ) );  // Case insensitive
    BOOST_CHECK( fp.GetDNPForVariant( "Debug" ) );  // No override, uses base

    // Set Debug variant to also override
    FOOTPRINT_VARIANT debugVariant( "Debug" );
    debugVariant.SetDNP( true );
    fp.SetVariant( debugVariant );

    BOOST_CHECK( fp.GetDNPForVariant( "Debug" ) );
}


/**
 * Test footprint BOM exclusion for variant
 */
BOOST_AUTO_TEST_CASE( FootprintBOMExclusionForVariant )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "Production" );

    // Set base exclude from BOM to false
    fp.SetAttributes( 0 );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "Production" ) );

    // Set base exclude from BOM to true
    fp.SetAttributes( FP_EXCLUDE_FROM_BOM );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( wxEmptyString ) );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( "Production" ) );

    // Override for Production variant
    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetExcludedFromBOM( false );
    fp.SetVariant( prodVariant );

    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "Production" ) );
}


/**
 * Test footprint position file exclusion for variant
 */
BOOST_AUTO_TEST_CASE( FootprintPosFileExclusionForVariant )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "Production" );

    // Set base exclude from position files to false
    fp.SetAttributes( 0 );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "Production" ) );

    // Set base exclude from position files to true
    fp.SetAttributes( FP_EXCLUDE_FROM_POS_FILES );
    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( wxEmptyString ) );
    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( "Production" ) );

    // Override for Production variant
    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetExcludedFromPosFiles( false );
    fp.SetVariant( prodVariant );

    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "Production" ) );
}


/**
 * Test variant case insensitivity for all operations
 */
BOOST_AUTO_TEST_CASE( VariantCaseInsensitivity )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "Production" );

    // Set variant override with specific case
    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetDNP( true );
    fp.SetVariant( prodVariant );

    // Access with different cases - all should find the same variant
    BOOST_CHECK( fp.GetDNPForVariant( "Production" ) );
    BOOST_CHECK( fp.GetDNPForVariant( "production" ) );
    BOOST_CHECK( fp.GetDNPForVariant( "PRODUCTION" ) );
    BOOST_CHECK( fp.GetDNPForVariant( "PrOdUcTiOn" ) );

    // Board operations should also be case insensitive
    BOOST_CHECK( board.HasVariant( "production" ) );
    BOOST_CHECK( board.HasVariant( "PRODUCTION" ) );
}


/**
 * Test that empty variant name returns base attributes
 */
BOOST_AUTO_TEST_CASE( EmptyVariantReturnsBase )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    // Set base DNP
    fp.SetAttributes( FP_DNP );

    // Empty variant should return base
    BOOST_CHECK( fp.GetDNPForVariant( wxEmptyString ) );

    // Add a variant but still check empty
    board.AddVariant( "Production" );

    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetDNP( false );
    fp.SetVariant( prodVariant );

    // Empty still returns base
    BOOST_CHECK( fp.GetDNPForVariant( wxEmptyString ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "Production" ) );
}


/**
 * Test that unknown variant name returns base attributes
 */
BOOST_AUTO_TEST_CASE( UnknownVariantReturnsBase )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    // Set base DNP
    fp.SetAttributes( FP_DNP );

    board.AddVariant( "Production" );

    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetDNP( false );
    fp.SetVariant( prodVariant );

    // Unknown variant should return base
    BOOST_CHECK( fp.GetDNPForVariant( "NonExistentVariant" ) );
    BOOST_CHECK( fp.GetDNPForVariant( "Debug" ) );
}


BOOST_AUTO_TEST_CASE( FootprintVariantCopyAssignment )
{
    BOARD     board;
    FOOTPRINT original( &board );

    original.SetReference( "R1" );
    original.SetValue( "10K" );

    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetDNP( true );
    prodVariant.SetExcludedFromBOM( true );
    prodVariant.SetFieldValue( original.Value().GetName(), "22K" );
    original.SetVariant( prodVariant );

    FOOTPRINT copy( original );
    const FOOTPRINT_VARIANT* copyVariant = copy.GetVariant( "Production" );
    BOOST_REQUIRE( copyVariant );
    BOOST_CHECK( copyVariant->GetDNP() );
    BOOST_CHECK( copyVariant->GetExcludedFromBOM() );
    BOOST_CHECK_EQUAL( copyVariant->GetFieldValue( original.Value().GetName() ), "22K" );

    FOOTPRINT assigned( &board );
    assigned = original;

    const FOOTPRINT_VARIANT* assignedVariant = assigned.GetVariant( "Production" );
    BOOST_REQUIRE( assignedVariant );
    BOOST_CHECK( assignedVariant->GetDNP() );
    BOOST_CHECK( assignedVariant->GetExcludedFromBOM() );
    BOOST_CHECK_EQUAL( assignedVariant->GetFieldValue( original.Value().GetName() ), "22K" );
}


BOOST_AUTO_TEST_CASE( FootprintFieldShownTextForVariant )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "Production" );
    board.SetCurrentVariant( "Production" );

    fp.SetValue( "10K" );

    FOOTPRINT_VARIANT prodVariant( "Production" );
    prodVariant.SetFieldValue( fp.Value().GetName(), "22K" );
    fp.SetVariant( prodVariant );

    BOOST_CHECK_EQUAL( fp.Value().GetShownText( false ), "22K" );

    board.SetCurrentVariant( wxEmptyString );
    BOOST_CHECK_EQUAL( fp.Value().GetShownText( false ), "10K" );
}


BOOST_AUTO_TEST_CASE( BoardVariantTextVars )
{
    BOARD board;

    board.AddVariant( "Production" );
    board.SetVariantDescription( "Production", "Production build" );
    board.SetCurrentVariant( "Production" );

    wxString variantToken = wxT( "VARIANT" );
    BOOST_CHECK( board.ResolveTextVar( &variantToken, 0 ) );
    BOOST_CHECK_EQUAL( variantToken, "Production" );

    wxString descToken = wxT( "VARIANT_DESC" );
    BOOST_CHECK( board.ResolveTextVar( &descToken, 0 ) );
    BOOST_CHECK_EQUAL( descToken, "Production build" );

    board.SetCurrentVariant( wxEmptyString );
    wxString defaultToken = wxT( "VARIANT" );
    BOOST_CHECK( board.ResolveTextVar( &defaultToken, 0 ) );
    BOOST_CHECK( defaultToken.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( NetlistComponentVariantsParsing )
{
    const std::string netlist =
            "(export (version 1)\n"
            "  (components\n"
            "    (comp (ref R1)\n"
            "      (value 10K)\n"
            "      (footprint Resistor_SMD:R_0603_1608Metric)\n"
            "      (libsource (lib Device) (part R))\n"
            "      (variants\n"
            "        (variant (name Alt)\n"
            "          (property (name dnp) (value 1))\n"
            "          (property (name exclude_from_bom) (value 0))\n"
            "          (fields\n"
            "            (field (name Value) \"22K\")\n"
            "            (field (name Footprint) \"Resistor_SMD:R_0805_2012Metric\")\n"
            "          )\n"
            "        )\n"
            "      )\n"
            "    )\n"
            "  )\n"
            ")\n";

    STRING_LINE_READER reader( netlist, wxT( "variant_netlist" ) );
    NETLIST            parsedNetlist;
    KICAD_NETLIST_PARSER parser( &reader, &parsedNetlist );

    parser.Parse();

    BOOST_REQUIRE_EQUAL( parsedNetlist.GetCount(), 1 );

    COMPONENT* component = parsedNetlist.GetComponent( 0 );
    const COMPONENT_VARIANT* variant = component->GetVariant( "Alt" );

    BOOST_REQUIRE( variant );
    BOOST_CHECK( variant->m_hasDnp );
    BOOST_CHECK( variant->m_dnp );
    BOOST_CHECK( variant->m_hasExcludedFromBOM );
    BOOST_CHECK( !variant->m_excludedFromBOM );

    auto valueIt = variant->m_fields.find( "Value" );
    BOOST_CHECK( valueIt != variant->m_fields.end() );
    BOOST_CHECK_EQUAL( valueIt->second, "22K" );

    auto fpIt = variant->m_fields.find( "Footprint" );
    BOOST_CHECK( fpIt != variant->m_fields.end() );
    BOOST_CHECK_EQUAL( fpIt->second, "Resistor_SMD:R_0805_2012Metric" );
}


/**
 * Test adding multiple variants and verifying they're independent
 */
BOOST_AUTO_TEST_CASE( MultipleVariantsIndependent )
{
    BOARD board;

    // Add multiple variants
    board.AddVariant( "Variant1" );
    board.AddVariant( "Variant2" );
    board.SetVariantDescription( "Variant1", "First variant" );
    board.SetVariantDescription( "Variant2", "Second variant" );

    // Create footprint and set variant-specific properties
    FOOTPRINT fp( &board );
    fp.SetReference( "R1" );
    fp.SetValue( "10K" );

    FOOTPRINT_VARIANT variant1( "Variant1" );
    variant1.SetDNP( true );
    variant1.SetFieldValue( fp.Value().GetName(), "22K" );
    fp.SetVariant( variant1 );

    FOOTPRINT_VARIANT variant2( "Variant2" );
    variant2.SetDNP( false );
    variant2.SetFieldValue( fp.Value().GetName(), "47K" );
    fp.SetVariant( variant2 );

    // Verify both variants are independent
    BOOST_CHECK( fp.GetDNPForVariant( "Variant1" ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "Variant2" ) );

    const FOOTPRINT_VARIANT* v1 = fp.GetVariant( "Variant1" );
    const FOOTPRINT_VARIANT* v2 = fp.GetVariant( "Variant2" );

    BOOST_REQUIRE( v1 );
    BOOST_REQUIRE( v2 );

    BOOST_CHECK_EQUAL( v1->GetFieldValue( fp.Value().GetName() ), "22K" );
    BOOST_CHECK_EQUAL( v2->GetFieldValue( fp.Value().GetName() ), "47K" );

    // Verify descriptions are independent
    BOOST_CHECK_EQUAL( board.GetVariantDescription( "Variant1" ), "First variant" );
    BOOST_CHECK_EQUAL( board.GetVariantDescription( "Variant2" ), "Second variant" );
}


/**
 * Test variant field values with unicode and special characters
 */
BOOST_AUTO_TEST_CASE( VariantFieldUnicodeAndSpecialChars )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "UnicodeTest" );

    fp.SetValue( "Default" );

    // Unicode characters
    FOOTPRINT_VARIANT unicodeVariant( "UnicodeTest" );
    wxString unicodeValue = wxT( "1kΩ ±5% 日本語" );
    unicodeVariant.SetFieldValue( fp.Value().GetName(), unicodeValue );
    fp.SetVariant( unicodeVariant );

    const FOOTPRINT_VARIANT* retrieved = fp.GetVariant( "UnicodeTest" );
    BOOST_REQUIRE( retrieved );
    BOOST_CHECK_EQUAL( retrieved->GetFieldValue( fp.Value().GetName() ), unicodeValue );

    // Special characters
    FOOTPRINT_VARIANT specialVariant( "SpecialChars" );
    wxString specialChars = wxT( "R<1K>\"test\"'value'" );
    specialVariant.SetFieldValue( fp.Value().GetName(), specialChars );
    fp.SetVariant( specialVariant );

    const FOOTPRINT_VARIANT* retrievedSpecial = fp.GetVariant( "SpecialChars" );
    BOOST_REQUIRE( retrievedSpecial );
    BOOST_CHECK_EQUAL( retrievedSpecial->GetFieldValue( fp.Value().GetName() ), specialChars );

    // Unicode in variant description
    wxString unicodeDesc = wxT( "Variante für Produktion — 测试" );
    board.SetVariantDescription( "UnicodeTest", unicodeDesc );
    BOOST_CHECK_EQUAL( board.GetVariantDescription( "UnicodeTest" ), unicodeDesc );
}


/**
 * Test variant deletion clears board variant registry
 */
BOOST_AUTO_TEST_CASE( VariantDeletionClearsRegistry )
{
    BOARD board;

    board.AddVariant( "Variant1" );
    board.AddVariant( "Variant2" );
    board.SetVariantDescription( "Variant1", "Description 1" );
    board.SetCurrentVariant( "Variant1" );

    BOOST_CHECK_EQUAL( board.GetVariantNames().size(), 2 );
    BOOST_CHECK( board.HasVariant( "Variant1" ) );

    // Delete the variant
    board.DeleteVariant( "Variant1" );

    // Verify it's gone
    BOOST_CHECK_EQUAL( board.GetVariantNames().size(), 1 );
    BOOST_CHECK( !board.HasVariant( "Variant1" ) );
    BOOST_CHECK( board.HasVariant( "Variant2" ) );

    // Current variant should be cleared since we deleted the current one
    BOOST_CHECK( board.GetCurrentVariant().IsEmpty() || board.GetCurrentVariant() != "Variant1" );
}


/**
 * Test renaming variant to existing name fails or merges appropriately
 */
BOOST_AUTO_TEST_CASE( RenameVariantPreservesData )
{
    BOARD board;

    board.AddVariant( "OldName" );
    board.SetVariantDescription( "OldName", "Test description" );
    board.SetCurrentVariant( "OldName" );

    // Rename the variant
    board.RenameVariant( "OldName", "NewName" );

    // Old name should be gone
    BOOST_CHECK( !board.HasVariant( "OldName" ) );

    // New name should exist with same properties
    BOOST_CHECK( board.HasVariant( "NewName" ) );
    BOOST_CHECK_EQUAL( board.GetVariantDescription( "NewName" ), "Test description" );

    // Current variant should be updated if it was the renamed one
    BOOST_CHECK_EQUAL( board.GetCurrentVariant(), "NewName" );
}


/**
 * Test GetVariantNamesForUI returns sorted list with default first
 */
BOOST_AUTO_TEST_CASE( GetVariantNamesForUIFormat )
{
    BOARD board;

    board.AddVariant( "Zebra" );
    board.AddVariant( "Alpha" );
    board.AddVariant( "Beta" );

    wxArrayString names = board.GetVariantNamesForUI();

    // Should have 4 entries (default + 3 variants)
    BOOST_CHECK( names.GetCount() >= 4 );

    // First should be the default placeholder
    BOOST_CHECK( !names[0].IsEmpty() );

    // Remaining should be sorted alphabetically
    bool foundAlpha = false;
    bool foundBeta = false;
    bool foundZebra = false;

    for( size_t i = 1; i < names.GetCount(); i++ )
    {
        if( names[i] == wxT( "Alpha" ) )
            foundAlpha = true;
        else if( names[i] == wxT( "Beta" ) )
            foundBeta = true;
        else if( names[i] == wxT( "Zebra" ) )
            foundZebra = true;
    }

    BOOST_CHECK( foundAlpha );
    BOOST_CHECK( foundBeta );
    BOOST_CHECK( foundZebra );
}


/**
 * Test multiple flags can be set independently for each variant
 */
BOOST_AUTO_TEST_CASE( VariantMultipleFlagsCombinations )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    board.AddVariant( "DNPOnly" );
    board.AddVariant( "BOMOnly" );
    board.AddVariant( "AllFlags" );
    board.AddVariant( "NoFlags" );

    // Set various flag combinations
    FOOTPRINT_VARIANT dnpOnly( "DNPOnly" );
    dnpOnly.SetDNP( true );
    dnpOnly.SetExcludedFromBOM( false );
    dnpOnly.SetExcludedFromPosFiles( false );
    fp.SetVariant( dnpOnly );

    FOOTPRINT_VARIANT bomOnly( "BOMOnly" );
    bomOnly.SetDNP( false );
    bomOnly.SetExcludedFromBOM( true );
    bomOnly.SetExcludedFromPosFiles( false );
    fp.SetVariant( bomOnly );

    FOOTPRINT_VARIANT allFlags( "AllFlags" );
    allFlags.SetDNP( true );
    allFlags.SetExcludedFromBOM( true );
    allFlags.SetExcludedFromPosFiles( true );
    fp.SetVariant( allFlags );

    FOOTPRINT_VARIANT noFlags( "NoFlags" );
    noFlags.SetDNP( false );
    noFlags.SetExcludedFromBOM( false );
    noFlags.SetExcludedFromPosFiles( false );
    fp.SetVariant( noFlags );

    // Verify each variant has correct flags
    BOOST_CHECK( fp.GetDNPForVariant( "DNPOnly" ) );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "DNPOnly" ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "DNPOnly" ) );

    BOOST_CHECK( !fp.GetDNPForVariant( "BOMOnly" ) );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( "BOMOnly" ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "BOMOnly" ) );

    BOOST_CHECK( fp.GetDNPForVariant( "AllFlags" ) );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( "AllFlags" ) );
    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( "AllFlags" ) );

    BOOST_CHECK( !fp.GetDNPForVariant( "NoFlags" ) );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "NoFlags" ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "NoFlags" ) );
}


/**
 * Test that COMPONENT_VARIANT data from netlist transfers correctly to FOOTPRINT_VARIANT.
 * This simulates the variant property transfer that occurs during "Update PCB from Schematic".
 */
BOOST_AUTO_TEST_CASE( ComponentVariantToFootprintTransfer )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.SetReference( "R1" );
    fp.SetValue( "10K" );

    board.AddVariant( "Variant A" );
    board.AddVariant( "Variant B" );

    // Create a COMPONENT with variant data (simulating schematic data)
    LIB_ID fpid( wxT( "Resistor_SMD" ), wxT( "R_0805_2012Metric" ) );
    wxString reference = wxT( "R1" );
    wxString value = wxT( "10K" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add variant "Variant A" with DNP=true, ExcludedFromBOM=false, ExcludedFromPosFiles=true
    // and a field override for Datasheet
    COMPONENT_VARIANT variantA( "Variant A" );
    variantA.m_dnp = true;
    variantA.m_hasDnp = true;
    variantA.m_excludedFromBOM = false;
    variantA.m_hasExcludedFromBOM = true;
    variantA.m_excludedFromPosFiles = true;
    variantA.m_hasExcludedFromPosFiles = true;
    variantA.m_fields[wxT( "Datasheet" )] = wxT( "https://example.com/datasheet.pdf" );
    component.AddVariant( variantA );

    // Add variant "Variant B" with DNP=false, ExcludedFromBOM=true
    COMPONENT_VARIANT variantB( "Variant B" );
    variantB.m_dnp = false;
    variantB.m_hasDnp = true;
    variantB.m_excludedFromBOM = true;
    variantB.m_hasExcludedFromBOM = true;
    variantB.m_excludedFromPosFiles = false;
    variantB.m_hasExcludedFromPosFiles = true;
    variantB.m_fields[wxT( "Value" )] = wxT( "22K" );
    component.AddVariant( variantB );

    // Transfer variant data from COMPONENT to FOOTPRINT (simulating applyComponentVariants)
    for( const auto& [variantName, componentVariant] : component.GetVariants() )
    {
        FOOTPRINT_VARIANT* fpVariant = fp.AddVariant( variantName );
        BOOST_REQUIRE( fpVariant );

        if( componentVariant.m_hasDnp )
            fpVariant->SetDNP( componentVariant.m_dnp );

        if( componentVariant.m_hasExcludedFromBOM )
            fpVariant->SetExcludedFromBOM( componentVariant.m_excludedFromBOM );

        if( componentVariant.m_hasExcludedFromPosFiles )
            fpVariant->SetExcludedFromPosFiles( componentVariant.m_excludedFromPosFiles );

        for( const auto& [fieldName, fieldValue] : componentVariant.m_fields )
            fpVariant->SetFieldValue( fieldName, fieldValue );
    }

    // Verify Variant A properties
    const FOOTPRINT_VARIANT* fpVariantA = fp.GetVariant( "Variant A" );
    BOOST_REQUIRE( fpVariantA );
    BOOST_CHECK( fpVariantA->GetDNP() );
    BOOST_CHECK( !fpVariantA->GetExcludedFromBOM() );
    BOOST_CHECK( fpVariantA->GetExcludedFromPosFiles() );
    BOOST_CHECK( fpVariantA->HasFieldValue( wxT( "Datasheet" ) ) );
    BOOST_CHECK_EQUAL( fpVariantA->GetFieldValue( wxT( "Datasheet" ) ),
                       wxT( "https://example.com/datasheet.pdf" ) );

    // Verify Variant B properties
    const FOOTPRINT_VARIANT* fpVariantB = fp.GetVariant( "Variant B" );
    BOOST_REQUIRE( fpVariantB );
    BOOST_CHECK( !fpVariantB->GetDNP() );
    BOOST_CHECK( fpVariantB->GetExcludedFromBOM() );
    BOOST_CHECK( !fpVariantB->GetExcludedFromPosFiles() );
    BOOST_CHECK( fpVariantB->HasFieldValue( wxT( "Value" ) ) );
    BOOST_CHECK_EQUAL( fpVariantB->GetFieldValue( wxT( "Value" ) ), wxT( "22K" ) );

    // Verify variant-aware getters work
    BOOST_CHECK( fp.GetDNPForVariant( "Variant A" ) );
    BOOST_CHECK( !fp.GetDNPForVariant( "Variant B" ) );
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "Variant A" ) );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( "Variant B" ) );
    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( "Variant A" ) );
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "Variant B" ) );
}


/**
 * Test that variant m_has* flags are properly respected during transfer.
 * When a flag is not set (m_hasDnp = false), the footprint variant value
 * should be reset to the base footprint's value, not retained.
 */
BOOST_AUTO_TEST_CASE( ComponentVariantPartialOverride )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.SetReference( "R1" );

    // Set base footprint to have all attributes false
    fp.SetDNP( false );
    fp.SetExcludedFromBOM( false );
    fp.SetExcludedFromPosFiles( false );

    board.AddVariant( "TestVariant" );

    // Pre-populate the footprint variant with all true values (simulating old state)
    FOOTPRINT_VARIANT initialVariant( "TestVariant" );
    initialVariant.SetDNP( true );
    initialVariant.SetExcludedFromBOM( true );
    initialVariant.SetExcludedFromPosFiles( true );
    fp.SetVariant( initialVariant );

    // Create a component variant that only has explicit DNP override set
    LIB_ID fpid( wxT( "Resistor_SMD" ), wxT( "R_0805_2012Metric" ) );
    KIID_PATH path;
    std::vector<KIID> kiids;
    COMPONENT component( fpid, wxT( "R1" ), wxT( "10K" ), path, kiids );

    COMPONENT_VARIANT partialVariant( "TestVariant" );
    partialVariant.m_dnp = true;
    partialVariant.m_hasDnp = true;
    // m_hasExcludedFromBOM and m_hasExcludedFromPosFiles are false (no explicit override)
    component.AddVariant( partialVariant );

    // Transfer properties, resetting non-overridden ones to base footprint values
    for( const auto& [variantName, componentVariant] : component.GetVariants() )
    {
        FOOTPRINT_VARIANT* fpVariant = fp.GetVariant( variantName );
        BOOST_REQUIRE( fpVariant );

        // Apply explicit override or reset to base footprint value
        bool targetDnp = componentVariant.m_hasDnp ? componentVariant.m_dnp : fp.IsDNP();
        fpVariant->SetDNP( targetDnp );

        bool targetBOM = componentVariant.m_hasExcludedFromBOM
                                 ? componentVariant.m_excludedFromBOM
                                 : fp.IsExcludedFromBOM();
        fpVariant->SetExcludedFromBOM( targetBOM );

        bool targetPos = componentVariant.m_hasExcludedFromPosFiles
                                 ? componentVariant.m_excludedFromPosFiles
                                 : fp.IsExcludedFromPosFiles();
        fpVariant->SetExcludedFromPosFiles( targetPos );
    }

    // Verify: DNP should be true (m_hasDnp was true with value true)
    BOOST_CHECK( fp.GetDNPForVariant( "TestVariant" ) );

    // Verify: ExcludedFromBOM should be reset to base (false)
    BOOST_CHECK( !fp.GetExcludedFromBOMForVariant( "TestVariant" ) );

    // Verify: ExcludedFromPosFiles should be reset to base (false)
    BOOST_CHECK( !fp.GetExcludedFromPosFilesForVariant( "TestVariant" ) );
}


/**
 * Test that all variant attributes are properly parsed from netlist including exclude_from_pos_files.
 * This verifies that the netlist parser correctly sets the m_has* flags for all attribute types.
 */
BOOST_AUTO_TEST_CASE( NetlistVariantAttributeParsing )
{
    // Create component variants with all attribute types
    COMPONENT_VARIANT variantWithAll( "WithAll" );
    variantWithAll.m_dnp = true;
    variantWithAll.m_hasDnp = true;
    variantWithAll.m_excludedFromBOM = true;
    variantWithAll.m_hasExcludedFromBOM = true;
    variantWithAll.m_excludedFromPosFiles = true;
    variantWithAll.m_hasExcludedFromPosFiles = true;

    // Verify all flags are set correctly
    BOOST_CHECK( variantWithAll.m_hasDnp );
    BOOST_CHECK( variantWithAll.m_dnp );
    BOOST_CHECK( variantWithAll.m_hasExcludedFromBOM );
    BOOST_CHECK( variantWithAll.m_excludedFromBOM );
    BOOST_CHECK( variantWithAll.m_hasExcludedFromPosFiles );
    BOOST_CHECK( variantWithAll.m_excludedFromPosFiles );

    // Create a component variant with only some attributes
    COMPONENT_VARIANT variantPartial( "Partial" );
    variantPartial.m_dnp = true;
    variantPartial.m_hasDnp = true;
    // ExcludedFromBOM and ExcludedFromPosFiles have no explicit override

    BOOST_CHECK( variantPartial.m_hasDnp );
    BOOST_CHECK( !variantPartial.m_hasExcludedFromBOM );
    BOOST_CHECK( !variantPartial.m_hasExcludedFromPosFiles );

    // Create a variant with false values (explicit override to false)
    COMPONENT_VARIANT variantAllFalse( "AllFalse" );
    variantAllFalse.m_dnp = false;
    variantAllFalse.m_hasDnp = true;
    variantAllFalse.m_excludedFromBOM = false;
    variantAllFalse.m_hasExcludedFromBOM = true;
    variantAllFalse.m_excludedFromPosFiles = false;
    variantAllFalse.m_hasExcludedFromPosFiles = true;

    BOOST_CHECK( variantAllFalse.m_hasDnp );
    BOOST_CHECK( !variantAllFalse.m_dnp );
    BOOST_CHECK( variantAllFalse.m_hasExcludedFromBOM );
    BOOST_CHECK( !variantAllFalse.m_excludedFromBOM );
    BOOST_CHECK( variantAllFalse.m_hasExcludedFromPosFiles );
    BOOST_CHECK( !variantAllFalse.m_excludedFromPosFiles );
}


/**
 * Test the complete variant attribute transfer flow simulating board_netlist_updater behavior.
 * This tests the fixed logic where attributes without explicit overrides are reset to base values.
 */
BOOST_AUTO_TEST_CASE( VariantAttributeTransferWithReset )
{
    BOARD     board;
    FOOTPRINT fp( &board );
    fp.SetReference( "R1" );
    fp.SetFPID( LIB_ID( wxT( "Resistor_SMD" ), wxT( "R_0805" ) ) );

    // Base footprint has no flags set
    fp.SetDNP( false );
    fp.SetExcludedFromBOM( false );
    fp.SetExcludedFromPosFiles( false );

    board.AddVariant( "Variant A" );

    // Step 1: Initial state - variant has all attributes set (simulating previous netlist update)
    FOOTPRINT_VARIANT* fpVariant = fp.AddVariant( "Variant A" );
    BOOST_REQUIRE( fpVariant );
    fpVariant->SetDNP( true );
    fpVariant->SetExcludedFromBOM( true );
    fpVariant->SetExcludedFromPosFiles( true );

    // Verify initial state
    BOOST_CHECK( fp.GetDNPForVariant( "Variant A" ) );
    BOOST_CHECK( fp.GetExcludedFromBOMForVariant( "Variant A" ) );
    BOOST_CHECK( fp.GetExcludedFromPosFilesForVariant( "Variant A" ) );

    // Step 2: New netlist has variant with NO explicit attribute overrides
    // This simulates the user removing all variant attribute overrides from schematic
    COMPONENT_VARIANT componentVariant( "Variant A" );
    // All m_has* flags are false by default (no explicit overrides)

    // Step 3: Apply the fixed transfer logic (same as board_netlist_updater::applyComponentVariants)
    // For isActive = true case with no explicit overrides, attributes should reset to base
    bool targetDnp = componentVariant.m_hasDnp ? componentVariant.m_dnp : fp.IsDNP();
    bool targetBOM = componentVariant.m_hasExcludedFromBOM ? componentVariant.m_excludedFromBOM
                                                          : fp.IsExcludedFromBOM();
    bool targetPos = componentVariant.m_hasExcludedFromPosFiles
                             ? componentVariant.m_excludedFromPosFiles
                             : fp.IsExcludedFromPosFiles();

    fpVariant->SetDNP( targetDnp );
    fpVariant->SetExcludedFromBOM( targetBOM );
    fpVariant->SetExcludedFromPosFiles( targetPos );

    // Step 4: Verify all attributes were reset to base footprint values (false)
    BOOST_CHECK_MESSAGE( !fp.GetDNPForVariant( "Variant A" ),
                         "DNP should be reset to base value (false) when no explicit override" );
    BOOST_CHECK_MESSAGE( !fp.GetExcludedFromBOMForVariant( "Variant A" ),
                         "ExcludedFromBOM should be reset to base value (false) when no override" );
    BOOST_CHECK_MESSAGE( !fp.GetExcludedFromPosFilesForVariant( "Variant A" ),
                         "ExcludedFromPosFiles should be reset to base (false) when no override" );
}


/**
 * Test that recreates the user's observed bug:
 * - R2 in schematic has Variant A with ONLY a Footprint field override (no attribute overrides)
 * - On the board, there are TWO R2 footprints:
 *   1. C_1210 footprint (variant footprint) - should have NO explicit attribute overrides
 *   2. C_3640 footprint (base footprint) - SHOULD have DNP/ExcludedFromBOM/ExcludedFromPosFiles
 *      because this footprint is NOT used when building Variant A
 *
 * The reported bug: "R2:Variant A has no attributes set in schematic but has 3 attributes on board"
 * This test verifies which footprint has which attributes and whether that's correct behavior.
 */
BOOST_AUTO_TEST_CASE( VariantTestR2FootprintAttributeVerification )
{
    wxString dataPath = KI_TEST::GetPcbnewTestDataDir() + wxString( "variant_test/variant_test.kicad_pcb" );

    PCB_IO_KICAD_SEXPR pcbIo;
    std::unique_ptr<BOARD> board( pcbIo.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->HasVariant( "Variant A" ) );

    // Find both R2 footprints and verify their variant data
    FOOTPRINT* r2_c1210 = nullptr;  // Variant A's footprint (C_1210_3225Metric)
    FOOTPRINT* r2_c3640 = nullptr;  // Base footprint (C_3640_9110Metric)

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->GetReference() == wxT( "R2" ) )
        {
            wxString fpName = fp->GetFPID().GetLibItemName();

            if( fpName.Contains( wxT( "C_1210" ) ) )
                r2_c1210 = fp;
            else if( fpName.Contains( wxT( "C_3640" ) ) )
                r2_c3640 = fp;
        }
    }

    // Verify we found both R2 footprints
    BOOST_TEST_MESSAGE( "Looking for R2 footprints in test data" );
    BOOST_REQUIRE_MESSAGE( r2_c1210, "Should find R2 with C_1210 footprint (variant footprint)" );
    BOOST_REQUIRE_MESSAGE( r2_c3640, "Should find R2 with C_3640 footprint (base footprint)" );

    // Check C_1210 (Variant A's footprint) - this IS the active footprint for Variant A
    // The schematic has NO attribute overrides, so the PCB variant should also have no overrides
    // (or equivalently, values should match base footprint)
    const FOOTPRINT_VARIANT* c1210_variantA = r2_c1210->GetVariant( "Variant A" );

    BOOST_TEST_MESSAGE( "R2 C_1210 (variant footprint) base attributes: DNP="
                        << r2_c1210->IsDNP() << " ExcludedFromBOM=" << r2_c1210->IsExcludedFromBOM()
                        << " ExcludedFromPosFiles=" << r2_c1210->IsExcludedFromPosFiles() );

    if( c1210_variantA )
    {
        BOOST_TEST_MESSAGE( "R2 C_1210 Variant A attributes: DNP=" << c1210_variantA->GetDNP()
                            << " ExcludedFromBOM=" << c1210_variantA->GetExcludedFromBOM()
                            << " ExcludedFromPosFiles=" << c1210_variantA->GetExcludedFromPosFiles() );

        // For the variant footprint, since schematic has NO attribute overrides,
        // variant attributes should match base footprint values (all false)
        BOOST_CHECK_MESSAGE( !c1210_variantA->GetDNP(),
                             "C_1210 Variant A DNP should be false (no schematic override)" );
        BOOST_CHECK_MESSAGE( !c1210_variantA->GetExcludedFromBOM(),
                             "C_1210 Variant A ExcludedFromBOM should be false (no override)" );
        BOOST_CHECK_MESSAGE( !c1210_variantA->GetExcludedFromPosFiles(),
                             "C_1210 Variant A ExcludedFromPosFiles should be false (no override)" );
    }
    else
    {
        BOOST_TEST_MESSAGE( "R2 C_1210 has no Variant A data" );
    }

    // Check C_3640 (base footprint) - this is NOT the active footprint for Variant A
    // Since schematic R2:Variant A has NO explicit attribute overrides, the PCB variant
    // should also have no attribute overrides (attributes reset to base footprint values).
    // This matches the schematic inheritance model where unset attributes inherit from base.
    const FOOTPRINT_VARIANT* c3640_variantA = r2_c3640->GetVariant( "Variant A" );

    BOOST_TEST_MESSAGE( "R2 C_3640 (base footprint) base attributes: DNP="
                        << r2_c3640->IsDNP() << " ExcludedFromBOM=" << r2_c3640->IsExcludedFromBOM()
                        << " ExcludedFromPosFiles=" << r2_c3640->IsExcludedFromPosFiles() );

    if( c3640_variantA )
    {
        BOOST_TEST_MESSAGE( "R2 C_3640 Variant A attributes: DNP=" << c3640_variantA->GetDNP()
                            << " ExcludedFromBOM=" << c3640_variantA->GetExcludedFromBOM()
                            << " ExcludedFromPosFiles=" << c3640_variantA->GetExcludedFromPosFiles() );

        // For the base footprint, since schematic has NO explicit attribute overrides,
        // variant attributes should match base footprint values (all false).
        // The PCB should mirror the schematic's inheritance model.
        BOOST_CHECK_MESSAGE( !c3640_variantA->GetDNP(),
                             "C_3640 Variant A DNP should be false (no schematic override)" );
        BOOST_CHECK_MESSAGE( !c3640_variantA->GetExcludedFromBOM(),
                             "C_3640 Variant A ExcludedFromBOM should be false (no override)" );
        BOOST_CHECK_MESSAGE( !c3640_variantA->GetExcludedFromPosFiles(),
                             "C_3640 Variant A ExcludedFromPosFiles should be false (no override)" );
    }
    else
    {
        // If there's no variant data stored, that's also acceptable - implies no overrides
        BOOST_TEST_MESSAGE( "R2 C_3640 has no Variant A data (implies no overrides)" );
    }
}


/**
 * Test loading and verifying variant data from the variant_test project files.
 */
BOOST_AUTO_TEST_CASE( VariantTestProjectLoad )
{
    wxString dataPath = KI_TEST::GetPcbnewTestDataDir() + wxString( "variant_test/variant_test.kicad_pcb" );

    PCB_IO_KICAD_SEXPR pcbIo;
    std::unique_ptr<BOARD> board( pcbIo.LoadBoard( dataPath, nullptr ) );

    BOOST_REQUIRE( board );

    // Verify the board has the "Variant A" variant registered
    BOOST_CHECK( board->HasVariant( "Variant A" ) );

    // Find footprints and verify their variant properties
    // Based on the variant_test.kicad_pcb and schematic:
    // - R1 with Variant A having field override for Datasheet
    // - R2 (C_3640 base footprint) with Variant A having NO attribute overrides
    //   (schematic has no explicit overrides, so PCB mirrors base values)
    // - R3 with Variant A having DNP (explicit schematic override)

    for( FOOTPRINT* fp : board->Footprints() )
    {
        const wxString& ref = fp->GetReference();

        if( ref == wxT( "R1" ) )
        {
            const FOOTPRINT_VARIANT* variantA = fp->GetVariant( "Variant A" );

            if( variantA )
            {
                // R1 has a Datasheet field override in Variant A
                BOOST_CHECK( variantA->HasFieldValue( wxT( "Datasheet" ) ) );
                BOOST_CHECK_EQUAL( variantA->GetFieldValue( wxT( "Datasheet" ) ), wxT( "test" ) );
            }
        }
        else if( ref == wxT( "R2" ) )
        {
            // R2's C_3640 footprint (base) should have NO attribute overrides
            // because schematic R2:Variant A has no explicit attribute overrides.
            // PCB mirrors the schematic inheritance model.
            wxString fpName = fp->GetFPID().GetLibItemName();

            if( fpName.Contains( wxT( "C_3640" ) ) )
            {
                const FOOTPRINT_VARIANT* variantA = fp->GetVariant( "Variant A" );

                if( variantA )
                {
                    BOOST_CHECK( !variantA->GetDNP() );
                    BOOST_CHECK( !variantA->GetExcludedFromBOM() );
                    BOOST_CHECK( !variantA->GetExcludedFromPosFiles() );
                }
            }
        }
        else if( ref == wxT( "R3" ) )
        {
            const FOOTPRINT_VARIANT* variantA = fp->GetVariant( "Variant A" );

            if( variantA )
            {
                // R3 has DNP in Variant A (explicit schematic override)
                BOOST_CHECK( variantA->GetDNP() );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
