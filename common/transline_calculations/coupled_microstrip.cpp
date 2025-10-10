/*
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2018 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#include <transline_calculations/coupled_microstrip.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


void COUPLED_MICROSTRIP::Analyse()
{
    // Compute thickness corrections
    delta_u_thickness();

    // Get effective dielectric constants
    er_eff_static();

    // Impedances for even- and odd-mode
    Z0_even_odd();

    // Calculate freq dependence of er_eff_e, er_eff_o
    er_eff_freq();

    // Calculate frequency  dependence of Z0e, Z0o */
    Z0_dispersion();

    // Calculate losses
    attenuation();

    // Calculate electrical lengths
    line_angle();

    // Calculate diff impedance
    diff_impedance();
}


bool COUPLED_MICROSTRIP::Synthesize( const SYNTHESIZE_OPTS aOpts )
{
    if( aOpts == SYNTHESIZE_OPTS::FIX_WIDTH )
        return MinimiseZ0Error1D( TCP::PHYS_S, TCP::Z0_O, false );

    if( aOpts == SYNTHESIZE_OPTS::FIX_SPACING )
        return MinimiseZ0Error1D( TCP::PHYS_WIDTH, TCP::Z0_O, false );

    double Z0_e, Z0_o, ang_l_dest;
    double f1, f2, ft1, ft2, j11, j12, j21, j22, d_s_h, d_w_h, err;
    double eps = 1e-04;
    double w_h, s_h, le, lo;

    /* required value of Z0_e and Z0_o */
    Z0_e = GetParameter( TCP::Z0_E );
    Z0_o = GetParameter( TCP::Z0_O );

    ang_l_e = GetParameter( TCP::ANG_L );
    ang_l_o = GetParameter( TCP::ANG_L );
    ang_l_dest = GetParameter( TCP::ANG_L );

    /* calculate width and use for initial value in Newton's method */
    synth_width();
    w_h = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H );
    s_h = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H );
    f1 = f2 = 0;

    int iters = 0;

    /* rather crude Newton-Rhapson */
    do
    {
        ++iters;

        /* compute Jacobian */
        syn_fun( &ft1, &ft2, s_h + eps, w_h, Z0_e, Z0_o );
        j11 = ( ft1 - f1 ) / eps;
        j21 = ( ft2 - f2 ) / eps;
        syn_fun( &ft1, &ft2, s_h, w_h + eps, Z0_e, Z0_o );
        j12 = ( ft1 - f1 ) / eps;
        j22 = ( ft2 - f2 ) / eps;

        /* compute next step; increments of s_h and w_h */
        d_s_h = ( -f1 * j22 + f2 * j12 ) / ( j11 * j22 - j21 * j12 );
        d_w_h = ( -f2 * j11 + f1 * j21 ) / ( j11 * j22 - j21 * j12 );

        s_h += d_s_h;
        w_h += d_w_h;

        /* compute the error with the new values of s_h and w_h */
        syn_fun( &f1, &f2, s_h, w_h, Z0_e, Z0_o );
        err = sqrt( f1 * f1 + f2 * f2 );

        /* converged ? */
    } while( err > 1e-04 && iters < 250 );

    if( err > 1e-04 )
        return false;

    /* denormalize computed width and spacing */
    SetParameter( TCP::PHYS_S, s_h * GetParameter( TCP::H ) );
    SetParameter( TCP::PHYS_WIDTH, w_h * GetParameter( TCP::H ) );

    /* calculate physical length */
    le = TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( er_eff_e ) * ang_l_dest / 2.0 / M_PI;
    lo = TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( er_eff_o ) * ang_l_dest / 2.0 / M_PI;
    SetParameter( TCP::PHYS_LEN, sqrt( le * lo ) );

    Analyse();

    SetParameter( TCP::ANG_L, ang_l_dest );
    SetParameter( TCP::Z0_E, Z0_e );
    SetParameter( TCP::Z0_O, Z0_o );

    return true;
}


