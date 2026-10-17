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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcb_dimension.h>

BOOST_AUTO_TEST_CASE( DimensionZeroSuppressionPreservesIntegerMagnitude )
{
    PCB_DIM_ALIGNED dimension( nullptr );
    dimension.SetUnits( EDA_UNITS::MM );
    dimension.SetSuppressZeroes( true );

    for( DIM_PRECISION precision : { DIM_PRECISION::X, DIM_PRECISION::X_XXX } )
    {
        dimension.SetPrecision( precision );

        for( int value : { 0, 10, 100, 1000 } )
        {
            dimension.SetMeasuredValue( pcbIUScale.mmToIU( value ) );
            BOOST_CHECK_EQUAL( dimension.GetValueText(), wxString::Format( "%d", value ) );
        }
    }

    dimension.SetPrecision( DIM_PRECISION::X_XXX );
    dimension.SetMeasuredValue( pcbIUScale.mmToIU( 10.25 ) );
    BOOST_CHECK_EQUAL( dimension.GetValueText(), "10.25" );
    dimension.SetSuppressZeroes( false );
    BOOST_CHECK_EQUAL( dimension.GetValueText(), "10.250" );

    dimension.SetUnits( EDA_UNITS::MILS );
    dimension.SetPrecision( DIM_PRECISION::V_VV );
    dimension.SetSuppressZeroes( true );
    dimension.SetMeasuredValue( pcbIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( dimension.GetValueText(), "10" );
}
