/*
 * microstrip.cpp - microstrip class implementation
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2018 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


/* microstrip.c - Puts up window for microstrip and
 * performs the associated calculations
 * Based on the original microstrip.c by Gopal Narayanan
 */

#include "microstrip.h"
#include "transline.h"
#include "units.h"


MICROSTRIP_UI::MICROSTRIP_UI()
{
    m_Name = "MicroStrip";
    Init();
}


void MICROSTRIP_UI::calcAnalyze()
{
    m_calc.Analyse();
}


void MICROSTRIP_UI::showAnalyze()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setProperty( Z0_PRM, results[TRANSLINE_PARAMETERS::Z0].first );
    setProperty( ANG_L_PRM, results[TRANSLINE_PARAMETERS::ANG_L].first );

    setResult( 0, results[TRANSLINE_PARAMETERS::EPSILON_EFF].first, "" );
    setResult( 1, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].first, "ps/cm" );
    setResult( 2, results[TRANSLINE_PARAMETERS::ATTEN_COND].first, "dB" );
    setResult( 3, results[TRANSLINE_PARAMETERS::ATTEN_DILECTRIC].first, "dB" );
    setResult( 4, results[TRANSLINE_PARAMETERS::SKIN_DEPTH].first / UNIT_MICRON, "µm" );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_WIDTH_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
}


void MICROSTRIP_UI::showSynthesize()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetSynthesisResults();

    setProperty( PHYS_WIDTH_PRM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first );
    setProperty( PHYS_LEN_PRM, results[TRANSLINE_PARAMETERS::PHYS_LEN].first );
    setProperty( Z0_PRM, results[TRANSLINE_PARAMETERS::Z0].first );
    setProperty( ANG_L_PRM, results[TRANSLINE_PARAMETERS::ANG_L].first );

    setResult( 0, results[TRANSLINE_PARAMETERS::EPSILON_EFF].first, "" );
    setResult( 1, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].first, "ps/cm" );
    setResult( 2, results[TRANSLINE_PARAMETERS::ATTEN_COND].first, "dB" );
    setResult( 3, results[TRANSLINE_PARAMETERS::ATTEN_DILECTRIC].first, "dB" );
    setResult( 4, results[TRANSLINE_PARAMETERS::SKIN_DEPTH].first / UNIT_MICRON, "µm" );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_WIDTH_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
}


void MICROSTRIP_UI::getProperties()
{
    TRANSLINE::getProperties();

    m_calc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, m_parameters[SIGMA_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILON_EFF, m_parameters[EPSILON_EFF_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, m_parameters[SKIN_DEPTH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, m_parameters[EPSILONR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::H_T, m_parameters[H_T_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::H, m_parameters[H_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, m_parameters[PHYS_WIDTH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::T, m_parameters[T_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::Z0, m_parameters[Z0_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, m_parameters[FREQUENCY_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::ROUGH, m_parameters[ROUGH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::TAND, m_parameters[TAND_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, m_parameters[PHYS_LEN_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MUR, m_parameters[MUR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MURC, m_parameters[MURC_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, m_parameters[ANG_L_PRM] );
}


void MICROSTRIP_UI::show_results()
{
}


/*
 * synthesis function
 */
void MICROSTRIP_UI::calcSynthesize()
{
    m_calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
}
