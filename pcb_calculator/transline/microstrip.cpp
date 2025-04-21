/*
 * microstrip.cpp - microstrip class implementation
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2018 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


/* microstrip.c - Puts up window for microstrip and
 * performs the associated calculations
 * Based on the original microstrip.c by Gopal Narayanan
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "microstrip.h"
#include "transline.h"

#include "units.h"

MICROSTRIP::MICROSTRIP() : TRANSLINE(),
    h( 0.0 ),                // height of substrate
    ht( 0.0 ),               // height to the top of box
    t( 0.0 ),                // thickness of top metal
    rough( 0.0 ),            // Roughness of top metal
    mur( 0.0 ),              // magnetic permeability of substrate
    w( 0.0 ),                // width of line
    l( 0.0 ),                // length of line
    Z0_0( 0.0 ),             // static characteristic impedance
    Z0( 0.0 ),               // characteristic impedance
    er_eff_0( 0.0 ),         // Static effective dielectric constant
    mur_eff( 0.0 ),          // Effective mag. permeability
    w_eff( 0.0 ),            // Effective width of line
    atten_dielectric( 0.0 ), // Loss in dielectric (dB)
    atten_cond( 0.0 ),       // Loss in conductors (dB)
    Z0_h_1( 0.0 )            // homogeneous stripline impedance
{
    m_Name = "MicroStrip";
    Init();
}


/*
 * Z0_homogeneous() - compute the impedance for a stripline in a
 * homogeneous medium, without cover effects
 */
double MICROSTRIP::Z0_homogeneous( double u )
{
    double freq, Z0_value;

    freq     = 6.0 + ( 2.0 * M_PI - 6.0 ) * exp( -pow( 30.666 / u, 0.7528 ) );
    Z0_value = ( ZF0 / ( 2.0 * M_PI ) ) * log( freq / u + sqrt( 1.0 + 4.0 / ( u * u ) ) );
    return Z0_value;
}


/*
 * delta_Z0_cover() - compute the cover effect on impedance for a
 * stripline in a homogeneous medium
 */
double MICROSTRIP::delta_Z0_cover( double u, double h2h )
{
    double P, Q;
    double h2hp1;

    h2hp1 = 1.0 + h2h;
    P     = 270.0 * ( 1.0 - tanh( 1.192 + 0.706 * sqrt( h2hp1 ) - 1.389 / h2hp1 ) );
    Q     = 1.0109 - atanh( ( 0.012 * u + 0.177 * u * u - 0.027 * u * u * u ) / ( h2hp1 * h2hp1 ) );
    return P * Q;
}


/*
 * filling_factor() - compute the filling factor for a microstrip
 * without cover and zero conductor thickness
 */
double MICROSTRIP::filling_factor( double u, double e_r )
{
    double a, b, q_inf;
    double u2, u3, u4;

    u2 = u * u;
    u3 = u2 * u;
    u4 = u3 * u;
    a = 1.0 + log( ( u4 + u2 / 2704 ) / ( u4 + 0.432 ) ) / 49.0 + log( 1.0 + u3 / 5929.741 ) / 18.7;
    b = 0.564 * pow( ( e_r - 0.9 ) / ( e_r + 3.0 ), 0.053 );
    q_inf = pow( 1.0 + 10.0 / u, -a * b );
    return q_inf;
}


/*
 * delta_q_cover() - compute the cover effect on filling factor
 */
double MICROSTRIP::delta_q_cover( double h2h )
{
    double q_c;

    q_c = tanh( 1.043 + 0.121 * h2h - 1.164 / h2h );
    return q_c;
}


/*
 * delta_q_thickness() - compute the thickness effect on filling factor
 */
double MICROSTRIP::delta_q_thickness( double u, double t_h )
{
    double q_t;

    q_t = ( 2.0 * log( 2.0 ) / M_PI ) * ( t_h / sqrt( u ) );
    return q_t;
}


/*
 * e_r_effective() - compute effective dielectric constant from
 * material e_r and filling factor
 */
