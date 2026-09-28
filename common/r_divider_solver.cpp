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

#include <r_divider_solver.h>

#include <algorithm>
#include <cmath>
#include <limits>


// Impedance windows of solutions 2 and 3: the larger of a fixed tolerance and a number of series
// steps either side of the target.
static constexpr double LOOSE_WINDOW_TOLERANCE = 0.3;
static constexpr int    LOOSE_WINDOW_STEPS = 3;
static constexpr double TIGHT_WINDOW_TOLERANCE = 0.1;
static constexpr int    TIGHT_WINDOW_STEPS = 2;

// Scoring of solutions 4 and 5, in points. Lower is better.
static constexpr double RATIO_ERROR_POINTS = 1;       // per percent of ratio error
static constexpr double IMPEDANCE_FREE_PERCENT = 1;   // impedance error that costs nothing
static constexpr double LIGHT_IMPEDANCE_POINTS = 0.1; // per percent of impedance error, solution 4
static constexpr double HEAVY_IMPEDANCE_POINTS = 0.5; // per percent of impedance error, solution 5
static constexpr double REUSED_RESISTOR_POINTS = 3;   // per resistor shared with solutions 1 to 3

// Three resistor dividers: the share of the split leg that R1 or R2 may take, leaving the rest to R3.
// It's silly to allow values of wildly different magnitudes to be used, because the precision
// of the small resistor will be swamped by the error of the large resistor
static constexpr double SERIES_LEG_SHARE_MIN = 0.05;
static constexpr double SERIES_LEG_SHARE_MAX = 0.95;

// The same for two resistors in parallel, where it is the conductance of the leg that they share
static constexpr double PARALLEL_LEG_SHARE_MIN = 0.05;
static constexpr double PARALLEL_LEG_SHARE_MAX = 0.95;

// Impedance error allowed for the last of the three resistor solutions
static constexpr double EXACT_WINDOW_TOLERANCE = 0.01;

// Candidates that are equally good on paper come out of the floating point arithmetic a few
// units in the last place apart. Values closer than this, relative to their size or absolutely
// for errors below one, are taken to be equal, so that the choice between them is left to the
// tiebreakers rather than to round-off. Limits are widened or narrowed by the same amount so that
// a value which lands exactly on one is consistently in or out.
static constexpr double TIE_EPSILON = 1e-13;


/**
 * True if \a aFirst is smaller than \a aSecond by more than round-off. Meant for errors and
 * scores, which are dimensionless.
 */
static bool isLess( double aFirst, double aSecond )
{
    return aFirst < aSecond - TIE_EPSILON * std::max( 1.0, std::abs( aSecond ) );
}


// The range of the resistors that are offered: series values 100 to 999 times 10^decade, from
// 1 mOhm to just under 1 TOhm. Requests that call for resistors outside it are refused.
static constexpr int LOWEST_DECADE = -5;
static constexpr int HIGHEST_DECADE = 9;


/**
 * List every resistor of a series within the offered range, in ascending order.
 *
 * Every resistor comes from this list, so that the same resistor always has the same value and
 * resistors can be compared for equality.
 */
static std::vector<double> standardResistors( const std::vector<uint16_t>& aSeries )
{
    std::vector<double> resistors;

    resistors.reserve( ( HIGHEST_DECADE - LOWEST_DECADE + 1 ) * aSeries.size() );

    for( int decade = LOWEST_DECADE; decade <= HIGHEST_DECADE; ++decade )
    {
        // Dividing by an exact power of ten rounds once, multiplying by an inexact one would not
        for( uint16_t value : aSeries )
            resistors.push_back( decade >= 0 ? value * std::pow( 10.0, decade ) : value / std::pow( 10.0, -decade ) );
    }

    return resistors;
}


/// True if \a aResistance lies within the range of \a aResistors
static bool isInRange( const std::vector<double>& aResistors, double aResistance )
{
    return aResistance >= aResistors.front() && aResistance <= aResistors.back();
}


