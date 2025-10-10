/*
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

/*
 * This implements the calculations described in:
 *
 * [1] S. B. Cohn, "Characteristic Impedance of the Shielded-Strip Transmission Line," in Transactions of the IRE
 *    Professional Group on Microwave Theory and Techniques, vol. 2, no. 2, pp. 52-57, July 1954
 * [2] S. B. Cohn, "Shielded Coupled-Strip Transmission Line," in IRE Transactions on Microwave Theory and Techniques,
 *    vol. 3, no. 5, pp. 29-38, October 1955
 */

#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


void COUPLED_STRIPLINE::Analyse()
{
    // Calculate skin depth
    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );

    // Get analysis parameters
    double       w = GetParameter( TCP::PHYS_WIDTH );
    double       t = GetParameter( TCP::T );
    double       s = GetParameter( TCP::PHYS_S );
    double       h = GetParameter( TCP::H );
    const double er = GetParameter( TCP::EPSILONR );

    calcZeroThicknessCoupledImpedances( h, w, s, er );

    // We've got the impedances now for an infinitely thin line
    if( t == 0.0 )
    {
        SetParameter( TCP::Z0_E, Z0_e_w_h_0_s_h );
        SetParameter( TCP::Z0_O, Z0_o_w_h_0_s_h );
    }
    else
    {
        calcSingleStripImpedances();
        calcFringeCapacitances( h, t, er );
        calcZ0EvenMode();
        calcZ0OddMode( t, s );
    }

    calcLosses();
    calcDielectrics();
}


bool COUPLED_STRIPLINE::Synthesize( const SYNTHESIZE_OPTS aOpts )
{
    if( aOpts == SYNTHESIZE_OPTS::FIX_WIDTH )
        return MinimiseZ0Error1D( TCP::PHYS_S, TCP::Z0_O, false );

    if( aOpts == SYNTHESIZE_OPTS::FIX_SPACING )
        return MinimiseZ0Error1D( TCP::PHYS_WIDTH, TCP::Z0_O, false );

    // This synthesis approach is modified from wcalc, which is released under GPL version 2
    // Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2006 Dan McMahill
    // All rights reserved

    double ze0 = 0;
    double zo0 = 0;

    const double h = GetParameter( TCP::H );
    const double er = GetParameter( TCP::EPSILONR );

    const double z0e_target = GetParameter( TCP::Z0_E );
    const double z0o_target = GetParameter( TCP::Z0_O );
    // Calculate Z0 and coupling, k
    const double z0 = sqrt( z0e_target * z0o_target );
    const double k = ( z0e_target - z0o_target ) / ( z0e_target + z0o_target );

    int maxiters = 50;

    // Initial guess at a solution. Note that this is an initial guess for coupled microstrip, not coupled stripline...
    static constexpr double ai[] = { 1, -0.301, 3.209, -27.282, 56.609, -37.746 };
    static constexpr double bi[] = { 0.020, -0.623, 17.192, -68.946, 104.740, -16.148 };
    static constexpr double ci[] = { 0.002, -0.347, 7.171, -36.910, 76.132, -51.616 };

    const double AW = exp( z0 * sqrt( er + 1.0 ) / 42.4 ) - 1.0;
    const double F1 = 8.0 * sqrt( AW * ( 7.0 + 4.0 / er ) / 11.0 + ( 1.0 + 1.0 / er ) / 0.81 ) / AW;

    double F2 = 0.0, F3 = 0.0;
    ;

    for( int i = 0; i <= 5; i++ )
        F2 = F2 + ai[i] * pow( k, i );

    for( int i = 0; i <= 5; i++ )
        F3 = F3 + ( bi[i] - ci[i] * ( 9.6 - er ) ) * pow( ( 0.6 - k ), static_cast<double>( i ) );

    double w = h * fabs( F1 * F2 );
    double s = h * fabs( F1 * F3 );

    int    iters = 0;
    bool   done = false;
    double delta = 0.0;

    delta = TC::UNIT_MIL * 1e-5;

    const double cval = 1e-12 * z0e_target * z0o_target;

    while( !done && iters < maxiters )
    {
        iters++;

        // Compute impedances with initial solution guess
        SetParameter( TCP::PHYS_WIDTH, w );
        SetParameter( TCP::PHYS_S, s );
        Analyse();

        // Check for convergence
        ze0 = GetParameter( TCP::Z0_E );
        zo0 = GetParameter( TCP::Z0_O );
        const double err = pow( ( ze0 - z0e_target ), 2.0 ) + pow( ( zo0 - z0o_target ), 2.0 );

        if( err < cval )
        {
            done = true;
        }
        else
        {
            // Approximate the first Jacobian
            SetParameter( TCP::PHYS_WIDTH, w + delta );
            SetParameter( TCP::PHYS_S, s );
            Analyse();

            const double ze1 = GetParameter( TCP::Z0_E );
            const double zo1 = GetParameter( TCP::Z0_O );

            SetParameter( TCP::PHYS_WIDTH, w );
            SetParameter( TCP::PHYS_S, s + delta );
            Analyse();

            const double ze2 = GetParameter( TCP::Z0_E );
            const double zo2 = GetParameter( TCP::Z0_O );

            const double dedw = ( ze1 - ze0 ) / delta;
            const double dodw = ( zo1 - zo0 ) / delta;
            const double deds = ( ze2 - ze0 ) / delta;
            const double dods = ( zo2 - zo0 ) / delta;

            // Find the determinate
            const double d = dedw * dods - deds * dodw;

            // Estimate the new solution, but don't change by more than 10% at a time to avoid convergence problems
            double dw = -1.0 * ( ( ze0 - z0e_target ) * dods - ( zo0 - z0o_target ) * deds ) / d;

            if( fabs( dw ) > 0.1 * w )
            {
                if( dw > 0.0 )
                    dw = 0.1 * w;
                else
                    dw = -0.1 * w;
            }

            w = fabs( w + dw );

            double ds = ( ( ze0 - z0e_target ) * dodw - ( zo0 - z0o_target ) * dedw ) / d;

            if( fabs( ds ) > 0.1 * s )
            {
                if( ds > 0.0 )
                    ds = 0.1 * s;
                else
                    ds = -0.1 * s;
            }

            s = fabs( s + ds );
        }
    }

    if( !done )
        return false;

    // Recompute with the final parameters
    SetParameter( TCP::PHYS_WIDTH, w );
    SetParameter( TCP::PHYS_S, s );
    Analyse();

    // Reset the impedances
    SetParameter( TCP::Z0_E, z0e_target );
    SetParameter( TCP::Z0_O, z0o_target );

    return true;
}