void COUPLED_MICROSTRIP::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF_EVEN, er_eff_e );
    SetAnalysisResult( TCP::EPSILON_EFF_ODD, er_eff_o );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY_EVEN, prop_delay_e );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY_ODD, prop_delay_o );
    SetAnalysisResult( TCP::ATTEN_COND_EVEN, atten_cond_e );
    SetAnalysisResult( TCP::ATTEN_COND_ODD, atten_cond_o );
    SetAnalysisResult( TCP::ATTEN_DILECTRIC_EVEN, atten_dielectric_e );
    SetAnalysisResult( TCP::ATTEN_DILECTRIC_ODD, atten_dielectric_o );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );
    SetAnalysisResult( TCP::Z_DIFF, Zdiff );

    const double Z0_E = GetParameter( TCP::Z0_E );
    const double Z0_O = GetParameter( TCP::Z0_O );
    const double ANG_L = sqrt( ang_l_e * ang_l_o );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_E_invalid = !std::isfinite( Z0_E ) || Z0_E <= 0;
    const bool Z0_O_invalid = !std::isfinite( Z0_O ) || Z0_O <= 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0;

    SetAnalysisResult( TCP::Z0_E, Z0_E, Z0_E_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::Z0_O, Z0_O, Z0_O_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_S, S, S_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void COUPLED_MICROSTRIP::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF_EVEN, er_eff_e );
    SetSynthesisResult( TCP::EPSILON_EFF_ODD, er_eff_o );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY_EVEN, prop_delay_e );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY_ODD, prop_delay_o );
    SetSynthesisResult( TCP::ATTEN_COND_EVEN, atten_cond_e );
    SetSynthesisResult( TCP::ATTEN_COND_ODD, atten_cond_o );
    SetSynthesisResult( TCP::ATTEN_DILECTRIC_EVEN, atten_dielectric_e );
    SetSynthesisResult( TCP::ATTEN_DILECTRIC_ODD, atten_dielectric_o );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );
    SetSynthesisResult( TCP::Z_DIFF, Zdiff );

    const double Z0_E = GetParameter( TCP::Z0_E );
    const double Z0_O = GetParameter( TCP::Z0_O );
    const double ANG_L = sqrt( ang_l_e * ang_l_o );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_E_invalid = !std::isfinite( Z0_E ) || Z0_E <= 0;
    const bool Z0_O_invalid = !std::isfinite( Z0_O ) || Z0_O <= 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0;

    SetSynthesisResult( TCP::Z0_E, Z0_E, Z0_E_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::Z0_O, Z0_O, Z0_O_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_S, S, S_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
}


double COUPLED_MICROSTRIP::delta_u_thickness_single( double u, double t_h )
{
    double delta_u;

    if( t_h > 0.0 )
    {
        delta_u = ( 1.25 * t_h / M_PI )
                  * ( 1.0
                      + log( ( 2.0 + ( 4.0 * M_PI * u - 2.0 ) / ( 1.0 + exp( -100.0 * ( u - 1.0 / ( 2.0 * M_PI ) ) ) ) )
                             / t_h ) );
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
void COUPLED_MICROSTRIP::delta_u_thickness()
{
    double e_r, u, g, t_h;
    double delta_u, delta_t, delta_u_e, delta_u_o;

    e_r = GetParameter( TCP::EPSILONR );
    u = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H ); /* normalized line width */
    g = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H );     /* normalized line spacing */
    t_h = GetParameter( TCP::T ) / GetParameter( TCP::H );        /* normalized strip thickness */

    if( t_h > 0.0 )
    {
        /* single microstrip correction for finite strip thickness */
        delta_u = delta_u_thickness_single( u, t_h );
        delta_t = t_h / ( g * e_r );
        /* thickness correction for the even- and odd-mode */
        delta_u_e = delta_u * ( 1.0 - 0.5 * exp( -0.69 * delta_u / delta_t ) );
        delta_u_o = delta_u_e + delta_t;
    }
    else
    {
        delta_u_e = delta_u_o = 0.0;
    }

    w_t_e = GetParameter( TCP::PHYS_WIDTH ) + delta_u_e * GetParameter( TCP::H );
    w_t_o = GetParameter( TCP::PHYS_WIDTH ) + delta_u_o * GetParameter( TCP::H );
}


void COUPLED_MICROSTRIP::compute_single_line()
{
    /* prepare parameters for single microstrip computations */
    m_aux_microstrip.SetParameter( TCP::EPSILONR, GetParameter( TCP::EPSILONR ) );
    m_aux_microstrip.SetParameter( TCP::PHYS_WIDTH, GetParameter( TCP::PHYS_WIDTH ) );
    m_aux_microstrip.SetParameter( TCP::H, GetParameter( TCP::H ) );
    m_aux_microstrip.SetParameter( TCP::T, 0.0 );

    //m_aux_microstrip.m_parameters[H_T ) = m_parameters[H_T );
    m_aux_microstrip.SetParameter( TCP::H_T, 1e12 ); /* arbitrarily high */
    m_aux_microstrip.SetParameter( TCP::FREQUENCY, GetParameter( TCP::FREQUENCY ) );
    m_aux_microstrip.SetParameter( TCP::MURC, GetParameter( TCP::MURC ) );
    m_aux_microstrip.microstrip_Z0();
    m_aux_microstrip.dispersion();
}


double COUPLED_MICROSTRIP::filling_factor_even( double u, double g, double e_r )
{
    double v, v3, v4, a_e, b_e, q_inf;

    v = u * ( 20.0 + g * g ) / ( 10.0 + g * g ) + g * exp( -g );
    v3 = v * v * v;
    v4 = v3 * v;
    a_e = 1.0 + log( ( v4 + v * v / 2704.0 ) / ( v4 + 0.432 ) ) / 49.0 + log( 1.0 + v3 / 5929.741 ) / 18.7;
    b_e = 0.564 * pow( ( ( e_r - 0.9 ) / ( e_r + 3.0 ) ), 0.053 );

    /* filling factor, with width corrected for thickness */
    q_inf = pow( ( 1.0 + 10.0 / v ), -a_e * b_e );

    return q_inf;
}


double COUPLED_MICROSTRIP::filling_factor_odd( double u, double g, double e_r )
{
    double b_odd = 0.747 * e_r / ( 0.15 + e_r );
    double c_odd = b_odd - ( b_odd - 0.207 ) * exp( -0.414 * u );
    double d_odd = 0.593 + 0.694 * exp( -0.562 * u );

    /* filling factor, with width corrected for thickness */
    double q_inf = exp( -c_odd * pow( g, d_odd ) );

    return q_inf;
}


