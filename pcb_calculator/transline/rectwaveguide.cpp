/*
 * rectwaveguide.cpp - rectangular waveguide class implementation
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
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

#include <stdio.h>
#include <string.h>
#include <cmath>

#include <units.h>
#include <transline.h>
#include <rectwaveguide.h>

RECTWAVEGUIDE::RECTWAVEGUIDE() : TRANSLINE()
{
    m_name = "RectWaveGuide";

    // Initialize these here variables mainly to avoid warnings from a static analyzer
    mur = 0.0;                  // magnetic permeability of substrate
    tanm = 0.0;                 // Magnetic Loss Tangent
    a = 0.0;                    // width of waveguide
    b = 0.0;                    // height of waveguide
    l = 0.0;                    // length of waveguide
    Z0 = 0.0;                   // characteristic impedance
    Z0EH = 0.0;                 // characteristic impedance of field quantities*/
    ang_l = 0.0;                // Electrical length in angle
    er_eff = 0.0;               // Effective dielectric constant
    mur_eff = 0.0;              // Effective mag. permeability
    atten_dielectric = 0.0;     // Loss in dielectric (dB)
    atten_cond = 0.0;           // Loss in conductors (dB)
    fc10 = 0.0;                 // Cutoff frequency for TE10 mode
}


/*
 * returns square of k
 */
double RECTWAVEGUIDE::kval_square()
{
    double kval;

    kval = 2.0* M_PI* f* sqrt( mur* er ) / C0;

    return kval * kval;
}


/*
 * given mode numbers m and n
 * returns square of cutoff kc value
 */
double RECTWAVEGUIDE::kc_square( int m, int n )
{
    return pow( (m * M_PI / a), 2.0 ) + pow( (n * M_PI / b), 2.0 );
}


/*
 * given mode numbers m and n
 * returns cutoff fc value
 */
double RECTWAVEGUIDE::fc( int m, int n )
{
    return sqrt( kc_square( m, n ) / mur / er ) * C0 / 2.0 / M_PI;
}


/*
 * alphac - returns attenuation due to conductor losses for all propagating
 * modes in the waveguide
 */
double RECTWAVEGUIDE::alphac()
{
    double Rs, f_c;
    double ac;
    short  m, n, mmax, nmax;

    Rs   = sqrt( M_PI * f * murC * MU0 / sigma );
    ac   = 0.0;
    mmax = (int) floor( f / fc( 1, 0 ) );
    nmax = mmax;

    /* below from Ramo, Whinnery & Van Duzer */

    /* TE(m,n) modes */
    for( n = 0; n<= nmax; n++ )
    {
        for( m = 1; m <= mmax; m++ )
        {
            f_c = fc( m, n );
            if( f > f_c )
            {
                switch( n )
                {
                case 0:
                    ac += ( Rs / ( b * ZF0 * sqrt( 1.0 - pow( (f_c / f), 2.0 ) ) ) ) *
                          ( 1.0 + ( (2 * b / a) * pow( (f_c / f), 2.0 ) ) );
                    break;

                default:
                    ac += ( (2. * Rs) / ( b * ZF0 * sqrt( 1.0 - pow( (f_c / f), 2.0 ) ) ) ) *
                          ( ( ( 1. + (b / a) ) * pow( (f_c / f), 2.0 ) ) +
                           ( ( 1. -
                              pow( (f_c / f),
                                  2.0 ) ) *
                            ( ( (b / a) * ( ( (b / a) * pow( m, 2. ) ) + pow( n, 2. ) ) ) /
                    ( pow( (b * m / a),
                          2.0 ) + pow( n, 2.0 ) ) ) ) );
                    break;
                }
            }
        }
    }

    /* TM(m,n) modes */
    for( n = 1; n<= nmax; n++ )
    {
        for( m = 1; m<= mmax; m++ )
        {
            f_c = fc( m, n );
            if( f > f_c )
            {
                ac += ( (2. * Rs) / ( b * ZF0 * sqrt( 1.0 - pow( (f_c / f), 2.0 ) ) ) ) *
                      ( ( ( pow( m, 2.0 ) * pow( (b / a), 3.0 ) ) + pow( n, 2. ) ) /
                       ( ( pow( (m * b / a), 2. ) ) + pow( n, 2.0 ) ) );
            }
        }
    }

    ac = ac * 20.0 * log10( exp( 1. ) ); /* convert from Np/m to db/m */
    return ac;
}


