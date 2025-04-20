/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file transline_ident.cpp
 */
#include <wx/intl.h>
#include <wx/arrstr.h>

#include <kiface_base.h>
#include <bitmaps.h>

// transline specific functions:
#include "transline/transline.h"
#include "transline/microstrip.h"
#include "transline/coplanar.h"
#include "transline/rectwaveguide.h"
#include "transline/coax.h"
#include "transline/c_microstrip.h"
#include "transline/stripline.h"
#include "transline/twistedpair.h"
#include "transline/c_stripline.h"

#include "pcb_calculator_settings.h"
#include "widgets/unit_selector.h"
#include "transline_ident.h"


TRANSLINE_PRM::TRANSLINE_PRM( PRM_TYPE aType, PRMS_ID aId, const char* aKeywordCfg, const wxString& aDlgLabel,
                              const wxString& aToolTip, double aValue, bool aConvUnit, int aDefaultUnit )
{
    m_Type          = aType;
    m_Id            = aId;
    m_DlgLabel      = aDlgLabel;
    m_KeyWord       = aKeywordCfg;
    m_ToolTip       = aToolTip;
    m_Value         = aValue;
    m_DefaultValue  = aValue;
    m_DefaultUnit = aDefaultUnit;
    m_ConvUnit      = aConvUnit;
    m_ValueCtrl     = nullptr;
    m_UnitCtrl      = nullptr;
    m_UnitSelection = 0;
    m_NormalizedValue = 0;
 }


double TRANSLINE_PRM::ToUserUnit()
{
    if( m_UnitCtrl && m_ConvUnit )
        return 1.0 / ( (UNIT_SELECTOR*) m_UnitCtrl )->GetUnitScale();
    else
        return 1.0;
}


double TRANSLINE_PRM::FromUserUnit()
{
    if( m_UnitCtrl )
        return ( (UNIT_SELECTOR*) m_UnitCtrl )->GetUnitScale();
    else
        return 1.0;
}


