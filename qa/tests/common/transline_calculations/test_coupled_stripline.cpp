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

#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Builds a coupled stripline instance with the fixture geometry described in the task spec.
// b = 20 mil substrate, symmetric (strip centred), W = 8 mil, S = 8 mil, T = 0.7 mil,
// εr = 4.3, tan δ = 0.02, σ = 5.8e7 S/m, f = 1 GHz, L = 100 mm.
COUPLED_STRIPLINE MakeFixture()
{
    const double b = 20.0 * TC::UNIT_MIL;
    const double w = 8.0 * TC::UNIT_MIL;
    const double s = 8.0 * TC::UNIT_MIL;
    const double t = 0.7 * TC::UNIT_MIL;

    COUPLED_STRIPLINE calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, b );
    calc.SetParameter( TCP::PHYS_WIDTH, w );
    calc.SetParameter( TCP::PHYS_S, s );
    calc.SetParameter( TCP::T, t );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, 1.0e9 );
    return calc;
}
} // namespace


BOOST_AUTO_TEST_SUITE( CoupledStriplineLosses )


BOOST_AUTO_TEST_CASE( DielectricLossIsFiniteAndSymmetric )
{
    COUPLED_STRIPLINE calc = MakeFixture();
    calc.Analyse();

    auto& results = calc.GetAnalysisResults();
    const double loss_d_e = results.at(TCP::ATTEN_DILECTRIC_EVEN).first;
    const double loss_d_o = results.at(TCP::ATTEN_DILECTRIC_ODD).first;

    BOOST_TEST( loss_d_e > 0.0 );
    BOOST_TEST( loss_d_o > 0.0 );

    // Homogeneous TEM dielectric means even and odd modes see the same εr_eff
    BOOST_TEST( std::fabs( loss_d_e - loss_d_o ) / loss_d_e < 0.001 );
}


BOOST_AUTO_TEST_CASE( DielectricLossMatchesClosedForm )
{
    COUPLED_STRIPLINE calc = MakeFixture();
    calc.Analyse();

    auto& results = calc.GetAnalysisResults();
    const double loss_d_e = results.at(TCP::ATTEN_DILECTRIC_EVEN).first;

    // α_d = π · f · √εr · tan δ / c  (Np/m); total loss (dB) = α_d · L · LOG2DB
    const double alpha_d = ( M_PI * 1.0e9 * std::sqrt( 4.3 ) * 0.02 ) / TC::C0;
    const double expected_dB = alpha_d * ( 100.0 * TC::UNIT_MM ) * TC::LOG2DB;

    BOOST_TEST( expected_dB == 0.378, boost::test_tools::tolerance( 0.01 ) );
    BOOST_TEST( loss_d_e == expected_dB, boost::test_tools::tolerance( 0.03 ) );
}


BOOST_AUTO_TEST_CASE( ConductorLossIsPositive )
{
    COUPLED_STRIPLINE calc = MakeFixture();
    calc.Analyse();

    auto& results = calc.GetAnalysisResults();
    const double loss_c_e = results.at(TCP::ATTEN_COND_EVEN).first;
    const double loss_c_o = results.at(TCP::ATTEN_COND_ODD).first;

    BOOST_TEST( loss_c_e > 0.0 );
    BOOST_TEST( loss_c_o > 0.0 );

    // Odd-mode conductor loss is higher than even mode at the same geometry since Z0_o < Z0_e;
    // current density per conductor is higher in the odd mode.
    BOOST_TEST( loss_c_o > loss_c_e );
}


BOOST_AUTO_TEST_SUITE_END()
