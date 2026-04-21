
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

#include <wx/colour.h>
#include <wx/settings.h>

#include "transline.h"
#include "units.h"

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

    // 1 GHz spec defaults to the canonical FR-4 reporting point for Er and tan delta.
    m_parameters[DIELECTRIC_MODEL_PRM] = 0.0;
    m_parameters[EPSILONR_SPEC_FREQ_PRM] = 1.0e9;

    // Soldermask defaults.  SOLDERMASK_PRESENT = 0 keeps the math path bit-identical to
    // the un-coated baseline.  Thickness 20 um, er 3.5, tan d 0.025 match a typical green
    // LPI; fills-gaps defaults on because standard LPI processes flood the CPW slots.
    m_parameters[SOLDERMASK_PRESENT_PRM] = 0.0;
    m_parameters[SOLDERMASK_THICKNESS_PRM] = 20.0e-6;
    m_parameters[SOLDERMASK_EPSILONR_PRM] = 3.5;
    m_parameters[SOLDERMASK_TAND_PRM] = 0.025;
    m_parameters[SOLDERMASK_FILLS_GAPS_PRM] = 1.0;
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

    // Warn on non-positive or non-finite H_T, and also on the regime where the cover
    // correction breaks down (air gap below roughly the conductor thickness plus 10 percent
    // of the substrate height).
    if( !std::isfinite( m_parameters[H_T_PRM] ) || m_parameters[H_T_PRM] <= 0
            || ( std::isfinite( m_parameters[H_PRM] ) && m_parameters[H_PRM] > 0
                 && std::isfinite( m_parameters[T_PRM] )
                 && m_parameters[H_T_PRM] < ( m_parameters[T_PRM] + 0.1 * m_parameters[H_PRM] ) ) )
    {
        setErrorLevel( H_T_PRM, TRANSLINE_WARNING );
    }

    // Wadell Sec. 3.6.3 (off-center stripline) claims +/-2 percent accuracy only when the strip
    // plane sits inside 0.2 < a/h < 0.8 and the strip-thickness ratio t/h stays below 0.2.  Flag
    // STRIPLINE_A_PRM and T_PRM when we leave that regime so the UI can warn that the offset
    // correction is extrapolating outside its fit range.
    if( std::isfinite( m_parameters[STRIPLINE_A_PRM] )
            && m_parameters[STRIPLINE_A_PRM] > 0
            && std::isfinite( m_parameters[H_PRM] )
            && m_parameters[H_PRM] > 0 )
    {
        const double aRatio = m_parameters[STRIPLINE_A_PRM] / m_parameters[H_PRM];

        if( aRatio < 0.2 || aRatio > 0.8 )
            setErrorLevel( STRIPLINE_A_PRM, TRANSLINE_WARNING );
    }

    if( std::isfinite( m_parameters[T_PRM] )
            && m_parameters[T_PRM] > 0
            && std::isfinite( m_parameters[H_PRM] )
            && m_parameters[H_PRM] > 0
            && ( m_parameters[T_PRM] / m_parameters[H_PRM] ) > 0.2 )
    {
        setErrorLevel( T_PRM, TRANSLINE_WARNING );
    }

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
double TRANSLINE::skin_depth()
{
    double depth;
    depth = 1.0
            / sqrt( M_PI * m_parameters[FREQUENCY_PRM] * m_parameters[MURC_PRM] * MU0
                    * m_parameters[SIGMA_PRM] );
    return depth;
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


void TRANSLINE::pushSoldermaskParameters( TRANSLINE_CALCULATION_BASE& aCalc,
                                          bool aIncludeFillsGaps ) const
{
    using TCP = TRANSLINE_PARAMETERS;

    aCalc.SetParameter( TCP::SOLDERMASK_PRESENT, m_parameters[SOLDERMASK_PRESENT_PRM] );
    aCalc.SetParameter( TCP::SOLDERMASK_THICKNESS, m_parameters[SOLDERMASK_THICKNESS_PRM] );
    aCalc.SetParameter( TCP::SOLDERMASK_EPSILONR, m_parameters[SOLDERMASK_EPSILONR_PRM] );
    aCalc.SetParameter( TCP::SOLDERMASK_TAND, m_parameters[SOLDERMASK_TAND_PRM] );

    if( aIncludeFillsGaps )
        aCalc.SetParameter( TCP::SOLDERMASK_FILLS_GAPS, m_parameters[SOLDERMASK_FILLS_GAPS_PRM] );
}
