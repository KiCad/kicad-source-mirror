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

#ifndef TRANSLINE_CALCULATIONS_COUPLED_MICROSTRIP_H
#define TRANSLINE_CALCULATIONS_COUPLED_MICROSTRIP_H


#include <transline_calculations/transline_calculation_base.h>
#include <transline_calculations/microstrip.h>


class COUPLED_MICROSTRIP : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    COUPLED_MICROSTRIP() :
            TRANSLINE_CALCULATION_BASE( { TCP::Z0_E,
                                          TCP::Z0_O,
                                          TCP::Z_DIFF,
                                          TCP::ANG_L,
                                          TCP::PHYS_WIDTH,
                                          TCP::PHYS_LEN,
                                          TCP::PHYS_S,
                                          TCP::H,
                                          TCP::T,
                                          TCP::H_T,
                                          TCP::FREQUENCY,
                                          TCP::EPSILONR,
                                          TCP::EPSILON_EFF_EVEN,
                                          TCP::EPSILON_EFF_ODD,
                                          TCP::UNIT_PROP_DELAY_EVEN,
                                          TCP::UNIT_PROP_DELAY_ODD,
                                          TCP::ATTEN_COND_EVEN,
                                          TCP::ATTEN_COND_ODD,
                                          TCP::ATTEN_DILECTRIC_EVEN,
                                          TCP::ATTEN_DILECTRIC_ODD,
                                          TCP::SKIN_DEPTH,
                                          TCP::MURC,
                                          TCP::SIGMA,
                                          TCP::ROUGH,
                                          TCP::TAND } )
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

    /**
     * Computes the thickness effect on normalized width for a single microstrip line
     *
     * References: H. A. Atwater, "Simplified Design Equations for Microstrip Line Parameters", Microwave Journal,
     * pp. 109-115,November 1989.
     */
    double delta_u_thickness_single( double, double );

    /**
     * Compute the thickness effect on normalized width for coupled microstrips
     *
     * References: Rolf Jansen, "High-Speed Computation of Single and Coupled Microstrip Parameters Including
     * Dispersion, High-Order Modes, Loss and Finite Strip Thickness", IEEE Trans. MTT, vol. 26, no. 2, pp. 75-82,
     * Feb. 1978
     */
    void delta_u_thickness();

    /// Computes initial parameters for a single microstrip
    void compute_single_line();

    /// Compute the filling factor for the coupled microstrip even mode without cover and zero conductor thickness
    double filling_factor_even( double, double, double );

    /**
     * Compute the filling factor for the coupled microstrip odd mode without cover and zero conductor thickness
     */
    double filling_factor_odd( double, double, double );

    /// Compute the cover effect on filling factor for the even mode
    double delta_q_cover_even( double );

    /// Compute the cover effect on filling factor for the odd mode
    double delta_q_cover_odd( double );

    /**
     * Compute the static effective dielectric constants
     *
     * References: Manfred Kirschning and Rolf Jansen, "Accurate Wide-Range Design Equations for the Frequency-Dependent
     * Characteristic of Parallel Coupled Microstrip Lines", IEEE Trans. MTT, vol. 32, no. 1, Jan. 1984
     */
    void er_eff_static();

    /**
     * Compute the even mode impedance correction for a homogeneous microstrip due to the cover
     *
     * References: S. March, "Microstrip Packaging: Watch the Last Step", Microwaves, vol. 20, no. 13, pp. 83.94,
     * Dec. 1981.
     */
    double delta_Z0_even_cover( double, double, double );

    /**
     * Compute the odd mode impedance correction for a homogeneous microstrip due to the cover
     *
     * References: S. March, "Microstrip Packaging: Watch the Last Step", Microwaves, vol. 20, no. 13, pp. 83.94,
     * Dec. 1981.
     */
    double delta_Z0_odd_cover( double, double, double );

    /**
     * Compute the static even- and odd-mode static impedances
     *
     * References: Manfred Kirschning and Rolf Jansen, "Accurate Wide-Range Design Equations for the Frequency-Dependent
     * Characteristic of Parallel Coupled Microstrip Lines", IEEE Trans. MTT, vol. 32, no. 1, Jan. 1984
     */
    void Z0_even_odd();

    /// Compute er_eff as a function of frequency
    void er_eff_freq();

    /// Compute conductor losses per unit length
    void conductor_losses();

    /// Compute dielectric losses per unit length
    void dielectric_losses();

    /// Compute attenuation
    void attenuation();

    /// Compute electrical length in radians
    void line_angle();

    /// Calculate the differential impedance of the coupled microstrips
    void diff_impedance();

    /// Calculate frequency dependency of characteristic  impedances
    void Z0_dispersion();

    /// Error function to minimise when synthesising trace geometry
    void syn_err_fun( double*, double*, double, double, double, double, double );

    /**
     * Calculate widths given Z0 and e_r
     *
     * From Akhtarzad S. et al., "The design of coupled microstrip lines", IEEE Trans. MTT-23, June 1975 and
     * Hinton, J.H., "On design of coupled microstrip lines", IEEE Trans. MTT-28, March 1980
     */
    void synth_width();

    void syn_fun( double*, double*, double, double, double, double );

    /// Runs intermediate single-track calculations
    MICROSTRIP m_aux_microstrip;

    double w_t_e{ 0.0 };
    double w_t_o{ 0.0 };
    double er_eff_e_0{ 0.0 };
    double er_eff_o_0{ 0.0 };
    double Z0_e_0{ 0.0 };
    double Z0_o_0{ 0.0 };
    double er_eff_e{ 0.0 };
    double er_eff_o{ 0.0 };
    double prop_delay_e{ 0.0 };
    double prop_delay_o{ 0.0 };
    double atten_cond_e{ 0.0 };
    double atten_cond_o{ 0.0 };
    double atten_dielectric_e{ 0.0 };
    double atten_dielectric_o{ 0.0 };
    double ang_l_e{ 0.0 };
    double ang_l_o{ 0.0 };
    double Zdiff{ 0.0 };
};


#endif //TRANSLINE_CALCULATIONS_COUPLED_MICROSTRIP_H