TRANSLINE_IDENT::TRANSLINE_IDENT( enum TRANSLINE_TYPE_ID aType )
{
    m_Type = aType;                         // The type of transline handled
    m_BitmapName = BITMAPS::INVALID_BITMAP; // The icon to display
    m_TLine = nullptr;                      // The TRANSLINE itself
    m_HasPrmSelection = false;              // true if selection of parameters must be enabled in dialog menu

    // Add common prms:
    // Default values are for FR4
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, EPSILONR_PRM,
                               "Er", wxT( "εr" ),
                               _( "Substrate relative permittivity (dielectric constant)" ),
                               4.5, false ) );
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TAND_PRM,
                               "TanD", wxT( "tan δ" ),
                               _( "Dielectric loss (dissipation factor)" ),
                               2e-2, false ) );

    // Default value is for copper
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, RHO_PRM,
                               "Rho", wxT( "ρ" ),
                               _( "Electrical resistivity or specific electrical resistance of "
                                  "conductor (ohm*meter)" ),
                               1.72e-8, false ) );

    // Default value is in GHz
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_FREQUENCY, FREQUENCY_PRM,
                               "Frequency", _( "Frequency" ),
                               _( "Frequency of the input signal" ), 1.0, true ) );


    switch( m_Type )
    {
    case MICROSTRIP_TYPE:      // microstrip
        m_TLine = new MICROSTRIP_UI();
        m_BitmapName = BITMAPS::microstrip;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   "H_t", "H(top)", _( "Height of box top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T",
                                   _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   "Rough", _( "Roughness" ),
                                   _( "Conductor roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel S", wxString::Format( wxT( "μr (%s)" ),
                                                                 _( "substrate" ) ),
                                   _( "Relative permeability (mu) of substrate" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μr (%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case CPW_TYPE:          // coplanar waveguide
        m_TLine           = new COPLANAR();
        m_BitmapName      = BITMAPS::cpw;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   "S", "S", _( "Gap width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case GROUNDED_CPW_TYPE:      // grounded coplanar waveguide
        m_TLine           = new GROUNDEDCOPLANAR();
        m_BitmapName      = BITMAPS::cpw_back;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   "S", "S", _( "Gap width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0, true ) );
        break;


    case RECTWAVEGUIDE_TYPE:      // rectangular waveguide
        m_TLine           = new RECTWAVEGUIDE();
        m_BitmapName      = BITMAPS::rectwaveguide;
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ZF(H10) = Ey / Hx:" ) );
        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "TE-modes:" ) );
        m_Messages.Add( _( "TM-modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel I", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "insulator" ) ),
                                   _( "Relative permeability (mu) of insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "a", "a", _( "Width of waveguide" ), 10.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   "b", "b", _( "Height of waveguide" ), 5.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Waveguide length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0, true ) );
        break;

    case COAX_TYPE:      // coaxial cable
        m_TLine           = new COAX();
        m_BitmapName      = BITMAPS::coax;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "TE-modes:" ) );
        m_Messages.Add( _( "TM-modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel I", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "insulator" ) ),
                                   _( "Relative permeability (mu) of insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_IN_PRM,
                                   "Din", _( "Din" ),
                                   _( "Inner diameter (conductor)" ), 1.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_OUT_PRM,
                                   "Dout", _( "Dout" ),
                                   _( "Outer diameter (insulator)" ), 8.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case C_MICROSTRIP_TYPE:      // coupled microstrip
        m_TLine           = new C_MICROSTRIP();
        m_BitmapName      = BITMAPS::c_microstrip;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s (even):" ), wxT( "εr" ) ) );
        m_Messages.Add( wxString::Format( _( "Effective %s (odd):" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay (even):" ) );
        m_Messages.Add( _( "Unit propagation delay (odd):" ) );
        m_Messages.Add( _( "Conductor losses (even):" ) );
        m_Messages.Add( _( "Conductor losses (odd):" ) );
        m_Messages.Add( _( "Dielectric losses (even):" ) );
        m_Messages.Add( _( "Dielectric losses (odd):" ) );
        m_Messages.Add( _( "Skin depth:" ) );
        m_Messages.Add( _( "Differential Impedance (Zd):" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   "H_t", "H_t", _( "Height of box top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   "Rough", _( "Roughness" ),
                                   _( "Conductor roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   "S", "S", _( "Gap width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_E_PRM, "Zeven", _( "Zeven" ),
                                   _( "Even mode impedance (lines driven by common voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_O_PRM, "Zodd", _( "Zodd" ),
                                   _( "Odd mode impedance (lines driven by opposite "
                                      "(differential) voltages)" ),
                                   50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM, "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case C_STRIPLINE_TYPE: // Coupled stripline
        m_TLine = new C_STRIPLINE();
        m_BitmapName = BITMAPS::coupled_stripline;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s (even):" ), wxT( "εr" ) ) );
        m_Messages.Add( wxString::Format( _( "Effective %s (odd):" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay (even):" ) );
        m_Messages.Add( _( "Unit propagation delay (odd):" ) );
        m_Messages.Add( _( "Skin depth:" ) );
        m_Messages.Add( _( "Differential Impedance (Zd):" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM, "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM, "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM, "mu rel C",
                                   wxString::Format( wxT( "μ(%s)" ), _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1, false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM, "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM, "S", "S", _( "Gap width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM, "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_E_PRM,
                                   "Zeven", _( "Zeven" ),
                                   _( "Even mode impedance (lines driven by common voltages)" ),
                                   50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_O_PRM,
                                   "Zodd", _( "Zodd" ),
                                   _( "Odd mode impedance (lines driven by opposite "
                                      "(differential) voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l",
                                   _( "Electrical length" ), 0.0, true ) );
        break;

    case STRIPLINE_TYPE:      // stripline
        m_TLine = new STRIPLINE_UI();
        m_BitmapName = BITMAPS::stripline;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Unit propagation delay:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, STRIPLINE_A_PRM,
                                   "a", "a", _( "Distance between strip and top metal" ), 0.2,
                                   true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1, false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0, true ) );
        break;

    case TWISTEDPAIR_TYPE:      // twisted pair
        m_TLine           = new TWISTEDPAIR();
        m_BitmapName      = BITMAPS::twistedpair;
        m_HasPrmSelection = true;

        m_Messages.Add( wxString::Format( _( "Effective %s:" ), wxT( "εr" ) ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_TWIST_PRM,
                                   "Twists", _( "Twists" ),
                                   _( "Number of twists per length" ), 0.0, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", wxString::Format( wxT( "μ(%s)" ),
                                                                 _( "conductor" ) ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_EPSILONR_ENV_PRM,
                                   "ErEnv", wxString::Format( wxT( "εr(%s)" ),
                                                              _( "environment" ) ),
                                   _( "Relative permittivity of environment" ), 1,
                                   false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_IN_PRM,
                                   "Din", _( "Din" ),
                                   _( "Inner diameter (conductor)" ), 1.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_OUT_PRM,
                                   "Dout", _( "Dout" ),
                                   _( "Outer diameter (insulator)" ), 8.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Cable length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case END_OF_LIST_TYPE:      // Not really used
        break;
    }
}

TRANSLINE_IDENT::~TRANSLINE_IDENT()
{
    delete m_TLine;

    for( auto& ii : m_prms_List )
        delete ii;

    m_prms_List.clear();
}


void TRANSLINE_IDENT::ReadConfig()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );
    std::string name( m_TLine->m_Name );

    if( cfg->m_TransLine.param_values.count( name ) )
    {
        wxASSERT( cfg->m_TransLine.param_units.count( name ) );

        for( auto& p : m_prms_List )
        {
            try
            {
                p->m_Value = cfg->m_TransLine.param_values.at( name ).at( p->m_KeyWord );
                p->m_UnitSelection =  cfg->m_TransLine.param_units.at( name ).at( p->m_KeyWord );
            }
            catch( ... )
            {}
        }
    }
}


void TRANSLINE_IDENT::WriteConfig()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );
    std::string name( m_TLine->m_Name );

    for( auto& param : m_prms_List )
    {
        if( !std::isfinite( param->m_Value ) )
            param->m_Value = 0;

        cfg->m_TransLine.param_values[ name ][ param->m_KeyWord ] = param->m_Value;
        cfg->m_TransLine.param_units[ name ][ param->m_KeyWord ] = param->m_UnitSelection;
    }
}

