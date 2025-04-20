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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
    EXTRA_PRMS_COUNT,
};

class TRANSLINE
{
public:
    TRANSLINE();
    virtual ~TRANSLINE();

    const char* m_Name;
    void        setProperty( enum PRMS_ID aPrmId, double aValue );
    double      getProperty( enum PRMS_ID aPrmId );


    virtual void getProperties();
    void checkProperties();
    void setResult( int, double, const char* );
    void setResult( int, const char* );
    bool isSelected( enum PRMS_ID aPrmId );

    void         Init();
    virtual void synthesize();
    virtual void calc() {}

    /**
     * Computation for analysis
     */
    virtual void calcAnalyze(){};

    /**
     * Computation for synthesis
     **/
    virtual void calcSynthesize() {}

    /**
     * Shows synthesis results and checks for errors / warnings.
     **/
    virtual void showAnalyze() {}

    /**
     * Shows analysis results and checks for errors / warnings.
     **/
    virtual void showSynthesize() {}

    /**
     * Shows results
     **/
    virtual void   show_results() {}

    void           analyze();

    KIGFX::COLOR4D errCol  = KIGFX::COLOR4D( 1, 0.63, 0.63, 1 );
    KIGFX::COLOR4D warnCol = KIGFX::COLOR4D( 1, 1, 0.57, 1 );
    KIGFX::COLOR4D okCol   = KIGFX::COLOR4D( 1, 1, 1, 1 );

protected:
    double m_parameters[EXTRA_PRMS_COUNT];

    bool   minimizeZ0Error1D( double* );
    double skin_depth();
    void   ellipke( double, double&, double& );
    double ellipk( double );
    void   setErrorLevel( PRMS_ID, char );

    /// Calculates the unit propagation delay (in ps/cm) for the given effective dielectric constant
    static double calcUnitPropagationDelay( double epsilonEff );

    /// Converts a TRANSLINE_PARAMETER status to a PCB Calculation status
    static char convertParameterStatusCode( TRANSLINE_STATUS aStatus );
};

#endif /* __TRANSLINE_H */