void COUPLED_STRIPLINE::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF_EVEN, e_eff_e );
    SetAnalysisResult( TCP::EPSILON_EFF_ODD, e_eff_o );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY_EVEN, unit_prop_delay_e );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY_ODD, unit_prop_delay_o );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0_E = GetParameter( TCP::Z0_E );
    const double Z0_O = GetParameter( TCP::Z0_O );
    const double Z_DIFF = GetParameter( TCP::Z_DIFF );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_E_invalid = !std::isfinite( Z0_E ) || Z0_E <= 0;
    const bool Z0_O_invalid = !std::isfinite( Z0_O ) || Z0_O <= 0;
    const bool Z_DIFF_invalid = !std::isfinite( Z_DIFF ) || Z_DIFF <= 0;
    const bool ANG_L_invalid = !std::isfinite( ang_l ) || ang_l < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0;

    SetAnalysisResult( TCP::Z0_E, Z0_E, Z0_E_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::Z0_O, Z0_O, Z0_O_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::Z_DIFF, Z_DIFF, Z_DIFF_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, ang_l, ANG_L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_S, S, S_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void COUPLED_STRIPLINE::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF_EVEN, e_eff_e );
    SetSynthesisResult( TCP::EPSILON_EFF_ODD, e_eff_o );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY_EVEN, unit_prop_delay_e );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY_ODD, unit_prop_delay_o );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0_E = GetParameter( TCP::Z0_E );
    const double Z0_O = GetParameter( TCP::Z0_O );
    const double Z_DIFF = GetParameter( TCP::Z_DIFF );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_E_invalid = !std::isfinite( Z0_E ) || Z0_E <= 0;
    const bool Z0_O_invalid = !std::isfinite( Z0_O ) || Z0_O <= 0;
    const bool Z_DIFF_invalid = !std::isfinite( Z_DIFF ) || Z_DIFF <= 0;
    const bool ANG_L_invalid = !std::isfinite( ang_l ) || ang_l < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0;

    SetSynthesisResult( TCP::Z0_E, Z0_E, Z0_E_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::Z0_O, Z0_O, Z0_O_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::Z_DIFF, Z_DIFF, Z_DIFF_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, ang_l, ANG_L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_S, S, S_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
}


void COUPLED_STRIPLINE::calcFringeCapacitances( const double h, const double t, const double er )
{
    // Reference [1], Eq. 2
    C_f_t_h = ( TC::E0 * er / M_PI )
              * ( ( 2.0 / ( 1.0 - t / h ) ) * log( ( 1.0 / ( 1.0 - t / h ) ) + 1.0 )
                  - ( 1.0 / ( 1.0 - t / h ) - 1.0 ) * log( ( 1.0 / pow( 1.0 - t / h, 2.0 ) ) - 1.0 ) );

    // Reference [2], Eq. 13
    C_f_0 = ( TC::E0 * er / M_PI ) * 2.0 * log( 2.0 );
}


