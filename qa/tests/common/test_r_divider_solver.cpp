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

#include <algorithm>
#include <cmath>

#include <r_divider_solver.h>

static const std::vector<uint16_t> E3 = { 100, 220, 470 };

static const std::vector<uint16_t> E24 = { 100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300,
                                           330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910 };


static std::vector<R_DIVIDER_SOLUTION> solve( double aVin, double aVout, double aImpedance,
                                              const std::vector<uint16_t>& aSeries = E24 )
{
    return SolveResistorDivider( aVin, aVout, aImpedance, R_DIVIDER_IMPEDANCE::SERIES, aSeries );
}


static double ratioError( const R_DIVIDER_SOLUTION& aSolution, double aVin, double aVout )
{
    return std::abs( aSolution.R2 / ( aSolution.R1 + aSolution.R2 ) / ( aVout / aVin ) - 1 );
}


static bool contains( const std::vector<R_DIVIDER_SOLUTION>& aSolutions, double aR1, double aR2 )
{
    return std::any_of( aSolutions.begin(), aSolutions.end(),
                        [&]( const R_DIVIDER_SOLUTION& aSolution )
                        {
                            return aSolution.R1 == aR1 && aSolution.R2 == aR2;
                        } );
}


BOOST_AUTO_TEST_SUITE( RDividerSolver )


BOOST_AUTO_TEST_CASE( RejectsUnusableInput )
{
    BOOST_CHECK( solve( 5, 0, 10e3 ).empty() );
    BOOST_CHECK( solve( 5, 5, 10e3 ).empty() );
    BOOST_CHECK( solve( 5, 6, 10e3 ).empty() );
    BOOST_CHECK( solve( 5, 1.2, 0 ).empty() );
    BOOST_CHECK( solve( 5, 1.2, -10e3 ).empty() );
    BOOST_CHECK( solve( NAN, 1.2, 10e3 ).empty() );
    BOOST_CHECK( solve( 5, NAN, 10e3 ).empty() );
    BOOST_CHECK( solve( 5, 1.2, NAN ).empty() );
    BOOST_CHECK( solve( 5, 1.2, 10e3, {} ).empty() );

    // Vout > 0, but Vout / Vin underflows to 0
    BOOST_CHECK( solve( 1e300, 1e-300, 10e3 ).empty() );
    BOOST_CHECK( SolveResistorDivider( 1e300, 1e-300, 10e3, R_DIVIDER_IMPEDANCE::PARALLEL, E24 ).empty() );
}


/**
 * Resistors are offered from 1 mOhm to 1 TOhm, so a request whose ideal R1 or R2 falls outside
 * that is refused, while one just inside it is served.
 */
BOOST_AUTO_TEST_CASE( RejectsResistorsOutOfRange )
{
    // Ideal R2 of 0.5 mOhm, then 2 mOhm
    BOOST_CHECK( solve( 2, 1, 1e-3 ).empty() );
    BOOST_CHECK( !solve( 2, 1, 4e-3 ).empty() );

    // Ideal R1 and R2 of 2 TOhm
    BOOST_CHECK( SolveResistorDivider( 2, 1, 1e12, R_DIVIDER_IMPEDANCE::PARALLEL, E24 ).empty() );

    // Ideal R1 of 1 uOhm, far below the range although R2 is fine
    BOOST_CHECK( solve( 1 + 1e-6, 1, 1 ).empty() );

    BOOST_CHECK( FindResistorDividers( 2, 1, 1e-3, R_DIVIDER_IMPEDANCE::SERIES, E24 ).empty() );
}


/**
 * With an output impedance target, halving a voltage takes two resistors of twice the target.
 */
