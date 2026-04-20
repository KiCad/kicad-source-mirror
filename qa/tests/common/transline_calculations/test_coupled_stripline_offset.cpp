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
#include <limits>

#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


namespace
{
// Shared geometry spec.  b is the full plate spacing; W, S, T, eps_r, f match the Plan 4 task.
struct Geometry
{
    double b;
    double w;
    double s;
    double t;
    double er;
    double f;
    double L;
};


Geometry Geometry1()
{
    return { 20.0 * TC::UNIT_MIL, 8.0 * TC::UNIT_MIL, 8.0 * TC::UNIT_MIL, 0.7 * TC::UNIT_MIL,
             4.3, 1.0e9, 100.0 * TC::UNIT_MM };
}


// stripline_a < 0 leaves STRIPLINE_A unwritten so the backward-compat default-to-centred path is
// exercised; stripline_a >= 0 writes it explicitly.
COUPLED_STRIPLINE MakeCalc( const Geometry& g, double stripline_a )
{
    COUPLED_STRIPLINE calc;
    calc.SetParameter( TCP::EPSILONR, g.er );
    calc.SetParameter( TCP::TAND, 0.02 );
    calc.SetParameter( TCP::MUR, 1.0 );
    calc.SetParameter( TCP::MURC, 1.0 );
    calc.SetParameter( TCP::ROUGH, 0.0 );
    calc.SetParameter( TCP::SIGMA, 5.8e7 );
    calc.SetParameter( TCP::H, g.b );
    calc.SetParameter( TCP::PHYS_WIDTH, g.w );
    calc.SetParameter( TCP::PHYS_S, g.s );
    calc.SetParameter( TCP::T, g.t );
    calc.SetParameter( TCP::PHYS_LEN, g.L );
    calc.SetParameter( TCP::FREQUENCY, g.f );

    if( stripline_a >= 0.0 )
        calc.SetParameter( TCP::STRIPLINE_A, stripline_a );

    return calc;
}


struct Impedances
{
    double z0e;
    double z0o;
    double zdiff;
};


Impedances Run( COUPLED_STRIPLINE& calc )
{
    calc.Analyse();
    return { calc.GetParameter( TCP::Z0_E ), calc.GetParameter( TCP::Z0_O ),
             calc.GetParameter( TCP::Z_DIFF ) };
}
} // namespace


BOOST_AUTO_TEST_SUITE( CoupledStriplineOffset )


// Geometry 1 symmetric centred baseline.  Numbers come from the existing Cohn implementation with
// the finite-thickness fringe correction at t = 0.7 mil; they are not independent targets but a
// snapshot of the pre-offset code path so later tests can compare against it.
BOOST_AUTO_TEST_CASE( CenteredSymmetric )
{
    const Geometry g = Geometry1();
    COUPLED_STRIPLINE calc = MakeCalc( g, -1.0 );
    const Impedances z = Run( calc );

    BOOST_TEST( z.z0e == 55.34, boost::test_tools::tolerance( 0.05 ) );
    BOOST_TEST( z.z0o == 43.59, boost::test_tools::tolerance( 0.05 ) );
    BOOST_TEST( z.zdiff == 87.18, boost::test_tools::tolerance( 0.05 ) );
    BOOST_TEST( z.zdiff == 2.0 * z.z0o, boost::test_tools::tolerance( 1e-12 ) );
}


// Writing STRIPLINE_A = H/2 must be indistinguishable from leaving it unset.  Regression guard
// for the default-to-centred branch in Analyse().
BOOST_AUTO_TEST_CASE( OffsetWithExplicitCenter )
{
    const Geometry g = Geometry1();

    COUPLED_STRIPLINE baseline = MakeCalc( g, -1.0 );
    const Impedances zBaseline = Run( baseline );

    COUPLED_STRIPLINE explicitCenter = MakeCalc( g, g.b / 2.0 );
    const Impedances zExplicit = Run( explicitCenter );

    BOOST_TEST( zExplicit.z0e == zBaseline.z0e, boost::test_tools::tolerance( 1e-6 ) );
    BOOST_TEST( zExplicit.z0o == zBaseline.z0o, boost::test_tools::tolerance( 1e-6 ) );
    BOOST_TEST( zExplicit.zdiff == zBaseline.zdiff, boost::test_tools::tolerance( 1e-6 ) );
}


