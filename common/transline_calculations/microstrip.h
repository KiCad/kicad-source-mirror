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

#ifndef TRANSLINE_CALCULATIONS_MICROSTRIP_H
#define TRANSLINE_CALCULATIONS_MICROSTRIP_H


#include <transline_calculations/transline_calculation_base.h>


class MICROSTRIP : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    MICROSTRIP() :
            TRANSLINE_CALCULATION_BASE( { TCP::EPSILONR, TCP::H_T, TCP::H, TCP::PHYS_WIDTH, TCP::T, TCP::Z0,
                                          TCP::FREQUENCY, TCP::EPSILON_EFF, TCP::SKIN_DEPTH, TCP::SIGMA, TCP::ROUGH,
                                          TCP::TAND, TCP::PHYS_LEN, TCP::MUR, TCP::MURC, TCP::ANG_L,
                                          TCP::UNIT_PROP_DELAY, TCP::ATTEN_COND, TCP::ATTEN_DILECTRIC } )
    {
    }

    friend class COUPLED_MICROSTRIP;

    /// Analyse track geometry parameters to output Z0 and Ang_L
    void Analyse() override;

    /// Synthesis track geometry parameters to match given Z0
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// Calculates the width with the current set of parameters
    double SynthesizeWidth() const;

    /// Calculates the dispersion correction factor for the  characteristic impedance static
    static double Z0_dispersion( double, double, double, double, double );

    /// Calculates the impedance for a stripline in a  homogeneous medium, without cover effects
    static double Z0_homogeneous( double );

    /// Calculates the cover effect on impedance for a  stripline in a homogeneous medium
    static double delta_Z0_cover( double, double );

    /// Calculates the filling factor for a microstrip without cover and zero conductor thickness
    static double filling_factor( double, double );

    /// Calculates the cover effect on filling factor
    static double delta_q_cover( double );

    /// Calculates the thickness effect on filling factor
    static double delta_q_thickness( double, double );

    /// Calculates effective dielectric constant from material e_r and filling factor
    static double e_r_effective( double, double );

    /// Calculates the thickness effect on normalized width
    static double delta_u_thickness( double, double, double );

    /// Calculates the dispersion correction factor for the effective permeability
    static double e_r_dispersion( double, double, double );

    /// Calculate the microstrip conductor losses per unit
    double conductor_losses() const;

    /// Calculates the microstrip dielectric losses per unit
    double dielectric_losses() const;

    /// Calculates the microstrip static impedance
    void microstrip_Z0();

    /// Calculates frequency dependent parameters of the microstrip
    void dispersion();

    /// Calculates the attenuation of the microstrip
    void attenuation();

    /// Calculates the effective magnetic permeability
    void mur_eff_ms();

    /// Calculates microstrip length in radians
    void line_angle();

    double Z0_0{ 0.0 };     ///< static characteristic impedance
    double er_eff_0{ 0.0 }; ///< Static effective dielectric constant
    double mur_eff{ 0.0 };  ///< Effective mag. permeability
    double w_eff{ 0.0 };    ///< Effective width of line
    double Z0_h_1{ 0.0 };   ///< homogeneous stripline impedance
};


#endif //TRANSLINE_CALCULATIONS_MICROSTRIP_H
