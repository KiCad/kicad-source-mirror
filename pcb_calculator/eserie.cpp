/*
 * This program source code file
 * is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 <janvi@veith.net>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <array>
#include "panel_eserie.h"


extern double DoubleFromString( const wxString& TextValue );

/* If BENCHMARK is defined, any 4R E12 calculations will print its execution time to console
 * My Hasswell Enthusiast reports 225 mSec what are reproducible within plusminus 2 percent
 */
//#define BENCHMARK

#ifdef BENCHMARK
#include <sys/time.h>
#endif

#include "eserie.h"

E_SERIE r;


void E_SERIE::Exclude( double aValue )
{
    if( aValue ) // if there is a value to exclude other than a wire jumper
    {
        for( R_DATA& i : m_luts[m_series] ) // then search it in the selected E-Serie lookup table
        {
            if( i.e_value == aValue )     // if value to exclude found
                i.e_use = false;          // disable its use
        }
    }
}


void E_SERIE::simple_solution( uint32_t aSize )
{
    uint32_t i;

    m_results[S2R].e_value = std::numeric_limits<double>::max(); // assume no 2R solution or max deviation

    for( i = 0; i < aSize; i++ )
    {
        if( abs( m_cmb_lut[i].e_value - m_required_value ) < abs( m_results[S2R].e_value ) )
        {
            m_results[S2R].e_value = m_cmb_lut[i].e_value - m_required_value; // save signed deviation in Ohms
            m_results[S2R].e_name  = m_cmb_lut[i].e_name;         // save combination text
            m_results[S2R].e_use   = true;                      // this is a possible solution
        }
    }
}


void E_SERIE::combine4( uint32_t aSize )
{
    uint32_t    i,j;
    double      tmp;
    std::string s;

    m_results[S4R].e_use = false;                          // disable 4R solution, until
    m_results[S4R].e_value = m_results[S3R].e_value;         // 4R becomes better than 3R solution

    #ifdef BENCHMARK
        PROF_COUNTER combine4_timer;                     // start timer to count execution time
    #endif

    for( i = 0; i < aSize; i++ )                         // 4R search outer loop
    {                                                    // scan valid intermediate 2R solutions
        for( j = 0; j < aSize; j++ )                     // inner loop combines all with itself
        {
            tmp = m_cmb_lut[i].e_value + m_cmb_lut[j].e_value;       // calculate 2R+2R serial
            tmp -= m_required_value;                                         // calculate 4R deviation

            if( abs( tmp ) < abs( m_results[S4R].e_value ) )       // if new 4R is better
            {
                m_results[S4R].e_value = tmp;                      // save amount of benefit
                std::string s = "( ";
                s.append( m_cmb_lut[i].e_name );                   // mention 1st 2 component
                s.append( " ) + ( " );                           // in series
                s.append( m_cmb_lut[j].e_name );                   // with 2nd 2 components
                s.append( " )" );
                m_results[S4R].e_name = s;                         // save the result and
                m_results[S4R].e_use = true;                       // enable for later use
            }

            tmp = ( m_cmb_lut[i].e_value * m_cmb_lut[j].e_value ) /
                  ( m_cmb_lut[i].e_value + m_cmb_lut[j].e_value );   // calculate 2R|2R parallel
            tmp -= m_required_value;                                         // calculate 4R deviation

            if( abs( tmp ) < abs( m_results[S4R].e_value ) )       // if new 4R is better
            {
                m_results[S4R].e_value = tmp;                      // save amount of benefit
                std::string s = "( ";
                s.append( m_cmb_lut[i].e_name );                   // mention 1st 2 component
                s.append( " ) | ( " );                           // in parallel
                s.append( m_cmb_lut[j].e_name );                   // with 2nd 2 components
                s.append( " )" );
                m_results[S4R].e_name = s;                         // save the result
                m_results[S4R].e_use  = true;                      // enable later use
            }
        }
    }

    #ifdef BENCHMARK
        if( m_series == E12 )
             std::cout<<"4R Time = "<<combine4_timer.msecs()<<" mSec"<<std::endl;
    #endif
}


void E_SERIE::NewCalc( void )
{
    for( R_DATA& i : m_cmb_lut )
        i.e_use = false;                // before any calculation is done, assume that

    for( R_DATA& i : m_results )
        i.e_use = false;                // no combinations and no results are available

    for( R_DATA& i : m_luts[m_series])
        i.e_use = true;                 // all selected E-values available
}


