/*
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
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
 */


#include "c_stripline.h"
#include "transline.h"
#include "units.h"


C_STRIPLINE::C_STRIPLINE()
{
    m_Name = "Coupled_MicroStrip";
    Init();
}


void C_STRIPLINE::calcAnalyze()
{
    m_calc.Analyse();
}


void C_STRIPLINE::calcSynthesize()
{
    m_calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
}


void C_STRIPLINE::showAnalyze()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setProperty( Z0_E_PRM, results[TRANSLINE_PARAMETERS::Z0_E].first );
    setProperty( Z0_O_PRM, results[TRANSLINE_PARAMETERS::Z0_O].first );
    setProperty( ANG_L_PRM, results[TRANSLINE_PARAMETERS::ANG_L].first );

    setResult( 0, results[TRANSLINE_PARAMETERS::EPSILON_EFF_EVEN].first, "" );
    setResult( 1, results[TRANSLINE_PARAMETERS::EPSILON_EFF_ODD].first, "" );
    setResult( 2, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_EVEN].first, "ps/cm" );
    setResult( 3, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].first, "ps/cm" );
    setResult( 4, results[TRANSLINE_PARAMETERS::SKIN_DEPTH].first / UNIT_MICRON, "µm" );
    setResult( 5, results[TRANSLINE_PARAMETERS::Z_DIFF].first, "Ω" );

    setErrorLevel( Z0_E_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0_E].second ) );
    setErrorLevel( Z0_O_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0_O].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_WIDTH_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_S_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void C_STRIPLINE::showSynthesize()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setProperty( PHYS_WIDTH_PRM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first );
    setProperty( PHYS_S_PRM, results[TRANSLINE_PARAMETERS::PHYS_S].first );
    setProperty( PHYS_LEN_PRM, results[TRANSLINE_PARAMETERS::PHYS_LEN].first );

    setResult( 0, results[TRANSLINE_PARAMETERS::EPSILON_EFF_EVEN].first, "" );
    setResult( 1, results[TRANSLINE_PARAMETERS::EPSILON_EFF_ODD].first, "" );
    setResult( 2, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_EVEN].first, "ps/cm" );
    setResult( 3, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].first, "ps/cm" );
    setResult( 4, results[TRANSLINE_PARAMETERS::SKIN_DEPTH].first / UNIT_MICRON, "µm" );
    setResult( 5, results[TRANSLINE_PARAMETERS::Z_DIFF].first, "Ω" );

    setErrorLevel( Z0_E_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0_E].second ) );
    setErrorLevel( Z0_O_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0_O].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_WIDTH_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_S_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void C_STRIPLINE::getProperties()
{
    TRANSLINE::getProperties();

    m_calc.SetParameter( TRANSLINE_PARAMETERS::Z0_E, m_parameters[Z0_E_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::Z0_O, m_parameters[Z0_O_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, m_parameters[EPSILONR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, m_parameters[PHYS_WIDTH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, m_parameters[PHYS_LEN_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_S, m_parameters[PHYS_S_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::H, m_parameters[H_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::T, m_parameters[T_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, m_parameters[FREQUENCY_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MURC, m_parameters[MURC_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, m_parameters[SKIN_DEPTH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, m_parameters[SIGMA_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, m_parameters[ANG_L_PRM] );
}

void C_STRIPLINE::show_results()
{
}
