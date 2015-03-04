/*
 * twistedpair.h - twisted pair class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2011 for Kicad: Jean-Pierre Charras
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

#include <units.h>
#include <transline.h>
#include <twistedpair.h>

TWISTEDPAIR::TWISTEDPAIR() : TRANSLINE()
{
    m_name = "TwistedPair";

    // Initialize these variables mainly to avoid warnings from a static analyzer
    Z0 = 0.0;               // characteristic impedance
    ang_l = 0.0;            // Electrical length in angle
    atten_dielectric = 0.0; // Loss in dielectric (dB)
    atten_cond = 0.0;       // Loss in conductors (dB)
    er_eff = 1.0;           // Effective dielectric constant
}


// -------------------------------------------------------------------
void TWISTEDPAIR::getProperties()
{
    f    = getProperty( FREQUENCY_PRM );
    din  = getProperty( PHYS_DIAM_IN_PRM );
    dout = getProperty( PHYS_DIAM_OUT_PRM );
    len  = getProperty( PHYS_LEN_PRM );

    er     = getProperty( EPSILONR_PRM );
    murC   = getProperty( MURC_PRM );
    tand   = getProperty( TAND_PRM );
    sigma  = 1.0 / getProperty( RHO_PRM );
    twists = getProperty( TWISTEDPAIR_TWIST_PRM );
    er_env = getProperty( TWISTEDPAIR_EPSILONR_ENV_PRM );
    Z0     = getProperty( Z0_PRM );
    ang_l  = getProperty( ANG_L_PRM );
}


// -------------------------------------------------------------------
void TWISTEDPAIR::calc()
{
    skindepth = skin_depth();

    double tw = atan( twists * M_PI * dout ); // pitch angle
    er_eff = er_env + (0.25 + 0.0007 * tw * tw) * (er - er_env);

    Z0 = ZF0 / M_PI / sqrt( er_eff ) * acosh( dout / din );

    atten_cond = 10.0 / log( 10.0 ) * len / skindepth / sigma / M_PI / Z0 / (din - skindepth);

    atten_dielectric = 20.0 / log( 10.0 ) * len * M_PI / C0* f* sqrt( er_eff ) * tand;

    ang_l = 2.0* M_PI* len* sqrt( er_eff ) * f / C0; // in radians
}


// -------------------------------------------------------------------
void TWISTEDPAIR::show_results()
{
    setProperty( Z0_PRM, Z0 );
    setProperty( ANG_L_PRM, ang_l );

    setResult( 0, er_eff, "" );
    setResult( 1, atten_cond, "dB" );
    setResult( 2, atten_dielectric, "dB" );

    setResult( 3, skindepth / UNIT_MICRON, "µm" );
}


// -------------------------------------------------------------------
void TWISTEDPAIR::analyze()
{
    getProperties();
    calc();
    show_results();
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
void TWISTEDPAIR::synthesize()
{
    double Z0_dest, Z0_current, Z0_result, increment, slope, error;
    int    iteration;

    getProperties();

    /* required value of Z0 */
    Z0_dest = Z0;

    /* Newton's method */
    iteration = 0;

    /* compute parameters */
    calc();
    Z0_current = Z0;

    error = fabs( Z0_dest - Z0_current );

    while( error > MAX_ERROR )
    {
        iteration++;
        if( isSelected( PHYS_DIAM_IN_PRM ) )
        {
            increment = din / 100.0;
            din += increment;
        }
        else
        {
            increment = dout / 100.0;
            dout += increment;
        }
        /* compute parameters */
        calc();
        Z0_result = Z0;
        /* f(w(n)) = Z0 - Z0(w(n)) */
        /* f'(w(n)) = -f'(Z0(w(n))) */
        /* f'(Z0(w(n))) = (Z0(w(n)) - Z0(w(n+delw))/delw */
        /* w(n+1) = w(n) - f(w(n))/f'(w(n)) */
        slope = (Z0_result - Z0_current) / increment;
        slope = (Z0_dest - Z0_current) / slope - increment;
        if( isSelected( PHYS_DIAM_IN_PRM ) )
            din += slope;
        else
            dout += slope;
        if( din <= 0.0 )
            din = increment;
        if( dout <= 0.0 )
            dout = increment;
        /* find new error */
        /* compute parameters */
        calc();
        Z0_current = Z0;
        error = fabs( Z0_dest - Z0_current );
        if( iteration > 100 )
            break;
    }

    setProperty( PHYS_DIAM_IN_PRM, din );
    setProperty( PHYS_DIAM_OUT_PRM, dout );
    /* calculate physical length */
    ang_l = getProperty( ANG_L_PRM );
    len   = C0 / f / sqrt( er_eff ) * ang_l / 2.0 / M_PI; /* in m */
    setProperty( PHYS_LEN_PRM, len );

    /* compute parameters */
    calc();

    /* print results in the subwindow */
    show_results();
}
