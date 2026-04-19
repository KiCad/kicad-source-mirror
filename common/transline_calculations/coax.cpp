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

#include <cstdio>

#include <transline_calculations/coax.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


// Formulas throughout this file follow Pozar, "Microwave Engineering", 4th ed.,
// Wiley 2012, §2.2 (TEM coaxial Z0, Table 2.1) and §3.5 (higher-order TE/TM modes).
// Collin, "Foundations for Microwave Engineering", 2nd ed., McGraw-Hill 1992, §3.5
// gives identical derivations.

double COAX::AlphaD() const
{
    // Homogeneous-TEM dielectric loss alpha_d = pi*f*sqrt(eps_r)*tan(delta)/c (Pozar §2.7).
    double ad = ( M_PI / TC::C0 ) * GetParameter( TCP::FREQUENCY ) * std::sqrt( GetParameter( TCP::EPSILONR ) )
                * GetParameter( TCP::TAND );
    return ad * TC::LOG2DB;
}


double COAX::AlphaC() const
{
    // Coax conductor loss alpha_c = (Rs/(2*eta))*(1/a + 1/b)/ln(b/a), from R = (Rs/(2*pi))*
    // (1/a + 1/b) in Pozar Table 2.1 divided by 2*Z0.  Surface resistance Rs = sqrt(pi*f*
    // mu_c*mu_0/sigma).  Expressed below in diameter form.
    const double Rs = std::sqrt( M_PI * GetParameter( TCP::FREQUENCY ) * GetParameter( TCP::MURC ) * TC::MU0
                                 / GetParameter( TCP::SIGMA ) );

    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );

    double ac = std::sqrt( GetParameter( TCP::EPSILONR ) ) * ( ( 1.0 / Din ) + ( 1.0 / Dout ) )
                / std::log( Dout / Din ) * ( Rs / TC::ZF0 );
    return ac * TC::LOG2DB;
}


void COAX::UpdateModeCutoffs()
{
    // Higher-order mode cutoffs.  Exact kc values are roots of the Bessel cross-product
    // transcendental (Pozar eq. 3.159); the forms below use the standard kc ~ 2/(a+b)
    // approximation for TE11 quoted just below Pozar eq. (3.159) on p. 132.
    // Wave velocity in the filling medium is c / sqrt(eps_r * mu_r), so every cutoff scales
    // by sqrt(eps_r * mu_r) rather than sqrt(eps_r) alone.
    // TE11 (dominant non-TEM): f_c = 2*c / (pi * sqrt(eps_r * mu_r) * (Din + Dout))
    // TM_0m:                   f_c = m*c / (sqrt(eps_r * mu_r) * (Dout - Din))
    // TE_1m (m>=2):            approx f_c(TE11) + (m-1)*c / (sqrt(eps_r * mu_r) * (Dout - Din))
    const double epsr = GetParameter( TCP::EPSILONR );
    const double mur = GetParameter( TCP::MUR );
    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double sqrtMuEr = std::sqrt( epsr * mur );

    const double fc_TE11 = ( 2.0 * TC::C0 ) / ( M_PI * sqrtMuEr * ( Din + Dout ) );
    const double fc_step = TC::C0 / ( sqrtMuEr * ( Dout - Din ) );

    SetParameter( TCP::CUTOFF_FREQUENCY, fc_TE11 );

    m_teModes.clear();
    m_tmModes.clear();

    if( fc_TE11 <= freq )
    {
        m_teModes = "H(1,1) ";

        for( int m = 2; m < 10; ++m )
        {
            const double fc = fc_TE11 + static_cast<double>( m - 1 ) * fc_step;

            if( fc > freq )
                break;

            char buf[32];
            std::snprintf( buf, sizeof( buf ), "H(1,%d) ", m );
            m_teModes += buf;
        }
    }

    for( int m = 1; m < 10; ++m )
    {
        const double fc = static_cast<double>( m ) * fc_step;

        if( fc > freq )
            break;

        char buf[32];
        std::snprintf( buf, sizeof( buf ), "E(0,%d) ", m );
        m_tmModes += buf;
    }
}