double COUPLED_MICROSTRIP::delta_q_cover_even( double h2h )
{
    double q_c;

    if( h2h <= 39 )
        q_c = tanh( 1.626 + 0.107 * h2h - 1.733 / sqrt( h2h ) );
    else
        q_c = 1.0;

    return q_c;
}


double COUPLED_MICROSTRIP::delta_q_cover_odd( double h2h )
{
    double q_c;

    if( h2h <= 7 )
        q_c = tanh( 9.575 / ( 7.0 - h2h ) - 2.965 + 1.68 * h2h - 0.311 * h2h * h2h );
    else
        q_c = 1.0;

    return q_c;
}


void COUPLED_MICROSTRIP::er_eff_static()
{
    double u_t_e, u_t_o, g, h2, h2h;
    double a_o, t_h, q, q_c, q_t, q_inf;
    double er_eff_single;
    double er;

    er = GetParameter( TCP::EPSILONR );

    /* compute zero-thickness single line parameters */
    compute_single_line();
    er_eff_single = m_aux_microstrip.er_eff_0;

    h2 = GetParameter( TCP::H_T );
    u_t_e = w_t_e / GetParameter( TCP::H );                   /* normalized even_mode line width */
    u_t_o = w_t_o / GetParameter( TCP::H );                   /* normalized odd_mode line width */
    g = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H ); /* normalized line spacing */
    h2h = h2 / GetParameter( TCP::H );                        /* normalized cover height */
    t_h = GetParameter( TCP::T ) / GetParameter( TCP::H );    /* normalized strip thickness */

    /* filling factor, computed with thickness corrected width */
    q_inf = filling_factor_even( u_t_e, g, er );
    /* cover effect */
    q_c = delta_q_cover_even( h2h );
    /* thickness effect */
    q_t = m_aux_microstrip.delta_q_thickness( u_t_e, t_h );
    /* resultant filling factor */
    q = ( q_inf - q_t ) * q_c;
    /* static even-mode effective dielectric constant */
    er_eff_e_0 = 0.5 * ( er + 1.0 ) + 0.5 * ( er - 1.0 ) * q;

    /* filling factor, with width corrected for thickness */
    q_inf = filling_factor_odd( u_t_o, g, er );
    /* cover effect */
    q_c = delta_q_cover_odd( h2h );
    /* thickness effect */
    q_t = m_aux_microstrip.delta_q_thickness( u_t_o, t_h );
    /* resultant filling factor */
    q = ( q_inf - q_t ) * q_c;

    a_o = 0.7287 * ( er_eff_single - 0.5 * ( er + 1.0 ) ) * ( 1.0 - exp( -0.179 * u_t_o ) );

    /* static odd-mode effective dielectric constant */
    er_eff_o_0 = ( 0.5 * ( er + 1.0 ) + a_o - er_eff_single ) * q + er_eff_single;
}


double COUPLED_MICROSTRIP::delta_Z0_even_cover( double g, double u, double h2h )
{
    double f_e, g_e, delta_Z0_even;
    double x, y, A, B, C, D, E, F;

    A = -4.351 / pow( 1.0 + h2h, 1.842 );
    B = 6.639 / pow( 1.0 + h2h, 1.861 );
    C = -2.291 / pow( 1.0 + h2h, 1.90 );
    f_e = 1.0 - atanh( A + ( B + C * u ) * u );

    x = pow( 10.0, 0.103 * g - 0.159 );
    y = pow( 10.0, 0.0492 * g - 0.073 );
    D = 0.747 / sin( 0.5 * M_PI * x );
    E = 0.725 * sin( 0.5 * M_PI * y );
    F = pow( 10.0, 0.11 - 0.0947 * g );
    g_e = 270.0 * ( 1.0 - tanh( D + E * sqrt( 1.0 + h2h ) - F / ( 1.0 + h2h ) ) );

    delta_Z0_even = f_e * g_e;

    return delta_Z0_even;
}


double COUPLED_MICROSTRIP::delta_Z0_odd_cover( double g, double u, double h2h )
{
    double f_o, g_o, delta_Z0_odd;
    double G, J, K, L;

    J = tanh( pow( 1.0 + h2h, 1.585 ) / 6.0 );
    f_o = pow( u, J );

    G = 2.178 - 0.796 * g;

    if( g > 0.858 )
        K = log10( 20.492 * pow( g, 0.174 ) );
    else
        K = 1.30;

    if( g > 0.873 )
        L = 2.51 * pow( g, -0.462 );
    else
        L = 2.674;

    g_o = 270.0 * ( 1.0 - tanh( G + K * sqrt( 1.0 + h2h ) - L / ( 1.0 + h2h ) ) );

    delta_Z0_odd = f_o * g_o;

    return delta_Z0_odd;
}


