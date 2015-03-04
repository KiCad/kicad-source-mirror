/*
 * c_microstrip.cpp - coupled microstrip class implementation
 *
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

/* c_microstrip.c - Puts up window for coupled microstrips and
 * performs the associated calculations
 * Based on the original microstrip.c by Gopal Narayanan
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

#include <units.h>
#include <transline.h>
#include <microstrip.h>
#include <c_microstrip.h>

C_MICROSTRIP::C_MICROSTRIP() : TRANSLINE()
{
    m_name = "Coupled_MicroStrip";
    aux_ms = NULL;

    // Initialize these variables mainly to avoid warnings from a static analyzer
    er_eff = 0.0;               // dummy
    w_eff = 0.0;                // Effective width of line
    atten_dielectric_e = 0.0;   // even-mode dielectric losses (dB)
    atten_cond_e = 0.0;         // even-mode conductors losses (dB)
    atten_dielectric_o = 0.0;   // odd-mode conductors losses (dB)
    atten_cond_o = 0.0;         // odd-mode conductors losses (dB)
}


C_MICROSTRIP::~C_MICROSTRIP()
{
    if( aux_ms )
        delete aux_ms;
}


/*
 * delta_u_thickness_single() computes the thickness effect on
 * normalized width for a single microstrip line
 *
 * References: H. A. Atwater, "Simplified Design Equations for
 * Microstrip Line Parameters", Microwave Journal, pp. 109-115,
 * November 1989.
 */
double C_MICROSTRIP::delta_u_thickness_single( double u, double t_h )
{
    double delta_u;

    if( t_h > 0.0 )
    {
        delta_u =
            (1.25 * t_h /
             M_PI) *
            ( 1.0 +
             log( ( 2.0 +
                   (4.0 * M_PI * u -
                    2.0) / ( 1.0 + exp( -100.0 * ( u - 1.0 / (2.0 * M_PI) ) ) ) ) / t_h ) );
    }
    else
    {
        delta_u = 0.0;
    }
    return delta_u;
}


/*
 * delta_u_thickness() - compute the thickness effect on normalized
 * width for coupled microstrips
 *
 * References: Rolf Jansen, "High-Speed Computation of Single and
 * Coupled Microstrip Parameters Including Dispersion, High-Order
 * Modes, Loss and Finite Strip Thickness", IEEE Trans. MTT, vol. 26,
 * no. 2, pp. 75-82, Feb. 1978
 */
void C_MICROSTRIP::delta_u_thickness()
{
    double e_r, u, g, t_h;
    double delta_u, delta_t, delta_u_e, delta_u_o;

    e_r = er;
    u   = w / h;        /* normalized line width */
    g   = s / h;        /* normalized line spacing */
    t_h = t / h;        /* normalized strip thickness */

    if( t_h > 0.0 )
    {
        /* single microstrip correction for finite strip thickness */
        delta_u = delta_u_thickness_single( u, t_h );
        delta_t = t_h / (g * e_r);
        /* thickness correction for the even- and odd-mode */
        delta_u_e = delta_u * ( 1.0 - 0.5 * exp( -0.69 * delta_u / delta_t ) );
        delta_u_o = delta_u_e + delta_t;
    }
    else
    {
        delta_u_e = delta_u_o = 0.0;
    }

    w_t_e = w + delta_u_e * h;
    w_t_o = w + delta_u_o * h;
}


/*
 * compute various parameters for a single line
 */
void C_MICROSTRIP::compute_single_line()
{
    if( aux_ms == NULL )
        aux_ms = new MICROSTRIP();

    /* prepare parameters for single microstrip computations */
    aux_ms->er = er;
    aux_ms->w  = w;
    aux_ms->h  = h;
    aux_ms->t  = 0.0;

    //aux_ms->t = t;
    aux_ms->ht   = 1e12;    /* arbitrarily high */
    aux_ms->f    = f;
    aux_ms->murC = murC;
    aux_ms->microstrip_Z0();
    aux_ms->dispersion();
}


/*
 * filling_factor_even() - compute the filling factor for the coupled
 * microstrips even-mode without cover and zero conductor thickness
 */
double C_MICROSTRIP::filling_factor_even( double u, double g, double e_r )
{
    double v, v3, v4, a_e, b_e, q_inf;

    v   = u * (20.0 + g * g) / (10.0 + g * g) + g* exp( -g );
    v3  = v * v * v;
    v4  = v3 * v;
    a_e = 1.0 + log( (v4 + v * v / 2704.0) / (v4 + 0.432) ) / 49.0 + log( 1.0 + v3 / 5929.741 )
          / 18.7;
    b_e = 0.564 * pow( ( (e_r - 0.9) / (e_r + 3.0) ), 0.053 );

    /* filling factor, with width corrected for thickness */
    q_inf = pow( (1.0 + 10.0 / v), -a_e * b_e );

    return q_inf;
}


/**
 * filling_factor_odd() - compute the filling factor for the coupled
 * microstrips odd-mode without cover and zero conductor thickness
 */