void COUPLED_STRIPLINE::calcZeroThicknessCoupledImpedances( const double h, const double w, const double s,
                                                            const double er )
{
    // Reference [2], Eqs. 2 - 7
    const double k_e = tanh( M_PI * w / ( 2.0 * h ) ) * tanh( M_PI * ( w + s ) / ( 2.0 * h ) );
    const double k_o = tanh( M_PI * w / ( 2.0 * h ) ) * coth( M_PI * ( w + s ) / ( 2.0 * h ) );
    const double k_e_p = std::sqrt( 1 - std::pow( k_e, 2 ) );
    const double k_o_p = std::sqrt( 1 - std::pow( k_o, 2 ) );
    Z0_e_w_h_0_s_h = ( TC::ZF0 / ( 4.0 * std::sqrt( er ) ) )
                     * ( EllipticIntegral( k_e_p ).first / EllipticIntegral( k_e ).first );
    Z0_o_w_h_0_s_h = ( TC::ZF0 / ( 4.0 * std::sqrt( er ) ) )
                     * ( EllipticIntegral( k_o_p ).first / EllipticIntegral( k_o ).first );
}


void COUPLED_STRIPLINE::calcSingleStripImpedances()
{
    const double er = GetParameter( TCP::EPSILONR );
    const double h = GetParameter( TCP::H );
    const double w = GetParameter( TCP::PHYS_WIDTH );

    // Finite-thickness single strip impedance
    Z0_w_h_t_h = calcZ0SymmetricStripline();

    // Zero-thickness single strip impedance
    // Reference [1], Eqs. 5 - 6 (corrected for sqrt(e_r))
    const double k = sech( M_PI * w / ( 2.0 * h ) );
    const double k_p = tanh( M_PI * w / ( 2.0 * h ) );
    Z0_w_h_0 =
            ( TC::ZF0 / ( 4.0 * std::sqrt( er ) ) ) * ( EllipticIntegral( k ).first / EllipticIntegral( k_p ).first );
}


void COUPLED_STRIPLINE::calcZ0EvenMode()
{
    // Reference [2], Eq. 18
    const double Z_e =
            1.0 / ( ( 1.0 / Z0_w_h_t_h ) - ( C_f_t_h / C_f_0 ) * ( ( 1.0 / Z0_w_h_0 ) - ( 1.0 / Z0_e_w_h_0_s_h ) ) );
    SetParameter( TCP::Z0_E, Z_e );
}


void COUPLED_STRIPLINE::calcZ0OddMode( const double t, const double s )
{
    // Reference [2], Eq. 20
    const double Z_o_1 =
            1.0 / ( ( 1.0 / Z0_w_h_t_h ) + ( C_f_t_h / C_f_0 ) * ( ( 1.0 / Z0_o_w_h_0_s_h ) - ( 1.0 / Z0_w_h_0 ) ) );

    // Reference [2], Eq. 22
    const double Z_o_2 =
            1.0
            / ( ( 1.0 / Z0_o_w_h_0_s_h ) + ( ( 1.0 / Z0_w_h_t_h ) - ( 1.0 / Z0_w_h_0 ) )
                - ( 2.0 / TC::ZF0 ) * ( C_f_t_h / TC::E0 - C_f_0 / TC::E0 ) + ( 2.0 * t ) / ( TC::ZF0 * s ) );

    const double Z_o = s / t >= 5.0 ? Z_o_1 : Z_o_2;

    SetParameter( TCP::Z0_O, Z_o );
    SetParameter( TCP::Z_DIFF, 2.0 * Z_o );
}


void COUPLED_STRIPLINE::calcLosses()
{
}


void COUPLED_STRIPLINE::calcDielectrics()
{
    // We assume here that the dielectric is homogenous surrounding the strips - in this case, the odd or even modes
    // don't change the effective dielectric constant of the transmission mode. This would not be the case if the
    // dielectric were inhomogenous, as there is more electric field permeating the dielectric between the traces in
    // the odd mode compared to the even mode.
    const double e_r = GetParameter( TCP::EPSILONR );
    e_eff_e = e_r;
    e_eff_o = e_r;

    // Both modes have the same propagation delay
    const double unitPropDelay = UnitPropagationDelay( e_r );
    unit_prop_delay_e = unitPropDelay;
    unit_prop_delay_o = unitPropDelay;

    // Electrical length (in radians)
    const double v = TC::C0 / sqrt( e_r );
    const double lambda_g = v / GetParameter( TCP::FREQUENCY );
    ang_l = 2.0 * M_PI * GetParameter( TCP::PHYS_LEN ) / lambda_g;
}


double COUPLED_STRIPLINE::calcZ0SymmetricStripline()
{
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, GetParameter( TCP::EPSILONR ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::T, GetParameter( TCP::T ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::STRIPLINE_A, GetParameter( TCP::H ) / 2.0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::H, GetParameter( TCP::H ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, GetParameter( TCP::PHYS_LEN ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, GetParameter( TCP::FREQUENCY ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::TAND, 0.0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, GetParameter( TCP::PHYS_WIDTH ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, 0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, GetParameter( TCP::SIGMA ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::MURC, GetParameter( TCP::MURC ) );
    m_striplineCalc.Analyse();

    return m_striplineCalc.GetParameter( TCP::Z0 );
}
