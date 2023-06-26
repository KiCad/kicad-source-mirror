/*
 * This program source code file
 * is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 <janvi@veith.net>
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cstdint>
#include <string>
#include <algorithm>
#include <limits>
#include "resistor_substitution_utils.h"
#include "eseries.h"

/*
 * If BENCHMARK is defined, any 4R E12 calculations will print its execution time to console
 * My Hasswell Enthusiast reports 225 mSec what are reproducible within plusminus 2 percent
 */
//#define BENCHMARK

#ifdef BENCHMARK
#include <profile.h>
#endif


// Return a string from aValue (aValue is expected in ohms)
// If aValue < 1000 the returned string is aValue with unit = R
// If aValue >= 1000 the returned string is aValue/1000 with unit = K
// with notation similar to 2K2
// If aValue >= 1e6 the returned string is aValue/1e6 with unit = M
// with notation = 1M
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
            result += std::to_string( static_cast<int>( ( mantissa * 10 ) + 0.5 ) );
    }

    return result;
}


RES_EQUIV_CALC::RES_EQUIV_CALC()
{
    // Build the list of available resistor values in each En serie
    const ESERIES::ESERIES_VALUES listValuesE1 = ESERIES::E1_VALUES();
    const ESERIES::ESERIES_VALUES listValuesE3 = ESERIES::E3_VALUES();
    const ESERIES::ESERIES_VALUES listValuesE6 = ESERIES::E6_VALUES();
    const ESERIES::ESERIES_VALUES listValuesE12 = ESERIES::E12_VALUES();
    const ESERIES::ESERIES_VALUES listValuesE24 = ESERIES::E24_VALUES();
    // buildSeriesData must be called in the order of En series, because
    // the list of series is expected indexed by En for the serie En
    buildSeriesData( listValuesE1 );
    buildSeriesData( listValuesE3 );
    buildSeriesData( listValuesE6 );
    buildSeriesData( listValuesE12 );
    int count = buildSeriesData( listValuesE24 );

    // Reserve a buffer for intermediate calculations:
    // the buffer size is 2*count*count to store all combinations of 2 values
    // there are 2*count*count = 29282 combinations for E24
    int bufsize = 2 * count * count;
    m_combined_table.reserve( bufsize );

    // Store predefined R_DATA items.
    for( int ii = 0; ii < bufsize; ii++ )
        m_combined_table.emplace_back( "", 0.0 );
}


int RES_EQUIV_CALC::buildSeriesData( const ESERIES::ESERIES_VALUES aList )
{
    uint_fast32_t curr_decade = FIRST_VALUE;
    int           count = 0;

    std::vector<R_DATA> curr_list;

    uint_fast16_t first_value_in_decade = aList[0];

    for( ;; )
    {
        double curr_r = LAST_VALUE;

        for( const uint16_t listvalue : aList )
        {
            curr_r = 1.0 * curr_decade * listvalue / first_value_in_decade;
            curr_list.emplace_back( strValue( curr_r ), curr_r );
            count++;

            if( curr_r >= LAST_VALUE )
                break;
        }

        if( curr_r >= LAST_VALUE )
            break;

        curr_decade *= 10;
    }

    m_tables.push_back( std::move( curr_list ) );

    return count;
}


void RES_EQUIV_CALC::Exclude( double aValue )
{
    if( aValue != 0.0 ) // if there is a value to exclude other than a wire jumper
    {
        for( R_DATA& i : m_tables[m_series] ) // then search it in the selected E-Series table
        {
            if( i.e_value == aValue )         // if the value to exclude is found
                i.e_use = false;              // disable its use
        }
    }
}


void RES_EQUIV_CALC::simple_solution( uint32_t aSize )
{
    uint32_t i;

    m_results.at( S2R ).e_value =
            std::numeric_limits<double>::max(); // assume no 2R solution or max deviation

    for( i = 0; i < aSize; i++ )
    {
        if( std::abs( m_combined_table.at( i ).e_value - m_required_value )
            < std::abs( m_results.at( S2R ).e_value ) )
        {
            m_results[S2R].e_value =
                    m_combined_table[i].e_value - m_required_value; // save signed deviation in Ohms
            m_results[S2R].e_name = m_combined_table[i].e_name;     // save combination text
            m_results[S2R].e_use = true;                            // this is a possible solution
        }
    }
}


