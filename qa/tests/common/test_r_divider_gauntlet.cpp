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

/**
 * @file
 * Round trip test of the resistor divider solver.
 *
 * Dividers are built from randomly chosen standard resistors and a random input voltage. Ohm's
 * law gives the output voltage and the impedance of each one, and those are handed back to the
 * solver, which has to come up with the divider they came from.
 *
 * Each divider is also asked for with its ratio and its impedance nudged off, so that no exact
 * answer exists and the whole search is exercised. The original divider is then no longer the
 * right answer, but it is still a candidate, so whatever the solver returns has to be at least
 * as good.
 *
 * A round trip always asks for the impedance of a real divider, which puts the right answer in
 * the middle of what the solver searches. A last test therefore asks for ratios and impedances
 * that have nothing to do with each other, and compares the answer with the closest ratio there
 * is, found by trying every pair of resistors.
 *
 * The random numbers come from a fixed seed: every run tests the same dividers.
 */

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include <r_divider_solver.h>


// Relative difference below which two doubles are taken to be the same value
static constexpr double SAME = 1e-12;

// How far the nudged requests move the resistor ratio, as a fraction of it
static constexpr double RATIO_NUDGE_MIN = 1e-4;
static constexpr double RATIO_NUDGE_MAX = 5e-3;

// How far the nudged requests move the impedance at most. This keeps the original divider within
// the 10 % that the tightest impedance window of the two resistor solver always allows.
static constexpr double IMPEDANCE_NUDGE_MAX = 0.05;

// Leeway on the ratio error of the three resistor solver when the request is nudged. The last
// resistor is the one closest to its ideal value, which may be the neighbour on the other side
// of it than the original one, and the ratio error is not quite symmetrical around the ideal.
static constexpr double FITTED_RESISTOR_LEEWAY = 1.05;

// Requests made of each series when looking for the closest ratio, half for each impedance type
static constexpr int CLOSEST_RATIO_RUNS = 60;


struct GAUNTLET_SERIES
{
    const char*           Name;
    std::vector<uint16_t> Values;
    int                   PairRuns;   // dividers tried for each impedance type of the two resistor solver
    int                   TripleRuns; // dividers tried with the three resistor solver
};


/**
 * The series to run through. The time to solve grows steeply with the size of the series, so
 * the largest ones only get a handful of runs to keep the whole test to a few seconds.
 */
static const std::vector<GAUNTLET_SERIES>& gauntletSeries()
{
    static const std::vector<uint16_t> e24 = { 100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300,
                                               330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910 };

    static const std::vector<uint16_t> e192 = {
        100, 101, 102, 104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129,
        130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 147, 149, 150, 152, 154, 156, 158, 160, 162, 164, 165, 167,
        169, 172, 174, 176, 178, 180, 182, 184, 187, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218,
        221, 223, 226, 229, 232, 234, 237, 240, 243, 246, 249, 252, 255, 258, 261, 264, 267, 271, 274, 277, 280, 284,
        287, 291, 294, 298, 301, 305, 309, 312, 316, 320, 324, 328, 332, 336, 340, 344, 348, 352, 357, 361, 365, 370,
        374, 379, 383, 388, 392, 397, 402, 407, 412, 417, 422, 427, 432, 437, 442, 448, 453, 459, 464, 470, 475, 481,
        487, 493, 499, 505, 511, 517, 523, 530, 536, 542, 549, 556, 562, 569, 576, 583, 590, 597, 604, 612, 619, 626,
        634, 642, 649, 657, 665, 673, 681, 690, 698, 706, 715, 723, 732, 741, 750, 759, 768, 777, 787, 796, 806, 816,
        825, 835, 845, 856, 866, 876, 887, 898, 909, 920, 931, 942, 953, 965, 976, 988
    };

    // The smaller series of each family are every second, fourth or eighth value of the largest
    auto every =
            []( const std::vector<uint16_t>& aValues, size_t aSkip )
            {
                std::vector<uint16_t> subset;

                for( size_t i = 0; i < aValues.size(); i += aSkip )
                    subset.push_back( aValues[i] );

                return subset;
            };

    static const std::vector<GAUNTLET_SERIES> series = { { "E3", every( e24, 8 ), 300, 200 },
                                                         { "E6", every( e24, 4 ), 300, 200 },
                                                         { "E12", every( e24, 2 ), 300, 120 },
                                                         { "E24", e24, 200, 60 },
                                                         { "E48", every( e192, 4 ), 120, 40 },
                                                         { "E96", every( e192, 2 ), 80, 16 },
                                                         { "E192", e192, 50, 8 } };

    return series;
}