void COUPLED_MICROSTRIP::Z0_even_odd()
{
    double er_eff, h2, u_t_e, u_t_o, g, h2h;
    double Q_1, Q_2, Q_3, Q_4, Q_5, Q_6, Q_7, Q_8, Q_9, Q_10;
    double delta_Z0_e_0, delta_Z0_o_0, Z0_single, er_eff_single;

    h2 = GetParameter( TCP::H_T );
    u_t_e = w_t_e / GetParameter( TCP::H );                   /* normalized even-mode line width */
    u_t_o = w_t_o / GetParameter( TCP::H );                   /* normalized odd-mode line width */
    g = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H ); /* normalized line spacing */
    h2h = h2 / GetParameter( TCP::H );                        /* normalized cover height */

    Z0_single = m_aux_microstrip.Z0_0;
    er_eff_single = m_aux_microstrip.er_eff_0;

    /* even-mode */
    er_eff = er_eff_e_0;
    Q_1 = 0.8695 * pow( u_t_e, 0.194 );
    Q_2 = 1.0 + 0.7519 * g + 0.189 * pow( g, 2.31 );
    Q_3 = 0.1975 + pow( ( 16.6 + pow( ( 8.4 / g ), 6.0 ) ), -0.387 )
          + log( pow( g, 10.0 ) / ( 1.0 + pow( g / 3.4, 10.0 ) ) ) / 241.0;
    Q_4 = 2.0 * Q_1 / ( Q_2 * ( exp( -g ) * pow( u_t_e, Q_3 ) + ( 2.0 - exp( -g ) ) * pow( u_t_e, -Q_3 ) ) );
    /* static even-mode impedance */
    Z0_e_0 = Z0_single * sqrt( er_eff_single / er_eff ) / ( 1.0 - sqrt( er_eff_single ) * Q_4 * Z0_single / TC::ZF0 );
    /* correction for cover */
    delta_Z0_e_0 = delta_Z0_even_cover( g, u_t_e, h2h ) / sqrt( er_eff );

    Z0_e_0 = Z0_e_0 - delta_Z0_e_0;

    /* odd-mode */
    er_eff = er_eff_o_0;
    Q_5 = 1.794 + 1.14 * log( 1.0 + 0.638 / ( g + 0.517 * pow( g, 2.43 ) ) );
    Q_6 = 0.2305 + log( pow( g, 10.0 ) / ( 1.0 + pow( g / 5.8, 10.0 ) ) ) / 281.3
          + log( 1.0 + 0.598 * pow( g, 1.154 ) ) / 5.1;
    Q_7 = ( 10.0 + 190.0 * g * g ) / ( 1.0 + 82.3 * g * g * g );
    Q_8 = exp( -6.5 - 0.95 * log( g ) - pow( g / 0.15, 5.0 ) );
    Q_9 = log( Q_7 ) * ( Q_8 + 1.0 / 16.5 );
    Q_10 = ( Q_2 * Q_4 - Q_5 * exp( log( u_t_o ) * Q_6 * pow( u_t_o, -Q_9 ) ) ) / Q_2;

    /* static odd-mode impedance */
    Z0_o_0 = Z0_single * sqrt( er_eff_single / er_eff ) / ( 1.0 - sqrt( er_eff_single ) * Q_10 * Z0_single / TC::ZF0 );
    /* correction for cover */
    delta_Z0_o_0 = delta_Z0_odd_cover( g, u_t_o, h2h ) / sqrt( er_eff );

    Z0_o_0 = Z0_o_0 - delta_Z0_o_0;
}