void RES_EQUIV_CALC::combine4( uint32_t aSize )
{
    uint32_t i, j;
    double   tmp;

    m_results[S4R].e_use = false;                    // disable 4R solution, until
    m_results[S4R].e_value = m_results[S3R].e_value; // 4R becomes better than 3R solution

#ifdef BENCHMARK
    PROF_TIMER timer; // start timer to count execution time
#endif

    for( i = 0; i < aSize; i++ )     // 4R search outer loop
    {                                // scan valid intermediate 2R solutions
        for( j = 0; j < aSize; j++ ) // inner loop combines all with itself
        {
            tmp = m_combined_table[i].e_value
                  + m_combined_table[j].e_value; // calculate 2R+2R serial
            tmp -= m_required_value;             // calculate 4R deviation

            if( std::abs( tmp ) < std::abs( m_results.at( S4R ).e_value ) ) // if new 4R is better
            {
                m_results[S4R].e_value = tmp;           // save amount of benefit
                std::string s = "( ";
                s.append( m_combined_table[i].e_name ); // mention 1st 2 component
                s.append( " ) + ( " );                  // in series
                s.append( m_combined_table[j].e_name ); // with 2nd 2 components
                s.append( " )" );
                m_results[S4R].e_name = s;              // save the result and
                m_results[S4R].e_use = true;            // enable for later use
            }

            tmp = ( m_combined_table[i].e_value * m_combined_table[j].e_value )
                  / ( m_combined_table[i].e_value
                      + m_combined_table[j].e_value );                 // calculate 2R|2R parallel
            tmp -= m_required_value;                                   // calculate 4R deviation

            if( std::abs( tmp ) < std::abs( m_results[S4R].e_value ) ) // if new 4R is better
            {
                m_results[S4R].e_value = tmp;                          // save amount of benefit
                std::string s = "( ";
                s.append( m_combined_table[i].e_name );                // mention 1st 2 component
                s.append( " ) | ( " );                                 // in parallel
                s.append( m_combined_table[j].e_name );                // with 2nd 2 components
                s.append( " )" );
                m_results[S4R].e_name = s;                             // save the result
                m_results[S4R].e_use = true;                           // enable later use
            }
        }
    }

#ifdef BENCHMARK
    printf( "Calculation time = %d mS", timer.msecs() );
    fflush( 0 );
#endif
}


void RES_EQUIV_CALC::NewCalc()
{
    for( R_DATA& i : m_combined_table )
        i.e_use = false; // before any calculation is done, assume that

    for( R_DATA& i : m_results )
        i.e_use = false; // no combinations and no results are available

    for( R_DATA& i : m_tables[m_series] )
        i.e_use = true; // all selected E-values available
}


uint32_t RES_EQUIV_CALC::combine2()
{
    uint32_t    combi2R = 0; // target index counts calculated 2R combinations
    std::string s;

    for( const R_DATA& i : m_tables[m_series] ) // outer loop to sweep selected source lookup table
    {
        if( i.e_use )
        {
            for( const R_DATA& j : m_tables[m_series] ) // inner loop to combine values with itself
            {
                if( j.e_use )
                {
                    m_combined_table[combi2R].e_use = true;
                    m_combined_table[combi2R].e_value =
                            i.e_value + j.e_value; // calculate 2R serial
                    s = i.e_name;
                    s.append( " + " );
                    m_combined_table[combi2R].e_name = s.append( j.e_name );
                    combi2R++;                              // next destination
                    m_combined_table[combi2R].e_use = true; // calculate 2R parallel
                    m_combined_table[combi2R].e_value =
                            i.e_value * j.e_value / ( i.e_value + j.e_value );
                    s = i.e_name;
                    s.append( " | " );
                    m_combined_table[combi2R].e_name = s.append( j.e_name );
                    combi2R++; // next destination
                }
            }
        }
    }
    return combi2R;
}