double MICROSTRIP::e_r_effective( double e_r, double q )
{
    double e_r_eff;

    e_r_eff = 0.5 * ( e_r + 1.0 ) + 0.5 * q * ( e_r - 1.0 );
    return e_r_eff;
}


/*
 * delta_u_thickness - compute the thickness effect on normalized width
 */
double MICROSTRIP::delta_u_thickness( double u, double t_h, double e_r )
{
    double delta_u;

    if( t_h > 0.0 )
    {
        /* correction for thickness for a homogeneous microstrip */
        delta_u = ( t_h / M_PI )
                  * log( 1.0 + ( 4.0 * M_E ) * pow( tanh( sqrt( 6.517 * u ) ), 2.0 ) / t_h );
        /* correction for strip on a substrate with relative permettivity e_r */
        delta_u = 0.5 * delta_u * ( 1.0 + 1.0 / cosh( sqrt( e_r - 1.0 ) ) );
    }
    else
    {
        delta_u = 0.0;
    }
    return delta_u;
}


/*
 * microstrip_Z0() - compute microstrip static impedance
 */
void MICROSTRIP::microstrip_Z0()
{
    double e_r, h2, h2h, u, t_h;
    double Z0_h_r;
    double delta_u_1, delta_u_r, q_inf, q_c, q_t, e_r_eff, e_r_eff_t, q;

    e_r = m_parameters[EPSILONR_PRM];
    h2  = m_parameters[H_T_PRM];
    h2h = h2 / m_parameters[H_PRM];
    u   = m_parameters[PHYS_WIDTH_PRM] / m_parameters[H_PRM];
    t_h = m_parameters[T_PRM] / m_parameters[H_PRM];

    /* compute normalized width correction for e_r = 1.0 */
    delta_u_1 = delta_u_thickness( u, t_h, 1.0 );
    /* compute homogeneous stripline impedance */
    Z0_h_1 = Z0_homogeneous( u + delta_u_1 );
    /* compute normalized width correction */
    delta_u_r = delta_u_thickness( u, t_h, e_r );
    u += delta_u_r;
    /* compute homogeneous stripline impedance */
    Z0_h_r = Z0_homogeneous( u );

    /* filling factor, with width corrected for thickness */
    q_inf = filling_factor( u, e_r );
    /* cover effect */
    q_c = delta_q_cover( h2h );
    /* thickness effect */
    q_t = delta_q_thickness( u, t_h );
    /* resultant filling factor */
    q = ( q_inf - q_t ) * q_c;

    /* e_r corrected for thickness and non homogeneous material */
    e_r_eff_t = e_r_effective( e_r, q );

    /* effective dielectric constant */
    e_r_eff = e_r_eff_t * pow( Z0_h_1 / Z0_h_r, 2.0 );

    /* characteristic impedance, corrected for thickness, cover */
    /*   and non homogeneous material */
    m_parameters[Z0_PRM] = Z0_h_r / sqrt( e_r_eff_t );

    w_eff    = u * m_parameters[H_PRM];
    er_eff_0 = e_r_eff;
    Z0_0     = m_parameters[Z0_PRM];
}


/*
 * e_r_dispersion() - computes the dispersion correction factor for
 * the effective permeability
 */
double MICROSTRIP::e_r_dispersion( double u, double e_r, double f_n )
{
    double P_1, P_2, P_3, P_4, P;

    P_1 = 0.27488 + u * ( 0.6315 + 0.525 / pow( 1.0 + 0.0157 * f_n, 20.0 ) )
          - 0.065683 * exp( -8.7513 * u );
    P_2 = 0.33622 * ( 1.0 - exp( -0.03442 * e_r ) );
    P_3 = 0.0363 * exp( -4.6 * u ) * ( 1.0 - exp( -pow( f_n / 38.7, 4.97 ) ) );
    P_4 = 1.0 + 2.751 * ( 1.0 - exp( -pow( e_r / 15.916, 8.0 ) ) );

    P = P_1 * P_2 * pow( ( P_3 * P_4 + 0.1844 ) * f_n, 1.5763 );

    return P;
}