double C_MICROSTRIP::filling_factor_odd( double u, double g, double e_r )
{
    double b_o, c_o, d_o, q_inf;

    b_o = 0.747 * e_r / (0.15 + e_r);
    c_o = b_o - (b_o - 0.207) * exp( -0.414 * u );
    d_o = 0.593 + 0.694 * exp( -0.562 * u );

    /* filling factor, with width corrected for thickness */
    q_inf = exp( -c_o * pow( g, d_o ) );

    return q_inf;
}


/*
 * delta_q_cover_even() - compute the cover effect on filling factor
 * for the even-mode
 */
double C_MICROSTRIP::delta_q_cover_even( double h2h )
{
    double q_c;

    if( h2h <= 39 )
    {
        q_c = tanh( 1.626 + 0.107 * h2h - 1.733 / sqrt( h2h ) );
    }
    else
    {
        q_c = 1.0;
    }

    return q_c;
}


/*
 * delta_q_cover_odd() - compute the cover effect on filling factor
 * for the odd-mode
 */
double C_MICROSTRIP::delta_q_cover_odd( double h2h )
{
    double q_c;

    if( h2h <= 7 )
    {
        q_c = tanh( 9.575 / (7.0 - h2h) - 2.965 + 1.68 * h2h - 0.311 * h2h * h2h );
    }
    else
    {
        q_c = 1.0;
    }

    return q_c;
}


/**
 * er_eff_static() - compute the static effective dielectric constants
 *
 * References: Manfred Kirschning and Rolf Jansen, "Accurate
 * Wide-Range Design Equations for the Frequency-Dependent
 * Characteristic of Parallel Coupled Microstrip Lines", IEEE
 * Trans. MTT, vol. 32, no. 1, Jan. 1984
 */
void C_MICROSTRIP::er_eff_static()
{
    double u_t_e, u_t_o, g, h2, h2h;
    double a_o, t_h, q, q_c, q_t, q_inf;
    double er_eff_single;

    /* compute zero-thickness single line parameters */
    compute_single_line();
    er_eff_single = aux_ms->er_eff_0;

    h2    = ht;
    u_t_e = w_t_e / h;      /* normalized even_mode line width */
    u_t_o = w_t_o / h;      /* normalized odd_mode line width */
    g     = s / h;          /* normalized line spacing */
    h2h   = h2 / h;         /* normalized cover height */
    t_h   = t / h;          /* normalized strip thickness */

    /* filling factor, computed with thickness corrected width */
    q_inf = filling_factor_even( u_t_e, g, er );
    /* cover effect */
    q_c = delta_q_cover_even( h2h );
    /* thickness effect */
    q_t = aux_ms->delta_q_thickness( u_t_e, t_h );
    /* resultant filling factor */
    q = (q_inf - q_t) * q_c;
    /* static even-mode effective dielectric constant */
    er_eff_e_0 = 0.5 * (er + 1.0) + 0.5 * (er - 1.0) * q;

    /* filling factor, with width corrected for thickness */
    q_inf = filling_factor_odd( u_t_o, g, er );
    /* cover effect */
    q_c = delta_q_cover_odd( h2h );
    /* thickness effect */
    q_t = aux_ms->delta_q_thickness( u_t_o, t_h );
    /* resultant filling factor */
    q = (q_inf - q_t) * q_c;

    a_o = 0.7287 * ( er_eff_single - 0.5 * (er + 1.0) ) * ( 1.0 - exp( -0.179 * u_t_o ) );

    /* static odd-mode effective dielectric constant */
    er_eff_o_0 = (0.5 * (er + 1.0) + a_o - er_eff_single) * q + er_eff_single;
}


/**
 * delta_Z0_even_cover() - compute the even-mode impedance correction
 * for a homogeneous microstrip due to the cover
 *
 * References: S. March, "Microstrip Packaging: Watch the Last Step",
 * Microwaves, vol. 20, no. 13, pp. 83.94, Dec. 1981.
 */
double C_MICROSTRIP::delta_Z0_even_cover( double g, double u, double h2h )
{
    double f_e, g_e, delta_Z0_even;
    double x, y, A, B, C, D, E, F;

    A   = -4.351 / pow( 1.0 + h2h, 1.842 );
    B   = 6.639 / pow( 1.0 + h2h, 1.861 );
    C   = -2.291 / pow( 1.0 + h2h, 1.90 );
    f_e = 1.0 - atanh( A + (B + C * u) * u );

    x   = pow( 10.0, 0.103 * g - 0.159 );
    y   = pow( 10.0, 0.0492 * g - 0.073 );
    D   = 0.747 / sin( 0.5 * M_PI * x );
    E   = 0.725 * sin( 0.5 * M_PI * y );
    F   = pow( 10.0, 0.11 - 0.0947 * g );
    g_e = 270.0 * ( 1.0 - tanh( D + E * sqrt( 1.0 + h2h ) - F / (1.0 + h2h) ) );

    delta_Z0_even = f_e * g_e;

    return delta_Z0_even;
}


/**
 * delta_Z0_odd_cover() - compute the odd-mode impedance correction
 * for a homogeneous microstrip due to the cover
 *
 * References: S. March, "Microstrip Packaging: Watch the Last Step",
 * Microwaves, vol. 20, no. 13, pp. 83.94, Dec. 1981.
 */
