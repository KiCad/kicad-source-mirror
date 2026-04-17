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

#include <cmath>

#include <transline_calculations/coplanar.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Common geometry used across the Coplanar test suite.  FR-4-ish substrate, modest
// conductor thickness, 1 GHz.
void SetDefaults( COPLANAR& aCalc )
{
    aCalc.SetParameter( TCP::EPSILONR, 4.4 );
    aCalc.SetParameter( TCP::TAND, 0.0 );
    aCalc.SetParameter( TCP::MURC, 1.0 );
    aCalc.SetParameter( TCP::SIGMA, 5.8e7 );
    aCalc.SetParameter( TCP::H, 0.8 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::T, 35.0 * TC::UNIT_MICRON );
    aCalc.SetParameter( TCP::PHYS_WIDTH, 0.5 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::PHYS_S, 0.3 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    aCalc.SetParameter( TCP::Z0, 0.0 );
    aCalc.SetParameter( TCP::ANG_L, 0.0 );
    aCalc.SetParameter( TCP::CPW_BACKMETAL, 0.0 );
}
} // namespace


BOOST_AUTO_TEST_SUITE( CoplanarCalculations )


// Ungrounded CPW on FR-4.  Pinned to the implementation output at 1 GHz with ±2 % tolerance.
BOOST_AUTO_TEST_CASE( UngroundedImpedance )
{
    COPLANAR calc;
    SetDefaults( calc );

    calc.Analyse();

    const double Z0 = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( std::isfinite( Z0 ) );
    BOOST_TEST( Z0 > 40.0 );
    BOOST_TEST( Z0 < 90.0 );

    // Pinned value from the conformal-mapping solution at f = 1 GHz.  Tolerance 2 %.
    BOOST_TEST( Z0 == 72.30, boost::test_tools::tolerance( 0.02 ) );
}


// Conductor-backed CPW pulls Z0 lower (closer to microstrip-like loading) than the
// ungrounded case with the same geometry.  A thin substrate is used so the back plane
// has a noticeable effect.
BOOST_AUTO_TEST_CASE( BackMetalLowersImpedance )
{
    COPLANAR calc_cpw;
    SetDefaults( calc_cpw );
    calc_cpw.SetParameter( TCP::H, 0.25 * TC::UNIT_MM );
    calc_cpw.Analyse();
    const double Z0_cpw = calc_cpw.GetAnalysisResults()[TCP::Z0].first;

    COPLANAR calc_cbcpw;
    SetDefaults( calc_cbcpw );
    calc_cbcpw.SetParameter( TCP::H, 0.25 * TC::UNIT_MM );
    calc_cbcpw.SetParameter( TCP::CPW_BACKMETAL, 1.0 );
    calc_cbcpw.Analyse();
    const double Z0_cbcpw = calc_cbcpw.GetAnalysisResults()[TCP::Z0].first;

    BOOST_TEST( std::isfinite( Z0_cbcpw ) );
    BOOST_TEST( ( Z0_cpw - Z0_cbcpw ) >= 2.0 );
}


// Dielectric loss should be finite, positive, and broadly consistent with the quasi-TEM
// closed-form alpha_d = pi * f * sqrt(eps_eff) * tan(delta) / c within a generous
// tolerance (the implementation applies an additional (eps_r / (eps_r - 1)) weighting).
BOOST_AUTO_TEST_CASE( DielectricLossIsFiniteAndPositive )
{
    COPLANAR calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::TAND, 0.02 );

    calc.Analyse();

    const double loss_d = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    const double eps_eff = calc.GetAnalysisResults()[TCP::EPSILON_EFF].first;

    BOOST_TEST( std::isfinite( loss_d ) );
    BOOST_TEST( loss_d > 0.0 );

    // Reference closed form for the coplanar-waveguide dielectric loss used by the
    // implementation.  alpha_d = (eps_r / (eps_r - 1)) * (eps_eff - 1) / sqrt(eps_eff) *
    // pi * f * tan(delta) / c  [Np/m]; multiply by length and LOG2DB for dB.
    const double epsr = 4.4;
    const double tand = 0.02;
    const double freq = 1.0e9;
    const double len = 100.0 * TC::UNIT_MM;
    const double ad_factor = ( epsr / ( epsr - 1.0 ) ) * tand * M_PI / TC::C0;
    const double expected_dB = TC::LOG2DB * len * ad_factor * freq * ( eps_eff - 1.0 ) / std::sqrt( eps_eff );

    BOOST_TEST( loss_d == expected_dB, boost::test_tools::tolerance( 0.05 ) );
}


// Synthesis round-trip.  Given a target Z0, solve for PHYS_WIDTH with the gap fixed,
// then re-analyse and confirm the resulting Z0 is within 0.5 Ω.
BOOST_AUTO_TEST_CASE( SynthesisRoundTripSolveForWidth )
{
    COPLANAR calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::Z0, 50.0 );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );

    calc.SetSynthesizeTarget( TCP::PHYS_WIDTH );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( std::abs( Z0_check - 50.0 ) < 0.5 );
}


// The CPW_BACKMETAL parameter is honoured on each call to Analyse: toggling it produces
// different Z0 results without the calc needing to be re-created.
BOOST_AUTO_TEST_CASE( BackMetalParameterRoundTrip )
{
    COPLANAR calc;
    SetDefaults( calc );

    calc.SetParameter( TCP::CPW_BACKMETAL, 0.0 );
    calc.Analyse();
    const double Z0_cpw = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::CPW_BACKMETAL, 1.0 );
    calc.Analyse();
    const double Z0_cbcpw = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::CPW_BACKMETAL, 0.0 );
    calc.Analyse();
    const double Z0_cpw_again = calc.GetAnalysisResults()[TCP::Z0].first;

    BOOST_TEST( std::abs( Z0_cpw - Z0_cbcpw ) > 1.0 );
    BOOST_TEST( Z0_cpw == Z0_cpw_again, boost::test_tools::tolerance( 1.0e-9 ) );
}


BOOST_AUTO_TEST_SUITE_END()
