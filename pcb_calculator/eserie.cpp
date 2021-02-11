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

#include <dialog_helpers.h>
#include "class_regulator_data.h"
#include "attenuators/attenuator_classes.h"
#include "pcb_calculator_frame.h"
#include <wx/wx.h>

#include <array>
#include <iostream>
#include <string>

#ifdef BENCHMARK
#include <sys/time.h>
#endif

#include <vector>

#include "eserie.h"
#include "profile.h"

wxString eseries_help =
#include "eserie_help.h"

    eserie r;

void eserie::set_reqR( double aR )
{
    reqR = aR;
}

void eserie::set_rb( uint32_t a_rb )
{
    rb_state = a_rb;
}

std::array<r_data, S4R + 1> eserie::get_rslt( void )
{
    return results;
}

void eserie::exclude( double aValue )
{
    if( aValue ) // if there is a value to exclude other than a wire jumper
    {
        for( r_data& i : luts[rb_state] ) // then search it in the selected E-Serie lookup table
        {
            if( i.e_value == aValue )     // if value to exclude found
            {
                i.e_use = false;          // disable its use
            }
        }
    }
}

void eserie::simple_solution( uint32_t aSize )
{
    uint32_t i;

    results[S2R].e_value = std::numeric_limits<double>::max(); // assume no 2R solution or max deviation

    for( i = 0; i < aSize; i++ )
    {
        if( abs( cmb_lut[i].e_value - reqR ) < abs( results[S2R].e_value ) )
        {
            results[S2R].e_value = cmb_lut[i].e_value - reqR; // save signed deviation in Ohms
            results[S2R].e_name  = cmb_lut[i].e_name;         // save combination text
            results[S2R].e_use   = true;                      // this is a possible solution
        }
    }
}

void eserie::combine4( uint32_t aSize )
{
    uint32_t    i,j;
    double      tmp;
    std::string s;

    results[S4R].e_use = false;                          // disable 4R solution, until
    results[S4R].e_value = results[S3R].e_value;         // 4R becomes better than 3R solution

    #ifdef BENCHMARK
        PROF_COUNTER combine4_timer;                     // start timer to count execution time
    #endif

    for( i = 0; i < aSize; i++ )                         // 4R search outer loop
    {                                                    // scan valid intermediate 2R solutions
        for( j = 0; j < aSize; j++ )                     // inner loop combines all with itself
        {
            tmp = cmb_lut[i].e_value + cmb_lut[j].e_value;       // calculate 2R+2R serial
            tmp -= reqR;                                         // calculate 4R deviation

            if( abs( tmp ) < abs( results[S4R].e_value ) )       // if new 4R is better
            {
                results[S4R].e_value = tmp;                      // save amount of benefit
                std::string s = "( ";
                s.append( cmb_lut[i].e_name );                   // mention 1st 2 component
                s.append( " ) + ( " );                           // in series
                s.append( cmb_lut[j].e_name );                   // with 2nd 2 components
                s.append( " )" );
                results[S4R].e_name = s;                         // save the result and
                results[S4R].e_use = true;                       // enable for later use
            }

            tmp = ( cmb_lut[i].e_value * cmb_lut[j].e_value ) /
                  ( cmb_lut[i].e_value + cmb_lut[j].e_value );   // calculate 2R|2R parallel
            tmp -= reqR;                                         // calculate 4R deviation

            if( abs( tmp ) < abs( results[S4R].e_value ) )       // if new 4R is better
            {
                results[S4R].e_value = tmp;                      // save amount of benefit
                std::string s = "( ";
                s.append( cmb_lut[i].e_name );                   // mention 1st 2 component
                s.append( " ) | ( " );                           // in parallel
                s.append( cmb_lut[j].e_name );                   // with 2nd 2 components
                s.append( " )" );
                results[S4R].e_name = s;                         // save the result
                results[S4R].e_use  = true;                      // enable later use
            }
        }
    }

    #ifdef BENCHMARK
        if( rb_state == E12 )
        {
             std::cout<<"4R Time = "<<combine4_timer.msecs()<<" mSec"<<std::endl;
        }
    #endif
}

void eserie::new_calc( void )
{
    for( r_data& i : cmb_lut )
    {
        i.e_use = false; // before any calculation is done, assume that
    }

    for( r_data& i : results )
    {
        i.e_use = false; // no combinations and no results are available
    }

    for( r_data& i : luts[rb_state])
    {
        i.e_use = true; // all selecte E-values available
    }
}