void COUPLED_MICROSTRIP::er_eff_freq()
{
    double P_1, P_2, P_3, P_4, P_5, P_6, P_7;
    double P_8, P_9, P_10, P_11, P_12, P_13, P_14, P_15;
    double F_e, F_o;
    double er_eff, u, g, f_n;

    u = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H ); /* normalize line width */
    g = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H );     /* normalize line spacing */

    /* normalized frequency [GHz * mm] */
    f_n = GetParameter( TCP::FREQUENCY ) * GetParameter( TCP::H ) / 1e06;

    er_eff = er_eff_e_0;
    P_1 = 0.27488 + ( 0.6315 + 0.525 / pow( 1.0 + 0.0157 * f_n, 20.0 ) ) * u - 0.065683 * exp( -8.7513 * u );
    P_2 = 0.33622 * ( 1.0 - exp( -0.03442 * GetParameter( TCP::EPSILONR ) ) );
    P_3 = 0.0363 * exp( -4.6 * u ) * ( 1.0 - exp( -pow( f_n / 38.7, 4.97 ) ) );
    P_4 = 1.0 + 2.751 * ( 1.0 - exp( -pow( GetParameter( TCP::EPSILONR ) / 15.916, 8.0 ) ) );
    P_5 = 0.334 * exp( -3.3 * pow( GetParameter( TCP::EPSILONR ) / 15.0, 3.0 ) ) + 0.746;
    P_6 = P_5 * exp( -pow( f_n / 18.0, 0.368 ) );
    P_7 = 1.0 + 4.069 * P_6 * pow( g, 0.479 ) * exp( -1.347 * pow( g, 0.595 ) - 0.17 * pow( g, 2.5 ) );

    F_e = P_1 * P_2 * pow( ( P_3 * P_4 + 0.1844 * P_7 ) * f_n, 1.5763 );
    /* even-mode effective dielectric constant */
    er_eff_e = GetParameter( TCP::EPSILONR ) - ( GetParameter( TCP::EPSILONR ) - er_eff ) / ( 1.0 + F_e );
    prop_delay_e = UnitPropagationDelay( er_eff_e );

    er_eff = er_eff_o_0;
    P_8 = 0.7168 * ( 1.0 + 1.076 / ( 1.0 + 0.0576 * ( GetParameter( TCP::EPSILONR ) - 1.0 ) ) );
    P_9 = P_8
          - 0.7913 * ( 1.0 - exp( -pow( f_n / 20.0, 1.424 ) ) )
                    * atan( 2.481 * pow( GetParameter( TCP::EPSILONR ) / 8.0, 0.946 ) );
    P_10 = 0.242 * pow( GetParameter( TCP::EPSILONR ) - 1.0, 0.55 );
    P_11 = 0.6366 * ( exp( -0.3401 * f_n ) - 1.0 ) * atan( 1.263 * pow( u / 3.0, 1.629 ) );
    P_12 = P_9 + ( 1.0 - P_9 ) / ( 1.0 + 1.183 * pow( u, 1.376 ) );
    P_13 = 1.695 * P_10 / ( 0.414 + 1.605 * P_10 );
    P_14 = 0.8928 + 0.1072 * ( 1.0 - exp( -0.42 * pow( f_n / 20.0, 3.215 ) ) );
    P_15 = fabs( 1.0 - 0.8928 * ( 1.0 + P_11 ) * P_12 * exp( -P_13 * pow( g, 1.092 ) ) / P_14 );

    F_o = P_1 * P_2 * pow( ( P_3 * P_4 + 0.1844 ) * f_n * P_15, 1.5763 );
    /* odd-mode effective dielectric constant */
    er_eff_o = GetParameter( TCP::EPSILONR ) - ( GetParameter( TCP::EPSILONR ) - er_eff ) / ( 1.0 + F_o );
    prop_delay_o = UnitPropagationDelay( er_eff_o );
}


void COUPLED_MICROSTRIP::conductor_losses()
{
    double e_r_eff_e_0, e_r_eff_o_0, Z0_h_e, Z0_h_o, delta;
    double K, R_s, Q_c_e, Q_c_o, alpha_c_e, alpha_c_o;

    e_r_eff_e_0 = er_eff_e_0;
    e_r_eff_o_0 = er_eff_o_0;
    Z0_h_e = Z0_e_0 * sqrt( e_r_eff_e_0 ); /* homogeneous stripline impedance */
    Z0_h_o = Z0_o_0 * sqrt( e_r_eff_o_0 ); /* homogeneous stripline impedance */
    delta = GetParameter( TCP::SKIN_DEPTH );

    if( GetParameter( TCP::FREQUENCY ) > 0.0 )
    {
        /* current distribution factor (same for the two modes) */
        K = exp( -1.2 * pow( ( Z0_h_e + Z0_h_o ) / ( 2.0 * TC::ZF0 ), 0.7 ) );
        /* skin resistance */
        R_s = 1.0 / ( GetParameter( TCP::SIGMA ) * delta );
        /* correction for surface roughness */
        R_s *= 1.0 + ( ( 2.0 / M_PI ) * atan( 1.40 * pow( ( GetParameter( TCP::ROUGH ) / delta ), 2.0 ) ) );

        /* even-mode strip inductive quality factor */
        Q_c_e = ( M_PI * Z0_h_e * GetParameter( TCP::PHYS_WIDTH ) * GetParameter( TCP::FREQUENCY ) )
                / ( R_s * TC::C0 * K );
        /* even-mode losses per unit length */
        alpha_c_e = ( 20.0 * M_PI / log( 10.0 ) ) * GetParameter( TCP::FREQUENCY ) * sqrt( e_r_eff_e_0 )
                    / ( TC::C0 * Q_c_e );

        /* odd-mode strip inductive quality factor */
        Q_c_o = ( M_PI * Z0_h_o * GetParameter( TCP::PHYS_WIDTH ) * GetParameter( TCP::FREQUENCY ) )
                / ( R_s * TC::C0 * K );
        /* odd-mode losses per unit length */
        alpha_c_o = ( 20.0 * M_PI / log( 10.0 ) ) * GetParameter( TCP::FREQUENCY ) * sqrt( e_r_eff_o_0 )
                    / ( TC::C0 * Q_c_o );
    }
    else
    {
        alpha_c_e = alpha_c_o = 0.0;
    }

    atten_cond_e = alpha_c_e * GetParameter( TCP::PHYS_LEN );
    atten_cond_o = alpha_c_o * GetParameter( TCP::PHYS_LEN );
}