/*
 * Z0_dispersion() - computes the dispersion correction factor for the
 * characteristic impedance
 */
double MICROSTRIP::Z0_dispersion(
        double u, double e_r, double e_r_eff_0, double e_r_eff_f, double f_n )
{
    double R_1, R_2, R_3, R_4, R_5, R_6, R_7, R_8, R_9, R_10, R_11, R_12, R_13, R_14, R_15, R_16,
           R_17, D, tmpf;

    R_1 = 0.03891 * pow( e_r, 1.4 );
    R_2 = 0.267 * pow( u, 7.0 );
    R_3 = 4.766 * exp( -3.228 * pow( u, 0.641 ) );
    R_4 = 0.016 + pow( 0.0514 * e_r, 4.524 );
    R_5 = pow( f_n / 28.843, 12.0 );
    R_6 = 22.2 * pow( u, 1.92 );
    R_7 = 1.206 - 0.3144 * exp( -R_1 ) * ( 1.0 - exp( -R_2 ) );
    R_8 = 1.0
          + 1.275
                    * ( 1.0
                            - exp( -0.004625 * R_3 * pow( e_r, 1.674 )
                                    * pow( f_n / 18.365, 2.745 ) ) );
    tmpf = pow( e_r - 1.0, 6.0 );
    R_9  = 5.086 * R_4 * ( R_5 / ( 0.3838 + 0.386 * R_4 ) )
          * ( exp( -R_6 ) / ( 1.0 + 1.2992 * R_5 ) ) * ( tmpf / ( 1.0 + 10.0 * tmpf ) );
    R_10 = 0.00044 * pow( e_r, 2.136 ) + 0.0184;
    tmpf = pow( f_n / 19.47, 6.0 );
    R_11 = tmpf / ( 1.0 + 0.0962 * tmpf );
    R_12 = 1.0 / ( 1.0 + 0.00245 * u * u );
    R_13 = 0.9408 * pow( e_r_eff_f, R_8 ) - 0.9603;
    R_14 = ( 0.9408 - R_9 ) * pow( e_r_eff_0, R_8 ) - 0.9603;
    R_15 = 0.707 * R_10 * pow( f_n / 12.3, 1.097 );
    R_16 = 1.0 + 0.0503 * e_r * e_r * R_11 * ( 1.0 - exp( -pow( u / 15.0, 6.0 ) ) );
    R_17 = R_7 * ( 1.0 - 1.1241 * ( R_12 / R_16 ) * exp( -0.026 * pow( f_n, 1.15656 ) - R_15 ) );

    D = pow( R_13 / R_14, R_17 );

    return D;
}


/*
 * dispersion() - compute frequency dependent parameters of
 * microstrip
 */
void MICROSTRIP::dispersion()
{
    double e_r, e_r_eff_0;
    double u, f_n, P, e_r_eff_f, D, Z0_f;

    e_r       = m_parameters[EPSILONR_PRM];
    e_r_eff_0 = er_eff_0;
    u         = m_parameters[PHYS_WIDTH_PRM] / m_parameters[H_PRM];

    /* normalized frequency [GHz * mm] */
    f_n = m_parameters[FREQUENCY_PRM] * m_parameters[H_PRM] / 1e06;

    P = e_r_dispersion( u, e_r, f_n );
    /* effective dielectric constant corrected for dispersion */
    e_r_eff_f = e_r - ( e_r - e_r_eff_0 ) / ( 1.0 + P );

    D    = Z0_dispersion( u, e_r, e_r_eff_0, e_r_eff_f, f_n );
    Z0_f = Z0_0 * D;

    // 1e10 factor is to convert from s/m to ps/cm
    unit_prop_delay = calcUnitPropagationDelay( e_r_eff_f );

    m_parameters[EPSILON_EFF_PRM] = e_r_eff_f;
    m_parameters[Z0_PRM] = Z0_f;
}


/*
 * conductor_losses() - compute microstrip conductor losses per unit
 * length
 */