/**
 * Source of the random dividers. Everything is derived from the raw output of the generator,
 * rather than through the standard distributions, so that every platform draws the same ones.
 */
class GAUNTLET_RANDOM
{
public:
    GAUNTLET_RANDOM( uint32_t aSeed ) :
            m_generator( aSeed )
    {
    }

    /// A whole number from \a aMin to \a aMax, both included
    int Integer( int aMin, int aMax ) { return aMin + static_cast<int>( m_generator() % ( aMax - aMin + 1 ) ); }

    /// A number from \a aMin to \a aMax
    double Real( double aMin, double aMax ) { return aMin + ( aMax - aMin ) * ( m_generator() / 4294967296.0 ); }

    /// A resistor of the series in the given decade, decade 0 being 1 to 10 ohms
    double Resistor( const std::vector<uint16_t>& aSeries, int aDecade )
    {
        // Multiply before dividing: both steps are then exact or correctly rounded
        return aSeries[Integer( 0, aSeries.size() - 1 )] * std::pow( 10.0, aDecade ) / 100.0;
    }

    /// A voltage from 1 mV to 10 kV with up to four significant digits
    double Voltage() { return Integer( 1, 9999 ) * std::pow( 10.0, Integer( 0, 3 ) ) / 1000.0; }

    /// A number between \a aMin and \a aMax in size, of either sign
    double Nudge( double aMin, double aMax ) { return Real( aMin, aMax ) * ( Integer( 0, 1 ) ? 1 : -1 ); }

private:
    std::mt19937 m_generator;
};


static double impedanceOf( const R_DIVIDER_SOLUTION& aDivider, R_DIVIDER_IMPEDANCE aType )
{
    double top = aDivider.Top();
    double bottom = aDivider.Bottom();

    return aType == R_DIVIDER_IMPEDANCE::PARALLEL ? top * bottom / ( top + bottom ) : top + bottom;
}


/// Relative error of the divider ratio, measured as the solver does on Vout / Vin
static double ratioError( const R_DIVIDER_SOLUTION& aDivider, double aVin, double aVout )
{
    return std::abs( aDivider.Bottom() / ( aDivider.Top() + aDivider.Bottom() ) / ( aVout / aVin ) - 1 );
}


static double impedanceError( const R_DIVIDER_SOLUTION& aDivider, R_DIVIDER_IMPEDANCE aType, double aImpedance )
{
    return std::abs( impedanceOf( aDivider, aType ) / aImpedance - 1 );
}


static bool isSame( double aFirst, double aSecond )
{
    return std::abs( aFirst / aSecond - 1 ) < SAME;
}


static bool isInSeries( const std::vector<uint16_t>& aSeries, double aResistor )
{
    if( !( aResistor > 0 ) )
        return false;

    // Bring the resistor to the 100 to 999 range of the series values
    double scaled = aResistor / std::pow( 10.0, std::floor( std::log10( aResistor ) ) - 2 );
    double rounded = std::round( scaled );

    // A value just short of a power of ten rounds up to 1000
    if( rounded >= 1000 )
        rounded /= 10;

    return std::abs( scaled / rounded - 1 ) < 1e-9
           && std::binary_search( aSeries.begin(), aSeries.end(), static_cast<uint16_t>( rounded ) );
}


/**
 * Tell whether the impedance of a divider is within the tight window of the solvers: 10 % of the
 * target, or two of the largest steps of the series, whichever reaches further.
 */
static bool isInTightWindow( const R_DIVIDER_SOLUTION& aDivider, R_DIVIDER_IMPEDANCE aType, double aImpedance,
                             const std::vector<uint16_t>& aSeries )
{
    double step = 10.0 * aSeries.front() / aSeries.back();

    for( size_t i = 1; i < aSeries.size(); ++i )
        step = std::max( step, static_cast<double>( aSeries[i] ) / aSeries[i - 1] );

    double lowest = aImpedance * std::min( 0.9, 1 / ( step * step ) ) * ( 1 - SAME );
    double highest = aImpedance * std::max( 1.1, step * step ) * ( 1 + SAME );
    double impedance = impedanceOf( aDivider, aType );

    return impedance >= lowest && impedance <= highest;
}


