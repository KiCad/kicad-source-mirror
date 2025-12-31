/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
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
 * @file test_pin_text_overlap.cpp
 * Test that pin names and numbers don't overlap between adjacent pins at any rotation.
 * This is a regression test for issue 21980.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_pin.h>
#include <lib_symbol.h>
#include <pin_layout_cache.h>
#include <transform.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/log.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( PinTextOverlap )

/**
 * Create a test symbol with adjacent pins for testing overlap.
 * This simulates the symbol structure from issue 21980 where pins are
 * closely spaced and have names displayed outside (offset 0).
 */
static std::unique_ptr<LIB_SYMBOL> createAdjacentPinsSymbol()
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestAdjacentPins" ) );

    // Set pin name offset to 0 so names are positioned outside (like numbers)
    // This is the configuration that triggers the overlap issue
    symbol->SetPinNameOffset( 0 );
    symbol->SetShowPinNames( true );
    symbol->SetShowPinNumbers( true );

    // Pin spacing of 5.08mm (200 mils) - typical for connector symbols
    const int pinSpacing = schIUScale.MilsToIU( 200 );
    const int pinLength = schIUScale.MilsToIU( 100 );

    // Create 4 adjacent pins pointing left (like connector J1 pins)
    // These pins will have names and numbers that could potentially overlap
    for( int i = 0; i < 4; i++ )
    {
        auto pin = std::make_unique<SCH_PIN>( symbol.get() );
        pin->SetPosition( VECTOR2I( 0, i * pinSpacing ) );
        pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
        pin->SetLength( pinLength );

        // Use realistic names similar to the issue report
        wxString name = wxString::Format( wxT( "GPIO%d" ), 28 + i );
        wxString number = wxString::Format( wxT( "J1.%d" ), i + 1 );

        pin->SetName( name );
        pin->SetNumber( number );
        pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
        pin->SetUnit( 1 );

        symbol->AddDrawItem( pin.release() );
    }

    return symbol;
}


/**
 * Get the bounding box of pin text (name or number) for a given pin.
 */
static BOX2I getTextBoundingBox( const PIN_LAYOUT_CACHE::TEXT_INFO& textInfo )
{
    // Estimate text dimensions based on character count and font size
    int textHeight = textInfo.m_TextSize;
    int textWidth = textInfo.m_Text.Length() * textInfo.m_TextSize * 6 / 10;

    // Handle vertical text - swap width and height
    if( textInfo.m_Angle == ANGLE_VERTICAL )
        std::swap( textWidth, textHeight );

    BOX2I bbox;
    bbox.SetOrigin( textInfo.m_TextPosition.x - textWidth / 2,
                    textInfo.m_TextPosition.y - textHeight / 2 );
    bbox.SetSize( textWidth, textHeight );

    return bbox;
}


/**
 * Calculate perpendicular distance from pin line to text center.
 * For horizontal pins (PIN_LEFT/PIN_RIGHT), this is the Y distance.
 * For vertical pins (PIN_UP/PIN_DOWN), this is the X distance.
 */
static int getPerpendicularDistance( const SCH_PIN* pin, const VECTOR2I& textPos,
                                     const TRANSFORM& transform )
{
    VECTOR2I pinPos = pin->GetPosition();
    PIN_ORIENTATION orient = pin->PinDrawOrient( transform );

    if( orient == PIN_ORIENTATION::PIN_LEFT || orient == PIN_ORIENTATION::PIN_RIGHT )
    {
        // Horizontal pin - perpendicular distance is in Y
        return std::abs( textPos.y - pinPos.y );
    }
    else
    {
        // Vertical pin - perpendicular distance is in X
        return std::abs( textPos.x - pinPos.x );
    }
}


/**
 * Test that pin names and numbers don't overlap with adjacent pins at 0° and 90°.
 *
 * This is the core test for issue 21980. At 0° the pins are horizontal and
 * names/numbers are positioned above/below. At 90° the pins become vertical
 * and names/numbers should be positioned left/right with the same clearance.
 */