double MICROSTRIP::conductor_losses()
{
    double e_r_eff_0, delta;
    double K, R_s, Q_c, alpha_c;

    e_r_eff_0 = er_eff_0;
    delta     = m_parameters[SKIN_DEPTH_PRM];

    if( m_parameters[FREQUENCY_PRM] > 0.0 )
    {
        /* current distribution factor */
        K = exp( -1.2 * pow( Z0_h_1 / ZF0, 0.7 ) );
        /* skin resistance */
        R_s = 1.0 / ( m_parameters[SIGMA_PRM] * delta );

        /* correction for surface roughness */
        R_s *= 1.0
               + ( ( 2.0 / M_PI )
                       * atan( 1.40 * pow( ( m_parameters[ROUGH_PRM] / delta ), 2.0 ) ) );
        /* strip inductive quality factor */
        Q_c = ( M_PI * Z0_h_1 * m_parameters[PHYS_WIDTH_PRM] * m_parameters[FREQUENCY_PRM] )
              / ( R_s * C0 * K );
        alpha_c = ( 20.0 * M_PI / log( 10.0 ) ) * m_parameters[FREQUENCY_PRM] * sqrt( e_r_eff_0 )
                  / ( C0 * Q_c );
    }
    else
    {
        alpha_c = 0.0;
    }

    return alpha_c;
}


/*
 * dielectric_losses() - compute microstrip dielectric losses per unit
 * length
 */
double MICROSTRIP::dielectric_losses()
{
    double e_r, e_r_eff_0;
    double alpha_d;

    e_r       = m_parameters[EPSILONR_PRM];
    e_r_eff_0 = er_eff_0;

    alpha_d = ( 20.0 * M_PI / log( 10.0 ) ) * ( m_parameters[FREQUENCY_PRM] / C0 )
              * ( e_r / sqrt( e_r_eff_0 ) ) * ( ( e_r_eff_0 - 1.0 ) / ( e_r - 1.0 ) )
              * m_parameters[TAND_PRM];

    return alpha_d;
}


/*
 * attenuation() - compute attenuation of microstrip
 */
void MICROSTRIP::attenuation()
{
    m_parameters[SKIN_DEPTH_PRM] = skin_depth();

    atten_cond       = conductor_losses() * m_parameters[PHYS_LEN_PRM];
    atten_dielectric = dielectric_losses() * m_parameters[PHYS_LEN_PRM];
}


/*
 * mur_eff_ms() - returns effective magnetic permeability
 */
void MICROSTRIP::mur_eff_ms()
{
    double* mur = &m_parameters[MUR_PRM];
    double* h   = &m_parameters[H_PRM];
    double* w   = &m_parameters[PHYS_WIDTH_PRM];
    mur_eff     = ( 2.0 * *mur )
              / ( ( 1.0 + *mur ) + ( ( 1.0 - *mur ) * pow( ( 1.0 + ( 10.0 * *h / *w ) ), -0.5 ) ) );
}


// synth_width - calculate width given Z0 and e_r
double MICROSTRIP::synth_width()
{
    double e_r, a, b;
    double w_h, width;

    e_r = m_parameters[EPSILONR_PRM];

    a = ( ( m_parameters[Z0_PRM] / ZF0 / 2 / M_PI ) * sqrt( ( e_r + 1 ) / 2. ) )
        + ( ( e_r - 1 ) / ( e_r + 1 ) * ( 0.23 + ( 0.11 / e_r ) ) );
    b = ZF0 / 2 * M_PI / ( m_parameters[Z0_PRM] * sqrt( e_r ) );

    if( a > 1.52 )
    {
        w_h = 8 * exp( a ) / ( exp( 2. * a ) - 2 );
    }
    else
    {
        w_h = ( 2. / M_PI )
              * ( b - 1. - log( ( 2 * b ) - 1. )
                      + ( ( e_r - 1 ) / ( 2 * e_r ) ) * ( log( b - 1. ) + 0.39 - 0.61 / e_r ) );
    }

    if( m_parameters[H_PRM] > 0.0 )
        width = w_h * m_parameters[H_PRM];
    else
        width = 0;

    return width;
}


