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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <transline_calculations/microstrip.h>
#include <transline_calculations/transline_calculation_base.h>


using TCP = TRANSLINE_PARAMETERS;


BOOST_AUTO_TEST_SUITE( DielectricSelector )


// The default DIELECTRIC_MODEL_SEL value is 0.0 which maps to CONSTANT.  Calling
// UpdateDielectricModel() must leave the DS state empty and the dispersed accessors
// must return the raw EPSILONR / TAND parameters verbatim.
BOOST_AUTO_TEST_CASE( ConstantDefault )
{
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );

    calc.UpdateDielectricModel();

    BOOST_TEST( calc.GetDispersedEpsilonR( 1.0e9 ) == calc.GetParameter( TCP::EPSILONR ) );
    BOOST_TEST( calc.GetDispersedTanDelta( 1.0e9 ) == calc.GetParameter( TCP::TAND ) );
}


// Switching DIELECTRIC_MODEL_SEL to DJORDJEVIC_SARKAR with a valid spec frequency must
// populate the DS model and disperse the permittivity per Djordjevic 2001.  Reference
// value (4.7872 at 1 MHz for FR-4 specified at 1 GHz) matches the DS unit test.
BOOST_AUTO_TEST_CASE( DjordjevicSarkarActivation )
{
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::EPSILONR_SPEC_FREQ, 1.0e9 );
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );

    calc.UpdateDielectricModel();

    BOOST_TEST( calc.GetDispersedEpsilonR( 1.0e6 ) == 4.7872, boost::test_tools::tolerance( 0.01 ) );
    BOOST_TEST( calc.GetDispersedTanDelta( 1.0e6 ) == 0.01838,
                boost::test_tools::tolerance( 0.0005 ) );
}


// A non-positive EPSILONR_SPEC_FREQ is an ill-formed user input.  UpdateDielectricModel
// must reject it and fall back to CONSTANT behaviour rather than throwing from Fit().
BOOST_AUTO_TEST_CASE( InvalidSpecFrequencyFallsBack )
{
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::EPSILONR_SPEC_FREQ, 0.0 );
    calc.SetParameter( TCP::DIELECTRIC_MODEL_SEL,
                       static_cast<double>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) );

    calc.UpdateDielectricModel();

    BOOST_TEST( calc.GetDispersedEpsilonR( 1.0e6 ) == 4.4 );
    BOOST_TEST( calc.GetDispersedTanDelta( 1.0e6 ) == 0.02 );
}


BOOST_AUTO_TEST_SUITE_END()