/// The resistors of a divider, sorted, so that dividers can be compared whatever their order
static std::vector<double> sortedResistors( const R_DIVIDER_SOLUTION& aDivider )
{
    std::vector<double> resistors = { aDivider.R1, aDivider.R2, aDivider.R3 };

    std::sort( resistors.begin(), resistors.end() );
    return resistors;
}


/**
 * Checks that hold for any answer of either solver: the number of solutions, the resistors being
 * standard ones, the first solution having the best ratio and no solution being listed twice.
 */
static void checkSolutions( const std::vector<R_DIVIDER_SOLUTION>& aSolutions, const GAUNTLET_SERIES& aSeries,
                            bool aThreeResistors, double aVin, double aVout )
{
    BOOST_REQUIRE( !aSolutions.empty() );
    BOOST_CHECK_LE( aSolutions.size(), aThreeResistors ? 4 : 5 );

    for( size_t i = 0; i < aSolutions.size(); ++i )
    {
        const R_DIVIDER_SOLUTION& solution = aSolutions[i];

        BOOST_CHECK( isInSeries( aSeries.Values, solution.R1 ) );
        BOOST_CHECK( isInSeries( aSeries.Values, solution.R2 ) );

        if( aThreeResistors )
        {
            BOOST_CHECK( solution.Topology != R_DIVIDER_TOPOLOGY::TWO_RESISTORS );
            BOOST_CHECK( isInSeries( aSeries.Values, solution.R3 ) );
        }
        else
        {
            BOOST_CHECK( solution.Topology == R_DIVIDER_TOPOLOGY::TWO_RESISTORS );
            BOOST_CHECK_EQUAL( solution.R3, 0 );
        }

        BOOST_CHECK_LE( ratioError( aSolutions[0], aVin, aVout ), ratioError( solution, aVin, aVout ) + SAME );

        for( size_t j = i + 1; j < aSolutions.size(); ++j )
        {
            const R_DIVIDER_SOLUTION& other = aSolutions[j];

            if( aThreeResistors )
            {
                BOOST_CHECK( solution.Topology != other.Topology
                             || sortedResistors( solution ) != sortedResistors( other ) );
            }
            else
            {
                BOOST_CHECK( solution.R1 != other.R1 || solution.R2 != other.R2 );
            }
        }
    }
}


/**
 * Move a request off the divider it was computed from.
 *
 * The ratio is nudged as R1 / R2 rather than as Vout / Vin: a small change of Vout / Vin is a
 * huge change of the resistors when Vout is close to Vin, and would leave the original divider
 * outside what the solver searches.
 */
static void nudgeRequest( GAUNTLET_RANDOM& aRandom, const R_DIVIDER_SOLUTION& aDivider, double aVin, double& aVout,
                          double& aImpedance )
{
    double resistorRatio = aDivider.Top() / aDivider.Bottom();

    resistorRatio *= 1 + aRandom.Nudge( RATIO_NUDGE_MIN, RATIO_NUDGE_MAX );

    aVout = aVin / ( 1 + resistorRatio );
    aImpedance *= 1 + aRandom.Nudge( 0, IMPEDANCE_NUDGE_MAX );
}


static std::string describe( const GAUNTLET_SERIES& aSeries, const R_DIVIDER_SOLUTION& aDivider, double aVin,
                             double aVout, double aImpedance )
{
    static const char* topologies[] = { "R1 / R2", "R1 / ( R2 + R3 )", "( R1 + R3 ) / R2", "R1 / ( R2 | R3 )",
                                        "( R1 | R3 ) / R2" };

    char text[256];

    snprintf( text, sizeof( text ), "%s, %s with R1 = %g, R2 = %g, R3 = %g, asking for %.15g V to %.15g V at %.15g ohm",
              aSeries.Name, topologies[static_cast<int>( aDivider.Topology )], aDivider.R1, aDivider.R2, aDivider.R3,
              aVin, aVout, aImpedance );

    return text;
}


/**
 * Run random two resistor dividers through the solver, for one way of specifying the impedance.
 */