/*
 * line_angle() - calculate microstrip length in radians
 */
void MICROSTRIP::line_angle()
{
    double e_r_eff;
    double v, lambda_g;

    e_r_eff = m_parameters[EPSILON_EFF_PRM];

    /* velocity */
    v = C0 / sqrt( e_r_eff * mur_eff );
    /* wavelength */
    lambda_g = v / m_parameters[FREQUENCY_PRM];
    /* electrical angles */
    m_parameters[ANG_L_PRM] = 2.0 * M_PI * m_parameters[PHYS_LEN_PRM] / lambda_g; /* in radians */
}


void MICROSTRIP::calcAnalyze()
{
    /* effective permeability */
    mur_eff_ms();
    /* static impedance */
    microstrip_Z0();
    /* calculate freq dependence of er and Z0 */
    dispersion();
    /* calculate electrical lengths */
    line_angle();
    /* calculate losses */
    attenuation();
}


void MICROSTRIP::show_results()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    setResult( 0, m_parameters[EPSILON_EFF_PRM], "" );
    setResult( 1, unit_prop_delay, "ps/cm" );
    setResult( 2, atten_cond, "dB" );
    setResult( 3, atten_dielectric, "dB" );

    setResult( 4, m_parameters[SKIN_DEPTH_PRM] / UNIT_MICRON, "µm" );
}


void MICROSTRIP::showSynthesize()
{
    setProperty( PHYS_WIDTH_PRM, m_parameters[PHYS_WIDTH_PRM] );
    setProperty( PHYS_LEN_PRM, m_parameters[PHYS_LEN_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || ( m_parameters[PHYS_LEN_PRM] < 0 ) )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || ( m_parameters[PHYS_WIDTH_PRM] <= 0 ) )
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_ERROR );

    // Check for warnings
    if( !std::isfinite( m_parameters[Z0_PRM] ) || ( m_parameters[Z0_PRM] < 0 ) )
        setErrorLevel( Z0_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || ( m_parameters[ANG_L_PRM] < 0 ) )
        setErrorLevel( ANG_L_PRM, TRANSLINE_WARNING );
}

void MICROSTRIP::showAnalyze()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[Z0_PRM] ) || ( m_parameters[Z0_PRM] < 0 ) )
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || ( m_parameters[ANG_L_PRM] < 0 ) )
        setErrorLevel( ANG_L_PRM, TRANSLINE_ERROR );

    // Check for warnings
    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || ( m_parameters[PHYS_LEN_PRM] < 0 ) )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || ( m_parameters[PHYS_WIDTH_PRM] <= 0 ) )
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_WARNING );
}

/*
 * synthesis function
 */
void MICROSTRIP::calcSynthesize()
{
    double z0_dest   = m_parameters[Z0_PRM];
    double angl_dest = m_parameters[ANG_L_PRM];
    /* calculate width and use for initial value in Newton's method */
    m_parameters[PHYS_WIDTH_PRM] = synth_width();
    minimizeZ0Error1D( &( m_parameters[PHYS_WIDTH_PRM] ) );

    m_parameters[Z0_PRM]       = z0_dest;
    m_parameters[ANG_L_PRM]    = angl_dest;
    double const er_eff = m_parameters[EPSILON_EFF_PRM];
    m_parameters[PHYS_LEN_PRM] = C0 / m_parameters[FREQUENCY_PRM] / sqrt( er_eff * mur_eff )
                                 * m_parameters[ANG_L_PRM] / 2.0 / M_PI; /* in m */
    calcAnalyze();

    m_parameters[Z0_PRM]       = z0_dest;
    m_parameters[ANG_L_PRM]    = angl_dest;
    m_parameters[PHYS_LEN_PRM] = C0 / m_parameters[FREQUENCY_PRM] / sqrt( er_eff * mur_eff )
                                 * m_parameters[ANG_L_PRM] / 2.0 / M_PI; /* in m */
}
