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

#include <utility>

/*
 * This implements the calculations described in:
 *
 * [1] S. B. Cohn, "Characteristic Impedance of the Shielded-Strip Transmission Line," in Transactions of the IRE
 *    Professional Group on Microwave Theory and Techniques, vol. 2, no. 2, pp. 52-57, July 1954
 * [2] S. B. Cohn, "Shielded Coupled-Strip Transmission Line," in IRE Transactions on Microwave Theory and Techniques,
 *    vol. 3, no. 5, pp. 29-38, October 1955
 * [3] B. C. Wadell, "Transmission Line Design Handbook," Artech House, Norwood, MA, 1991.  Sec. 3.6.3
 *    "Off-Center Stripline" (Eqs. 3.6.3.21 - 3.6.3.23) gives the image-method plus three-term
 *    correction for a single strip offset between two ground planes; applied here per mode to the
 *    Reference [2] coupled-stripline solver.
 */

class COUPLED_STRIPLINE : public TRANSLINE_CALCULATION_BASE
{
    using TCP = TRANSLINE_PARAMETERS;

public:
    COUPLED_STRIPLINE() :
            TRANSLINE_CALCULATION_BASE( { TCP::SKIN_DEPTH, TCP::Z0_E, TCP::Z0_O, TCP::Z_DIFF, TCP::Z_COMM,
                                          TCP::COUPLING_K, TCP::PHYS_WIDTH, TCP::FREQUENCY, TCP::PHYS_LEN, TCP::H,
                                          TCP::PHYS_S, TCP::T, TCP::EPSILONR, TCP::MUR, TCP::MURC, TCP::SIGMA,
                                          TCP::TAND, TCP::ROUGH, TCP::ANG_L, TCP::STRIPLINE_A, TCP::ATTEN_COND_EVEN,
                                          TCP::ATTEN_COND_ODD, TCP::ATTEN_DILECTRIC_EVEN, TCP::ATTEN_DILECTRIC_ODD,
                                          TCP::DIELECTRIC_MODEL_SEL, TCP::EPSILONR_SPEC_FREQ } )
    {
    }

    /// Analyse track geometry parameters to output Z0 and Ang_L
    void Analyse() override;

    /// Synthesis track geometry parameters to match given Z0
    bool Synthesize( SYNTHESIZE_OPTS aOpts ) override;

    /// Returns true when the strip plane offset a is effectively at the centre (a = h/2 within
    /// numerical tolerance).  When a <= 0 is treated as unset and maps to centred.
    static bool IsCenteredOffset( double a, double h );

private:
    /// Sets the output values and status following analysis
    void SetAnalysisResults() override;

    /// Sets the output values and status following synthesis
    void SetSynthesisResults() override;

    /// Identifies which result map publishResults() should populate.
    enum class ResultSink
    {
        ANALYSIS,
        SYNTHESIS
    };

    /// Shared body of SetAnalysisResults / SetSynthesisResults.  The status-severity arguments
    /// flip between the two callers so that bad mode impedances are an error during analysis but
    /// only a warning during synthesis (and vice versa for geometry).
    void publishResults( ResultSink aSink, TRANSLINE_STATUS aImpedanceFailure, TRANSLINE_STATUS aGeometryFailure );

    /// Calculates the impedance of a finite-width single strip
    double calcZ0SymmetricStripline();

    /// Calculate the coupling fringe capacitances
    void calcFringeCapacitances( double h, double t, double er );

    /// Calculates impedances of finite- and zero-thickness single strips
    void calcSingleStripImpedances();

    /// Calculates zero-thickness coupled strip impedances
    void calcZeroThicknessCoupledImpedances( double h, double w, double s, double er );

    /// Returns true when the offset a is far enough from each ground plane that the Reference [1]
    /// finite-thickness fringe formula is well defined on both virtual centred striplines produced
    /// by the Reference [3] Eq. 3.6.3.22 image split (i.e. t/2 < a < h - t/2).
    static bool isOffsetWithinFiniteThicknessLimits( double a, double h, double t );

    /// Offset-aware wrapper around calcZeroThicknessCoupledImpedances built on Reference [3] Sec.
    /// 3.6.3 (off-center stripline).  For centred cases it calls the base function once; otherwise
    /// Eq. 3.6.3.22 combines two virtual centred striplines at plate spacings 2a and 2(h - a) via
    /// parallel admittance, then Eq. 3.6.3.23 applies the three-term correction that restores the
    /// image-method result to within ~2 percent of numerical reference.  Wadell derives the
    /// correction for a single strip; we apply it per even / odd mode, which preserves the
    /// centred limit (the position factor vanishes at a = h/2) while improving accuracy away from
    /// it.
    void calcOffsetZeroThicknessCoupledImpedances( double h, double a, double w, double s, double t, double er );

    /// Applies the Reference [3] Eq. 3.6.3.23 correction to an image-method impedance.  The
    /// position factor |0.5 - a/h|^2.2 is zero at a = h/2 (centred) and grows with offset; the
    /// width factor ((t + w) / h)^2.9 scales with strip proximity.  The correction is fit to
    /// single-ended data with a claimed ~2 percent accuracy for 0.2 < a/h < 0.8 and t/h < 0.2;
    /// used per mode here with the corresponding mode impedance.
    static double applyOffsetCorrection( double aZImage, double aOffset, double aPlateSpacing, double aWidth,
                                         double aThickness, double aEr );

    /// Runs the centred finite-thickness pipeline for a single Reference [3] virtual stripline of
    /// plate spacing aVirtualH.  Returns the (Z0e, Z0o) pair seen at that plate spacing so the
    /// caller can combine the two halves via Eq. 3.6.3.22.
    std::pair<double, double> calcOffsetVirtualBranch( double aVirtualH, double w, double s, double t, double er );

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
