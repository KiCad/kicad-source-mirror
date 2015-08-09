/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <wx/wx.h>
#include <wx/config.h>
#include <wx/filename.h>

#include <pcb_calculator_frame_base.h>

#include <pcb_calculator.h>
#include <UnitSelector.h>

extern double DoubleFromString( const wxString& TextValue );


// these values come from QucsStudio ( by Michael Margraf )

// Display a selection of usual Er, TanD, Rho  values
// format is <value><space><comment>

/**
 * Function OnEpsilonR_Button
 * Shows a list of current relative dielectric constant(Er)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnTranslineEpsilonR_Button( wxCommandEvent& event )
{
    wxArrayString list;

    // EpsilonR ( relative dielectric constant) list
    list.Add( wxT( "4.5  FR4" ) );
    list.Add( wxT( "9.8  alumina (Al2O3)" ) );
    list.Add( wxT( "3.78  fused quartz" ) );
    list.Add( wxT( "3.38  RO4003" ) );
    list.Add( wxT( "2.2  RT/duroid 5880" ) );
    list.Add( wxT( "10.2  RT/duroid 6010LM" ) );
    list.Add( wxT( "2.1  teflon (PTFE)" ) );
    list.Add( wxT( "4.0  PVC" ) );
    list.Add( wxT( "2.3  PE" ) );
    list.Add( wxT( "6.6  beryllia (BeO)" ) );
    list.Add( wxT( "8.7  aluminum nitride" ) );
    list.Add( wxT( "11.9  silicon" ) );
    list.Add( wxT( "12.9  GaAs" ) );

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Relative Dielectric Constants"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_Value_EpsilonR->SetValue( value );
}

/**
 * Function OnTanD_Button
 * Shows a list of current dielectric loss factor (tangent delta)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnTranslineTanD_Button( wxCommandEvent& event )
{
    wxArrayString list;

    // List of current dielectric loss factor (tangent delta)
    list.Clear();
    list.Add( wxT( "2e-2  FR4 @ 1GHz" ) );
    list.Add( wxT( "3e-4  beryllia @ 10GHz" ) );
    list.Add( wxT( "2e-4  aluminia (Al2O3) @ 10GHz" ) );
    list.Add( wxT( "1e-4  fused quartz @ 10GHz" ) );
    list.Add( wxT( "2e-3  RO4003 @ 10GHz" ) );
    list.Add( wxT( "9e-4  RT/duroid 5880 @ 10GHz" ) );
    list.Add( wxT( "2e-4  teflon (PTFE) @ 1MHz" ) );
    list.Add( wxT( "5e-2  PVC @ 1MHz" ) );
    list.Add( wxT( "2e-4  PE @ 1MHz" ) );
    list.Add( wxT( "1e-3  aluminum nitride @ 10GHz" ) );
    list.Add( wxT( "0.015  silicon @ 10GHz" ) );
    list.Add( wxT( "0.002  GaAs @ 10GHz" ) );

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Dielectric Loss Factor"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_Value_TanD->SetValue( value );
}

/**
 * Function OnTranslineRho_Button
 * Shows a list of current Specific resistance list (rho)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnTranslineRho_Button( wxCommandEvent& event )
{
    wxArrayString list;

    // Specific resistance list in ohms*meters (rho):
    list.Clear();
    list.Add( wxT( "2.4e-8  gold" ) );
    list.Add( wxT( "1.72e-8  copper" ) );
    list.Add( wxT( "1.62e-8  silver" ) );
    list.Add( wxT( "12.4e-8  tin" ) );
    list.Add( wxT( "10.5e-8  platinum" ) );
    list.Add( wxT( "2.62e-8  aluminum" ) );
    list.Add( wxT( "6.9e-8  nickel" ) );
    list.Add( wxT( "3.9e-8  brass (66Cu 34Zn)" ) );
    list.Add( wxT( "9.71e-8  iron" ) );
    list.Add( wxT( "6.0e-8  zinc" ) );

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Specific Resistance"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_Value_Rho->SetValue( value );
}

// Minor helper struct to handle dialog items for a given parameter
struct DLG_PRM_DATA
{
        wxStaticText * name;
        wxTextCtrl * value;
        UNIT_SELECTOR * unit;
};
/**
 * Function TranslineTypeSelection
 * Must be called after selection of a new transline.
 * Update all values, labels and tool tips of parameters needed
 * by the new transline
 * Irrelevant parameters texts are blanked.
 * @param aType = the transline_type_id of the new selected transline
*/
void PCB_CALCULATOR_FRAME::TranslineTypeSelection( enum TRANSLINE_TYPE_ID aType )
{
    wxString msg;
    #define DOUBLE_TO_CTLR( dlg_item, value ) { msg.Printf( wxT( "%g" ), value );\
                                      dlg_item->SetValue( msg ); }
    m_currTransLineType = aType;

    if( (m_currTransLineType < START_OF_LIST_TYPE )
       || ( m_currTransLineType >= END_OF_LIST_TYPE ) )
        m_currTransLineType = DEFAULT_TYPE;

    // This helper bitmap is shown for coupled microstrip only:
    m_bmCMicrostripZoddZeven->Show( aType == C_MICROSTRIP_TYPE );

    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    m_currTransLine = tr_ident->m_TLine;

    m_radioBtnPrm1->Show( tr_ident->m_HasPrmSelection );
    m_radioBtnPrm2->Show( tr_ident->m_HasPrmSelection );

    // Setup messages
    wxStaticText* left_msg_list[] =
    {
        m_left_message1, m_left_message2, m_left_message3,
        m_left_message4, m_left_message5, m_left_message6,
        m_left_message7, NULL
    };
    wxStaticText* msg_list[] =
    {
        m_Message1, m_Message2, m_Message3,
        m_Message4, m_Message5, m_Message6,
        m_Message7, NULL
    };

    unsigned      ii = 0;
    for( ; ii < tr_ident->m_Messages.GetCount(); ii++ )
    {
        if( left_msg_list[ii] == NULL )
            break;
        left_msg_list[ii]->SetLabel( tr_ident->m_Messages[ii] );
        msg_list[ii]->SetLabel( wxEmptyString );
    }

    while( left_msg_list[ii] )
    {
        left_msg_list[ii]->SetLabel( wxEmptyString );
        msg_list[ii]->SetLabel( wxEmptyString );
        ii++;
    }

    // Init parameters dialog items
    struct DLG_PRM_DATA substrateprms[] =
    {
        { m_EpsilonR_label,m_Value_EpsilonR, NULL },
        { m_TanD_label,m_Value_TanD, NULL },
        { m_Rho_label, m_Value_Rho, NULL },
        { m_substrate_prm4_label,m_Substrate_prm4_Value, m_SubsPrm4_choiceUnit },
        { m_substrate_prm5_label,m_Substrate_prm5_Value, m_SubsPrm5_choiceUnit },
        { m_substrate_prm6_label,m_Substrate_prm6_Value, m_SubsPrm6_choiceUnit },
        { m_substrate_prm7_label,m_Substrate_prm7_Value, m_SubsPrm7_choiceUnit },
        { m_substrate_prm8_label,m_Substrate_prm8_Value, m_SubsPrm8_choiceUnit },
        { m_substrate_prm9_label,m_Substrate_prm9_Value, m_SubsPrm9_choiceUnit }
    };
    #define substrateprms_cnt (sizeof(substrateprms)/sizeof(substrateprms[0]))

    struct DLG_PRM_DATA physprms[] =
    {
        { m_phys_prm1_label,m_Phys_prm1_Value,m_choiceUnit_Param1 },
        { m_phys_prm2_label,m_Phys_prm2_Value,m_choiceUnit_Param2 },
        { m_phys_prm3_label,m_Phys_prm3_Value,m_choiceUnit_Param3 }
    };
    #define physprms_cnt (sizeof(physprms)/sizeof(physprms[0]))

    struct DLG_PRM_DATA elecprms[] =
    {
        { m_elec_prm1_label,m_Elec_prm1_Value, m_choiceUnit_ElecPrm1 },
        { m_elec_prm2_label,m_Elec_prm2_Value, m_choiceUnit_ElecPrm2 },
        { m_elec_prm3_label,m_Elec_prm3_Value, m_choiceUnit_ElecPrm3 }
    };
    #define elecprms_cnt (sizeof(elecprms)/sizeof(elecprms[0]))

    struct DLG_PRM_DATA frequencyprms[] =
    {
        { m_Frequency_label,m_Value_Frequency_Ctrl, m_choiceUnit_Frequency }
    };
    #define frequencyprms_cnt (sizeof(frequencyprms)/sizeof(frequencyprms[0]))

    unsigned idxsubs = 0;
    unsigned idxphys = 0;
    unsigned idxelec = 0;
    unsigned idxfreq = 0;

    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        struct DLG_PRM_DATA * data = NULL;
        switch( prm->m_Type )
        {
            case PRM_TYPE_SUBS:
                wxASSERT( idxsubs < substrateprms_cnt );
                data = &substrateprms[idxsubs];
                idxsubs++;
                break;

            case PRM_TYPE_PHYS:
                wxASSERT( idxphys < physprms_cnt );
                data = &physprms[idxphys];
                idxphys++;
                break;

            case PRM_TYPE_ELEC:
                wxASSERT( idxelec < elecprms_cnt );
                data = &elecprms[idxelec];
                idxelec++;
                break;

            case PRM_TYPE_FREQUENCY:
                wxASSERT( idxfreq < frequencyprms_cnt );
                data = &frequencyprms[idxfreq];
                idxfreq++;
                break;

        }
        wxASSERT ( data );
        data->name->SetToolTip( prm->m_ToolTip );
        data->name->SetLabel( prm->m_Label );
        prm->m_ValueCtrl = data->value;
        if( prm->m_Id != DUMMY_PRM )
        {
            DOUBLE_TO_CTLR( data->value, prm->m_Value );
            data->value->Enable( true );
        }
        else
        {
            data->value->SetValue( wxEmptyString );
            data->value->Enable( false );
        }
        if( prm->m_ConvUnit )
            prm->m_UnitCtrl = data->unit;
        if( data->unit )
        {
            data->unit->Show( prm->m_ConvUnit );
            data->unit->Enable( prm->m_ConvUnit );
            data->unit->SetSelection( prm->m_UnitSelection );
        }

    }

    // Clear all unused params
    for( ; idxsubs < substrateprms_cnt; idxsubs++ )
    {
        substrateprms[idxsubs].name->SetLabel(wxEmptyString);
        substrateprms[idxsubs].name->SetToolTip(wxEmptyString);
        substrateprms[idxsubs].value->SetValue(wxEmptyString);
        substrateprms[idxsubs].value->Enable( false );
        if( substrateprms[idxsubs].unit)
        {
            substrateprms[idxsubs].unit->Show( false );
            substrateprms[idxsubs].unit->Enable( false );
            substrateprms[idxsubs].unit->SetSelection( 0 );
        }
    }

    for( ; idxphys < physprms_cnt; idxphys++ )
    {
        physprms[idxphys].name->SetLabel(wxEmptyString);
        physprms[idxphys].name->SetToolTip(wxEmptyString);
        physprms[idxphys].value->SetValue(wxEmptyString);
        physprms[idxphys].value->Enable( false );
        if( physprms[idxphys].unit)
        {
            physprms[idxphys].unit->Show( false );
            physprms[idxphys].unit->Enable( false );
            physprms[idxphys].unit->SetSelection( 0 );
        }
    }

    for( ; idxelec < elecprms_cnt; idxelec++)
    {
        elecprms[idxelec].name->SetLabel(wxEmptyString);
        elecprms[idxelec].name->SetToolTip(wxEmptyString);
        elecprms[idxelec].value->SetValue(wxEmptyString);
        elecprms[idxelec].value->Enable( false );
        if( elecprms[idxelec].unit)
        {
            elecprms[idxelec].unit->Show( false );
            elecprms[idxelec].unit->Enable( false );
            elecprms[idxelec].unit->SetSelection( 0 );
        }
    }

    for( ; idxfreq < frequencyprms_cnt; idxfreq++ )
    {
        frequencyprms[idxfreq].name->SetLabel(wxEmptyString);
        frequencyprms[idxfreq].name->SetToolTip(wxEmptyString);
        frequencyprms[idxfreq].value->SetValue(wxEmptyString);
        frequencyprms[idxfreq].value->Enable( false );
        if( frequencyprms[idxfreq].unit )
        {
            frequencyprms[idxfreq].unit->Show( false );
            frequencyprms[idxfreq].unit->Enable( false );
            frequencyprms[idxfreq].unit->SetSelection( 0 );
        }
   }
}