/**
 * The largest ratio between two neighbouring values of a series, including the step from the last
 * value of one decade to the first of the next.
 *
 * The nominal step of a series with N values is 10^(1/N), but the values are rounded, so the
 * largest step is somewhat bigger: 15/13 rather than 1.1 for E24.
 */
static double largestStep( const std::vector<uint16_t>& aSeries )
{
    double step = 10.0 * aSeries.front() / aSeries.back();

    for( size_t i = 1; i < aSeries.size(); ++i )
        step = std::max( step, static_cast<double>( aSeries[i] ) / aSeries[i - 1] );

    return step;
}


/**
 * How far either side of its ideal value a resistor is searched for, as a multiplier.
 *
 * These values are multiplicative. For example, if the center of our search space is a 1k, a span of 5 means we
 * include resistors between 200 and 5k in our search.
 *
 * Sparse series need a wide search to find a good ratio at all, dense ones are narrowed to keep
 * the number of combinations sensible.
 */
static int searchSpan( size_t aSeriesSize )
{
    if( aSeriesSize <= 3 )
        return 15;
    else if( aSeriesSize <= 48 )
        return 10;
    else if( aSeriesSize <= 96 )
        return 5;
    else // E192
        return 4;
}


/**
 * List the resistors of \a aResistors that lie strictly between \a aMin and \a aMax.
 */
static std::vector<double> resistorsBetween( const std::vector<double>& aResistors, double aMin, double aMax )
{
    auto first = std::upper_bound( aResistors.begin(), aResistors.end(), aMin * ( 1 + TIE_EPSILON ) );
    auto last = std::lower_bound( first, aResistors.end(), aMax * ( 1 - TIE_EPSILON ) );

    return std::vector<double>( first, last );
}


/**
 * Helper that returns the acceptable candidate with the lowest score, or nullptr if none is acceptable.
 * The caller must provide the helpers, ACCEPT to filter out candidates that are not acceptable, and SCORE to score
 * candidates
 *
 * Equal scores are settled by the impedance error, then by the order of the candidates. Scores
 * within round-off of each other count as equal; that is not transitive, so in a run of nearly
 * equal scores the order of the candidates matters, but it is a fixed order.
 */
template <typename ACCEPT, typename SCORE>
static const R_DIVIDER_RESULT* pickBest( const std::vector<R_DIVIDER_RESULT>& aCandidates, ACCEPT aAccept,
                                         SCORE aScore )
{
    const R_DIVIDER_RESULT* best = nullptr;
    double                  bestScore = 0;

    for( const R_DIVIDER_RESULT& candidate : aCandidates )
    {
        if( !aAccept( candidate ) )
            continue;

        const double score = aScore( candidate );

        if( !best || isLess( score, bestScore )
            || ( !isLess( bestScore, score )
                 && isLess( std::abs( candidate.ImpedanceError ), std::abs( best->ImpedanceError ) ) ) )
        {
            best = &candidate;
            bestScore = score;
        }
    }

    return best;
}


/**
 * Return the resistor of \a aResistors closest to \a aTarget. The smaller one wins if two are
 * equally close.
 *
 * @param aByConductance measures the distance between conductances rather than resistances, which
 *                       is what counts for a resistor that goes in parallel with another.
 */
static double nearestResistor( const std::vector<double>& aResistors, double aTarget, bool aByConductance = false )
{
    auto above = std::lower_bound( aResistors.begin(), aResistors.end(), aTarget );

    // Beyond either end of the list there is only one neighbour
    if( above == aResistors.end() )
        return aResistors.back();

    if( above == aResistors.begin() )
        return aResistors.front();

    const double higher = *above;
    const double lower = *( above - 1 );

    const double toHigher = aByConductance ? 1 / aTarget - 1 / higher : higher - aTarget;
    const double toLower = aByConductance ? 1 / lower - 1 / aTarget : aTarget - lower;

    return toHigher < toLower - TIE_EPSILON * ( std::abs( toHigher ) + std::abs( toLower ) ) ? higher : lower;
}


