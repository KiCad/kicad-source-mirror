/*
 * coplanar.cpp - coplanar class implementation
 *
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
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

#include "coplanar.h"
#include "units.h"

COPLANAR::COPLANAR() : TRANSLINE()
{
    m_Name    = "CoPlanar";
    backMetal = false;
    unit_prop_delay = 0.0;
    Init();
}


GROUNDEDCOPLANAR::GROUNDEDCOPLANAR() : COPLANAR()
{
    m_Name    = "GrCoPlanar";
    backMetal = true;
}


// -------------------------------------------------------------------
void COPLANAR::calcAnalyze()
{
    m_parameters[SKIN_DEPTH_PRM] = skin_depth();

    // other local variables (quasi-static constants)
    double k1, kk1, kpk1, k2, k3, q1, q2, q3 = 0, qz, er0 = 0;
    double zl_factor;

    // compute the necessary quasi-static approx. (K1, K3, er(0) and Z(0))
    k1   = m_parameters[PHYS_WIDTH_PRM]
           / ( m_parameters[PHYS_WIDTH_PRM] + m_parameters[PHYS_S_PRM] + m_parameters[PHYS_S_PRM] );
    kk1  = ellipk( k1 );
    kpk1 = ellipk( sqrt( 1 - k1 * k1 ) );
    q1   = kk1 / kpk1;


    // backside is metal
    if( backMetal )
    {
        k3 = tanh( ( M_PI / 4 ) * ( m_parameters[PHYS_WIDTH_PRM] / m_parameters[H_PRM] ) )
             / tanh( ( M_PI / 4 )
                     * ( m_parameters[PHYS_WIDTH_PRM] + m_parameters[PHYS_S_PRM]
                             + m_parameters[PHYS_S_PRM] )
                     / m_parameters[H_PRM] );
        q3        = ellipk( k3 ) / ellipk( sqrt( 1 - k3 * k3 ) );
        qz        = 1 / ( q1 + q3 );
        er0       = 1 + q3 * qz * ( m_parameters[EPSILONR_PRM] - 1 );
        zl_factor = ZF0 / 2 * qz;
    }
    // backside is air
    else
    {
        k2 = sinh( ( M_PI / 4 ) * ( m_parameters[PHYS_WIDTH_PRM] / m_parameters[H_PRM] ) )
             / sinh( ( M_PI / 4 )
                     * ( m_parameters[PHYS_WIDTH_PRM] + m_parameters[PHYS_S_PRM]
                             + m_parameters[PHYS_S_PRM] )
                     / m_parameters[H_PRM] );
        q2        = ellipk( k2 ) / ellipk( sqrt( 1 - k2 * k2 ) );
        er0       = 1 + ( m_parameters[EPSILONR_PRM] - 1 ) / 2 * q2 / q1;
        zl_factor = ZF0 / 4 / q1;
    }


    // adds effect of strip thickness
    if( m_parameters[T_PRM] > 0 )
    {
        double d, se, We, ke, qe;
        d = ( m_parameters[T_PRM] * 1.25 / M_PI )
            * ( 1 + log( 4 * M_PI * m_parameters[PHYS_WIDTH_PRM] / m_parameters[T_PRM] ) );
        se = m_parameters[PHYS_S_PRM] - d;
        We = m_parameters[PHYS_WIDTH_PRM] + d;

        // modifies k1 accordingly (k1 = ke)
        ke = We / ( We + se + se ); // ke = k1 + (1 - k1 * k1) * d / 2 / s;
        qe = ellipk( ke ) / ellipk( sqrt( 1 - ke * ke ) );

        // backside is metal
        if( backMetal )
        {
            qz        = 1 / ( qe + q3 );
            er0       = 1 + q3 * qz * ( m_parameters[EPSILONR_PRM] - 1 );
            zl_factor = ZF0 / 2 * qz;
        }
        // backside is air
        else
        {
            zl_factor = ZF0 / 4 / qe;
        }

        // modifies er0 as well
        er0 = er0
              - ( 0.7 * ( er0 - 1 ) * m_parameters[T_PRM] / m_parameters[PHYS_S_PRM] )
                        / ( q1 + ( 0.7 * m_parameters[T_PRM] / m_parameters[PHYS_S_PRM] ) );
    }

    // pre-compute square roots
    double sr_er  = sqrt( m_parameters[EPSILONR_PRM] );
    double sr_er0 = sqrt( er0 );

    // cut-off frequency of the TE0 mode
    double fte = ( C0 / 4 ) / ( m_parameters[H_PRM] * sqrt( m_parameters[EPSILONR_PRM] - 1 ) );

    // dispersion factor G
    double p = log( m_parameters[PHYS_WIDTH_PRM] / m_parameters[H_PRM] );
    double u = 0.54 - ( 0.64 - 0.015 * p ) * p;
    double v = 0.43 - ( 0.86 - 0.54 * p ) * p;
    double G = exp( u * log( m_parameters[PHYS_WIDTH_PRM] / m_parameters[PHYS_S_PRM] ) + v );

    // loss constant factors (computed only once for efficiency's sake)
    double ac = 0;

    if( m_parameters[T_PRM] > 0 )
    {
        // equations by GHIONE
        double n = ( 1 - k1 ) * 8 * M_PI / ( m_parameters[T_PRM] * ( 1 + k1 ) );
        double a = m_parameters[PHYS_WIDTH_PRM] / 2;
        double b = a + m_parameters[PHYS_S_PRM];
        ac       = ( M_PI + log( n * a ) ) / a + ( M_PI + log( n * b ) ) / b;
    }

    double ac_factor = ac / ( 4 * ZF0 * kk1 * kpk1 * ( 1 - k1 * k1 ) );
    double ad_factor = ( m_parameters[EPSILONR_PRM] / ( m_parameters[EPSILONR_PRM] - 1 ) )
                       * m_parameters[TAND_PRM] * M_PI / C0;


    // ....................................................
    double sr_er_f = sr_er0;

    // add the dispersive effects to er0
    sr_er_f += ( sr_er - sr_er0 ) / ( 1 + G * pow( m_parameters[FREQUENCY_PRM] / fte, -1.8 ) );

    // for now, the loss are limited to strip losses (no radiation
    // losses yet) losses in neper/length
    m_parameters[LOSS_CONDUCTOR_PRM] =
            20.0 / log( 10.0 ) * m_parameters[PHYS_LEN_PRM] * ac_factor * sr_er0
            * sqrt( M_PI * MU0 * m_parameters[FREQUENCY_PRM] / m_parameters[SIGMA_PRM] );
    m_parameters[LOSS_DIELECTRIC_PRM] = 20.0 / log( 10.0 ) * m_parameters[PHYS_LEN_PRM] * ad_factor
                                        * m_parameters[FREQUENCY_PRM] * ( sr_er_f * sr_er_f - 1 )
                                        / sr_er_f;

    m_parameters[ANG_L_PRM] = 2.0 * M_PI * m_parameters[PHYS_LEN_PRM] * sr_er_f
                              * m_parameters[FREQUENCY_PRM] / C0; /* in radians */

    m_parameters[EPSILON_EFF_PRM] = sr_er_f * sr_er_f;
    m_parameters[Z0_PRM]          = zl_factor / sr_er_f;

    unit_prop_delay = calcUnitPropagationDelay( m_parameters[EPSILON_EFF_PRM] );
}