double C_MICROSTRIP::delta_Z0_odd_cover( double g, double u, double h2h )
{
    double f_o, g_o, delta_Z0_odd;
    double G, J, K, L;

    J   = tanh( pow( 1.0 + h2h, 1.585 ) / 6.0 );
    f_o = pow( u, J );

    G = 2.178 - 0.796 * g;
    if( g > 0.858 )
    {
        K = log10( 20.492 * pow( g, 0.174 ) );
    }
    else
    {
        K = 1.30;
    }
    if( g > 0.873 )
    {
        L = 2.51 * pow( g, -0.462 );
    }
    else
    {
        L = 2.674;
    }
    g_o = 270.0 * ( 1.0 - tanh( G + K * sqrt( 1.0 + h2h ) - L / (1.0 + h2h) ) );

    delta_Z0_odd = f_o * g_o;

    return delta_Z0_odd;
}


/**
 * Z0_even_odd() - compute the static even- and odd-mode static
 * impedances
 *
 * References: Manfred Kirschning and Rolf Jansen, "Accurate
 * Wide-Range Design Equations for the Frequency-Dependent
 * Characteristic of Parallel Coupled Microstrip Lines", IEEE
 * Trans. MTT, vol. 32, no. 1, Jan. 1984
 */
void C_MICROSTRIP::Z0_even_odd()
{
    double er_eff, h2, u_t_e, u_t_o, g, h2h;
    double Q_1, Q_2, Q_3, Q_4, Q_5, Q_6, Q_7, Q_8, Q_9, Q_10;
    double delta_Z0_e_0, delta_Z0_o_0, Z0_single, er_eff_single;

    h2    = ht;
    u_t_e = w_t_e / h;      /* normalized even-mode line width */
    u_t_o = w_t_o / h;      /* normalized odd-mode line width */
    g     = s / h;          /* normalized line spacing */
    h2h   = h2 / h;         /* normalized cover height */

    Z0_single     = aux_ms->Z0_0;
    er_eff_single = aux_ms->er_eff_0;

    /* even-mode */
    er_eff = er_eff_e_0;
    Q_1    = 0.8695 * pow( u_t_e, 0.194 );
    Q_2    = 1.0 + 0.7519 * g + 0.189 * pow( g, 2.31 );
    Q_3    = 0.1975 +
             pow( ( 16.6 +
                   pow( (8.4 / g),
                       6.0 ) ),
                 -0.387 ) + log( pow( g, 10.0 ) / ( 1.0 + pow( g / 3.4, 10.0 ) ) ) / 241.0;
    Q_4 = 2.0 * Q_1 /
          ( Q_2 * ( exp( -g ) * pow( u_t_e, Q_3 ) + ( 2.0 - exp( -g ) ) * pow( u_t_e, -Q_3 ) ) );
    /* static even-mode impedance */
    Z0_e_0 = Z0_single *
             sqrt( er_eff_single / er_eff ) / (1.0 - sqrt( er_eff_single ) * Q_4 * Z0_single / ZF0);
    /* correction for cover */
    delta_Z0_e_0 = delta_Z0_even_cover( g, u_t_e, h2h ) / sqrt( er_eff );

    Z0_e_0 = Z0_e_0 - delta_Z0_e_0;

    /* odd-mode */
    er_eff = er_eff_o_0;
    Q_5    = 1.794 + 1.14 * log( 1.0 + 0.638 / ( g + 0.517 * pow( g, 2.43 ) ) );
    Q_6    = 0.2305 + log( pow( g, 10.0 ) / ( 1.0 + pow( g / 5.8, 10.0 ) ) ) / 281.3 + log(
         1.0 + 0.598 * pow( g, 1.154 ) ) / 5.1;
    Q_7    = (10.0 + 190.0 * g * g) / (1.0 + 82.3 * g * g * g);
    Q_8    = exp( -6.5 - 0.95 * log( g ) - pow( g / 0.15, 5.0 ) );
    Q_9    = log( Q_7 ) * (Q_8 + 1.0 / 16.5);
    Q_10   = ( Q_2 * Q_4 - Q_5 * exp( log( u_t_o ) * Q_6 * pow( u_t_o, -Q_9 ) ) ) / Q_2;

    /* static odd-mode impedance */
    Z0_o_0 = Z0_single *
             sqrt( er_eff_single /
                   er_eff ) / (1.0 - sqrt( er_eff_single ) * Q_10 * Z0_single / ZF0);
    /* correction for cover */
    delta_Z0_o_0 = delta_Z0_odd_cover( g, u_t_o, h2h ) / sqrt( er_eff );

    Z0_o_0 = Z0_o_0 - delta_Z0_o_0;
}


/*
 * er_eff_freq() - compute er_eff as a function of frequency
 */
