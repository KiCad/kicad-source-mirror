/*
 * coax.cpp - coaxial class implementation
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
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


/*
 * coax.c - Puts up window for microstrip and
 * performs the associated calculations
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "coax.h"
#include "units.h"

COAX::COAX() : TRANSLINE()
{
    m_Name = "Coax";
    Init();
}

double COAX::alphad_coax()
{
    double ad;

    ad = ( M_PI / C0 ) * m_parameters[FREQUENCY_PRM] * sqrt( m_parameters[EPSILONR_PRM] )
         * m_parameters[TAND_PRM];
    ad = ad * 20.0 / log( 10.0 );
    return ad;
}


double COAX::alphac_coax()
{
    double ac, Rs;

    Rs = sqrt( M_PI * m_parameters[FREQUENCY_PRM] * m_parameters[MURC_PRM] * MU0
               / m_parameters[SIGMA_PRM] );
    ac = sqrt( m_parameters[EPSILONR_PRM] )
         * ( ( ( 1 / m_parameters[PHYS_DIAM_IN_PRM] ) + ( 1 / m_parameters[PHYS_DIAM_OUT_PRM] ) )
                 / log( m_parameters[PHYS_DIAM_OUT_PRM] / m_parameters[PHYS_DIAM_IN_PRM] ) )
         * ( Rs / ZF0 );
    ac = ac * 20.0 / log( 10.0 );
    return ac;
}


/**
 *  \f$ Z_0 = \frac{Z_{0_{\mathrm{vacuum}}}}{\sqrt{\epsilon_r}}\log_{10}\left( \frac{D_{\mathrm{out}}}{D_{\mathrm{in}}}\right) \f$
 *
 *  \f$ \lambda_g = \frac{c}{f \cdot \sqrt{ \epsilon_r \cdot \mu_r}} \f$
 *
 *  \f$ L_{[\mathrm{rad}]} = \frac{ 2\pi\cdot L_{[\mathrm{m}]}}{\lambda_g} \f$
 * */
void COAX::calcAnalyze()
{
    double lambda_g;


    m_parameters[Z0_PRM] =
            ( ZF0 / 2 / M_PI / sqrt( m_parameters[EPSILONR_PRM] ) )
            * log( m_parameters[PHYS_DIAM_OUT_PRM] / m_parameters[PHYS_DIAM_IN_PRM] );

    lambda_g = ( C0 / ( m_parameters[FREQUENCY_PRM] ) )
               / sqrt( m_parameters[EPSILONR_PRM] * m_parameters[MUR_PRM] );
    /* calculate electrical angle */
    m_parameters[ANG_L_PRM] =
            ( 2.0 * M_PI * m_parameters[PHYS_LEN_PRM] ) / lambda_g; /* in radians */
}


/**
 *  \f$ D_{\mathrm{in}} = D_{\mathrm{out}}  \cdot e^{-\frac{Z_0*\sqrt{\epsilon_r}}{2\pi \cdot  Z_{0_{\mathrm{vacuum}}}}} \f$
 *
 *  \f$ D_{\mathrm{out}} = D_{\mathrm{in}}  \cdot e^{ \frac{Z_0*\sqrt{\epsilon_r}}{2\pi \cdot  Z_{0_{\mathrm{vacuum}}}}} \f$
 *
 *  \f$ \lambda_g = \frac{c}{f \cdot \sqrt{ \epsilon_r \cdot \mu_r}} \f$
 *
 *  \f$ L_{[\mathrm{m}]} = \frac{ \lambda_g cdot L_{[\mathrm{m}]}}{2\pi} \f$
 * */
