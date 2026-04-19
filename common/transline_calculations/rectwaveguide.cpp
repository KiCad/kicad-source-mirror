/*
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2015 jean-pierre.charras
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
 */

#include <cstdio>
#include <cstring>

#include <transline_calculations/rectwaveguide.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


// Formulas throughout this file follow Pozar, "Microwave Engineering", 4th ed.,
// Wiley 2012, §3.3 (rectangular waveguide, eq. 3.84 cutoff, Table 3.2 summary).
// Ramo, Whinnery, Van Duzer, "Fields and Waves in Communication Electronics", 3rd
// ed., Wiley 1994, chapter 8 gives equivalent derivations; Collin, "Foundations for
// Microwave Engineering", 2nd ed., McGraw-Hill 1992, §3.3 tabulates the same
// attenuation sums.  Open-access cross-reference: Orfanidis, "Electromagnetic Waves
// and Antennas", Ch. 9, eq. (9.8.1), freely available at
// https://eceweb1.rutgers.edu/~orfanidi/ewa/.

double RECTWAVEGUIDE::KvalSquare() const
{
    // Free-medium wavenumber squared, k^2 = omega^2*mu*eps (Pozar §3.3).
    const double kval = 2.0 * M_PI * GetParameter( TCP::FREQUENCY )
                        * std::sqrt( GetParameter( TCP::MUR ) * GetParameter( TCP::EPSILONR ) ) / TC::C0;
    return kval * kval;
}


double RECTWAVEGUIDE::KcSquare( const int aM, const int aN ) const
{
    // TE(m,n)/TM(m,n) cutoff wavenumber, kc^2 = (m*pi/a)^2 + (n*pi/b)^2 (Pozar eq. 3.84).
    const double a = GetParameter( TCP::PHYS_WIDTH );
    const double b = GetParameter( TCP::PHYS_S );
    return std::pow( aM * M_PI / a, 2.0 ) + std::pow( aN * M_PI / b, 2.0 );
}


double RECTWAVEGUIDE::Fc( const int aM, const int aN ) const
{
    // Cutoff frequency, fc = kc / (2*pi*sqrt(mu*eps)) (Pozar eq. 3.84).
    return std::sqrt( KcSquare( aM, aN ) / GetParameter( TCP::MUR ) / GetParameter( TCP::EPSILONR ) ) * TC::C0
           / ( 2.0 * M_PI );
}


double RECTWAVEGUIDE::AlphaC() const
{
    // Per-mode conductor attenuation from the surface-resistance integrals given in
    // Pozar §3.3 (TE: eq. form of Pozar table 3.2; TM: analogous closed form).  Ramo,
    // Whinnery, Van Duzer §8.7 derives the same integrals from the tangential H field.
    const double a = GetParameter( TCP::PHYS_WIDTH );
    const double b = GetParameter( TCP::PHYS_S );
    const double f = GetParameter( TCP::FREQUENCY );
    const double murc = GetParameter( TCP::MURC );
    const double sigma = GetParameter( TCP::SIGMA );

    const double Rs = std::sqrt( M_PI * f * murc * TC::MU0 / sigma );
    double ac = 0.0;

    const int mmax = static_cast<int>( std::floor( f / Fc( 1, 0 ) ) );
    const int nmax = mmax;

    // TE(m, n) modes.
    for( int n = 0; n <= nmax; ++n )
    {
        for( int m = 1; m <= mmax; ++m )
        {
            const double f_c = Fc( m, n );

            if( f <= f_c )
                continue;

            if( n == 0 )
            {
                ac += ( Rs / ( b * TC::ZF0 * std::sqrt( 1.0 - std::pow( f_c / f, 2.0 ) ) ) )
                      * ( 1.0 + ( ( 2.0 * b / a ) * std::pow( f_c / f, 2.0 ) ) );
            }
            else
            {
                ac += ( ( 2.0 * Rs ) / ( b * TC::ZF0 * std::sqrt( 1.0 - std::pow( f_c / f, 2.0 ) ) ) )
                      * ( ( ( 1.0 + ( b / a ) ) * std::pow( f_c / f, 2.0 ) )
                          + ( ( 1.0 - std::pow( f_c / f, 2.0 ) )
                              * ( ( ( b / a ) * ( ( ( b / a ) * std::pow( m, 2.0 ) ) + std::pow( n, 2.0 ) ) )
                                  / ( std::pow( b * m / a, 2.0 ) + std::pow( n, 2.0 ) ) ) ) );
            }
        }
    }

    // TM(m, n) modes.
    for( int n = 1; n <= nmax; ++n )
    {
        for( int m = 1; m <= mmax; ++m )
        {
            const double f_c = Fc( m, n );

            if( f <= f_c )
                continue;

            ac += ( ( 2.0 * Rs ) / ( b * TC::ZF0 * std::sqrt( 1.0 - std::pow( f_c / f, 2.0 ) ) ) )
                  * ( ( ( std::pow( m, 2.0 ) * std::pow( b / a, 3.0 ) ) + std::pow( n, 2.0 ) )
                      / ( std::pow( m * b / a, 2.0 ) + std::pow( n, 2.0 ) ) );
        }
    }

    return ac * TC::LOG2DB;
}


