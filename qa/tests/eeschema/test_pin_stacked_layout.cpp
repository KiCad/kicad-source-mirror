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
 * @file test_pin_stacked_layout.cpp
 * Test pin number layout for stacked multi-line numbers across all rotations
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_pin.h>
#include <lib_symbol.h>
#include <pin_layout_cache.h>
#include <transform.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/log.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( PinStackedLayout )

/**
 * Create a test symbol with stacked pin numbers for rotation testing
 */
static std::unique_ptr<LIB_SYMBOL> createTestResistorSymbol()
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestResistor" ) );

    // Set pin name offset to 0 so names are positioned outside (like numbers)
    symbol->SetPinNameOffset( 0 );

    // Create first pin with stacked numbers [1-5]
    auto pin1 = std::make_unique<SCH_PIN>( symbol.get() );
    pin1->SetPosition( VECTOR2I( 0, schIUScale.MilsToIU( 250 ) ) ); // top pin
    pin1->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
    pin1->SetLength( schIUScale.MilsToIU( 50 ) );
    pin1->SetNumber( wxT( "[1-5]" ) );
    pin1->SetName( wxT( "A" ) ); // Short name
    pin1->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin1->SetUnit( 1 );

    // Create second pin with stacked numbers [6,7,9-11]
    auto pin2 = std::make_unique<SCH_PIN>( symbol.get() );
    pin2->SetPosition( VECTOR2I( 0, schIUScale.MilsToIU( -340 ) ) ); // bottom pin
    pin2->SetOrientation( PIN_ORIENTATION::PIN_UP );
    pin2->SetLength( schIUScale.MilsToIU( 50 ) );
    pin2->SetNumber( wxT( "[6,7,9-11]" ) );
    pin2->SetName( wxT( "B" ) ); // Short name
    pin2->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    pin2->SetUnit( 1 );

    // Add pins to symbol
    symbol->AddDrawItem( pin1.release() );
    symbol->AddDrawItem( pin2.release() );

    return symbol;
}

/**
 * Get pin geometry (line segment from connection point to pin end)
 */
static VECTOR2I getPinLineEnd( const SCH_PIN* pin, const TRANSFORM& transform )
{
    VECTOR2I start = pin->GetPosition();
    VECTOR2I end = start;

    int length = pin->GetLength();

    switch( pin->PinDrawOrient( transform ) )
    {
    case PIN_ORIENTATION::PIN_UP:
        end.y += length;
        break;
    case PIN_ORIENTATION::PIN_DOWN:
        end.y -= length;
        break;
    case PIN_ORIENTATION::PIN_LEFT:
        end.x -= length;
        break;
    case PIN_ORIENTATION::PIN_RIGHT:
        end.x += length;
        break;
    case PIN_ORIENTATION::INHERIT:
    default:
        break;
    }

    return end;
}

/**
 * Check if a box intersects with a line segment
 */
static bool boxIntersectsLine( const BOX2I& box, const VECTOR2I& lineStart, const VECTOR2I& lineEnd )
{
    // Simple bbox vs line segment intersection
    // First check if line bbox intersects text bbox
    BOX2I lineBbox;
    lineBbox.SetOrigin( std::min( lineStart.x, lineEnd.x ), std::min( lineStart.y, lineEnd.y ) );
    lineBbox.SetEnd( std::max( lineStart.x, lineEnd.x ), std::max( lineStart.y, lineEnd.y ) );

    if( !lineBbox.Intersects( box ) )
        return false;

    // For vertical/horizontal lines, do precise check
    if( lineStart.x == lineEnd.x ) // vertical line
    {
        int lineX = lineStart.x;
        return ( lineX >= box.GetLeft() && lineX <= box.GetRight() &&
                 box.GetTop() <= std::max( lineStart.y, lineEnd.y ) &&
                 box.GetBottom() >= std::min( lineStart.y, lineEnd.y ) );
    }
    else if( lineStart.y == lineEnd.y ) // horizontal line
    {
        int lineY = lineStart.y;
        return ( lineY >= box.GetBottom() && lineY <= box.GetTop() &&
                 box.GetLeft() <= std::max( lineStart.x, lineEnd.x ) &&
                 box.GetRight() >= std::min( lineStart.x, lineEnd.x ) );
    }

    // For diagonal lines, use the bbox intersection as approximation
    return true;
}

/**
 * Test that pin numbers don't overlap with pin geometry across all rotations
 */
