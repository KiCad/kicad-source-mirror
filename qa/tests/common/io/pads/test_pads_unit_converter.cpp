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
 * @file test_pads_unit_converter.cpp
 * Test suite for PADS_UNIT_CONVERTER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <io/pads/pads_unit_converter.h>

#include <ostream>

std::ostream& operator<<( std::ostream& os, PADS_UNIT_TYPE aType )
{
    switch( aType )
    {
    case PADS_UNIT_TYPE::MILS:   os << "MILS"; break;
    case PADS_UNIT_TYPE::METRIC: os << "METRIC"; break;
    case PADS_UNIT_TYPE::INCHES: os << "INCHES"; break;
    }

    return os;
}

struct PADS_UNIT_CONVERTER_FIXTURE
{
    PADS_UNIT_CONVERTER_FIXTURE() {}
};

BOOST_FIXTURE_TEST_SUITE( PadsUnitConverter, PADS_UNIT_CONVERTER_FIXTURE )


BOOST_AUTO_TEST_CASE( DefaultsToMils )
{
    PADS_UNIT_CONVERTER converter;
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::MILS );
}


BOOST_AUTO_TEST_CASE( MilsConversion )
{
    PADS_UNIT_CONVERTER converter;
    converter.SetBaseUnits( PADS_UNIT_TYPE::MILS );

    // 1000 mils = 1 inch = 25.4 mm = 25,400,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1000.0 ), 25400000 );

    // 1 mil = 25,400 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 );

    // 100 mils = 2,540,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 100.0 ), 2540000 );

    // Test size conversion (same as coordinate for now)
    BOOST_CHECK_EQUAL( converter.ToNanometersSize( 1000.0 ), 25400000 );
}


BOOST_AUTO_TEST_CASE( MetricConversion )
{
    PADS_UNIT_CONVERTER converter;
    converter.SetBaseUnits( PADS_UNIT_TYPE::METRIC );

    // 1 mm = 1,000,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 );

    // 25.4 mm = 25,400,000 nm (1 inch)
    BOOST_CHECK_EQUAL( converter.ToNanometers( 25.4 ), 25400000 );

    // 0.1 mm = 100,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.1 ), 100000 );

    // Test size conversion
    BOOST_CHECK_EQUAL( converter.ToNanometersSize( 1.0 ), 1000000 );
}


BOOST_AUTO_TEST_CASE( InchesConversion )
{
    PADS_UNIT_CONVERTER converter;
    converter.SetBaseUnits( PADS_UNIT_TYPE::INCHES );

    // 1 inch = 25,400,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400000 );

    // 0.001 inch (1 mil) = 25,400 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.001 ), 25400 );

    // 0.1 inch = 2,540,000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.1 ), 2540000 );

    // Test size conversion
    BOOST_CHECK_EQUAL( converter.ToNanometersSize( 1.0 ), 25400000 );
}


BOOST_AUTO_TEST_CASE( UnitTypeGetter )
{
    PADS_UNIT_CONVERTER converter;

    converter.SetBaseUnits( PADS_UNIT_TYPE::MILS );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::MILS );

    converter.SetBaseUnits( PADS_UNIT_TYPE::METRIC );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::METRIC );

    converter.SetBaseUnits( PADS_UNIT_TYPE::INCHES );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::INCHES );
}


BOOST_AUTO_TEST_CASE( ZeroConversion )
{
    PADS_UNIT_CONVERTER converter;

    converter.SetBaseUnits( PADS_UNIT_TYPE::MILS );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.0 ), 0 );

    converter.SetBaseUnits( PADS_UNIT_TYPE::METRIC );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.0 ), 0 );

    converter.SetBaseUnits( PADS_UNIT_TYPE::INCHES );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 0.0 ), 0 );
}


BOOST_AUTO_TEST_CASE( NegativeConversion )
{
    PADS_UNIT_CONVERTER converter;
    converter.SetBaseUnits( PADS_UNIT_TYPE::MILS );

    // Negative values should convert correctly
    BOOST_CHECK_EQUAL( converter.ToNanometers( -1000.0 ), -25400000 );
    BOOST_CHECK_EQUAL( converter.ToNanometersSize( -100.0 ), -2540000 );
}


BOOST_AUTO_TEST_CASE( ConstantsAreCorrect )
{
    // Verify the conversion constants are accurate
    BOOST_CHECK_EQUAL( PADS_UNIT_CONVERTER::MILS_TO_NM, 25400.0 );
    BOOST_CHECK_EQUAL( PADS_UNIT_CONVERTER::MM_TO_NM, 1000000.0 );
    BOOST_CHECK_EQUAL( PADS_UNIT_CONVERTER::INCHES_TO_NM, 25400000.0 );
    BOOST_CHECK_CLOSE( PADS_UNIT_CONVERTER::BASIC_TO_NM,
                       PADS_UNIT_CONVERTER::MILS_TO_NM / 38100.0, 1e-10 );

    // Cross-check: 1000 mils should equal 1 inch
    BOOST_CHECK_EQUAL( 1000.0 * PADS_UNIT_CONVERTER::MILS_TO_NM,
                       1.0 * PADS_UNIT_CONVERTER::INCHES_TO_NM );

    // Cross-check: 25.4 mm should equal 1 inch
    BOOST_CHECK_EQUAL( 25.4 * PADS_UNIT_CONVERTER::MM_TO_NM,
                       1.0 * PADS_UNIT_CONVERTER::INCHES_TO_NM );

    // Cross-check: 38100 BASIC units should equal 1 mil
    BOOST_CHECK_CLOSE( 38100.0 * PADS_UNIT_CONVERTER::BASIC_TO_NM,
                       1.0 * PADS_UNIT_CONVERTER::MILS_TO_NM, 1e-6 );
}


