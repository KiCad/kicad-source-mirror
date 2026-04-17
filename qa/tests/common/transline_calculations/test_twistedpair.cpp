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

#include <transline_calculations/twistedpair.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Cat5e-style polyethylene-jacketed copper pair.  AWG 24 conductor (0.511 mm) under a
// 0.245 mm PE sheath (Dout = 1.0 mm), 49 twists/m.  Individual tests override twist,
// dielectric, and frequency.
void SetCat5eLike( TWISTEDPAIR& aCalc )
{
    aCalc.SetParameter( TCP::EPSILONR, 2.3 );
    aCalc.SetParameter( TCP::TAND, 0.0 );
    aCalc.SetParameter( TCP::SIGMA, 5.8e7 );
    aCalc.SetParameter( TCP::MURC, 1.0 );
    aCalc.SetParameter( TCP::PHYS_DIAM_IN, 0.511 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::PHYS_DIAM_OUT, 1.000 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::PHYS_LEN, 1.000 * TC::UNIT_M );
    aCalc.SetParameter( TCP::FREQUENCY, 100.0 * TC::UNIT_MHZ );
    aCalc.SetParameter( TCP::Z0, 0.0 );
    aCalc.SetParameter( TCP::ANG_L, 0.0 );
    aCalc.SetParameter( TCP::TWISTEDPAIR_TWIST, 49.0 );
    aCalc.SetParameter( TCP::TWISTEDPAIR_EPSILONR_ENV, 1.0 );
}
} // namespace


BOOST_AUTO_TEST_SUITE( TwistedPairCalculations )


// Cat5e-geometry pair using the Lefferson 1971 model with theta in degrees.  At 49
// twists/m and a 1.0 mm outer diameter the pitch angle is only ~8.75 deg, so the
// twist raises epsEff from 1.325 to ~1.395 and drops Z0 from ~134.5 to ~131.2.
// Real Cat5e reaches 100 ohm through tighter pair pitch and thinner insulation than
// what this reference geometry exercises.  We pin the math-core value so future
// Lefferson refactors are caught immediately.
BOOST_AUTO_TEST_CASE( Cat5eReferenceAnalysis )
{
    TWISTEDPAIR calc;
    SetCat5eLike( calc );

    calc.Analyse();

    auto& results = calc.GetAnalysisResults();

    const double Z0 = results.at(TCP::Z0).first;
    BOOST_TEST( std::isfinite( Z0 ) );
    BOOST_TEST( Z0 > 0.0 );
    BOOST_TEST( Z0 == 131.16, boost::test_tools::tolerance( 0.01 ) );

    const double epsEff = results.at(TCP::EPSILON_EFF).first;
    BOOST_TEST( epsEff == 1.3947, boost::test_tools::tolerance( 0.01 ) );
}


// Air-spaced parallel wires with T=0 collapse to the untwisted TEM two-wire line and must
// match (ZF0/π)·acosh(Dout/Din) exactly.
BOOST_AUTO_TEST_CASE( DegenerateAirSpacedParallelWires )
{
    TWISTEDPAIR calc;
    SetCat5eLike( calc );
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::TWISTEDPAIR_EPSILONR_ENV, 1.0 );
    calc.SetParameter( TCP::TWISTEDPAIR_TWIST, 0.0 );

    calc.Analyse();

    auto& results = calc.GetAnalysisResults();

    const double Din = 0.511e-3;
    const double Dout = 1.000e-3;
    const double expected = ( TC::ZF0 / M_PI ) * std::acosh( Dout / Din );

    BOOST_TEST( results.at(TCP::Z0).first == expected, boost::test_tools::tolerance( 1e-9 ) );
    BOOST_TEST( results.at(TCP::EPSILON_EFF).first == 1.0, boost::test_tools::tolerance( 1e-12 ) );
}


// Dielectric loss scales linearly with tan δ.  Zero tan δ gives zero dielectric loss.
BOOST_AUTO_TEST_CASE( DielectricLossScalesWithTanDelta )
{
    TWISTEDPAIR calc;
    SetCat5eLike( calc );
    calc.SetParameter( TCP::TAND, 0.0 );

    calc.Analyse();
    const double loss_zero = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    BOOST_TEST( loss_zero == 0.0, boost::test_tools::tolerance( 1e-12 ) );

    calc.SetParameter( TCP::TAND, 0.001 );
    calc.Analyse();
    const double loss_lo = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    BOOST_TEST( std::isfinite( loss_lo ) );
    BOOST_TEST( loss_lo > 0.0 );

    // Double tan δ → loss_d doubles (α_d is linear in tan δ).
    calc.SetParameter( TCP::TAND, 0.002 );
    calc.Analyse();
    const double loss_hi = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    BOOST_TEST( loss_hi == 2.0 * loss_lo, boost::test_tools::tolerance( 1e-9 ) );
}


// Round-trip synthesis.  Set Z0 target, solve for Dout, re-analyse, confirm Z0 hits.
BOOST_AUTO_TEST_CASE( SynthesisRoundTripSolveForDout )
{
    TWISTEDPAIR calc;
    SetCat5eLike( calc );

    // Ask for a 100 ohm pair.  At 1.0 mm Dout the model sits near 131 ohm, so the
    // solver will shrink Dout (expect ~0.78 mm) to hit the target.
    constexpr double Z0_target = 100.0;
    calc.SetParameter( TCP::Z0, Z0_target );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );

    calc.SetSynthesizeTarget( TCP::PHYS_DIAM_OUT );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double Dout_synth = calc.GetParameter( TCP::PHYS_DIAM_OUT );
    BOOST_TEST( std::isfinite( Dout_synth ) );
    BOOST_TEST( Dout_synth > 0.0 );

    // Re-analyse and verify Z0 is within 1 Ω of the 100 Ω target.
    calc.SetParameter( TCP::Z0, 0.0 );
    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_target, boost::test_tools::tolerance( 1.0 / Z0_target ) );
}


// Increasing the twist count raises the pitch angle, which raises the twist weight
// on (epsr - epsr_env) and therefore raises epsEff while lowering Z0.  Monotonic in
// both directions for epsr > epsr_env.
BOOST_AUTO_TEST_CASE( TwistRaisesEpsEffLowersZ0 )
{
    TWISTEDPAIR calc;
    SetCat5eLike( calc );

    calc.SetParameter( TCP::TWISTEDPAIR_TWIST, 0.0 );
    calc.Analyse();
    const double eps0 = calc.GetAnalysisResults()[TCP::EPSILON_EFF].first;
    const double z0_0 = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::TWISTEDPAIR_TWIST, 49.0 );
    calc.Analyse();
    const double eps49 = calc.GetAnalysisResults()[TCP::EPSILON_EFF].first;
    const double z0_49 = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::TWISTEDPAIR_TWIST, 1000.0 );
    calc.Analyse();
    const double eps1000 = calc.GetAnalysisResults()[TCP::EPSILON_EFF].first;
    const double z0_1000 = calc.GetAnalysisResults()[TCP::Z0].first;

    BOOST_TEST( eps49 > eps0 );
    BOOST_TEST( eps1000 > eps49 );
    BOOST_TEST( z0_49 < z0_0 );
    BOOST_TEST( z0_1000 < z0_49 );
}


BOOST_AUTO_TEST_SUITE_END()