/**
 * Accepts candidates whose impedance is within \a aTolerance or \a aSteps series steps of the
 * target, whichever reaches further.
 */
struct IMPEDANCE_WINDOW
{
    IMPEDANCE_WINDOW( double aTarget, double aStep, double aTolerance, int aSteps )
    {
        const double factor = std::pow( aStep, aSteps );

        Lowest = std::min( aTarget * ( 1 - aTolerance ), aTarget / factor ) * ( 1 - TIE_EPSILON );
        Highest = std::max( aTarget * ( 1 + aTolerance ), aTarget * factor ) * ( 1 + TIE_EPSILON );
    }

    bool operator()( const R_DIVIDER_RESULT& aCandidate ) const
    {
        return aCandidate.Impedance >= Lowest && aCandidate.Impedance <= Highest;
    }

    double Lowest;
    double Highest;
};


static bool isUsableInput( double aVin, double aVout, double aImpedance, const std::vector<uint16_t>& aSeries )
{
    // Comparisons with NaN are false, so NaN is rejected too. Vout / Vin can underflow to 0, but
    // can't round up to 1 while Vout < Vin.
    return aVout > 0 && aVout < aVin && aVout / aVin > 0 && aImpedance > 0 && std::isfinite( aVin )
           && std::isfinite( aImpedance ) && !aSeries.empty();
}


/**
 * What a divider is measured against. The ratio is kept both ways round so that the error is
 * measured on what the user asked for.
 */
struct DIVIDER_REQUEST
{
    double              Ratio;         // Vout / Vin
    double              ResistorRatio; // R1 / R2, that is ( Vin - Vout ) / Vout
    double              Impedance;
    R_DIVIDER_IMPEDANCE ImpedanceType;
    R_DIVIDER_TARGET    Target;
};


/**
 * Compute the impedance and the errors of \a aSolution.
 *
 * The solvers rank their candidates by the same numbers that are reported, so a divider they
 * find exact is reported with no error.
 */
static R_DIVIDER_RESULT evaluate( const R_DIVIDER_SOLUTION& aSolution, const DIVIDER_REQUEST& aRequest )
{
    const double top = aSolution.Top();
    const double bottom = aSolution.Bottom();

    R_DIVIDER_RESULT result;

    result.Solution = aSolution;
    result.Impedance = aRequest.ImpedanceType == R_DIVIDER_IMPEDANCE::PARALLEL ? top * bottom / ( top + bottom )
                                                                               : top + bottom;
    result.ImpedanceError = result.Impedance / aRequest.Impedance - 1;

    if( aRequest.Target == R_DIVIDER_TARGET::RESISTOR_RATIO )
        result.TargetError = top / bottom / aRequest.ResistorRatio - 1;
    else
        result.TargetError = bottom / ( top + bottom ) / aRequest.Ratio - 1;

    return result;
}


static double absTargetError( const R_DIVIDER_RESULT& aCandidate )
{
    return std::abs( aCandidate.TargetError );
}


static bool anyImpedance( const R_DIVIDER_RESULT& )
{
    return true;
}


/**
 * The solutions of \a aPicks in order, each one only once. The same divider often wins more than
 * one pick. A nullptr, where a pick found nothing, is skipped.
 */
static std::vector<R_DIVIDER_SOLUTION> uniqueSolutions( const std::vector<const R_DIVIDER_RESULT*>& aPicks )
{
    std::vector<R_DIVIDER_SOLUTION> solutions;

    for( const R_DIVIDER_RESULT* picked : aPicks )
    {
        if( picked && std::find( solutions.begin(), solutions.end(), picked->Solution ) == solutions.end() )
            solutions.push_back( picked->Solution );
    }

    return solutions;
}


