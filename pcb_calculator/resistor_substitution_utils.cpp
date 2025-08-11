/*
 * This program source code file
 * is part of KiCad, a free EDA CAD application.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resistor_substitution_utils.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <optional>

// If BENCHMARK is defined, calculations will print their execution time to the STDERR
// #define BENCHMARK

#ifdef BENCHMARK
#include <core/profile.h>
#endif

// Comparison operators used by std::sort and std::lower_bound
bool operator<( const RESISTANCE& aLhs, double aRhs )
{
    return aLhs.value < aRhs;
}

bool operator<( const RESISTANCE& aLhs, const RESISTANCE& aRhs )
{
    return aLhs.value < aRhs.value;
}

class SolutionCollector
/**
 * Helper class that collects solutions and keeps one with the best deviation.
 * In order to avoid performing costly string operations too frequently,
 * they are postponed until the very end, when we know the best combination.
 */
{
public:
    SolutionCollector( double aTarget ) :
            m_target( aTarget )
    {
    }

    /**
     * Add two solutions, based on single 2R buffer lookup, to the collector.
     *
     * @param aResults are the resistances found in 2R buffer
     * @param aValueFunc transforms value from aResults into final value of the combination
     * @param aResultFunc transforms RESISTANCE instance from aResults into final instance
     */
    void Add2RLookupResults( std::pair<RESISTANCE&, RESISTANCE&> aResults, std::function<double( double )> aValueFunc,
                             std::function<RESISTANCE( RESISTANCE& )> aResultFunc )
    {
        considerSolution( aValueFunc( aResults.first.value ), aResults.first, aResultFunc );
        considerSolution( aValueFunc( aResults.second.value ), aResults.second, aResultFunc );
    }

    /**
     * Return the best collected combination.
     */
    RESISTANCE GetBest()
    {
        if( !m_best_solution )
            throw std::logic_error( "Empty solution collector" );

        return *m_best_solution;
    }

private:
    void considerSolution( double aValue, RESISTANCE& aFound, std::function<RESISTANCE( RESISTANCE& )>& aResultFunc )
    {
        double deviation = std::abs( aValue - m_target );

        if( deviation + epsilon < m_best_deviation )
        {
            m_best_deviation = deviation;
            m_best_solution = aResultFunc( aFound );
        }
        else if( std::abs( deviation - m_best_deviation ) < epsilon )
        {
            RESISTANCE candidate = aResultFunc( aFound );

            if( !m_best_solution || betterCandidate( candidate, *m_best_solution ) )
                m_best_solution = std::move( candidate );
        }
    }

    static int uniqueCount( const RESISTANCE& aRes )
    {
        std::vector<double> parts = aRes.parts;
        std::sort( parts.begin(), parts.end() );

        int    count = 0;
        double last = 0.0;
        bool   first = true;

        for( double v : parts )
        {
            if( first || std::abs( v - last ) > epsilon )
            {
                count++;
                last = v;
                first = false;
            }
        }

        return count;
    }

    static bool betterCandidate( const RESISTANCE& aCand, const RESISTANCE& aBest )
    {
        int candUnique = uniqueCount( aCand );
        int bestUnique = uniqueCount( aBest );

        if( candUnique != bestUnique )
            return candUnique < bestUnique;

        if( aCand.parts.size() != aBest.parts.size() )
            return aCand.parts.size() < aBest.parts.size();

        return aCand.name < aBest.name;
    }

    double                    m_target;
    double                    m_best_deviation = INFINITY;
    std::optional<RESISTANCE> m_best_solution;
};

/**
 * If aText contains aRequiredSymbol as top-level (i.e. not in parentheses) operator,
 * return aText enclosed in parentheses.
 * Otherwise, return aText unmodified.
 */
static std::string maybeEmbrace( const std::string& aText, char aRequiredSymbol )
{
    bool shouldEmbrace = false;

    // scan for required top-level symbol
    int parenLevel = 0;

    for( char c : aText )
    {
        if( c == '(' )
            parenLevel++;
        else if( c == ')' )
            parenLevel--;
        else if( c == aRequiredSymbol && parenLevel == 0 )
            shouldEmbrace = true;
    }

    // embrace or not
    if( shouldEmbrace )
        return '(' + aText + ')';
    else
        return aText;
}