/*
 * alphac_cutoff - returns attenuation for a cutoff wg
 */
double RECTWAVEGUIDE::alphac_cutoff()
{
    double acc;

    acc = sqrt( kc_square( 1, 0 ) - kval_square() );
    acc = 20 * log10( exp( 1.0 ) ) * acc;
    return acc;
}


/*
 * returns attenuation due to dielectric losses
 */
double RECTWAVEGUIDE::alphad()
{
    double k_square, beta;
    double ad;

    k_square = kval_square();
    beta     = sqrt( k_square - kc_square( 1, 0 ) );

    ad = (k_square * tand) / (2.0 * beta);
    ad = ad * 20.0 * log10( exp( 1. ) ); /* convert from Np/m to db/m */
    return ad;
}


/*
 * get_rectwaveguide_sub
 * get and assign rectwaveguide substrate parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_sub()
{
    er    = getProperty( EPSILONR_PRM );
    mur   = getProperty( MUR_PRM );
    murC  = getProperty( MURC_PRM );
    sigma = 1.0 / getProperty( RHO_PRM );
    tand  = getProperty( TAND_PRM );
    tanm  = getProperty( TANM_PRM );
}


/*
 * get_rectwaveguide_comp
 * get and assign rectwaveguide component parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_comp()
{
    f = getProperty( FREQUENCY_PRM );
}


/*
 * get_rectwaveguide_elec
 * get and assign rectwaveguide electrical parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_elec()
{
    Z0    = getProperty( Z0_PRM );
    ang_l = getProperty( ANG_L_PRM );
}


/*
 * get_rectwaveguide_phys
 * get and assign rectwaveguide physical parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_phys()
{
    a = getProperty( PHYS_WIDTH_PRM );
    b = getProperty( PHYS_S_PRM );
    l = getProperty( PHYS_LEN_PRM );
}


/*
 * analyze - analysis function
 */
void RECTWAVEGUIDE::analyze()
{
    double lambda_g;
    double k_square;

    /* Get and assign substrate parameters */
    get_rectwaveguide_sub();

    /* Get and assign component parameters */
    get_rectwaveguide_comp();

    /* Get and assign physical parameters */
    get_rectwaveguide_phys();

    k_square = kval_square();

    if( kc_square( 1, 0 ) <= k_square )
    {
        /* propagating modes */

        // Z0 definition using fictive voltages and currents
        Z0 = 2.0* ZF0* sqrt( mur / er ) * (b / a) / sqrt( 1.0 - pow( (fc( 1, 0 ) / f), 2.0 ) );

        /* calculate electrical angle */
        lambda_g   = 2.0 * M_PI / sqrt( k_square - kc_square( 1, 0 ) );
        ang_l      = 2.0 * M_PI * l / lambda_g; /* in radians */
        atten_cond = alphac() * l;
        atten_dielectric = alphad() * l;
        er_eff = ( 1.0 - pow( fc( 1, 0 ) / f, 2.0 ) );
    }
    else
    {
        /* evanascent modes */
        Z0     = 0;
        ang_l  = 0;
        er_eff = 0;
        atten_dielectric = 0.0;
        atten_cond = alphac_cutoff() * l;
    }

    setProperty( Z0_PRM, Z0 );
    setProperty( ANG_L_PRM, ang_l );

    show_results();
}


/*
 * synthesize - synthesis function
 */