void COAX::Analyse()
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

    const double Din = GetParameter( TCP::PHYS_DIAM_IN );
    const double Dout = GetParameter( TCP::PHYS_DIAM_OUT );
    const double epsr = GetParameter( TCP::EPSILONR );
    const double mur = GetParameter( TCP::MUR );
    const double len = GetParameter( TCP::PHYS_LEN );

    // Coax Z0 = eta*ln(b/a)/(2*pi) with eta = eta_0/sqrt(eps_r) (Pozar eq. 2.32).
    SetParameter( TCP::Z0, ( TC::ZF0 / ( 2.0 * M_PI * std::sqrt( epsr ) ) ) * std::log( Dout / Din ) );

    const double lambda_g = ( TC::C0 / freq ) / std::sqrt( epsr * mur );

    SetParameter( TCP::ANG_L, ( 2.0 * M_PI * len ) / lambda_g );

    SetParameter( TCP::LOSS_DIELECTRIC, AlphaD() * len );
    SetParameter( TCP::LOSS_CONDUCTOR, AlphaC() * len );

    UpdateModeCutoffs();

    SetParameter( TCP::EPSILONR, rawEpsR );
    SetParameter( TCP::TAND, rawTanD );
}


bool COAX::Synthesize( const SYNTHESIZE_OPTS /* aOpts */ )
{
    // Fit the DS model before sizing so the follow-up Analyse() sees the same eps_r.
    UpdateDielectricModel();

    const double mur = GetParameter( TCP::MUR );
    const double freq = GetParameter( TCP::FREQUENCY );
    const double Z0 = GetParameter( TCP::Z0 );
    const double angL = GetParameter( TCP::ANG_L );
    const double epsr = GetDispersedEpsilonR( freq );

    const double k = Z0 * std::sqrt( epsr ) / TC::ZF0 * 2.0 * M_PI;

    if( m_synthesizeTarget == TCP::PHYS_DIAM_IN )
        SetParameter( TCP::PHYS_DIAM_IN, GetParameter( TCP::PHYS_DIAM_OUT ) / std::exp( k ) );
    else if( m_synthesizeTarget == TCP::PHYS_DIAM_OUT )
        SetParameter( TCP::PHYS_DIAM_OUT, GetParameter( TCP::PHYS_DIAM_IN ) * std::exp( k ) );

    const double lambda_g = ( TC::C0 / freq ) / std::sqrt( epsr * mur );
    SetParameter( TCP::PHYS_LEN, ( lambda_g * angL ) / ( 2.0 * M_PI ) );

    // Re-run analysis so losses, skin depth, and mode cutoffs reflect the synthesized geometry.
    Analyse();

    // Restore user-supplied Z0 and ANG_L targets (Analyse overwrites them from the new geometry).
    SetParameter( TCP::Z0, Z0 );
    SetParameter( TCP::ANG_L, angL );

    return true;
}


void COAX::SetAnalysisResults()
{
    SetAnalysisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetAnalysisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetAnalysisResult( TCP::EPSILONR, GetParameter( TCP::EPSILONR ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );
    SetAnalysisResult( TCP::CUTOFF_FREQUENCY, GetParameter( TCP::CUTOFF_FREQUENCY ) );

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
    const bool geometry_invalid = Din >= Dout;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, angL, angL_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, len, len_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_DIAM_IN, Din,
                       ( Din_invalid || geometry_invalid ) ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_DIAM_OUT, Dout,
                       ( Dout_invalid || geometry_invalid ) ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void COAX::SetSynthesisResults()
{
    SetSynthesisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetSynthesisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetSynthesisResult( TCP::EPSILONR, GetParameter( TCP::EPSILONR ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );
    SetSynthesisResult( TCP::CUTOFF_FREQUENCY, GetParameter( TCP::CUTOFF_FREQUENCY ) );

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
    const bool geometry_invalid = Din >= Dout;

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
