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

#include <cmath>
#include <cstdio>
#include <cstring>

#include "rectwaveguide.h"
#include "units.h"

RECTWAVEGUIDE::RECTWAVEGUIDE() : TRANSLINE(),
    mur( 0.0 ),                 // magnetic permeability of substrate
    a( 0.0 ),                   // width of waveguide
    b( 0.0 ),                   // height of waveguide
    l( 0.0 ),                   // length of waveguide
    Z0( 0.0 ),                  // characteristic impedance
    Z0EH( 0.0 ),                // characteristic impedance of field quantities*/
    mur_eff( 0.0 ),             // Effective mag. permeability
    atten_dielectric( 0.0 ),    // Loss in dielectric (dB)
    atten_cond( 0.0 ),          // Loss in conductors (dB)
    fc10( 1.0 )                 // Cutoff frequency for TE10 mode
{
    m_Name = "RectWaveGuide";
    Init();
}


/*
 * returns square of k
 */
double RECTWAVEGUIDE::kval_square()
{
    double kval;

    kval = 2.0 * M_PI * m_parameters[FREQUENCY_PRM]
           * sqrt( m_parameters[MUR_PRM] * m_parameters[EPSILONR_PRM] ) / C0;

    return kval * kval;
}


/*
 * given mode numbers m and n
 * returns square of cutoff kc value
 */
double RECTWAVEGUIDE::kc_square( int m, int n )
{
    return pow( ( m * M_PI / m_parameters[PHYS_A_PRM] ), 2.0 )
           + pow( ( n * M_PI / m_parameters[PHYS_B_PRM] ), 2.0 );
}


/*
 * given mode numbers m and n
 * returns cutoff fc value
 */
double RECTWAVEGUIDE::fc( int m, int n )
{
    return sqrt( kc_square( m, n ) / m_parameters[MUR_PRM] / m_parameters[EPSILONR_PRM] ) * C0 / 2.0
           / M_PI;
}


/*
 * alphac - returns attenuation due to conductor losses for all propagating
 * modes in the waveguide
 */
