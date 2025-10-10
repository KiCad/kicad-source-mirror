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
