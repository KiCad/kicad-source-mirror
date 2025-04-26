
/*
 * TRANSLINE.cpp - base for a transmission line implementation
 *
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2018 jean-pierre.charras
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
 *
 */

#include <cmath>
#include <limits>

#include <wx/colour.h>
#include <wx/settings.h>

#include "transline.h"
#include "units.h"

#ifndef INFINITY
#define INFINITY std::numeric_limits<double>::infinity()
#endif


#ifndef M_PI_2
#define M_PI_2 ( M_PI / 2 )
#endif


// Functions to Read/Write parameters in pcb_calculator main frame:
// They are wrapper to actual functions, so all transline functions do not
// depend on Graphic User Interface
void SetPropertyInDialog( enum PRMS_ID aPrmId, double value );

/* Puts the text into the given result line.
*/
void SetResultInDialog( int line, const char* text );

/* print aValue into the given result line.
*/
void SetResultInDialog( int aLineNumber, double aValue, const char* aText );

/* Returns a named property value. */
double GetPropertyInDialog( enum PRMS_ID aPrmId );

// Returns true if the param aPrmId is selected
// Has meaning only for params that have a radio button
bool IsSelectedInDialog( enum PRMS_ID aPrmId );

/** Function SetPropertyBgColorInDialog
 *  Set the background color of a parameter
 *  @param aPrmId = param id to set
 *  @param aCol = new color
 */
void SetPropertyBgColorInDialog( enum PRMS_ID aPrmId, const KIGFX::COLOR4D* aCol );


/* Constructor creates a transmission line instance. */
TRANSLINE::TRANSLINE()
{
    m_parameters[MURC_PRM] = 1.0;
    m_Name                 = nullptr;
    Init();
}


/* Destructor destroys a transmission line instance. */
TRANSLINE::~TRANSLINE()
{
}


void TRANSLINE::Init()
{
    wxColour wxcol = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    okCol          = KIGFX::COLOR4D( wxcol );
    okCol.r        = wxcol.Red() / 255.0;
    okCol.g        = wxcol.Green() / 255.0;
    okCol.b        = wxcol.Blue() / 255.0;
    int i;
    // Initialize these variables mainly to avoid warnings from a static analyzer
    for( i = 0; i < EXTRA_PRMS_COUNT; ++i )
    {
        m_parameters[i] = 0;
    }
}


/* Sets a named property to the given value, access through the
 *  application.
 */
void TRANSLINE::setProperty( enum PRMS_ID aPrmId, double value )
{
    SetPropertyInDialog( aPrmId, value );
}


/*
 *Returns true if the param aPrmId is selected
 * Has meaning only for params that have a radio button
 */
bool TRANSLINE::isSelected( enum PRMS_ID aPrmId )
{
    return IsSelectedInDialog( aPrmId );
}


/* Puts the text into the given result line.
*/
void TRANSLINE::setResult( int line, const char* text )
{
    SetResultInDialog( line, text );
}


void TRANSLINE::setResult( int line, double value, const char* text )
{
    SetResultInDialog( line, value, text );
}


/* Returns a property value. */
double TRANSLINE::getProperty( enum PRMS_ID aPrmId )
{
    return GetPropertyInDialog( aPrmId );
}


/** @function getProperties
 *
 *  Get all properties from the UI. Computes some extra ones.
 **/
void TRANSLINE::getProperties()
{
    for( int i = 0; i < DUMMY_PRM; ++i )
    {
        m_parameters[i] = getProperty( (PRMS_ID) i );
        setErrorLevel( (PRMS_ID) i, TRANSLINE_OK );
    }

    m_parameters[SIGMA_PRM]       = 1.0 / getProperty( RHO_PRM );
    m_parameters[EPSILON_EFF_PRM] = 1.0;
    m_parameters[SKIN_DEPTH_PRM]  = skin_depth();
}


/** @function checkProperties
 *
 *  Checks the input parameters (ie: negative length).
 *  Does not check for incompatibility between values as this depends on the line shape.
 **/