void C_MICROSTRIP::er_eff_freq()
{
    double P_1, P_2, P_3, P_4, P_5, P_6, P_7;
    double P_8, P_9, P_10, P_11, P_12, P_13, P_14, P_15;
    double F_e, F_o;
    double er_eff, u, g, f_n;

    u = w / h;          /* normalize line width */
    g = s / h;          /* normalize line spacing */

    /* normalized frequency [GHz * mm] */
    f_n = f * h / 1e06;

    er_eff = er_eff_e_0;
    P_1    = 0.27488 + ( 0.6315 + 0.525 / pow( 1.0 + 0.0157 * f_n, 20.0 ) ) * u - 0.065683 * exp(
        -8.7513 * u );
    P_2    = 0.33622 * ( 1.0 - exp( -0.03442 * er ) );
    P_3    = 0.0363 * exp( -4.6 * u ) * ( 1.0 - exp( -pow( f_n / 38.7, 4.97 ) ) );
    P_4    = 1.0 + 2.751 * ( 1.0 - exp( -pow( er / 15.916, 8.0 ) ) );
    P_5    = 0.334 * exp( -3.3 * pow( er / 15.0, 3.0 ) ) + 0.746;
    P_6    = P_5 * exp( -pow( f_n / 18.0, 0.368 ) );
    P_7    = 1.0 +
             4.069* P_6* pow( g, 0.479 ) * exp( -1.347 * pow( g, 0.595 ) - 0.17 * pow( g, 2.5 ) );

    F_e = P_1 * P_2 * pow( (P_3 * P_4 + 0.1844 * P_7) * f_n, 1.5763 );
    /* even-mode effective dielectric constant */
    er_eff_e = er - (er - er_eff) / (1.0 + F_e);

    er_eff = er_eff_o_0;
    P_8    = 0.7168 * ( 1.0 + 1.076 / ( 1.0 + 0.0576 * (er - 1.0) ) );
    P_9    = P_8 - 0.7913 *
             ( 1.0 - exp( -pow( f_n / 20.0, 1.424 ) ) ) * atan( 2.481 * pow( er / 8.0, 0.946 ) );
    P_10 = 0.242 * pow( er - 1.0, 0.55 );
    P_11 = 0.6366 * (exp( -0.3401 * f_n ) - 1.0) * atan( 1.263 * pow( u / 3.0, 1.629 ) );
    P_12 = P_9 + (1.0 - P_9) / ( 1.0 + 1.183 * pow( u, 1.376 ) );
    P_13 = 1.695 * P_10 / (0.414 + 1.605 * P_10);
    P_14 = 0.8928 + 0.1072 * ( 1.0 - exp( -0.42 * pow( f_n / 20.0, 3.215 ) ) );
    P_15 = fabs( 1.0 - 0.8928 * (1.0 + P_11) * P_12 * exp( -P_13 * pow( g, 1.092 ) ) / P_14 );

    F_o = P_1 * P_2 * pow( (P_3 * P_4 + 0.1844) * f_n * P_15, 1.5763 );
    /* odd-mode effective dielectric constant */
    er_eff_o = er - (er - er_eff) / (1.0 + F_o);
}


/*
 * conductor_losses() - compute microstrips conductor losses per unit
 * length
 */
void C_MICROSTRIP::conductor_losses()
{
    double e_r_eff_e_0, e_r_eff_o_0, Z0_h_e, Z0_h_o, delta;
    double K, R_s, Q_c_e, Q_c_o, alpha_c_e, alpha_c_o;

    e_r_eff_e_0 = er_eff_e_0;
    e_r_eff_o_0 = er_eff_o_0;
    Z0_h_e = Z0_e_0 * sqrt( e_r_eff_e_0 );  /* homogeneous stripline impedance */
    Z0_h_o = Z0_o_0 * sqrt( e_r_eff_o_0 );  /* homogeneous stripline impedance */
    delta  = skindepth;

    if( f > 0.0 )
    {
        /* current distribution factor (same for the two modes) */
        K = exp( -1.2 * pow( (Z0_h_e + Z0_h_o) / (2.0 * ZF0), 0.7 ) );
        /* skin resistance */
        R_s = 1.0 / (sigma * delta);
        /* correction for surface roughness */
        R_s *= 1.0 + ( (2.0 / M_PI) * atan( 1.40 * pow( (rough / delta), 2.0 ) ) );

        /* even-mode strip inductive quality factor */
        Q_c_e = (M_PI * Z0_h_e * w * f) / (R_s * C0 * K);
        /* even-mode losses per unith length */
        alpha_c_e = ( 20.0 * M_PI / log( 10.0 ) ) * f * sqrt( e_r_eff_e_0 ) / (C0 * Q_c_e);

        /* odd-mode strip inductive quality factor */
        Q_c_o = (M_PI * Z0_h_o * w * f) / (R_s * C0 * K);
        /* odd-mode losses per unith length */
        alpha_c_o = ( 20.0 * M_PI / log( 10.0 ) ) * f * sqrt( e_r_eff_o_0 ) / (C0 * Q_c_o);
    }
    else
    {
        alpha_c_e = alpha_c_o = 0.0;
    }

    atten_cond_e = alpha_c_e * l;
    atten_cond_o = alpha_c_o * l;
}


/*
 * dielectric_losses() - compute microstrips dielectric losses per
 * unit length
 */