BOOST_AUTO_TEST_CASE( BasicUnitsMode )
{
    PADS_UNIT_CONVERTER converter;

    // Default should not be in BASIC mode
    BOOST_CHECK( !converter.IsBasicUnitsMode() );

    // Enable BASIC mode
    converter.SetBasicUnitsMode( true );
    BOOST_CHECK( converter.IsBasicUnitsMode() );

    // 38100 BASIC units = 1 mil = 25,400 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 38100.0 ), 25400 );

    // 1 BASIC unit rounds to 1 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1 );

    // 3 BASIC units = 2 nm (3 * 2/3 = 2)
    BOOST_CHECK_EQUAL( converter.ToNanometers( 3.0 ), 2 );

    // Disable BASIC mode should revert to base units
    converter.SetBasicUnitsMode( false );
    BOOST_CHECK( !converter.IsBasicUnitsMode() );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 ); // Back to MILS
}


BOOST_AUTO_TEST_CASE( BasicUnitsCustomScale )
{
    PADS_UNIT_CONVERTER converter;
    converter.SetBasicUnitsMode( true );

    // Default scale
    BOOST_CHECK_EQUAL( converter.GetBasicUnitsScale(), PADS_UNIT_CONVERTER::BASIC_TO_NM );

    // Set custom scale (e.g., 5.0 nm per unit)
    converter.SetBasicUnitsScale( 5.0 );
    BOOST_CHECK_EQUAL( converter.GetBasicUnitsScale(), 5.0 );

    // 1000 units at 5.0 nm/unit = 5000 nm
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1000.0 ), 5000 );
}


BOOST_AUTO_TEST_CASE( ParseFileHeaderBasic )
{
    PADS_UNIT_CONVERTER converter;

    // Test BASIC header detection
    BOOST_CHECK( converter.ParseFileHeader( "!PADS-POWERPCB-V9.0-BASIC!" ) );
    BOOST_CHECK( converter.IsBasicUnitsMode() );

    // Reset
    converter.SetBasicUnitsMode( false );

    // Test lowercase basic
    BOOST_CHECK( converter.ParseFileHeader( "!PADS-POWERPCB-V9.0-basic!" ) );
    BOOST_CHECK( converter.IsBasicUnitsMode() );
}


BOOST_AUTO_TEST_CASE( ParseFileHeaderMils )
{
    PADS_UNIT_CONVERTER converter;

    // Test MILS header detection
    BOOST_CHECK( converter.ParseFileHeader( "!PADS-POWERPCB-V9.5-MILS!" ) );
    BOOST_CHECK( !converter.IsBasicUnitsMode() );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::MILS );
}


BOOST_AUTO_TEST_CASE( ParseFileHeaderMetric )
{
    PADS_UNIT_CONVERTER converter;

    // Test METRIC header detection
    BOOST_CHECK( converter.ParseFileHeader( "!PADS-POWERPCB-V9.5-METRIC!" ) );
    BOOST_CHECK( !converter.IsBasicUnitsMode() );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::METRIC );
}


BOOST_AUTO_TEST_CASE( ParseFileHeaderInches )
{
    PADS_UNIT_CONVERTER converter;

    // Test INCHES header detection
    BOOST_CHECK( converter.ParseFileHeader( "!PADS-POWERPCB-V9.5-INCHES!" ) );
    BOOST_CHECK( !converter.IsBasicUnitsMode() );
    BOOST_CHECK_EQUAL( converter.GetUnitType(), PADS_UNIT_TYPE::INCHES );
}


BOOST_AUTO_TEST_CASE( ParseFileHeaderUnknown )
{
    PADS_UNIT_CONVERTER converter;

    // Unknown header should return false
    BOOST_CHECK( !converter.ParseFileHeader( "!UNKNOWN-FORMAT!" ) );
}


BOOST_AUTO_TEST_CASE( BasicModeOverridesBaseUnits )
{
    PADS_UNIT_CONVERTER converter;

    // Set to METRIC
    converter.SetBaseUnits( PADS_UNIT_TYPE::METRIC );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 ); // 1 mm

    // Enable BASIC - should override METRIC
    converter.SetBasicUnitsMode( true );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1 );

    // Disable BASIC - should revert to METRIC
    converter.SetBasicUnitsMode( false );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 ); // Back to mm
}