static void runTwoResistorGauntlet( R_DIVIDER_IMPEDANCE aType, uint32_t aSeed )
{
    GAUNTLET_RANDOM random( aSeed );

    for( const GAUNTLET_SERIES& series : gauntletSeries() )
    {
        for( int run = 0; run < series.PairRuns; ++run )
        {
            // Anything from 1 ohm to 10 Mohm on either side, so the ratio goes from tiny to huge
            R_DIVIDER_SOLUTION divider;

            divider.R1 = random.Resistor( series.Values, random.Integer( 0, 6 ) );
            divider.R2 = random.Resistor( series.Values, random.Integer( 0, 6 ) );

            double vin = random.Voltage();
            double vout = vin * divider.R2 / ( divider.R1 + divider.R2 );
            double impedance = impedanceOf( divider, aType );

            // Asked for exactly, the divider is the only one that is right on both counts
            BOOST_TEST_CONTEXT( describe( series, divider, vin, vout, impedance ) )
            {
                std::vector<R_DIVIDER_SOLUTION> solutions =
                        SolveResistorDivider( vin, vout, impedance, aType, series.Values );

                checkSolutions( solutions, series, false, vin, vout );

                BOOST_CHECK( isSame( solutions[0].R1, divider.R1 ) );
                BOOST_CHECK( isSame( solutions[0].R2, divider.R2 ) );
            }

            // Asked for slightly off, it is still a candidate that the solver has to match or
            // beat: on the ratio alone, and on the ratio with the impedance tightly held.
            nudgeRequest( random, divider, vin, vout, impedance );

            BOOST_TEST_CONTEXT( describe( series, divider, vin, vout, impedance ) )
            {
                std::vector<R_DIVIDER_SOLUTION> solutions =
                        SolveResistorDivider( vin, vout, impedance, aType, series.Values );

                checkSolutions( solutions, series, false, vin, vout );

                double toBeat = ratioError( divider, vin, vout ) + SAME;

                BOOST_CHECK_LE( ratioError( solutions[0], vin, vout ), toBeat );

                BOOST_CHECK( std::any_of( solutions.begin(), solutions.end(),
                                          [&]( const R_DIVIDER_SOLUTION& aSolution )
                                          {
                                              return ratioError( aSolution, vin, vout ) <= toBeat
                                                     && isInTightWindow( aSolution, aType, impedance, series.Values );
                                          } ) );
            }
        }
    }
}


/**
 * Smallest ratio error that two resistors of a series can reach, found by trying them all.
 *
 * The ratio only depends on R1 / R2, so R2 stays in one decade while R1 goes four decades either
 * side of it, which is further than any ratio asked for here needs.
 */
static double closestRatioError( const std::vector<uint16_t>& aSeries, double aVin, double aVout )
{
    double closest = std::numeric_limits<double>::infinity();

    for( int decade = -4; decade <= 4; ++decade )
    {
        double scale = std::pow( 10.0, decade );

        for( uint16_t r1 : aSeries )
        {
            for( uint16_t r2 : aSeries )
                closest = std::min( closest, std::abs( r2 / ( r1 * scale + r2 ) / ( aVout / aVin ) - 1 ) );
        }
    }

    return closest;
}


BOOST_AUTO_TEST_SUITE( RDividerGauntlet )


BOOST_AUTO_TEST_CASE( TwoResistorsSeriesImpedance )
{
    runTwoResistorGauntlet( R_DIVIDER_IMPEDANCE::SERIES, 20260401 );
}


BOOST_AUTO_TEST_CASE( TwoResistorsOutputImpedance )
{
    runTwoResistorGauntlet( R_DIVIDER_IMPEDANCE::PARALLEL, 20260402 );
}


/**
 * The first solution has the closest ratio that the series can make, wherever the impedance asked
 * for puts the search.
 */
BOOST_AUTO_TEST_CASE( TwoResistorsClosestRatio )
{
    GAUNTLET_RANDOM random( 20260404 );

    for( const GAUNTLET_SERIES& series : gauntletSeries() )
    {
        for( int run = 0; run < CLOSEST_RATIO_RUNS; ++run )
        {
            // Vout / Vin from 0.001 to 0.999, crowded towards both ends
            double ratio = std::pow( 10.0, -random.Real( 0.01, 3 ) );

            if( random.Integer( 0, 1 ) )
                ratio = 1 - ratio;

            double vin = random.Voltage();
            double vout = vin * ratio;
            double impedance = std::pow( 10.0, random.Real( 2, 6 ) ); // 100 ohm to 1 Mohm

            R_DIVIDER_IMPEDANCE type = run % 2 ? R_DIVIDER_IMPEDANCE::PARALLEL : R_DIVIDER_IMPEDANCE::SERIES;

            BOOST_TEST_CONTEXT( series.Name << ", asking for " << vin << " V to " << vout << " V at " << impedance
                                            << ( run % 2 ? " ohm in parallel" : " ohm in series" ) )
            {
                std::vector<R_DIVIDER_SOLUTION> solutions =
                        SolveResistorDivider( vin, vout, impedance, type, series.Values );

                checkSolutions( solutions, series, false, vin, vout );

                BOOST_CHECK_LE( ratioError( solutions[0], vin, vout ),
                                closestRatioError( series.Values, vin, vout ) * ( 1 + 1e-9 ) + SAME );
            }
        }
    }
}


