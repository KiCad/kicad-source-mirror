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

#include <transline_calculations/stripline.h>
#include <transline_calculations/units.h>


namespace TC = TRANSLINE_CALCULATIONS;
using TCP = TRANSLINE_PARAMETERS;


void STRIPLINE::Analyse()
{
    SetParameter( TCP::SKIN_DEPTH, SkinDepth() );
    SetParameter( TCP::EPSILON_EFF, GetParameter( TCP::EPSILONR ) ); // no dispersion

    double ac1, ac2;
    double t = GetParameter( TCP::T );
    double a = GetParameter( TCP::STRIPLINE_A );
    double h = GetParameter( TCP::H );
    SetParameter( TCP::Z0,
                  2.0 / ( 1.0 / lineImpedance( 2.0 * a + t, ac1 ) + 1.0 / lineImpedance( 2.0 * ( h - a ) - t, ac2 ) ) );
    SetParameter( TCP::LOSS_CONDUCTOR, GetParameter( TCP::PHYS_LEN ) * ( ac1 + ac2 ) );
    SetParameter( TCP::LOSS_DIELECTRIC, TC::LOG2DB * GetParameter( TCP::PHYS_LEN ) * ( M_PI / TC::C0 )
                                                * GetParameter( TCP::FREQUENCY ) * sqrt( GetParameter( TCP::EPSILONR ) )
                                                * GetParameter( TCP::TAND ) );

    SetParameter( TCP::ANG_L, 2.0 * M_PI * GetParameter( TCP::PHYS_LEN ) * sqrt( GetParameter( TCP::EPSILONR ) )
                                      * GetParameter( TCP::FREQUENCY ) / TC::C0 ); // in radians

    unit_prop_delay = UnitPropagationDelay( GetParameter( TCP::EPSILON_EFF ) );
}


bool STRIPLINE::Synthesize( const SYNTHESIZE_OPTS aOpts )
{
    return MinimiseZ0Error1D( TCP::PHYS_WIDTH, TCP::Z0 );
}


void STRIPLINE::SetAnalysisResults()
{
    SetAnalysisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetAnalysisResult( TCP::UNIT_PROP_DELAY, unit_prop_delay );
    SetAnalysisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetAnalysisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetAnalysisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double ANG_L = GetParameter( TCP::ANG_L );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;

    bool invalid = false;

    if( GetParameter( TCP::STRIPLINE_A ) + GetParameter( TCP::T ) >= GetParameter( TCP::H ) )
        invalid = true;

    SetAnalysisResult( TCP::Z0, Z0, Z0_invalid || invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::PHYS_WIDTH, W, W_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::STRIPLINE_A, GetParameter( TCP::STRIPLINE_A ),
                       invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::T, GetParameter( TCP::T ), invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::H, GetParameter( TCP::H ), invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetAnalysisResult( TCP::Z0, GetParameter( TCP::Z0 ), invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


void STRIPLINE::SetSynthesisResults()
{
    SetSynthesisResult( TCP::EPSILON_EFF, GetParameter( TCP::EPSILON_EFF ) );
    SetSynthesisResult( TCP::UNIT_PROP_DELAY, unit_prop_delay );
    SetSynthesisResult( TCP::LOSS_CONDUCTOR, GetParameter( TCP::LOSS_CONDUCTOR ) );
    SetSynthesisResult( TCP::LOSS_DIELECTRIC, GetParameter( TCP::LOSS_DIELECTRIC ) );
    SetSynthesisResult( TCP::SKIN_DEPTH, GetParameter( TCP::SKIN_DEPTH ) );

    const double Z0 = GetParameter( TCP::Z0 );
    const double ANG_L = GetParameter( TCP::ANG_L );
    const double L = GetParameter( TCP::PHYS_LEN );
    const double W = GetParameter( TCP::PHYS_WIDTH );

    const bool Z0_invalid = !std::isfinite( Z0 ) || Z0 < 0;
    const bool ANG_L_invalid = !std::isfinite( ANG_L ) || ANG_L < 0;
    const bool L_invalid = !std::isfinite( L ) || L < 0;
    const bool W_invalid = !std::isfinite( W ) || W <= 0;

    bool invalid = false;

    if( GetParameter( TCP::STRIPLINE_A ) + GetParameter( TCP::T ) >= GetParameter( TCP::H ) )
        invalid = true;

    SetSynthesisResult( TCP::Z0, Z0, Z0_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::ANG_L, ANG_L, ANG_L_invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_LEN, L, L_invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::PHYS_WIDTH, W, W_invalid || invalid ? TRANSLINE_STATUS::TS_ERROR : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::STRIPLINE_A, GetParameter( TCP::STRIPLINE_A ),
                        invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::T, GetParameter( TCP::T ), invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
    SetSynthesisResult( TCP::H, GetParameter( TCP::H ), invalid ? TRANSLINE_STATUS::WARNING : TRANSLINE_STATUS::OK );
}


double STRIPLINE::lineImpedance( double aHeight, double& aAc ) const
{
    double       ZL;
    const double hmt = aHeight - GetParameter( TCP::T );

    aAc = sqrt( GetParameter( TCP::FREQUENCY ) / GetParameter( TCP::SIGMA ) / 17.2 );

    if( GetParameter( TCP::PHYS_WIDTH ) / hmt >= 0.35 )
    {
        ZL = GetParameter( TCP::PHYS_WIDTH )
             + ( 2.0 * aHeight * log( ( 2.0 * aHeight - GetParameter( TCP::T ) ) / hmt )
                 - GetParameter( TCP::T ) * log( aHeight * aHeight / hmt / hmt - 1.0 ) )
                       / M_PI;
        ZL = TC::ZF0 * hmt / sqrt( GetParameter( TCP::EPSILONR ) ) / 4.0 / ZL;

        aAc *= 2.02e-6 * GetParameter( TCP::EPSILONR ) * ZL / hmt;
        aAc *= 1.0 + 2.0 * GetParameter( TCP::PHYS_WIDTH ) / hmt
               + ( aHeight + GetParameter( TCP::T ) ) / hmt / M_PI
                         * log( 2.0 * aHeight / GetParameter( TCP::T ) - 1.0 );
    }
    else
    {
        double tdw = GetParameter( TCP::T ) / GetParameter( TCP::PHYS_WIDTH );

        if( GetParameter( TCP::T ) / GetParameter( TCP::PHYS_WIDTH ) > 1.0 )
            tdw = GetParameter( TCP::PHYS_WIDTH ) / GetParameter( TCP::T );

        double de = 1.0 + tdw / M_PI * ( 1.0 + log( 4.0 * M_PI / tdw ) ) + 0.236 * pow( tdw, 1.65 );

        if( GetParameter( TCP::T ) / GetParameter( TCP::PHYS_WIDTH ) > 1.0 )
            de *= GetParameter( TCP::T ) / 2.0;
        else
            de *= GetParameter( TCP::PHYS_WIDTH ) / 2.0;

        ZL = TC::ZF0 / 2.0 / M_PI / sqrt( GetParameter( TCP::EPSILONR ) ) * log( 4.0 * aHeight / M_PI / de );

        aAc *= 0.01141 / ZL / de;
        aAc *= de / aHeight + 0.5 + tdw / 2.0 / M_PI + 0.5 / M_PI * log( 4.0 * M_PI / tdw ) + 0.1947 * pow( tdw, 0.65 )
               - 0.0767 * pow( tdw, 1.65 );
    }

    return ZL;
}