void RECTWAVEGUIDE::synthesize()
{
    double lambda_g, k_square, beta;

    /* Get and assign substrate parameters */
    get_rectwaveguide_sub();

    /* Get and assign component parameters */
    get_rectwaveguide_comp();

    /* Get and assign electrical parameters */
    get_rectwaveguide_elec();

    /* Get and assign physical parameters */
    get_rectwaveguide_phys();


    if( isSelected( PHYS_S_PRM ) )
    {
        /* solve for b */
        b = Z0 * a * sqrt( 1.0 - pow( fc( 1, 0 ) / f, 2.0 ) ) / ( 2.0 * ZF0 * sqrt( mur / er ) );
        setProperty( PHYS_S_PRM, b );
    }
    else if( isSelected( PHYS_WIDTH_PRM ) )
    {
        /* solve for a */
        a = sqrt( pow( 2.0 * ZF0 * b / Z0, 2.0 ) + pow( C0 / (2.0 * f), 2.0 ) );
        setProperty( PHYS_WIDTH_PRM, a );
    }

    k_square = kval_square();
    beta     = sqrt( k_square - kc_square( 1, 0 ) );
    lambda_g = 2.0 * M_PI / beta;
    l = (ang_l * lambda_g) / (2.0 * M_PI); /* in m */

    setProperty( PHYS_LEN_PRM, l );

    if( kc_square( 1, 0 ) <= k_square )
    {
        /*propagating modes */
        beta = sqrt( k_square - kc_square( 1, 0 ) );
        lambda_g   = 2.0 * M_PI / beta;
        atten_cond = alphac() * l;
        atten_dielectric = alphad() * l;
        er_eff = ( 1.0 - pow( (fc( 1, 0 ) / f), 2.0 ) );
    }
    else
    {
        /*evanascent modes */
        Z0     = 0;
        ang_l  = 0;
        er_eff = 0;
        atten_dielectric = 0.0;
        atten_cond = alphac_cutoff() * l;
    }

    show_results();
}


#define MAXSTRLEN 128
void RECTWAVEGUIDE::show_results()
{
    int  m, n, max = 6;
    char text[MAXSTRLEN], txt[32];

    // Z0EH = Ey / Hx (definition with field quantities)
    Z0EH = ZF0 * sqrt( kval_square() / ( kval_square() - kc_square( 1, 0 ) ) );
    setResult( 0, Z0EH, "Ohm" );

    setResult( 1, er_eff, "" );
    setResult( 2, atten_cond, "dB" );
    setResult( 3, atten_dielectric, "dB" );

    // show possible TE modes (H modes)
    if( f < fc( 1, 0 ) )
        strcpy( text, "none" );
    else
    {
        strcpy( text, "" );
        for( m = 0; m<= max; m++ )
        {
            for( n = 0; n<= max; n++ )
            {
                if( (m == 0) && (n == 0) )
                    continue;
                if( f >= ( fc( m, n ) ) )
                {
                    sprintf( txt, "H(%d,%d) ", m, n );
                    if( (strlen( text ) + strlen( txt ) + 5) < MAXSTRLEN )
                        strcat( text, txt );
                    else
                    {
                        strcat( text, "..." );
                        m = n = max + 1; // print no more modes
                    }
                }
            }
        }
    }
    setResult( 4, text );

    // show possible TM modes (E modes)
    if( f < fc( 1, 1 ) )
        strcpy( text, "none" );
    else
    {
        strcpy( text, "" );
        for( m = 1; m<= max; m++ )
        {
            for( n = 1; n<= max; n++ )
            {
                if( f >= fc( m, n ) )
                {
                    sprintf( txt, "E(%d,%d) ", m, n );
                    if( (strlen( text ) + strlen( txt ) + 5) < MAXSTRLEN )
                        strcat( text, txt );
                    else
                    {
                        strcat( text, "..." );
                        m = n = max + 1; // print no more modes
                    }
                }
            }
        }
    }
    setResult( 5, text );
}
