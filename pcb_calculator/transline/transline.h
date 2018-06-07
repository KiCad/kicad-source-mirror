/*
 * transline.h - base for a transmission line class definition
 *
 * Copyright (C) 2005 Stefan Jahn <stefan@lkcc.org>
 * Modifications 2018 for Kicad: Jean-Pierre Charras
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

// IDs for lines parameters used in calculation:
// (Used to retrieve these parameters from UI.
// DUMMY_PRM is used to skip a param line in dialogs. It is not really a parameter
enum PRMS_ID
{
    UNKNOWN_ID = 0,
    EPSILONR_PRM,
    TAND_PRM,
    RHO_PRM,
    H_PRM,
    TWISTEDPAIR_TWIST_PRM,
    H_T_PRM,
    STRIPLINE_A_PRM,
    T_PRM,
    ROUGH_PRM,
    MUR_PRM,
    TWISTEDPAIR_EPSILONR_ENV_PRM,
    MURC_PRM,
    TANM_PRM,
    FREQUENCY_PRM,
    Z0_PRM,
    Z0_E_PRM,
    Z0_O_PRM,
    ANG_L_PRM,
    PHYS_WIDTH_PRM,
    PHYS_DIAM_IN_PRM,
    PHYS_S_PRM,
    PHYS_DIAM_OUT_PRM,
    PHYS_LEN_PRM,
    DUMMY_PRM
};

class TRANSLINE
{
public: TRANSLINE();
    virtual ~TRANSLINE();

    const char *m_Name;
    void   setProperty( enum PRMS_ID aPrmId, double aValue);
    double getProperty( enum PRMS_ID aPrmId );
    void   setResult( int, double, const char* );
    void   setResult( int, const char* );
    bool   isSelected( enum PRMS_ID aPrmId );

    virtual void synthesize() { };
    virtual void analyze() { };

protected:
    double m_freq;      // Frequency of operation
    double er;          /* dielectric constant */
    double m_tand;      // Dielectric Loss Tangent
    double m_sigma;     // Conductivity of the metal
    double m_murC;      // magnetic permeability of conductor
    double m_skindepth; // Skin depth

    double skin_depth();
    void   ellipke( double, double&, double& );
    double ellipk( double );
};

#endif /* __TRANSLINE_H */