/**
 * Random three resistor dividers, going round the four arrangements, with a series impedance
 * target, which is the only one the three resistor solver handles.
 *
 * Different sets of three resistors can make up the very same two legs, so it is the legs that
 * have to come back rather than the resistors.
 */
BOOST_AUTO_TEST_CASE( ThreeResistors )
{
    static const R_DIVIDER_TOPOLOGY topologies[] = { R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM, R_DIVIDER_TOPOLOGY::SPLIT_TOP,
                                                     R_DIVIDER_TOPOLOGY::PARALLEL_BOTTOM,
                                                     R_DIVIDER_TOPOLOGY::PARALLEL_TOP };

    const R_DIVIDER_IMPEDANCE type = R_DIVIDER_IMPEDANCE::SERIES;

    GAUNTLET_RANDOM random( 20260403 );

    // The arrangements are taken in turn, carrying on from one series to the next so that the
    // series with a single run don't all get the same one
    size_t turn = 0;

    for( const GAUNTLET_SERIES& series : gauntletSeries() )
    {
        for( int run = 0; run < series.TripleRuns; ++run )
        {
            R_DIVIDER_TOPOLOGY topology = topologies[turn++ % std::size( topologies )];

            bool splitTop = topology == R_DIVIDER_TOPOLOGY::SPLIT_TOP || topology == R_DIVIDER_TOPOLOGY::PARALLEL_TOP;

            // The two resistors sharing a leg are kept within a factor of ten of each other:
            // the solver leaves at least 5 % of a leg to each of them. The other leg may be
            // up to a few decades away.
            int    decade = random.Integer( 2, 4 );
            double first = random.Resistor( series.Values, decade );
            double second = 0;

            do
            {
                second = random.Resistor( series.Values, decade + random.Integer( -1, 1 ) );
            } while( second > 10 * first || first > 10 * second );

            double single = random.Resistor( series.Values, decade + random.Integer( -2, 2 ) );

            R_DIVIDER_SOLUTION divider;

            divider.Topology = topology;
            divider.R1 = splitTop ? first : single;
            divider.R2 = splitTop ? single : first;
            divider.R3 = second;

            double vin = random.Voltage();
            double vout = vin * divider.Bottom() / ( divider.Top() + divider.Bottom() );
            double impedance = impedanceOf( divider, type );

            // Asked for exactly, both legs come back
            BOOST_TEST_CONTEXT( describe( series, divider, vin, vout, impedance ) )
            {
                std::vector<R_DIVIDER_SOLUTION> solutions =
                        SolveThreeResistorDivider( vin, vout, impedance, type, series.Values );

                checkSolutions( solutions, series, true, vin, vout );

                BOOST_CHECK( isSame( solutions[0].Top(), divider.Top() ) );
                BOOST_CHECK( isSame( solutions[0].Bottom(), divider.Bottom() ) );
            }

            // Asked for slightly off, the divider is still to be matched or beaten
            nudgeRequest( random, divider, vin, vout, impedance );

            BOOST_TEST_CONTEXT( describe( series, divider, vin, vout, impedance ) )
            {
                std::vector<R_DIVIDER_SOLUTION> solutions =
                        SolveThreeResistorDivider( vin, vout, impedance, type, series.Values );

                checkSolutions( solutions, series, true, vin, vout );

                double toBeat = ratioError( divider, vin, vout ) * FITTED_RESISTOR_LEEWAY + SAME;

                BOOST_CHECK_LE( ratioError( solutions[0], vin, vout ), toBeat );

                BOOST_CHECK( std::any_of( solutions.begin(), solutions.end(),
                                          [&]( const R_DIVIDER_SOLUTION& aSolution )
                                          {
                                              return ratioError( aSolution, vin, vout ) <= toBeat
                                                     && isInTightWindow( aSolution, type, impedance, series.Values );
                                          } ) );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