BOOST_AUTO_TEST_CASE( AdjacentPinTextNoOverlap )
{
    // Create test symbol with adjacent pins
    std::unique_ptr<LIB_SYMBOL> symbol = createAdjacentPinsSymbol();
    BOOST_REQUIRE( symbol );

    // Get the pins
    std::vector<SCH_PIN*> pins;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            pins.push_back( static_cast<SCH_PIN*>( &item ) );
    }

    BOOST_REQUIRE_GE( pins.size(), 2 );

    // Test at 0° (identity transform) and 90° rotation
    std::vector<TRANSFORM> rotations = {
        TRANSFORM( 1, 0, 0, 1 ),   // 0° (identity) - pins are horizontal
        TRANSFORM( 0, -1, 1, 0 ),  // 90° CCW - pins become vertical
    };

    std::vector<wxString> rotationNames = { wxT( "0°" ), wxT( "90°" ) };

    for( size_t r = 0; r < rotations.size(); r++ )
    {
        const TRANSFORM& transform = rotations[r];
        const wxString&  rotName = rotationNames[r];

        // Set global transform for this test
        TRANSFORM oldTransform = DefaultTransform;
        DefaultTransform = transform;

        // Collect text bounding boxes for all pins
        struct PinTextBoxes
        {
            wxString pinNumber;
            BOX2I    nameBBox;
            BOX2I    numberBBox;
            bool     hasName;
            bool     hasNumber;
        };

        std::vector<PinTextBoxes> pinBoxes;

        for( SCH_PIN* pin : pins )
        {
            PinTextBoxes boxes;
            boxes.pinNumber = pin->GetNumber();
            boxes.hasName = false;
            boxes.hasNumber = false;

            PIN_LAYOUT_CACHE cache( *pin );

            // Get name bounding box
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo = cache.GetPinNameInfo( 0 );

            if( nameInfo.has_value() && !nameInfo->m_Text.IsEmpty() )
            {
                boxes.nameBBox = getTextBoundingBox( *nameInfo );
                boxes.hasName = true;
            }

            // Get number bounding box
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfo = cache.GetPinNumberInfo( 0 );

            if( numberInfo.has_value() && !numberInfo->m_Text.IsEmpty() )
            {
                boxes.numberBBox = getTextBoundingBox( *numberInfo );
                boxes.hasNumber = true;
            }

            pinBoxes.push_back( boxes );
        }

        // Check that names and numbers of different pins don't overlap
        for( size_t i = 0; i < pinBoxes.size(); i++ )
        {
            for( size_t j = i + 1; j < pinBoxes.size(); j++ )
            {
                const PinTextBoxes& pinA = pinBoxes[i];
                const PinTextBoxes& pinB = pinBoxes[j];

                // Check name-to-name overlap
                if( pinA.hasName && pinB.hasName )
                {
                    bool namesOverlap = pinA.nameBBox.Intersects( pinB.nameBBox );
                    BOOST_CHECK_MESSAGE(
                            !namesOverlap,
                            "At " << rotName << ": Pin " << pinA.pinNumber << " name overlaps with pin "
                                  << pinB.pinNumber << " name. "
                                  << "BBoxA=(" << pinA.nameBBox.GetLeft() << ","
                                  << pinA.nameBBox.GetTop() << ")-(" << pinA.nameBBox.GetRight()
                                  << "," << pinA.nameBBox.GetBottom() << ") "
                                  << "BBoxB=(" << pinB.nameBBox.GetLeft() << ","
                                  << pinB.nameBBox.GetTop() << ")-(" << pinB.nameBBox.GetRight()
                                  << "," << pinB.nameBBox.GetBottom() << ")" );
                }

                // Check number-to-number overlap
                if( pinA.hasNumber && pinB.hasNumber )
                {
                    bool numbersOverlap = pinA.numberBBox.Intersects( pinB.numberBBox );
                    BOOST_CHECK_MESSAGE(
                            !numbersOverlap,
                            "At " << rotName << ": Pin " << pinA.pinNumber
                                  << " number overlaps with pin " << pinB.pinNumber << " number. "
                                  << "BBoxA=(" << pinA.numberBBox.GetLeft() << ","
                                  << pinA.numberBBox.GetTop() << ")-("
                                  << pinA.numberBBox.GetRight() << ","
                                  << pinA.numberBBox.GetBottom() << ") "
                                  << "BBoxB=(" << pinB.numberBBox.GetLeft() << ","
                                  << pinB.numberBBox.GetTop() << ")-("
                                  << pinB.numberBBox.GetRight() << ","
                                  << pinB.numberBBox.GetBottom() << ")" );
                }

                // Check name-to-number overlap between pins
                if( pinA.hasName && pinB.hasNumber )
                {
                    bool overlap = pinA.nameBBox.Intersects( pinB.numberBBox );
                    BOOST_CHECK_MESSAGE(
                            !overlap,
                            "At " << rotName << ": Pin " << pinA.pinNumber
                                  << " name overlaps with pin " << pinB.pinNumber << " number." );
                }

                if( pinA.hasNumber && pinB.hasName )
                {
                    bool overlap = pinA.numberBBox.Intersects( pinB.nameBBox );
                    BOOST_CHECK_MESSAGE(
                            !overlap,
                            "At " << rotName << ": Pin " << pinA.pinNumber
                                  << " number overlaps with pin " << pinB.pinNumber << " name." );
                }
            }
        }

        // Restore original transform
        DefaultTransform = oldTransform;
    }
}