// -------------------------------------------------------------------
void COPLANAR::show_results()
{

    setResult( 0, m_parameters[EPSILON_EFF_PRM], "" );
    setResult( 1, unit_prop_delay, "ps/cm" );
    setResult( 2, m_parameters[LOSS_CONDUCTOR_PRM], "dB" );
    setResult( 3, m_parameters[LOSS_DIELECTRIC_PRM], "dB" );

    setResult( 4, m_parameters[SKIN_DEPTH_PRM] / UNIT_MICRON, "µm" );
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
/* @function calcSynthesize
 *
 * @TODO Add a warning in case the synthetizin algorithm did not converge.
 * Add it for all transmission lines that uses @ref minimizeZ0Error1D .
 */
void COPLANAR::calcSynthesize()
{
    if( isSelected( PHYS_WIDTH_PRM ) )
        minimizeZ0Error1D( &( m_parameters[PHYS_WIDTH_PRM] ) );
    else
        minimizeZ0Error1D( &( m_parameters[PHYS_S_PRM] ) );
}

// -------------------------------------------------------------------
void COPLANAR::showSynthesize()
{
    if( isSelected( PHYS_WIDTH_PRM ) )
        setProperty( PHYS_WIDTH_PRM, m_parameters[PHYS_WIDTH_PRM] );

    if( isSelected( PHYS_S_PRM ) )
        setProperty( PHYS_S_PRM, m_parameters[PHYS_S_PRM] );

    setProperty( PHYS_LEN_PRM, m_parameters[PHYS_LEN_PRM] );

    if( !std::isfinite( m_parameters[PHYS_S_PRM] ) || m_parameters[PHYS_S_PRM] <= 0 )
    {
        if( isSelected( PHYS_S_PRM ) )
            setErrorLevel( PHYS_S_PRM, TRANSLINE_ERROR );
        else
            setErrorLevel( PHYS_S_PRM, TRANSLINE_WARNING );
    }

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || m_parameters[PHYS_WIDTH_PRM] <= 0 )
    {
        if( isSelected( PHYS_WIDTH_PRM ) )
            setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_ERROR );
        else
            setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_WARNING );
    }

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_WARNING );
}


void COPLANAR::showAnalyze()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    if( !std::isfinite( m_parameters[PHYS_S_PRM] ) || m_parameters[PHYS_S_PRM] <= 0 )
        setErrorLevel( PHYS_S_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_WIDTH_PRM] ) || m_parameters[PHYS_WIDTH_PRM] <= 0 )
        setErrorLevel( PHYS_WIDTH_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_ERROR );
}