void RES_EQUIV_CALC::combine3( uint32_t aSize )
{
    uint32_t    j = 0;
    double      tmp = 0; // avoid warning for being uninitialized
    std::string s;

    m_results[S3R].e_use = false;                    // disable 3R solution, until 3R
    m_results[S3R].e_value = m_results[S2R].e_value; //  becomes better than 2R solution

    for( const R_DATA& i : m_tables[m_series] ) // 3R  Outer loop to selected primary E series table
    {
        if( i.e_use )                           // skip all excluded values
        {
            for( j = 0; j < aSize; j++ )        // inner loop combines with all 2R intermediate
            {                                   //  results R+2R serial combi
                tmp = m_combined_table[j].e_value + i.e_value;
                tmp -= m_required_value;        // calculate deviation

                if( std::abs( tmp ) < std::abs( m_results[S3R].e_value ) ) // compare if better
                {                                                          // then take it
                    s = i.e_name;                                          // mention 3rd component
                    s.append( " + ( " );                                   // in series
                    s.append( m_combined_table[j].e_name );                // with 2R combination
                    s.append( " )" );
                    m_results[S3R].e_name = s;                             // save S3R result
                    m_results[S3R].e_value = tmp;                          // save amount of benefit
                    m_results[S3R].e_use = true;                           // enable later use
                }

                tmp = i.e_value * m_combined_table[j].e_value
                      / ( i.e_value + m_combined_table[j].e_value ); // calculate R + 2R parallel
                tmp -= m_required_value;                             // calculate deviation

                if( std::abs( tmp ) < std::abs( m_results[S3R].e_value ) ) // compare if better
                {                                                          // then take it
                    s = i.e_name;                                          // mention 3rd component
                    s.append( " | ( " );                                   // in parallel
                    s.append( m_combined_table[j].e_name );                // with 2R combination
                    s.append( " )" );
                    m_results[S3R].e_name = s;
                    m_results[S3R].e_value = tmp; // save amount of benefit
                    m_results[S3R].e_use = true;  // enable later use
                }
            }
        }
    }

    // If there is a 3R result with remaining deviation consider to search a possibly better
    // 4R solution
    // calculate 4R for small series always
    if( m_results[S3R].e_use && tmp )
        combine4( aSize );
}


void RES_EQUIV_CALC::Calculate()
{
    uint32_t no_of_2Rcombi = 0;

    no_of_2Rcombi = combine2();       // combine all 2R combinations for selected E serie
    simple_solution( no_of_2Rcombi ); // search for simple 2 component solution

    if( m_results[S2R].e_value )      // if simple 2R result is not exact
        combine3( no_of_2Rcombi );    // continiue searching for a possibly better solution

    strip3();
    strip4();
}


void RES_EQUIV_CALC::strip3()
{
    std::string s;

    if( m_results[S3R].e_use ) // if there is a 3 term result available
    {                          // what is connected either by two "|" or by 3 plus
        s = m_results[S3R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 2 )
            || ( std::count( s.begin(), s.end(), '|' ) == 2 ) )
        {                                 // then strip one pair of braces
            s.erase( s.find( "( " ), 2 ); // it is known sure, this is available
            s.erase( s.find( " )" ), 2 ); // in any unstripped 3R result term
            m_results[S3R].e_name = s;    // use stripped result
        }
    }
}


void RES_EQUIV_CALC::strip4()
{
    std::string s;

    if( m_results[S4R].e_use ) // if there is a 4 term result available
    {                          // what are connected either by 3 "+" or by 3 "|"
        s = m_results[S4R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 3 )
            || ( std::count( s.begin(), s.end(), '|' ) == 3 ) )
        {                                 // then strip two pair of braces
            s.erase( s.find( "( " ), 2 ); // it is known sure, they are available
            s.erase( s.find( " )" ), 2 ); // in any unstripped 4R result term
            s.erase( s.find( "( " ), 2 );
            s.erase( s.find( " )" ), 2 );
            m_results[S4R].e_name = s; // use stripped result
        }
    }
}
