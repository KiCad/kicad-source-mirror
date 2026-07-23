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

#include <constraints/constraint_system_2d.h>
#include <GCS.h>

#include <algorithm>
#include <chrono>
#include <vector>


BOOST_AUTO_TEST_SUITE( ConstraintSystem2D )


BOOST_AUTO_TEST_CASE( CoordinateFrameRoundTripsFarFromOrigin )
{
    CONSTRAINT_SYSTEM_2D system;
    system.SetCoordinateFrame( VECTOR2I( 500000000, 700000000 ), 1000000.0 );

    BOOST_CHECK_CLOSE( system.NormalizeX( 512345678 ), 12.345678, 1e-9 );
    BOOST_CHECK_CLOSE( system.NormalizeY( 698765433 ), -1.234567, 1e-9 );
    BOOST_CHECK_CLOSE( system.DenormalizeX( 12.345678 ), 512345678.0, 1e-9 );
    BOOST_CHECK_CLOSE( system.DenormalizeY( -1.234567 ), 698765433.0, 1e-9 );
}


BOOST_AUTO_TEST_CASE( ParameterAddressesRemainStable )
{
    CONSTRAINT_SYSTEM_2D system;
    int                  first = system.AddParameter( 42.0 );
    double*              address = system.ParameterPointer( first );

    for( int i = 0; i < 16'384; ++i )
        system.AddParameter( i );

    BOOST_CHECK_EQUAL( system.ParameterPointer( first ), address );
    BOOST_CHECK_EQUAL( *address, 42.0 );
}


BOOST_AUTO_TEST_CASE( SnapshotRestoresEveryParameter )
{
    CONSTRAINT_SYSTEM_2D system;
    int                  x = system.AddParameter( 1.25 );
    int                  y = system.AddParameter( -4.5 );
    auto                 snapshot = system.Snapshot();

    system.Parameter( x ) = 99.0;
    system.Parameter( y ) = 101.0;
    BOOST_REQUIRE( system.Restore( snapshot ) );

    BOOST_CHECK_EQUAL( system.Parameter( x ), 1.25 );
    BOOST_CHECK_EQUAL( system.Parameter( y ), -4.5 );
}


BOOST_AUTO_TEST_CASE( PrefixRestorePreservesLaterStableStorage )
{
    CONSTRAINT_SYSTEM_2D system;
    int first = system.AddParameter( 1.0 );
    CONSTRAINT_SYSTEM_2D::SNAPSHOT snapshot = system.Snapshot();
    int temporary = system.AddParameter( 2.0 );

    system.Parameter( first ) = 3.0;
    system.Parameter( temporary ) = 4.0;

    BOOST_CHECK( !system.Restore( snapshot ) );
    BOOST_REQUIRE( system.RestorePrefix( snapshot ) );
    BOOST_CHECK_EQUAL( system.Parameter( first ), 1.0 );
    BOOST_CHECK_EQUAL( system.Parameter( temporary ), 4.0 );
}


BOOST_AUTO_TEST_CASE( BoundedParameterHandlingP95StaysWithinSixteenMilliseconds )
{
    std::vector<std::chrono::microseconds> samples;

    for( int run = 0; run < 20; ++run )
    {
        auto                 start = std::chrono::steady_clock::now();
        CONSTRAINT_SYSTEM_2D system;
        GCS::VEC_pD unknowns;

        for( int i = 0; i < 128; ++i )
        {
            int parameter = system.AddParameter( i / 2 );
            unknowns.push_back( system.ParameterPointer( parameter ) );
        }

        for( int relation = 0; relation < 64; ++relation )
            system.Solver().addConstraintEqual( unknowns[relation * 2],
                                                unknowns[relation * 2 + 1], relation + 1 );

        system.Solver().declareUnknowns( unknowns );
        system.Solver().initSolution();
        int solveResult = system.Solver().solve();
        BOOST_CHECK( solveResult == GCS::Success || solveResult == GCS::Converged );
        system.Solver().applySolution();
        samples.push_back( std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start ) );
    }

    std::sort( samples.begin(), samples.end() );
    std::chrono::microseconds p95 = samples[18];
    BOOST_TEST_MESSAGE( "128-parameter/64-relation solve p95: " << p95.count() << " us" );
    BOOST_CHECK_LE( p95.count(), 16000 );
}


BOOST_AUTO_TEST_CASE( LargeParameterHandlingRecordsTelemetry )
{
    auto                 start = std::chrono::steady_clock::now();
    CONSTRAINT_SYSTEM_2D system;
    GCS::VEC_pD          unknowns;

    for( int i = 0; i < 1024; ++i )
    {
        int parameter = system.AddParameter( i / 2 );
        unknowns.push_back( system.ParameterPointer( parameter ) );
    }

    for( int relation = 0; relation < 512; ++relation )
        system.Solver().addConstraintEqual( unknowns[relation * 2],
                                            unknowns[relation * 2 + 1], relation + 1 );

    system.Solver().declareUnknowns( unknowns );
    system.Solver().initSolution();
    int solveResult = system.Solver().solve();
    BOOST_CHECK( solveResult == GCS::Success || solveResult == GCS::Converged );
    system.Solver().applySolution();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start );
    BOOST_TEST_MESSAGE( "1024-parameter/512-relation solve: " << elapsed.count() << " us" );
}


BOOST_AUTO_TEST_SUITE_END()