/**
 * Functions calculating values and text representations of serial and parallel combinations.
 * Functions marked as 'Simple' do not care about parentheses, which makes them faster.
 */

static inline double serialValue( double aR1, double aR2 )
{
    return aR1 + aR2;
}

static inline double parallelValue( double aR1, double aR2 )
{
    return aR1 * aR2 / ( aR1 + aR2 );
}

static inline RESISTANCE serialResistance( const RESISTANCE& aR1, const RESISTANCE& aR2 )
{
    std::string         name = maybeEmbrace( aR1.name, '|' ) + " + " + maybeEmbrace( aR2.name, '|' );
    std::vector<double> parts = aR1.parts;
    parts.insert( parts.end(), aR2.parts.begin(), aR2.parts.end() );
    return RESISTANCE( serialValue( aR1.value, aR2.value ), name, parts );
}

static inline RESISTANCE parallelResistance( const RESISTANCE& aR1, const RESISTANCE& aR2 )
{
    std::string         name = maybeEmbrace( aR1.name, '+' ) + " | " + maybeEmbrace( aR2.name, '+' );
    std::vector<double> parts = aR1.parts;
    parts.insert( parts.end(), aR2.parts.begin(), aR2.parts.end() );
    return RESISTANCE( parallelValue( aR1.value, aR2.value ), name, parts );
}

static inline RESISTANCE serialResistanceSimple( const RESISTANCE& aR1, const RESISTANCE& aR2 )
{
    std::string         name = aR1.name + " + " + aR2.name;
    std::vector<double> parts = aR1.parts;
    parts.insert( parts.end(), aR2.parts.begin(), aR2.parts.end() );
    return RESISTANCE( serialValue( aR1.value, aR2.value ), name, parts );
}

static inline RESISTANCE parallelResistanceSimple( const RESISTANCE& aR1, const RESISTANCE& aR2 )
{
    std::string         name = aR1.name + " | " + aR2.name;
    std::vector<double> parts = aR1.parts;
    parts.insert( parts.end(), aR2.parts.begin(), aR2.parts.end() );
    return RESISTANCE( parallelValue( aR1.value, aR2.value ), name, parts );
}

// Return a string from aValue (aValue is expected in ohms).
// If aValue < 1000 the returned string is aValue with unit = R.
// If aValue >= 1000 the returned string is aValue/1000 with unit = K
// with notation similar to 2K2.
// If aValue >= 1e6 the returned string is aValue/1e6 with unit = M
// with notation = 1M.
static std::string strValue( double aValue )
{
    std::string result;

    if( aValue < 1000.0 )
    {
        result = std::to_string( static_cast<int>( aValue ) );
        result += 'R';
    }
    else
    {
        double div = 1e3;
        char   unit = 'K';

        if( aValue >= 1e6 )
        {
            div = 1e6;
            unit = 'M';
        }

        aValue /= div;

        int valueAsInt = static_cast<int>( aValue );
        result = std::to_string( valueAsInt );
        result += unit;

        // Add mantissa: 1 digit, suitable for series up to E24
        double mantissa = aValue - valueAsInt;

        if( mantissa > 0 )
            result += std::to_string( lround( mantissa * 10 ) );
    }

    return result;
}


RES_EQUIV_CALC::RES_EQUIV_CALC()
{
    // series must be added to vector in correct order
    m_e_series.push_back( buildSeriesData( ESERIES::E1_VALUES() ) );
    m_e_series.push_back( buildSeriesData( ESERIES::E3_VALUES() ) );
    m_e_series.push_back( buildSeriesData( ESERIES::E6_VALUES() ) );
    m_e_series.push_back( buildSeriesData( ESERIES::E12_VALUES() ) );
    m_e_series.push_back( buildSeriesData( ESERIES::E24_VALUES() ) );
}

void RES_EQUIV_CALC::SetSeries( uint32_t aSeries )
{
    m_series = aSeries;
}

