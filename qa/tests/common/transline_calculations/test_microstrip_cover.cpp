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

#include <cmath>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <transline_calculations/microstrip.h>
#include <transline_calculations/transline_calculation_base.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{

// Reference geometry for the covered-microstrip tests.  Substrate eps_r = 4.4, h = 1 mm,
// W = 1.875 mm (u = W/h = 1.875), T = 35 um (1 oz copper).  Driving f at 1 MHz keeps the
// Kirschning-Jansen dispersion term negligible (f_n = FREQ*H/1e6 << 1) so Z0 and eps_eff
// match the static values computed directly from the closed-form expressions.
constexpr double FIXTURE_EPS_R = 4.4;
constexpr double FIXTURE_TAND = 0.02;
constexpr double FIXTURE_H = 1.0 * TC::UNIT_MM;
constexpr double FIXTURE_W = 1.875 * TC::UNIT_MM;
constexpr double FIXTURE_T = 35.0 * TC::UNIT_MICRON;
constexpr double FIXTURE_FREQ = 1.0 * TC::UNIT_MHZ;
constexpr double FIXTURE_SIGMA = 5.8e7;
constexpr double OPEN_COVER = 1.0e20;


MICROSTRIP MakeFixture( double aHTop )
{
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, FIXTURE_EPS_R );
    calc.SetParameter( TCP::TAND, FIXTURE_TAND );
    calc.SetParameter( TCP::H, FIXTURE_H );
    calc.SetParameter( TCP::H_T, aHTop );
    calc.SetParameter( TCP::PHYS_WIDTH, FIXTURE_W );
    calc.SetParameter( TCP::T, FIXTURE_T );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, FIXTURE_SIGMA );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::FREQUENCY, FIXTURE_FREQ );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    return calc;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MicrostripCover )


// Baseline without a cover reproduces the well-known open-microstrip impedance.  Reference
// Z0 ~ 49.6 ohm, eps_eff ~ 3.33 for u = 1.875, eps_r = 4.4, T/h = 0.035.
BOOST_AUTO_TEST_CASE( NoCover )
{
    MICROSTRIP calc = MakeFixture( OPEN_COVER );
    calc.Analyse();

    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::Z0 ), 49.6, 0.03 );
    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::EPSILON_EFF ), 3.33, 0.03 );
}


BOOST_AUTO_TEST_CASE( CoverAtTwoH )
{
    MICROSTRIP calc = MakeFixture( 2.0 * FIXTURE_H );
    calc.Analyse();

    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::Z0 ), 47.3, 0.03 );
    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::EPSILON_EFF ), 3.12, 0.03 );
}


// Cover at H_T = h (tight shielding, air gap equal to substrate thickness).  h'/h = 1 is
// the lower boundary of GBB Eq. (2.128) stated validity (1 < h'/h < infty) so we accept a
// 5 percent Z0 tolerance.  checkProperties also flags this regime in the UI via the
// T + 0.1*h air-gap warning.
BOOST_AUTO_TEST_CASE( CoverAtH )
{
    MICROSTRIP calc = MakeFixture( FIXTURE_H );
    calc.Analyse();

    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::Z0 ), 41.2, 0.05 );
    BOOST_CHECK_CLOSE_FRACTION( calc.GetParameter( TCP::EPSILON_EFF ), 2.70, 0.03 );
}


// At H_T = 10h the cover contribution is already within a fraction of a percent of the
// open case.  Verifies the asymptotic behaviour of the cover correction as h'/h -> infty.
BOOST_AUTO_TEST_CASE( CoverAt10HIsAsymptoticallyOpen )
{
    MICROSTRIP open = MakeFixture( OPEN_COVER );
    open.Analyse();
    const double openZ0 = open.GetParameter( TCP::Z0 );
    const double openEps = open.GetParameter( TCP::EPSILON_EFF );

    MICROSTRIP tenH = MakeFixture( 10.0 * FIXTURE_H );
    tenH.Analyse();

    BOOST_CHECK_CLOSE_FRACTION( tenH.GetParameter( TCP::Z0 ), openZ0, 0.01 );
    BOOST_CHECK_CLOSE_FRACTION( tenH.GetParameter( TCP::EPSILON_EFF ), openEps, 0.01 );
}


// A lower cover must produce a lower static impedance because the added parallel
// capacitance to the cover plane can only increase line capacitance.  Verified only in
// the regime where GBB's P is non-negligible; at h'/h >= 10 the P correction is below
// 0.1 percent of Z0 and rides the floor of the independent filling-factor fit's accuracy,
// so we do not assert strict ordering between open and 10h.
BOOST_AUTO_TEST_CASE( Z0MonotonicInH_T )
{
    MICROSTRIP twoH = MakeFixture( 2.0 * FIXTURE_H );
    twoH.Analyse();

    MICROSTRIP oneH = MakeFixture( FIXTURE_H );
    oneH.Analyse();

    BOOST_TEST( twoH.GetParameter( TCP::Z0 ) > oneH.GetParameter( TCP::Z0 ) );
}


// Exercises the out-of-validity fallback in delta_Z0_cover: wide trace + low cover
// saturates the Q argument, so the P-only branch keeps Z0 finite.
BOOST_AUTO_TEST_CASE( WideTraceWithCloseCoverDoesNotProduceNaN )
{
    MICROSTRIP calc = MakeFixture( 2.0 * FIXTURE_H );
    calc.SetParameter( TCP::PHYS_WIDTH, 10.0 * FIXTURE_H );
    calc.Analyse();

    BOOST_TEST( std::isfinite( calc.GetParameter( TCP::Z0 ) ) );
    BOOST_TEST( std::isfinite( calc.GetParameter( TCP::EPSILON_EFF ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
