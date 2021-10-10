/*
 * twistedpair.h - twisted pair class definition
 *
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Modifications 2011 for Kicad: Jean-Pierre Charras
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

#include "twistedpair.h"
#include "units.h"

TWISTEDPAIR::TWISTEDPAIR() : TRANSLINE()
{
    m_Name = "TwistedPair";
    Init();
}


/**
 *  \f$ \theta = \arctan\left( T \cdot \pi \cdot D_{out} \right) \f$
 *
 * Where :
 * - \f$ \theta \f$ : pitch angle
 * - \f$ T \f$ : Number of twists per unit length
 * - \f$ D_{out} \f$ : Wire diameter with insulation
 *
 *  \f$ e_{eff} = e_{env} \cdot \left( 0.25 + 0.0007 \cdot \theta^2 \right)\cdot\left(e_r-e_{env}\right) \f$
 *
 * Where :
 * - \f$ e_{env} \f$ : relative dielectric constant of air ( or some other surronding material ),
 * - \f$ e_r \f$ : relative dielectric constant of the film insulation,
 * - \f$ e_{eff} \f$ : effective relative dielectric constant
 *
 * \f$ Z_0 = \frac{Z_\mathrm{VACUUM}}{\pi \cdot \sqrt{e_{eff}}}\cosh^{-1}\left(\frac{D_{out}}{D_{in}}\right) \f$
 *
 * - \f$ Z_0 \f$ : line impedance
 * - \f$ Z_\mathrm{VACUUM} \f$ : vacuum impedance
 * - \f$ D_{in} \f$ : Wire diameter without insulation
 *
 * Reference for above equations :
 *
 * [1] : P. Lefferson, ``Twisted Magnet Wire Transmission Line,''
 * IEEE Transactions on Parts, Hybrids, and Packaging, vol. PHP-7, no. 4, pp. 148-154, Dec. 1971.
 *
 * The following URL can be used as reference : http://qucs.sourceforge.net/tech/node93.html
 **/
void TWISTEDPAIR::calcAnalyze()
{

    double tw = atan( m_parameters[TWISTEDPAIR_TWIST_PRM] * M_PI
                      * m_parameters[PHYS_DIAM_OUT_PRM] ); // pitch angle

    m_parameters[EPSILON_EFF_PRM] =
            m_parameters[TWISTEDPAIR_EPSILONR_ENV_PRM]
            + ( 0.25 + 0.0007 * tw * tw )
                      * ( m_parameters[EPSILONR_PRM] - m_parameters[TWISTEDPAIR_EPSILONR_ENV_PRM] );

    m_parameters[Z0_PRM] =
            ZF0 / M_PI / sqrt( m_parameters[EPSILON_EFF_PRM] )
            * acosh( m_parameters[PHYS_DIAM_OUT_PRM] / m_parameters[PHYS_DIAM_IN_PRM] );

    m_parameters[LOSS_CONDUCTOR_PRM] =
            10.0 / log( 10.0 ) * m_parameters[PHYS_LEN_PRM] / m_parameters[SKIN_DEPTH_PRM]
            / m_parameters[SIGMA_PRM] / M_PI / m_parameters[Z0_PRM]
            / ( m_parameters[PHYS_DIAM_IN_PRM] - m_parameters[SKIN_DEPTH_PRM] );

    m_parameters[LOSS_DIELECTRIC_PRM] = 20.0 / log( 10.0 ) * m_parameters[PHYS_LEN_PRM] * M_PI / C0
                                        * m_parameters[FREQUENCY_PRM]
                                        * sqrt( m_parameters[EPSILON_EFF_PRM] )
                                        * m_parameters[TAND_PRM];

    m_parameters[ANG_L_PRM] = 2.0 * M_PI * m_parameters[PHYS_LEN_PRM]
                              * sqrt( m_parameters[EPSILON_EFF_PRM] ) * m_parameters[FREQUENCY_PRM]
                              / C0; // in radians
}


// -------------------------------------------------------------------
void TWISTEDPAIR::show_results()
{
    setResult( 0, m_parameters[EPSILON_EFF_PRM], "" );
    setResult( 1, m_parameters[LOSS_CONDUCTOR_PRM], "dB" );
    setResult( 2, m_parameters[LOSS_DIELECTRIC_PRM], "dB" );
    setResult( 3, m_parameters[SKIN_DEPTH_PRM] / UNIT_MICRON, "µm" );
}