void RES_EQUIV_CALC::NewCalc( double aTargetValue )
{
    m_target = aTargetValue;

    m_exclude_mask.resize( m_e_series[m_series].size() );
    std::fill( m_exclude_mask.begin(), m_exclude_mask.end(), false );

    std::fill( m_results.begin(), m_results.end(), std::nullopt );
}

void RES_EQUIV_CALC::Exclude( double aValue )
{
    if( std::isnan( aValue ) )
        return;

    std::vector<RESISTANCE>& series = m_e_series[m_series];
    auto                     it = std::lower_bound( series.begin(), series.end(), aValue - epsilon );

    if( it != series.end() && std::abs( it->value - aValue ) < epsilon )
        m_exclude_mask[it - series.begin()] = true;
}

void RES_EQUIV_CALC::Calculate()
{
#ifdef BENCHMARK
    PROF_TIMER timer( "Resistor calculation" );
#endif

    prepare1RBuffer();
    prepare2RBuffer();

    RESISTANCE solution_2r = calculate2RSolution();
    m_results[S2R] = solution_2r;

    if( std::abs( solution_2r.value - m_target ) > epsilon )
    {
        RESISTANCE solution_3r = calculate3RSolution();
        m_results[S3R] = solution_3r;

        if( std::abs( solution_3r.value - m_target ) > epsilon )
            m_results[S4R] = calculate4RSolution();
    }

#ifdef BENCHMARK
    timer.Show();
#endif
}

std::vector<RESISTANCE> RES_EQUIV_CALC::buildSeriesData( const ESERIES::ESERIES_VALUES& aList )
{
    std::vector<RESISTANCE> result_list;

    for( double curr_decade = RES_EQUIV_CALC_FIRST_VALUE;; curr_decade *= 10.0 ) // iterate over decades
    {
        double multiplier = curr_decade / aList[0];

        for( const uint16_t listvalue : aList ) // iterate over values in decade
        {
            double value = multiplier * listvalue;
            result_list.emplace_back( value, strValue( value ) );

            if( value >= RES_EQUIV_CALC_LAST_VALUE )
                return result_list;
        }
    }
}

void RES_EQUIV_CALC::prepare1RBuffer()
{
    std::vector<RESISTANCE>& series = m_e_series[m_series];
    m_buffer_1R.clear();

    for( size_t i = 0; i < series.size(); i++ )
    {
        if( !m_exclude_mask[i] )
            m_buffer_1R.push_back( series[i] );
    }
}

void RES_EQUIV_CALC::prepare2RBuffer()
{
    m_buffer_2R.clear();

    for( size_t i1 = 0; i1 < m_buffer_1R.size(); i1++ )
    {
        for( size_t i2 = i1; i2 < m_buffer_1R.size(); i2++ )
        {
            m_buffer_2R.push_back( serialResistanceSimple( m_buffer_1R[i1], m_buffer_1R[i2] ) );
            m_buffer_2R.push_back( parallelResistanceSimple( m_buffer_1R[i1], m_buffer_1R[i2] ) );
        }
    }

    std::sort( m_buffer_2R.begin(), m_buffer_2R.end() );
}

std::pair<RESISTANCE&, RESISTANCE&> RES_EQUIV_CALC::findIn2RBuffer( double aTarget )
{
    // in case of NaN, return anything valid
    if( std::isnan( aTarget ) )
        return { m_buffer_2R[0], m_buffer_2R[0] };

    // target value is often too small or too big, so check that manually
    if( aTarget <= m_buffer_2R.front().value || aTarget >= m_buffer_2R.back().value )
        return { m_buffer_2R.front(), m_buffer_2R.back() };

    auto it = std::lower_bound( m_buffer_2R.begin(), m_buffer_2R.end(), aTarget ) - m_buffer_2R.begin();

    if( it == 0 )
        return { m_buffer_2R[0], m_buffer_2R[0] };
    else if( it == m_buffer_2R.size() )
        return { m_buffer_2R[it - 1], m_buffer_2R[it - 1] };
    else
        return { m_buffer_2R[it - 1], m_buffer_2R[it] };
}