double RECTWAVEGUIDE::AlphaCCutoff() const
{
    double acc = std::sqrt( KcSquare( 1, 0 ) - KvalSquare() );
    acc = TC::LOG2DB * acc;
    return acc;
}


double RECTWAVEGUIDE::AlphaD() const
{
    const double k_square = KvalSquare();
    const double beta = std::sqrt( k_square - KcSquare( 1, 0 ) );

    double ad = ( k_square * GetParameter( TCP::TAND ) ) / ( 2.0 * beta );
    ad = ad * TC::LOG2DB;
    return ad;
}


void RECTWAVEGUIDE::UpdateModeStrings()
{
    // Walks a bounded (m, n) grid and emits every mode whose cutoff lies below the current
    // operating frequency.  Buffer length caps the display string; excess modes collapse to
    // an ellipsis.  Format matches the legacy pcb_calculator output: H(m,n) for TE, E(m,n)
    // for TM.
    constexpr int MAX_INDEX = 6;
    constexpr std::size_t MAX_STR = 128;
    char buf[MAX_STR];
    char token[32];

    const double freq = GetParameter( TCP::FREQUENCY );

    m_teModes.clear();
    m_tmModes.clear();

    if( freq >= Fc( 1, 0 ) )
    {
        buf[0] = '\0';
        bool truncated = false;

        for( int m = 0; m <= MAX_INDEX && !truncated; ++m )
        {
            for( int n = 0; n <= MAX_INDEX && !truncated; ++n )
            {
                if( m == 0 && n == 0 )
                    continue;

                if( freq >= Fc( m, n ) )
                {
                    std::snprintf( token, sizeof( token ), "H(%d,%d) ", m, n );

                    if( std::strlen( buf ) + std::strlen( token ) + 5 < MAX_STR )
                    {
                        std::strcat( buf, token );
                    }
                    else
                    {
                        std::strcat( buf, "..." );
                        truncated = true;
                    }
                }
            }
        }

        m_teModes = buf;
    }

    if( freq >= Fc( 1, 1 ) )
    {
        buf[0] = '\0';
        bool truncated = false;

        for( int m = 1; m <= MAX_INDEX && !truncated; ++m )
        {
            for( int n = 1; n <= MAX_INDEX && !truncated; ++n )
            {
                if( freq >= Fc( m, n ) )
                {
                    std::snprintf( token, sizeof( token ), "E(%d,%d) ", m, n );

                    if( std::strlen( buf ) + std::strlen( token ) + 5 < MAX_STR )
                    {
                        std::strcat( buf, token );
                    }
                    else
                    {
                        std::strcat( buf, "..." );
                        truncated = true;
                    }
                }
            }
        }

        m_tmModes = buf;
    }
}


void RECTWAVEGUIDE::Analyse()
{
    UpdateDielectricModel();

    const double freq = GetParameter( TCP::FREQUENCY );
    const double rawEpsR = GetParameter( TCP::EPSILONR );
    const double rawTanD = GetParameter( TCP::TAND );

    // Overlay dispersed values so helpers reading EPSILONR / TAND via GetParameter
    // pick them up.  Raw inputs are restored before return.
    SetParameter( TCP::EPSILONR, GetDispersedEpsilonR( freq ) );
    SetParameter( TCP::TAND, GetDispersedTanDelta( freq ) );

    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );
    SetParameter( TCP::CUTOFF_FREQUENCY, Fc( 1, 0 ) );

    const double len = GetParameter( TCP::PHYS_LEN );
    const double k_square = KvalSquare();
    const double kc10_square = KcSquare( 1, 0 );

    if( kc10_square <= k_square )
    {
        // Propagating TE10 mode.  Z0 uses the fictive-voltage / fictive-current definition,
        // i.e. ZF0 * sqrt(mur/epsr) / sqrt(1 - (fc/f)^2).  This is the definition the
        // pcb_calculator UI has always reported as the analysis output.
        const double fc10 = Fc( 1, 0 );
        SetParameter( TCP::Z0,
                      TC::ZF0 * std::sqrt( GetParameter( TCP::MUR ) / GetParameter( TCP::EPSILONR ) )
                              / std::sqrt( 1.0 - std::pow( fc10 / freq, 2.0 ) ) );

        const double lambda_g = 2.0 * M_PI / std::sqrt( k_square - kc10_square );
        SetParameter( TCP::ANG_L, 2.0 * M_PI * len / lambda_g );

        SetParameter( TCP::LOSS_CONDUCTOR, AlphaC() * len );
        SetParameter( TCP::LOSS_DIELECTRIC, AlphaD() * len );
        SetParameter( TCP::EPSILON_EFF, 1.0 - std::pow( fc10 / freq, 2.0 ) );
    }
    else
    {
        // Evanescent region.  No propagating energy; conductor "loss" is the bulk
        // attenuation of the cutoff waveguide, dielectric loss is undefined.
        SetParameter( TCP::Z0, 0.0 );
        SetParameter( TCP::ANG_L, 0.0 );
        SetParameter( TCP::EPSILON_EFF, 0.0 );
        SetParameter( TCP::LOSS_DIELECTRIC, 0.0 );
        SetParameter( TCP::LOSS_CONDUCTOR, AlphaCCutoff() * len );
    }

    UpdateModeStrings();

    SetParameter( TCP::EPSILONR, rawEpsR );
    SetParameter( TCP::TAND, rawTanD );
}


