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

#include <transline_calculations/microstrip.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


void MICROSTRIP::Analyse()
{
    // Effective permeability
    mur_eff_ms();

    // Static impedance
    microstrip_Z0();

    // Calculate freq dependence of er and Z0
    dispersion();

    // Calculate electrical lengths
    line_angle();

    // Calculate losses
    attenuation();
}


bool MICROSTRIP::Synthesize( const SYNTHESIZE_OPTS aOpts )
{
    const double z0_dest = GetParameter( TCP::Z0 );
    const double angl_dest = GetParameter( TCP::ANG_L );

    // Calculate width and use for initial value in Newton's method
    SetParameter( TCP::PHYS_WIDTH, SynthesizeWidth() );

    // Optimise Z0, varying width
    if( !MinimiseZ0Error1D( TCP::PHYS_WIDTH, TCP::Z0 ) )
        return false;

    // Re-calculate with required output parameters
    SetParameter( TCP::Z0, z0_dest );
    SetParameter( TCP::ANG_L, angl_dest );
    double const er_eff = GetParameter( TCP::EPSILON_EFF );
    SetParameter( TCP::PHYS_LEN, TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( er_eff * mur_eff )
                                         * GetParameter( TCP::ANG_L ) / 2.0 / M_PI ); /* in m */
    Analyse();

    // Set the output parameters
    SetParameter( TCP::Z0, z0_dest );
    SetParameter( TCP::ANG_L, angl_dest );
    SetParameter( TCP::PHYS_LEN, TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( er_eff * mur_eff )
                                         * GetParameter( TCP::ANG_L ) / 2.0 / M_PI ); /* in m */

    return true;
}