void COUPLED_MICROSTRIP::dielectric_losses()
{
    double e_r, e_r_eff_e_0, e_r_eff_o_0;
    double alpha_d_e, alpha_d_o;

    e_r = GetParameter( TCP::EPSILONR );
    e_r_eff_e_0 = er_eff_e_0;
    e_r_eff_o_0 = er_eff_o_0;

    alpha_d_e = ( 20.0 * M_PI / log( 10.0 ) ) * ( GetParameter( TCP::FREQUENCY ) / TC::C0 )
                * ( e_r / sqrt( e_r_eff_e_0 ) ) * ( ( e_r_eff_e_0 - 1.0 ) / ( e_r - 1.0 ) ) * GetParameter( TCP::TAND );
    alpha_d_o = ( 20.0 * M_PI / log( 10.0 ) ) * ( GetParameter( TCP::FREQUENCY ) / TC::C0 )
                * ( e_r / sqrt( e_r_eff_o_0 ) ) * ( ( e_r_eff_o_0 - 1.0 ) / ( e_r - 1.0 ) ) * GetParameter( TCP::TAND );

    atten_dielectric_e = alpha_d_e * GetParameter( TCP::PHYS_LEN );
    atten_dielectric_o = alpha_d_o * GetParameter( TCP::PHYS_LEN );
}


void COUPLED_MICROSTRIP::attenuation()
{
    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );
    conductor_losses();
    dielectric_losses();
}


void COUPLED_MICROSTRIP::line_angle()
{
    double e_r_eff_e, e_r_eff_o;
    double v_e, v_o, lambda_g_e, lambda_g_o;

    e_r_eff_e = er_eff_e;
    e_r_eff_o = er_eff_o;

    /* even-mode velocity */
    v_e = TC::C0 / sqrt( e_r_eff_e );
    /* odd-mode velocity */
    v_o = TC::C0 / sqrt( e_r_eff_o );
    /* even-mode wavelength */
    lambda_g_e = v_e / GetParameter( TCP::FREQUENCY );
    /* odd-mode wavelength */
    lambda_g_o = v_o / GetParameter( TCP::FREQUENCY );
    /* electrical angles */
    ang_l_e = 2.0 * M_PI * GetParameter( TCP::PHYS_LEN ) / lambda_g_e; /* in radians */
    ang_l_o = 2.0 * M_PI * GetParameter( TCP::PHYS_LEN ) / lambda_g_o; /* in radians */
}


void COUPLED_MICROSTRIP::diff_impedance()
{
    // Note that differential impedance is exactly twice the odd mode impedance.
    // Odd mode is not the same as single-ended impedance, so avoid approximations found
    // on websites that use static single ended impedance as the starting point

    Zdiff = 2 * Z0_o_0;
}


/*
 * Z0_dispersion() - calculate frequency dependency of characteristic
 * impedances
 */