uint32_t E_SERIE::combine2( void )
{
    uint32_t    combi2R = 0;                // target index counts calculated 2R combinations
    std::string s;

    for( const R_DATA& i : m_luts[m_series] ) // outer loop to sweep selected source lookup table
    {
        if( i.e_use )
        {
            for( const R_DATA& j : m_luts[m_series] ) // inner loop to combine values with itself
            {
                if( j.e_use )
                {
                    m_cmb_lut[combi2R].e_use    = true;
                    m_cmb_lut[combi2R].e_value  = i.e_value + j.e_value; // calculate 2R serial
                    s = i.e_name;
                    s.append( " + " );
                    m_cmb_lut[combi2R].e_name = s.append( j.e_name);
                    combi2R++;                                         // next destination
                    m_cmb_lut[combi2R].e_use    = true;                  // calculate 2R parallel
                    m_cmb_lut[combi2R].e_value = i.e_value * j.e_value /
                                                 ( i.e_value + j.e_value );
                    s = i.e_name;
                    s.append( " | " );
                    m_cmb_lut[combi2R].e_name = s.append( j.e_name );
                    combi2R++;                                         // next destination
                }
            }
        }
    }
    return ( combi2R );
}


void E_SERIE::combine3( uint32_t aSize )
{
    uint32_t    j   = 0;
    double      tmp = 0; // avoid warning for being uninitialized
    std::string s;

    m_results[S3R].e_use   = false;                // disable 3R solution, until
    m_results[S3R].e_value = m_results[S2R].e_value; // 3R becomes better than 2R solution

    for( const R_DATA& i : m_luts[m_series] )      // 3R  Outer loop to selected primary E serie LUT
    {
        if( i.e_use )                            // skip all excluded values
        {
            for( j = 0; j < aSize; j++ )  // inner loop combines with all 2R intermediate results
            {                                                  //  R+2R serial combi
                tmp  = m_cmb_lut[j].e_value + i.e_value;
                tmp -= m_required_value;                                   // calculate deviation

                if( abs( tmp ) < abs( m_results[S3R].e_value ) ) // compare if better
                {                                              // then take it
                    s = i.e_name;                              // mention 3rd component
                    s.append( " + ( " );                       // in series
                    s.append( m_cmb_lut[j].e_name );             // with 2R combination
                    s.append( " )" );
                    m_results[S3R].e_name = s;                   // save S3R result
                    m_results[S3R].e_value = tmp;                // save amount of benefit
                    m_results[S3R].e_use = true;                 // enable later use
                }

                tmp = i.e_value * m_cmb_lut[j].e_value /
                      ( i.e_value + m_cmb_lut[j].e_value );      // calculate R + 2R parallel
                tmp -= m_required_value;                                   // calculate deviation

                if( abs( tmp ) < abs( m_results[S3R].e_value ) ) // compare if better
                {                                              // then take it
                    s = i.e_name;                              // mention 3rd component
                    s.append( " | ( " );                       // in parallel
                    s.append( m_cmb_lut[j].e_name );             // with 2R combination
                    s.append( " )" );
                    m_results[S3R].e_name  = s;
                    m_results[S3R].e_value = tmp;                // save amount of benefit
                    m_results[S3R].e_use   = true;               // enable later use
                }
            }
        }
    }
                                                 // if there is a 3R result with remaining deviation
    if(( m_results[S3R].e_use == true ) && tmp )
    {                                            // consider to search a possibly better 4R solution
        combine4( aSize );                       // calculate 4R for small series always
    }
}


void E_SERIE::Calculate( void )
{
    uint32_t no_of_2Rcombi = 0;

    no_of_2Rcombi = combine2();       // combine all 2R combinations for selected E serie
    simple_solution( no_of_2Rcombi ); // search for simple 2 component solution

    if( m_results[S2R].e_value )        // if simple 2R result is not exact
        combine3( no_of_2Rcombi );    // continiue searching for a possibly better solution

    strip3();
    strip4();
}


void E_SERIE::strip3( void )
{
    std::string s;

    if( m_results[S3R].e_use )     // if there is a 3 term result available
    {                            // what is connected either by two "|" or by 3 plus
        s = m_results[S3R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 2 )
                || ( std::count( s.begin(), s.end(), '|' ) == 2 ) )
        {                                // then strip one pair of braces
            s.erase( s.find( "(" ), 1 ); // it is known sure, this is available
            s.erase( s.find( ")" ), 1 ); // in any unstripped 3R result term
            m_results[S3R].e_name = s;     // use stripped result
        }
    }
}


