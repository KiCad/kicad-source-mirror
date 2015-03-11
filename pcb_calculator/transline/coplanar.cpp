/*
 * coplanar.cpp - coplanar class implementation
 *
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
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
#include <coplanar.h>

COPLANAR::COPLANAR() : TRANSLINE()
{
    m_name = "CoPlanar";
    backMetal = false;

    // Initialize these variables mainly to avoid warnings from a static analyzer
    h = 0.0;                    // height of substrate
    t = 0.0;                    // thickness of top metal
    w = 0.0;                    // width of line
    s = 0.0;                    // width of gap between line and ground
    len = 0.0;                  // length of line
    Z0 = 0.0;                   // characteristic impedance
    ang_l = 0.0;                // Electrical length in angle
    atten_dielectric = 0.0;     // Loss in dielectric (dB)
    atten_cond = 0.0;           // Loss in conductors (dB)
    er_eff = 1.0;               // Effective dielectric constant
}


GROUNDEDCOPLANAR::GROUNDEDCOPLANAR() : COPLANAR()
{
    m_name = "GrCoPlanar";
    backMetal = true;
}


// -------------------------------------------------------------------
void COPLANAR::getProperties()
{
    f   = getProperty( FREQUENCY_PRM );
    w   = getProperty( PHYS_WIDTH_PRM );
    s   = getProperty( PHYS_S_PRM );
    len = getProperty( PHYS_LEN_PRM );
    h   = getProperty( H_PRM );
    t   = getProperty( T_PRM );

    er    = getProperty( EPSILONR_PRM );
    murC  = getProperty( MURC_PRM );
    tand  = getProperty( TAND_PRM );
    sigma = 1.0 / getProperty( RHO_PRM );
    Z0    = getProperty( Z0_PRM );
    ang_l = getProperty( ANG_L_PRM );
}


// -------------------------------------------------------------------
void COPLANAR::calc()
{
    skindepth = skin_depth();

    // other local variables (quasi-static constants)
    double k1, kk1, kpk1, k2, k3, q1, q2, q3 = 0, qz, er0 = 0;
    double zl_factor;

    // compute the necessary quasi-static approx. (K1, K3, er(0) and Z(0))
    k1   = w / (w + s + s);
    kk1  = ellipk( k1 );
    kpk1 = ellipk( sqrt( 1 - k1 * k1 ) );
    q1   = kk1 / kpk1;

    // backside is metal
    if( backMetal )
    {
        k3  = tanh( (M_PI / 4) * (w / h) ) / tanh( (M_PI / 4) * (w + s + s) / h );
        q3  = ellipk( k3 ) / ellipk( sqrt( 1 - k3 * k3 ) );
        qz  = 1 / (q1 + q3);
        er0 = 1 + q3 * qz * (er - 1);
        zl_factor = ZF0 / 2 * qz;
    }
    // backside is air
    else
    {
        k2  = sinh( (M_PI / 4) * (w / h) ) / sinh( (M_PI / 4) * (w + s + s) / h );
        q2  = ellipk( k2 ) / ellipk( sqrt( 1 - k2 * k2 ) );
        er0 = 1 + (er - 1) / 2 * q2 / q1;
        zl_factor = ZF0 / 4 / q1;
    }

    // adds effect of strip thickness
    if( t > 0 )
    {
        double d, se, We, ke, qe;
        d  = (t * 1.25 / M_PI) * ( 1 + log( 4 * M_PI * w / t ) );
        se = s - d;
        We = w + d;

        // modifies k1 accordingly (k1 = ke)
        ke = We / (We + se + se); // ke = k1 + (1 - k1 * k1) * d / 2 / s;
        qe = ellipk( ke ) / ellipk( sqrt( 1 - ke * ke ) );

        // backside is metal
        if( backMetal )
        {
            qz  = 1 / (qe + q3);
            er0 = 1 + q3 * qz * (er - 1);
            zl_factor = ZF0 / 2 * qz;
        }
        // backside is air
        else
        {
            zl_factor = ZF0 / 4 / qe;
        }

        // modifies er0 as well
        er0 = er0 - (0.7 * (er0 - 1) * t / s) / ( q1 + (0.7 * t / s) );
    }

    // pre-compute square roots
    double sr_er  = sqrt( er );
    double sr_er0 = sqrt( er0 );

    // cut-off frequency of the TE0 mode
    double fte = (C0 / 4) / ( h * sqrt( er - 1 ) );

    // dispersion factor G
    double p = log( w / h );
    double u = 0.54 - (0.64 - 0.015 * p) * p;
    double v = 0.43 - (0.86 - 0.54 * p) * p;
    double G = exp( u * log( w / s ) + v );

    // loss constant factors (computed only once for efficency sake)
    double ac = 0;
    if( t > 0 )
    {
        // equations by GHIONE
        double n = (1 - k1) * 8 * M_PI / ( t * (1 + k1) );
        double a = w / 2;
        double b = a + s;
        ac = ( M_PI + log( n * a ) ) / a + ( M_PI + log( n * b ) ) / b;
    }
    double ac_factor = ac / ( 4 * ZF0 * kk1 * kpk1 * (1 - k1 * k1) );
    double ad_factor = ( er / (er - 1) ) * tand * M_PI / C0;


    // ....................................................
    double sr_er_f = sr_er0;

    // add the dispersive effects to er0
    sr_er_f += (sr_er - sr_er0) / ( 1 + G * pow( f / fte, -1.8 ) );

    // for now, the loss are limited to strip losses (no radiation
    // losses yet) losses in neper/length
    atten_cond = 20.0 / log( 10.0 ) * len
                 * ac_factor * sr_er0 * sqrt( M_PI * MU0 * f / sigma );
    atten_dielectric = 20.0 / log( 10.0 ) * len
                       * ad_factor * f * (sr_er_f * sr_er_f - 1) / sr_er_f;

    ang_l = 2.0 * M_PI * len * sr_er_f * f / C0; /* in radians */

    er_eff = sr_er_f * sr_er_f;
    Z0     = zl_factor / sr_er_f;
}


