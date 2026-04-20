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
 * [3] B. C. Wadell, "Transmission Line Design Handbook," Artech House, Norwood, MA, 1991.  Sec. 3.6.3
 *    "Off-Center Stripline" (Eqs. 3.6.3.21 - 3.6.3.23) gives the image-method plus three-term
 *    correction for a single strip offset between two ground planes; applied here per mode to the
 *    Reference [2] coupled-stripline solver.
 */

#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/units.h>
#include <transline_calculations/units_scales.h>

#include <limits>
#include <utility>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


void COUPLED_STRIPLINE::Analyse()
{
    UpdateDielectricModel();

    const double f = GetParameter( TCP::FREQUENCY );
    const double rawEpsR = GetParameter( TCP::EPSILONR );
    const double rawTanD = GetParameter( TCP::TAND );

    // Overlay dispersed values so helpers reading EPSILONR / TAND via GetParameter
    // pick them up.  Raw inputs are restored before return.
    SetParameter( TCP::EPSILONR, GetDispersedEpsilonR( f ) );
    SetParameter( TCP::TAND, GetDispersedTanDelta( f ) );

    // Calculate skin depth
    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );

    // Get analysis parameters
    double       w = GetParameter( TCP::PHYS_WIDTH );
    double       t = GetParameter( TCP::T );
    double       s = GetParameter( TCP::PHYS_S );
    double       h = GetParameter( TCP::H );
    double       a = GetParameter( TCP::STRIPLINE_A );
    const double er = GetParameter( TCP::EPSILONR );

    // A non-positive value means STRIPLINE_A was never written by the UI; default to the centred
    // case so existing callers keep the exact pre-offset behaviour.
    if( a <= 0.0 )
        a = h / 2.0;

    calcOffsetZeroThicknessCoupledImpedances( h, a, w, s, t, er );

    // We've got the impedances now for an infinitely thin line
    if( t == 0.0 )
    {
        SetParameter( TCP::Z0_E, Z0_e_w_h_0_s_h );
        SetParameter( TCP::Z0_O, Z0_o_w_h_0_s_h );
    }
    else if( IsCenteredOffset( a, h ) )
    {
        calcSingleStripImpedances();
        calcFringeCapacitances( h, t, er );
        calcZ0EvenMode();
        calcZ0OddMode( t, s );
    }
    else if( !isOffsetWithinFiniteThicknessLimits( a, h, t ) )
    {
        // The Reference [1] finite-thickness fringe correction (Eq. 2) requires t < h on each
        // virtual plate spacing.  After the Reference [3] Eq. 3.6.3.22 image split the spacings
        // are 2a and 2(h - a), so the safe range is t/2 < a < h - t/2.  Outside it the fringe
        // formula divides by zero or takes log of a non-positive, producing NaN.  Surface the
        // failure as TS_ERROR via the NaN-driven status path rather than returning silently
        // bogus impedances.
        const double nan = std::numeric_limits<double>::quiet_NaN();
        SetParameter( TCP::Z0_E, nan );
        SetParameter( TCP::Z0_O, nan );
        SetParameter( TCP::Z_DIFF, nan );
    }
    else
    {
        // Reference [3] Eq. 3.6.3.22 image-method: combine two virtual centred finite-thickness
        // striplines at plate spacings 2a and 2(h - a) via parallel admittance.  Each branch runs
        // the centred-style fringe-capacitance correction internally.  Reference [3] Eq. 3.6.3.23
        // then applies the three-term correction that recovers ~2 percent agreement with rigorous
        // numerical solutions; we apply it per mode.  H is swapped around each inner pass because
        // the helpers read TCP::H directly.
        const auto [z0e_1, z0o_1] = calcOffsetVirtualBranch( 2.0 * a, w, s, t, er );
        const auto [z0e_2, z0o_2] = calcOffsetVirtualBranch( 2.0 * ( h - a ), w, s, t, er );

        SetParameter( TCP::H, h );

        const double z0e_image = 2.0 / ( 1.0 / z0e_1 + 1.0 / z0e_2 );
        const double z0o_image = 2.0 / ( 1.0 / z0o_1 + 1.0 / z0o_2 );

        const double z0e = applyOffsetCorrection( z0e_image, a, h, w, t, er );
        const double z0o = applyOffsetCorrection( z0o_image, a, h, w, t, er );

        SetParameter( TCP::Z0_E, z0e );
        SetParameter( TCP::Z0_O, z0o );
        SetParameter( TCP::Z_DIFF, 2.0 * z0o );

        // Restore the offset zero-thickness impedances in the member variables so downstream
        // consumers (e.g. diagnostics) see the corrected values rather than the last virtual
        // branch's output.
        calcOffsetZeroThicknessCoupledImpedances( h, a, w, s, t, er );
    }

    calcLosses();
    calcDielectrics();

    // Expose the two derived coupled-line figures-of-merit.  Z_comm = Z0e / 2 follows from
    // Pozar "Microwave Engineering" 4th ed. Sec. 7.6 Eqs. 7.68-7.69 (two Z0e lines in parallel
    // at equal voltage); k_c = (Z0e - Z0o) / (Z0e + Z0o) is Pozar Eq. 7.81.
    const double z0e = GetParameter( TCP::Z0_E );
    const double z0o = GetParameter( TCP::Z0_O );

    SetParameter( TCP::Z_COMM, 0.5 * z0e );

    if( ( z0e + z0o ) > 0.0 )
        SetParameter( TCP::COUPLING_K, ( z0e - z0o ) / ( z0e + z0o ) );
    else
        SetParameter( TCP::COUPLING_K, 0.0 );

    SetParameter( TCP::EPSILONR, rawEpsR );
    SetParameter( TCP::TAND, rawTanD );
}