void COAX::calcSynthesize()
{
    double lambda_g;

    if( isSelected( PHYS_DIAM_IN_PRM ) )
    {
        /* solve for din */
        m_parameters[PHYS_DIAM_IN_PRM] =
                m_parameters[PHYS_DIAM_OUT_PRM]
                / exp( m_parameters[Z0_PRM] * sqrt( m_parameters[EPSILONR_PRM] ) / ZF0 * 2 * M_PI );
    }
    else if( isSelected( PHYS_DIAM_OUT_PRM ) )
    {
        /* solve for dout */
        m_parameters[PHYS_DIAM_OUT_PRM] =
                m_parameters[PHYS_DIAM_IN_PRM]
                * exp( m_parameters[Z0_PRM] * sqrt( m_parameters[EPSILONR_PRM] ) / ZF0 * 2 * M_PI );
    }

    lambda_g = ( C0 / ( m_parameters[FREQUENCY_PRM] ) )
               / sqrt( m_parameters[EPSILONR_PRM] * m_parameters[MUR_PRM] );
    /* calculate physical length */
    m_parameters[PHYS_LEN_PRM] = ( lambda_g * m_parameters[ANG_L_PRM] ) / ( 2.0 * M_PI ); /* in m */
}


void COAX::showAnalyze()
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

void COAX::showSynthesize()
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
/*
 * show_results() - show results
 */
void COAX::show_results()
{
    int  m, n;
    char text[256], txt[256];

    m_parameters[LOSS_DIELECTRIC_PRM] = alphad_coax() * m_parameters[PHYS_LEN_PRM];
    m_parameters[LOSS_CONDUCTOR_PRM]  = alphac_coax() * m_parameters[PHYS_LEN_PRM];

    setResult( 0, m_parameters[EPSILONR_PRM], "" );
    setResult( 1, m_parameters[LOSS_CONDUCTOR_PRM], "dB" );
    setResult( 2, m_parameters[LOSS_DIELECTRIC_PRM], "dB" );

    n = 1;
    m_parameters[CUTOFF_FREQUENCY_PRM] =
            C0
            / ( M_PI * ( m_parameters[PHYS_DIAM_OUT_PRM] + m_parameters[MUR_PRM] ) / (double) n );

    if( m_parameters[CUTOFF_FREQUENCY_PRM] > m_parameters[FREQUENCY_PRM] )
    {
        strcpy( text, "none" );
    }
    else
    {
        strcpy( text, "H(1,1) " );
        m = 2;
        m_parameters[CUTOFF_FREQUENCY_PRM] =
                C0
                / ( 2 * ( m_parameters[PHYS_DIAM_OUT_PRM] - m_parameters[MUR_PRM] )
                        / (double) ( m - 1 ) );

        while( ( m_parameters[CUTOFF_FREQUENCY_PRM] <= m_parameters[FREQUENCY_PRM] ) && ( m < 10 ) )
        {
            snprintf( txt, sizeof( text ), "H(n,%d) ", m );
            strcat( text, txt );
            m++;
            m_parameters[CUTOFF_FREQUENCY_PRM] =
                    C0
                    / ( 2 * ( m_parameters[PHYS_DIAM_OUT_PRM] - m_parameters[MUR_PRM] )
                            / (double) ( m - 1 ) );
        }
    }
    setResult( 3, text );

    m = 1;
    m_parameters[CUTOFF_FREQUENCY_PRM] =
            C0 / ( 2 * ( m_parameters[PHYS_DIAM_OUT_PRM] - m_parameters[MUR_PRM] ) / (double) m );
    if( m_parameters[CUTOFF_FREQUENCY_PRM] > m_parameters[FREQUENCY_PRM] )
    {
        strcpy( text, "none" );
    }
    else
    {
        strcpy( text, "" );

        while( ( m_parameters[CUTOFF_FREQUENCY_PRM] <= m_parameters[FREQUENCY_PRM] ) && ( m < 10 ) )
        {
            snprintf( txt, sizeof( text ), "E(n,%d) ", m );
            strcat( text, txt );
            m++;
            m_parameters[CUTOFF_FREQUENCY_PRM] =
                    C0
                    / ( 2 * ( m_parameters[PHYS_DIAM_OUT_PRM] - m_parameters[MUR_PRM] )
                            / (double) m );
        }
    }
    setResult( 4, text );
}
