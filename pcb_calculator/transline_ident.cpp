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

#include <kiface_i.h>
#include <bitmap_types.h>

// Bitmaps:
#include "bitmaps/c_microstrip.cpp"
#include "bitmaps/microstrip.cpp"
#include "bitmaps/twistedpair.cpp"
#include "bitmaps/coax.cpp"
#include "bitmaps/cpw.cpp"
#include "bitmaps/cpw_back.cpp"
#include "bitmaps/stripline.cpp"
#include "bitmaps/rectwaveguide.cpp"

// transline specific functions:
#include "transline/transline.h"
#include "transline/microstrip.h"
#include "transline/coplanar.h"
#include "transline/rectwaveguide.h"
#include "transline/coax.h"
#include "transline/c_microstrip.h"
#include "transline/stripline.h"
#include "transline/twistedpair.h"

#include "pcb_calculator_settings.h"
#include "widgets/unit_selector.h"
#include "transline_ident.h"


/*
 * TRANSLINE_PRM
 * A class to handle one parameter of transline
 */
TRANSLINE_PRM::TRANSLINE_PRM( PRM_TYPE aType, PRMS_ID aId,
                              const char* aKeywordCfg,
                              const wxString& aDlgLabel,
                              const wxString& aToolTip,
                              double aValue,
                              bool aConvUnit )
{
    m_Type          = aType;
    m_Id            = aId;
    m_DlgLabel      = aDlgLabel;
    m_KeyWord       = aKeywordCfg;
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
 * Important note:
 * the first string of TRANSLINE_PRM (m_KeyWord) is a keyword in config file.
 * it can contain only ASCII7 chars
 * the second string of TRANSLINE_PRM (m_DlgLabel) is a string translated for dialog,
 * so mark it for translation (m_KeyWord and m_DlgLabel are usually the same in English)
 * and of course do not mark translatable m_DlgLabel that obviously cannot be translated,
 * like "H" or "H_t"
 */

TRANSLINE_IDENT::TRANSLINE_IDENT( enum TRANSLINE_TYPE_ID aType )
{
    m_Type = aType;               // The type of transline handled
    m_Icon = NULL;                // An xpm icon to display in dialogs
    m_TLine = NULL;               // The TRANSLINE itself
    m_HasPrmSelection = false;    // true if selection of parameters must be enabled in dialog menu

    // Add common prms:
    // Default values are for FR4
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, EPSILONR_PRM,
                               "Er", _( "Er" ),
                               _( "Epsilon R: substrate relative dielectric constant" ),
                               4.6, false ) );
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TAND_PRM,
                               "TanD", _( "TanD" ),
                               _( "Tangent delta: dielectric loss factor." ), 2e-2,
                               false ) );

    // Default value is for copper
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, RHO_PRM,
                               "Rho", _( "Rho" ),
                               _( "Electrical resistivity or specific electrical resistance of conductor (Ohm*meter)" ),
                               1.72e-8, false ) );

    // Default value is in GHz
    AddPrm( new TRANSLINE_PRM( PRM_TYPE_FREQUENCY, FREQUENCY_PRM,
                               "Frequency", _( "Frequency" ),
                               _( "Frequency of the input signal" ), 1.0, true ) );


    switch( m_Type )
    {
    case MICROSTRIP_TYPE:      // microstrip
        m_TLine = new MICROSTRIP();
        m_Icon = new wxBitmap( KiBitmap( microstrip_xpm ) );

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   "H_t", "H_t", _( "Height of box top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T",
                                   _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   "Rough", _( "Rough" ),
                                   _( "Conductor roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel S",_( "mu substrate" ),
                                   _( "Relative permeability (mu) of substrate" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", _( "mu conductor" ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line wength" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case CPW_TYPE:          // coplanar waveguide
        m_TLine = new COPLANAR();
        m_Icon = new wxBitmap( KiBitmap( cpw_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", _( "mu conductor" ),
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
        m_TLine = new GROUNDEDCOPLANAR();
        m_Icon = new wxBitmap( KiBitmap( cpw_back_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", "mu condutor",
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
        m_TLine = new RECTWAVEGUIDE();
        m_Icon = new wxBitmap( KiBitmap( rectwaveguide_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ZF(H10) = Ey / Hx:" ) );
        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "TE-modes:" ) );
        m_Messages.Add( _( "TM-modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel I",_( "mu insulator" ),
                                   _( "Relative permeability (mu) of insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C",_( "mu conductor" ),
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
        m_TLine = new COAX();
        m_Icon = new wxBitmap( KiBitmap( coax_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "TE-modes:" ) );
        m_Messages.Add( _( "TM-modes:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MUR_PRM,
                                   "mu Rel I", _( "mu insulator" ),
                                   _( "Relative Permeability (mu) of insulator" ), 1, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", _( "mu conductor" ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_IN_PRM,
                                   "Din", _( "Din" ), _( "Inner diameter (conductor)" ), 1.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_DIAM_OUT_PRM,
                                   "Dout", _( "Dout" ), _( "Outer diameter (insulator)" ), 8.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_PRM,
                                   "Z0", "Z0", _( "Characteristic impedance" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, DUMMY_PRM ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l", _( "Electrical length" ), 0.0, true ) );
        break;

    case C_MICROSTRIP_TYPE:      // coupled microstrip
        m_TLine = new C_MICROSTRIP();
        m_Icon = new wxBitmap( KiBitmap( c_microstrip_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff even:" ) );
        m_Messages.Add( _( "ErEff odd:" ) );
        m_Messages.Add( _( "Conductor losses even:" ) );
        m_Messages.Add( _( "Conductor losses odd:" ) );
        m_Messages.Add( _( "Dielectric losses even:" ) );
        m_Messages.Add( _( "Dielectric losses odd:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_PRM,
                                   "H", "H", _( "Height of substrate" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, H_T_PRM,
                                   "H_t", "H_t", _( "Height of box top" ), 1e20, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, T_PRM,
                                   "T", "T", _( "Strip thickness" ), 0.035, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, ROUGH_PRM,
                                   "Rough", _( "Rough" ), _( "Conductor roughness" ), 0.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu rel C", _( "mu conductor" ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_WIDTH_PRM,
                                   "W", "W", _( "Line width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_S_PRM,
                                   "S", "S", _( "Gap width" ), 0.2, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_PHYS, PHYS_LEN_PRM,
                                   "L", "L", _( "Line length" ), 50.0, true ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_E_PRM,
                                   "Zeven", _( "Zeven" ),
                                   _( "Even mode impedance (lines driven by common voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, Z0_O_PRM,
                                   "Zodd", _( "Zodd" ),
                                   _( "Odd mode impedance (lines driven by opposite (differential) voltages)" ), 50.0, true ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_ELEC, ANG_L_PRM,
                                   "Ang_l", "Ang_l",
                                   _( "Electrical length" ), 0.0, true ) );
        break;

    case STRIPLINE_TYPE:      // stripline
        m_TLine = new STRIPLINE();
        m_Icon = new wxBitmap( KiBitmap( stripline_xpm ) );

        m_Messages.Add( _( "ErEff:" ) );
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
                                   "mu Rel C", _( "mu conductor" ),
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
        m_TLine = new TWISTEDPAIR();
        m_Icon = new wxBitmap( KiBitmap( twistedpair_xpm ) );
        m_HasPrmSelection = true;

        m_Messages.Add( _( "ErEff:" ) );
        m_Messages.Add( _( "Conductor losses:" ) );
        m_Messages.Add( _( "Dielectric losses:" ) );
        m_Messages.Add( _( "Skin depth:" ) );

        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_TWIST_PRM,
                                   "Twists", _( "Twists" ),
                                   _( "Number of twists per length" ), 0.0, false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, MURC_PRM,
                                   "mu Rel C", _( "mu conductor" ),
                                   _( "Relative permeability (mu) of conductor" ), 1,
                                   false ) );
        AddPrm( new TRANSLINE_PRM( PRM_TYPE_SUBS, TWISTEDPAIR_EPSILONR_ENV_PRM,
                                   "ErEnv", _( "ErEnv" ),
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