void C_MICROSTRIP::dielectric_losses()
{
    double e_r, e_r_eff_e_0, e_r_eff_o_0;
    double alpha_d_e, alpha_d_o;

    e_r = er;
    e_r_eff_e_0 = er_eff_e_0;
    e_r_eff_o_0 = er_eff_o_0;

    alpha_d_e =
        ( 20.0 * M_PI /
         log( 10.0 ) ) *
        (f / C0) * ( e_r / sqrt( e_r_eff_e_0 ) ) * ( (e_r_eff_e_0 - 1.0) / (e_r - 1.0) ) * tand;
    alpha_d_o =
        ( 20.0 * M_PI /
         log( 10.0 ) ) *
        (f / C0) * ( e_r / sqrt( e_r_eff_o_0 ) ) * ( (e_r_eff_o_0 - 1.0) / (e_r - 1.0) ) * tand;

    atten_dielectric_e = alpha_d_e * l;
    atten_dielectric_o = alpha_d_o * l;
}


/*
 * c_microstrip_attenuation() - compute attenuation of coupled
 * microstrips
 */
void C_MICROSTRIP::attenuation()
{
    skindepth = skin_depth();
    conductor_losses();
    dielectric_losses();
}


/*
 * line_angle() - calculate strips electrical lengths in radians
 */
void C_MICROSTRIP::line_angle()
{
    double e_r_eff_e, e_r_eff_o;
    double v_e, v_o, lambda_g_e, lambda_g_o;

    e_r_eff_e = er_eff_e;
    e_r_eff_o = er_eff_o;

    /* even-mode velocity */
    v_e = C0 / sqrt( e_r_eff_e );
    /* odd-mode velocity */
    v_o = C0 / sqrt( e_r_eff_o );
    /* even-mode wavelength */
    lambda_g_e = v_e / f;
    /* odd-mode wavelength */
    lambda_g_o = v_o / f;
    /* electrical angles */
    ang_l_e = 2.0 * M_PI * l / lambda_g_e;  /* in radians */
    ang_l_o = 2.0 * M_PI * l / lambda_g_o;  /* in radians */
}


void C_MICROSTRIP::syn_err_fun( double* f1,
                                double* f2,
                                double  s_h,
                                double  w_h,
                                double  e_r,
                                double  w_h_se,
                                double  w_h_so )
{
    double g, h;

    g = cosh( 0.5 * M_PI * s_h );
    h = cosh( M_PI * w_h + 0.5 * M_PI * s_h );

    *f1 = (2.0 / M_PI) * acosh( (2.0 * h - g + 1.0) / (g + 1.0) );
    *f2 = (2.0 / M_PI) * acosh( (2.0 * h - g - 1.0) / (g - 1.0) );
    if( e_r <= 6.0 )
    {
        *f2 += ( 4.0 / ( M_PI * (1.0 + e_r / 2.0) ) ) * acosh( 1.0 + 2.0 * w_h / s_h );
    }
    else
    {
        *f2 += (1.0 / M_PI) * acosh( 1.0 + 2.0 * w_h / s_h );
    }
    *f1 -= w_h_se;
    *f2 -= w_h_so;
}


/*
 * synth_width - calculate widths given Z0 and e_r
 * from Akhtarzad S. et al., "The design of coupled microstrip lines",
 * IEEE Trans. MTT-23, June 1975 and
 * Hinton, J.H., "On design of coupled microstrip lines", IEEE Trans.
 * MTT-28, March 1980
 */
void C_MICROSTRIP::synth_width()
{
    double Z0, e_r;
    double w_h_se, w_h_so, w_h, a, ce, co, s_h;
    double f1, f2, ft1, ft2, j11, j12, j21, j22, d_s_h, d_w_h, err;
    double eps = 1e-04;

    f1  = f2 = 0;
    e_r = er;

    Z0 = Z0e / 2.0;
    /* Wheeler formula for single microstrip synthesis */
    a = exp( Z0 * sqrt( e_r + 1.0 ) / 42.4 ) - 1.0;
    w_h_se = 8.0 * sqrt( a * ( (7.0 + 4.0 / e_r) / 11.0 ) + ( (1.0 + 1.0 / e_r) / 0.81 ) ) / a;

    Z0 = Z0o / 2.0;
    /* Wheeler formula for single microstrip synthesis */
    a = exp( Z0 * sqrt( e_r + 1.0 ) / 42.4 ) - 1.0;
    w_h_so = 8.0 * sqrt( a * ( (7.0 + 4.0 / e_r) / 11.0 ) + ( (1.0 + 1.0 / e_r) / 0.81 ) ) / a;

    ce = cosh( 0.5 * M_PI * w_h_se );
    co = cosh( 0.5 * M_PI * w_h_so );
    /* first guess at s/h */
    s_h = (2.0 / M_PI) * acosh( (ce + co - 2.0) / (co - ce) );
    /* first guess at w/h */
    w_h = acosh( (ce * co - 1.0) / (co - ce) ) / M_PI - s_h / 2.0;

    s = s_h * h;
    w = w_h * h;

    syn_err_fun( &f1, &f2, s_h, w_h, e_r, w_h_se, w_h_so );

    /* rather crude Newton-Rhapson; we need this beacuse the estimate of */
    /* w_h is often quite far from the true value (see Akhtarzad S. et al.) */
    do {
        /* compute Jacobian */
        syn_err_fun( &ft1, &ft2, s_h + eps, w_h, e_r, w_h_se, w_h_so );
        j11 = (ft1 - f1) / eps;
        j21 = (ft2 - f2) / eps;
        syn_err_fun( &ft1, &ft2, s_h, w_h + eps, e_r, w_h_se, w_h_so );
        j12 = (ft1 - f1) / eps;
        j22 = (ft2 - f2) / eps;

        /* compute next step */
        d_s_h = (-f1 * j22 + f2 * j12) / (j11 * j22 - j21 * j12);
        d_w_h = (-f2 * j11 + f1 * j21) / (j11 * j22 - j21 * j12);

        //g_print("j11 = %e\tj12 = %e\tj21 = %e\tj22 = %e\n", j11, j12, j21, j22);
        //g_print("det = %e\n", j11*j22 - j21*j22);
        //g_print("d_s_h = %e\td_w_h = %e\n", d_s_h, d_w_h);

        s_h += d_s_h;
        w_h += d_w_h;

        /* chech the error */
        syn_err_fun( &f1, &f2, s_h, w_h, e_r, w_h_se, w_h_so );

        err = sqrt( f1 * f1 + f2 * f2 );
        /* converged ? */
    } while( err > 1e-04 );


    s = s_h * h;
    w = w_h * h;
}


