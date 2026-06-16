/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <boost/test/unit_test.hpp>
#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_item.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

// Issue #24657: a pure field position change must not trip the ERC lib-symbol
// mismatch check, matching the behavior when the field is moved in the schematic.

BOOST_AUTO_TEST_SUITE( ERCLibSymbolFieldPosition )


BOOST_AUTO_TEST_CASE( Issue24657FieldPositionIgnoredForERC )
{
    LIB_SYMBOL original( "R" );

    original.GetValueField().SetText( wxS( "R" ) );
    original.GetValueField().SetTextPos( VECTOR2I( 0, 0 ) );

    // Move only the value field's position, leaving everything else untouched.
    LIB_SYMBOL moved( original );
    moved.GetValueField().SetTextPos( VECTOR2I( 0, 254000 ) );

    constexpr int flags = SCH_ITEM::COMPARE_FLAGS::EQUALITY | SCH_ITEM::COMPARE_FLAGS::ERC;

    int result = original.Compare( moved, flags );

    BOOST_CHECK_MESSAGE( result == 0,
                         "ERC lib-symbol comparison flagged a pure field position change "
                         "(Compare returned " << result << "); expected 0 to match the "
                         "schematic-side move behavior (issue #24657)." );
}


BOOST_AUTO_TEST_SUITE_END()