BOOST_AUTO_TEST_CASE( PinNumbersNoOverlapAllRotations )
{
    // Create test symbol
    std::unique_ptr<LIB_SYMBOL> symbol = createTestResistorSymbol();
    BOOST_REQUIRE( symbol );

    // Get the pins
    std::vector<SCH_PIN*> pins;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            pins.push_back( static_cast<SCH_PIN*>( &item ) );
    }

    BOOST_REQUIRE_EQUAL( pins.size(), 2 );

    // Test rotations: 0°, 90°, 180°, 270°
    std::vector<TRANSFORM> rotations = {
        TRANSFORM( 1, 0, 0, 1 ),   // 0° (identity)
        TRANSFORM( 0, -1, 1, 0 ),  // 90° CCW
        TRANSFORM( -1, 0, 0, -1 ), // 180°
        TRANSFORM( 0, 1, -1, 0 )   // 270° CCW (90° CW)
    };

    std::vector<wxString> rotationNames = { wxT("0°"), wxT("90°"), wxT("180°"), wxT("270°") };

    for( size_t r = 0; r < rotations.size(); r++ )
    {
        const TRANSFORM& transform = rotations[r];
        const wxString& rotName = rotationNames[r];

        // Set global transform for this test
        TRANSFORM oldTransform = DefaultTransform;
        DefaultTransform = transform;

        for( size_t p = 0; p < pins.size(); p++ )
        {
            SCH_PIN* pin = pins[p];

            // Create layout cache for this pin
            PIN_LAYOUT_CACHE cache( *pin );

            // Get pin number text info (shadow width 0 for testing)
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfoOpt = cache.GetPinNumberInfo( 0 );

            if( !numberInfoOpt.has_value() )
                continue;

            const PIN_LAYOUT_CACHE::TEXT_INFO& numberInfo = numberInfoOpt.value();

            if( numberInfo.m_Text.IsEmpty() )
                continue;

            // Get pin line geometry
            VECTOR2I pinStart = pin->GetPosition();
            VECTOR2I pinEnd = getPinLineEnd( pin, transform );

            // Get text bounding box - we need to estimate this since we don't have full font rendering
            // For now, use a simple estimation based on text size and string length
            int textHeight = numberInfo.m_TextSize;
            int textWidth = numberInfo.m_Text.Length() * numberInfo.m_TextSize * 0.6; // rough char width

            // Handle multi-line text
            if( numberInfo.m_Text.Contains( '\n' ) )
            {
                wxArrayString lines;
                wxStringSplit( numberInfo.m_Text, lines, '\n' );

                if( numberInfo.m_Angle == ANGLE_VERTICAL )
                {
                    // For vertical text, lines are spaced horizontally
                    int lineSpacing = textHeight * 1.3;
                    textWidth = lines.size() * lineSpacing;
                    // Find longest line for height
                    size_t maxLen = 0;

                    for( const wxString& line : lines )
                        maxLen = std::max( maxLen, line.Length() );

                    textHeight = maxLen * textHeight * 0.6;
                }
                else
                {
                    // For horizontal text, lines are spaced vertically
                    int lineSpacing = textHeight * 1.3;
                    textHeight = lines.size() * lineSpacing;
                    // Find longest line for width
                    size_t maxLen = 0;

                    for( const wxString& line : lines )
                        maxLen = std::max( maxLen, line.Length() );

                    textWidth = maxLen * textHeight * 0.6;
                }
            }

            // Create text bounding box around text position
            BOX2I textBbox;
            textBbox.SetOrigin( numberInfo.m_TextPosition.x - textWidth/2,
                                numberInfo.m_TextPosition.y - textHeight/2 );
            textBbox.SetSize( textWidth, textHeight );

            // Check for intersection
            bool overlaps = boxIntersectsLine( textBbox, pinStart, pinEnd );

            // Log detailed info for debugging
            wxLogTrace( "KICAD_PINS", wxT("Rotation %s, Pin %s: pos=(%d,%d) textPos=(%d,%d) pinLine=(%d,%d)-(%d,%d) textBox=(%d,%d,%dx%d) overlap=%s"),
                          rotName, pin->GetNumber(),
                          pinStart.x, pinStart.y,
                          numberInfo.m_TextPosition.x, numberInfo.m_TextPosition.y,
                          pinStart.x, pinStart.y, pinEnd.x, pinEnd.y,
                          (int)textBbox.GetLeft(), (int)textBbox.GetTop(), (int)textBbox.GetWidth(), (int)textBbox.GetHeight(),
                          overlaps ? wxT("YES") : wxT("NO") );

            // Test assertion
            BOOST_CHECK_MESSAGE( !overlaps,
                                 "Pin number '" << pin->GetNumber() << "' overlaps with pin geometry at rotation " << rotName );
        }

        // Restore original transform
        DefaultTransform = oldTransform;
    }
}

