/*
 * coplanar.cpp - coplanar UI wrapper
 *
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
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


#include "coplanar.h"
#include "units.h"


COPLANAR_UI::COPLANAR_UI()
{
    m_Name = "CoPlanar";
    Init();
}


GROUNDEDCOPLANAR_UI::GROUNDEDCOPLANAR_UI()
{
    m_Name = "GrCoPlanar";
}


void COPLANAR_UI::getProperties()
{
    TRANSLINE::getProperties();

    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, m_parameters[EPSILONR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::DIELECTRIC_MODEL_SEL, m_parameters[DIELECTRIC_MODEL_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR_SPEC_FREQ, m_parameters[EPSILONR_SPEC_FREQ_PRM] );
    pushSoldermaskParameters( m_calc, /* aIncludeFillsGaps */ true );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::TAND, m_parameters[TAND_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, m_parameters[SIGMA_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MURC, m_parameters[MURC_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, m_parameters[PHYS_WIDTH_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_S, m_parameters[PHYS_S_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::H, m_parameters[H_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::T, m_parameters[T_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, m_parameters[PHYS_LEN_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, m_parameters[FREQUENCY_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::Z0, m_parameters[Z0_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, m_parameters[ANG_L_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, m_parameters[SKIN_DEPTH_PRM] );

    // Ungrounded CPW.  The grounded subclass overrides this to flip the flag.
    m_calc.SetParameter( TRANSLINE_PARAMETERS::CPW_BACKMETAL, 0.0 );

    if( isSelected( PHYS_WIDTH_PRM ) )
        m_calc.SetSynthesizeTarget( TRANSLINE_PARAMETERS::PHYS_WIDTH );
    else if( isSelected( PHYS_S_PRM ) )
        m_calc.SetSynthesizeTarget( TRANSLINE_PARAMETERS::PHYS_S );
}


void GROUNDEDCOPLANAR_UI::getProperties()
{
    COPLANAR_UI::getProperties();
    m_calc.SetParameter( TRANSLINE_PARAMETERS::CPW_BACKMETAL, 1.0 );
}


void COPLANAR_UI::calcAnalyze()
{
    m_calc.Analyse();
}


void COPLANAR_UI::calcSynthesize()
{
    m_calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
}


void COPLANAR_UI::showAnalyze()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setProperty( Z0_PRM, results[TRANSLINE_PARAMETERS::Z0].first );
    setProperty( ANG_L_PRM, results[TRANSLINE_PARAMETERS::ANG_L].first );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_WIDTH_PRM,
                   convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_S_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void COPLANAR_UI::showSynthesize()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetSynthesisResults();

    if( isSelected( PHYS_WIDTH_PRM ) )
        setProperty( PHYS_WIDTH_PRM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first );

    if( isSelected( PHYS_S_PRM ) )
        setProperty( PHYS_S_PRM, results[TRANSLINE_PARAMETERS::PHYS_S].first );

    setProperty( PHYS_LEN_PRM, results[TRANSLINE_PARAMETERS::PHYS_LEN].first );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_WIDTH_PRM,
                   convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_S_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void COPLANAR_UI::show_results()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setResult( 0, results[TRANSLINE_PARAMETERS::EPSILON_EFF].first, "" );
    setResult( 1, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].first, "ps/cm" );
    setResult( 2, results[TRANSLINE_PARAMETERS::LOSS_CONDUCTOR].first, "dB" );
    setResult( 3, results[TRANSLINE_PARAMETERS::LOSS_DIELECTRIC].first, "dB" );
    setResult( 4, results[TRANSLINE_PARAMETERS::SKIN_DEPTH].first / UNIT_MICRON, "µm" );
}