/**
 * Function TransfDlgDataToTranslineParams
 * Read values entered in dialog frame, and copy these values
 * in current transline parameters, converted in normalized units
 */
void PCB_CALCULATOR_FRAME::TransfDlgDataToTranslineParams()
{
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        wxTextCtrl * value_ctrl = (wxTextCtrl * ) prm->m_ValueCtrl;
        wxString value_txt = value_ctrl->GetValue();
        double value = DoubleFromString(value_txt);
        prm->m_Value = value;
        UNIT_SELECTOR * unit_ctrl = (UNIT_SELECTOR * ) prm->m_UnitCtrl;
        if( unit_ctrl )
        {
            prm->m_UnitSelection = unit_ctrl->GetSelection();
            value *= unit_ctrl->GetUnitScale();
        }
        prm->m_NormalizedValue = value;
    }
}


/**
 * Function OnTranslineSelection
 * Called on new transmission line selection
*/
void PCB_CALCULATOR_FRAME::OnTranslineSelection( wxCommandEvent& event )
{
    enum TRANSLINE_TYPE_ID id = (enum TRANSLINE_TYPE_ID) event.GetSelection();

    TranslineTypeSelection( id );

    // Texts and units choice widgets can have their size modified:
    // The new size must be taken in account
    m_panelTransline->GetSizer()->Layout();
    m_panelTransline->Refresh();
}