void E_SERIE::strip4( void )
{
    std::string s;

    if( m_results[S4R].e_use )      // if there is a 4 term result available
    {                             // what are connected either by 3 "+" or by 3 "|"
        s = m_results[S4R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 3 )
                || ( std::count( s.begin(), s.end(), '|' ) == 3 ) )
        {                                // then strip two pair of braces
            s.erase( s.find( "(" ), 1 ); // it is known sure, they are available
            s.erase( s.find( ")" ), 1 ); // in any unstripped 4R result term
            s.erase( s.find( "(" ), 1 );
            s.erase( s.find( ")" ), 1 );
            m_results[S4R].e_name = s;     // use stripped result
        }
    }
}


void PANEL_E_SERIE::OnCalculateESeries( wxCommandEvent& event )
{
    double   reqr;            // required resistor stored in local copy
    double   error, err3 = 0;
    wxString es, fs;          // error and formula strings

    reqr = ( 1000 * DoubleFromString( m_ResRequired->GetValue() ) );
    r.SetRequiredValue( reqr ); // keep a local copy of required resistor value
    r.NewCalc();     // assume all values available
    /*
     * Exclude itself. For the case, a value from the available series is found as required value,
     * the calculator assumes this value needs a replacement for the reason of being not available.
     * Two further exclude values can be entered to exclude and are skipped as not being available.
     * All values entered in KiloOhms are converted to Ohm for internal calculation
     */
    r.Exclude( 1000 * DoubleFromString( m_ResRequired->GetValue()));
    r.Exclude( 1000 * DoubleFromString( m_ResExclude1->GetValue()));
    r.Exclude( 1000 * DoubleFromString( m_ResExclude2->GetValue()));
    r.Calculate();

    fs = r.GetResults()[S2R].e_name;               // show 2R solution formula string
    m_ESeries_Sol2R->SetValue( fs );
    error = reqr + r.GetResults()[S2R].e_value;    // absolute value of solution
    error = ( reqr / error - 1 ) * 100;          // error in percent

    if( error )
    {
        if( std::abs( error ) < 0.01 )
            es.Printf( "<%.2f", 0.01 );
        else
            es.Printf( "%+.2f",error);
    }
    else
    {
        es = _( "Exact" );
    }

    m_ESeriesError2R->SetValue( es );            // anyway show 2R error string

    if( r.GetResults()[S3R].e_use )                // if 3R solution available
    {
        err3 = reqr + r.GetResults()[S3R].e_value; // calculate the 3R
        err3 = ( reqr / err3 - 1 ) * 100;        // error in percent

        if( err3 )
        {
            if( std::abs( err3 ) < 0.01 )
                es.Printf( "<%.2f", 0.01 );
            else
                es.Printf( "%+.2f",err3);
        }
        else
        {
            es = _( "Exact" );
        }

        m_ESeriesError3R->SetValue( es );         // show 3R error string
        fs = r.GetResults()[S3R].e_name;
        m_ESeries_Sol3R->SetValue( fs );         // show 3R formula string
    }
    else                                         // nothing better than 2R found
    {
        fs = _( "Not worth using" );
        m_ESeries_Sol3R->SetValue( fs );
        m_ESeriesError3R->SetValue( wxEmptyString );
    }

    fs = wxEmptyString;

    if( r.GetResults()[S4R].e_use )                 // show 4R solution if available
    {
        fs = r.GetResults()[S4R].e_name;

        error = reqr + r.GetResults()[S4R].e_value; // absolute value of solution
        error = ( reqr / error - 1 ) * 100;       // error in percent

        if( error )
            es.Printf( "%+.2f",error );
        else
            es = _( "Exact" );

        m_ESeriesError4R->SetValue( es );
    }
    else                                          // no 4R solution
    {
        fs = _( "Not worth using" );
        es = wxEmptyString;
        m_ESeriesError4R->SetValue( es );
    }

    m_ESeries_Sol4R->SetValue( fs );
}


void PANEL_E_SERIE::OnESeriesSelection( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_e1 )
        r.SetSeries( E1 );
    else if( event.GetEventObject() == m_e3 )
        r.SetSeries( E3 );
    else if( event.GetEventObject() == m_e12 )
        r.SetSeries( E12 );
    else
        r.SetSeries( E6 );
}
