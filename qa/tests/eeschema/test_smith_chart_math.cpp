/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <cmath>
#include <sim/sim_plot_tab.h>
#include <sim/smith_math.h>

BOOST_AUTO_TEST_SUITE( SmithChartMath )


BOOST_AUTO_TEST_CASE( GammaScreenRoundTrip )
{
    SMITH_VIEW view;
    view.center = wxPoint( 400, 300 );
    view.radius = 250.0;
    view.zoom = 1.0;
    view.pan = wxRealPoint( 0.0, 0.0 );

    // one pixel of rounding corresponds to 1/radius in gamma
    double tol = 1.0 / view.radius;

    for( double re : { -1.0, -0.5, 0.0, 0.3, 1.0 } )
    {
        for( double im : { -1.0, -0.25, 0.0, 0.6, 1.0 } )
        {
            wxRealPoint back = view.ToGamma( view.ToScreen( re, im ) );

            BOOST_CHECK_SMALL( back.x - re, tol );
            BOOST_CHECK_SMALL( back.y - im, tol );
        }
    }

    // positive Im (inductive) plots above the center
    BOOST_CHECK_LT( view.ToScreen( 0.0, 0.5 ).y, view.center.y );
    BOOST_CHECK_GT( view.ToScreen( 0.0, -0.5 ).y, view.center.y );
    BOOST_CHECK_GT( view.ToScreen( 0.5, 0.0 ).x, view.center.x );

    view.zoom = 8.0;
    view.radius = 250.0 * view.zoom;
    view.pan = wxRealPoint( 0.4, -0.2 );

    tol = 1.0 / view.radius;

    wxRealPoint back = view.ToGamma( view.ToScreen( 0.45, -0.15 ) );

    BOOST_CHECK_SMALL( back.x - 0.45, tol );
    BOOST_CHECK_SMALL( back.y - -0.15, tol );

    // the pan point lands on the window center
    BOOST_CHECK_EQUAL( view.ToScreen( 0.4, -0.2 ).x, view.center.x );
    BOOST_CHECK_EQUAL( view.ToScreen( 0.4, -0.2 ).y, view.center.y );
}


BOOST_AUTO_TEST_CASE( ZoomAboutPointKeepsGamma )
{
    // the gamma under the cursor must stay fixed while zooming
    double      baseRadius = 250.0;
    double      oldZoom = 2.0;
    double      newZoom = 5.0;
    wxPoint     center( 400, 300 );
    wxPoint     pos( 520, 180 );

    SMITH_VIEW view;
    view.center = center;
    view.zoom = oldZoom;
    view.radius = baseRadius * oldZoom;
    view.pan = wxRealPoint( 0.1, 0.3 );

    wxRealPoint gamma = view.ToGamma( pos );

    SMITH_VIEW zoomed;
    zoomed.center = center;
    zoomed.zoom = newZoom;
    zoomed.radius = baseRadius * newZoom;
    zoomed.pan = SMITH_MATH::ZoomAboutPoint( view, pos, newZoom );

    wxRealPoint after = zoomed.ToGamma( pos );

    BOOST_CHECK_CLOSE( after.x, gamma.x, 1e-9 );
    BOOST_CHECK_CLOSE( after.y, gamma.y, 1e-9 );

    // zooming back out returns the original pan
    SMITH_VIEW restored;
    restored.center = center;
    restored.zoom = oldZoom;
    restored.radius = baseRadius * oldZoom;
    restored.pan = SMITH_MATH::ZoomAboutPoint( zoomed, pos, oldZoom );

    BOOST_CHECK_CLOSE( restored.pan.x, view.pan.x, 1e-9 );
    BOOST_CHECK_CLOSE( restored.pan.y, view.pan.y, 1e-9 );
}


