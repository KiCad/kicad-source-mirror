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

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <transline_calculations/units.h>
#include <transline_calculations/transline_calculation_base.h>


using TCP = TRANSLINE_PARAMETERS;
namespace TC = TRANSLINE_CALCULATIONS;


void TRANSLINE_CALCULATION_BASE::InitProperties( const std::initializer_list<TRANSLINE_PARAMETERS>& aParams )
{
    for( const TRANSLINE_PARAMETERS& param : aParams )
        m_parameters[param] = 0.0;
}


std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>&
TRANSLINE_CALCULATION_BASE::GetAnalysisResults()
{
    SetAnalysisResults();
    return m_analysisStatus;
}


std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>&
TRANSLINE_CALCULATION_BASE::GetSynthesisResults()
{
    SetSynthesisResults();
    return m_synthesisStatus;
}


void TRANSLINE_CALCULATION_BASE::SetAnalysisResult( const TRANSLINE_PARAMETERS aParam, const double aValue,
                                                    const TRANSLINE_STATUS aStatus )
{
    m_analysisStatus[aParam] = { aValue, aStatus };
}


void TRANSLINE_CALCULATION_BASE::SetSynthesisResult( const TRANSLINE_PARAMETERS aParam, const double aValue,
                                                     const TRANSLINE_STATUS aStatus )
{
    m_synthesisStatus[aParam] = { aValue, aStatus };
}


bool TRANSLINE_CALCULATION_BASE::MinimiseZ0Error1D( const TRANSLINE_PARAMETERS aOptimise,
                                                    const TRANSLINE_PARAMETERS aMeasure, bool aRecalculateLength )
{
    double& var = GetParameterRef( aOptimise );
    double& Z0_param = GetParameterRef( aMeasure );
    double& ANG_L_param = GetParameterRef( TCP::ANG_L );

    if( !std::isfinite( Z0_param ) )
    {
        var = NAN;
        return false;
    }

    if( ( !std::isfinite( var ) ) || ( var == 0 ) )
        var = 0.001;

    /* required value of Z0 */
    double Z0_dest = Z0_param;

    /* required value of angl_l */
    double angl_l_dest = ANG_L_param;

    /* Newton's method */
    int iteration = 0;

    /* compute parameters */
    Analyse();
    double Z0_current = Z0_param;

    double error = fabs( Z0_dest - Z0_current );

    while( error > m_maxError )
    {
        iteration++;
        double increment = var / 100.0;
        var += increment;

        /* compute parameters */
        Analyse();
        double Z0_result = Z0_param;

        // f(w(n)) = Z0 - Z0(w(n))
        // f'(w(n)) = -f'(Z0(w(n)))
        // f'(Z0(w(n))) = (Z0(w(n)) - Z0(w(n+delw))/delw
        // w(n+1) = w(n) - f(w(n))/f'(w(n))
        double slope = ( Z0_result - Z0_current ) / increment;
        slope = ( Z0_dest - Z0_current ) / slope - increment;
        var += slope;

        if( var <= 0.0 )
            var = increment;

        /* find new error */
        /* compute parameters */
        Analyse();
        Z0_current = Z0_param;
        error = fabs( Z0_dest - Z0_current );

        if( iteration > 250 )
            break;
    }

    /* Compute one last time, but with correct length */
    if( aRecalculateLength )
    {
        Z0_param = Z0_dest;
        ANG_L_param = angl_l_dest;
        SetParameter( TCP::PHYS_LEN, TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( GetParameter( TCP::EPSILON_EFF ) )
                                             * ANG_L_param / 2.0 / M_PI ); /* in m */
        Analyse();

        /* Restore parameters */
        Z0_param = Z0_dest;
        ANG_L_param = angl_l_dest;
        SetParameter( TCP::PHYS_LEN, TC::C0 / GetParameter( TCP::FREQUENCY ) / sqrt( GetParameter( TCP::EPSILON_EFF ) )
                                             * ANG_L_param / 2.0 / M_PI ); /* in m */
    }

    return error <= m_maxError;
}


double TRANSLINE_CALCULATION_BASE::SkinDepth() const
{
    double depth = 1.0
                   / sqrt( M_PI * GetParameter( TCP::FREQUENCY ) * GetParameter( TCP::MURC )
                           * TRANSLINE_CALCULATIONS::MU0 * GetParameter( TCP::SIGMA ) );
    return depth;
}


double TRANSLINE_CALCULATION_BASE::UnitPropagationDelay( const double aEpsilonEff )
{
    return std::sqrt( aEpsilonEff ) * ( 1.0e10 / 2.99e8 );
}