double RECTWAVEGUIDE::alphac()
{
    double  Rs, f_c;
    double  ac;
    short   m, n, mmax, nmax;
    double* a     = &m_parameters[PHYS_A_PRM];
    double* b     = &m_parameters[PHYS_B_PRM];
    double* f     = &m_parameters[FREQUENCY_PRM];
    double* murc  = &m_parameters[MURC_PRM];
    double* sigma = &m_parameters[SIGMA_PRM];

    Rs   = sqrt( M_PI * *f * *murc * MU0 / *sigma );
    ac   = 0.0;
    mmax = (int) floor( *f / fc( 1, 0 ) );
    nmax = mmax;

    /* below from Ramo, Whinnery & Van Duzer */

    /* TE(m,n) modes */
    for( n = 0; n <= nmax; n++ )
    {
        for( m = 1; m <= mmax; m++ )
        {
            f_c = fc( m, n );

            if( *f > f_c )
            {
                switch( n )
                {
                case 0:
                    ac += ( Rs / ( *b * ZF0 * sqrt( 1.0 - pow( ( f_c / *f ), 2.0 ) ) ) )
                          * ( 1.0 + ( ( 2 * *b / *a ) * pow( ( f_c / *f ), 2.0 ) ) );
                    break;

                default:
                    ac += ( ( 2. * Rs ) / ( *b * ZF0 * sqrt( 1.0 - pow( ( f_c / *f ), 2.0 ) ) ) )
                          * ( ( ( 1. + ( *b / *a ) ) * pow( ( f_c / *f ), 2.0 ) )
                                  + ( ( 1. - pow( ( f_c / *f ), 2.0 ) )
                                          * ( ( ( *b / *a )
                                                      * ( ( ( *b / *a ) * pow( m, 2. ) )
                                                              + pow( n, 2. ) ) )
                                                  / ( pow( ( *b * m / *a ), 2.0 )
                                                          + pow( n, 2.0 ) ) ) ) );
                    break;
                }
            }
        }
    }

    /* TM(m,n) modes */
    for( n = 1; n <= nmax; n++ )
    {
        for( m = 1; m <= mmax; m++ )
        {
            f_c = fc( m, n );

            if( *f > f_c )
            {
                ac += ( ( 2. * Rs ) / ( *b * ZF0 * sqrt( 1.0 - pow( ( f_c / *f ), 2.0 ) ) ) )
                      * ( ( ( pow( m, 2.0 ) * pow( ( *b / *a ), 3.0 ) ) + pow( n, 2. ) )
                              / ( ( pow( ( m * *b / *a ), 2. ) ) + pow( n, 2.0 ) ) );
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

    ad = ( k_square * m_parameters[TAND_PRM] ) / ( 2.0 * beta );
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
    m_parameters[EPSILONR_PRM] = getProperty( EPSILONR_PRM );
    m_parameters[MUR_PRM]      = getProperty( MUR_PRM );
    m_parameters[MURC_PRM]     = getProperty( MURC_PRM );
    m_parameters[SIGMA_PRM]    = 1.0 / getProperty( RHO_PRM );
    m_parameters[TAND_PRM]     = getProperty( TAND_PRM );
}


/*
 * get_rectwaveguide_comp
 * get and assign rectwaveguide component parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_comp()
{
    m_parameters[FREQUENCY_PRM] = getProperty( FREQUENCY_PRM );
}


/*
 * get_rectwaveguide_elec
 * get and assign rectwaveguide electrical parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_elec()
{
    m_parameters[Z0_PRM]    = getProperty( Z0_PRM );
    m_parameters[ANG_L_PRM] = getProperty( ANG_L_PRM );
}


/*
 * get_rectwaveguide_phys
 * get and assign rectwaveguide physical parameters
 * into rectwaveguide structure
 */
void RECTWAVEGUIDE::get_rectwaveguide_phys()
{
    m_parameters[PHYS_A_PRM]   = getProperty( PHYS_A_PRM );
    m_parameters[PHYS_B_PRM]   = getProperty( PHYS_B_PRM );
    m_parameters[PHYS_LEN_PRM] = getProperty( PHYS_LEN_PRM );
}


/*
 * analyze - analysis function
 * source: https://empossible.net/wp-content/uploads/2018/03/Lecture-5c-Rectangular-waveguide.pdf
 */
void RECTWAVEGUIDE::calcAnalyze()
{
    double lambda_g;
    double k_square;

    k_square = kval_square();

    if( kc_square( 1, 0 ) <= k_square )
    {
        /* propagating modes */

        // Z0 definition using fictive voltages and currents
        m_parameters[Z0_PRM] =
            ZF0 * sqrt( m_parameters[MUR_PRM] / m_parameters[EPSILONR_PRM] )
            / sqrt( 1.0 - pow( ( fc( 1, 0 ) / m_parameters[FREQUENCY_PRM] ), 2.0 ) );

        /* calculate electrical angle */
        lambda_g = 2.0 * M_PI / sqrt( k_square - kc_square( 1, 0 ) );
        m_parameters[ANG_L_PRM] =
                2.0 * M_PI * m_parameters[PHYS_LEN_PRM] / lambda_g; /* in radians */
        m_parameters[LOSS_CONDUCTOR_PRM]  = alphac() * m_parameters[PHYS_LEN_PRM];
        m_parameters[LOSS_DIELECTRIC_PRM] = alphad() * m_parameters[PHYS_LEN_PRM];
        m_parameters[EPSILON_EFF_PRM] =
                ( 1.0 - pow( fc( 1, 0 ) / m_parameters[FREQUENCY_PRM], 2.0 ) );
    }
    else
    {
        /* evanascent modes */
        m_parameters[Z0_PRM]              = 0;
        m_parameters[ANG_L_PRM]           = 0;
        m_parameters[EPSILON_EFF_PRM]     = 0;
        m_parameters[LOSS_DIELECTRIC_PRM] = 0.0;
        m_parameters[LOSS_CONDUCTOR_PRM]  = alphac_cutoff() * m_parameters[PHYS_LEN_PRM];
    }
}

/*
 * synthesize - synthesis function
 * source: re-arrangment of calcAnalyze equation
 * TE10 (via fc(1,0) ) results in the b term not influencing the result, as long as
 * 1) fc > f
 * 2) a > b
 */
void RECTWAVEGUIDE::calcSynthesize()
{
    double lambda_g, k_square, beta;
    /* solve for a */
    m_parameters[PHYS_A_PRM] =
        C0
        / ( sqrt( ( m_parameters[MUR_PRM] * m_parameters[EPSILONR_PRM] ) ) * 2
        * m_parameters[FREQUENCY_PRM] * sqrt( 1
            - pow( ( ZF0 * sqrt( m_parameters[MUR_PRM]
                / m_parameters[EPSILONR_PRM] ) )
                / m_parameters[Z0_PRM]
                ,2.0 ) ) );

    k_square                   = kval_square();
    beta                       = sqrt( k_square - kc_square( 1, 0 ) );
    lambda_g                   = 2.0 * M_PI / beta;
    m_parameters[PHYS_LEN_PRM] = ( m_parameters[ANG_L_PRM] * lambda_g ) / ( 2.0 * M_PI ); /* in m */

    if( kc_square( 1, 0 ) <= k_square )
    {
        /*propagating modes */
        beta                              = sqrt( k_square - kc_square( 1, 0 ) );
        lambda_g                          = 2.0 * M_PI / beta;
        m_parameters[LOSS_CONDUCTOR_PRM]  = alphac() * m_parameters[PHYS_LEN_PRM];
        m_parameters[LOSS_DIELECTRIC_PRM] = alphad() * m_parameters[PHYS_LEN_PRM];
        m_parameters[EPSILON_EFF_PRM] =
                ( 1.0 - pow( ( fc( 1, 0 ) / m_parameters[FREQUENCY_PRM] ), 2.0 ) );
    }
    else
    {
        /*evanascent modes */
        m_parameters[Z0_PRM]              = 0;
        m_parameters[ANG_L_PRM]           = 0;
        m_parameters[EPSILON_EFF_PRM]     = 0;
        m_parameters[LOSS_DIELECTRIC_PRM] = 0.0;
        m_parameters[LOSS_CONDUCTOR_PRM]  = alphac_cutoff() * m_parameters[PHYS_LEN_PRM];
    }
}

void RECTWAVEGUIDE::showSynthesize()
{
    if( isSelected( PHYS_A_PRM ) )
        setProperty( PHYS_A_PRM, m_parameters[PHYS_A_PRM] );

    if( isSelected( PHYS_B_PRM ) )
        setProperty( PHYS_B_PRM, m_parameters[PHYS_B_PRM] );

    setProperty( PHYS_LEN_PRM, m_parameters[PHYS_LEN_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[PHYS_A_PRM] ) || m_parameters[PHYS_A_PRM] <= 0 )
        setErrorLevel( PHYS_A_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[PHYS_B_PRM] ) || m_parameters[PHYS_B_PRM] <= 00 )
        setErrorLevel( PHYS_B_PRM, TRANSLINE_ERROR );

    // Check for warnings
    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_WARNING );
}


void RECTWAVEGUIDE::showAnalyze()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_ERROR );

    // Check for warnings
    if( !std::isfinite( m_parameters[PHYS_A_PRM] ) || m_parameters[PHYS_A_PRM] <= 0 )
        setErrorLevel( PHYS_A_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_B_PRM] ) || m_parameters[PHYS_B_PRM] <= 00 )
        setErrorLevel( PHYS_B_PRM, TRANSLINE_WARNING );
}