/**
 * Test that multiline and non-multiline pin numbers/names are positioned consistently
 * with the name above (or left) and the number below (or right) for all rotations.
 */
BOOST_AUTO_TEST_CASE( PinTextConsistentSidePlacement )
{
    // Create test symbol with both types of pins
    std::unique_ptr<LIB_SYMBOL> symbol = createTestResistorSymbol();
    BOOST_REQUIRE( symbol );

    // Get the pins - one will be multiline formatted, one will not
    std::vector<SCH_PIN*> pins;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            pins.push_back( static_cast<SCH_PIN*>( &item ) );
    }

    BOOST_REQUIRE_EQUAL( pins.size(), 2 );

    // Test rotations
    std::vector<TRANSFORM> rotations = {
        TRANSFORM( 1, 0, 0, 1 ),   // 0° (identity)
        TRANSFORM( 0, -1, 1, 0 ),  // 90° CCW
        TRANSFORM( -1, 0, 0, -1 ), // 180°
        TRANSFORM( 0, 1, -1, 0 )   // 270° CCW (90° CW)
    };

    std::vector<wxString> rotationNames = { wxT("0°"), wxT("90°"), wxT("180°"), wxT("270°") };

    for( size_t r = 0; r < rotations.size(); r++ )
    {
        const TRANSFORM& transform = rotations[r];
        const wxString& rotName = rotationNames[r];

        // Set global transform for this test
        TRANSFORM oldTransform = DefaultTransform;
        DefaultTransform = transform;

        // For each rotation, collect pin number and name positions relative to pin center
        struct PinTextInfo {
            VECTOR2I pinPos;
            VECTOR2I numberPos;
            VECTOR2I namePos;
            wxString pinNumber;
            bool isMultiline;
        };

        std::vector<PinTextInfo> pinInfos;

        for( SCH_PIN* pin : pins )
        {
            PinTextInfo info;
            info.pinPos = pin->GetPosition();
            info.pinNumber = pin->GetNumber();

            // Create layout cache for this pin
            PIN_LAYOUT_CACHE cache( *pin );

            // Get number position (shadow width 0 for testing)
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfoOpt = cache.GetPinNumberInfo( 0 );

            if( numberInfoOpt.has_value() )
            {
                const PIN_LAYOUT_CACHE::TEXT_INFO& numberInfo = numberInfoOpt.value();
                info.numberPos = numberInfo.m_TextPosition;
                info.isMultiline = numberInfo.m_Text.Contains( '\n' );
            }

            // Get name position
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfoOpt = cache.GetPinNameInfo( 0 );

            if( nameInfoOpt.has_value() )
            {
                const PIN_LAYOUT_CACHE::TEXT_INFO& nameInfo = nameInfoOpt.value();
                info.namePos = nameInfo.m_TextPosition;
            }

            pinInfos.push_back( info );

            wxLogTrace( "KICAD_PINS", "Rotation %s, Pin %s: pos=(%d,%d) numberPos=(%d,%d) namePos=(%d,%d) multiline=%s",
                        rotName, info.pinNumber,
                        info.pinPos.x, info.pinPos.y,
                        info.numberPos.x, info.numberPos.y,
                        info.namePos.x, info.namePos.y,
                        info.isMultiline ? wxT("YES") : wxT("NO") );
        }

        BOOST_REQUIRE_EQUAL( pinInfos.size(), 2 );

        // New semantics:
        //  * Vertical pins (UP/DOWN): numbers and names must be LEFT (x < pin.x)
        //  * Horizontal pins (LEFT/RIGHT): numbers/names must be ABOVE (y < pin.y)
        PIN_ORIENTATION orient = pins[0]->PinDrawOrient( DefaultTransform );

        if( orient == PIN_ORIENTATION::PIN_UP || orient == PIN_ORIENTATION::PIN_DOWN )
        {
            for( const PinTextInfo& inf : pinInfos )
            {
                BOOST_CHECK_MESSAGE( inf.numberPos.x > inf.pinPos.x,
                    "At rotation " << rotName << ", number for pin " << inf.pinNumber << " not right of vertical pin." );
                BOOST_CHECK_MESSAGE( inf.namePos.x < inf.pinPos.x,
                    "At rotation " << rotName << ", name for pin " << inf.pinNumber << " not left of vertical pin." );
            }
        }
        else if( orient == PIN_ORIENTATION::PIN_LEFT || orient == PIN_ORIENTATION::PIN_RIGHT )
        {
            for( const PinTextInfo& inf : pinInfos )
            {
                BOOST_CHECK_MESSAGE( inf.numberPos.y > inf.pinPos.y,
                    "At rotation " << rotName << ", number for pin " << inf.pinNumber << " not below horizontal pin." );
                BOOST_CHECK_MESSAGE( inf.namePos.y < inf.pinPos.y,
                    "At rotation " << rotName << ", name for pin " << inf.pinNumber << " not above horizontal pin." );
            }
        }

        // Restore original transform
        DefaultTransform = oldTransform;
    }
}