/*
 * Z0_dispersion() - calculate frequency dependency of characteristic
 * impedances
 */
void C_MICROSTRIP::Z0_dispersion()
{
    double Q_0;
    double Q_11, Q_12, Q_13, Q_14, Q_15, Q_16, Q_17, Q_18, Q_19, Q_20, Q_21;
    double Q_22, Q_23, Q_24, Q_25, Q_26, Q_27, Q_28, Q_29;
    double r_e, q_e, p_e, d_e, C_e;
    double e_r_eff_o_f, e_r_eff_o_0;
    double e_r_eff_single_f, e_r_eff_single_0, Z0_single_f;
    double f_n, g, u, e_r;
    double R_1, R_2, R_7, R_10, R_11, R_12, R_15, R_16, tmpf;

    e_r = er;

    u = w / h;          /* normalize line width */
    g = s / h;          /* normalize line spacing */

    /* normalized frequency [GHz * mm] */
    f_n = f * h / 1e06;

    e_r_eff_single_f = aux_ms->er_eff;
    e_r_eff_single_0 = aux_ms->er_eff_0;
    Z0_single_f = aux_ms->Z0;

    e_r_eff_o_f = er_eff_o;
    e_r_eff_o_0 = er_eff_o_0;

    Q_11 = 0.893 * ( 1.0 - 0.3 / ( 1.0 + 0.7 * (e_r - 1.0) ) );
    Q_12 = 2.121 * ( pow( f_n / 20.0, 4.91 ) / ( 1.0 + Q_11 * pow( f_n / 20.0, 4.91 ) ) ) * exp(
        -2.87 * g ) * pow( g, 0.902 );
    Q_13 = 1.0 + 0.038 * pow( e_r / 8.0, 5.1 );
    Q_14 = 1.0 + 1.203 * pow( e_r / 15.0, 4.0 ) / ( 1.0 + pow( e_r / 15.0, 4.0 ) );
    Q_15 = 1.887 *
           exp( -1.5 *
               pow( g,
                    0.84 ) ) *
           pow( g,
                Q_14 ) /
           ( 1.0 + 0.41 *
            pow( f_n / 15.0, 3.0 ) * pow( u, 2.0 / Q_13 ) / ( 0.125 + pow( u, 1.626 / Q_13 ) ) );
    Q_16 = ( 1.0 + 9.0 / ( 1.0 + 0.403 * pow( e_r - 1.0, 2 ) ) ) * Q_15;
    Q_17 = 0.394 *
           ( 1.0 -
            exp( -1.47 * pow( u / 7.0, 0.672 ) ) ) * ( 1.0 - exp( -4.25 * pow( f_n / 20.0, 1.87 ) ) );
    Q_18 = 0.61 * ( 1.0 - exp( -2.13 * pow( u / 8.0, 1.593 ) ) ) / ( 1.0 + 6.544 * pow( g, 4.17 ) );
    Q_19 = 0.21 * g * g * g * g /
           ( ( 1.0 + 0.18 * pow( g, 4.9 ) ) * (1.0 + 0.1 * u * u) * ( 1.0 + pow( f_n / 24.0, 3.0 ) ) );
    Q_20 = ( 0.09 + 1.0 / ( 1.0 + 0.1 * pow( e_r - 1, 2.7 ) ) ) * Q_19;
    Q_21 =
        fabs( 1.0 - 42.54 *
             pow( g, 0.133 ) * exp( -0.812 * g ) * pow( u, 2.5 ) / ( 1.0 + 0.033 * pow( u, 2.5 ) ) );

    r_e = pow( f_n / 28.843, 12 );
    q_e = 0.016 + pow( 0.0514 * e_r * Q_21, 4.524 );
    p_e = 4.766 * exp( -3.228 * pow( u, 0.641 ) );
    d_e = 5.086 * q_e *
          ( r_e /
           (0.3838 + 0.386 *
            q_e) ) *
          ( exp( -22.2 *
                pow( u,
                     1.92 ) ) /
           (1.0 + 1.2992 * r_e) ) * ( pow( e_r - 1.0, 6.0 ) / ( 1.0 + 10 * pow( e_r - 1.0, 6.0 ) ) );
    C_e = 1.0 + 1.275 *
          ( 1.0 -
           exp( -0.004625 * p_e *
               pow( e_r,
                    1.674 ) * pow( f_n / 18.365, 2.745 ) ) ) - Q_12 + Q_16 - Q_17 + Q_18 + Q_20;


    R_1  = 0.03891 * pow( e_r, 1.4 );
    R_2  = 0.267 * pow( u, 7.0 );
    R_7  = 1.206 - 0.3144 * exp( -R_1 ) * ( 1.0 - exp( -R_2 ) );
    R_10 = 0.00044 * pow( e_r, 2.136 ) + 0.0184;
    tmpf = pow( f_n / 19.47, 6.0 );
    R_11 = tmpf / (1.0 + 0.0962 * tmpf);
    R_12 = 1.0 / (1.0 + 0.00245 * u * u);
    R_15 = 0.707* R_10* pow( f_n / 12.3, 1.097 );
    R_16 = 1.0 + 0.0503 * e_r * e_r * R_11 * ( 1.0 - exp( -pow( u / 15.0, 6.0 ) ) );
    Q_0  = R_7 * ( 1.0 - 1.1241 * (R_12 / R_16) * exp( -0.026 * pow( f_n, 1.15656 ) - R_15 ) );

    /* even-mode frequency-dependent characteristic impedances */
    Z0e = Z0_e_0 * pow( 0.9408 * pow( e_r_eff_single_f, C_e ) - 0.9603, Q_0 ) / pow(
         (0.9408 - d_e) * pow( e_r_eff_single_0, C_e ) - 0.9603, Q_0 );

    Q_29 = 15.16 / ( 1.0 + 0.196 * pow( e_r - 1.0, 2.0 ) );
    tmpf = pow( e_r - 1.0, 3.0 );
    Q_28 = 0.149 * tmpf / (94.5 + 0.038 * tmpf);
    tmpf = pow( e_r - 1.0, 1.5 );
    Q_27 = 0.4 * pow( g, 0.84 ) * ( 1.0 + 2.5 * tmpf / (5.0 + tmpf) );
    tmpf = pow( (e_r - 1.0) / 13.0, 12.0 );
    Q_26 = 30.0 - 22.2 * ( tmpf / (1.0 + 3.0 * tmpf) ) - Q_29;
    tmpf = (e_r - 1.0) * (e_r - 1.0);
    Q_25 = ( 0.3 * f_n * f_n / (10.0 + f_n * f_n) ) * ( 1.0 + 2.333 * tmpf / (5.0 + tmpf) );
    Q_24 =
        2.506* Q_28* pow( u,
                          0.894 ) *
        pow( (1.0 + 1.3 * u) * f_n / 99.25, 4.29 ) / ( 3.575 + pow( u, 0.894 ) );
    Q_23 = 1.0 + 0.005 * f_n * Q_27 /
           ( ( 1.0 + 0.812 * pow( f_n / 15.0, 1.9 ) ) * (1.0 + 0.025 * u * u) );
    Q_22 = 0.925 * pow( f_n / Q_26, 1.536 ) / ( 1.0 + 0.3 * pow( f_n / 30.0, 1.536 ) );

    /* odd-mode frequency-dependent characteristic impedances */
    Z0o = Z0_single_f +
          (Z0_o_0 *
           pow( e_r_eff_o_f / e_r_eff_o_0,
                Q_22 ) - Z0_single_f * Q_23) / (1.0 + Q_24 + pow( 0.46 * g, 2.2 ) * Q_25);
}


