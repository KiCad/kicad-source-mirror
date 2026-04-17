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
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Loads a COAX instance with defaults common to all fixtures.
// Callers override geometry / material / frequency as required.
void SetDefaults( COAX& aCalc )
{
    aCalc.SetParameter( TCP::MUR, 1.0 );
    aCalc.SetParameter( TCP::MURC, 1.0 );
    aCalc.SetParameter( TCP::SIGMA, 5.8e7 );
    aCalc.SetParameter( TCP::TAND, 0.0 );
    aCalc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    aCalc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    aCalc.SetParameter( TCP::ANG_L, 0.0 );
}
} // namespace


BOOST_AUTO_TEST_SUITE( CoaxCalculations )


// Air-filled 50 Ω coax.  Din = 1.63 mm, Dout = 3.75 mm, εr = 1.
// Analytical Z0 = 60 * ln(Dout/Din) ≈ 49.96 Ω.
BOOST_AUTO_TEST_CASE( AirFilled50OhmImpedance )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 1.63 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 3.75 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );

    calc.Analyse();

    const double Z0 = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0 == 50.0, boost::test_tools::tolerance( 0.5 / 50.0 ) );
}


// TE11 cutoff for the air-filled 50 Ω cable above.
// f_c,TE11 = 2*c / (pi * (Din + Dout)) ≈ 35.4 GHz.
BOOST_AUTO_TEST_CASE( AirFilledTE11Cutoff )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 1.63 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 3.75 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );

    calc.Analyse();

    const double fc = calc.GetAnalysisResults()[TCP::CUTOFF_FREQUENCY].first;
    BOOST_TEST( fc == 35.4e9, boost::test_tools::tolerance( 1.0 / 35.4 ) );
}


// Teflon-filled 50 Ω (RG-142 family geometry).
// εr = 2.04, Din = 0.91 mm, Dout = 2.95 mm.  Z0 = 60/√εr * ln(Dout/Din) ≈ 49.37 Ω.
BOOST_AUTO_TEST_CASE( TeflonFilled50OhmImpedance )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 2.04 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 0.91 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 2.95 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );

    calc.Analyse();

    const double Z0 = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0 == 50.0, boost::test_tools::tolerance( 1.0 / 50.0 ) );
}


// Dielectric loss is finite and positive for a lossy dielectric.
// εr ≈ 4.3, tan δ = 0.02 at 1 GHz, 100 mm length.
BOOST_AUTO_TEST_CASE( DielectricLossIsFiniteAndPositive )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 1.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 3.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );

    calc.Analyse();

    const double loss_d = calc.GetAnalysisResults()[TCP::LOSS_DIELECTRIC].first;

    BOOST_TEST( std::isfinite( loss_d ) );
    BOOST_TEST( loss_d > 0.0 );

    // α_d = π · f · √εr · tan δ / c  (Np/m); convert to dB/m via LOG2DB.
    const double alpha_d = ( M_PI * 1.0e9 * std::sqrt( 4.3 ) * 0.02 ) / TC::C0;
    const double expected_dB = alpha_d * ( 100.0 * TC::UNIT_MM ) * TC::LOG2DB;

    BOOST_TEST( loss_d == expected_dB, boost::test_tools::tolerance( 0.01 ) );
}


// Round-trip synthesis.  Given Z0 = 50 Ω, Dout = 3.75 mm, εr = 1, solve for Din,
// then re-analyse and confirm Z0 matches within 0.1 Ω.
BOOST_AUTO_TEST_CASE( SynthesisRoundTripSolveForDin )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 1.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 3.75 * TC::UNIT_MM );
    calc.SetParameter( TCP::Z0, 50.0 );
    calc.SetParameter( TCP::ANG_L, M_PI / 2.0 );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );

    calc.SetSynthesizeTarget( TCP::PHYS_DIAM_IN );
    BOOST_TEST( calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT ) );

    const double Din_synth = calc.GetParameter( TCP::PHYS_DIAM_IN );
    BOOST_TEST( std::isfinite( Din_synth ) );
    BOOST_TEST( Din_synth > 0.0 );
    BOOST_TEST( Din_synth < 3.75 * TC::UNIT_MM );

    // Now analyse with the synthesized Din and confirm the Z0 target is met.
    calc.Analyse();
    const double Z0_check = calc.GetAnalysisResults()[TCP::Z0].first;
    BOOST_TEST( Z0_check == 50.0, boost::test_tools::tolerance( 0.1 / 50.0 ) );
}


// TE11 mode string correctly reports "H(1,1) " above cutoff and is empty below.
BOOST_AUTO_TEST_CASE( TE11ModeString )
{
    COAX calc;
    SetDefaults( calc );
    calc.SetParameter( TCP::EPSILONR, 1.0 );
    calc.SetParameter( TCP::PHYS_DIAM_IN, 1.63 * TC::UNIT_MM );
    calc.SetParameter( TCP::PHYS_DIAM_OUT, 3.75 * TC::UNIT_MM );

    // Below TE11 cutoff (f ≈ 35.4 GHz) the TE string is empty.
    calc.SetParameter( TCP::FREQUENCY, 10.0e9 );
    calc.Analyse();
    BOOST_TEST( calc.GetTEModes() == std::string( "" ) );

    // Above TE11 cutoff the string begins with "H(1,1) ".
    calc.SetParameter( TCP::FREQUENCY, 50.0e9 );
    calc.Analyse();
    const std::string teModes = calc.GetTEModes();
    BOOST_TEST( teModes.rfind( "H(1,1) ", 0 ) == 0u );
}


BOOST_AUTO_TEST_SUITE_END()
