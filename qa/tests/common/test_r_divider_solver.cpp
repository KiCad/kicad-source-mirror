/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <limits>

#include <r_divider_solver.h>


/// The E24 or E3 resistors from 1 ohm to 9.1 Mohm
static std::vector<double> resistors( bool aE3 = false )
{
    static const std::vector<int> e24 = { 10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30,
                                          33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91 };
    static const std::vector<int> e3 = { 10, 22, 47 };

    std::vector<double> values;

    for( double decade = 1; decade < 1e7; decade *= 10 )
    {
        for( int value : aE3 ? e3 : e24 )
            values.push_back( value * decade / 10 );
    }

    return values;
}


static double legValue( const R_DIVIDER_LEG& aLeg )
{
    if( aLeg.B == 0 )
        return aLeg.A;

    return aLeg.Parallel ? aLeg.A * aLeg.B / ( aLeg.A + aLeg.B ) : aLeg.A + aLeg.B;
}


/// Every divider must be what it claims, and no more than three resistors
static void checkDividers( const std::vector<R_DIVIDER>& aDividers, double aRatio, double aImpedance )
{
    BOOST_REQUIRE( !aDividers.empty() );

    for( const R_DIVIDER& divider : aDividers )
    {
        const double top = legValue( divider.Top );
        const double bottom = legValue( divider.Bottom );

        BOOST_CHECK( divider.Top.B == 0 || divider.Bottom.B == 0 );
        BOOST_CHECK_CLOSE( divider.Top.Value, top, 1e-9 );
        BOOST_CHECK_CLOSE( divider.Bottom.Value, bottom, 1e-9 );
        BOOST_CHECK_CLOSE( divider.Impedance, top + bottom, 1e-9 );
        BOOST_CHECK_CLOSE( 1 + divider.ImpedanceError, ( top + bottom ) / aImpedance, 1e-9 );
        BOOST_CHECK_CLOSE( 1 + divider.TargetError, top / bottom / aRatio, 1e-9 );
    }
}


BOOST_AUTO_TEST_SUITE( RDividerSolver )


BOOST_AUTO_TEST_CASE( RejectsUnusableInput )
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    // Ratios beyond 1e6 either way, then resistors outside 1 ohm to 9.1 Mohm
    const std::pair<double, double> requests[] = { { 0, 1e4 }, { -1, 1e4 }, { nan, 1e4 }, { 1, 0 }, { 1, inf },
                                                   { 5e-7, 3e6 }, { 2e6, 3e6 }, { 1, 1e12 }, { 1e-5, 1e4 } };

    for( const auto& [ratio, impedance] : requests )
    {
        BOOST_TEST_CONTEXT( "ratio " << ratio << ", impedance " << impedance )
        {
            BOOST_CHECK( FindResistorDividers( ratio, impedance, R_DIVIDER_IMPEDANCE::SERIES, resistors(),
                                               R_DIVIDER_TARGET::RESISTOR_RATIO )
                                 .empty() );
        }
    }

    BOOST_CHECK(
            FindResistorDividers( 1, 1e4, R_DIVIDER_IMPEDANCE::SERIES, {}, R_DIVIDER_TARGET::RESISTOR_RATIO ).empty() );
}


/// Any two equal resistors halve a voltage exactly, so only the impedance can pick between them
BOOST_AUTO_TEST_CASE( ExactRatioTieIsBrokenByImpedance )
{
    std::vector<R_DIVIDER> dividers =
            FindResistorDividers( 1, 2e4, R_DIVIDER_IMPEDANCE::SERIES, resistors(), R_DIVIDER_TARGET::VOLTAGE );

    BOOST_REQUIRE_EQUAL( dividers.size(), 1u );
    BOOST_CHECK( dividers[0].Top == ( R_DIVIDER_LEG{ 1e4, 1e4 } ) );
    BOOST_CHECK( dividers[0].Bottom == ( R_DIVIDER_LEG{ 1e4, 1e4 } ) );

    // R1 || R2 of twice the target
    dividers = FindResistorDividers( 1, 5e3, R_DIVIDER_IMPEDANCE::PARALLEL, resistors(), R_DIVIDER_TARGET::VOLTAGE );

    BOOST_REQUIRE_EQUAL( dividers.size(), 1u );
    BOOST_CHECK_EQUAL( dividers[0].Top.A, 1e4 );
    BOOST_CHECK_EQUAL( dividers[0].Impedance, 5e3 );
}