void TRANSLINE::checkProperties()
{
    // Do not check for values that are results of analyzing / synthesizing
    // Do not check for transline specific incompatibilities ( like " conductor height should be lesser than dielectric height")
    if( !std::isfinite( m_parameters[EPSILONR_PRM] ) || m_parameters[EPSILONR_PRM] <= 0 )
        setErrorLevel( EPSILONR_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[TAND_PRM] ) || m_parameters[TAND_PRM] < 0 )
        setErrorLevel( TAND_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[RHO_PRM] ) || m_parameters[RHO_PRM] < 0 )
        setErrorLevel( RHO_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[H_PRM] ) || m_parameters[H_PRM] < 0 )
        setErrorLevel( H_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[TWISTEDPAIR_TWIST_PRM] )
            || m_parameters[TWISTEDPAIR_TWIST_PRM] < 0 )
        setErrorLevel( TWISTEDPAIR_TWIST_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[STRIPLINE_A_PRM] ) || m_parameters[STRIPLINE_A_PRM] <= 0 )
        setErrorLevel( STRIPLINE_A_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[H_T_PRM] ) || m_parameters[H_T_PRM] <= 0 )
        setErrorLevel( H_T_PRM, TRANSLINE_WARNING );

    // How can we check ROUGH_PRM ?

    if( !std::isfinite( m_parameters[MUR_PRM] ) || m_parameters[MUR_PRM] < 0 )
        setErrorLevel( MUR_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[TWISTEDPAIR_EPSILONR_ENV_PRM] )
            || m_parameters[TWISTEDPAIR_EPSILONR_ENV_PRM] <= 0 )
        setErrorLevel( TWISTEDPAIR_EPSILONR_ENV_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[MURC_PRM] ) || m_parameters[MURC_PRM] < 0 )
        setErrorLevel( MURC_PRM, TRANSLINE_WARNING );

    if( !std::isfinite( m_parameters[FREQUENCY_PRM] ) || m_parameters[FREQUENCY_PRM] <= 0 )
        setErrorLevel( FREQUENCY_PRM, TRANSLINE_WARNING );
}

void TRANSLINE::analyze()
{
    getProperties();
    checkProperties();
    calcAnalyze();
    showAnalyze();
    show_results();
}

void TRANSLINE::synthesize()
{
    getProperties();
    checkProperties();
    calcSynthesize();
    showSynthesize();
    show_results();
}


/**
 * @function skin_depth
 * calculate skin depth
 *
 * \f$ \frac{1}{\sqrt{ \pi \cdot f \cdot \mu \cdot \sigma }} \f$
 */
#include <cstdio>
double TRANSLINE::skin_depth()
{
    double depth;
    depth = 1.0
            / sqrt( M_PI * m_parameters[FREQUENCY_PRM] * m_parameters[MURC_PRM] * MU0
                    * m_parameters[SIGMA_PRM] );
    return depth;
}


/* *****************************************************************
**********                                             **********
**********          mathematical functions             **********
**********                                             **********
***************************************************************** */

#define NR_EPSI 2.2204460492503131e-16

/* The function computes the complete elliptic integral of first kind
 *  K() and the second kind E() using the arithmetic-geometric mean
 *  algorithm (AGM) by Abramowitz and Stegun.
 */
void TRANSLINE::ellipke( double arg, double& k, double& e )
{
    int iMax = 16;

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

        a  = 1;
        b  = sqrt( 1 - da );
        c  = sqrt( da );
        fr = 0.5;
        s  = fr * c * c;

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
}


/* We need to know only K(k), and if possible KISS. */
double TRANSLINE::ellipk( double k )
{
    double r, lost;

    ellipke( k, r, lost );
    return r;
}

#define MAX_ERROR 0.000001

/**
 * @function minimizeZ0Error1D
 *
 * Tries to find a parameter that minimizes the error ( on Z0 ).
 * This function only works with a single parameter.
 * Calls @ref calcAnalyze several times until the error is acceptable.
 * While the error is unnacceptable, changes slightly the parameter.
 *
 * This function does not change Z0 / Angl_L.
 *
 * @param avar Parameter to synthesize
 * @return 'true' if error < MAX_ERROR, else 'false'
 */