RESISTANCE RES_EQUIV_CALC::calculate2RSolution()
{
    SolutionCollector solution( m_target );

    auto valueFunc = []( double aFoundValue )
    {
        return aFoundValue;
    };
    auto resultFunc = []( RESISTANCE& aFoundRes )
    {
        return aFoundRes;
    };
    solution.Add2RLookupResults( findIn2RBuffer( m_target ), valueFunc, resultFunc );

    return solution.GetBest();
}

RESISTANCE RES_EQUIV_CALC::calculate3RSolution()
{
    SolutionCollector solution( m_target );

    for( RESISTANCE& r : m_buffer_1R )
    {
        // try r + 2R combination
        {
            auto valueFunc = [&]( double aFoundValue )
            {
                return serialValue( aFoundValue, r.value );
            };
            auto resultFunc = [&]( RESISTANCE& aFoundRes )
            {
                return serialResistance( aFoundRes, r );
            };
            solution.Add2RLookupResults( findIn2RBuffer( m_target - r.value ), valueFunc, resultFunc );
        }

        // try r | 2R combination
        {
            auto valueFunc = [&]( double aFoundValue )
            {
                return parallelValue( aFoundValue, r.value );
            };
            auto resultFunc = [&]( RESISTANCE& aFoundRes )
            {
                return parallelResistance( aFoundRes, r );
            };
            solution.Add2RLookupResults( findIn2RBuffer( m_target * r.value / ( r.value - m_target ) ), valueFunc,
                                         resultFunc );
        }
    }

    return solution.GetBest();
}

RESISTANCE RES_EQUIV_CALC::calculate4RSolution()
{
    SolutionCollector solution( m_target );

    for( RESISTANCE& rr : m_buffer_2R )
    {
        // try 2R + 2R combination
        {
            auto valueFunc = [&]( double aFoundValue )
            {
                return serialValue( aFoundValue, rr.value );
            };
            auto resultFunc = [&]( RESISTANCE& aFoundRes )
            {
                return serialResistance( aFoundRes, rr );
            };
            solution.Add2RLookupResults( findIn2RBuffer( m_target - rr.value ), valueFunc, resultFunc );
        }

        // try 2R | 2R combination
        {
            auto valueFunc = [&]( double aFoundValue )
            {
                return parallelValue( aFoundValue, rr.value );
            };
            auto resultFunc = [&]( RESISTANCE& aFoundRes )
            {
                return parallelResistance( aFoundRes, rr );
            };
            solution.Add2RLookupResults( findIn2RBuffer( m_target * rr.value / ( rr.value - m_target ) ), valueFunc,
                                         resultFunc );
        }
    }

    for( RESISTANCE& r1 : m_buffer_1R )
    {
        for( RESISTANCE& r2 : m_buffer_1R )
        {
            // try r1 + (r2 | 2R)
            {
                auto valueFunc = [&]( double aFoundValue )
                {
                    return serialValue( r1.value, parallelValue( r2.value, aFoundValue ) );
                };
                auto resultFunc = [&]( RESISTANCE& aFoundRes )
                {
                    return serialResistance( r1, parallelResistance( r2, aFoundRes ) );
                };
                solution.Add2RLookupResults(
                        findIn2RBuffer( ( m_target - r1.value ) * r2.value / ( r1.value + r2.value - m_target ) ),
                        valueFunc, resultFunc );
            }

            // try r1 | (r2 + 2R)
            {
                auto valueFunc = [&]( double aFoundValue )
                {
                    return parallelValue( r1.value, serialValue( r2.value, aFoundValue ) );
                };
                auto resultFunc = [&]( RESISTANCE& aFoundRes )
                {
                    return parallelResistance( r1, serialResistance( r2, aFoundRes ) );
                };
                solution.Add2RLookupResults( findIn2RBuffer( m_target * r1.value / ( r1.value - m_target ) - r2.value ),
                                             valueFunc, resultFunc );
            }
        }
    }

    return solution.GetBest();
}
