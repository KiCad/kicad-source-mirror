/*
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2011 for Kicad: Jean-Pierre Charras
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

#include <transline_calculations/twistedpair.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


/**
 *  \f$ \theta = \arctan\left( T \cdot \pi \cdot D_{out} \right) \f$
 *
 * Where :
 * - \f$ \theta \f$ : pitch angle
 * - \f$ T \f$ : Number of twists per unit length
 * - \f$ D_{out} \f$ : Wire diameter with insulation
 *
 *  \f$ e_{eff} = e_{env} + \left( 0.25 + 0.0007 \cdot \theta^2 \right)\cdot\left(e_r-e_{env}\right) \f$
 *
 * Where :
 * - \f$ e_{env} \f$ : relative dielectric constant of air (or some other surrounding material)
 * - \f$ e_r \f$ : relative dielectric constant of the film insulation
 * - \f$ e_{eff} \f$ : effective relative dielectric constant
 *
 * \f$ Z_0 = \frac{Z_\mathrm{VACUUM}}{\pi \cdot \sqrt{e_{eff}}}\cosh^{-1}\left(\frac{D_{out}}{D_{in}}\right) \f$
 *
 * - \f$ Z_0 \f$ : differential line impedance
 * - \f$ Z_\mathrm{VACUUM} \f$ : vacuum impedance
 * - \f$ D_{in} \f$ : Wire diameter without insulation
 *
 * Reference: P. Lefferson, "Twisted Magnet Wire Transmission Line", IEEE Transactions on
 * Parts, Hybrids, and Packaging, vol. PHP-7, no. 4, pp. 148-154, Dec. 1971.
 * Twist angle from Fig. 2 caption: T = tan(theta)/(pi*D).
 * Z0 from eq. (1) and Fig. 1 caption (cosh^-1 form, 120/sqrt(eps_req) coefficient).
 * epsEff from eq. (3) combined with the empirical fit in eq. (7) (polyester film,
 * beta = 0.25 + 4e-4*theta^2) and eq. (8) (Teflon, beta = 0.25 + 1e-3*theta^2).  KiCad
 * uses a mid-range coefficient 7e-4 that approximates typical magnet-wire insulation.
 **/
void TWISTEDPAIR::Analyse()
{
    UpdateDielectricModel();

    const double twist = GetParameter( TCP::TWISTEDPAIR_TWIST );
    const double epsrEnv = GetParameter( TCP::TWISTEDPAIR_EPSILONR_ENV );
    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double epsr = GetDispersedEpsilonR( freq );
    const double tand = GetDispersedTanDelta( freq );

    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );

    // Lefferson defines theta in degrees for the 0.0007 * theta^2 term.  The KiCad
    // legacy implementation used radians, which made the twist contribution about
    // 3300x too small.  Convert to degrees before squaring.
    const double theta_rad = std::atan( twist * M_PI * Dout );
    const double theta_deg = theta_rad * ( 180.0 / M_PI );

    const double epsEff = epsrEnv + ( 0.25 + 0.0007 * theta_deg * theta_deg ) * ( epsr - epsrEnv );
    SetParameter( TCP::EPSILON_EFF, epsEff );

    const double z0 = ( TC::ZF0 / M_PI / std::sqrt( epsEff ) ) * std::acosh( Dout / Din );
    SetParameter( TCP::Z0, z0 );

    const double skinDepth = GetParameter( TCP::SKIN_DEPTH );
    const double sigma = GetParameter( TCP::SIGMA );

    // Thin-wire conductor and dielectric attenuation.  Lefferson 1971 does not cover loss;
    // these are the standard parallel-wire forms summarised in Wadell, "Transmission Line
    // Design Handbook", Artech House 1991, §3.2.3 (Twisted Pair).
    SetParameter( TCP::LOSS_CONDUCTOR, ( TC::LOG2DB / 2.0 ) * len / skinDepth / sigma / M_PI / z0
                                               / ( Din - skinDepth ) );

    SetParameter( TCP::LOSS_DIELECTRIC,
                  TC::LOG2DB * len * M_PI / TC::C0 * freq * std::sqrt( epsEff ) * tand );

    SetParameter( TCP::ANG_L, 2.0 * M_PI * len * std::sqrt( epsEff ) * freq / TC::C0 );

    SetParameter( TCP::UNIT_PROP_DELAY, UnitPropagationDelay( epsEff ) );
}


bool TWISTEDPAIR::Synthesize( const SYNTHESIZE_OPTS /* aOpts */ )
{
    // 1-D Newton on whichever diameter the UI flagged as the unknown.  MinimiseZ0Error1D
    // runs Analyse repeatedly until |Z0 - Z0_target| is below the base-class tolerance or
    // the iteration cap is reached, then recomputes PHYS_LEN from ANG_L to keep the
    // UI round-trip consistent with the legacy TRANSLINE::minimizeZ0Error1D contract.
    const TCP target =
            ( m_synthesizeTarget == TCP::PHYS_DIAM_IN ) ? TCP::PHYS_DIAM_IN : TCP::PHYS_DIAM_OUT;

    return MinimiseZ0Error1D( target, TCP::Z0, true );
}


void TWISTEDPAIR::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetAnalysisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetAnalysisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool Din_invalid = !std::isfinite( Din ) || Din <= 0.0;
    const bool Dout_invalid = !std::isfinite( Dout ) || Dout <= 0.0;
    const bool geometry_invalid = Din > Dout;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_DIAM_IN, Din,
                       ( Din_invalid || geometry_invalid ) ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_DIAM_OUT, Dout,
                       ( Dout_invalid || geometry_invalid ) ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void TWISTEDPAIR::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetSynthesisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetSynthesisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double len = GetParameter( TCP::PHYS_LEN );
    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool angL_invalid = !std::isfinite( angL ) || angL < 0;
    const bool len_invalid = !std::isfinite( len ) || len < 0;
    const bool Din_invalid = !std::isfinite( Din ) || Din <= 0.0;
    const bool Dout_invalid = !std::isfinite( Dout ) || Dout <= 0.0;
    const bool geometry_invalid = Din > Dout;

    const TRANSLINE_STATUS Din_status = ( m_synthesizeTarget == TCP::PHYS_DIAM_IN )
                                                ? ( Din_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK )
                                                : ( Din_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    const TRANSLINE_STATUS Dout_status = ( m_synthesizeTarget == TCP::PHYS_DIAM_OUT )
                                                 ? ( Dout_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK )
                                                 : ( Dout_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );

    SetSynthesisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_DIAM_IN, Din,
                        geometry_invalid ? ( m_synthesizeTarget == TCP::PHYS_DIAM_IN ? TRANSLINE_STATUS::TS_ERROR
                                                                                    : TRANSLINE_STATUS::WARNING )
                                         : Din_status );
    SetSynthesisResult( TCP::PHYS_DIAM_OUT, Dout,
                        geometry_invalid ? ( m_synthesizeTarget == TCP::PHYS_DIAM_OUT ? TRANSLINE_STATUS::TS_ERROR
                                                                                     : TRANSLINE_STATUS::WARNING )
                                         : Dout_status );
}