void C_MICROSTRIP::calc()
{
    /* compute thickness corrections */
    delta_u_thickness();
    /* get effective dielectric constants */
    er_eff_static();
    /* impedances for even- and odd-mode */
    Z0_even_odd();
    /* calculate freq dependence of er_eff_e, er_eff_o */
    er_eff_freq();
    /* calculate frequency  dependence of Z0e, Z0o */
    Z0_dispersion();
    /* calculate losses */
    attenuation();
    /* calculate electrical lengths */
    line_angle();
}


/*
 * get_microstrip_sub
 * get and assign microstrip substrate parameters
 * into microstrip structure
 */
void C_MICROSTRIP::get_c_microstrip_sub()
{
    er    = getProperty( EPSILONR_PRM );
    murC  = getProperty( MURC_PRM );
    h     = getProperty( H_PRM );
    ht    = getProperty( H_T_PRM );
    t     = getProperty( T_PRM );
    sigma = 1.0/getProperty( RHO_PRM );
    tand  = getProperty( TAND_PRM );
    rough = getProperty( ROUGH_PRM );
}


/*
 * get_c_microstrip_comp
 * get and assign microstrip component parameters
 * into microstrip structure
 */
void C_MICROSTRIP::get_c_microstrip_comp()
{
    f = getProperty( FREQUENCY_PRM );
}


