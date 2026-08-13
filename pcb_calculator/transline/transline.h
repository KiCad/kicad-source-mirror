/*
 * transline.h - base for a transmission line class definition
 *
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modifications 2018 for Kicad: Jean-Pierre Charras
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
 * along with this package.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef __TRANSLINE_H
#define __TRANSLINE_H

#include <gal/color4d.h>
#include <transline_calculations/transline_calculation_base.h>

#define TRANSLINE_OK 0
#define TRANSLINE_WARNING 1
#define TRANSLINE_ERROR 2

// IDs for lines parameters used in calculation:
// (Used to retrieve these parameters from UI.
// DUMMY_PRM is used to skip a param line in dialogs. It is not really a parameter
enum PRMS_ID
{
    UNKNOWN_ID = -1,
    EPSILONR_PRM,          // dielectric constant
    TAND_PRM,              // Dielectric Loss Tangent
    RHO_PRM,               // Conductivity of conductor
    H_PRM,                 // height of substrate
    TWISTEDPAIR_TWIST_PRM, // Twists per length
    H_T_PRM,
    STRIPLINE_A_PRM, // Stripline : distance from line to top metal
    T_PRM,           // thickness of top metal
    ROUGH_PRM,
    MUR_PRM, // magnetic permeability of substrate
    TWISTEDPAIR_EPSILONR_ENV_PRM,
    MURC_PRM,      // magnetic permeability of conductor
    FREQUENCY_PRM, // Frequency of operation
    Z0_PRM,        // characteristic impedance
    Z0_E_PRM,
    Z0_O_PRM,
    ANG_L_PRM, // Electrical length in angle
    PHYS_WIDTH_PRM,
    PHYS_DIAM_IN_PRM,  // Inner diameter of cable
    PHYS_S_PRM,        // width of gap between line and ground
    PHYS_DIAM_OUT_PRM, // Outer diameter of cable
    PHYS_LEN_PRM,      // Length of cable
    DUMMY_PRM
};


// IDs for lines parameters used in calculation that are not given by the UI
enum EXTRA_PRMS_ID
{
    EXTRA_PRMS_START = DUMMY_PRM - 1,
    SIGMA_PRM,            // Conductivity of the metal
    SKIN_DEPTH_PRM,       // Skin depth
    LOSS_DIELECTRIC_PRM,  // Loss in dielectric (dB)
    LOSS_CONDUCTOR_PRM,   // Loss in conductors (dB)
    CUTOFF_FREQUENCY_PRM, // Cutoff frequency for higher order modes
    EPSILON_EFF_PRM,      // Effective dielectric constant
    DIELECTRIC_MODEL_PRM, // 0 = CONSTANT, 1 = DJORDJEVIC_SARKAR (panel-level selection)
    EPSILONR_SPEC_FREQ_PRM, // Frequency (Hz) at which EpsilonR/TanD are specified
    SOLDERMASK_PRESENT_PRM,    // 0 = no mask correction, 1 = apply Wan-Hoorfar 2000 cover.
    SOLDERMASK_THICKNESS_PRM,  // Cured mask thickness in metres.  Typical LPI is 15 - 30 um.
    SOLDERMASK_EPSILONR_PRM,   // Mask relative permittivity.  Typical LPI 3.3 - 3.8.
    SOLDERMASK_TAND_PRM,       // Mask loss tangent.  Typical LPI 0.025 - 0.035.
    SOLDERMASK_FILLS_GAPS_PRM, // CPW / CBCPW only.  1 = mask fills the coplanar slots.
    EXTRA_PRMS_COUNT,
};

class TRANSLINE
{
public:
    TRANSLINE();
    virtual ~TRANSLINE();

    const char* m_Name;

    /**
     * Set a named property to the given value, access through the application.
     */
    void        setProperty( enum PRMS_ID aPrmId, double aValue );

    /**
     * Return a property value.
     */
    double      getProperty( enum PRMS_ID aPrmId );

    /// Setter for panel-level parameters that are shared across every calculator type.
    void SetExtraParameter( enum EXTRA_PRMS_ID aPrmId, double aValue ) { m_parameters[aPrmId] = aValue; }

    /**
     *  Get all properties from the UI. Computes some extra ones.
     */
    virtual void getProperties();

    /**
     *  Check the input parameters (ie: negative length).
     *
     *  Does not check for incompatibility between values as this depends on the line shape.
     */
    void checkProperties();

    /**
     * Put the text into the given result line.
     */
    void setResult( int aLine, double aValue, const char* aText );
    void setResult( int aLine, const char* aText );

    /**
     * Return true if the param @a aPrmId is selected.
     *
     * Has meaning only for parameters that have a radio button.
     */
    bool isSelected( enum PRMS_ID aPrmId );

    void         Init();
    virtual void synthesize();

    /**
     * Computation for analysis
     */
    virtual void calcAnalyze(){};

    /**
     * Computation for synthesis
     */
    virtual void calcSynthesize() {}

    /**
     * Show synthesis results and checks for errors / warnings.
     */
    virtual void showAnalyze() {}

    /**
     * Show analysis results and checks for errors / warnings.
     */
    virtual void showSynthesize() {}

    /**
     * Show results.
     */
    virtual void   show_results() {}

    void           analyze();

    KIGFX::COLOR4D errCol  = KIGFX::COLOR4D( 1, 0.63, 0.63, 1 );
    KIGFX::COLOR4D warnCol = KIGFX::COLOR4D( 1, 1, 0.57, 1 );
    KIGFX::COLOR4D okCol   = KIGFX::COLOR4D( 1, 1, 1, 1 );

protected:
    double m_parameters[EXTRA_PRMS_COUNT];

    /**
     * Calculate skin depth.
     *
     * \f$ \frac{1}{\sqrt{ \pi \cdot f \cdot \mu \cdot \sigma }} \f$
     */
    double skin_depth();

    /**
     * Set an error / warning level for a given parameter.
     *
     * @see TRANSLINE_OK
     * @see TRANSLINE_WARNING
     * @see TRANSLINE_ERROR
     *
     * @param aP parameter
     * @param aErrorLevel Error level
     */
    void   setErrorLevel( PRMS_ID, char );

    /**
     * Push the mask-eligible subset of soldermask parameters (PRESENT, THICKNESS, EPSILONR,
     * TAND) into the supplied calculator.  Callers that need the CPW-only FILLS_GAPS flag
     * pass aIncludeFillsGaps = true.  Mask-ineligible calculators (stripline, coax,
     * waveguide, twisted pair) do not override GetSoldermaskFillingG so these writes are
     * harmless no-ops on those backends; this helper is purely a deduplication aid for the
     * four eligible subclasses (microstrip, c_microstrip, coplanar).
     */
    void pushSoldermaskParameters( TRANSLINE_CALCULATION_BASE& aCalc,
                                   bool aIncludeFillsGaps = false ) const;

    /**
     * Convert a #TRANSLINE_STATUS status to a PCB Calculation status.
     */
    static char convertParameterStatusCode( TRANSLINE_STATUS aStatus );
};

#endif /* __TRANSLINE_H */
