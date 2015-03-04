/*
 * TRANSLINE.cpp - base for a transmission line implementation
 *
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <limits>
#include <transline.h>
#include <units.h>


#ifndef INFINITY
#define INFINITY std::numeric_limits<double>::infinity()
#endif


#ifndef M_PI_2
#define M_PI_2 (M_PI/2)
#endif


// Functions to Read/Write parameters in pcb_calculator main frame:
// They are wrapper to actual functions, so all transline functions do not
// depend on Graphic User Interface
void SetPropertyInDialog( enum PRMS_ID aPrmId, double value );

/* Puts the text into the given result line.
*/
void SetResultInDialog( int line, const char* text );

/* print aValue into the given result line.
*/
void SetResultInDialog( int aLineNumber, double aValue, const char* aText );

/* Returns a named property value. */
double GetPropertyInDialog( enum PRMS_ID aPrmId );

// Returns true if the param aPrmId is selected
// Has meaning only for params that have a radio button
bool   IsSelectedInDialog( enum PRMS_ID aPrmId );


/* Constructor creates a transmission line instance. */
TRANSLINE::TRANSLINE()
{
    murC = 1.0;
    m_name = (const char*) 0;

    // Initialize these variables mainly to avoid warnings from a static analyzer
    f = 0.0;            // Frequency of operation
    er = 0.0;           // dielectric constant
    tand = 0.0;         // Dielectric Loss Tangent
    sigma = 0.0;        // Conductivity of the metal
    skindepth = 0.0;    // Skin depth
}


/* Destructor destroys a transmission line instance. */
TRANSLINE::~TRANSLINE()
{
}


/* Sets a named property to the given value, access through the
 *  application.
 */
void TRANSLINE::setProperty( enum PRMS_ID aPrmId, double value )
{
    SetPropertyInDialog( aPrmId, value );
}

/*
 *Returns true if the param aPrmId is selected
 * Has meaning only for params that have a radio button
 */
bool TRANSLINE::isSelected( enum PRMS_ID aPrmId )
{
    return IsSelectedInDialog( aPrmId );
}


/* Puts the text into the given result line.
*/
void TRANSLINE::setResult( int line, const char* text )
{
    SetResultInDialog( line, text );
}
void TRANSLINE::setResult( int line, double value, const char* text )
{
    SetResultInDialog( line, value, text );
}


/* Returns a property value. */
double TRANSLINE::getProperty( enum PRMS_ID aPrmId )
{
    return GetPropertyInDialog( aPrmId );
}

/*
 * skin_depth - calculate skin depth
 */
#include <stdio.h>
double TRANSLINE::skin_depth()
{
    double depth;
    depth = 1.0 / sqrt( M_PI * f * murC * MU0 * sigma );
    return depth;
}


/* *****************************************************************
**********                                             **********
**********          mathematical functions             **********
**********                                             **********
***************************************************************** */

#define NR_EPSI 2.2204460492503131e-16

/* The function computes the complete elliptic integral of first kind
 *  K() and the second kind E() using the arithmetic-geometric mean
 *  algorithm (AGM) by Abramowitz and Stegun. */
void TRANSLINE::ellipke( double arg, double& k, double& e )
{
    int iMax = 16;

    if( arg == 1.0 )
    {
        k = INFINITY; // infinite
        e = 0;
    }
    else if( std::isinf( arg ) && arg < 0 )
    {
        k = 0;
        e = INFINITY; // infinite
    }
    else
    {
        double a, b, c, f, s, fk = 1, fe = 1, t, da = arg;
        int    i;
        if( arg < 0 )
        {
            fk = 1 / sqrt( 1 - arg );
            fe = sqrt( 1 - arg );
            da = -arg / (1 - arg);
        }
        a = 1;
        b = sqrt( 1 - da );
        c = sqrt( da );
        f = 0.5;
        s = f * c * c;
        for( i = 0; i < iMax; i++ )
        {
            t  = (a + b) / 2;
            c  = (a - b) / 2;
            b  = sqrt( a * b );
            a  = t;
            f *= 2;
            s += f * c * c;
            if( c / a < NR_EPSI )
                break;
        }

        if( i >= iMax )
        {
            k = 0; e = 0;
        }
        else
        {
            k = M_PI_2 / a;
            e = M_PI_2 * (1 - s) / a;
            if( arg < 0 )
            {
                k *= fk;
                e *= fe;
            }
        }
    }
}


/* We need to know only K(k), and if possible KISS. */
double TRANSLINE::ellipk( double k )
{
    double r, lost;

    ellipke( k, r, lost );
    return r;
}