BOOST_AUTO_TEST_CASE( ToScreenOverflowSafety )
{
    SMITH_VIEW view;
    view.center = wxPoint( 400, 300 );
    view.radius = 25000.0; // 50x zoom on a large window
    view.zoom = 50.0;
    view.pan = wxRealPoint( 0.0, 0.0 );

    // a huge gamma from an active circuit must clamp, not overflow the int math
    wxPoint far = view.ToScreen( 1e12, -1e12 );

    BOOST_CHECK( far.x > view.center.x );
    BOOST_CHECK( far.y > view.center.y );

    // non-finite samples land on a defined point instead of feeding NaN to KiROUND
    double nan = std::nan( "" );
    double inf = std::numeric_limits<double>::infinity();

    BOOST_CHECK_EQUAL( view.ToScreen( nan, 0.0 ).x, view.center.x );
    BOOST_CHECK_EQUAL( view.ToScreen( 0.0, nan ).y, view.center.y );
    BOOST_CHECK_EQUAL( view.ToScreen( inf, inf ).x, view.center.x );

    // a NaN gamma cannot produce an impedance
    double r, x;
    BOOST_CHECK( !SMITH_MATH::GammaToImpedance( nan, 0.0, 50.0, r, x ) );
    BOOST_CHECK( !SMITH_MATH::GammaToImpedance( 0.0, nan, 50.0, r, x ) );
}


BOOST_AUTO_TEST_CASE( ImpedanceFromGamma )
{
    double r, x;

    // matched, gamma = 0 -> z = z0
    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( 0.0, 0.0, 50.0, r, x ) );
    BOOST_CHECK_CLOSE( r, 50.0, 1e-9 );
    BOOST_CHECK_SMALL( x, 1e-9 );

    // gamma = 1/3 on the real axis -> z = 2 z0
    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( 1.0 / 3.0, 0.0, 50.0, r, x ) );
    BOOST_CHECK_CLOSE( r, 100.0, 1e-9 );
    BOOST_CHECK_SMALL( x, 1e-9 );

    // gamma = -1/3 -> z = z0 / 2
    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( -1.0 / 3.0, 0.0, 50.0, r, x ) );
    BOOST_CHECK_CLOSE( r, 25.0, 1e-9 );

    // gamma = j0.5 -> z = 0.6 + j0.8 normalized
    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( 0.0, 0.5, 50.0, r, x ) );
    BOOST_CHECK_CLOSE( r, 30.0, 1e-9 );
    BOOST_CHECK_CLOSE( x, 40.0, 1e-9 );

    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( 0.0, -0.5, 50.0, r, x ) );
    BOOST_CHECK_CLOSE( x, -40.0, 1e-9 );

    // 75 ohm reference scales linearly
    BOOST_REQUIRE( SMITH_MATH::GammaToImpedance( 1.0 / 3.0, 0.0, 75.0, r, x ) );
    BOOST_CHECK_CLOSE( r, 150.0, 1e-9 );

    // open circuit singularity
    BOOST_CHECK( !SMITH_MATH::GammaToImpedance( 1.0, 0.0, 50.0, r, x ) );
}


BOOST_AUTO_TEST_CASE( SeriesEquivalents )
{
    BOOST_CHECK_CLOSE( SMITH_MATH::SeriesInductance( 72.652, 13.56e6 ), 852.72e-9, 0.1 );
    BOOST_CHECK_CLOSE( SMITH_MATH::SeriesInductance( 3259.6, 33.470438e6 ), 15.5e-6, 0.1 );

    BOOST_CHECK_CLOSE( SMITH_MATH::SeriesCapacitance( -100.0, 1e6 ), 1.5915e-9, 0.1 );
}


BOOST_AUTO_TEST_CASE( MatchMetrics )
{
    BOOST_CHECK_CLOSE( SMITH_MATH::VSWR( 0.0 ), 1.0, 1e-9 );
    BOOST_CHECK_CLOSE( SMITH_MATH::VSWR( 1.0 / 3.0 ), 2.0, 1e-9 );
    BOOST_CHECK_CLOSE( SMITH_MATH::VSWR( 0.5 ), 3.0, 1e-9 );
    BOOST_CHECK( std::isinf( SMITH_MATH::VSWR( 1.0 ) ) );

    BOOST_CHECK_SMALL( SMITH_MATH::ReturnLoss( 1.0 ), 1e-9 );
    BOOST_CHECK_CLOSE( SMITH_MATH::ReturnLoss( 0.1 ), 20.0, 1e-9 );
    BOOST_CHECK( std::isinf( SMITH_MATH::ReturnLoss( 0.0 ) ) );

    // the two dialects always agree
    for( double gm : { 0.05, 0.2, 0.45, 0.7, 0.95 } )
    {
        double rl = SMITH_MATH::ReturnLoss( gm );
        double lin = std::pow( 10.0, rl / 20.0 );

        BOOST_CHECK_CLOSE( SMITH_MATH::VSWR( gm ), ( lin + 1.0 ) / ( lin - 1.0 ), 1e-6 );
    }
}