std::vector<R_DIVIDER_SOLUTION> SolveResistorDivider( double aVin, double aVout, double aImpedance,
                                                      R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                      const std::vector<uint16_t>& aSeries, R_DIVIDER_TARGET aTarget )
{
    if( !isUsableInput( aVin, aVout, aImpedance, aSeries ) )
        return {};

    // --------- Step 1: Evaluation of every divider --------

    const double targetRatio = aVout / aVin;

    const DIVIDER_REQUEST request{ targetRatio, ( aVin - aVout ) / aVout, aImpedance, aImpedanceType, aTarget };

    const double step = largestStep( aSeries );
    const int    span = searchSpan( aSeries.size() );

    const bool parallel = aImpedanceType == R_DIVIDER_IMPEDANCE::PARALLEL;

    // With R1 + R2 = Z the ideal resistors are R2 = r * Z and R1 = ( 1 - r ) * Z. With
    // R1 || R2 = Z they are R1 = Z / r and R2 = Z / ( 1 - r ). Look for each one within the
    // search span of its ideal value.
    //
    // This treats R1 and R2 alike, and the errors below are measured on the quantity the user
    // sees, so dividers below and above one half need no separate handling.
    const double idealR1 = parallel ? aImpedance / targetRatio : ( 1 - targetRatio ) * aImpedance;
    const double idealR2 = parallel ? aImpedance / ( 1 - targetRatio ) : targetRatio * aImpedance;

    const std::vector<double> resistors = standardResistors( aSeries );

    if( !isInRange( resistors, idealR1 ) || !isInRange( resistors, idealR2 ) )
        return {};

    const std::vector<double> r1Values = resistorsBetween( resistors, idealR1 / span, idealR1 * span );
    const std::vector<double> r2Values = resistorsBetween( resistors, idealR2 / span, idealR2 * span );

    std::vector<R_DIVIDER_RESULT> candidates;
    candidates.reserve( r1Values.size() * r2Values.size() );

    for( double r1 : r1Values )
    {
        for( double r2 : r2Values )
            candidates.push_back( evaluate( { r1, r2 }, request ) );
    }

    // ---------- Step 2: Search for the best ratio ---------

    std::vector<const R_DIVIDER_RESULT*> picks; // nullptr where nothing was found

    // Solutions 1 to 3: the best ratio, with the impedance progressively reined in
    const IMPEDANCE_WINDOW looseImpedance( aImpedance, step, LOOSE_WINDOW_TOLERANCE, LOOSE_WINDOW_STEPS );
    const IMPEDANCE_WINDOW tightImpedance( aImpedance, step, TIGHT_WINDOW_TOLERANCE, TIGHT_WINDOW_STEPS );

    picks.push_back( pickBest( candidates, anyImpedance, absTargetError ) );
    picks.push_back( pickBest( candidates, looseImpedance, absTargetError ) );
    picks.push_back( pickBest( candidates, tightImpedance, absTargetError ) );

    // ---------- Step 3: Search for the best compromise ---------

    // Solutions 4 and 5: trade ratio error against impedance error, preferring resistor values
    // that the solutions above don't already offer
    std::vector<double> usedResistors;

    for( const R_DIVIDER_RESULT* used : picks )
    {
        if( used )
        {
            usedResistors.push_back( used->Solution.R1 );
            usedResistors.push_back( used->Solution.R2 );
        }
    }

    auto isUsed =
            [&]( double aResistor )
            {
                return std::find( usedResistors.begin(), usedResistors.end(), aResistor ) != usedResistors.end();
            };

    auto weightedScore =
            [&]( double aImpedancePoints )
            {
                return [&, aImpedancePoints]( const R_DIVIDER_RESULT& aCandidate )
                       {
                           const int    reused = isUsed( aCandidate.Solution.R1 ) + isUsed( aCandidate.Solution.R2 );
                           const double impedancePercent =
                                   100 * std::abs( aCandidate.ImpedanceError ) - IMPEDANCE_FREE_PERCENT;

                           return RATIO_ERROR_POINTS * 100 * std::abs( aCandidate.TargetError )
                                  + aImpedancePoints * std::max( impedancePercent, 0.0 )
                                  + REUSED_RESISTOR_POINTS * reused;
                       };
            };

    picks.push_back( pickBest( candidates, anyImpedance, weightedScore( LIGHT_IMPEDANCE_POINTS ) ) );
    picks.push_back( pickBest( candidates, anyImpedance, weightedScore( HEAVY_IMPEDANCE_POINTS ) ) );

    return uniqueSolutions( picks );
}


