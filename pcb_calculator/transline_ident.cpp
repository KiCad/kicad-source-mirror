/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright (C) 2004-2014 KiCad Developers, see AUTHORS.txt for contributors.
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


// Bitmaps:
#include <c_microstrip.xpm>
#include <microstrip.xpm>
#include <twistedpair.xpm>
#include <coax.xpm>
#include <cpw.xpm>
#include <cpw_back.xpm>
#include <stripline.xpm>
#include <rectwaveguide.xpm>

// transline specific functions:
#include <transline.h>
#include <microstrip.h>
#include <coplanar.h>
#include <rectwaveguide.h>
#include <coax.h>
#include <c_microstrip.h>
#include <stripline.h>
#include <twistedpair.h>

#include <kiface_i.h>
#include <pcb_calculator_settings.h>
#include <UnitSelector.h>
#include <transline_ident.h>


/*
 * TRANSLINE_PRM
 * A class to handle one parameter of transline
 */
TRANSLINE_PRM::TRANSLINE_PRM( PRM_TYPE aType, PRMS_ID aId,
                              const wxString& aLabel,
                              const wxString& aToolTip,
                              double aValue,
                              bool aConvUnit )
{
    m_Type          = aType;
    m_Id            = aId;
    m_Label         = aLabel;
    m_ToolTip       = aToolTip;
    m_Value         = aValue;
    m_ConvUnit      = aConvUnit;
    m_ValueCtrl     = NULL;
    m_UnitCtrl      = NULL;
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


/*
 * TRANSLINE_IDENT
 * A class to handle a list of parameters of a given transline
 */

TRANSLINE_IDENT::TRANSLINE_IDENT( enum TRANSLINE_TYPE_ID aType )
{
    m_Type = aType;                     // The type of transline handled
    m_Icon = NULL;                      // An xpm icon to display in dialogs
    m_TLine    = NULL;                  // The TRANSLINE itself
    m_HasPrmSelection = false;          // true if selection of parameters must be enabled in dialog menu

    // Add common prms:
    // Default values are for FR4
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, EPSILONR_PRM,
                               _( "Er:" ), _( "Epsilon R: substrate relative dielectric constant" ),
                               4.6, false ) );
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TAND_PRM,
                               _( "TanD:" ), _( "Tangent delta: dielectric loss factor." ), 2e-2,
                               false ) );

    // Default value is for copper
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, RHO_PRM,
                               _( "Rho:" ),
                               _( "Electrical resistivity or specific electrical resistance of conductor (Ohm*meter)" ),
                               1.72e-8, false ) );

    // Default value is in GHz
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_FREQUENCY, FREQUENCY_PRM,
                               _( "Frequency:" ), _( "Frequency of the input signal" ), 1.0, true ) );


    switch( m_Type )
    {
    case MICROSTRIP_TYPE:      // microstrip
        m_TLine    = new MICROSTRIP();
        m_Icon = new wxBitmap( microstrip_xpm );

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   _( "H:" ), _( "Height of Substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   _( "H_t:" ), _( "Height of Box Top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   _( "T:" ), _( "Strip Thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   _( "Rough:" ), _( "Conductor Roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   _( "mu Rel S:" ),
                                   _( "Relative Permeability (mu) of Substrate" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "W:" ), _( "Line Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0.0, true ) );
        break;

    case CPW_TYPE:          // coplanar waveguide
        m_TLine    = new COPLANAR();
        m_Icon = new wxBitmap( cpw_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   _( "H:" ), _( "Height of Substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   _( "T:" ), _( "Strip Thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "W:" ), _( "Line Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   _( "S:" ), _( "Gap Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0.0, true ) );
        break;

    case GROUNDED_CPW_TYPE:      // grounded coplanar waveguide
        m_TLine    = new GROUNDEDCOPLANAR();
        m_Icon = new wxBitmap( cpw_back_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   _( "H:" ), _( "Height of Substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   _( "T:" ), _( "Strip Thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "W:" ), _( "Line Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   _( "S:" ), _( "Gap Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0, true ) );
        break;


    case RECTWAVEGUIDE_TYPE:      // rectangular waveguide
        m_TLine    = new RECTWAVEGUIDE();
        m_Icon = new wxBitmap( rectwaveguide_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ZF(H10) = Ey / Hx:" ) );
        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "TE-Modes:" ) );
        m_Messages.Add( _( "TM-Modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   _( "mu Rel I:" ), _( "Relative Permeability (mu) of Insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "a:" ), _( "Width of Waveguide" ), 10.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   _( "b:" ), _( "Height of Waveguide" ), 5.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Waveguide Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0, true ) );
        break;

    case COAX_TYPE:      // coaxial cable
        m_TLine    = new COAX();
        m_Icon = new wxBitmap( coax_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "TE-Modes:" ) );
        m_Messages.Add( _( "TM-Modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   _( "mu Rel I:" ), _( "Relative Permeability (mu) of Insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_IN_PRM,
                                   _( "Din:" ), _( "Inner Diameter (conductor)" ), 1.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_OUT_PRM,
                                   _( "Dout:" ), _( "Outer Diameter (insulator)" ), 8.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0.0, true ) );
        break;

    case C_MICROSTRIP_TYPE:      // coupled microstrip
        m_TLine    = new C_MICROSTRIP();
        m_Icon = new wxBitmap( c_microstrip_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff Even:" ) );
        m_Messages.Add( _( "ErEff Odd:" ) );
        m_Messages.Add( _( "Conductor Losses Even:" ) );
        m_Messages.Add( _( "Conductor Losses Odd:" ) );
        m_Messages.Add( _( "Dielectric Losses Even:" ) );
        m_Messages.Add( _( "Dielectric Losses Odd:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   _( "H:" ), _( "Height of Substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   _( "H_t:" ), _( "Height of Box Top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   _( "T:" ), _( "Strip Thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   _( "Rough:" ), _( "Conductor Roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "W:" ), _( "Line Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   _( "S:" ), _( "Gap Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_E_PRM,
                                   _( "Zeven:" ), _( "Even mode impedance (lines driven by common voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_O_PRM,
                                   _( "Zodd:" ), _( "Odd mode impedance (lines driven by opposite (differential) voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0.0, true ) );
        break;

    case STRIPLINE_TYPE:      // stripline
        m_TLine    = new STRIPLINE();
        m_Icon = new wxBitmap( stripline_xpm );

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   _( "H:" ), _( "Height of Substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, STRIPLINE_A_PRM,
                                   _( "a:" ), _( "distance between strip and top metal" ), 0.2,
                                   true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   _( "T:" ), _( "Strip Thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   _( "W:" ), _( "Line Width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Line Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0, true ) );
        break;

    case TWISTEDPAIR_TYPE:      // twisted pair
        m_TLine    = new TWISTEDPAIR();
        m_Icon = new wxBitmap( twistedpair_xpm );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor Losses:" ) );
        m_Messages.Add( _( "Dielectric Losses:" ) );
        m_Messages.Add( _( "Skin Depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_TWIST_PRM,
                                   _( "Twists:" ), _( "Number of Twists per Length" ), 0.0, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   _( "mu Rel C:" ), _( "Relative Permeability (mu) of Conductor" ), 1,
                                   false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_EPSILONR_ENV_PRM,
                                   _( "ErEnv:" ), _( "Relative Permittivity of Environment" ), 1,
                                   false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_IN_PRM,
                                   _( "Din:" ), _( "Inner Diameter (conductor)" ), 1.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_OUT_PRM,
                                   _( "Dout:" ), _( "Outer Diameter (insulator)" ), 8.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   _( "L:" ), _( "Cable Length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   _( "Z0:" ), _( "Characteristic Impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   _( "Ang_l:" ), _( "Electrical Length" ), 0.0, true ) );
        break;

    case END_OF_LIST_TYPE:      // Not really used
        break;
    }
}

TRANSLINE_IDENT::~TRANSLINE_IDENT()
{
    delete m_TLine;
    delete m_Icon;

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

        for( auto& param : m_prms_List )
        {
            std::string id = std::to_string( param->m_Id );

            try
            {
                param->m_Value = cfg->m_TransLine.param_values.at( name ).at( id );
                param->m_UnitSelection = cfg->m_TransLine.param_units.at( name ).at( id );
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
        std::string id = std::to_string( param->m_Id );
        cfg->m_TransLine.param_values[ name ][ id ] = param->m_Value;
        cfg->m_TransLine.param_units[ name ][ id ] = param->m_UnitSelection;
    }
}