/**
 * Test that perpendicular distances from pin to text are preserved across rotations.
 *
 * The Y distance at 0° (horizontal pins) should equal the X distance at 90° (vertical pins).
 * This ensures consistent visual appearance regardless of symbol rotation.
 */
BOOST_AUTO_TEST_CASE( PerpendicularDistanceConsistentAcrossRotations )
{
    // Create test symbol with adjacent pins
    std::unique_ptr<LIB_SYMBOL> symbol = createAdjacentPinsSymbol();
    BOOST_REQUIRE( symbol );

    // Get the first pin for testing
    SCH_PIN* testPin = nullptr;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
        {
            testPin = static_cast<SCH_PIN*>( &item );
            break;
        }
    }

    BOOST_REQUIRE( testPin );

    // Transforms for 0° and 90°
    TRANSFORM transform0( 1, 0, 0, 1 );    // 0° (identity)
    TRANSFORM transform90( 0, -1, 1, 0 );  // 90° CCW

    // Measure perpendicular distance at 0°
    TRANSFORM oldTransform = DefaultTransform;
    DefaultTransform = transform0;

    PIN_LAYOUT_CACHE cache0( *testPin );
    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo0 = cache0.GetPinNameInfo( 0 );
    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfo0 = cache0.GetPinNumberInfo( 0 );

    int nameDistance0 = 0;
    int numberDistance0 = 0;

    if( nameInfo0.has_value() )
        nameDistance0 = getPerpendicularDistance( testPin, nameInfo0->m_TextPosition, transform0 );

    if( numberInfo0.has_value() )
        numberDistance0 = getPerpendicularDistance( testPin, numberInfo0->m_TextPosition, transform0 );

    // Measure perpendicular distance at 90°
    DefaultTransform = transform90;

    PIN_LAYOUT_CACHE cache90( *testPin );
    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo90 = cache90.GetPinNameInfo( 0 );
    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfo90 = cache90.GetPinNumberInfo( 0 );

    int nameDistance90 = 0;
    int numberDistance90 = 0;

    if( nameInfo90.has_value() )
        nameDistance90 = getPerpendicularDistance( testPin, nameInfo90->m_TextPosition, transform90 );

    if( numberInfo90.has_value() )
        numberDistance90 = getPerpendicularDistance( testPin, numberInfo90->m_TextPosition, transform90 );

    // Restore original transform
    DefaultTransform = oldTransform;

    // Allow some tolerance for rounding differences
    const int tolerance = schIUScale.MilsToIU( 5 );  // 5 mils tolerance

    // Check that name distances are consistent
    if( nameInfo0.has_value() && nameInfo90.has_value() )
    {
        int nameDiff = std::abs( nameDistance0 - nameDistance90 );
        BOOST_CHECK_MESSAGE(
                nameDiff <= tolerance,
                "Pin name perpendicular distance changed across rotation. "
                        << "At 0°: " << nameDistance0 << ", at 90°: " << nameDistance90
                        << ", difference: " << nameDiff << " (tolerance: " << tolerance << ")" );
    }

    // Check that number distances are consistent
    if( numberInfo0.has_value() && numberInfo90.has_value() )
    {
        int numberDiff = std::abs( numberDistance0 - numberDistance90 );
        BOOST_CHECK_MESSAGE(
                numberDiff <= tolerance,
                "Pin number perpendicular distance changed across rotation. "
                        << "At 0°: " << numberDistance0 << ", at 90°: " << numberDistance90
                        << ", difference: " << numberDiff << " (tolerance: " << tolerance << ")" );
    }
}


