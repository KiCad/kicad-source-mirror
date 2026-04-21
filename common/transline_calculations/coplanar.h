/*
 * Copyright (C) 2008 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
 * Copyright (C) 2005, 2006 Stefan Jahn <stefan@lkcc.org>
 * Modified for Kicad: 2011 jean-pierre.charras
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

#ifndef TRANSLINE_CALCULATIONS_COPLANAR_H
#define TRANSLINE_CALCULATIONS_COPLANAR_H


#include <transline_calculations/transline_calculation_base.h>


/**
 * Coplanar waveguide (CPW) and conductor-backed coplanar waveguide (CBCPW) calculation.
 *
 * The CPW_BACKMETAL parameter selects between the two variants.  A value of 0.0 models an
 * ungrounded CPW (air-backed); a value of 1.0 models a grounded CPW with a continuous back
 * plane at depth H.  Analysis uses the standard conformal-mapping quasi-static solution
 * with a finite-thickness correction for the conductor, Ghione-style conductor loss, and an
 * ad-hoc quadratic dispersion term for the effective permittivity.  Synthesis solves for
 * either PHYS_WIDTH or PHYS_S to match a target Z0.
 */
class COPLANAR : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    COPLANAR() :
            TRANSLINE_CALCULATION_BASE( { TCP::EPSILONR, TCP::TAND, TCP::SIGMA, TCP::MURC, TCP::PHYS_WIDTH, TCP::PHYS_S,
                                          TCP::H, TCP::T, TCP::PHYS_LEN, TCP::FREQUENCY, TCP::Z0, TCP::ANG_L,
                                          TCP::LOSS_CONDUCTOR, TCP::LOSS_DIELECTRIC, TCP::SKIN_DEPTH, TCP::EPSILON_EFF,
                                          TCP::UNIT_PROP_DELAY, TCP::CPW_BACKMETAL,
                                          TCP::DIELECTRIC_MODEL_SEL, TCP::EPSILONR_SPEC_FREQ,
                                          TCP::SOLDERMASK_PRESENT, TCP::SOLDERMASK_THICKNESS,
                                          TCP::SOLDERMASK_EPSILONR, TCP::SOLDERMASK_TAND,
                                          TCP::SOLDERMASK_FILLS_GAPS } )
    {
        m_synthesizeTarget = TCP::PHYS_WIDTH;
    }

    /// Analyse trace geometry to produce Z0, electrical length, effective permittivity, and losses
    void Analyse() override;

    /// Synthesize the unknown geometry parameter to match the Z0 target.
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

    /**
     * Coplanar waveguide soldermask filling factor.
     *
     * CPW without back metal and mask filling the slots: g = 0.5 (mask occupies the top
     * half of the coplanar slot; substrate + air share the bottom half via q2/q1).  CPW
     * with mask over traces only (gaps left air-filled): fall back to the microstrip
     * Hammerstad weight because the effective upper cover is the mask layer above the
     * strip plus unperturbed air over the slots.  CBCPW: Wadell practical approximation
     * g_cbcpw = 0.5 (g_cpw + g_microstrip) blends the ground-backed confinement toward
     * the microstrip limit.
     */
    double GetSoldermaskDeltaQ( double aWOverH, double aCOverH ) const override;

    /**
     * Choose which geometry parameter will be solved for during synthesis.
     *
     * Accepts PHYS_WIDTH or PHYS_S.  Any other value is ignored.
     */
    void SetSynthesizeTarget( TRANSLINE_PARAMETERS aTarget ) override
    {
        if( aTarget == TCP::PHYS_WIDTH || aTarget == TCP::PHYS_S )
            m_synthesizeTarget = aTarget;
    }

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// True when CPW_BACKMETAL reads as grounded (CBCPW)
    bool hasBackMetal() const { return GetParameter( TCP::CPW_BACKMETAL ) >= 0.5; }
};


#endif // TRANSLINE_CALCULATIONS_COPLANAR_H
