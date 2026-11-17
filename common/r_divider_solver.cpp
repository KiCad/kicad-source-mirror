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
#include <ranges>
#include <utility>


// How far from its ideal value a leg is searched for, as a factor either way
static constexpr double SEARCH_SPAN = 10;

// Neither resistor of a pair may take under 5 % of the leg, or its tolerance is lost in the other's
static constexpr double MAX_PAIR_SPREAD = 19;

// Errors closer than this are a tie, which keeps round-off from picking between equal dividers
static constexpr double EPSILON = 1e-12;

// Vout / Vin errors shrink with the ratio, and beyond this they shrink into the ties
static constexpr double MAX_RATIO = 1e6;


/// Single resistors, or pairs of them in series and in parallel, within SEARCH_SPAN of aIdeal, sorted by value
static std::vector<R_DIVIDER_LEG> legsNear( const std::vector<double>& aResistors, double aIdeal, bool aPairs )
{
    std::vector<R_DIVIDER_LEG> legs;

    auto add =
            [&]( const R_DIVIDER_LEG& aLeg )
            {
                if( aLeg.Value >= aIdeal / SEARCH_SPAN && aLeg.Value <= aIdeal * SEARCH_SPAN )
                    legs.push_back( aLeg );
            };

    for( auto a = aResistors.begin(); a != aResistors.end(); ++a )
    {
        if( !aPairs )
        {
            add( { *a, *a } );
            continue;
        }

        for( auto b = std::lower_bound( aResistors.begin(), a, *a / MAX_PAIR_SPREAD ); b <= a; ++b )
        {
            add( { *a + *b, *a, *b, false } );
            add( { *a * *b / ( *a + *b ), *a, *b, true } );
        }
    }

    std::ranges::sort( legs, {}, &R_DIVIDER_LEG::Value );
    return legs;
}


/// The legs either side of aValue
static auto nearest( const std::vector<R_DIVIDER_LEG>& aLegs, double aValue )
{
    auto above = std::ranges::lower_bound( aLegs, aValue, {}, &R_DIVIDER_LEG::Value );
    auto below = above == aLegs.begin() ? above : std::prev( above );

    return std::ranges::subrange( below, above == aLegs.end() ? above : std::next( above ) );
}