/**
 * Test that name is positioned on opposite side from number (when both are outside).
 *
 * When pin_name_offset is 0 (names outside), the name should be on one side of the
 * pin and the number on the other side to avoid overlap.
 */
BOOST_AUTO_TEST_CASE( NameAndNumberOnOppositeSides )
{
    // Create test symbol with adjacent pins
    std::unique_ptr<LIB_SYMBOL> symbol = createAdjacentPinsSymbol();
    BOOST_REQUIRE( symbol );

    // Get the first pin for testing
    SCH_PIN* testPin = nullptr;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
        {
            testPin = static_cast<SCH_PIN*>( &item );
            break;
        }
    }

    BOOST_REQUIRE( testPin );

    // Test at both 0° and 90°
    std::vector<TRANSFORM>  rotations = { TRANSFORM( 1, 0, 0, 1 ), TRANSFORM( 0, -1, 1, 0 ) };
    std::vector<wxString> rotationNames = { wxT( "0°" ), wxT( "90°" ) };

    for( size_t r = 0; r < rotations.size(); r++ )
    {
        const TRANSFORM& transform = rotations[r];
        const wxString&  rotName = rotationNames[r];

        TRANSFORM oldTransform = DefaultTransform;
        DefaultTransform = transform;

        PIN_LAYOUT_CACHE                           cache( *testPin );
        std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo = cache.GetPinNameInfo( 0 );
        std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfo = cache.GetPinNumberInfo( 0 );

        BOOST_REQUIRE_MESSAGE( nameInfo.has_value(), "Pin should have name at " << rotName );
        BOOST_REQUIRE_MESSAGE( numberInfo.has_value(), "Pin should have number at " << rotName );

        VECTOR2I          pinPos = testPin->GetPosition();
        PIN_ORIENTATION orient = testPin->PinDrawOrient( transform );

        if( orient == PIN_ORIENTATION::PIN_LEFT || orient == PIN_ORIENTATION::PIN_RIGHT )
        {
            // Horizontal pin - name and number should be on opposite Y sides
            bool nameAbove = nameInfo->m_TextPosition.y < pinPos.y;
            bool numberAbove = numberInfo->m_TextPosition.y < pinPos.y;

            BOOST_CHECK_MESSAGE(
                    nameAbove != numberAbove,
                    "At " << rotName << " (horizontal pin): name and number should be on opposite "
                          << "Y sides of pin. Name Y=" << nameInfo->m_TextPosition.y
                          << ", Number Y=" << numberInfo->m_TextPosition.y
                          << ", Pin Y=" << pinPos.y );
        }
        else
        {
            // Vertical pin - name and number should be on opposite X sides
            bool nameLeft = nameInfo->m_TextPosition.x < pinPos.x;
            bool numberLeft = numberInfo->m_TextPosition.x < pinPos.x;

            BOOST_CHECK_MESSAGE(
                    nameLeft != numberLeft,
                    "At " << rotName << " (vertical pin): name and number should be on opposite "
                          << "X sides of pin. Name X=" << nameInfo->m_TextPosition.x
                          << ", Number X=" << numberInfo->m_TextPosition.x
                          << ", Pin X=" << pinPos.x );
        }

        DefaultTransform = oldTransform;
    }
}


BOOST_AUTO_TEST_SUITE_END()