bool RECTWAVEGUIDE::Synthesize( const SYNTHESIZE_OPTS /* aOpts */ )
{
    // Closed-form inverse of the TE10 Z0 expression.  The narrow dimension b does not
    // appear in either Z0(a) or f_c10(a), so only a is solved for.  Caller supplies b
    // as a free input (typically a / 2 to push TE01 cutoff above the operating band).
    UpdateDielectricModel();

    const double mur = GetParameter( TCP::MUR );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double rawEpsR = GetParameter( TCP::EPSILONR );
    const double rawTanD = GetParameter( TCP::TAND );
    const double epsr = GetDispersedEpsilonR( freq );

    SetParameter( TCP::EPSILONR, epsr );
    SetParameter( TCP::TAND, GetDispersedTanDelta( freq ) );

    const double eta = TC::ZF0 * std::sqrt( mur / epsr );

    SetParameter( TCP::PHYS_WIDTH,
                  TC::C0 / ( std::sqrt( mur * epsr ) * 2.0 * freq * std::sqrt( 1.0 - std::pow( eta / Z0, 2.0 ) ) ) );

    const double k_square = KvalSquare();
    const double kc10_square = KcSquare( 1, 0 );
    const double beta = std::sqrt( k_square - kc10_square );
    const double lambda_g = 2.0 * M_PI / beta;

    SetParameter( TCP::PHYS_LEN, ( angL * lambda_g ) / ( 2.0 * M_PI ) );

    if( kc10_square <= k_square )
    {
        const double len = GetParameter( TCP::PHYS_LEN );
        SetParameter( TCP::LOSS_CONDUCTOR, AlphaC() * len );
        SetParameter( TCP::LOSS_DIELECTRIC, AlphaD() * len );
        SetParameter( TCP::EPSILON_EFF, 1.0 - std::pow( Fc( 1, 0 ) / freq, 2.0 ) );
    }
    else
    {
        // Synthesized geometry is below cutoff: warn via zeroed outputs.  The legacy UI
        // did the same and marked Z0 / ANG_L with the warning status.
        SetParameter( TCP::EPSILON_EFF, 0.0 );
        SetParameter( TCP::LOSS_DIELECTRIC, 0.0 );
        SetParameter( TCP::LOSS_CONDUCTOR, AlphaCCutoff() * GetParameter( TCP::PHYS_LEN ) );
    }

    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );
    SetParameter( TCP::CUTOFF_FREQUENCY, Fc( 1, 0 ) );

    UpdateModeStrings();

    // Restore user-supplied Z0 and ANG_L targets so the result map reports what was asked
    // for, matching the Synthesize contract used by the other migrated calculators.
    SetParameter( TCP::Z0, Z0 );
    SetParameter( TCP::ANG_L, angL );

    SetParameter( TCP::EPSILONR, rawEpsR );
    SetParameter( TCP::TAND, rawTanD );

    return std::isfinite( GetParameter( TCP::PHYS_WIDTH ) ) && GetParameter( TCP::PHYS_WIDTH ) > 0.0;
}


void RECTWAVEGUIDE::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetAnalysisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetAnalysisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetAnalysisResult( TCP::CUTOFF_FREQUENCY, GetParameter( TCP::CUTOFF_FREQUENCY ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double a = GetParameter( TCP::PHYS_WIDTH );
    const double b = GetParameter( TCP::PHYS_S );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool a_invalid = !std::isfinite( a ) || a <= 0.0;
    const bool b_invalid = !std::isfinite( b ) || b <= 0.0;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, a, a_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_S, b, b_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void RECTWAVEGUIDE::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetSynthesisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetSynthesisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetSynthesisResult( TCP::CUTOFF_FREQUENCY, GetParameter( TCP::CUTOFF_FREQUENCY ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double a = GetParameter( TCP::PHYS_WIDTH );
    const double b = GetParameter( TCP::PHYS_S );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool a_invalid = !std::isfinite( a ) || a <= 0.0;
    const bool b_invalid = !std::isfinite( b ) || b <= 0.0;

    const TRANSLINE_STATUS a_status = ( m_synthesizeTarget == TCP::PHYS_WIDTH )
                                              ? ( a_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK )
                                              : ( a_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );

    SetSynthesisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, a, a_status );
    SetSynthesisResult( TCP::PHYS_S, b, b_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}
