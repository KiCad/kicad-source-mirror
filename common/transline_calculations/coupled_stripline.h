/*
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

#ifndef TRANSLINE_CALCULATIONS_COUPLED_STRIPLINE_H
#define TRANSLINE_CALCULATIONS_COUPLED_STRIPLINE_H


#include <transline_calculations/transline_calculation_base.h>
#include <transline_calculations/stripline.h>

/*
 * This implements the calculations described in:
 *
 * [1] S. B. Cohn, "Characteristic Impedance of the Shielded-Strip Transmission Line," in Transactions of the IRE
 *    Professional Group on Microwave Theory and Techniques, vol. 2, no. 2, pp. 52-57, July 1954
 * [2] S. B. Cohn, "Shielded Coupled-Strip Transmission Line," in IRE Transactions on Microwave Theory and Techniques,
 *    vol. 3, no. 5, pp. 29-38, October 1955
 */

class COUPLED_STRIPLINE : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    COUPLED_STRIPLINE() :
            TRANSLINE_CALCULATION_BASE( { TCP::SKIN_DEPTH, TCP::Z0_E, TCP::Z0_O, TCP::Z_DIFF, TCP::PHYS_WIDTH,
                                          TCP::FREQUENCY, TCP::PHYS_LEN, TCP::H, TCP::PHYS_S, TCP::T, TCP::EPSILONR,
                                          TCP::MURC, TCP::SIGMA, TCP::ANG_L } )
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

    /// Calculates the impedance of a finite-width single strip
    double calcZ0SymmetricStripline();

    /// Calculate the coupling fringe capacitances
    void calcFringeCapacitances( double h, double t, double er );

    /// Calculates impedances of finite- and zero-thickness single strips
    void calcSingleStripImpedances();

    /// Calculates zero-thickness coupled strip impedances
    void calcZeroThicknessCoupledImpedances( double h, double w, double s, double er );

    /// Calculates even mode Z0
    void calcZ0EvenMode();

    /// Calculates odd mode Z0
    void calcZ0OddMode( double t, double s );

    /// Calculates conductor and dielectric losses
    void calcLosses();

    /// Calculate dialectric and propagation parameters
    void calcDielectrics();

    double C_f_0{ 0.0 };   ///< Fringing capacitance from one edge to ground of zero thickness strip
    double C_f_t_h{ 0.0 }; ///< Fringing capacitance of single strip of finite width

    double Z0_w_h_0{ 0.0 };       ///< Impedance of single strip of zero thickness
    double Z0_w_h_t_h{ 0.0 };     ///< Impedance of single strip of finite thickness
    double Z0_e_w_h_0_s_h{ 0.0 }; ///< Even mode impedance of coupled zero thickness strips
    double Z0_o_w_h_0_s_h{ 0.0 }; ///< Odd mode impedance of coupled zero thickness strips

    double e_eff_e{ 0.0 };           ///< Even mode effective dielectric constant
    double e_eff_o{ 0.0 };           ///< Odd mode effective dielectric constant
    double ang_l{ 0.0 };             ///< Angular length (rad)
    double unit_prop_delay_e{ 0.0 }; ///< Even mode unit propagation delay (ps/cm)
    double unit_prop_delay_o{ 0.0 }; ///< Odd mode unit propagation delay (ps/cm)

    /// Calculator used to determine single stripline values
    STRIPLINE m_striplineCalc;
};


#endif //TRANSLINE_CALCULATIONS_COUPLED_STRIPLINE_H
