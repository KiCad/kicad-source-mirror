/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <wx/debug.h>

#include <base_units.h>
#include <eda_units.h>
#include <qa_utils/wx_utils/wx_assert.h>
#include <widgets/unit_binder.h>

BOOST_AUTO_TEST_SUITE( UnitBinder )


// A distance bound must convert and format exactly as it always has (single unit conversion,
// bare units label), guarding the generalized code against DISTANCE regressions.
BOOST_AUTO_TEST_CASE( RangeBoundDistance )
{
    UNIT_BINDER::RANGE_BOUND bound = UNIT_BINDER::ConvertRangeBound(
            pcbIUScale, EDA_UNITS::MM, 4.0, EDA_UNITS::MM, EDA_DATA_TYPE::DISTANCE );

    BOOST_CHECK_CLOSE( bound.internalUnits, 4.0 * PCB_IU_PER_MM, 1e-9 );
    BOOST_CHECK_EQUAL( bound.displayText, wxString::FromUTF8( "4 mm" ) );
}


// An area bound scales by the square of the unit factor and its label carries the ² exponent.
// The pre-fix code single-converted and formatted as DISTANCE, so both checks failed.
BOOST_AUTO_TEST_CASE( RangeBoundArea )
{
    UNIT_BINDER::RANGE_BOUND bound = UNIT_BINDER::ConvertRangeBound(
            pcbIUScale, EDA_UNITS::MM, 4.0, EDA_UNITS::MM, EDA_DATA_TYPE::AREA );

    BOOST_CHECK_CLOSE( bound.internalUnits, 4.0 * PCB_IU_PER_MM * PCB_IU_PER_MM, 1e-9 );
    BOOST_CHECK_EQUAL( bound.displayText, wxString::FromUTF8( "4 mm\xC2\xB2" ) );
}


// A volume bound scales by the cube of the unit factor and its label carries the ³ exponent.
BOOST_AUTO_TEST_CASE( RangeBoundVolume )
{
    UNIT_BINDER::RANGE_BOUND bound = UNIT_BINDER::ConvertRangeBound(
            pcbIUScale, EDA_UNITS::MM, 2.0, EDA_UNITS::MM, EDA_DATA_TYPE::VOLUME );

    BOOST_CHECK_CLOSE( bound.internalUnits, 2.0 * PCB_IU_PER_MM * PCB_IU_PER_MM * PCB_IU_PER_MM,
                       1e-9 );
    BOOST_CHECK_EQUAL( bound.displayText, wxString::FromUTF8( "2 mm\xC2\xB3" ) );
}


// A unitless bound converts nothing and must not carry a units label; requesting one trips
// GetText's UNIMPLEMENTED_FOR assertion. qa_common does not install the assert thrower
// globally, so arm it here or a regression would assert silently and the string checks
// would still pass.
BOOST_AUTO_TEST_CASE( RangeBoundUnitless )
{
    wxAssertHandler_t prevHandler = wxSetAssertHandler( &KI_TEST::wxAssertThrower );

    UNIT_BINDER::RANGE_BOUND bound{};

    BOOST_CHECK_NO_THROW( bound = UNIT_BINDER::ConvertRangeBound( pcbIUScale, EDA_UNITS::UNSCALED,
                                                                  5.0, EDA_UNITS::UNSCALED,
                                                                  EDA_DATA_TYPE::UNITLESS ) );

    wxSetAssertHandler( prevHandler );

    BOOST_CHECK_CLOSE( bound.internalUnits, 5.0, 1e-9 );
    BOOST_CHECK_EQUAL( bound.displayText, wxString::FromUTF8( "5" ) );
}


BOOST_AUTO_TEST_SUITE_END()
