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
 * @file test_pads_attribute_mapper.cpp
 * Test suite for PADS_ATTRIBUTE_MAPPER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <io/pads/pads_attribute_mapper.h>


struct PADS_ATTRIBUTE_MAPPER_FIXTURE
{
    PADS_ATTRIBUTE_MAPPER_FIXTURE() {}
};


BOOST_FIXTURE_TEST_SUITE( PadsAttributeMapper, PADS_ATTRIBUTE_MAPPER_FIXTURE )


BOOST_AUTO_TEST_CASE( ReferenceFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Various PADS reference designator formats
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Ref.Des." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "REF.DES." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Ref Des" ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "REFDES" ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Reference" ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );

    // Verify IsReferenceField
    BOOST_CHECK( mapper.IsReferenceField( "Ref.Des." ) );
    BOOST_CHECK( mapper.IsReferenceField( "refdes" ) );
    BOOST_CHECK( !mapper.IsReferenceField( "Part Type" ) );
}


BOOST_AUTO_TEST_CASE( ValueFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Various PADS value/part type formats
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Part Type" ), PADS_ATTRIBUTE_MAPPER::FIELD_VALUE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PART TYPE" ), PADS_ATTRIBUTE_MAPPER::FIELD_VALUE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PartType" ), PADS_ATTRIBUTE_MAPPER::FIELD_VALUE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "VALUE" ), PADS_ATTRIBUTE_MAPPER::FIELD_VALUE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Part_Type" ), PADS_ATTRIBUTE_MAPPER::FIELD_VALUE );

    // Verify IsValueField
    BOOST_CHECK( mapper.IsValueField( "Part Type" ) );
    BOOST_CHECK( mapper.IsValueField( "value" ) );
    BOOST_CHECK( !mapper.IsValueField( "Ref.Des." ) );
}


BOOST_AUTO_TEST_CASE( FootprintFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Various PADS footprint/decal formats
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PCB Decal" ), PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PCB_DECAL" ), PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "DECAL" ), PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Footprint" ), PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Pattern" ), PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT );

    // Verify IsFootprintField
    BOOST_CHECK( mapper.IsFootprintField( "PCB Decal" ) );
    BOOST_CHECK( mapper.IsFootprintField( "decal" ) );
    BOOST_CHECK( !mapper.IsFootprintField( "Value" ) );
}


BOOST_AUTO_TEST_CASE( DatasheetFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Datasheet" ), PADS_ATTRIBUTE_MAPPER::FIELD_DATASHEET );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Data Sheet" ), PADS_ATTRIBUTE_MAPPER::FIELD_DATASHEET );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "SPEC" ), PADS_ATTRIBUTE_MAPPER::FIELD_DATASHEET );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Specification" ), PADS_ATTRIBUTE_MAPPER::FIELD_DATASHEET );
}


BOOST_AUTO_TEST_CASE( MPNFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Part Number" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PART_NUMBER" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PartNumber" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PN" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "MPN" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Mfr Part Number" ), PADS_ATTRIBUTE_MAPPER::FIELD_MPN );
}


BOOST_AUTO_TEST_CASE( ManufacturerFieldMapping )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Manufacturer" ), PADS_ATTRIBUTE_MAPPER::FIELD_MANUFACTURER );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "MFR" ), PADS_ATTRIBUTE_MAPPER::FIELD_MANUFACTURER );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "MFG" ), PADS_ATTRIBUTE_MAPPER::FIELD_MANUFACTURER );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Vendor" ), PADS_ATTRIBUTE_MAPPER::FIELD_MANUFACTURER );
}


BOOST_AUTO_TEST_CASE( UnknownAttributePassthrough )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Unknown attributes should pass through unchanged
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "CustomField" ), "CustomField" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "My Custom Attr" ), "My Custom Attr" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "XYZ_123" ), "XYZ_123" );

    // Unknown attributes are not standard fields
    BOOST_CHECK( !mapper.IsStandardField( "CustomField" ) );
    BOOST_CHECK( !mapper.IsReferenceField( "CustomField" ) );
    BOOST_CHECK( !mapper.IsValueField( "CustomField" ) );
    BOOST_CHECK( !mapper.IsFootprintField( "CustomField" ) );
}


BOOST_AUTO_TEST_CASE( CaseInsensitivity )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // All lookups should be case-insensitive
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "ref.des." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "REF.DES." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Ref.Des." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "rEf.dEs." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );

    // Case insensitivity for standard field checks
    BOOST_CHECK( mapper.IsStandardField( "REF.DES." ) );
    BOOST_CHECK( mapper.IsStandardField( "ref.des." ) );
    BOOST_CHECK( mapper.IsStandardField( "PART TYPE" ) );
    BOOST_CHECK( mapper.IsStandardField( "pcb decal" ) );
}


BOOST_AUTO_TEST_CASE( IsStandardFieldMethod )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Standard fields (Reference, Value, Footprint)
    BOOST_CHECK( mapper.IsStandardField( "Ref.Des." ) );
    BOOST_CHECK( mapper.IsStandardField( "Part Type" ) );
    BOOST_CHECK( mapper.IsStandardField( "PCB Decal" ) );

    // Non-standard fields (MPN, Manufacturer, Datasheet)
    BOOST_CHECK( !mapper.IsStandardField( "Part Number" ) );
    BOOST_CHECK( !mapper.IsStandardField( "Manufacturer" ) );
    BOOST_CHECK( !mapper.IsStandardField( "Datasheet" ) );
}


BOOST_AUTO_TEST_CASE( CustomMappingOverride )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Add a custom mapping that overrides a standard one
    mapper.AddMapping( "Part Type", "CustomValue" );

    // Custom mapping should take precedence
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Part Type" ), "CustomValue" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "PART TYPE" ), "CustomValue" );

    // Other standard mappings should still work
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Ref.Des." ), PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE );
}


BOOST_AUTO_TEST_CASE( CustomMappingNew )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    // Add a completely new custom mapping
    mapper.AddMapping( "My PADS Attr", "MyKiCadField" );

    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "My PADS Attr" ), "MyKiCadField" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "MY PADS ATTR" ), "MyKiCadField" );

    // Verify it appears in GetMappings
    const auto& mappings = mapper.GetMappings();
    BOOST_CHECK_EQUAL( mappings.size(), 1 );
    BOOST_CHECK_EQUAL( mappings.at( "my pads attr" ), "MyKiCadField" );
}


BOOST_AUTO_TEST_CASE( MultipleCustomMappings )
{
    PADS_ATTRIBUTE_MAPPER mapper;

    mapper.AddMapping( "Attr1", "Field1" );
    mapper.AddMapping( "Attr2", "Field2" );
    mapper.AddMapping( "Attr3", "Field3" );

    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Attr1" ), "Field1" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Attr2" ), "Field2" );
    BOOST_CHECK_EQUAL( mapper.GetKiCadFieldName( "Attr3" ), "Field3" );

    const auto& mappings = mapper.GetMappings();
    BOOST_CHECK_EQUAL( mappings.size(), 3 );
}


BOOST_AUTO_TEST_CASE( FieldConstants )
{
    // Verify the field constants have expected values
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_REFERENCE ), "Reference" );
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_VALUE ), "Value" );
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_FOOTPRINT ), "Footprint" );
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_DATASHEET ), "Datasheet" );
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_MPN ), "MPN" );
    BOOST_CHECK_EQUAL( std::string( PADS_ATTRIBUTE_MAPPER::FIELD_MANUFACTURER ), "Manufacturer" );
}


BOOST_AUTO_TEST_SUITE_END()