BOOST_AUTO_TEST_CASE( SParamPortParsing )
{
    long response, drive;

    BOOST_REQUIRE( SMITH_MATH::ParseSParamPorts( wxS( "S_1_1" ), &response, &drive ) );
    BOOST_CHECK_EQUAL( response, 1 );
    BOOST_CHECK_EQUAL( drive, 1 );

    BOOST_REQUIRE( SMITH_MATH::ParseSParamPorts( wxS( "S_2_1" ), &response, &drive ) );
    BOOST_CHECK_EQUAL( response, 2 );
    BOOST_CHECK_EQUAL( drive, 1 );

    BOOST_REQUIRE( SMITH_MATH::ParseSParamPorts( wxS( "S_12_3" ), &response, &drive ) );
    BOOST_CHECK_EQUAL( response, 12 );
    BOOST_CHECK_EQUAL( drive, 3 );

    // user-defined or unrelated names are not S-parameter vectors
    BOOST_CHECK( !SMITH_MATH::ParseSParamPorts( wxS( "S_out" ), &response, &drive ) );
    BOOST_CHECK( !SMITH_MATH::ParseSParamPorts( wxS( "V(out)" ), &response, &drive ) );
    BOOST_CHECK( !SMITH_MATH::ParseSParamPorts( wxS( "S_1_" ), &response, &drive ) );
    BOOST_CHECK( !SMITH_MATH::ParseSParamPorts( wxS( "S__1" ), &response, &drive ) );
    BOOST_CHECK( !SMITH_MATH::ParseSParamPorts( wxS( "user0" ), &response, &drive ) );
}


BOOST_AUTO_TEST_CASE( SmithCursorSurvivesPartialSweep )
{
    std::vector<double> freqs, re, im;

    for( int ii = 0; ii <= 100; ii++ )
    {
        freqs.push_back( 1e9 + ii * 10e6 );
        re.push_back( 0.5 * std::cos( ii * 0.05 ) );
        im.push_back( 0.5 * std::sin( ii * 0.05 ) );
    }

    SMITH_TRACE  trace( wxS( "S_1_1" ), (SIM_TRACE_TYPE) ( SPT_VOLTAGE | SPT_SP_SMITH ) );
    SMITH_CURSOR cursor( &trace, nullptr );

    trace.SetCursor( 1, &cursor );
    trace.SetFrequencies( freqs );
    trace.SetData( re, im );

    cursor.SetCoordX( freqs[80] );
    BOOST_REQUIRE_EQUAL( cursor.GetCoords().x, freqs[80] );

    trace.SetFrequencies( std::vector<double>( freqs.begin(), freqs.begin() + 5 ) );
    trace.SetData( std::vector<double>( re.begin(), re.begin() + 5 ),
                   std::vector<double>( im.begin(), im.begin() + 5 ) );
    cursor.UpdateForNewData();

    trace.SetFrequencies( freqs );
    trace.SetData( re, im );
    cursor.UpdateForNewData();

    BOOST_CHECK_EQUAL( cursor.GetCoords().x, freqs[80] );
    BOOST_CHECK_EQUAL( cursor.GetGamma().x, re[80] );
    BOOST_CHECK_EQUAL( cursor.GetGamma().y, im[80] );
}


BOOST_AUTO_TEST_CASE( SmithCursorResolvesFrequencyOnceDataArrives )
{
    std::vector<double> freqs, re, im;

    for( int ii = 0; ii <= 100; ii++ )
    {
        freqs.push_back( 1e9 + ii * 10e6 );
        re.push_back( 0.5 * std::cos( ii * 0.05 ) );
        im.push_back( 0.5 * std::sin( ii * 0.05 ) );
    }

    SMITH_TRACE  trace( wxS( "S_1_1" ), (SIM_TRACE_TYPE) ( SPT_VOLTAGE | SPT_SP_SMITH ) );
    SMITH_CURSOR cursor( &trace, nullptr );

    trace.SetCursor( 1, &cursor );
    cursor.SetCoordX( freqs[80] );

    trace.SetFrequencies( freqs );
    trace.SetData( re, im );
    cursor.UpdateForNewData();

    BOOST_CHECK_EQUAL( cursor.GetCoords().x, freqs[80] );
    BOOST_CHECK_EQUAL( cursor.GetGamma().x, re[80] );
}


BOOST_AUTO_TEST_SUITE_END()