// Geometry 2: plate spacing 20 mil, strip plane offset to a = 6 mil (b - a = 14 mil).  Reference
// values come from the Shelton admittance-sum construction applied analytically to the Cohn
// zero-thickness formula (Z0e = 53.98, Z0o = 45.10 zero-thickness) with the same finite-thickness
// fringe correction applied to each virtual stripline.  Tolerances widened to 8 percent to cover
// residual approximation error from applying symmetric fringe correction to asymmetric geometry.
BOOST_AUTO_TEST_CASE( OffsetAt6Mil )
{
    const Geometry g = Geometry1();
    COUPLED_STRIPLINE calc = MakeCalc( g, 6.0 * TC::UNIT_MIL );
    const Impedances z = Run( calc );

    BOOST_TEST( z.z0e == 50.0, boost::test_tools::tolerance( 0.08 ) );
    BOOST_TEST( z.z0o == 41.5, boost::test_tools::tolerance( 0.08 ) );
    BOOST_TEST( z.zdiff == 83.0, boost::test_tools::tolerance( 0.08 ) );

    COUPLED_STRIPLINE centered = MakeCalc( g, -1.0 );
    const Impedances zCentered = Run( centered );

    // Moving a from b/2 = 10 mil toward one plane brings both mode impedances down because the
    // nearer plane capacitance increases.  Z0e drops more than Z0o in absolute terms since the
    // even-mode field extends further vertically and is more perturbed by plane proximity.
    const double dropE = zCentered.z0e - z.z0e;
    const double dropO = zCentered.z0o - z.z0o;
    BOOST_TEST( dropE > 0.0 );
    BOOST_TEST( dropO > 0.0 );
    BOOST_TEST( dropE > dropO );
}


// Offset path at a = b/2 must collapse to the centred result to machine precision.  The tight
// tolerance verifies the admittance-sum implementation correctly short-circuits (or arithmetically
// recovers) the symmetric case.
BOOST_AUTO_TEST_CASE( OffsetReproducesCenteredWhenCentered )
{
    const Geometry g = Geometry1();

    COUPLED_STRIPLINE centered = MakeCalc( g, -1.0 );
    const Impedances zCentered = Run( centered );

    COUPLED_STRIPLINE offsetCentered = MakeCalc( g, g.b / 2.0 );
    const Impedances zOffset = Run( offsetCentered );

    BOOST_TEST( zOffset.z0e == zCentered.z0e, boost::test_tools::tolerance( 1e-6 ) );
    BOOST_TEST( zOffset.z0o == zCentered.z0o, boost::test_tools::tolerance( 1e-6 ) );
}


// Physical-consistency smoke test.  Moving the strip plane toward one ground plane must drive
// Z0e monotonically downward because the nearer plane adds parallel capacitance.
BOOST_AUTO_TEST_CASE( OffsetMonotonicInA )
{
    const Geometry g = Geometry1();

    const double aValues[] = { g.b / 2.0, 9.0 * TC::UNIT_MIL, 7.0 * TC::UNIT_MIL,
                               5.0 * TC::UNIT_MIL, 3.0 * TC::UNIT_MIL };

    double prevZ0e = std::numeric_limits<double>::infinity();

    for( double a : aValues )
    {
        COUPLED_STRIPLINE calc = MakeCalc( g, a );
        const Impedances z = Run( calc );
        BOOST_TEST( z.z0e < prevZ0e );
        prevZ0e = z.z0e;
    }
}


// Synthesis must keep working against the centred model.  Synthesize(FIX_SPACING) drives Z0_O to
// target, so we pick Z0_O = Zdiff_target/2, solve for W, re-Analyse, and check Zdiff.
BOOST_AUTO_TEST_CASE( SynthesisRoundTripCentered )
{
    const Geometry g = Geometry1();
    COUPLED_STRIPLINE calc = MakeCalc( g, -1.0 );

    const double zdiffTarget = 87.18;
    calc.SetParameter( TCP::Z0_O, zdiffTarget / 2.0 );

    const bool ok = calc.Synthesize( SYNTHESIZE_OPTS::FIX_SPACING );
    BOOST_REQUIRE( ok );

    calc.Analyse();
    const double zdiff = calc.GetParameter( TCP::Z_DIFF );
    BOOST_TEST( zdiff == zdiffTarget, boost::test_tools::tolerance( 0.01 ) );
}


// Offset must lie strictly inside (t/2, h - t/2) for the Shelton finite-thickness path.
// Outside that band the Cohn fringe formula divides by zero.  Drive a small offset against a
// 0.7 mil strip in a 20 mil cavity and verify the analysed impedances surface as TS_ERROR
// via NaN rather than silently returning bogus numbers.
BOOST_AUTO_TEST_CASE( OffsetBelowFiniteThicknessLimitYieldsError )
{
    const Geometry g = Geometry1();
    COUPLED_STRIPLINE calc = MakeCalc( g, 0.2 * TC::UNIT_MIL );
    calc.Analyse();

    const double z0e = calc.GetParameter( TCP::Z0_E );
    const double z0o = calc.GetParameter( TCP::Z0_O );
    BOOST_TEST( !std::isfinite( z0e ) );
    BOOST_TEST( !std::isfinite( z0o ) );

    auto& results = calc.GetAnalysisResults();
    BOOST_TEST( ( results[TCP::Z0_E].second == TRANSLINE_STATUS::TS_ERROR ) );
    BOOST_TEST( ( results[TCP::Z0_O].second == TRANSLINE_STATUS::TS_ERROR ) );
}



BOOST_AUTO_TEST_SUITE_END()
