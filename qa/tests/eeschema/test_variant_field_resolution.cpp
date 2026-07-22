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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_symbol.h>
#include <sch_sheet_path.h>
#include <lib_id.h>
#include <wildcards_and_files_ext.h>


class TEST_VARIANT_FIELD_RESOLUTION_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
public:
    void LoadTestSchematic()
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.AppendDir( wxS( "variant_field_resolution" ) );
        fn.SetName( wxS( "variant_field_resolution" ) );
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        LoadSchematic( fn.GetFullPath() );
    }

    SCH_SYMBOL* GetR1()
    {
        SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();
        SCH_SCREEN*    screen = m_schematic->RootScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            if( sym->GetRef( &hierarchy[0] ) == wxS( "R1" ) )
                return sym;
        }

        return nullptr;
    }

    SCH_SYMBOL_INSTANCE* GetMutableInstance( SCH_SYMBOL* aSymbol )
    {
        SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();
        KIID_PATH      path = hierarchy[0].Path();

        auto& instances = const_cast<std::vector<SCH_SYMBOL_INSTANCE>&>(
                aSymbol->GetInstances() );

        for( SCH_SYMBOL_INSTANCE& inst : instances )
        {
            if( inst.m_Path == path )
                return &inst;
        }

        return nullptr;
    }

    void SetupVariantWithSymbolOverride( SCH_SYMBOL* aSymbol, const wxString& aVariantName,
                                        const wxString& aLibIdStr )
    {
        SCH_SYMBOL_INSTANCE* inst = GetMutableInstance( aSymbol );
        BOOST_REQUIRE( inst );

        SCH_SYMBOL_VARIANT& variant = inst->m_Variants[aVariantName];
        LIB_ID               libId;
        BOOST_REQUIRE( libId.Parse( aLibIdStr ) == -1 );
        variant.m_SymbolOverride = libId;
    }

    void SetVariantField( SCH_SYMBOL* aSymbol, const wxString& aVariantName,
                          const wxString& aFieldName, const wxString& aValue )
    {
        SCH_SYMBOL_INSTANCE* inst = GetMutableInstance( aSymbol );
        BOOST_REQUIRE( inst );

        inst->m_Variants[aVariantName].m_Fields[aFieldName] = aValue;
    }

    void ClearVariants( SCH_SYMBOL* aSymbol )
    {
        SCH_SYMBOL_INSTANCE* inst = GetMutableInstance( aSymbol );

        if( inst )
            inst->m_Variants.clear();

        aSymbol->ClearCaches();
    }
};