std::vector<R_DIVIDER_SOLUTION> SolveThreeResistorDivider( double aVin, double aVout, double aImpedance,
                                                           R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                           const std::vector<uint16_t>& aSeries,
                                                           R_DIVIDER_TARGET             aTarget )
{
    if( !isUsableInput( aVin, aVout, aImpedance, aSeries ) )
        return {};

    if( aImpedanceType != R_DIVIDER_IMPEDANCE::SERIES )
        return {};

    // --------- Step 1: Evaluation of every divider --------

    const double targetRatio = aVout / aVin;

    const DIVIDER_REQUEST request{ targetRatio, ( aVin - aVout ) / aVout, aImpedance, aImpedanceType, aTarget };

    const double step = largestStep( aSeries );
    const int    span = searchSpan( aSeries.size() );

    const std::vector<double> resistors = standardResistors( aSeries );

    // The ideal two resistor divider, which each leg is close to
    const double idealR1 = ( 1 - targetRatio ) * aImpedance;
    const double idealR2 = targetRatio * aImpedance;

    if( !isInRange( resistors, idealR1 ) || !isInRange( resistors, idealR2 ) )
        return {};

    // The single resistor of each divider is searched for around its value in the ideal two
    // resistor divider. It fixes the leg the other two must make up for the ratio to be exact.
    // One of those two takes every value that makes up 5 % to 95 % of that leg, of its resistance
    // in series or of its conductance in parallel, and the other the value nearest to the rest.
    //
    // The two resistors of a leg are interchangeable, so the larger is always stored first and
    // the smaller in R3. The same set of resistors then always reads the same.
    //
    // The order of the candidates settles the ties that the errors leave.
    const std::vector<double> r1Values = resistorsBetween( resistors, idealR1 / span, idealR1 * span );
    const std::vector<double> r2Values = resistorsBetween( resistors, idealR2 / span, idealR2 * span );

    std::vector<R_DIVIDER_RESULT> candidates;

    // R1 on top, R2 and R3 in series to ground
    for( double r1 : r1Values )
    {
        const double bottom = r1 / request.ResistorRatio;

        for( double r2 : resistorsBetween( resistors, bottom * SERIES_LEG_SHARE_MIN, bottom * SERIES_LEG_SHARE_MAX ) )
        {
            const double r3 = nearestResistor( resistors, bottom - r2 );

            candidates.push_back( evaluate( { r1, std::max( r2, r3 ), std::min( r2, r3 ),
                                              R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM },
                                            request ) );
        }
    }

    // R1 and R3 in series on top, R2 to ground
    for( double r2 : r2Values )
    {
        const double top = r2 * request.ResistorRatio;

        for( double r1 : resistorsBetween( resistors, top * SERIES_LEG_SHARE_MIN, top * SERIES_LEG_SHARE_MAX ) )
        {
            const double r3 = nearestResistor( resistors, top - r1 );

            candidates.push_back( evaluate( { std::max( r1, r3 ), r2, std::min( r1, r3 ),
                                              R_DIVIDER_TOPOLOGY::SPLIT_TOP },
                                            request ) );
        }
    }

    // R1 on top, R2 and R3 in parallel to ground. A resistor carrying a share of the current of a
    // leg is larger than the leg by that share.
    for( double r1 : r1Values )
    {
        const double bottom = r1 / request.ResistorRatio;

        for( double r2 :
             resistorsBetween( resistors, bottom / PARALLEL_LEG_SHARE_MAX, bottom / PARALLEL_LEG_SHARE_MIN ) )
        {
            // 1 / R3 = 1 / bottom - 1 / R2
            const double r3 = nearestResistor( resistors, bottom * r2 / ( r2 - bottom ), true );

            candidates.push_back( evaluate( { r1, std::max( r2, r3 ), std::min( r2, r3 ),
                                              R_DIVIDER_TOPOLOGY::PARALLEL_BOTTOM },
                                            request ) );
        }
    }

    // R1 and R3 in parallel on top, R2 to ground
    for( double r2 : r2Values )
    {
        const double top = r2 * request.ResistorRatio;

        for( double r1 : resistorsBetween( resistors, top / PARALLEL_LEG_SHARE_MAX, top / PARALLEL_LEG_SHARE_MIN ) )
        {
            // 1 / R3 = 1 / top - 1 / R1
            const double r3 = nearestResistor( resistors, top * r1 / ( r1 - top ), true );

            candidates.push_back( evaluate( { std::max( r1, r3 ), r2, std::min( r1, r3 ),
                                              R_DIVIDER_TOPOLOGY::PARALLEL_TOP },
                                            request ) );
        }
    }

    // ---------- Step 2: Search for the best ratio ---------

    auto exactImpedance =
            []( const R_DIVIDER_RESULT& aCandidate )
            {
                return std::abs( aCandidate.ImpedanceError ) <= EXACT_WINDOW_TOLERANCE + TIE_EPSILON;
            };

    const IMPEDANCE_WINDOW looseImpedance( aImpedance, step, LOOSE_WINDOW_TOLERANCE, LOOSE_WINDOW_STEPS );
    const IMPEDANCE_WINDOW tightImpedance( aImpedance, step, TIGHT_WINDOW_TOLERANCE, TIGHT_WINDOW_STEPS );

    // The best ratio, with the impedance progressively reined in
    const std::vector<const R_DIVIDER_RESULT*> picks = { pickBest( candidates, anyImpedance, absTargetError ),
                                                         pickBest( candidates, looseImpedance, absTargetError ),
                                                         pickBest( candidates, tightImpedance, absTargetError ),
                                                         pickBest( candidates, exactImpedance, absTargetError ) };

    return uniqueSolutions( picks );
}