void TWISTEDPAIR::showAnalyze()
{
    setProperty( Z0_PRM, m_parameters[Z0_PRM] );
    setProperty( ANG_L_PRM, m_parameters[ANG_L_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_ERROR );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_ERROR );

    // Find warnings to display - physical parameters
    if( !std::isfinite( m_parameters[PHYS_DIAM_IN_PRM] ) || m_parameters[PHYS_DIAM_IN_PRM] <= 0.0 )
        setErrorLevel( PHYS_DIAM_IN_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[PHYS_DIAM_OUT_PRM] )
            || m_parameters[PHYS_DIAM_OUT_PRM] <= 0.0 )
    {
        setErrorLevel( PHYS_DIAM_OUT_PRM, TRANSLINE_WARNING );
    }

    if( m_parameters[PHYS_DIAM_IN_PRM] > m_parameters[PHYS_DIAM_OUT_PRM] )
    {
        setErrorLevel( PHYS_DIAM_IN_PRM, TRANSLINE_WARNING );
        setErrorLevel( PHYS_DIAM_OUT_PRM, TRANSLINE_WARNING );
    }

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0.0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_WARNING );
}

void TWISTEDPAIR::showSynthesize()
{
    if( isSelected( PHYS_DIAM_IN_PRM ) )
        setProperty( PHYS_DIAM_IN_PRM, m_parameters[PHYS_DIAM_IN_PRM] );
    else if( isSelected( PHYS_DIAM_OUT_PRM ) )
        setProperty( PHYS_DIAM_OUT_PRM, m_parameters[PHYS_DIAM_OUT_PRM] );

    setProperty( PHYS_LEN_PRM, m_parameters[PHYS_LEN_PRM] );

    // Check for errors
    if( !std::isfinite( m_parameters[PHYS_DIAM_IN_PRM] ) || m_parameters[PHYS_DIAM_IN_PRM] <= 0.0 )
    {
        if( isSelected( PHYS_DIAM_IN_PRM ) )
            setErrorLevel( PHYS_DIAM_IN_PRM, TRANSLINE_ERROR );
        else
            setErrorLevel( PHYS_DIAM_IN_PRM, TRANSLINE_WARNING );
    }

    if( !std::isfinite( m_parameters[PHYS_DIAM_OUT_PRM] )
            || m_parameters[PHYS_DIAM_OUT_PRM] <= 0.0 )
    {
        if( isSelected( PHYS_DIAM_OUT_PRM ) )
            setErrorLevel( PHYS_DIAM_OUT_PRM, TRANSLINE_ERROR );
        else
            setErrorLevel( PHYS_DIAM_OUT_PRM, TRANSLINE_WARNING );
    }

    if( m_parameters[PHYS_DIAM_IN_PRM] > m_parameters[PHYS_DIAM_OUT_PRM] )
    {
        if( isSelected( PHYS_DIAM_IN_PRM ) )
            setErrorLevel( PHYS_DIAM_IN_PRM, TRANSLINE_ERROR );
        else if( isSelected( PHYS_DIAM_OUT_PRM ) )
            setErrorLevel( PHYS_DIAM_OUT_PRM, TRANSLINE_ERROR );
    }

    if( !std::isfinite( m_parameters[PHYS_LEN_PRM] ) || m_parameters[PHYS_LEN_PRM] < 0.0 )
        setErrorLevel( PHYS_LEN_PRM, TRANSLINE_ERROR );

    // Check for warnings
    if( !std::isfinite( m_parameters[Z0_PRM] ) || m_parameters[Z0_PRM] < 0 )
        setErrorLevel( Z0_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[ANG_L_PRM] ) || m_parameters[ANG_L_PRM] < 0 )
        setErrorLevel( ANG_L_PRM, TRANSLINE_WARNING );
}


#define MAX_ERROR 0.000001

// -------------------------------------------------------------------
void TWISTEDPAIR::calcSynthesize()
{
    if( isSelected( PHYS_DIAM_IN_PRM ) )
        minimizeZ0Error1D( &( m_parameters[PHYS_DIAM_IN_PRM] ) );
    else
        minimizeZ0Error1D( &( m_parameters[PHYS_DIAM_OUT_PRM] ) );
}
