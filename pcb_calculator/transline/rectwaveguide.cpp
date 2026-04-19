/*
 * rectwaveguide.cpp - rectangular waveguide UI wrapper
 *
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
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
 *
 */

#include <cmath>

#include "rectwaveguide.h"
#include "units.h"


RECTWAVEGUIDE_UI::RECTWAVEGUIDE_UI()
{
    m_Name = "RectWaveGuide";
    Init();
}


void RECTWAVEGUIDE_UI::getProperties()
{
    TRANSLINE::getProperties();

    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, m_parameters[EPSILONR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::DIELECTRIC_MODEL_SEL, m_parameters[DIELECTRIC_MODEL_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR_SPEC_FREQ, m_parameters[EPSILONR_SPEC_FREQ_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::TAND, m_parameters[TAND_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, m_parameters[SIGMA_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MUR, m_parameters[MUR_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::MURC, m_parameters[MURC_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, m_parameters[PHYS_A_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_S, m_parameters[PHYS_B_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, m_parameters[PHYS_LEN_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, m_parameters[FREQUENCY_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::Z0, m_parameters[Z0_PRM] );
    m_calc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, m_parameters[ANG_L_PRM] );

    if( isSelected( PHYS_A_PRM ) )
        m_calc.SetSynthesizeTarget( TRANSLINE_PARAMETERS::PHYS_WIDTH );
}


void RECTWAVEGUIDE_UI::calcAnalyze()
{
    m_calc.Analyse();
}


void RECTWAVEGUIDE_UI::calcSynthesize()
{
    m_calc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
}


void RECTWAVEGUIDE_UI::showAnalyze()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    setProperty( Z0_PRM, results[TRANSLINE_PARAMETERS::Z0].first );
    setProperty( ANG_L_PRM, results[TRANSLINE_PARAMETERS::ANG_L].first );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_A_PRM,
                   convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_B_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void RECTWAVEGUIDE_UI::showSynthesize()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetSynthesisResults();

    if( isSelected( PHYS_A_PRM ) )
        setProperty( PHYS_A_PRM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first );

    if( isSelected( PHYS_B_PRM ) )
        setProperty( PHYS_B_PRM, results[TRANSLINE_PARAMETERS::PHYS_S].first );

    setProperty( PHYS_LEN_PRM, results[TRANSLINE_PARAMETERS::PHYS_LEN].first );

    setErrorLevel( Z0_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::Z0].second ) );
    setErrorLevel( ANG_L_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::ANG_L].second ) );
    setErrorLevel( PHYS_LEN_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_LEN].second ) );
    setErrorLevel( PHYS_A_PRM,
                   convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second ) );
    setErrorLevel( PHYS_B_PRM, convertParameterStatusCode( results[TRANSLINE_PARAMETERS::PHYS_S].second ) );
}


void RECTWAVEGUIDE_UI::show_results()
{
    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            m_calc.GetAnalysisResults();

    // Field-quantity impedance Z0EH = Ey / Hx for TE10 above cutoff, matching the legacy
    // "ZF(H10) = Ey / Hx:" row.  In terms of the analysis outputs, Z0EH = ZF0 / sqrt(eps_eff)
    // (eps_eff = 1 - (fc/f)^2 here).  Below cutoff eps_eff is 0 and we fall back to zero.
    const double epsEff = results[TRANSLINE_PARAMETERS::EPSILON_EFF].first;
    const double z0eh = ( epsEff > 0.0 ) ? 376.730313668 / std::sqrt( epsEff ) : 0.0;

    setResult( 0, z0eh, "Ohm" );
    setResult( 1, results[TRANSLINE_PARAMETERS::EPSILON_EFF].first, "" );
    setResult( 2, results[TRANSLINE_PARAMETERS::LOSS_CONDUCTOR].first, "dB" );
    setResult( 3, results[TRANSLINE_PARAMETERS::LOSS_DIELECTRIC].first, "dB" );

    std::string teText = m_calc.GetTEModes();
    std::string tmText = m_calc.GetTMModes();

    if( teText.empty() )
        teText = "none";

    if( tmText.empty() )
        tmText = "none";

    setResult( 4, teText.c_str() );
    setResult( 5, tmText.c_str() );
}