uint32_t eserie::combine2( void )
{
    uint32_t    combi2R = 0;                // target index counts calculated 2R combinations
    std::string s;

    for( const r_data& i : luts[rb_state] ) // outer loop to sweep selected source lookup table
    {
        if( i.e_use )
        {
            for( const r_data& j : luts[rb_state] ) // inner loop to combine values with itself
            {
                if( j.e_use )
                {
                    cmb_lut[combi2R].e_use    = true;
                    cmb_lut[combi2R].e_value  = i.e_value + j.e_value; // calculate 2R serial
                    s = i.e_name;
                    s.append( " + " );
                    cmb_lut[combi2R].e_name = s.append(j.e_name);
                    combi2R++;                                         // next destination
                    cmb_lut[combi2R].e_use    = true;                  // calculate 2R parallel
                    cmb_lut[combi2R].e_value = i.e_value * j.e_value /
                                             ( i.e_value + j.e_value );
                    s = i.e_name;
                    s.append( " | " );
                    cmb_lut[combi2R].e_name = s.append( j.e_name );
                    combi2R++;                                         // next destination
                }
            }
        }
    }
    return ( combi2R );
}

void eserie::combine3( uint32_t aSize )
{
    uint32_t    j   = 0;
    double      tmp = 0; // avoid warning for being uninitialized
    std::string s;

    results[S3R].e_use   = false;                // disable 3R solution, until
    results[S3R].e_value = results[S2R].e_value; // 3R becomes better than 2R solution

    for( const r_data& i : luts[rb_state] )      // 3R  Outer loop to selected primary E serie LUT
    {
        if( i.e_use )                            // skip all excluded values
        {
            for( j = 0; j < aSize; j++ )  // inner loop combines with all 2R intermediate results
            {                                                  //  R+2R serial combi
                tmp  = cmb_lut[j].e_value + i.e_value;
                tmp -= reqR;                                   // calculate deviation

                if( abs( tmp ) < abs( results[S3R].e_value ) ) // compare if better
                {                                              // then take it
                    s = i.e_name;                              // mention 3rd component
                    s.append( " + ( " );                       // in series
                    s.append( cmb_lut[j].e_name );             // with 2R combination
                    s.append( " )" );
                    results[S3R].e_name = s;                   // save S3R result
                    results[S3R].e_value = tmp;                // save amount of benefit
                    results[S3R].e_use = true;                 // enable later use
                }

                tmp =   i.e_value * cmb_lut[j].e_value /
                      ( i.e_value + cmb_lut[j].e_value );      // calculate R + 2R parallel
                tmp -= reqR;                                   // calculate deviation

                if( abs( tmp ) < abs( results[S3R].e_value ) ) // compare if better
                {                                              // then take it
                    s = i.e_name;                              // mention 3rd component
                    s.append( " | ( " );                       // in parallel
                    s.append( cmb_lut[j].e_name );             // with 2R combination
                    s.append( " )" );
                    results[S3R].e_name  = s;
                    results[S3R].e_value = tmp;                // save amount of benefit
                    results[S3R].e_use   = true;               // enable later use
                }
            }
        }
    }
                                                 // if there is a 3R result with remaining deviation
    if( ( results[S3R].e_use == true ) && tmp )
    {                                            // consider to search a possibly better 4R solution
        combine4( aSize );                       // calculate 4R for small series always
    }
}

void eserie::calculate( void )
{
    uint32_t no_of_2Rcombi = 0;

    no_of_2Rcombi = combine2();       // combine all 2R combinations for selected E serie
    simple_solution( no_of_2Rcombi ); // search for simple 2 component solution

    if( results[S2R].e_value )        // if simple 2R result is not exact
    {
        combine3( no_of_2Rcombi );    // continiue searching for a possibly better solution
    }
    strip3();
    strip4();
}

void eserie::strip3( void )
{
    std::string s;

    if( results[S3R].e_use )     // if there is a 3 term result available
    {                            // what is connected either by two "|" or by 3 plus
        s = results[S3R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 2 ) || \
            ( std::count( s.begin(), s.end(), '|' ) == 2 ) )
        {                                // then strip one pair of braces
            s.erase( s.find( "(" ), 1 ); // it is known sure, this is available
            s.erase( s.find( ")" ), 1 ); // in any unstripped 3R result term
            results[S3R].e_name = s;     // use stripped result
        }
    }
}