/**
 * Test that multiline and non-multiline pin numbers/names have the same bottom coordinate
 * (distance from pin along the axis connecting pin and text)
 */
BOOST_AUTO_TEST_CASE( PinTextSameBottomCoordinate )
{
    // Create test symbol with both types of pins
    std::unique_ptr<LIB_SYMBOL> symbol = createTestResistorSymbol();
    BOOST_REQUIRE( symbol );

    // Get the pins - one will be multiline formatted, one will not
    std::vector<SCH_PIN*> pins;

    for( SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            pins.push_back( static_cast<SCH_PIN*>( &item ) );
    }

    BOOST_REQUIRE_EQUAL( pins.size(), 2 );

    // Test rotations
    std::vector<TRANSFORM> rotations = {
        TRANSFORM( 1, 0, 0, 1 ),   // 0° (identity)
        TRANSFORM( 0, -1, 1, 0 ),  // 90° CCW
        TRANSFORM( -1, 0, 0, -1 ), // 180°
        TRANSFORM( 0, 1, -1, 0 )   // 270° CCW (90° CW)
    };

    std::vector<wxString> rotationNames = { wxT("0°"), wxT("90°"), wxT("180°"), wxT("270°") };

    for( size_t r = 0; r < rotations.size(); r++ )
    {
        const TRANSFORM& transform = rotations[r];
        const wxString& rotName = rotationNames[r];

        // Set global transform for this test
        TRANSFORM oldTransform = DefaultTransform;
        DefaultTransform = transform;

        // For each rotation, collect pin and text position data
        struct PinTextData {
            VECTOR2I pinPos;
            VECTOR2I numberPos;
            VECTOR2I namePos;
            wxString pinNumber;
            bool isMultiline;
            int numberBottomDistance;
            int nameBottomDistance;
        };

        std::vector<PinTextData> pinData;

        for( SCH_PIN* pin : pins )
        {
            PinTextData data;
            data.pinPos = pin->GetPosition();
            data.pinNumber = pin->GetNumber();

            // Create layout cache for this pin
            PIN_LAYOUT_CACHE cache( *pin );

            // Get number position (shadow width 0 for testing)
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfoOpt = cache.GetPinNumberInfo( 0 );
            PIN_LAYOUT_CACHE::TEXT_INFO numberInfo; // store for later heuristics

            if( numberInfoOpt.has_value() )
            {
                numberInfo = numberInfoOpt.value();
                data.numberPos = numberInfo.m_TextPosition;
                data.isMultiline = numberInfo.m_Text.Contains( '\n' );
            }
            else
            {
                BOOST_FAIL( "Expected pin number text info" );
            }

            // Get name position
            std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfoOpt = cache.GetPinNameInfo( 0 );
            PIN_LAYOUT_CACHE::TEXT_INFO nameInfo; // store for width/height heuristic

            if( nameInfoOpt.has_value() )
            {
                nameInfo = nameInfoOpt.value();
                data.namePos = nameInfo.m_TextPosition;
            }
            else
            {
                BOOST_FAIL( "Expected pin name text info" );
            }

            // Calculate bottom distance (closest distance to pin along pin-text axis)
            PIN_ORIENTATION orient = pin->PinDrawOrient( DefaultTransform );

            if( orient == PIN_ORIENTATION::PIN_UP || orient == PIN_ORIENTATION::PIN_DOWN )
            {
                // Vertical pins: numbers are to the right and should be LEFT-aligned.
                // Calculate the left edge of the number text box.
                int textWidth = data.isMultiline ? 0 : (int)( data.pinNumber.Length() * numberInfo.m_TextSize * 0.6 );

                // (Multiline case: numberInfo.m_Text already contains \n; heuristic in earlier section)
                if( data.isMultiline )
                {
                    wxArrayString lines; wxStringSplit( numberInfo.m_Text, lines, '\n' );
                    int lineSpacing = numberInfo.m_TextSize * 1.3;
                    textWidth = lines.size() * lineSpacing; // when vertical orientation text is rotated
                }

                // Left edge = center - halfWidth
                int leftEdge = data.numberPos.x - textWidth / 2;
                data.numberBottomDistance = leftEdge - data.pinPos.x; // distance from pin to left edge

                // For names (to the left of the pin), measure right edge
                int nameWidth = (int)( nameInfo.m_Text.Length() * nameInfo.m_TextSize * 0.6 );
                int nameRightEdge = data.namePos.x + nameWidth / 2;
                data.nameBottomDistance = data.pinPos.x - nameRightEdge; // distance from name right edge to pin
            }
            else
            {
                // Horizontal pins: numbers are below the pin and should be TOP-aligned.
                // Calculate the top edge of the number text box.
                int textHeight = data.isMultiline ? 0 : numberInfo.m_TextSize;

                if( data.isMultiline )
                {
                    wxArrayString lines;
                    wxStringSplit( numberInfo.m_Text, lines, '\n' );
                    int lineSpacing = numberInfo.m_TextSize * 1.3;
                    textHeight = lines.size() * lineSpacing;
                }

                // Top edge = center - halfHeight
                int topEdge = data.numberPos.y - textHeight / 2;
                data.numberBottomDistance = topEdge - data.pinPos.y; // distance from pin to top edge

                // For names (above the pin), measure bottom edge
                int nameHeight = nameInfo.m_TextSize;
                int nameBottomEdge = data.namePos.y + nameHeight / 2;
                data.nameBottomDistance = data.pinPos.y - nameBottomEdge; // distance from name bottom to pin
            }

            pinData.push_back( data );

            wxLogTrace( "KICAD_PINS", "Rotation %s, Pin %s: pos=(%d,%d) numberPos=(%d,%d) namePos=(%d,%d) multiline=%s numberBottomDist=%d nameBottomDist=%d",
                        rotName, data.pinNumber,
                        data.pinPos.x, data.pinPos.y,
                        data.numberPos.x, data.numberPos.y,
                        data.namePos.x, data.namePos.y,
                        data.isMultiline ? wxT("YES") : wxT("NO"),
                        data.numberBottomDistance, data.nameBottomDistance );
        }

        BOOST_REQUIRE_EQUAL( pinData.size(), 2 );

        // Check that both pins have their numbers at the same bottom distance from pin
        // Allow small tolerance for rounding differences
        const int tolerance = 100; // 100 internal units tolerance

        int bottomDist1 = pinData[0].numberBottomDistance;
        int bottomDist2 = pinData[1].numberBottomDistance;
        int distanceDiff = abs( bottomDist1 - bottomDist2 );

        BOOST_CHECK_MESSAGE( distanceDiff <= tolerance,
                           "At rotation " << rotName << ", pin numbers have different bottom distances from pin. "
                           << "Pin " << pinData[0].pinNumber << " distance=" << bottomDist1
                           << ", Pin " << pinData[1].pinNumber << " distance=" << bottomDist2
                           << ", difference=" << distanceDiff << " (tolerance=" << tolerance << ")" );

        // Check that both pins have their names at the same bottom distance from pin
        int nameBottomDist1 = pinData[0].nameBottomDistance;
        int nameBottomDist2 = pinData[1].nameBottomDistance;
        int nameDistanceDiff = abs( nameBottomDist1 - nameBottomDist2 );

        BOOST_CHECK_MESSAGE( nameDistanceDiff <= tolerance,
                           "At rotation " << rotName << ", pin names have different bottom distances from pin. "
                           << "Pin " << pinData[0].pinNumber << " name distance=" << nameBottomDist1
                           << ", Pin " << pinData[1].pinNumber << " name distance=" << nameBottomDist2
                           << ", difference=" << nameDistanceDiff << " (tolerance=" << tolerance << ")" );

        // Restore original transform
        DefaultTransform = oldTransform;
    }
}

BOOST_AUTO_TEST_SUITE_END()
