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

#ifndef TRANSLINE_CALCULATIONS_STRIPLINE_H
#define TRANSLINE_CALCULATIONS_STRIPLINE_H


#include <transline_calculations/transline_calculation_base.h>


class STRIPLINE : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    STRIPLINE() :
            TRANSLINE_CALCULATION_BASE( { TCP::SKIN_DEPTH, TCP::EPSILON_EFF, TCP::EPSILONR, TCP::T, TCP::STRIPLINE_A,
                                          TCP::H, TCP::Z0, TCP::LOSS_CONDUCTOR, TCP::PHYS_LEN, TCP::LOSS_DIELECTRIC,
                                          TCP::FREQUENCY, TCP::TAND, TCP::PHYS_WIDTH, TCP::UNIT_PROP_DELAY, TCP::ANG_L,
                                          TCP::SIGMA, TCP::MURC } ),
            unit_prop_delay( 0.0 )
    {
    }

    /// Analyse track geometry parameters to output Z0 and Ang_L
    void Analyse() override;

    /// Synthesis track geometry parameters to match given Z0
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// Calculate characteristic impedance and conductor loss (in db/meter)
    double lineImpedance( double aHeight, double& aAc ) const;

    double unit_prop_delay;
};


#endif //TRANSLINE_CALCULATIONS_STRIPLINE_H