BOOST_AUTO_TEST_CASE( ParallelImpedanceExactRatioTieIsBrokenByImpedance )
{
    std::vector<R_DIVIDER_SOLUTION> solutions = SolveResistorDivider( 2, 1, 5e3, R_DIVIDER_IMPEDANCE::PARALLEL, E24 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_EQUAL( solutions[0].R1, 10e3 );
    BOOST_CHECK_EQUAL( solutions[0].R2, 10e3 );
}


/**
 * The best ratio does not depend on which impedance is targeted, but the resistors that reach
 * it do: R1 || R2 = 10k needs much larger resistors than R1 + R2 = 10k. The later solutions
 * give up ratio accuracy to bring R1 || R2 closer to the target.
 */
BOOST_AUTO_TEST_CASE( ParallelImpedanceTarget )
{
    auto outputImpedance =
            []( const R_DIVIDER_SOLUTION& aSolution )
            {
                return aSolution.R1 * aSolution.R2 / ( aSolution.R1 + aSolution.R2 );
            };

    std::vector<R_DIVIDER_SOLUTION> series = solve( 5, 1.2, 10e3 );
    std::vector<R_DIVIDER_SOLUTION> parallel = SolveResistorDivider( 5, 1.2, 10e3, R_DIVIDER_IMPEDANCE::PARALLEL, E24 );

    BOOST_REQUIRE( !series.empty() );
    BOOST_REQUIRE_GE( parallel.size(), 2 );
    BOOST_CHECK_LE( parallel.size(), 5 );

    BOOST_CHECK_CLOSE( ratioError( parallel[0], 5, 1.2 ), ratioError( series[0], 5, 1.2 ), 1e-6 );
    BOOST_CHECK_EQUAL( parallel[0].R1, 51e3 );
    BOOST_CHECK_EQUAL( parallel[0].R2, 16e3 );

    double closest = std::abs( outputImpedance( parallel[0] ) / 10e3 - 1 );

    for( const R_DIVIDER_SOLUTION& solution : parallel )
        closest = std::min( closest, std::abs( outputImpedance( solution ) / 10e3 - 1 ) );

    BOOST_CHECK_LT( closest, 0.01 );
    BOOST_CHECK_LT( closest, std::abs( outputImpedance( parallel[0] ) / 10e3 - 1 ) );
}


/**
 * Halving a voltage is exact with any two equal resistors, so every decade ties on the ratio and
 * the impedance has to pick the winner. This only works if ties survive round-off.
 */
BOOST_AUTO_TEST_CASE( ExactRatioTieIsBrokenByImpedance )
{
    std::vector<R_DIVIDER_SOLUTION> solutions = solve( 2, 1, 20e3 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_EQUAL( solutions[0].R1, 10e3 );
    BOOST_CHECK_EQUAL( solutions[0].R2, 10e3 );
}


/**
 * 5 V to 1.2 V needs R1 / R2 = 19 / 6, which E24 cannot hit exactly.
 */
BOOST_AUTO_TEST_CASE( FiveVoltsToOnePointTwo )
{
    std::vector<R_DIVIDER_SOLUTION> solutions = solve( 5, 1.2, 10e3 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_LE( solutions.size(), 5 );

    // The first solution has the best ratio of them all
    for( const R_DIVIDER_SOLUTION& solution : solutions )
        BOOST_CHECK_LE( ratioError( solutions[0], 5, 1.2 ), ratioError( solution, 5, 1.2 ) );

    // 7.5k / 2.4k is the nearest pair to 10k total and is within 1.1 % of the ratio
    BOOST_CHECK( contains( solutions, 7.5e3, 2.4e3 ) );

    // No pair is reported twice
    for( size_t i = 0; i < solutions.size(); ++i )
    {
        for( size_t j = i + 1; j < solutions.size(); ++j )
            BOOST_CHECK( solutions[i].R1 != solutions[j].R1 || solutions[i].R2 != solutions[j].R2 );
    }
}


/**
 * Verfiy that we rank solutions correctly depending on whether the tool is set to sort by
 * in/out ratio or R1/R2 ratio
 */
BOOST_AUTO_TEST_CASE( RanksByRequestedQuantity )
{
    // R1 / R2 = 1.5 falls between 10k / 4.7k (2.13) and 4.7k / 4.7k (1). The first is closer on
    // Vout / Vin (20 % against 25 %), the second on R1 / R2 (33 % against 42 %).
    std::vector<R_DIVIDER_SOLUTION> byVoltage =
            SolveResistorDivider( 2.5, 1, 10e3, R_DIVIDER_IMPEDANCE::SERIES, E3, R_DIVIDER_TARGET::VOLTAGE );
    std::vector<R_DIVIDER_SOLUTION> byResistorRatio =
            SolveResistorDivider( 2.5, 1, 10e3, R_DIVIDER_IMPEDANCE::SERIES, E3, R_DIVIDER_TARGET::RESISTOR_RATIO );

    BOOST_REQUIRE( !byVoltage.empty() );
    BOOST_REQUIRE( !byResistorRatio.empty() );

    BOOST_CHECK_CLOSE( byVoltage[0].R1 / byVoltage[0].R2, 10.0 / 4.7, 1e-9 );
    BOOST_CHECK_CLOSE( byResistorRatio[0].R1 / byResistorRatio[0].R2, 1.0, 1e-9 );

    // The three resistor solver follows the same rule: no solution is further off on R1 / R2
    // than the first one
    std::vector<R_DIVIDER_SOLUTION> triples = SolveThreeResistorDivider( 2.5, 1, 10e3, R_DIVIDER_IMPEDANCE::SERIES, E3,
                                                                         R_DIVIDER_TARGET::RESISTOR_RATIO );

    BOOST_REQUIRE( !triples.empty() );

    auto resistorRatioError =
            []( const R_DIVIDER_SOLUTION& aSolution )
            {
                return std::abs( aSolution.Top() / aSolution.Bottom() / 1.5 - 1 );
            };

    for( const R_DIVIDER_SOLUTION& triple : triples )
        BOOST_CHECK_LE( resistorRatioError( triples[0] ), resistorRatioError( triple ) + 1e-12 );
}


/**
 * Swapping the two resistors turns a divider for r into one for 1 - r, so the solver should treat
 * small and large ratios alike.
 */
BOOST_AUTO_TEST_CASE( BestRatioIsSymmetric )
{
    std::vector<R_DIVIDER_SOLUTION> low = solve( 10, 2, 47e3 );
    std::vector<R_DIVIDER_SOLUTION> high = solve( 10, 8, 47e3 );

    BOOST_REQUIRE( !low.empty() );
    BOOST_REQUIRE( !high.empty() );
    BOOST_CHECK_EQUAL( low[0].R1, high[0].R2 );
    BOOST_CHECK_EQUAL( low[0].R2, high[0].R1 );
}


/**
 * Only the ratio of the voltages and the scale of the impedance matter.
 */
BOOST_AUTO_TEST_CASE( ScalesWithImpedance )
{
    std::vector<R_DIVIDER_SOLUTION> base = solve( 3.3, 1.8, 10e3 );
    std::vector<R_DIVIDER_SOLUTION> scaled = solve( 33, 18, 10e6 );

    BOOST_REQUIRE_EQUAL( base.size(), scaled.size() );

    for( size_t i = 0; i < base.size(); ++i )
    {
        BOOST_CHECK_CLOSE( scaled[i].R1, base[i].R1 * 1000, 1e-9 );
        BOOST_CHECK_CLOSE( scaled[i].R2, base[i].R2 * 1000, 1e-9 );
    }
}


/**
 * E3 is sparse enough that the search has to range far from the ideal values. A resistor ratio of
 * 47 (pass k + 1 and 1) is exact with 47k over 1k.
 */
BOOST_AUTO_TEST_CASE( SparseSeriesFindsExactRatio )
{
    std::vector<R_DIVIDER_SOLUTION> solutions = solve( 48, 1, 48e3, E3 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_EQUAL( solutions[0].R1, 47e3 );
    BOOST_CHECK_EQUAL( solutions[0].R2, 1e3 );
}


static std::vector<R_DIVIDER_SOLUTION> solveThree( double aVin, double aVout, double aImpedance,
                                                   const std::vector<uint16_t>& aSeries = E24 )
{
    return SolveThreeResistorDivider( aVin, aVout, aImpedance, R_DIVIDER_IMPEDANCE::SERIES, aSeries );
}


static double threeResistorRatioError( const R_DIVIDER_SOLUTION& aSolution, double aVin, double aVout )
{
    return std::abs( aSolution.Bottom() / ( aSolution.Top() + aSolution.Bottom() ) / ( aVout / aVin ) - 1 );
}


static double threeResistorImpedanceError( const R_DIVIDER_SOLUTION& aSolution, double aImpedance )
{
    return std::abs( ( aSolution.Top() + aSolution.Bottom() ) / aImpedance - 1 );
}


BOOST_AUTO_TEST_CASE( ThreeResistorsRejectUnusableInput )
{
    BOOST_CHECK( solveThree( 5, 0, 10e3 ).empty() );
    BOOST_CHECK( solveThree( 5, 6, 10e3 ).empty() );
    BOOST_CHECK( solveThree( 5, 1.2, 0 ).empty() );
    BOOST_CHECK( solveThree( NAN, 1.2, 10e3 ).empty() );
    BOOST_CHECK( solveThree( 5, 1.2, 10e3, {} ).empty() );
    BOOST_CHECK( solveThree( 1e300, 1e-300, 10e3 ).empty() );
    BOOST_CHECK( SolveThreeResistorDivider( 5, 1.2, 10e3, R_DIVIDER_IMPEDANCE::PARALLEL, E24 ).empty() );

    // Legs of 0.5 mOhm and 2 TOhm are outside the range of resistors, 2 mOhm is inside
    BOOST_CHECK( solveThree( 2, 1, 1e-3 ).empty() );
    BOOST_CHECK( solveThree( 2, 1, 4e12 ).empty() );
    BOOST_CHECK( !solveThree( 2, 1, 4e-3 ).empty() );
}


/**
 * A two resistor divider leaves R3 at 0.
 */
BOOST_AUTO_TEST_CASE( TwoResistorSolutionsHaveNoThirdResistor )
{
    for( const R_DIVIDER_SOLUTION& solution : solve( 5, 1.2, 10e3 ) )
    {
        BOOST_CHECK_EQUAL( solution.R3, 0 );
        BOOST_CHECK( solution.Topology == R_DIVIDER_TOPOLOGY::TWO_RESISTORS );
        BOOST_CHECK_EQUAL( solution.Top(), solution.R1 );
        BOOST_CHECK_EQUAL( solution.Bottom(), solution.R2 );
    }
}


/**
 * 5 V to 1.2 V needs a bottom leg of 6 / 19 of the top one, which two E24 resistors only reach
 * to 0.5 %. A third resistor makes it exact, and on the impedance too: 2k and 5.6k over 2.4k.
 */
BOOST_AUTO_TEST_CASE( ThreeResistorsBeatTwo )
{
    std::vector<R_DIVIDER_SOLUTION> pairs = solve( 5, 1.2, 10e3 );
    std::vector<R_DIVIDER_SOLUTION> triples = solveThree( 5, 1.2, 10e3 );

    BOOST_REQUIRE( !pairs.empty() );
    BOOST_REQUIRE( !triples.empty() );
    BOOST_CHECK_LE( triples.size(), 4 );

    BOOST_CHECK_LT( threeResistorRatioError( triples[0], 5, 1.2 ), ratioError( pairs[0], 5, 1.2 ) );

    BOOST_CHECK( triples[0].Topology == R_DIVIDER_TOPOLOGY::SPLIT_TOP );
    BOOST_CHECK_EQUAL( triples[0].Top(), 7.6e3 );
    BOOST_CHECK_EQUAL( triples[0].Bottom(), 2.4e3 );

    for( const R_DIVIDER_SOLUTION& solution : triples )
    {
        BOOST_CHECK_GT( solution.R3, 0 );

        // The first solution has the best ratio of them all
        BOOST_CHECK_LE( threeResistorRatioError( triples[0], 5, 1.2 ), threeResistorRatioError( solution, 5, 1.2 ) );
    }

    // No set of resistors is reported twice, R2 and R3 being interchangeable
    for( size_t i = 0; i < triples.size(); ++i )
    {
        for( size_t j = i + 1; j < triples.size(); ++j )
        {
            std::vector<double> first = { triples[i].R1, triples[i].R2, triples[i].R3 };
            std::vector<double> second = { triples[j].R1, triples[j].R2, triples[j].R3 };

            std::sort( first.begin(), first.end() );
            std::sort( second.begin(), second.end() );

            BOOST_CHECK( triples[i].Topology != triples[j].Topology || first != second );
        }
    }
}


/**
 * Splitting the top leg for r is splitting the bottom leg for 1 - r, so the two requests should
 * give mirror images of each other.
 */
BOOST_AUTO_TEST_CASE( ThreeResistorsAreSymmetric )
{
    std::vector<R_DIVIDER_SOLUTION> low = solveThree( 5, 1.2, 10e3 );
    std::vector<R_DIVIDER_SOLUTION> high = solveThree( 5, 3.8, 10e3 );

    BOOST_REQUIRE( !low.empty() );
    BOOST_REQUIRE( !high.empty() );
    BOOST_CHECK( high[0].Topology == R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM );
    BOOST_CHECK_EQUAL( low[0].Top(), high[0].Bottom() );
    BOOST_CHECK_EQUAL( low[0].Bottom(), high[0].Top() );
}


/**
 * E6 cannot get both the ratio and the impedance right here, so the solutions differ. Later ones
 * give up ratio accuracy for a closer impedance, and none is listed twice.
 */
BOOST_AUTO_TEST_CASE( ThreeResistorsTradeRatioForImpedance )
{
    static const std::vector<uint16_t> E6 = { 100, 150, 220, 330, 470, 680 };

    std::vector<R_DIVIDER_SOLUTION> solutions = solveThree( 5, 1.2, 10e3, E6 );

    BOOST_REQUIRE_GE( solutions.size(), 2 );

    for( size_t i = 1; i < solutions.size(); ++i )
    {
        BOOST_CHECK_GE( threeResistorRatioError( solutions[i], 5, 1.2 ),
                        threeResistorRatioError( solutions[i - 1], 5, 1.2 ) );
    }

    BOOST_CHECK_LT( threeResistorImpedanceError( solutions.back(), 10e3 ),
                    threeResistorImpedanceError( solutions.front(), 10e3 ) );
}


/**
 * Halving a voltage is exact when the split leg adds up to the single resistor, and of all the
 * ways to do that the impedance picks the winner.
 */
BOOST_AUTO_TEST_CASE( ThreeResistorsExactRatioTieIsBrokenByImpedance )
{
    std::vector<R_DIVIDER_SOLUTION> solutions = solveThree( 2, 1, 20e3 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_EQUAL( solutions[0].Top(), 10e3 );
    BOOST_CHECK_EQUAL( solutions[0].Bottom(), 10e3 );
}


/**
 * R3 is in parallel with R1 or with R2 in the parallel arrangements.
 */
BOOST_AUTO_TEST_CASE( ParallelLegs )
{
    R_DIVIDER_SOLUTION solution = { 3e3, 6e3, 6e3, R_DIVIDER_TOPOLOGY::PARALLEL_TOP };

    BOOST_CHECK_EQUAL( solution.Top(), 2e3 );
    BOOST_CHECK_EQUAL( solution.Bottom(), 6e3 );

    solution.Topology = R_DIVIDER_TOPOLOGY::PARALLEL_BOTTOM;

    BOOST_CHECK_EQUAL( solution.Top(), 3e3 );
    BOOST_CHECK_EQUAL( solution.Bottom(), 3e3 );
}


/**
 * 3.3 V to 1.8 V needs a top leg of 5 / 6 of the bottom one. No three E48 resistors with two in
 * series get there, but 59k in parallel with 649k over 64.9k does. The parallel pair competes
 * with the series ones for the same places, so it comes first.
 */
BOOST_AUTO_TEST_CASE( ParallelResistorsBeatSeries )
{
    static const std::vector<uint16_t> E48 = { 100, 105, 110, 115, 121, 127, 133, 140, 147, 154, 162, 169,
                                               178, 187, 196, 205, 215, 226, 237, 249, 261, 274, 287, 301,
                                               316, 332, 348, 365, 383, 402, 422, 442, 464, 487, 511, 536,
                                               562, 590, 619, 649, 681, 715, 750, 787, 825, 866, 909, 953 };

    std::vector<R_DIVIDER_SOLUTION> solutions = solveThree( 3.3, 1.8, 100e3, E48 );

    BOOST_REQUIRE( !solutions.empty() );
    BOOST_CHECK_LE( solutions.size(), 4 );

    BOOST_CHECK( solutions[0].Topology == R_DIVIDER_TOPOLOGY::PARALLEL_TOP );
    BOOST_CHECK_EQUAL( std::min( solutions[0].R1, solutions[0].R3 ), 59e3 );
    BOOST_CHECK_EQUAL( std::max( solutions[0].R1, solutions[0].R3 ), 649e3 );
    BOOST_CHECK_EQUAL( solutions[0].R2, 64.9e3 );
    BOOST_CHECK_LT( threeResistorRatioError( solutions[0], 3.3, 1.8 ), 1e-12 );

    for( const R_DIVIDER_SOLUTION& solution : solutions )
    {
        bool series = solution.Topology == R_DIVIDER_TOPOLOGY::SPLIT_TOP
                      || solution.Topology == R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM;

        if( series )
            BOOST_CHECK_GT( threeResistorRatioError( solution, 3.3, 1.8 ), 1e-12 );
    }
}


/**
 * Putting the parallel pair on top for r is putting it on the bottom for 1 - r.
 */
BOOST_AUTO_TEST_CASE( ParallelResistorsAreSymmetric )
{
    static const std::vector<uint16_t> E6 = { 100, 150, 220, 330, 470, 680 };

    std::vector<R_DIVIDER_SOLUTION> low = solveThree( 5, 1.2, 10e3, E6 );
    std::vector<R_DIVIDER_SOLUTION> high = solveThree( 5, 3.8, 10e3, E6 );

    BOOST_REQUIRE_EQUAL( low.size(), high.size() );

    for( size_t i = 0; i < low.size(); ++i )
    {
        BOOST_CHECK_EQUAL( low[i].Top(), high[i].Bottom() );
        BOOST_CHECK_EQUAL( low[i].Bottom(), high[i].Top() );
    }
}


/**
 * Verify that three resistor solutions strictly worse then an existing two-resistor solution
 * are not shown.
 */
BOOST_AUTO_TEST_CASE( ThreeResistorsMustEarnTheirPlace )
{
    auto find =
            []( double aVin, double aVout, double aImpedance )
            {
                return FindResistorDividers( aVin, aVout, aImpedance, R_DIVIDER_IMPEDANCE::SERIES, E24 );
            };

    auto threeResistorCount =
            []( const std::vector<R_DIVIDER_RESULT>& aResults )
            {
                return std::count_if( aResults.begin(), aResults.end(),
                                      []( const R_DIVIDER_RESULT& aResult )
                                      {
                                          return aResult.Solution.Topology != R_DIVIDER_TOPOLOGY::TWO_RESISTORS;
                                      } );
            };

    // 10k / 10k and 1k / 1k are exact on both the ratio and the impedance: nothing can beat them
    BOOST_CHECK_EQUAL( threeResistorCount( find( 5, 2.5, 20e3 ) ), 0 );
    BOOST_CHECK_EQUAL( threeResistorCount( find( 12, 6, 2e3 ) ), 0 );

    // 5.1k / 5.1k is exact on the ratio but 2 % off on the impedance, which leaves room for one of
    // the two dividers that the three resistor solver returns
    BOOST_CHECK_EQUAL( SolveThreeResistorDivider( 5, 2.5, 10e3, R_DIVIDER_IMPEDANCE::SERIES, E24 ).size(), 2 );
    BOOST_CHECK_EQUAL( threeResistorCount( find( 5, 2.5, 10e3 ) ), 1 );

    // No pair is exact on the ratio here
    BOOST_CHECK_EQUAL( threeResistorCount( find( 5, 1.2, 10e3 ) ), 1 );

    // Two resistor dividers come first, with their errors
    std::vector<R_DIVIDER_RESULT> results = find( 5, 2.5, 10e3 );

    BOOST_REQUIRE_GE( results.size(), 3 );
    BOOST_CHECK( results[0].Solution.Topology == R_DIVIDER_TOPOLOGY::TWO_RESISTORS );
    BOOST_CHECK_EQUAL( results[0].Solution.R1, 5100 );
    BOOST_CHECK_EQUAL( results[0].Solution.R2, 5100 );
    BOOST_CHECK_SMALL( results[0].TargetError, 1e-12 );
    BOOST_CHECK_CLOSE( results[0].Impedance, 10200, 1e-9 );
    BOOST_CHECK_CLOSE( results[0].ImpedanceError, 0.02, 1e-6 );
    BOOST_CHECK( results.back().Solution.Topology != R_DIVIDER_TOPOLOGY::TWO_RESISTORS );

    // Every three resistor divider listed beats all the pairs on one of the errors
    double bestTargetError = 1;
    double bestImpedanceError = 1;

    for( const R_DIVIDER_RESULT& result : results )
    {
        if( result.Solution.Topology == R_DIVIDER_TOPOLOGY::TWO_RESISTORS )
        {
            bestTargetError = std::min( bestTargetError, std::abs( result.TargetError ) );
            bestImpedanceError = std::min( bestImpedanceError, std::abs( result.ImpedanceError ) );
        }
        else
        {
            BOOST_CHECK( std::abs( result.TargetError ) < bestTargetError
                         || std::abs( result.ImpedanceError ) < bestImpedanceError );
        }
    }

    // The error is measured on the requested quantity: an exact resistor ratio has none, however
    // small the ratio. 1 + 1e-6 is itself off by about 1e-10 of the ratio as a double.
    std::vector<R_DIVIDER_RESULT> byRatio = FindResistorDividers( 1 + 1e-6, 1, 10e3, R_DIVIDER_IMPEDANCE::SERIES, E24,
                                                                  R_DIVIDER_TARGET::RESISTOR_RATIO );

    BOOST_REQUIRE( !byRatio.empty() );
    BOOST_CHECK_SMALL( byRatio[0].TargetError, 1e-9 );

    BOOST_CHECK( find( 5, 6, 10e3 ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