BOOST_FIXTURE_TEST_CASE( VariantFieldResolution, TEST_VARIANT_FIELD_RESOLUTION_FIXTURE )
{
    LoadTestSchematic();

    SCH_SYMBOL* r1 = GetR1();
    BOOST_REQUIRE( r1 );

    // No variant returns base value
    {
        wxString value =
                r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0], wxEmptyString );
        BOOST_CHECK_EQUAL( value, wxS( "100R" ) );
    }

    // Explicit variant field override takes precedence over the base value
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "ExplicitOnly" ), wxS( "Value" ), wxS( "220R" ) );

        wxString value = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                           wxS( "ExplicitOnly" ) );
        BOOST_CHECK_EQUAL( value, wxS( "220R" ) );
    }

    // Explicit override on multiple fields simultaneously
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "Multi" ), wxS( "Value" ), wxS( "470R" ) );
        SetVariantField( r1, wxS( "Multi" ), wxS( "MPN" ), wxS( "CUSTOM-MPN-123" ) );

        wxString value = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                           wxS( "Multi" ) );
        wxString mpn =
                r1->GetFieldText( wxS( "MPN" ), &m_schematic->Hierarchy()[0], wxS( "Multi" ) );

        BOOST_CHECK_EQUAL( value, wxS( "470R" ) );
        BOOST_CHECK_EQUAL( mpn, wxS( "CUSTOM-MPN-123" ) );
    }

    // Reference is always resolved from the base instance
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "RefTest" ), wxS( "Value" ), wxS( "999R" ) );

        wxString ref = r1->GetFieldText( wxS( "Reference" ), &m_schematic->Hierarchy()[0],
                                         wxS( "RefTest" ) );
        BOOST_CHECK_EQUAL( ref, wxS( "R1" ) );
    }

    // An unresolvable symbol override with no explicit field override falls back to the base
    // value.
    {
        ClearVariants( r1 );
        SetupVariantWithSymbolOverride( r1, wxS( "BadAlt" ),
                                       wxS( "nonexistent_lib:no_such_symbol" ) );

        wxString value = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                           wxS( "BadAlt" ) );
        BOOST_CHECK_EQUAL( value, wxS( "100R" ) );
    }

    // Non-existent variant name falls back to base
    {
        ClearVariants( r1 );

        wxString value = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                           wxS( "NoSuchVariant" ) );
        BOOST_CHECK_EQUAL( value, wxS( "100R" ) );
    }

    // Footprint explicit override
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "FPTest" ), wxS( "Footprint" ),
                         wxS( "Resistor_SMD:R_0805_2012Metric" ) );

        wxString fp = r1->GetFieldText( wxS( "Footprint" ), &m_schematic->Hierarchy()[0],
                                        wxS( "FPTest" ) );
        BOOST_CHECK_EQUAL( fp, wxS( "Resistor_SMD:R_0805_2012Metric" ) );
    }

    // Footprint without variant override returns base
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "NoFP" ), wxS( "Value" ), wxS( "330R" ) );

        wxString fp = r1->GetFieldText( wxS( "Footprint" ), &m_schematic->Hierarchy()[0],
                                        wxS( "NoFP" ) );
        BOOST_CHECK_EQUAL( fp, wxS( "Resistor_SMD:R_0402_1005Metric" ) );
    }

    // Multiple variants resolve independently
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "VarA" ), wxS( "Value" ), wxS( "1K" ) );
        SetVariantField( r1, wxS( "VarB" ), wxS( "Value" ), wxS( "10K" ) );

        wxString valueA =
                r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0], wxS( "VarA" ) );
        wxString valueB =
                r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0], wxS( "VarB" ) );

        BOOST_CHECK_EQUAL( valueA, wxS( "1K" ) );
        BOOST_CHECK_EQUAL( valueB, wxS( "10K" ) );
    }

    // Clearing variants and re-adding must not resolve through a stale cache
    {
        ClearVariants( r1 );
        SetVariantField( r1, wxS( "CacheA" ), wxS( "Value" ), wxS( "FIRST" ) );

        wxString value1 = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                            wxS( "CacheA" ) );
        BOOST_CHECK_EQUAL( value1, wxS( "FIRST" ) );

        ClearVariants( r1 );
        SetVariantField( r1, wxS( "CacheA" ), wxS( "Value" ), wxS( "SECOND" ) );

        wxString value2 = r1->GetFieldText( wxS( "Value" ), &m_schematic->Hierarchy()[0],
                                            wxS( "CacheA" ) );
        BOOST_CHECK_EQUAL( value2, wxS( "SECOND" ) );
    }
}


BOOST_FIXTURE_TEST_CASE( VariantFieldWriteRoundTrip, TEST_VARIANT_FIELD_RESOLUTION_FIXTURE )
{
    LoadTestSchematic();

    SCH_SYMBOL* r1 = GetR1();
    BOOST_REQUIRE( r1 );

    SCH_SHEET_LIST  hierarchy = m_schematic->Hierarchy();
    SCH_SHEET_PATH& sheet = hierarchy[0];

    // Writing back the resolved value must not materialize an explicit override
    {
        ClearVariants( r1 );
        SetupVariantWithSymbolOverride( r1, wxS( "RoundTrip" ), wxS( "variant_test_lib:R_10K" ) );

        wxString resolved = r1->GetFieldText( wxS( "Value" ), &sheet, wxS( "RoundTrip" ) );
        r1->SetFieldText( wxS( "Value" ), resolved, &sheet, wxS( "RoundTrip" ) );

        std::optional<SCH_SYMBOL_VARIANT> variant = r1->GetVariant( sheet, wxS( "RoundTrip" ) );
        BOOST_REQUIRE( variant.has_value() );
        BOOST_CHECK( !variant->m_Fields.contains( wxS( "Value" ) ) );
    }

    // A real edit creates an override and reverting the edit erases it again
    {
        r1->SetFieldText( wxS( "Value" ), wxS( "EDITED" ), &sheet, wxS( "RoundTrip" ) );

        std::optional<SCH_SYMBOL_VARIANT> variant = r1->GetVariant( sheet, wxS( "RoundTrip" ) );
        BOOST_REQUIRE( variant.has_value() );
        BOOST_CHECK( variant->m_Fields.contains( wxS( "Value" ) ) );

        wxString baseValue = r1->GetFieldText( wxS( "Value" ), &sheet, wxEmptyString );
        r1->SetFieldText( wxS( "Value" ), baseValue, &sheet, wxS( "RoundTrip" ) );

        variant = r1->GetVariant( sheet, wxS( "RoundTrip" ) );
        BOOST_REQUIRE( variant.has_value() );
        BOOST_CHECK( !variant->m_Fields.contains( wxS( "Value" ) ) );
    }
}
