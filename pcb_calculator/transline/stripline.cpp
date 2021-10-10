/*
 * stripline.cpp - stripline class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2018 for Kicad: Jean-Pierre Charras
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
 *
 */


#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "stripline.h"
#include "units.h"

STRIPLINE::STRIPLINE() : TRANSLINE()
{
    m_Name = "StripLine";
    Init();
}


// -------------------------------------------------------------------
// calculate characteristic impedance and conductor loss
double STRIPLINE::lineImpedance( double height, double& ac )
{
    double ZL;
    double hmt = height - m_parameters[T_PRM];

    ac = sqrt( m_parameters[FREQUENCY_PRM] / m_parameters[SIGMA_PRM] / 17.2 );

    if( m_parameters[PHYS_WIDTH_PRM] / hmt >= 0.35 )
    {
        ZL = m_parameters[PHYS_WIDTH_PRM]
             + ( 2.0 * height * log( ( 2.0 * height - m_parameters[T_PRM] ) / hmt )
                       - m_parameters[T_PRM] * log( height * height / hmt / hmt - 1.0 ) )
                       / M_PI;
        ZL = ZF0 * hmt / sqrt( m_parameters[EPSILONR_PRM] ) / 4.0 / ZL;

        ac *= 2.02e-6 * m_parameters[EPSILONR_PRM] * ZL / hmt;
        ac *= 1.0 + 2.0 * m_parameters[PHYS_WIDTH_PRM] / hmt
              + ( height + m_parameters[T_PRM] ) / hmt / M_PI
                        * log( 2.0 * height / m_parameters[T_PRM] - 1.0 );
    }
    else
    {
        double tdw = m_parameters[T_PRM] / m_parameters[PHYS_WIDTH_PRM];
        if( m_parameters[T_PRM] / m_parameters[PHYS_WIDTH_PRM] > 1.0 )
            tdw = m_parameters[PHYS_WIDTH_PRM] / m_parameters[T_PRM];
        double de = 1.0 + tdw / M_PI * ( 1.0 + log( 4.0 * M_PI / tdw ) ) + 0.236 * pow( tdw, 1.65 );
        if( m_parameters[T_PRM] / m_parameters[PHYS_WIDTH_PRM] > 1.0 )
            de *= m_parameters[T_PRM] / 2.0;
        else
            de *= m_parameters[PHYS_WIDTH_PRM] / 2.0;
        ZL = ZF0 / 2.0 / M_PI / sqrt( m_parameters[EPSILONR_PRM] )
             * log( 4.0 * height / M_PI / de );

        ac *= 0.01141 / ZL / de;
        ac *= de / height + 0.5 + tdw / 2.0 / M_PI + 0.5 / M_PI * log( 4.0 * M_PI / tdw )
              + 0.1947 * pow( tdw, 0.65 ) - 0.0767 * pow( tdw, 1.65 );
    }

    return ZL;
}


// -------------------------------------------------------------------
void STRIPLINE::calcAnalyze()
{
    m_parameters[SKIN_DEPTH_PRM] = skin_depth();

    m_parameters[EPSILON_EFF_PRM] = m_parameters[EPSILONR_PRM]; // no dispersion

    double ac1, ac2;
    double t             = m_parameters[T_PRM];
    double a             = m_parameters[STRIPLINE_A_PRM];
    double h             = m_parameters[H_PRM];
    m_parameters[Z0_PRM] = 2.0
                           / ( 1.0 / lineImpedance( 2.0 * a + t, ac1 )
                                   + 1.0 / lineImpedance( 2.0 * ( h - a ) - t, ac2 ) );
    m_parameters[LOSS_CONDUCTOR_PRM]  = m_parameters[PHYS_LEN_PRM] * 0.5 * ( ac1 + ac2 );
    m_parameters[LOSS_DIELECTRIC_PRM] = 20.0 / log( 10.0 ) * m_parameters[PHYS_LEN_PRM]
                                        * ( M_PI / C0 ) * m_parameters[FREQUENCY_PRM]
                                        * sqrt( m_parameters[EPSILONR_PRM] )
                                        * m_parameters[TAND_PRM];

    m_parameters[ANG_L_PRM] = 2.0 * M_PI * m_parameters[PHYS_LEN_PRM]
                              * sqrt( m_parameters[EPSILONR_PRM] ) * m_parameters[FREQUENCY_PRM]
                              / C0; // in radians
}


void STRIPLINE::showAnalyze()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || m_parameters[PHYS_WIDTH_PRM] < 0 )
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_WARNING );

    if( m_parameters[STRIPLINE_A_PRM] + m_parameters[T_PRM] >= m_parameters[H_PRM] )
    {
        setErrorLevel( STRIPLINE_A_PRM, TRANSLINE_WARNING );
        setErrorLevel( T_PRM, TRANSLINE_WARNING );
        setErrorLevel( H_PRM, TRANSLINE_WARNING );
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );
    }
}

void STRIPLINE::showSynthesize()
{
    setProperty( PHYS_LEN_PRM, m_parameters[PHYS_LEN_PRM] );
    setProperty( PHYS_WIDTH_PRM, m_parameters[PHYS_WIDTH_PRM] );

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || m_parameters[PHYS_WIDTH_PRM] < 0 )
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_WARNING );

    if( m_parameters[STRIPLINE_A_PRM] + m_parameters[T_PRM] >= m_parameters[H_PRM] )
    {
        setErrorLevel( STRIPLINE_A_PRM, TRANSLINE_WARNING );
        setErrorLevel( T_PRM, TRANSLINE_WARNING );
        setErrorLevel( H_PRM, TRANSLINE_WARNING );
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_ERROR );
    }
}
// -------------------------------------------------------------------
void STRIPLINE::show_results()
{

    setResult( 0, m_parameters[EPSILON_EFF_PRM], "" );
    setResult( 1, m_parameters[LOSS_CONDUCTOR_PRM], "dB" );
    setResult( 2, m_parameters[LOSS_DIELECTRIC_PRM], "dB" );

    setResult( 3, m_parameters[SKIN_DEPTH_PRM] / UNIT_MICRON, "µm" );
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
void STRIPLINE::calcSynthesize()
{
    minimizeZ0Error1D( &( m_parameters[PHYS_WIDTH_PRM] ) );
}
