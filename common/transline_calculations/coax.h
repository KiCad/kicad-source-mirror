/*
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2002 Claudio Girardi <claudio.girardi@ieee.org>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2018 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#ifndef TRANSLINE_CALCULATIONS_COAX_H
#define TRANSLINE_CALCULATIONS_COAX_H


#include <string>

#include <transline_calculations/transline_calculation_base.h>


/**
 * Coaxial transmission line calculation (TEM mode).
 *
 * Supports analysis of characteristic impedance, electrical length, conductor and
 * dielectric loss, skin depth, and higher-order mode cutoffs (TE_1m, TM_0m) per
 * Pozar, "Microwave Engineering" 4th ed., section 3.5.  Synthesis solves for either
 * the inner or outer diameter given a target characteristic impedance.
 */
class COAX : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    COAX() :
            TRANSLINE_CALCULATION_BASE( { TCP::SKIN_DEPTH, TCP::EPSILONR, TCP::TAND, TCP::PHYS_DIAM_IN,
                                          TCP::PHYS_DIAM_OUT, TCP::MUR, TCP::MURC, TCP::SIGMA, TCP::FREQUENCY,
                                          TCP::PHYS_LEN, TCP::Z0, TCP::ANG_L, TCP::LOSS_CONDUCTOR,
                                          TCP::LOSS_DIELECTRIC, TCP::CUTOFF_FREQUENCY,
                                          TCP::DIELECTRIC_MODEL_SEL, TCP::EPSILONR_SPEC_FREQ } )
    {
        m_synthesizeTarget = TCP::PHYS_DIAM_IN;
    }

    /// Analyse cable geometry parameters to output Z0, electrical length, losses and cutoffs
    void Analyse() override;

    /// Synthesize the cable geometry to match the given Z0 target
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

    /**
     * Choose which geometry parameter will be solved for during synthesis.
     *
     * Accepts PHYS_DIAM_IN or PHYS_DIAM_OUT.  Any other value is ignored.
     */
    void SetSynthesizeTarget( TRANSLINE_PARAMETERS aTarget ) override
    {
        if( aTarget == TCP::PHYS_DIAM_IN || aTarget == TCP::PHYS_DIAM_OUT )
            m_synthesizeTarget = aTarget;
    }

    /// Returns a UI-friendly string enumerating propagating TE_1m modes at the current frequency
    std::string GetTEModes() const { return m_teModes; }

    /// Returns a UI-friendly string enumerating propagating TM_0m modes at the current frequency
    std::string GetTMModes() const { return m_tmModes; }

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// Dielectric loss per unit length, in dB/m
    double AlphaD() const;

    /// Conductor loss per unit length, in dB/m
    double AlphaC() const;

    /// Populates the TE / TM mode cutoff display strings and CUTOFF_FREQUENCY from current geometry
    void UpdateModeCutoffs();

    /// Cached TE_1m propagating-modes string produced by UpdateModeCutoffs
    std::string m_teModes;

    /// Cached TM_0m propagating-modes string produced by UpdateModeCutoffs
    std::string m_tmModes;
};


#endif // TRANSLINE_CALCULATIONS_COAX_H
