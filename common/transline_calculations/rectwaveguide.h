/*
 * Copyright (C) 2001 Gopal Narayanan <gopal@astro.umass.edu>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2015 jean-pierre.charras
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

#ifndef TRANSLINE_CALCULATIONS_RECTWAVEGUIDE_H
#define TRANSLINE_CALCULATIONS_RECTWAVEGUIDE_H


#include <string>

#include <transline_calculations/transline_calculation_base.h>


/**
 * Rectangular waveguide calculation.
 *
 * PHYS_WIDTH is the broad dimension a, PHYS_S is the narrow dimension b.  Analysis
 * evaluates the TE10 dominant-mode characteristic impedance (fictive-voltage definition),
 * electrical length, conductor loss (Ramo / Whinnery / Van Duzer summation over every
 * propagating TE_mn and TM_mn mode), and dielectric loss.  Below TE10 cutoff the
 * conductor-loss term collapses to the evanescent attenuation per unit length.
 *
 * Synthesis is the closed-form inverse of the TE10 Z0 expression and solves for a from
 * a target Z0; the narrow dimension b is a free input with no synthesis path (choosing b
 * is a power-handling / higher-mode-suppression tradeoff rather than a Z0 constraint).
 *
 * CUTOFF_FREQUENCY is populated with the TE10 cutoff so the UI can display it alongside
 * the propagating-mode string accessors GetTEModes / GetTMModes.
 */
class RECTWAVEGUIDE : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    RECTWAVEGUIDE() :
            TRANSLINE_CALCULATION_BASE( { TCP::EPSILONR, TCP::TAND, TCP::SIGMA, TCP::MUR, TCP::MURC,
                                          TCP::PHYS_WIDTH, TCP::PHYS_S, TCP::PHYS_LEN, TCP::FREQUENCY,
                                          TCP::Z0, TCP::ANG_L, TCP::LOSS_CONDUCTOR, TCP::LOSS_DIELECTRIC,
                                          TCP::CUTOFF_FREQUENCY, TCP::SKIN_DEPTH, TCP::EPSILON_EFF,
                                          TCP::DIELECTRIC_MODEL_SEL, TCP::EPSILONR_SPEC_FREQ } )
    {
        m_synthesizeTarget = TCP::PHYS_WIDTH;
    }

    /// Analyse waveguide geometry to produce Z0, electrical length, loss, and mode cutoffs
    void Analyse() override;

    /// Synthesize the broad dimension a from a target Z0.  Only PHYS_WIDTH is a valid target.
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

    /**
     * Choose which geometry parameter will be solved for during synthesis.
     *
     * Only PHYS_WIDTH (a) is supported.  b is a free input because the TE10 Z0
     * expression is independent of b.  Any other value is ignored.
     */
    void SetSynthesizeTarget( TRANSLINE_PARAMETERS aTarget ) override
    {
        if( aTarget == TCP::PHYS_WIDTH )
            m_synthesizeTarget = aTarget;
    }

    /// Returns a UI-friendly string enumerating propagating TE_mn modes at the current frequency
    std::string GetTEModes() const { return m_teModes; }

    /// Returns a UI-friendly string enumerating propagating TM_mn modes at the current frequency
    std::string GetTMModes() const { return m_tmModes; }

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// Square of the free-space wavenumber k = omega * sqrt(mu * eps)
    double KvalSquare() const;

    /// Square of the transverse cutoff wavenumber kc for mode (m, n)
    double KcSquare( int aM, int aN ) const;

    /// Cutoff frequency for mode (m, n), in Hz
    double Fc( int aM, int aN ) const;

    /// Conductor loss summed over all propagating TE_mn and TM_mn modes, in dB/m
    double AlphaC() const;

    /// Evanescent attenuation below TE10 cutoff, in dB/m
    double AlphaCCutoff() const;

    /// Dielectric loss of the dominant TE10 mode, in dB/m
    double AlphaD() const;

    /// Populates m_teModes and m_tmModes with all propagating modes at the current frequency
    void UpdateModeStrings();

    /// Cached TE_mn propagating-modes string produced by UpdateModeStrings
    std::string m_teModes;

    /// Cached TM_mn propagating-modes string produced by UpdateModeStrings
    std::string m_tmModes;
};


#endif // TRANSLINE_CALCULATIONS_RECTWAVEGUIDE_H