void TRANSLINE_CALCULATION_BASE::UpdateDielectricModel()
{
    const auto selected = static_cast<int>( GetParameter( TCP::DIELECTRIC_MODEL_SEL ) );

    if( selected != static_cast<int>( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR ) )
    {
        m_dsModel.reset();
        return;
    }

    const double epsRSpec = GetParameter( TCP::EPSILONR );
    const double tanDSpec = GetParameter( TCP::TAND );
    const double fSpec = GetParameter( TCP::EPSILONR_SPEC_FREQ );

    // User-supplied spec frequency must be positive, otherwise fall back to CONSTANT.
    if( !std::isfinite( fSpec ) || fSpec <= 0.0 )
    {
        m_dsModel.reset();
        return;
    }

    try
    {
        DIELECTRIC_DJORDJEVIC_SARKAR ds;
        ds.Fit( epsRSpec, tanDSpec, fSpec );
        m_dsModel.emplace( std::move( ds ) );
    }
    catch( const std::invalid_argument& )
    {
        m_dsModel.reset();
    }
}


double TRANSLINE_CALCULATION_BASE::GetDispersedEpsilonR( double aF ) const
{
    if( m_dsModel )
        return m_dsModel->EpsilonRealAt( aF );

    return GetParameter( TCP::EPSILONR );
}


double TRANSLINE_CALCULATION_BASE::GetDispersedTanDelta( double aF ) const
{
    if( m_dsModel )
        return m_dsModel->TanDeltaAt( aF );

    return GetParameter( TCP::TAND );
}


double TRANSLINE_CALCULATION_BASE::WanHoorfarQ2( double aU, double aHBarTop )
{
    // Wan-Hoorfar 2000 eq. (4): Hammerstad-style effective strip width for wide strips.
    const double wBarEff = aU + ( 2.0 / M_PI ) * std::log( 17.08 * ( 0.5 * aU + 0.92 ) );

    // Wan-Hoorfar eq. (5): v-bar parametrising the field contraction above the strip as
    // the boundary h_2 moves up.  Clamps to 1 as h_2 -> infinity, 0 as h_2 -> h.
    const double denom = wBarEff * M_PI - 4.0;

    if( denom <= 0.0 || !std::isfinite( denom ) )
        return 0.0;

    const double vBar = ( 2.0 / M_PI ) * std::atan( ( 2.0 * M_PI / denom ) * ( aHBarTop - 1.0 ) );
    const double halfPi = 0.5 * M_PI * vBar;

    if( aU >= 1.0 )
    {
        // Wide strip.  q_1 from eq (2) and q_2 from the improved eq (12).  With n = 3 this
        // reduces to q_2 = 1 - q_1 - correction, i.e. the fraction of the total field that
        // lies between h and h_2.  The full sum-form collapses here because we only need q_2.
        const double q1 = 1.0 - std::log( wBarEff * M_PI - 1.0 ) / ( 2.0 * wBarEff );
        const double inner = 2.0 * wBarEff * std::cos( halfPi ) / ( 2.0 * aHBarTop - 1.0 + vBar )
                             + std::sin( halfPi );

        if( inner <= 0.0 || !std::isfinite( inner ) )
            return 0.0;

        const double correction = ( 1.0 - vBar ) / ( 2.0 * wBarEff ) * std::log( inner );
        return std::max( 0.0, 1.0 - q1 - correction );
    }

    // Narrow strip.  Wan-Hoorfar eq (6), (7), (8), (13).  b_j is the strip-geometry
    // constant; the arccos term encodes the same field-capture geometry as the wide-strip
    // branch but fit against the narrow-strip conformal mapping.
    const double logEighth = std::log( 0.125 * aU );

    if( !std::isfinite( logEighth ) || logEighth == 0.0 )
        return 0.0;

    const double q1 = 0.5 + 0.9 / ( M_PI * logEighth );
    const double bj = ( aHBarTop + 1.0 ) / ( aHBarTop + 0.25 * aU - 1.0 );

    if( bj <= 0.0 || !std::isfinite( bj ) )
        return 0.0;

    const double acosArg = std::sqrt( bj / aHBarTop ) * ( aHBarTop - 1.0 + 0.125 * aU );

    if( acosArg < -1.0 || acosArg > 1.0 )
        return 0.0;

    const double correction = ( std::log( bj ) * std::acos( acosArg ) ) / ( 4.0 * logEighth );
    return std::max( 0.0, 1.0 - q1 - correction );
}


