/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_pin_number_default_style_clearance.cpp
 * When a pin's name and number are both shown outside (name offset 0) with equal text
 * sizes, they are placed on opposite sides of the pin at what should be the same distance.
 * The number side previously measured its clearance from the font's inflated glyph bbox
 * instead of the nominal text size, pushing the number noticeably further from the pin
 * than the name and shifting pin numbers up across every symbol using a stroke font.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_pin.h>
#include <lib_symbol.h>
#include <pin_layout_cache.h>
#include <transform.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( PinNumberDefaultStyleClearance )

BOOST_AUTO_TEST_CASE( NumberClearanceMatchesNameClearance )
{
    auto symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestIC" ) );

    // Name offset 0 puts both name and number outside the pin, on opposite sides
    symbol->SetPinNameOffset( 0 );
    symbol->SetShowPinNames( true );
    symbol->SetShowPinNumbers( true );

    auto pinOwner = std::make_unique<SCH_PIN>( symbol.get() );
    pinOwner->SetPosition( VECTOR2I( 0, 0 ) );
    pinOwner->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
    pinOwner->SetLength( schIUScale.MilsToIU( 200 ) );
    pinOwner->SetName( wxT( "SDI" ) );
    pinOwner->SetNumber( wxT( "14" ) );
    pinOwner->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
    pinOwner->SetUnit( 1 );

    SCH_PIN* pin = pinOwner.get();
    symbol->AddDrawItem( pinOwner.release() );

    PIN_LAYOUT_CACHE cache( *pin );

    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> nameInfo = cache.GetPinNameInfo( 0 );
    std::optional<PIN_LAYOUT_CACHE::TEXT_INFO> numberInfo = cache.GetPinNumberInfo( 0 );

    BOOST_REQUIRE( nameInfo );
    BOOST_REQUIRE( numberInfo );

    int pinY = pin->GetPosition().y;
    int nameClearance = std::abs( nameInfo->m_TextPosition.y - pinY );
    int numberClearance = std::abs( numberInfo->m_TextPosition.y - pinY );

    // Both use the same 50 mil default text size, so with the same layout formula they
    // should sit at the same distance from the pin. A couple of IU covers int rounding.
    const int tolerance = 2;

    BOOST_CHECK_MESSAGE( std::abs( nameClearance - numberClearance ) <= tolerance,
                         "Pin number clearance (" << numberClearance
                                << " IU) does not match pin name clearance (" << nameClearance
                                << " IU) even though both use the default 50 mil text size" );
}

BOOST_AUTO_TEST_SUITE_END()