BOOST_AUTO_TEST_CASE( ParseUnitCodeMils )
{
    // Test "M" -> MILS
    auto result = PADS_UNIT_CONVERTER::ParseUnitCode( "M" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::MILS );

    // Test lowercase
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "m" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::MILS );

    // Test "D" (default) -> MILS
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "D" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::MILS );

    // Test long form
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "MILS" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::MILS );
}


BOOST_AUTO_TEST_CASE( ParseUnitCodeMetric )
{
    // Test "MM" -> METRIC
    auto result = PADS_UNIT_CONVERTER::ParseUnitCode( "MM" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::METRIC );

    // Test lowercase
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "mm" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::METRIC );

    // Test long form
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "METRIC" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::METRIC );
}


BOOST_AUTO_TEST_CASE( ParseUnitCodeInches )
{
    // Test "I" -> INCHES
    auto result = PADS_UNIT_CONVERTER::ParseUnitCode( "I" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::INCHES );

    // Test lowercase
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "i" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::INCHES );

    // Test long form
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "INCHES" );
    BOOST_CHECK( result.has_value() );
    BOOST_CHECK_EQUAL( *result, PADS_UNIT_TYPE::INCHES );
}


BOOST_AUTO_TEST_CASE( ParseUnitCodeNoOverride )
{
    // Test "N" -> no override (empty optional)
    auto result = PADS_UNIT_CONVERTER::ParseUnitCode( "N" );
    BOOST_CHECK( !result.has_value() );

    // Test empty string
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "" );
    BOOST_CHECK( !result.has_value() );

    // Test invalid code
    result = PADS_UNIT_CONVERTER::ParseUnitCode( "X" );
    BOOST_CHECK( !result.has_value() );

    result = PADS_UNIT_CONVERTER::ParseUnitCode( "UNKNOWN" );
    BOOST_CHECK( !result.has_value() );
}


BOOST_AUTO_TEST_CASE( PushPopUnitOverride )
{
    PADS_UNIT_CONVERTER converter;

    // Start with MILS
    converter.SetBaseUnits( PADS_UNIT_TYPE::MILS );
    BOOST_CHECK( !converter.HasUnitOverride() );
    BOOST_CHECK_EQUAL( converter.GetOverrideDepth(), 0 );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 ); // 1 mil

    // Push METRIC override
    BOOST_CHECK( converter.PushUnitOverride( "MM" ) );
    BOOST_CHECK( converter.HasUnitOverride() );
    BOOST_CHECK_EQUAL( converter.GetOverrideDepth(), 1 );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 ); // 1 mm

    // Push INCHES override (nested)
    BOOST_CHECK( converter.PushUnitOverride( "I" ) );
    BOOST_CHECK_EQUAL( converter.GetOverrideDepth(), 2 );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400000 ); // 1 inch

    // Pop back to METRIC
    converter.PopUnitOverride();
    BOOST_CHECK_EQUAL( converter.GetOverrideDepth(), 1 );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 ); // 1 mm

    // Pop back to base MILS
    converter.PopUnitOverride();
    BOOST_CHECK( !converter.HasUnitOverride() );
    BOOST_CHECK_EQUAL( converter.GetOverrideDepth(), 0 );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 ); // 1 mil
}


BOOST_AUTO_TEST_CASE( PushInvalidUnitCode )
{
    PADS_UNIT_CONVERTER converter;

    // Invalid code should return false and not push
    BOOST_CHECK( !converter.PushUnitOverride( "X" ) );
    BOOST_CHECK( !converter.HasUnitOverride() );

    // "N" (no override) should return false and not push
    BOOST_CHECK( !converter.PushUnitOverride( "N" ) );
    BOOST_CHECK( !converter.HasUnitOverride() );

    // Empty string should return false and not push
    BOOST_CHECK( !converter.PushUnitOverride( "" ) );
    BOOST_CHECK( !converter.HasUnitOverride() );
}


BOOST_AUTO_TEST_CASE( PopEmptyStack )
{
    PADS_UNIT_CONVERTER converter;

    // Popping empty stack should be safe (no-op)
    BOOST_CHECK( !converter.HasUnitOverride() );
    converter.PopUnitOverride();
    BOOST_CHECK( !converter.HasUnitOverride() );

    // Conversion should still work
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 ); // Default MILS
}


BOOST_AUTO_TEST_CASE( OverrideDoesNotAffectBasicMode )
{
    PADS_UNIT_CONVERTER converter;

    // Enable BASIC mode
    converter.SetBasicUnitsMode( true );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 38100.0 ), 25400 ); // BASIC

    // Push override - should have no effect in BASIC mode
    BOOST_CHECK( converter.PushUnitOverride( "MM" ) );
    BOOST_CHECK( converter.HasUnitOverride() );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 38100.0 ), 25400 ); // Still BASIC

    // Disable BASIC - now override should take effect
    converter.SetBasicUnitsMode( false );
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 1000000 ); // METRIC override

    // Pop override
    converter.PopUnitOverride();
    BOOST_CHECK_EQUAL( converter.ToNanometers( 1.0 ), 25400 ); // Back to MILS
}


BOOST_AUTO_TEST_SUITE_END()
