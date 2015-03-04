/*
 * stripline.cpp - stripline class definition
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
#include <stripline.h>

STRIPLINE::STRIPLINE() : TRANSLINE()
{
    m_name = "StripLine";

    // Initialize these variables mainly to avoid warnings from a static analyzer
    Z0 = 0.0;               // characteristic impedance
    ang_l = 0.0;            // Electrical length in angle
    er_eff = 0.0;           // effective dielectric constant
    atten_dielectric = 0.0; // Loss in dielectric (dB)
    atten_cond = 0.0;       // Loss in conductors (dB)
}


// -------------------------------------------------------------------
void STRIPLINE::getProperties()
{
    f   = getProperty( FREQUENCY_PRM );
    w   = getProperty( PHYS_WIDTH_PRM );
    len = getProperty( PHYS_LEN_PRM );
    h   = getProperty( H_PRM);
    a   = getProperty( STRIPLINE_A_PRM );
    t   = getProperty( T_PRM );

    er    = getProperty( EPSILONR_PRM );
    murC  = getProperty( MURC_PRM );
    tand  = getProperty( TAND_PRM );
    sigma = 1.0 / getProperty( RHO_PRM );
    Z0    = getProperty( Z0_PRM );
    ang_l = getProperty( ANG_L_PRM );
}


// -------------------------------------------------------------------
// calculate characteristic impedance and conductor loss
double STRIPLINE::lineImpedance( double height, double& ac )
{
    double ZL;
    double hmt = height - t;

    ac = sqrt( f / sigma / 17.2 );
    if( w / hmt >= 0.35 )
    {
        ZL = w +
             ( 2.0 * height *
        log( (2.0 * height - t) / hmt ) - t * log( height * height / hmt / hmt - 1.0 ) ) / M_PI;
        ZL = ZF0 * hmt / sqrt( er ) / 4.0 / ZL;

        ac *= 2.02e-6 * er * ZL / hmt;
        ac *= 1.0 + 2.0 * w / hmt + (height + t) / hmt / M_PI* log( 2.0 * height / t - 1.0 );
    }
    else
    {
        double tdw = t / w;
        if( t / w > 1.0 )
            tdw = w / t;
        double de = 1.0 + tdw / M_PI * ( 1.0 + log( 4.0 * M_PI / tdw ) ) + 0.236 * pow( tdw, 1.65 );
        if( t / w > 1.0 )
            de *= t / 2.0;
        else
            de *= w / 2.0;
        ZL = ZF0 / 2.0 / M_PI / sqrt( er ) * log( 4.0 * height / M_PI / de );

        ac *= 0.01141 / ZL / de;
        ac *= de / height + 0.5 + tdw / 2.0 / M_PI + 0.5 / M_PI* log( 4.0 * M_PI / tdw )
              + 0.1947 * pow( tdw, 0.65 ) - 0.0767 * pow( tdw, 1.65 );
    }

    return ZL;
}


// -------------------------------------------------------------------
void STRIPLINE::calc()
{
    skindepth = skin_depth();

    er_eff = er; // no dispersion

    double ac1, ac2;
    Z0 = 2.0 /
         ( 1.0 / lineImpedance( 2.0 * a + t, ac1 ) + 1.0 / lineImpedance( 2.0 * (h - a) - t, ac2 ) );

    atten_cond = len * 0.5 * (ac1 + ac2);
    atten_dielectric = 20.0 / log( 10.0 ) * len * (M_PI / C0) * f * sqrt( er ) * tand;

    ang_l = 2.0* M_PI* len* sqrt( er ) * f / C0; // in radians
}


// -------------------------------------------------------------------
void STRIPLINE::show_results()
{
    setProperty( Z0_PRM, Z0 );
    setProperty( ANG_L_PRM, ang_l );

    setResult( 0, er_eff, "" );
    setResult( 1, atten_cond, "dB" );
    setResult( 2, atten_dielectric, "dB" );

    setResult( 3, skindepth / UNIT_MICRON, "µm" );
}


// -------------------------------------------------------------------
void STRIPLINE::analyze()
{
    getProperties();
    calc();
    show_results();
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
void STRIPLINE::synthesize()
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
        increment = w / 100.0;
        w += increment;
        /* compute parameters */
        calc();
        Z0_result = Z0;
        /* f(w(n)) = Z0 - Z0(w(n)) */
        /* f'(w(n)) = -f'(Z0(w(n))) */
        /* f'(Z0(w(n))) = (Z0(w(n)) - Z0(w(n+delw))/delw */
        /* w(n+1) = w(n) - f(w(n))/f'(w(n)) */
        slope = (Z0_result - Z0_current) / increment;
        slope = (Z0_dest - Z0_current) / slope - increment;
        w    += slope;
        if( w <= 0.0 )
            w = increment;
        /* find new error */
        /* compute parameters */
        calc();
        Z0_current = Z0;
        error = fabs( Z0_dest - Z0_current );
        if( iteration > 100 )
            break;
    }

    setProperty( PHYS_WIDTH_PRM, w );
    /* calculate physical length */
    ang_l = getProperty( ANG_L_PRM );
    len   = C0 / f / sqrt( er_eff ) * ang_l / 2.0 / M_PI; /* in m */
    setProperty( PHYS_LEN_PRM, len );

    /* compute parameters */
    calc();

    /* print results in the subwindow */
    show_results();
}