std::vector<R_DIVIDER> FindResistorDividers( double aRatio, double aImpedance, R_DIVIDER_IMPEDANCE aImpedanceType,
                                             const std::vector<double>& aResistors, R_DIVIDER_TARGET aTarget )
{
    if( !( aRatio >= 1 / MAX_RATIO && aRatio <= MAX_RATIO ) || !( aImpedance > 0 ) || !std::isfinite( aImpedance )
        || aResistors.empty() )
    {
        return {};
    }

    const bool   parallel = aImpedanceType == R_DIVIDER_IMPEDANCE::PARALLEL;
    const double idealBottom = parallel ? aImpedance * ( 1 + aRatio ) / aRatio : aImpedance / ( 1 + aRatio );
    const double idealTop = idealBottom * aRatio;

    for( double ideal : { idealTop, idealBottom } )
    {
        if( ideal < aResistors.front() || ideal > aResistors.back() )
            return {};
    }

    auto evaluate =
            [&]( const R_DIVIDER_LEG& aTop, const R_DIVIDER_LEG& aBottom )
            {
                const double top = aTop.Value;
                const double bottom = aBottom.Value;

                R_DIVIDER divider{ aTop, aBottom };

                divider.Impedance = parallel ? top * bottom / ( top + bottom ) : top + bottom;
                divider.ImpedanceError = divider.Impedance / aImpedance - 1;

                if( aTarget == R_DIVIDER_TARGET::RESISTOR_RATIO )
                    divider.TargetError = top / bottom / aRatio - 1;
                else
                    divider.TargetError = bottom * ( 1 + aRatio ) / ( top + bottom ) - 1;

                return divider;
            };

    const std::vector<R_DIVIDER_LEG> topSingles = legsNear( aResistors, idealTop, false );
    const std::vector<R_DIVIDER_LEG> topPairs = legsNear( aResistors, idealTop, true );
    const std::vector<R_DIVIDER_LEG> bottomSingles = legsNear( aResistors, idealBottom, false );
    const std::vector<R_DIVIDER_LEG> bottomPairs = legsNear( aResistors, idealBottom, true );

    // Sparse series can't get near a tight impedance limit, so each limit also reaches a number of steps
    double step = 1;

    for( size_t i = 1; i < aResistors.size(); ++i )
        step = std::max( step, aResistors[i] / aResistors[i - 1] );

    // The leg that gives the impedance aZ along with the leg aFixed
    auto partnerFor =
            [&]( double aFixed, double aZ )
            {
                if( aZ <= 0 )
                    return 0.0;

                if( !parallel )
                    return std::max( aZ - aFixed, 0.0 );

                return aZ < aFixed ? aFixed * aZ / ( aFixed - aZ ) : std::numeric_limits<double>::infinity();
            };

    auto better =
            []( const R_DIVIDER& aFirst, const R_DIVIDER& aSecond )
            {
                const double first = std::abs( aFirst.TargetError );
                const double second = std::abs( aSecond.TargetError );

                if( std::abs( first - second ) > EPSILON )
                    return first < second;

                return std::abs( aFirst.ImpedanceError ) < std::abs( aSecond.ImpedanceError ) - EPSILON;
            };

    auto twoResistors =
            []( const R_DIVIDER& aDivider )
            {
                return aDivider.Top.B == 0 && aDivider.Bottom.B == 0;
            };

    std::vector<R_DIVIDER> results;

    auto keepBest =
            [&]( auto&& aDividers )
            {
                auto best = std::ranges::min_element( aDividers, better );

                if( best != std::ranges::end( aDividers ) && std::ranges::find( results, *best ) == results.end() )
                    results.push_back( *best );
            };

    const std::pair<double, int> limits[] = {
        { std::numeric_limits<double>::infinity(), 0 }, { 0.3, 3 }, { 0.1, 2 }, { 0.01, 0 }
    };

    for( const auto& [tolerance, steps] : limits )
    {
        const double reach = std::max( tolerance, std::pow( step, steps ) - 1 ) + EPSILON;

        // The ratio error grows away from the ideal leg and the impedance grows with the leg, so the best leg
        // within the limit is next to the ideal one clamped to the limit
        std::vector<R_DIVIDER> candidates;

        auto addNearest =
                [&]( const R_DIVIDER_LEG& aSingle, const std::vector<R_DIVIDER_LEG>& aLegs, bool aSingleOnTop )
                {
                    const double ideal = aSingleOnTop ? aSingle.Value / aRatio : aSingle.Value * aRatio;
                    const double lowest = partnerFor( aSingle.Value, aImpedance * ( 1 - reach ) );
                    const double highest = partnerFor( aSingle.Value, aImpedance * ( 1 + reach ) );

                    for( const R_DIVIDER_LEG& leg : nearest( aLegs, std::clamp( ideal, lowest, highest ) ) )
                        candidates.push_back( aSingleOnTop ? evaluate( aSingle, leg ) : evaluate( leg, aSingle ) );
                };

        for( const R_DIVIDER_LEG& top : topSingles )
        {
            addNearest( top, bottomSingles, true );
            addNearest( top, bottomPairs, true );
        }

        for( const R_DIVIDER_LEG& bottom : bottomSingles )
            addNearest( bottom, topPairs, false );

        auto fits = candidates
                    | std::views::filter(
                            [reach]( const R_DIVIDER& aDivider )
                            {
                                return std::abs( aDivider.ImpedanceError ) <= reach;
                            } );

        keepBest( fits | std::views::filter( twoResistors ) );
        keepBest( fits );
    }

    std::stable_partition( results.begin(), results.end(), twoResistors );
    return results;
}