void eserie::strip4( void )
{
    std::string s;

    if( results[S4R].e_use )      // if there is a 4 term result available
    {                             // what are connected either by 3 "+" or by 3 "|"
        s = results[S4R].e_name;

        if( ( std::count( s.begin(), s.end(), '+' ) == 3 ) ||
            ( std::count( s.begin(), s.end(), '|' ) == 3 ) )
        {                                // then strip two pair of braces
            s.erase( s.find( "(" ), 1 ); // it is known sure, they are available
            s.erase( s.find( ")" ), 1 ); // in any unstripped 4R result term
            s.erase( s.find( "(" ), 1 );
            s.erase( s.find( ")" ), 1 );
            results[S4R].e_name = s;     // use stripped result
        }
    }
}

void PCB_CALCULATOR_FRAME::OnCalculateESeries( wxCommandEvent& event )
{
    double   reqr;            // required resistor stored in local copy
    double   error, err3 = 0;
    wxString es, fs;          // error and formula strings

    reqr = ( 1000 * DoubleFromString( m_ResRequired->GetValue() ) );
    r.set_reqR(reqr); // keep a local copy of requred resistor value
    r.new_calc();     // assume all values available
    /*
     * Exclude itself. For the case, a value from the available series is found as required value,
     * the calculator assumes this value needs a replacement for the reason of being not available.
     * Two further exclude values can be entered to exclude and are skipped as not being availabe.
     * All values entered in KiloOhms are converted to Ohm for internal calculation
     */
    r.exclude( 1000 * DoubleFromString( m_ResRequired->GetValue() ) );
    r.exclude( 1000 * DoubleFromString( m_ResExclude1->GetValue() ) );
    r.exclude( 1000 * DoubleFromString( m_ResExclude2->GetValue() ) );
    r.calculate();

    fs = r.get_rslt()[S2R].e_name;               // show 2R solution formula string
    m_ESeries_Sol2R->SetValue( fs );
    error = reqr + r.get_rslt()[S2R].e_value;    // absolute value of solution
    error = ( reqr / error - 1 ) * 100;          // error in percent

    if( error )
    {
        if( std::abs( error ) < 0.01 )
        {
            es.Printf( "<%.2f", 0.01 );
        }
        else
        {
            es.Printf( "%+.2f",error);
        }
    }
    else
    {
        es = _( "Exact" );
    }

    m_ESeriesError2R->SetValue( es );            // anyway show 2R error string

    if( r.get_rslt()[S3R].e_use )                // if 3R solution available
    {
        err3 = reqr + r.get_rslt()[S3R].e_value; // calculate the 3R
        err3 = ( reqr / err3 - 1 ) * 100;        // error in percent

        if( err3 )
        {
            if( std::abs( err3 ) < 0.01 )
            {
                es.Printf( "<%.2f", 0.01 );
            }
            else
            {
                es.Printf( "%+.2f",err3);
            }
        }
        else
        {
            es = _( "Exact" );
        }

        m_ESeriesError3R->SetValue( es );         // show 3R error string
        fs = r.get_rslt()[S3R].e_name;
        m_ESeries_Sol3R->SetValue( fs );         // show 3R formula string
    }
    else                                         // nothing better than 2R found
    {
        fs = _( "Not worth using" );
        m_ESeries_Sol3R->SetValue( fs );
        m_ESeriesError3R->SetValue( wxEmptyString );
    }

    fs = wxEmptyString;

    if( r.get_rslt()[S4R].e_use )                 // show 4R solution if available
    {
        fs = r.get_rslt()[S4R].e_name;

        error = reqr + r.get_rslt()[S4R].e_value; // absolute value of solution
        error = ( reqr / error - 1 ) * 100;       // error in percent

        if( error )
        {
            es.Printf( "%+.2f",error );
        }
        else
        {
            es = _( "Exact" );
        }

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

void PCB_CALCULATOR_FRAME::OnESeriesSelection( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_e1 )
        r.set_rb( E1 );
    else if( event.GetEventObject() == m_e3 )
        r.set_rb( E3 );
    else if( event.GetEventObject() == m_e12 )
        r.set_rb( E12 );
    else
        r.set_rb( E6 );
}

void PCB_CALCULATOR_FRAME::ES_Init()    // initialize ESeries tab at each pcb-calculator start
{
    wxString msg;

    // show markdown formula explanation in lower help panel
    ConvertMarkdown2Html( wxGetTranslation( eseries_help ), msg );
    m_panelESeriesHelp->SetPage( msg );
}
