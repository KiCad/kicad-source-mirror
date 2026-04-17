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

#include <transline_calculations/rectwaveguide.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// WR-90 geometry and copper-on-air defaults.  Individual tests override FREQUENCY and,
// where relevant, EPSILONR / TAND.
void SetWR90( RECTWAVEGUIDE& aCalc )
{
    aCalc.SetParameter( TCP::EPSILONR, 1.0 );
    aCalc.SetParameter( TCP::TAND, 0.0 );
    aCalc.SetParameter( TCP::SIGMA, 5.8e7 );
    aCalc.SetParameter( TCP::MUR, 1.0 );
    aCalc.SetParameter( TCP::MURC, 1.0 );
    aCalc.SetParameter( TCP::PHYS_WIDTH, 22.86 * TC::UNIT_MM ); // a = 0.9"
    aCalc.SetParameter( TCP::PHYS_S, 10.16 * TC::UNIT_MM );    // b = 0.4"
    aCalc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    aCalc.SetParameter( TCP::Z0, 0.0 );
    aCalc.SetParameter( TCP::ANG_L, 0.0 );
}
} // namespace


BOOST_AUTO_TEST_SUITE( RectWaveguideCalculations )


// WR-90 air-filled at 10 GHz.  TE10 cutoff c/(2a) ≈ 6.557 GHz.  Fictive-voltage Z0 ≈ 499.4 Ω.
// Conductor loss on copper per the Ramo/Whinnery/Van Duzer summation is ~0.11 dB/m; allow
// ±30 % because the task pins an approximate value, not a specific textbook cell.
BOOST_AUTO_TEST_CASE( WR90AirFilledReference )
{
    RECTWAVEGUIDE calc;
    SetWR90( calc );

    calc.Analyse();

    auto& results = calc.GetAnalysisResults();

    const double fc = results.at(TCP::CUTOFF_FREQUENCY).first;
    BOOST_TEST( fc == 6.557e9, boost::test_tools::tolerance( 0.005 ) );

    const double Z0 = results.at(TCP::Z0).first;
    BOOST_TEST( Z0 == 500.0, boost::test_tools::tolerance( 20.0 / 500.0 ) );

    // 100 mm of WR-90 on copper: α_c * L should land near 11 mdB (0.11 dB/m * 0.1 m).
    const double loss_c = results.at(TCP::LOSS_CONDUCTOR).first;
    BOOST_TEST( std::isfinite( loss_c ) );
    BOOST_TEST( loss_c > 0.0 );
    BOOST_TEST( loss_c == 0.011, boost::test_tools::tolerance( 0.30 ) );
}


// Below TE10 cutoff: TE mode string empty, TM string empty.
// Above TE10 cutoff: TE string starts with the dominant "H(1,0) " token.
BOOST_AUTO_TEST_CASE( TE10ModeString )
{
    RECTWAVEGUIDE calc;
    SetWR90( calc );

    // f = 4 GHz is below TE10 cutoff (~6.56 GHz).  Both mode strings must be empty.
    calc.SetParameter( TCP::FREQUENCY, 4.0e9 );
    calc.Analyse();
    BOOST_TEST( calc.GetTEModes() == std::string( "" ) );
    BOOST_TEST( calc.GetTMModes() == std::string( "" ) );

    // f = 10 GHz propagates TE10 only.  Legacy format: "H(m,n) " with the dominant mode
    // listed first (m-major order starting at m=0).
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    calc.Analyse();
    const std::string teModes = calc.GetTEModes();
    BOOST_TEST( teModes.rfind( "H(1,0) ", 0 ) == 0u );
    BOOST_TEST( calc.GetTMModes() == std::string( "" ) );
}


// TE10 dielectric-filled waveguide, lossy dielectric.  α_d must be finite, positive, and
// scale monotonically with tan δ (doubling tan δ doubles α_d for fixed k and beta).
BOOST_AUTO_TEST_CASE( DielectricLossIsFiniteAndPositive )
{
    RECTWAVEGUIDE calc;
    SetWR90( calc );
    calc.SetParameter( TCP::EPSILONR, 2.1 );
    calc.SetParameter( TCP::TAND, 0.001 );
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );

    calc.Analyse();

    const double loss_d_lo = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    BOOST_TEST( std::isfinite( loss_d_lo ) );
    BOOST_TEST( loss_d_lo > 0.0 );

    // Double tan δ → loss_d doubles (α_d is linear in tan δ).
    calc.SetParameter( TCP::TAND, 0.002 );
    calc.Analyse();
    const double loss_d_hi = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;
    BOOST_TEST( loss_d_hi == 2.0 * loss_d_lo, boost::test_tools::tolerance( 1e-9 ) );
}


// Round-trip synthesis.  Set Z0 target at 500 Ω, solve for a, re-analyse, confirm Z0 hits.
BOOST_AUTO_TEST_CASE( SynthesisRoundTripSolveForA )
{
    RECTWAVEGUIDE calc;
    SetWR90( calc );
    calc.SetParameter( TCP::Z0, 500.0 );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );

    calc.SetSynthesizeTarget( TCP::PHYS_WIDTH );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double a_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( a_synth ) );
    BOOST_TEST( a_synth > 0.0 );

    // Re-analyse and verify Z0 sits within 1 Ω of the 500 Ω target.
    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == 500.0, boost::test_tools::tolerance( 1.0 / 500.0 ) );
}


// High-frequency excitation of WR-90.  At 40 GHz, cutoff frequencies:
//   TE10 ≈  6.56 GHz
//   TE20 ≈ 13.12 GHz
//   TE01 ≈ 14.76 GHz
//   TE11 = TM11 ≈ 16.16 GHz
//   ... many more propagate below 40 GHz.
// The TE mode list must enumerate several tokens; TM must enumerate at least one.
BOOST_AUTO_TEST_CASE( HigherModeListing )
{
    RECTWAVEGUIDE calc;
    SetWR90( calc );
    calc.SetParameter( TCP::FREQUENCY, 40.0e9 );

    calc.Analyse();

    const std::string teModes = calc.GetTEModes();
    const std::string tmModes = calc.GetTMModes();

    // TE list must include the dominant and at least the next higher horizontal / vertical
    // modes; TM list must include the fundamental TM11.
    BOOST_TEST( teModes.find( "H(1,0)" ) != std::string::npos );
    BOOST_TEST( teModes.find( "H(2,0)" ) != std::string::npos );
    BOOST_TEST( teModes.find( "H(0,1)" ) != std::string::npos );
    BOOST_TEST( teModes.find( "H(1,1)" ) != std::string::npos );
    BOOST_TEST( tmModes.find( "E(1,1)" ) != std::string::npos );
}


BOOST_AUTO_TEST_SUITE_END()