void COUPLED_MICROSTRIP::Z0_dispersion()
{
    double Q_0;
    double Q_11, Q_12, Q_13, Q_14, Q_15, Q_16, Q_17, Q_18, Q_19, Q_20, Q_21;
    double Q_22, Q_23, Q_24, Q_25, Q_26, Q_27, Q_28, Q_29;
    double r_e, q_e, p_e, d_e, C_e;
    double e_r_eff_o_f, e_r_eff_o_0;
    double e_r_eff_single_f, e_r_eff_single_0, Z0_single_f;
    double f_n, g, u, e_r;
    double R_1, R_2, R_7, R_10, R_11, R_12, R_15, R_16, tmpf;

    e_r = GetParameter( TCP::EPSILONR );

    u = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H ); /* normalize line width */
    g = GetParameter( TCP::PHYS_S ) / GetParameter( TCP::H );     /* normalize line spacing */

    /* normalized frequency [GHz * mm] */
    f_n = GetParameter( TCP::FREQUENCY ) * GetParameter( TCP::H ) / 1e06;

    e_r_eff_single_f = m_aux_microstrip.GetParameter( TCP::EPSILON_EFF );
    e_r_eff_single_0 = m_aux_microstrip.er_eff_0;
    Z0_single_f = m_aux_microstrip.GetParameter( TCP::Z0 );

    e_r_eff_o_f = er_eff_o;
    e_r_eff_o_0 = er_eff_o_0;

    Q_11 = 0.893 * ( 1.0 - 0.3 / ( 1.0 + 0.7 * ( e_r - 1.0 ) ) );
    Q_12 = 2.121 * ( pow( f_n / 20.0, 4.91 ) / ( 1.0 + Q_11 * pow( f_n / 20.0, 4.91 ) ) ) * exp( -2.87 * g )
           * pow( g, 0.902 );
    Q_13 = 1.0 + 0.038 * pow( e_r / 8.0, 5.1 );
    Q_14 = 1.0 + 1.203 * pow( e_r / 15.0, 4.0 ) / ( 1.0 + pow( e_r / 15.0, 4.0 ) );
    Q_15 = 1.887 * exp( -1.5 * pow( g, 0.84 ) ) * pow( g, Q_14 )
           / ( 1.0 + 0.41 * pow( f_n / 15.0, 3.0 ) * pow( u, 2.0 / Q_13 ) / ( 0.125 + pow( u, 1.626 / Q_13 ) ) );
    Q_16 = ( 1.0 + 9.0 / ( 1.0 + 0.403 * pow( e_r - 1.0, 2 ) ) ) * Q_15;
    Q_17 = 0.394 * ( 1.0 - exp( -1.47 * pow( u / 7.0, 0.672 ) ) ) * ( 1.0 - exp( -4.25 * pow( f_n / 20.0, 1.87 ) ) );
    Q_18 = 0.61 * ( 1.0 - exp( -2.13 * pow( u / 8.0, 1.593 ) ) ) / ( 1.0 + 6.544 * pow( g, 4.17 ) );
    Q_19 = 0.21 * g * g * g * g
           / ( ( 1.0 + 0.18 * pow( g, 4.9 ) ) * ( 1.0 + 0.1 * u * u ) * ( 1.0 + pow( f_n / 24.0, 3.0 ) ) );
    Q_20 = ( 0.09 + 1.0 / ( 1.0 + 0.1 * pow( e_r - 1, 2.7 ) ) ) * Q_19;
    Q_21 = fabs( 1.0 - 42.54 * pow( g, 0.133 ) * exp( -0.812 * g ) * pow( u, 2.5 ) / ( 1.0 + 0.033 * pow( u, 2.5 ) ) );

    r_e = pow( f_n / 28.843, 12 );
    q_e = 0.016 + pow( 0.0514 * e_r * Q_21, 4.524 );
    p_e = 4.766 * exp( -3.228 * pow( u, 0.641 ) );
    d_e = 5.086 * q_e * ( r_e / ( 0.3838 + 0.386 * q_e ) ) * ( exp( -22.2 * pow( u, 1.92 ) ) / ( 1.0 + 1.2992 * r_e ) )
          * ( pow( e_r - 1.0, 6.0 ) / ( 1.0 + 10 * pow( e_r - 1.0, 6.0 ) ) );
    C_e = 1.0 + 1.275 * ( 1.0 - exp( -0.004625 * p_e * pow( e_r, 1.674 ) * pow( f_n / 18.365, 2.745 ) ) ) - Q_12 + Q_16
          - Q_17 + Q_18 + Q_20;


    R_1 = 0.03891 * pow( e_r, 1.4 );
    R_2 = 0.267 * pow( u, 7.0 );
    R_7 = 1.206 - 0.3144 * exp( -R_1 ) * ( 1.0 - exp( -R_2 ) );
    R_10 = 0.00044 * pow( e_r, 2.136 ) + 0.0184;
    tmpf = pow( f_n / 19.47, 6.0 );
    R_11 = tmpf / ( 1.0 + 0.0962 * tmpf );
    R_12 = 1.0 / ( 1.0 + 0.00245 * u * u );
    R_15 = 0.707 * R_10 * pow( f_n / 12.3, 1.097 );
    R_16 = 1.0 + 0.0503 * e_r * e_r * R_11 * ( 1.0 - exp( -pow( u / 15.0, 6.0 ) ) );
    Q_0 = R_7 * ( 1.0 - 1.1241 * ( R_12 / R_16 ) * exp( -0.026 * pow( f_n, 1.15656 ) - R_15 ) );

    /* even-mode frequency-dependent characteristic impedances */
    SetParameter( TCP::Z0_E, Z0_e_0 * pow( 0.9408 * pow( e_r_eff_single_f, C_e ) - 0.9603, Q_0 )
                                     / pow( ( 0.9408 - d_e ) * pow( e_r_eff_single_0, C_e ) - 0.9603, Q_0 ) );

    Q_29 = 15.16 / ( 1.0 + 0.196 * pow( e_r - 1.0, 2.0 ) );
    tmpf = pow( e_r - 1.0, 3.0 );
    Q_28 = 0.149 * tmpf / ( 94.5 + 0.038 * tmpf );
    tmpf = pow( e_r - 1.0, 1.5 );
    Q_27 = 0.4 * pow( g, 0.84 ) * ( 1.0 + 2.5 * tmpf / ( 5.0 + tmpf ) );
    tmpf = pow( ( e_r - 1.0 ) / 13.0, 12.0 );
    Q_26 = 30.0 - 22.2 * ( tmpf / ( 1.0 + 3.0 * tmpf ) ) - Q_29;
    tmpf = ( e_r - 1.0 ) * ( e_r - 1.0 );
    Q_25 = ( 0.3 * f_n * f_n / ( 10.0 + f_n * f_n ) ) * ( 1.0 + 2.333 * tmpf / ( 5.0 + tmpf ) );
    Q_24 = 2.506 * Q_28 * pow( u, 0.894 ) * pow( ( 1.0 + 1.3 * u ) * f_n / 99.25, 4.29 ) / ( 3.575 + pow( u, 0.894 ) );
    Q_23 = 1.0 + 0.005 * f_n * Q_27 / ( ( 1.0 + 0.812 * pow( f_n / 15.0, 1.9 ) ) * ( 1.0 + 0.025 * u * u ) );
    Q_22 = 0.925 * pow( f_n / Q_26, 1.536 ) / ( 1.0 + 0.3 * pow( f_n / 30.0, 1.536 ) );

    /* odd-mode frequency-dependent characteristic impedances */
    SetParameter( TCP::Z0_O, Z0_single_f
                                     + ( Z0_o_0 * pow( e_r_eff_o_f / e_r_eff_o_0, Q_22 ) - Z0_single_f * Q_23 )
                                               / ( 1.0 + Q_24 + pow( 0.46 * g, 2.2 ) * Q_25 ) );
}