void MICROSTRIP::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY, GetParameter( TCP::UNIT_PROP_DELAY ) );
    SetAnalysisResult( TCP::ATTEN_COND, GetParameter( TCP::ATTEN_COND ) );
    SetAnalysisResult( TCP::ATTEN_DILECTRIC, GetParameter( TCP::ATTEN_DILECTRIC ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double ANG_L = GetParameter( TCP::ANG_L );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void MICROSTRIP::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY, GetParameter( TCP::UNIT_PROP_DELAY ) );
    SetSynthesisResult( TCP::ATTEN_COND, GetParameter( TCP::ATTEN_COND ) );
    SetSynthesisResult( TCP::ATTEN_DILECTRIC, GetParameter( TCP::ATTEN_DILECTRIC ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double ANG_L = GetParameter( TCP::ANG_L );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;

    SetSynthesisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
}


double MICROSTRIP::SynthesizeWidth() const
{
    const double e_r = GetParameter( TCP::EPSILONR );
    const double a = ( ( GetParameter( TCP::Z0 ) / TC::ZF0 / 2 / M_PI ) * sqrt( ( e_r + 1 ) / 2. ) )
                     + ( ( e_r - 1 ) / ( e_r + 1 ) * ( 0.23 + ( 0.11 / e_r ) ) );
    const double b = TC::ZF0 / 2 * M_PI / ( GetParameter( TCP::Z0 ) * sqrt( e_r ) );

    double w_h;

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

    double width;

    if( GetParameter( TCP::H ) > 0.0 )
        width = w_h * GetParameter( TCP::H );
    else
        width = 0;

    return width;
}


double MICROSTRIP::Z0_dispersion( double u, double e_r, double e_r_eff_0, double e_r_eff_f, double f_n )
{
    const double R_1 = 0.03891 * pow( e_r, 1.4 );
    const double R_2 = 0.267 * pow( u, 7.0 );
    const double R_3 = 4.766 * exp( -3.228 * pow( u, 0.641 ) );
    const double R_4 = 0.016 + pow( 0.0514 * e_r, 4.524 );
    const double R_5 = pow( f_n / 28.843, 12.0 );
    const double R_6 = 22.2 * pow( u, 1.92 );
    const double R_7 = 1.206 - 0.3144 * exp( -R_1 ) * ( 1.0 - exp( -R_2 ) );
    const double R_8 = 1.0 + 1.275 * ( 1.0 - exp( -0.004625 * R_3 * pow( e_r, 1.674 ) * pow( f_n / 18.365, 2.745 ) ) );
    double       tmpf = pow( e_r - 1.0, 6.0 );
    const double R_9 = 5.086 * R_4 * ( R_5 / ( 0.3838 + 0.386 * R_4 ) ) * ( exp( -R_6 ) / ( 1.0 + 1.2992 * R_5 ) )
                       * ( tmpf / ( 1.0 + 10.0 * tmpf ) );
    const double R_10 = 0.00044 * pow( e_r, 2.136 ) + 0.0184;
    tmpf = pow( f_n / 19.47, 6.0 );
    const double R_11 = tmpf / ( 1.0 + 0.0962 * tmpf );
    const double R_12 = 1.0 / ( 1.0 + 0.00245 * u * u );
    const double R_13 = 0.9408 * pow( e_r_eff_f, R_8 ) - 0.9603;
    const double R_14 = ( 0.9408 - R_9 ) * pow( e_r_eff_0, R_8 ) - 0.9603;
    const double R_15 = 0.707 * R_10 * pow( f_n / 12.3, 1.097 );
    const double R_16 = 1.0 + 0.0503 * e_r * e_r * R_11 * ( 1.0 - exp( -pow( u / 15.0, 6.0 ) ) );
    const double R_17 = R_7 * ( 1.0 - 1.1241 * ( R_12 / R_16 ) * exp( -0.026 * pow( f_n, 1.15656 ) - R_15 ) );

    return pow( R_13 / R_14, R_17 );
}


double MICROSTRIP::Z0_homogeneous( double u )
{
    const double freq = 6.0 + ( 2.0 * M_PI - 6.0 ) * exp( -pow( 30.666 / u, 0.7528 ) );
    return ( TC::ZF0 / ( 2.0 * M_PI ) ) * log( freq / u + sqrt( 1.0 + 4.0 / ( u * u ) ) );
}


double MICROSTRIP::delta_Z0_cover( double u, double h2h )
{
    const double h2hp1 = 1.0 + h2h;
    const double P = 270.0 * ( 1.0 - tanh( 1.192 + 0.706 * sqrt( h2hp1 ) - 1.389 / h2hp1 ) );
    const double Q = 1.0109 - atanh( ( 0.012 * u + 0.177 * u * u - 0.027 * u * u * u ) / ( h2hp1 * h2hp1 ) );
    return P * Q;
}


double MICROSTRIP::filling_factor( double u, double e_r )
{
    const double u2 = u * u;
    const double u3 = u2 * u;
    const double u4 = u3 * u;
    const double a = 1.0 + log( ( u4 + u2 / 2704 ) / ( u4 + 0.432 ) ) / 49.0 + log( 1.0 + u3 / 5929.741 ) / 18.7;
    const double b = 0.564 * pow( ( e_r - 0.9 ) / ( e_r + 3.0 ), 0.053 );
    return pow( 1.0 + 10.0 / u, -a * b );
}


double MICROSTRIP::delta_q_cover( double h2h )
{
    return tanh( 1.043 + 0.121 * h2h - 1.164 / h2h );
}


double MICROSTRIP::delta_q_thickness( double u, double t_h )
{
    return ( 2.0 * log( 2.0 ) / M_PI ) * ( t_h / sqrt( u ) );
}


double MICROSTRIP::e_r_effective( double e_r, double q )
{
    return 0.5 * ( e_r + 1.0 ) + 0.5 * q * ( e_r - 1.0 );
}


double MICROSTRIP::delta_u_thickness( double u, double t_h, double e_r )
{
    double delta_u;

    if( t_h > 0.0 )
    {
        /* correction for thickness for a homogeneous microstrip */
        delta_u = ( t_h / M_PI ) * log( 1.0 + ( 4.0 * M_E ) * pow( tanh( sqrt( 6.517 * u ) ), 2.0 ) / t_h );
        /* correction for strip on a substrate with relative permettivity e_r */
        delta_u = 0.5 * delta_u * ( 1.0 + 1.0 / cosh( sqrt( e_r - 1.0 ) ) );
    }
    else
    {
        delta_u = 0.0;
    }
    return delta_u;
}


double MICROSTRIP::e_r_dispersion( double u, double e_r, double f_n )
{
    const double P_1 =
            0.27488 + u * ( 0.6315 + 0.525 / pow( 1.0 + 0.0157 * f_n, 20.0 ) ) - 0.065683 * exp( -8.7513 * u );
    const double P_2 = 0.33622 * ( 1.0 - exp( -0.03442 * e_r ) );
    const double P_3 = 0.0363 * exp( -4.6 * u ) * ( 1.0 - exp( -pow( f_n / 38.7, 4.97 ) ) );
    const double P_4 = 1.0 + 2.751 * ( 1.0 - exp( -pow( e_r / 15.916, 8.0 ) ) );

    return P_1 * P_2 * pow( ( P_3 * P_4 + 0.1844 ) * f_n, 1.5763 );
}


double MICROSTRIP::conductor_losses() const
{
    double       alpha_c;
    const double e_r_eff_0 = er_eff_0;
    const double delta = GetParameter( TCP::SKIN_DEPTH );

    if( GetParameter( TCP::FREQUENCY ) > 0.0 )
    {
        /* current distribution factor */
        const double K = exp( -1.2 * pow( Z0_h_1 / TC::ZF0, 0.7 ) );
        /* skin resistance */
        double R_s = 1.0 / ( GetParameter( TCP::SIGMA ) * delta );

        /* correction for surface roughness */
        R_s *= 1.0 + ( ( 2.0 / M_PI ) * atan( 1.40 * pow( ( GetParameter( TCP::ROUGH ) / delta ), 2.0 ) ) );
        /* strip inductive quality factor */
        const double Q_c = ( M_PI * Z0_h_1 * GetParameter( TCP::PHYS_WIDTH ) * GetParameter( TCP::FREQUENCY ) )
                           / ( R_s * TC::C0 * K );
        alpha_c = ( 20.0 * M_PI / log( 10.0 ) ) * GetParameter( TCP::FREQUENCY ) * sqrt( e_r_eff_0 ) / ( TC::C0 * Q_c );
    }
    else
    {
        alpha_c = 0.0;
    }

    return alpha_c;
}


double MICROSTRIP::dielectric_losses() const
{
    const double e_r = GetParameter( TCP::EPSILONR );
    const double e_r_eff_0 = er_eff_0;

    return ( 20.0 * M_PI / log( 10.0 ) ) * ( GetParameter( TCP::FREQUENCY ) / TC::C0 ) * ( e_r / sqrt( e_r_eff_0 ) )
           * ( ( e_r_eff_0 - 1.0 ) / ( e_r - 1.0 ) ) * GetParameter( TCP::TAND );
}


void MICROSTRIP::microstrip_Z0()
{
    const double e_r = GetParameter( TCP::EPSILONR );
    const double h2 = GetParameter( TCP::H_T );
    const double h2h = h2 / GetParameter( TCP::H );
    double       u = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H );
    const double t_h = GetParameter( TCP::T ) / GetParameter( TCP::H );

    /* compute normalized width correction for e_r = 1.0 */
    const double delta_u_1 = delta_u_thickness( u, t_h, 1.0 );
    /* compute homogeneous stripline impedance */
    Z0_h_1 = Z0_homogeneous( u + delta_u_1 );
    /* compute normalized width correction */
    const double delta_u_r = delta_u_thickness( u, t_h, e_r );
    u += delta_u_r;
    /* compute homogeneous stripline impedance */
    const double Z0_h_r = Z0_homogeneous( u );

    /* filling factor, with width corrected for thickness */
    const double q_inf = filling_factor( u, e_r );
    /* cover effect */
    const double q_c = delta_q_cover( h2h );
    /* thickness effect */
    const double q_t = delta_q_thickness( u, t_h );
    /* resultant filling factor */
    const double q = ( q_inf - q_t ) * q_c;

    /* e_r corrected for thickness and non-homogeneous material */
    const double e_r_eff_t = e_r_effective( e_r, q );

    /* effective dielectric constant */
    const double e_r_eff = e_r_eff_t * pow( Z0_h_1 / Z0_h_r, 2.0 );

    /* characteristic impedance, corrected for thickness, cover */
    /*   and non-homogeneous material */
    SetParameter( TCP::Z0, Z0_h_r / sqrt( e_r_eff_t ) );

    w_eff = u * GetParameter( TCP::H );
    er_eff_0 = e_r_eff;
    Z0_0 = GetParameter( TCP::Z0 );
}


void MICROSTRIP::dispersion()
{
    const double e_r = GetParameter( TCP::EPSILONR );
    const double e_r_eff_0 = er_eff_0;
    const double u = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::H );

    /* normalized frequency [GHz * mm] */
    const double f_n = GetParameter( TCP::FREQUENCY ) * GetParameter( TCP::H ) / 1e06;

    const double P = e_r_dispersion( u, e_r, f_n );
    /* effective dielectric constant corrected for dispersion */
    const double e_r_eff_f = e_r - ( e_r - e_r_eff_0 ) / ( 1.0 + P );

    const double D = Z0_dispersion( u, e_r, e_r_eff_0, e_r_eff_f, f_n );
    const double Z0_f = Z0_0 * D;

    SetParameter( TCP::UNIT_PROP_DELAY, UnitPropagationDelay( e_r_eff_f ) );
    SetParameter( TCP::EPSILON_EFF, e_r_eff_f );
    SetParameter( TCP::Z0, Z0_f );
}


void MICROSTRIP::attenuation()
{
    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );
    SetParameter( TCP::ATTEN_COND, conductor_losses() * GetParameter( TCP::PHYS_LEN ) );
    SetParameter( TCP::ATTEN_DILECTRIC, dielectric_losses() * GetParameter( TCP::PHYS_LEN ) );
}


void MICROSTRIP::mur_eff_ms()
{
    const double mur = GetParameter( TCP::MUR );
    const double h = GetParameter( TCP::H );
    const double w = GetParameter( TCP::PHYS_WIDTH );
    mur_eff = ( 2.0 * mur ) / ( ( 1.0 + mur ) + ( ( 1.0 - mur ) * pow( ( 1.0 + ( 10.0 * h / w ) ), -0.5 ) ) );
}


void MICROSTRIP::line_angle()
{
    double e_r_eff = GetParameter( TCP::EPSILON_EFF );

    // Velocity
    double v = TC::C0 / sqrt( e_r_eff * mur_eff );
    // Wavelength
    double lambda_g = v / GetParameter( TCP::FREQUENCY );
    // Electrical angles (rad)
    SetParameter( TCP::ANG_L, 2.0 * M_PI * GetParameter( TCP::PHYS_LEN ) / lambda_g );
}