std::pair<double, double>
TRANSLINE_CALCULATION_BASE::ApplySoldermaskCorrection( double aEpsEffUncoated, double aTanDeltaSubstrate,
                                                       double aEpsRSubstrate, double aWOverH,
                                                       double /*aF*/ ) const
{
    // Bit-identical no-op paths.  Any single missing ingredient disables the correction.
    if( GetParameter( TCP::SOLDERMASK_PRESENT ) < 0.5 )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    const double C = GetParameter( TCP::SOLDERMASK_THICKNESS );
    const double h = GetParameter( TCP::H );

    if( C <= 0.0 || h <= 0.0 || !std::isfinite( C ) || !std::isfinite( h ) )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    const double epsMask = GetParameter( TCP::SOLDERMASK_EPSILONR );
    const double tanDMask = GetParameter( TCP::SOLDERMASK_TAND );

    // Reject non-physical mask material parameters.  eps_r < 1 violates causality, NaN/inf
    // would propagate into Z0 / loss outputs, and a negative tan delta would drive
    // ATTEN_DILECTRIC negative (a "dielectric gain" energy-conservation violation).
    if( !std::isfinite( epsMask ) || epsMask <= 1.0 )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    if( !std::isfinite( tanDMask ) || tanDMask < 0.0 )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    if( aEpsRSubstrate <= 1.0 || !std::isfinite( aEpsRSubstrate ) )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    // Delta q_mask: fraction of the un-coated air region above the trace that the mask
    // displaces.  Subtracting the h_2 = h baseline cancels a non-zero offset in Svacina's
    // formula so the correction vanishes smoothly at C = 0.
    const double deltaQ = GetSoldermaskDeltaQ( aWOverH, C / h );

    if( deltaQ <= 0.0 )
        return { aEpsEffUncoated, aTanDeltaSubstrate };

    // Air-replacement decomposition (Bahl and Stuchly 1980, "Analysis of a Microstrip
    // Covered with a Lossy Dielectric", IEEE MTT-28 (2), eq. 22 in the d -> infinity
    // limit).  The mask displaces AIR, not substrate, so
    //     eps_eff_coated = eps_eff_uncoated + Delta q_mask * (eps_mask - 1).
    const double epsEffCoated = aEpsEffUncoated + deltaQ * ( epsMask - 1.0 );

    // q_sub from the standard filling-factor identity for a two-layer microstrip; held
    // fixed when the mask is added on top because the substrate field distribution does
    // not change at leading order.
    const double qSub = std::clamp( ( aEpsEffUncoated - 1.0 ) / ( aEpsRSubstrate - 1.0 ), 0.0, 1.0 );

    // Cap Delta q_mask so the residual air fraction stays non-negative.  Saturates when
    // the subclass factor overshoots (e.g. an empirical CPW model driven by a very thick
    // mask) and prevents dielectric loss from being synthesised out of nowhere.
    const double deltaQCapped = std::min( deltaQ, std::max( 0.0, 1.0 - qSub ) );

    double tanDCoated = aTanDeltaSubstrate;

    if( epsEffCoated > 0.0 )
    {
        tanDCoated = ( qSub * aEpsRSubstrate * aTanDeltaSubstrate
                       + deltaQCapped * epsMask * tanDMask )
                     / epsEffCoated;
    }

    return { epsEffCoated, tanDCoated };
}


std::pair<double, double> TRANSLINE_CALCULATION_BASE::EllipticIntegral( const double arg )
{
    static constexpr double NR_EPSI = 2.2204460492503131e-16;
    int                     iMax = 16;

    double k = 0.0, e = 0.0;

    if( arg == 1.0 )
    {
        k = INFINITY; // infinite
        e = 0;
    }
    else if( std::isinf( arg ) && arg < 0 )
    {
        k = 0;
        e = INFINITY; // infinite
    }
    else
    {
        double a, b, c, fr, s, fk = 1, fe = 1, t, da = arg;
        int    i;

        if( arg < 0 )
        {
            fk = 1 / sqrt( 1 - arg );
            fe = sqrt( 1 - arg );
            da = -arg / ( 1 - arg );
        }

        a = 1;
        b = sqrt( 1 - da );
        c = sqrt( da );
        fr = 0.5;
        s = fr * c * c;

        for( i = 0; i < iMax; i++ )
        {
            t = ( a + b ) / 2;
            c = ( a - b ) / 2;
            b = sqrt( a * b );
            a = t;
            fr *= 2;
            s += fr * c * c;

            if( c / a < NR_EPSI )
                break;
        }

        if( i >= iMax )
        {
            k = 0;
            e = 0;
        }
        else
        {
            k = M_PI_2 / a;
            e = M_PI_2 * ( 1 - s ) / a;
            if( arg < 0 )
            {
                k *= fk;
                e *= fe;
            }
        }
    }

    return { k, e };
}