bool TRANSLINE::minimizeZ0Error1D( double* aVar )
{
    double Z0_dest, Z0_current, Z0_result, angl_l_dest, increment, slope, error;
    int    iteration;

    if( !std::isfinite( m_parameters[Z0_PRM] ) )
    {
        *aVar = NAN;
        return false;
    }

    if( ( !std::isfinite( *aVar ) ) || ( *aVar == 0 ) )
        *aVar = 0.001;

    /* required value of Z0 */
    Z0_dest = m_parameters[Z0_PRM];

    /* required value of angl_l */
    angl_l_dest = m_parameters[ANG_L_PRM];

    /* Newton's method */
    iteration = 0;

    /* compute parameters */
    calcAnalyze();
    Z0_current = m_parameters[Z0_PRM];

    error = fabs( Z0_dest - Z0_current );

    while( error > MAX_ERROR )
    {
        iteration++;
        increment = *aVar / 100.0;
        *aVar += increment;
        /* compute parameters */
        calcAnalyze();
        Z0_result = m_parameters[Z0_PRM];
        /* f(w(n)) = Z0 - Z0(w(n)) */
        /* f'(w(n)) = -f'(Z0(w(n))) */
        /* f'(Z0(w(n))) = (Z0(w(n)) - Z0(w(n+delw))/delw */
        /* w(n+1) = w(n) - f(w(n))/f'(w(n)) */
        slope = ( Z0_result - Z0_current ) / increment;
        slope = ( Z0_dest - Z0_current ) / slope - increment;
        *aVar += slope;

        if( *aVar <= 0.0 )
            *aVar = increment;

        /* find new error */
        /* compute parameters */
        calcAnalyze();
        Z0_current = m_parameters[Z0_PRM];
        error      = fabs( Z0_dest - Z0_current );

        if( iteration > 100 )
            break;
    }

    /* Compute one last time, but with correct length */
    m_parameters[Z0_PRM]       = Z0_dest;
    m_parameters[ANG_L_PRM]    = angl_l_dest;
    m_parameters[PHYS_LEN_PRM] = C0 / m_parameters[FREQUENCY_PRM]
                                 / sqrt( m_parameters[EPSILON_EFF_PRM] ) * m_parameters[ANG_L_PRM]
                                 / 2.0 / M_PI; /* in m */
    calcAnalyze();

    /* Restore parameters */
    m_parameters[Z0_PRM]       = Z0_dest;
    m_parameters[ANG_L_PRM]    = angl_l_dest;
    m_parameters[PHYS_LEN_PRM] = C0 / m_parameters[FREQUENCY_PRM]
                                 / sqrt( m_parameters[EPSILON_EFF_PRM] ) * m_parameters[ANG_L_PRM]
                                 / 2.0 / M_PI; /* in m */
    return error <= MAX_ERROR;
}
/**
 * @function setErrorLevel
 *
 * set an error / warning level for a given parameter.
 *
 * @see TRANSLINE_OK
 * @see TRANSLINE_WARNING
 * @see TRANSLINE_ERROR
 *
 * @param aP parameter
 * @param aErrorLevel Error level
 */
void TRANSLINE::setErrorLevel( PRMS_ID aP, char aErrorLevel )
{
    switch( aErrorLevel )
    {
    case TRANSLINE_WARNING:  SetPropertyBgColorInDialog( aP, &warnCol );  break;
    case TRANSLINE_ERROR:    SetPropertyBgColorInDialog( aP, &errCol );   break;
    default:                 SetPropertyBgColorInDialog( aP, &okCol );    break;
    }
}


double TRANSLINE::calcUnitPropagationDelay( const double epsilonEff )
{
    return std::sqrt( epsilonEff ) * ( 1.0e10 / 2.99e8 );
}


char TRANSLINE::convertParameterStatusCode( TRANSLINE_STATUS aStatus )
{
    switch( aStatus )
    {
    case TRANSLINE_STATUS::OK: return TRANSLINE_OK;
    case TRANSLINE_STATUS::WARNING: return TRANSLINE_WARNING;
    case TRANSLINE_STATUS::TS_ERROR: return TRANSLINE_ERROR;
    }

    return TRANSLINE_OK;
}
