/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue24972_sheet_pin_constrain.cpp
 *
 * Regression test for issue #24972: a sheet pin driven through CalcEdit (the
 * interactive placement path) must stay clamped to the sheet border.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_sheet.h>
#include <sch_sheet_pin.h>

BOOST_AUTO_TEST_SUITE( Issue24972SheetPinConstrain )


static bool onSheetBorder( const VECTOR2I& aPos, const SCH_SHEET& aSheet )
{
    int left = aSheet.GetPosition().x;
    int right = aSheet.GetPosition().x + aSheet.GetSize().x;
    int top = aSheet.GetPosition().y;
    int bot = aSheet.GetPosition().y + aSheet.GetSize().y;

    bool onVertical = ( aPos.x == left || aPos.x == right ) && aPos.y >= top && aPos.y <= bot;
    bool onHorizontal = ( aPos.y == top || aPos.y == bot ) && aPos.x >= left && aPos.x <= right;

    return onVertical || onHorizontal;
}


BOOST_AUTO_TEST_CASE( CalcEditClampsToBorder )
{
    SCH_SHEET sheet;
    sheet.SetSize( VECTOR2I( 1000, 1000 ) );

    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( &sheet, VECTOR2I( 0, 500 ), "P1" );
    sheet.AddPin( pin );

    pin->CalcEdit( VECTOR2I( 500, 5000 ) );
    BOOST_CHECK( onSheetBorder( pin->GetPosition(), sheet ) );
    BOOST_CHECK_EQUAL( pin->GetPosition(), VECTOR2I( 500, 1000 ) );

    pin->CalcEdit( VECTOR2I( 5000, 300 ) );
    BOOST_CHECK( onSheetBorder( pin->GetPosition(), sheet ) );
    BOOST_CHECK_EQUAL( pin->GetPosition(), VECTOR2I( 1000, 300 ) );

    pin->CalcEdit( VECTOR2I( -5000, -5000 ) );
    BOOST_CHECK( onSheetBorder( pin->GetPosition(), sheet ) );
}


BOOST_AUTO_TEST_SUITE_END()