bool COUPLED_STRIPLINE::Synthesize( const SYNTHESIZE_OPTS aOpts )
{
    // Opt-in (Z_diff, Z_comm) target translation.  The 1-D fix-W / fix-S paths only optimise
    // Z0_O so they cannot honour Z_comm; reject the option there.  When selected, translate
    // via Z_diff = 2 * Z0_O and Z_comm = Z0_E / 2 (Pozar "Microwave Engineering" 4th ed.
    // Sec. 7.6 Eqs. 7.68-7.71) and fall through to the joint 2-D solver below.
    if( aOpts == SYNTHESIZE_OPTS::FROM_ZDIFF_ZCOMM )
    {
        const double zDiffTarget = GetParameter( TCP::Z_DIFF );
        const double zCommTarget = GetParameter( TCP::Z_COMM );

        if( !( zDiffTarget > 0.0 ) || !( zCommTarget > 0.0 ) )
            return false;

        SetParameter( TCP::Z0_O, 0.5 * zDiffTarget );
        SetParameter( TCP::Z0_E, 2.0 * zCommTarget );
    }

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


void COUPLED_STRIPLINE::publishResults( const ResultSink aSink, const TRANSLINE_STATUS aImpedanceFailure,
                                        const TRANSLINE_STATUS aGeometryFailure )
{
    auto write = [this, aSink]( const TRANSLINE_PARAMETERS aParam, const double aValue,
                                const TRANSLINE_STATUS aStatus )
    {
        if( aSink == ResultSink::ANALYSIS )
            SetAnalysisResult( aParam, aValue, aStatus );
        else
            SetSynthesisResult( aParam, aValue, aStatus );
    };

    auto writeOk = [&write]( const TRANSLINE_PARAMETERS aParam, const double aValue )
    {
        write( aParam, aValue, TRANSLINE_STATUS::OK );
    };

    auto writeChecked = [&write]( const TRANSLINE_PARAMETERS aParam, const double aValue, const bool aPositive,
                                  const TRANSLINE_STATUS aFailureStatus )
    {
        const bool invalid = !std::isfinite( aValue ) || ( aPositive ? aValue <= 0.0 : aValue < 0.0 );
        write( aParam, aValue, invalid ? aFailureStatus : TRANSLINE_STATUS::OK );
    };

    writeOk( TCP::EPSILON_EFF_EVEN, e_eff_e );
    writeOk( TCP::EPSILON_EFF_ODD, e_eff_o );
    writeOk( TCP::UNIT_PROP_DELAY_EVEN, unit_prop_delay_e );
    writeOk( TCP::UNIT_PROP_DELAY_ODD, unit_prop_delay_o );
    writeOk( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );
    writeOk( TCP::ATTEN_COND_EVEN, GetParameter( TCP::ATTEN_COND_EVEN ) );
    writeOk( TCP::ATTEN_COND_ODD, GetParameter( TCP::ATTEN_COND_ODD ) );
    writeOk( TCP::ATTEN_DILECTRIC_EVEN, GetParameter( TCP::ATTEN_DILECTRIC_EVEN ) );
    writeOk( TCP::ATTEN_DILECTRIC_ODD, GetParameter( TCP::ATTEN_DILECTRIC_ODD ) );

    writeChecked( TCP::Z0_E, GetParameter( TCP::Z0_E ), true, aImpedanceFailure );
    writeChecked( TCP::Z0_O, GetParameter( TCP::Z0_O ), true, aImpedanceFailure );
    writeChecked( TCP::Z_DIFF, GetParameter( TCP::Z_DIFF ), true, aImpedanceFailure );
    writeOk( TCP::Z_COMM, GetParameter( TCP::Z_COMM ) );
    writeOk( TCP::COUPLING_K, GetParameter( TCP::COUPLING_K ) );
    writeChecked( TCP::ANG_L, ang_l, false, aImpedanceFailure );
    writeChecked( TCP::PHYS_WIDTH, GetParameter( TCP::PHYS_WIDTH ), true, aGeometryFailure );
    writeChecked( TCP::PHYS_LEN, GetParameter( TCP::PHYS_LEN ), false, aGeometryFailure );
    writeChecked( TCP::PHYS_S, GetParameter( TCP::PHYS_S ), true, aGeometryFailure );
}


void COUPLED_STRIPLINE::SetAnalysisResults()
{
    // Analysis treats invalid mode impedances as hard errors and invalid geometry as a warning,
    // since the user supplied geometry directly and the math layer simply could not produce a
    // physically meaningful Z out of it.
    publishResults( ResultSink::ANALYSIS, TRANSLINE_STATUS::TS_ERROR, TRANSLINE_STATUS::WARNING );
}


void COUPLED_STRIPLINE::SetSynthesisResults()
{
    // Synthesis flips the severity: geometry is the solver output so a non-finite W / L / S is
    // a hard error, while the user-supplied target impedances are demoted to warnings.
    publishResults( ResultSink::SYNTHESIS, TRANSLINE_STATUS::WARNING, TRANSLINE_STATUS::TS_ERROR );
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


bool COUPLED_STRIPLINE::IsCenteredOffset( const double a, const double h )
{
    // Tight relative tolerance so the centred fast path is taken only when the UI genuinely meant
    // "centred" and any physically meaningful offset routes through the image-method branch.
    return std::fabs( a - h / 2.0 ) <= h * 1e-9;
}


bool COUPLED_STRIPLINE::isOffsetWithinFiniteThicknessLimits( const double a, const double h, const double t )
{
    // Reference [3] splits the asymmetric stripline into two virtual symmetric striplines with
    // plate spacings 2a and 2(h - a).  The Reference [1] finite-thickness fringe formula
    // (Eq. 2) needs t < h on each, so the strip plane must sit strictly inside (t/2, h - t/2).
    // Reject zero and negative t-clearances rather than letting the fringe formula produce NaN.
    return ( a > 0.5 * t ) && ( a < h - 0.5 * t );
}


std::pair<double, double> COUPLED_STRIPLINE::calcOffsetVirtualBranch( const double aVirtualH, const double w,
                                                                      const double s, const double t,
                                                                      const double er )
{
    SetParameter( TCP::H, aVirtualH );
    calcZeroThicknessCoupledImpedances( aVirtualH, w, s, er );
    calcSingleStripImpedances();
    calcFringeCapacitances( aVirtualH, t, er );
    calcZ0EvenMode();
    calcZ0OddMode( t, s );

    return { GetParameter( TCP::Z0_E ), GetParameter( TCP::Z0_O ) };
}


void COUPLED_STRIPLINE::calcOffsetZeroThicknessCoupledImpedances( const double h, const double a, const double w,
                                                                  const double s, const double t, const double er )
{
    // Centred short-circuit so the default-input path is bit-identical to the pre-offset code.
    if( IsCenteredOffset( a, h ) )
    {
        calcZeroThicknessCoupledImpedances( h, w, s, er );
        return;
    }

    // Reference [3] Eq. 3.6.3.22 image-method: represent the offset-strip geometry as the parallel
    // combination of two virtual centred striplines at plate spacings 2a and 2(h - a).  Each
    // virtual stripline carries half the total capacitance-to-ground per unit length, so the
    // combined Y = (Y1 + Y2) / 2 collapses to Y_centred at a = h/2 and writes the correct 1/Z
    // offset elsewhere.
    calcZeroThicknessCoupledImpedances( 2.0 * a, w, s, er );
    const double z0e_1 = Z0_e_w_h_0_s_h;
    const double z0o_1 = Z0_o_w_h_0_s_h;

    calcZeroThicknessCoupledImpedances( 2.0 * ( h - a ), w, s, er );
    const double z0e_2 = Z0_e_w_h_0_s_h;
    const double z0o_2 = Z0_o_w_h_0_s_h;

    const double z0e_image = 2.0 / ( 1.0 / z0e_1 + 1.0 / z0e_2 );
    const double z0o_image = 2.0 / ( 1.0 / z0o_1 + 1.0 / z0o_2 );

    // Reference [3] Eq. 3.6.3.23 three-term correction.  Zero thickness (t = 0) still carries the
    // (t + w)^2.9 width factor so the correction is driven purely by strip width when no
    // finite-thickness branch is active.  Reduces to zero at a = h/2 so the centred call above
    // stays bit-exact.
    Z0_e_w_h_0_s_h = applyOffsetCorrection( z0e_image, a, h, w, t, er );
    Z0_o_w_h_0_s_h = applyOffsetCorrection( z0o_image, a, h, w, t, er );
}


double COUPLED_STRIPLINE::applyOffsetCorrection( const double aZImage, const double aOffset, const double aPlateSpacing,
                                                 const double aWidth, const double aThickness, const double aEr )
{
    // Reference [3] Eq. 3.6.3.23:
    //   delta_Z_air = (0.26 * pi / 8) * Z_air^2 * |0.5 - h1 / h|^2.2 * ((t + w) / h)^2.9
    //   Z = (1 / sqrt(er)) * (Z_air - delta_Z_air)
    //
    // Wadell's derivation is in air; express in dielectric space via Z_air = sqrt(er) * Z.
    // Substituting and factoring out Z_image:
    //   Z_corrected = Z_image * (1 - (0.26 * pi / 8) * sqrt(er) * Z_image * |0.5 - a/h|^2.2
    //                                 * ((t + w) / h)^2.9)
    //
    // The position factor uses a/h (strip centreline as a fraction of total stack), so |0.5 - a/h|
    // is the offset fraction from the centre.  Absolute value handles strips above or below centre
    // symmetrically; the 2.2 exponent ensures a smooth zero at a = h/2.
    const double position = std::fabs( 0.5 - aOffset / aPlateSpacing );
    const double positionFactor = std::pow( position, 2.2 );
    const double widthFactor = std::pow( ( aThickness + aWidth ) / aPlateSpacing, 2.9 );
    const double correction = ( 0.26 * M_PI / 8.0 ) * std::sqrt( aEr ) * aZImage * positionFactor * widthFactor;

    return aZImage * ( 1.0 - correction );
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
    const double length = GetParameter( TCP::PHYS_LEN );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double er = GetParameter( TCP::EPSILONR );
    const double tand = GetParameter( TCP::TAND );

    // Dielectric loss for a homogeneous TEM stripline. The same α_d applies to both even and odd
    // modes because the dielectric fills the whole cross-section, so εr_eff does not change with
    // mode excitation. α_d = π · f · √εr · tan δ / c (Np/m). Convert to dB via LOG2DB.
    // Reference Pozar, "Microwave Engineering" 4th ed., §3.1 homogeneous-TEM loss.
    const double alpha_d_dB_per_m = TC::LOG2DB * ( M_PI / TC::C0 ) * freq * std::sqrt( er ) * tand;
    const double atten_d = alpha_d_dB_per_m * length;

    SetParameter( TCP::ATTEN_DILECTRIC_EVEN, atten_d );
    SetParameter( TCP::ATTEN_DILECTRIC_ODD, atten_d );

    // Conductor loss via the single-stripline incremental-inductance formulation. STRIPLINE::Analyse
    // calls lineImpedance() twice (strip to top plane, strip to bottom plane) and sums per-unit-length
    // attenuations into LOSS_CONDUCTOR (already in dB after multiplication by PHYS_LEN). Reuse it
    // rather than duplicate the incremental-inductance algebra.
    // Reference Wheeler, "Formulas for the Skin Effect", Proc. IRE 30(9):412-424, Sept. 1942
    // (incremental-inductance rule).
    m_striplineCalc.SetParameter( TCP::EPSILONR, er );
    m_striplineCalc.SetParameter( TCP::T, GetParameter( TCP::T ) );
    m_striplineCalc.SetParameter( TCP::STRIPLINE_A, GetParameter( TCP::H ) / 2.0 );
    m_striplineCalc.SetParameter( TCP::H, GetParameter( TCP::H ) );
    m_striplineCalc.SetParameter( TCP::PHYS_LEN, length );
    m_striplineCalc.SetParameter( TCP::FREQUENCY, freq );
    m_striplineCalc.SetParameter( TCP::TAND, 0.0 );
    m_striplineCalc.SetParameter( TCP::PHYS_WIDTH, GetParameter( TCP::PHYS_WIDTH ) );
    m_striplineCalc.SetParameter( TCP::ANG_L, 0.0 );
    m_striplineCalc.SetParameter( TCP::SIGMA, GetParameter( TCP::SIGMA ) );
    m_striplineCalc.SetParameter( TCP::MURC, GetParameter( TCP::MURC ) );
    m_striplineCalc.Analyse();

    const double loss_single = m_striplineCalc.GetParameter( TCP::LOSS_CONDUCTOR );
    const double z0_single = m_striplineCalc.GetParameter( TCP::Z0 );
    const double z0_e = GetParameter( TCP::Z0_E );
    const double z0_o = GetParameter( TCP::Z0_O );

    // Scale single-strip conductor attenuation by Z0_single / Z0_mode. The per-unit-length conductor
    // attenuation in the incremental-inductance model is inversely proportional to Z0 of the mode,
    // so a line of the same geometry but driven at Z0_mode dissipates loss_single · Z0_single / Z0_mode.
    double atten_c_e = 0.0;
    double atten_c_o = 0.0;

    if( z0_e > 0.0 && std::isfinite( z0_e ) )
        atten_c_e = loss_single * ( z0_single / z0_e );

    if( z0_o > 0.0 && std::isfinite( z0_o ) )
        atten_c_o = loss_single * ( z0_single / z0_o );

    SetParameter( TCP::ATTEN_COND_EVEN, atten_c_e );
    SetParameter( TCP::ATTEN_COND_ODD, atten_c_o );
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