void COUPLED_MICROSTRIP::syn_err_fun( double* f1, double* f2, double s_h, double w_h, double e_r, double w_h_se,
                                      double w_h_so )
{
    double g, he;

    g = cosh( 0.5 * M_PI * s_h );
    he = cosh( M_PI * w_h + 0.5 * M_PI * s_h );

    *f1 = ( 2.0 / M_PI ) * acosh( ( 2.0 * he - g + 1.0 ) / ( g + 1.0 ) );
    *f2 = ( 2.0 / M_PI ) * acosh( ( 2.0 * he - g - 1.0 ) / ( g - 1.0 ) );

    if( e_r <= 6.0 )
        *f2 += ( 4.0 / ( M_PI * ( 1.0 + e_r / 2.0 ) ) ) * acosh( 1.0 + 2.0 * w_h / s_h );
    else
        *f2 += ( 1.0 / M_PI ) * acosh( 1.0 + 2.0 * w_h / s_h );

    *f1 -= w_h_se;
    *f2 -= w_h_so;
}


void COUPLED_MICROSTRIP::synth_width()
{
    double Z0, e_r;
    double w_h_se, w_h_so, w_h, a, ce, co, s_h;
    double f1, f2, ft1, ft2, j11, j12, j21, j22, d_s_h, d_w_h, err;
    double eps = 1e-04;

    f1 = f2 = 0;
    e_r = GetParameter( TCP::EPSILONR );

    Z0 = GetParameter( TCP::Z0_E ) / 2.0;
    /* Wheeler formula for single microstrip synthesis */
    a = exp( Z0 * sqrt( e_r + 1.0 ) / 42.4 ) - 1.0;
    w_h_se = 8.0 * sqrt( a * ( ( 7.0 + 4.0 / e_r ) / 11.0 ) + ( ( 1.0 + 1.0 / e_r ) / 0.81 ) ) / a;

    Z0 = GetParameter( TCP::Z0_O ) / 2.0;
    /* Wheeler formula for single microstrip synthesis */
    a = exp( Z0 * sqrt( e_r + 1.0 ) / 42.4 ) - 1.0;
    w_h_so = 8.0 * sqrt( a * ( ( 7.0 + 4.0 / e_r ) / 11.0 ) + ( ( 1.0 + 1.0 / e_r ) / 0.81 ) ) / a;

    ce = cosh( 0.5 * M_PI * w_h_se );
    co = cosh( 0.5 * M_PI * w_h_so );
    /* first guess at m_parameters[PHYS_S )/h */
    s_h = ( 2.0 / M_PI ) * acosh( ( ce + co - 2.0 ) / ( co - ce ) );
    /* first guess at w/h */
    w_h = acosh( ( ce * co - 1.0 ) / ( co - ce ) ) / M_PI - s_h / 2.0;

    SetParameter( TCP::PHYS_S, s_h * GetParameter( TCP::H ) );
    SetParameter( TCP::PHYS_WIDTH, w_h * GetParameter( TCP::H ) );

    syn_err_fun( &f1, &f2, s_h, w_h, e_r, w_h_se, w_h_so );

    /* rather crude Newton-Rhapson; we need this because the estimate of */
    /* w_h is often quite far from the true value (see Akhtarzad S. et al.) */
    do
    {
        /* compute Jacobian */
        syn_err_fun( &ft1, &ft2, s_h + eps, w_h, e_r, w_h_se, w_h_so );
        j11 = ( ft1 - f1 ) / eps;
        j21 = ( ft2 - f2 ) / eps;
        syn_err_fun( &ft1, &ft2, s_h, w_h + eps, e_r, w_h_se, w_h_so );
        j12 = ( ft1 - f1 ) / eps;
        j22 = ( ft2 - f2 ) / eps;

        /* compute next step */
        d_s_h = ( -f1 * j22 + f2 * j12 ) / ( j11 * j22 - j21 * j12 );
        d_w_h = ( -f2 * j11 + f1 * j21 ) / ( j11 * j22 - j21 * j12 );

        //g_print("j11 = %e\tj12 = %e\tj21 = %e\tj22 = %e\n", j11, j12, j21, j22);
        //g_print("det = %e\n", j11*j22 - j21*j22);
        //g_print("d_s_h = %e\td_w_h = %e\n", d_s_h, d_w_h);

        s_h += d_s_h;
        w_h += d_w_h;

        /* check the error */
        syn_err_fun( &f1, &f2, s_h, w_h, e_r, w_h_se, w_h_so );

        err = sqrt( f1 * f1 + f2 * f2 );
        /* converged ? */
    } while( err > 1e-04 );


    SetParameter( TCP::PHYS_S, s_h * GetParameter( TCP::H ) );
    SetParameter( TCP::PHYS_WIDTH, w_h * GetParameter( TCP::H ) );
}


void COUPLED_MICROSTRIP::syn_fun( double* f1, double* f2, double s_h, double w_h, double Z0_e, double Z0_o )
{
    SetParameter( TCP::PHYS_S, s_h * GetParameter( TCP::H ) );
    SetParameter( TCP::PHYS_WIDTH, w_h * GetParameter( TCP::H ) );

    /* compute coupled microstrip parameters */
    Analyse();

    *f1 = GetParameter( TCP::Z0_E ) - Z0_e;
    *f2 = GetParameter( TCP::Z0_O ) - Z0_o;
}