/*
 * get_c_microstrip_elec
 * get and assign microstrip electrical parameters
 * into microstrip structure
 */
void C_MICROSTRIP::get_c_microstrip_elec()
{
    Z0e     = getProperty( Z0_E_PRM );
    Z0o     = getProperty( Z0_O_PRM );
    ang_l_e = getProperty( ANG_L_PRM );
    ang_l_o = getProperty( ANG_L_PRM );
}


/*
 * get_c_microstrip_phys
 * get and assign microstrip physical parameters
 * into microstrip structure
 */
void C_MICROSTRIP::get_c_microstrip_phys()
{
    w = getProperty( PHYS_WIDTH_PRM );
    s = getProperty( PHYS_S_PRM );
    l = getProperty( PHYS_LEN_PRM );
}


void C_MICROSTRIP::show_results()
{
    setProperty( Z0_E_PRM, Z0e );
    setProperty( Z0_O_PRM , Z0o );
    setProperty( ANG_L_PRM, sqrt( ang_l_e * ang_l_o ) );

    setResult( 0, er_eff_e, "" );
    setResult( 1, er_eff_o, "" );
    setResult( 2, atten_cond_e, "dB" );
    setResult( 3, atten_cond_o, "dB" );
    setResult( 4, atten_dielectric_e, "dB" );
    setResult( 5, atten_dielectric_o, "dB" );

    setResult( 6, skindepth / UNIT_MICRON, "µm" );
}


/*
 * analysis function
 */
void C_MICROSTRIP::analyze()
{
    /* Get and assign substrate parameters */
    get_c_microstrip_sub();
    /* Get and assign component parameters */
    get_c_microstrip_comp();
    /* Get and assign physical parameters */
    get_c_microstrip_phys();

    /* compute coupled microstrip parameters */
    calc();
    /* print results in the subwindow */
    show_results();
}


void C_MICROSTRIP::syn_fun( double* f1,
                            double* f2,
                            double  s_h,
                            double  w_h,
                            double  Z0_e,
                            double  Z0_o )
{
    s = s_h * h;
    w = w_h * h;

    /* compute coupled microstrip parameters */
    calc();

    *f1 = Z0e - Z0_e;
    *f2 = Z0o - Z0_o;
}


/*
 * synthesis function
 */
void C_MICROSTRIP::synthesize()
{
    double Z0_e, Z0_o;
    double f1, f2, ft1, ft2, j11, j12, j21, j22, d_s_h, d_w_h, err;
    double eps = 1e-04;
    double w_h, s_h, le, lo;

    /* Get and assign substrate parameters */
    get_c_microstrip_sub();

    /* Get and assign component parameters */
    get_c_microstrip_comp();

    /* Get and assign electrical parameters */
    get_c_microstrip_elec();

    /* Get and assign physical parameters */
    /* at present it is required only for getting strips length */
    get_c_microstrip_phys();


    /* required value of Z0_e and Z0_o */
    Z0_e = Z0e;
    Z0_o = Z0o;

    /* calculate width and use for initial value in Newton's method */
    synth_width();
    w_h = w / h;
    s_h = s / h;
    f1  = f2 = 0;

    /* rather crude Newton-Rhapson */
    do {
        /* compute Jacobian */
        syn_fun( &ft1, &ft2, s_h + eps, w_h, Z0_e, Z0_o );
        j11 = (ft1 - f1) / eps;
        j21 = (ft2 - f2) / eps;
        syn_fun( &ft1, &ft2, s_h, w_h + eps, Z0_e, Z0_o );
        j12 = (ft1 - f1) / eps;
        j22 = (ft2 - f2) / eps;

        /* compute next step; increments of s_h and w_h */
        d_s_h = (-f1 * j22 + f2 * j12) / (j11 * j22 - j21 * j12);
        d_w_h = (-f2 * j11 + f1 * j21) / (j11 * j22 - j21 * j12);

        s_h += d_s_h;
        w_h += d_w_h;

        /* compute the error with the new values of s_h and w_h */
        syn_fun( &f1, &f2, s_h, w_h, Z0_e, Z0_o );
        err = sqrt( f1 * f1 + f2 * f2 );

        /* converged ? */
    } while( err > 1e-04 );

    /* denormalize computed width and spacing */
    s = s_h * h;
    w = w_h * h;

    setProperty( PHYS_WIDTH_PRM, w );
    setProperty( PHYS_S_PRM, s );

    /* calculate physical length */
    ang_l_e = getProperty( ANG_L_PRM );
    ang_l_o = getProperty( ANG_L_PRM );
    le = C0 / f / sqrt( er_eff_e ) * ang_l_e / 2.0 / M_PI;
    lo = C0 / f / sqrt( er_eff_o ) * ang_l_o / 2.0 / M_PI;
    l  = sqrt( le * lo );
    setProperty( PHYS_LEN_PRM, l );

    calc();
    /* print results in the subwindow */
    show_results();
}