#define MAXSTRLEN 128
void RECTWAVEGUIDE::show_results()
{
    int  m, n, max = 6;
    char text[MAXSTRLEN], txt[32];

    // Z0EH = Ey / Hx (definition with field quantities)
    Z0EH = ZF0 * sqrt( kval_square() / ( kval_square() - kc_square( 1, 0 ) ) );
    setResult( 0, Z0EH, "Ohm" );

    setResult( 1, m_parameters[EPSILON_EFF_PRM], "" );
    setResult( 2, m_parameters[LOSS_CONDUCTOR_PRM], "dB" );
    setResult( 3, m_parameters[LOSS_DIELECTRIC_PRM], "dB" );

    // show possible TE modes (H modes)
    if( m_parameters[FREQUENCY_PRM] < fc( 1, 0 ) )
    {
        strcpy( text, "none" );
    }
    else
    {
        strcpy( text, "" );
        for( m = 0; m <= max; m++ )
        {
            for( n = 0; n <= max; n++ )
            {
                if( ( m == 0 ) && ( n == 0 ) )
                    continue;

                if( m_parameters[FREQUENCY_PRM] >= ( fc( m, n ) ) )
                {
                    snprintf( txt, sizeof( txt ), "H(%d,%d) ", m, n );
                    if( ( strlen( text ) + strlen( txt ) + 5 ) < MAXSTRLEN )
                    {
                        strcat( text, txt );
                    }
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
    if( m_parameters[FREQUENCY_PRM] < fc( 1, 1 ) )
    {
        strcpy( text, "none" );
    }
    else
    {
        strcpy( text, "" );
        for( m = 1; m <= max; m++ )
        {
            for( n = 1; n <= max; n++ )
            {
                if( m_parameters[FREQUENCY_PRM] >= fc( m, n ) )
                {
                    snprintf( txt, sizeof( txt ), "E(%d,%d) ", m, n );
                    if( ( strlen( text ) + strlen( txt ) + 5 ) < MAXSTRLEN )
                    {
                        strcat( text, txt );
                    }
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