std::vector<R_DIVIDER_RESULT> FindResistorDividers( double aVin, double aVout, double aImpedance,
                                                    R_DIVIDER_IMPEDANCE          aImpedanceType,
                                                    const std::vector<uint16_t>& aSeries, R_DIVIDER_TARGET aTarget )
{
    // Keeps two results that are equally good from beating each other on round-off
    static constexpr double MARGIN = 1e-11;

    if( !isUsableInput( aVin, aVout, aImpedance, aSeries ) )
        return {};

    const double targetRatio = aVout / aVin;

    const DIVIDER_REQUEST request{ targetRatio, ( aVin - aVout ) / aVout, aImpedance, aImpedanceType, aTarget };

    std::vector<R_DIVIDER_RESULT> results;

    double bestTargetError = std::numeric_limits<double>::infinity();
    double bestImpedanceError = std::numeric_limits<double>::infinity();

    // Get results from the two-resistor topologies, and add them to the results array
    for( const R_DIVIDER_SOLUTION& solution :
         SolveResistorDivider( aVin, aVout, aImpedance, aImpedanceType, aSeries, aTarget ) )
    {
        results.push_back( evaluate( solution, request ) );

        // Store the best performance figures from the two-resistor topologies
        bestTargetError = std::min( bestTargetError, std::abs( results.back().TargetError ) );
        bestImpedanceError = std::min( bestImpedanceError, std::abs( results.back().ImpedanceError ) );
    }

    // Get results from the three resistor topologies, and add them to the results array.
    for( const R_DIVIDER_SOLUTION& solution :
         SolveThreeResistorDivider( aVin, aVout, aImpedance, aImpedanceType, aSeries, aTarget ) )
    {
        const R_DIVIDER_RESULT result = evaluate( solution, request );

        // Only solutions that are better then all of the two-resistor solutions on at least one metric
        // are shown to the user
        if( std::abs( result.TargetError ) < bestTargetError - MARGIN
            || std::abs( result.ImpedanceError ) < bestImpedanceError - MARGIN )
        {
            results.push_back( result );
        }
    }

    return results;
}
