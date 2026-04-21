/*
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
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

#include <algorithm>

#include <transline_calculations/coplanar.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


double COPLANAR::GetSoldermaskDeltaQ( double aWOverH, double aCOverH ) const
{
    if( aWOverH <= 0.0 || aCOverH <= 0.0 )
        return 0.0;

    // Start from the microstrip incremental filling factor (Wan-Hoorfar 2000 improvement
    // on Svacina 1992).  CPW shares the above-substrate geometry with microstrip but
    // only half of the total field lies in the upper half-space in the symmetric case
    // with no back metal, so the capturable fraction is half that of microstrip.  This
    // is a first-order empirical adaptation: a rigorous treatment would extend
    // Ghione-Naldi conformal mapping to include the overlay, which is not attempted
    // here.
    const double q2Coated = WanHoorfarQ2( aWOverH, 1.0 + aCOverH );
    const double q2Base = WanHoorfarQ2( aWOverH, 1.0 );
    const double microstripDelta = std::max( 0.0, q2Coated - q2Base );

    const bool fillsGaps = GetParameter( TCP::SOLDERMASK_FILLS_GAPS ) >= 0.5;
    const bool backMetal = hasBackMetal();

    // Upper half-space factor.  A symmetric CPW stores roughly half its capacitance in
    // the upper half-space above the substrate; a back-metal-backed CPW shifts more of
    // the capacitance to the substrate side so the upper-half share is smaller.
    const double halfSpace = backMetal ? 0.25 : 0.5;

    // Slot-coverage factor.  With the slots mask-filled the whole upper-half fringe sees
    // mask; with the slots air-filled the mask covers only the conductor strips and
    // captures a smaller share of the fringe energy.  0.4 is a coarse estimate of the
    // conductor-to-structure area ratio for typical 50 ohm CPW geometries and matches
    // the ordering in Bogatin 2018 Ch 7 that gaps-filled produces a larger Z0 drop than
    // gaps-air on identical geometry.
    const double slotCoverage = fillsGaps ? 1.0 : 0.4;

    return halfSpace * slotCoverage * microstripDelta;
}


void COPLANAR::Analyse()
{
    UpdateDielectricModel();

    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );

    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double S = GetParameter( TCP::PHYS_S );
    const double H = GetParameter( TCP::H );
    const double T = GetParameter( TCP::T );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double epsr = GetDispersedEpsilonR( freq );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double tand_substrate = GetDispersedTanDelta( freq );
    const double sigma = GetParameter( TCP::SIGMA );

    const bool backMetal = hasBackMetal();

    // Quasi-static conformal-mapping approximation.  K() is the complete elliptic integral of the
    // first kind; k1 is the ratio used for the coplanar half-plane solution.  The air-backed
    // expressions follow Ghione & Naldi, "Analytical Formulas for Coplanar Lines in Hybrid and
    // Monolithic MICs", Electronics Letters 20(4):179-181, Feb. 1984, eq. (1) and (3a); the
    // metal-backed k3 branch follows Ghione & Naldi, "Parameters of Coplanar Waveguides with
    // Lower Common Planes", Electronics Letters 19(18):734-735, Sept. 1983, eq. (8).  The
    // original CPW geometry is from Wen, "Coplanar Waveguide: A Surface Strip Transmission Line
    // Suitable for Nonreciprocal Gyromagnetic Device Applications", IEEE Trans. MTT
    // 17(12):1087-1090, Dec. 1969.
    const double k1 = W / ( W + S + S );
    const double kk1 = EllipticIntegral( k1 ).first;
    const double kpk1 = EllipticIntegral( std::sqrt( 1.0 - k1 * k1 ) ).first;
    const double q1 = kk1 / kpk1;

    double q3 = 0.0;
    double qz = 0.0;
    double er0 = 0.0;
    double zl_factor = 0.0;

    if( backMetal )
    {
        const double k3 = std::tanh( ( M_PI / 4.0 ) * ( W / H ) )
                          / std::tanh( ( M_PI / 4.0 ) * ( W + S + S ) / H );
        q3 = EllipticIntegral( k3 ).first / EllipticIntegral( std::sqrt( 1.0 - k3 * k3 ) ).first;
        qz = 1.0 / ( q1 + q3 );
        er0 = 1.0 + q3 * qz * ( epsr - 1.0 );
        zl_factor = TC::ZF0 / 2.0 * qz;
    }
    else
    {
        const double k2 = std::sinh( ( M_PI / 4.0 ) * ( W / H ) )
                          / std::sinh( ( M_PI / 4.0 ) * ( W + S + S ) / H );
        const double q2 = EllipticIntegral( k2 ).first / EllipticIntegral( std::sqrt( 1.0 - k2 * k2 ) ).first;
        er0 = 1.0 + ( epsr - 1.0 ) / 2.0 * q2 / q1;
        zl_factor = TC::ZF0 / 4.0 / q1;
    }

    // Finite-thickness correction.  Widens the effective centre conductor and narrows the slots
    // per Gupta, Garg, Bahl, Bhartia, "Microstrip Lines and Slotlines", 2nd ed., Artech House
    // 1996, eq. (7.98)-(7.100) (strip-thickness adjustment d = (T*1.25/pi)*(1 + ln(4*pi*W/T))).
    if( T > 0.0 )
    {
        const double d = ( T * 1.25 / M_PI ) * ( 1.0 + std::log( 4.0 * M_PI * W / T ) );
        const double se = S - d;
        const double We = W + d;
        const double ke = We / ( We + se + se );
        const double qe = EllipticIntegral( ke ).first / EllipticIntegral( std::sqrt( 1.0 - ke * ke ) ).first;

        if( backMetal )
        {
            qz = 1.0 / ( qe + q3 );
            er0 = 1.0 + q3 * qz * ( epsr - 1.0 );
            zl_factor = TC::ZF0 / 2.0 * qz;
        }
        else
        {
            zl_factor = TC::ZF0 / 4.0 / qe;
        }

        er0 = er0 - ( 0.7 * ( er0 - 1.0 ) * T / S ) / ( q1 + ( 0.7 * T / S ) );
    }

    // Apply the soldermask cover correction to the static er0 and scale the Z0
    // pre-factor so Z0 = zl_factor / sr_er_f remains self-consistent after dispersion.
    // SOLDERMASK_PRESENT == 0, zero thickness, and zero filling-factor each take the
    // no-op path out of ApplySoldermaskCorrection so the un-coated results are unchanged.
    double tand_eff = tand_substrate;
    const double uOverH = ( H > 0.0 ) ? ( W / H ) : 0.0;
    const auto [ er0_coated, tand_coated ] =
            ApplySoldermaskCorrection( er0, tand_substrate, epsr, uOverH, freq );

    if( er0_coated != er0 )
    {
        zl_factor *= std::sqrt( er0 / er0_coated );
        er0 = er0_coated;
        tand_eff = tand_coated;
    }

    const double sr_er = std::sqrt( epsr );
    const double sr_er0 = std::sqrt( er0 );

    // TE0-mode cutoff frequency and the dispersion factor G, used by the ad-hoc quadratic
    // correction that lifts er0 toward er at high frequency.
    const double fte = ( TC::C0 / 4.0 ) / ( H * std::sqrt( epsr - 1.0 ) );

    const double p = std::log( W / H );
    const double u = 0.54 - ( 0.64 - 0.015 * p ) * p;
    const double v = 0.43 - ( 0.86 - 0.54 * p ) * p;
    const double G = std::exp( u * std::log( W / S ) + v );

    // Skin-effect conductor loss from Wheeler's incremental-inductance rule (Wheeler,
    // "Formulas for the Skin Effect", Proc. IRE 30(9):412-424, Sept. 1942), first applied
    // to CPW by Owyang & Wu, "The Approximate Parameters of Slot Lines and Their Complement",
    // IRE Trans. Antennas and Propagation AP-6:49-55, Jan. 1958, extended to asymmetric
    // geometry in Ghione, "A CAD-Oriented Analytical Model for the Losses of General
    // Asymmetric Coplanar Lines...", IEEE Trans. MTT 41(9):1499-1510, Sept. 1993, and
    // corrected/generalised to multiconductor form in Ghione-Goano-Naldi, "A CAD-oriented
    // model for the ohmic losses of multiconductor coplanar lines in hybrid and monolithic
    // MIC's", GAAS'96 Symposium, Paris, 5-7 June 1996, paper 8A2, eq. (13) and (14).
    // Valid only for finite strip thickness T > 0.
    double ac = 0.0;

    if( T > 0.0 )
    {
        const double n = ( 1.0 - k1 ) * 8.0 * M_PI / ( T * ( 1.0 + k1 ) );
        const double a = W / 2.0;
        const double b = a + S;
        ac = ( M_PI + std::log( n * a ) ) / a + ( M_PI + std::log( n * b ) ) / b;
    }

    const double ac_factor = ac / ( 4.0 * TC::ZF0 * kk1 * kpk1 * ( 1.0 - k1 * k1 ) );
    const double ad_factor = ( epsr / ( epsr - 1.0 ) ) * tand_eff * M_PI / TC::C0;

    // Effective-permittivity dispersion from Gevorgian, Martinsson, Deleniv, Kollberg, Vendik,
    // "Simple and accurate dispersion expression for the effective dielectric constant of
    // coplanar waveguides", IEE Proc. Microwaves, Antennas and Propagation 144(2):145-148,
    // Apr. 1997, DOI 10.1049/ip-map:19970843.  The u/v coefficients and the (f/fte)^-1.8
    // scaling come from that paper's dispersion factor G.
    double sr_er_f = sr_er0;
    sr_er_f += ( sr_er - sr_er0 ) / ( 1.0 + G * std::pow( freq / fte, -1.8 ) );

    // Strip losses only (no radiation losses yet).  Neper-to-dB conversion via LOG2DB.
    SetParameter( TCP::LOSS_CONDUCTOR,
                  TC::LOG2DB * len * ac_factor * sr_er0 * std::sqrt( M_PI * TC::MU0 * freq / sigma ) );
    SetParameter( TCP::LOSS_DIELECTRIC,
                  TC::LOG2DB * len * ad_factor * freq * ( sr_er_f * sr_er_f - 1.0 ) / sr_er_f );

    SetParameter( TCP::ANG_L, 2.0 * M_PI * len * sr_er_f * freq / TC::C0 );
    SetParameter( TCP::EPSILON_EFF, sr_er_f * sr_er_f );
    SetParameter( TCP::Z0, zl_factor / sr_er_f );
    SetParameter( TCP::UNIT_PROP_DELAY, UnitPropagationDelay( sr_er_f * sr_er_f ) );
}


bool COPLANAR::Synthesize( const SYNTHESIZE_OPTS /* aOpts */ )
{
    const TRANSLINE_PARAMETERS target =
            ( m_synthesizeTarget == TCP::PHYS_S ) ? TCP::PHYS_S : TCP::PHYS_WIDTH;

    // Recompute PHYS_LEN from the requested ANG_L to preserve the legacy round-trip contract.
    return MinimiseZ0Error1D( target, TCP::Z0, true );
}


void COPLANAR::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY, GetParameter( TCP::UNIT_PROP_DELAY ) );
    SetAnalysisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetAnalysisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0.0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0.0;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_S, S, S_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void COPLANAR::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY, GetParameter( TCP::UNIT_PROP_DELAY ) );
    SetSynthesisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetSynthesisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );
    const double S = GetParameter( TCP::PHYS_S );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0.0;
    const bool S_invalid = !std::isfinite( S ) || S <= 0.0;

    const TRANSLINE_STATUS W_status =
            ( m_synthesizeTarget == TCP::PHYS_WIDTH )
                    ? ( W_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK )
                    : ( W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    const TRANSLINE_STATUS S_status =
            ( m_synthesizeTarget == TCP::PHYS_S )
                    ? ( S_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK )
                    : ( S_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );

    SetSynthesisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, W, W_status );
    SetSynthesisResult( TCP::PHYS_S, S, S_status );
}