// -------------------------------------------------------------------
void COPLANAR::show_results()
{
    setProperty( Z0_PRM, Z0 );
    setProperty( ANG_L_PRM, ang_l );

    setResult( 0, er_eff, "" );
    setResult( 1, atten_cond, "dB" );
    setResult( 2, atten_dielectric, "dB" );

    setResult( 3, skindepth / UNIT_MICRON, "µm" );
}


// -------------------------------------------------------------------
void COPLANAR::analyze()
{
    getProperties();

    /* compute coplanar parameters */
    calc();

    /* print results in the subwindow */
    show_results();
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
void COPLANAR::synthesize()
{
    double Z0_dest, Z0_current, Z0_result, increment, slope, error;
    int    iteration;

    getProperties();

    /* required value of Z0 */
    Z0_dest = Z0;

    /* Newton's method */
    iteration = 0;

    /* compute coplanar parameters */
    calc();
    Z0_current = Z0;

    error = fabs( Z0_dest - Z0_current );

    while( error > MAX_ERROR )
    {
        iteration++;
        if( isSelected( PHYS_WIDTH_PRM ) )
        {
            increment = w / 100.0;
            w += increment;
        }
        else
        {
            increment = s / 100.0;
            s += increment;
        }
        /* compute coplanar parameters */
        calc();
        Z0_result = Z0;
        /* f(w(n)) = Z0 - Z0(w(n)) */
        /* f'(w(n)) = -f'(Z0(w(n))) */
        /* f'(Z0(w(n))) = (Z0(w(n)) - Z0(w(n+delw))/delw */
        /* w(n+1) = w(n) - f(w(n))/f'(w(n)) */
        slope = (Z0_result - Z0_current) / increment;
        slope = (Z0_dest - Z0_current) / slope - increment;
        if( isSelected( PHYS_WIDTH_PRM ) )
            w += slope;
        else
            s += slope;
        if( w <= 0.0 )
            w = increment;
        if( s <= 0.0 )
            s = increment;
        /* find new error */
        /* compute coplanar parameters */
        calc();
        Z0_current = Z0;
        error = fabs( Z0_dest - Z0_current );
        if( iteration > 100 )
            break;
    }

    setProperty( PHYS_WIDTH_PRM, w );
    setProperty( PHYS_S_PRM, s );
    /* calculate physical length */
    ang_l = getProperty( ANG_L_PRM );
    len   = C0 / f / sqrt( er_eff ) * ang_l / 2.0 / M_PI; /* in m */
    setProperty( PHYS_LEN_PRM, len );

    /* compute coplanar parameters */
    calc();

    /* print results in the subwindow */
    show_results();
}
