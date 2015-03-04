/*
 * coax.cpp - coaxial class implementation
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
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


/*
 * coax.c - Puts up window for microstrip and
 * performs the associated calculations
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

#include <units.h>
#include <transline.h>
#include <coax.h>

COAX::COAX() : TRANSLINE()
{
    m_name = "Coax";

    // Initialize these variables mainly to avoid warnings from a static analyzer
    Z0 = 0.0;               // characteristic impedance
    ang_l = 0.0;            // Electrical length in angle
    atten_dielectric = 0.0; // Loss in dielectric (dB)
    atten_cond = 0.0;       // Loss in conductors (dB)
    fc = 0.0;               // Cutoff frequency for higher order modes
}


/*
 * get_coax_sub() - get and assign coax substrate parameters into coax
 * structure
 */
void COAX::get_coax_sub()
{
    er    = getProperty( EPSILONR_PRM );
    mur   = getProperty( MUR_PRM );
    murC  = getProperty( MURC_PRM );
    tand  = getProperty( TAND_PRM );
    sigma = 1.0 / getProperty( RHO_PRM );
}


/*
 * get_coax_comp() - get and assign coax component parameters into
 * coax structure
 */
void COAX::get_coax_comp()
{
    f = getProperty( FREQUENCY_PRM );
}


/*
 * get_coax_elec() - get and assign coax electrical parameters into
 * coax structure
 */
void COAX::get_coax_elec()
{
    Z0    = getProperty( Z0_PRM );
    ang_l = getProperty( ANG_L_PRM );
}


/*
 * get_coax_phys() - get and assign coax physical parameters into coax
 * structure
 */
void COAX::get_coax_phys()
{
    din  = getProperty( PHYS_DIAM_IN_PRM );
    dout = getProperty( PHYS_DIAM_OUT_PRM );
    l    = getProperty( PHYS_LEN_PRM );
}


double COAX::alphad_coax()
{
    double ad;

    ad = (M_PI / C0) * f * sqrt( er ) * tand;
    ad = ad * 20.0 / log( 10.0 );
    return ad;
}


double COAX::alphac_coax()
{
    double ac, Rs;

    Rs = sqrt( M_PI * f * murC * MU0 / sigma );
    ac = sqrt( er ) * ( ( (1 / din) + (1 / dout) ) / log( dout / din ) ) * (Rs / ZF0);
    ac = ac * 20.0 / log( 10.0 );
    return ac;
}


/*
 * analyze() - analysis function
 */
void COAX::analyze()
{
    double lambda_g;

    /* Get and assign substrate parameters */
    get_coax_sub();

    /* Get and assign component parameters */
    get_coax_comp();

    /* Get and assign physical parameters */
    get_coax_phys();

    if( din != 0.0 )
    {
        Z0 = ( ZF0 / 2 / M_PI / sqrt( er ) ) * log( dout / din );
    }

    lambda_g = ( C0 / (f) ) / sqrt( er * mur );
    /* calculate electrical angle */
    ang_l = (2.0 * M_PI * l) / lambda_g; /* in radians */

    setProperty( Z0_PRM, Z0 );
    setProperty( ANG_L_PRM, ang_l );

    show_results();
}


/*
 * synthesize() - synthesis function
 */
void COAX::synthesize()
{
    double lambda_g;

    /* Get and assign substrate parameters */
    get_coax_sub();

    /* Get and assign component parameters */
    get_coax_comp();

    /* Get and assign electrical parameters */
    get_coax_elec();

    /* Get and assign physical parameters */
    get_coax_phys();

    if( isSelected( PHYS_DIAM_IN_PRM ) )
    {
        /* solve for din */
        din = dout / exp( Z0 * sqrt( er ) / ZF0 * 2 * M_PI );
        setProperty( PHYS_DIAM_IN_PRM, din );
    }
    else if( isSelected( PHYS_DIAM_OUT_PRM ) )
    {
        /* solve for dout */
        dout = din * exp( Z0 * sqrt( er ) / ZF0 * 2 * M_PI );
        setProperty( PHYS_DIAM_OUT_PRM, dout );
    }

    lambda_g = ( C0 / (f) ) / sqrt( er * mur );
    /* calculate physical length */
    l = (lambda_g * ang_l) / (2.0 * M_PI); /* in m */
    setProperty( PHYS_LEN_PRM, l );

    show_results();
}


/*
 * show_results() - show results
 */
void COAX::show_results()
{
    double fc;
    short  m, n;
    char   text[256], txt[256];

    atten_dielectric = alphad_coax() * l;
    atten_cond = alphac_coax() * l;

    setResult( 0, er, "" );
    setResult( 1, atten_cond, "dB" );
    setResult( 2, atten_dielectric, "dB" );

    n  = 1;
    fc = C0 / (M_PI * (dout + din) / (double) n);
    if( fc > f )
        strcpy( text, "none" );
    else
    {
        strcpy( text, "H(1,1) " );
        m  = 2;
        fc = C0 / ( 2 * (dout - din) / (double) (m - 1) );
        while( (fc <= f) && (m<10) )
        {
            sprintf( txt, "H(n,%d) ", m );
            strcat( text, txt );
            m++;
            fc = C0 / ( 2 * (dout - din) / (double) (m - 1) );
        }
    }
    setResult( 3, text );

    m  = 1;
    fc = C0 / (2 * (dout - din) / (double) m);
    if( fc > f )
        strcpy( text, "none" );
    else
    {
        strcpy( text, "" );
        while( (fc <= f) && (m<10) )
        {
            sprintf( txt, "E(n,%d) ", m );
            strcat( text, txt );
            m++;
            fc = C0 / (2 * (dout - din) / (double) m);
        }
    }
    setResult( 4, text );
}
