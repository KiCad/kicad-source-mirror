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

#include <transline_calculations/coax.h>
#include <transline_calculations/coplanar.h>
#include <transline_calculations/coupled_microstrip.h>
#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/microstrip.h>
#include <transline_calculations/rectwaveguide.h>
#include <transline_calculations/stripline.h>
#include <transline_calculations/twistedpair.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


/*
 * Parameter-flow regression net: for every transmission-line calculator, drive
 * Analyse -> set Z0 as the synthesis target -> Synthesize the canonical geometry
 * unknown -> re-Analyse, and assert Z0 (or Z0_E / Z0_O for coupled lines) recovers
 * within tolerance.  The per-calculator test files already cover each math class
 * in isolation; this file exists so that a parameter added to TRANSLINE_PARAMETERS
 * that is forgotten in one calculator's parameter list is caught by a single
 * failing test rather than requiring coverage of every edge case in every suite.
 *
 * Tolerances are 1% relative on Z0 except where the underlying synthesis is
 * ill-conditioned at the fixture's starting geometry; in that case the tolerance
 * is widened and the site is flagged with a comment explaining why.
 */


BOOST_AUTO_TEST_SUITE( TranslineRoundTrip )


// Microstrip.  Fixture mirrors a typical 50 Ohm FR-4 line: εr 4.3, tanδ 0.02,
// H 0.2 mm, W 0.4 mm, T 35 μm, 1 GHz.  Synthesis target is PHYS_WIDTH.
BOOST_AUTO_TEST_CASE( MicrostripWidthRoundTrip )
{
    MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, 0.2 * TC::UNIT_MM );
    calc.SetParameter( TCP::H_T, 1.0e20 );
    calc.SetParameter( TCP::T, 35.0 * TC::UNIT_MICRON );
    calc.SetParameter( TCP::PHYS_WIDTH, 0.4 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


// Stripline.  b = 1.6 mm (H), W = 0.4 mm, T = 35 μm, STRIPLINE_A centres the strip
// (STRIPLINE_A = (H - T) / 2).  εr 4.3, 1 GHz.  Synthesis target is PHYS_WIDTH.
BOOST_AUTO_TEST_CASE( StriplineWidthRoundTrip )
{
    STRIPLINE calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    const double h = 1.6 * TC::UNIT_MM;
    const double t = 35.0 * TC::UNIT_MICRON;
    calc.SetParameter( TCP::H, h );
    calc.SetParameter( TCP::T, t );
    calc.SetParameter( TCP::STRIPLINE_A, ( h - t ) / 2.0 );
    calc.SetParameter( TCP::PHYS_WIDTH, 0.4 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


// Coupled microstrip.  Same fixture as test_coupled_microstrip.cpp.  Synthesises
// PHYS_WIDTH with spacing fixed (FIX_SPACING).  The 1D Z0_O-only solve is less
// conditioned than the full 2D newton in the default branch, so use a 2%
// tolerance on Z0_O recovery.
BOOST_AUTO_TEST_CASE( CoupledMicrostripWidthRoundTrip )
{
    COUPLED_MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, 0.2 * TC::UNIT_MM );
    calc.SetParameter( TCP::H_T, 1.0e20 );
    calc.SetParameter( TCP::PHYS_WIDTH, 0.15 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_S, 0.2 * TC::UNIT_MM );
    calc.SetParameter( TCP::T, 35.0 * TC::UNIT_MICRON );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    auto& results = calc.GetAnalysisResults();
    const double Z0_E_orig = results.at(TCP::Z0_E).first;
    const double Z0_O_orig = results.at(TCP::Z0_O).first;

    // FIX_SPACING path only drives Z0_O; we'll verify Z0_O recovers after solving W.
    calc.SetParameter( TCP::Z0_E, Z0_E_orig );
    calc.SetParameter( TCP::Z0_O, Z0_O_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );

    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::FIX_SPACING ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_O_check = calc.GetAnalysisResults()[TCP::Z0_O].first;
    BOOST_TEST( Z0_O_check == Z0_O_orig, boost::test_tools::tolerance( 0.02 ) );
}


// Coupled stripline.  Mirrors the fixture from test_coupled_stripline.cpp.  The
// FIX_SPACING path solves PHYS_WIDTH against Z0_O; keep 2% tolerance to match
// the coupled microstrip convention for 1D coupled solves.
BOOST_AUTO_TEST_CASE( CoupledStriplineWidthRoundTrip )
{
    COUPLED_STRIPLINE calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, 20.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_WIDTH, 8.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_S, 8.0 * TC::UNIT_MIL );
    calc.SetParameter( TCP::T, 0.7 * TC::UNIT_MIL );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    auto& results = calc.GetAnalysisResults();
    const double Z0_E_orig = results.at(TCP::Z0_E).first;
    const double Z0_O_orig = results.at(TCP::Z0_O).first;

    calc.SetParameter( TCP::Z0_E, Z0_E_orig );
    calc.SetParameter( TCP::Z0_O, Z0_O_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );

    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::FIX_SPACING ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_O_check = calc.GetAnalysisResults()[TCP::Z0_O].first;
    BOOST_TEST( Z0_O_check == Z0_O_orig, boost::test_tools::tolerance( 0.02 ) );
}


// Coax.  Teflon-filled RG-142-ish geometry.  Default synthesis target is
// PHYS_DIAM_IN.  The closed-form synthesis is exact, so 1% is comfortable.
BOOST_AUTO_TEST_CASE( CoaxDinRoundTrip )
{
    COAX calc;
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::TAND, 0.0 );
    calc.SetParameter( TCP::EPSILONR, 2.04 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 0.91 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 2.95 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double Din_synth = calc.GetParameter( TCP::PHYS_DIAM_IN );
    BOOST_TEST( std::isfinite( Din_synth ) );
    BOOST_TEST( Din_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


// Coplanar (ungrounded CPW).  Default synthesis target is PHYS_WIDTH.
BOOST_AUTO_TEST_CASE( CoplanarWidthRoundTrip )
{
    COPLANAR calc;
    calc.SetParameter( TCP::EPSILONR, 4.4 );
    calc.SetParameter( TCP::TAND, 0.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, 0.8 * TC::UNIT_MM );
    calc.SetParameter( TCP::T, 35.0 * TC::UNIT_MICRON );
    calc.SetParameter( TCP::PHYS_WIDTH, 0.5 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_S, 0.3 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );
    calc.SetParameter( TCP::CPW_BACKMETAL, 0.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double W_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( W_synth ) );
    BOOST_TEST( W_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


// Rectangular waveguide.  WR-90-ish geometry at 10 GHz.  Default synthesis
// target is PHYS_WIDTH (the broad dimension a).
BOOST_AUTO_TEST_CASE( RectWaveguideWidthRoundTrip )
{
    RECTWAVEGUIDE calc;
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::TAND, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::PHYS_WIDTH, 22.86 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_S, 10.16 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    calc.SetParameter( TCP::ANG_L, 0.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double a_synth = calc.GetParameter( TCP::PHYS_WIDTH );
    BOOST_TEST( std::isfinite( a_synth ) );
    BOOST_TEST( a_synth > 0.0 );

    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


// Twisted pair.  Cat5e-like geometry.  Default synthesis target is
// PHYS_DIAM_OUT.  Lefferson model; a 1% tolerance is achievable because the
// single-variable solve over Dout is well-conditioned for Z0 in the 100-135 Ω
// band that this fixture covers.
BOOST_AUTO_TEST_CASE( TwistedPairDoutRoundTrip )
{
    TWISTEDPAIR calc;
    calc.SetParameter( TCP::EPSILONR, 2.3 );
    calc.SetParameter( TCP::TAND, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 0.511 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 1.000 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 1.000 * TC::UNIT_M );
    calc.SetParameter( TCP::FREQUENCY, 100.0 * TC::UNIT_MHZ );
    calc.SetParameter( TCP::ANG_L, 0.0 );
    calc.SetParameter( TCP::TWISTEDPAIR_TWIST, 49.0 );
    calc.SetParameter( TCP::TWISTEDPAIR_EPSILONR_ENV, 1.0 );

    calc.Analyse();
    const double Z0_orig = calc.GetAnalysisResults()[TCP::Z0].first;

    calc.SetParameter( TCP::Z0, Z0_orig );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double Dout_synth = calc.GetParameter( TCP::PHYS_DIAM_OUT );
    BOOST_TEST( std::isfinite( Dout_synth ) );
    BOOST_TEST( Dout_synth > 0.0 );

    calc.SetParameter( TCP::Z0, 0.0 );
    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == Z0_orig, boost::test_tools::tolerance( 0.01 ) );
}


BOOST_AUTO_TEST_SUITE_END()
