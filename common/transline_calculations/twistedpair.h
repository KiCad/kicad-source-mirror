/*
 * Copyright (C) 2011 Michael Margraf <michael.margraf@alumni.tu-berlin.de>
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

#ifndef TRANSLINE_CALCULATIONS_TWISTEDPAIR_H
#define TRANSLINE_CALCULATIONS_TWISTEDPAIR_H


#include <transline_calculations/transline_calculation_base.h>


/**
 * Twisted-pair transmission line calculation.
 *
 * Implements the Lefferson 1971 closed-form model: pitch angle θ = atan(T·π·Dout),
 * effective permittivity εeff = εr_env + (0.25 + 0.0007·θ²)·(εr − εr_env), and
 * characteristic impedance Z0 = (ZF0/π)·cosh⁻¹(Dout/Din)/√εeff.  The reported Z0 is the
 * differential impedance of the pair because Lefferson's model is inherently a two-wire
 * model.  Loss is skin-effect conductor loss plus standard tan δ dielectric loss.
 *
 * Synthesis uses 1-D Newton iteration on either PHYS_DIAM_IN or PHYS_DIAM_OUT to hit a
 * target Z0.  The UI picks which diameter is the unknown via SetSynthesizeTarget; the
 * default matches the legacy behaviour and solves for PHYS_DIAM_OUT.
 *
 * Reference: P. Lefferson, "Twisted Magnet Wire Transmission Line", IEEE Transactions on
 * Parts, Hybrids, and Packaging, vol. PHP-7, no. 4, pp. 148-154, Dec. 1971.
 */
class TWISTEDPAIR : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    TWISTEDPAIR() :
            TRANSLINE_CALCULATION_BASE( { TCP::EPSILONR, TCP::TAND, TCP::SIGMA, TCP::MURC,
                                          TCP::PHYS_DIAM_IN, TCP::PHYS_DIAM_OUT, TCP::PHYS_LEN,
                                          TCP::FREQUENCY, TCP::Z0, TCP::ANG_L, TCP::LOSS_CONDUCTOR,
                                          TCP::LOSS_DIELECTRIC, TCP::SKIN_DEPTH, TCP::EPSILON_EFF,
                                          TCP::UNIT_PROP_DELAY, TCP::TWISTEDPAIR_TWIST,
                                          TCP::TWISTEDPAIR_EPSILONR_ENV,
                                          TCP::DIELECTRIC_MODEL_SEL, TCP::EPSILONR_SPEC_FREQ } )
    {
        m_synthesizeTarget = TCP::PHYS_DIAM_OUT;
    }

    /// Analyse pair geometry to output Z0, electrical length, losses, skin depth, εeff
    void Analyse() override;

    /// Synthesize Din or Dout to hit the target Z0.  Length is recomputed from ANG_L.
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

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;
};


#endif // TRANSLINE_CALCULATIONS_TWISTEDPAIR_H
