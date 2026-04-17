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

#include <transline_calculations/coupled_microstrip.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Tightly coupled differential pair on a thin substrate. Geometry is chosen so that
// Kirschning-Jansen dispersion produces a measurable shift in the odd-mode impedance
// between 1 MHz and 10 GHz.
COUPLED_MICROSTRIP MakeFixture( double aFrequencyHz )
{
    const double h = 0.2 * TC::UNIT_MM;
    const double w = 0.15 * TC::UNIT_MM;
    const double s = 0.2 * TC::UNIT_MM;
    const double t = 35.0 * TC::UNIT_MICRON;

    COUPLED_MICROSTRIP calc;
    calc.SetParameter( TCP::EPSILONR, 4.3 );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, h );
    calc.SetParameter( TCP::H_T, 1.0e20 );
    calc.SetParameter( TCP::PHYS_WIDTH, w );
    calc.SetParameter( TCP::PHYS_S, s );
    calc.SetParameter( TCP::T, t );
    calc.SetParameter( TCP::PHYS_LEN, 100.0 * TC::UNIT_MM );
    calc.SetParameter( TCP::FREQUENCY, aFrequencyHz );
    return calc;
}
} // namespace


BOOST_AUTO_TEST_SUITE( CoupledMicrostripDiffImpedance )


BOOST_AUTO_TEST_CASE( ZdiffTracksDispersedOddMode )
{
    // diff_impedance() must read the Z0_O value left in the parameter map by
    // Z0_dispersion(), so Zdiff always equals exactly twice the published odd-mode
    // impedance regardless of frequency. Before the fix Zdiff was always 2 * Z0_o_0
    // (the static value), which drifts from 2 * Z0_O as soon as dispersion kicks in.
    for( double freq : { 1.0e6, 1.0e9, 5.0e9, 10.0e9 } )
    {
        COUPLED_MICROSTRIP calc = MakeFixture( freq );
        calc.Analyse();

        auto& results = calc.GetAnalysisResults();
        const double Zdiff = results.at(TCP::Z_DIFF).first;
        const double Z0_o = results.at(TCP::Z0_O).first;

        BOOST_TEST_MESSAGE( "f=" << freq << " Zdiff=" << Zdiff << " Z0_o=" << Z0_o );
        BOOST_TEST( std::fabs( Zdiff - 2.0 * Z0_o ) < 1.0e-9 );
    }
}


BOOST_AUTO_TEST_CASE( ZdiffChangesWithFrequency )
{
    // If diff_impedance() ignored Z0_dispersion() then Zdiff would be identical at
    // every frequency.  Confirm that the dispersed value actually moves across the
    // DC-to-10 GHz band on a ~100 Ohm pair. Direction of the shift depends on the
    // geometry; Kirschning-Jansen can raise or lower Z0_O.
    COUPLED_MICROSTRIP lo = MakeFixture( 1.0e6 );
    lo.Analyse();

    COUPLED_MICROSTRIP hi = MakeFixture( 10.0e9 );
    hi.Analyse();

    const double Zdiff_lo = lo.GetAnalysisResults()[TCP::Z_DIFF].first;
    const double Zdiff_hi = hi.GetAnalysisResults()[TCP::Z_DIFF].first;

    BOOST_TEST_MESSAGE( "Zdiff@1MHz=" << Zdiff_lo << " Zdiff@10GHz=" << Zdiff_hi );
    BOOST_TEST( std::fabs( Zdiff_hi - Zdiff_lo ) > 0.1 );
}


BOOST_AUTO_TEST_CASE( LowFrequencyMatchesStaticOddMode )
{
    // At 1 MHz Kirschning-Jansen dispersion is negligible, so the dispersed Z0_O stays
    // within floating-point noise of the static Z0_o_0 and Zdiff therefore still equals
    // 2 * static odd-mode impedance.  Low-frequency behaviour must not regress.
    COUPLED_MICROSTRIP calc = MakeFixture( 1.0e6 );
    calc.Analyse();

    auto& results = calc.GetAnalysisResults();

    const double Zdiff = results.at(TCP::Z_DIFF).first;
    const double Z0_o = results.at(TCP::Z0_O).first;

    BOOST_TEST( std::fabs( Zdiff - 2.0 * Z0_o ) < 0.1 );
}


BOOST_AUTO_TEST_SUITE_END()