/// 5 V to 1.2 V needs R1 / R2 = 19 / 6, which no two E24 resistors make but 2k + 5k6 over 2k4 does at 10k
BOOST_AUTO_TEST_CASE( ThreeResistorsBeatTwo )
{
    const double           ratio = 19.0 / 6;
    std::vector<R_DIVIDER> dividers = FindResistorDividers( ratio, 1e4, R_DIVIDER_IMPEDANCE::SERIES, resistors(),
                                                            R_DIVIDER_TARGET::RESISTOR_RATIO );

    checkDividers( dividers, ratio, 1e4 );

    // Two resistor dividers come first and a third resistor makes both errors vanish
    BOOST_CHECK( dividers.front().Top.B == 0 && dividers.front().Bottom.B == 0 );
    BOOST_CHECK_GT( std::abs( dividers.front().TargetError ), 1e-3 );

    BOOST_CHECK( dividers.back().Top.B != 0 || dividers.back().Bottom.B != 0 );
    BOOST_CHECK_SMALL( dividers.back().TargetError, 1e-12 );
    BOOST_CHECK_SMALL( dividers.back().ImpedanceError, 1e-12 );
}


/// E3 is sparse, so the search has to go well away from the impedance to find 47k over 1k
BOOST_AUTO_TEST_CASE( SparseSeriesFindsExactRatio )
{
    std::vector<R_DIVIDER> dividers = FindResistorDividers( 47, 3e4, R_DIVIDER_IMPEDANCE::SERIES, resistors( true ),
                                                            R_DIVIDER_TARGET::RESISTOR_RATIO );

    checkDividers( dividers, 47, 3e4 );
    BOOST_CHECK_EQUAL( dividers.front().Top.A, 4.7e4 );
    BOOST_CHECK_EQUAL( dividers.front().Bottom.A, 1e3 );
    BOOST_CHECK_EQUAL( dividers.front().TargetError, 0 );
}


/// No pair of resistors has a better ratio, near the ideal ones or within 1 % of the impedance
/// Ratio 1 at 2k1 only fits 1 % away from the ideal ratio
BOOST_AUTO_TEST_CASE( BestPairMatchesBruteForce )
{
    const std::vector<double> values = resistors();

    const std::pair<double, double> requests[] = { { 0.0137, 1e4 }, { 0.37, 1e4 }, { 1, 2100 },
                                                   { 2.5, 1e4 },    { 7.3, 1e4 },  { 4567, 1e4 } };

    for( const auto& [ratio, impedance] : requests )
    {
        const double idealTop = impedance * ratio / ( 1 + ratio );

        double bestFree = std::numeric_limits<double>::infinity();
        double bestTight = std::numeric_limits<double>::infinity();

        for( double top : values )
        {
            for( double bottom : values )
            {
                const double error = std::abs( top / bottom / ratio - 1 );

                if( top >= idealTop / 3 && top <= idealTop * 3 )
                    bestFree = std::min( bestFree, error );

                if( std::abs( ( top + bottom ) / impedance - 1 ) <= 0.01 + 1e-9 )
                    bestTight = std::min( bestTight, error );
            }
        }

        BOOST_TEST_CONTEXT( "ratio " << ratio << ", impedance " << impedance )
        {
            std::vector<R_DIVIDER> dividers = FindResistorDividers( ratio, impedance, R_DIVIDER_IMPEDANCE::SERIES,
                                                                    values, R_DIVIDER_TARGET::RESISTOR_RATIO );

            checkDividers( dividers, ratio, impedance );
            BOOST_CHECK_LE( std::abs( dividers.front().TargetError ), bestFree + 1e-12 );

            double solverTight = std::numeric_limits<double>::infinity();

            for( const R_DIVIDER& divider : dividers )
            {
                if( divider.Top.B == 0 && divider.Bottom.B == 0 && std::abs( divider.ImpedanceError ) <= 0.01 + 1e-9 )
                    solverTight = std::min( solverTight, std::abs( divider.TargetError ) );
            }

            BOOST_CHECK_CLOSE( solverTight, bestTight, 1e-9 );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
