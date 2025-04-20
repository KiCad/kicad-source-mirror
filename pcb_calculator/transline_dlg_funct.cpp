/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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
#include <wx/choicdlg.h>
#include <wx/filename.h>
#include <wx/settings.h>

#include <bitmaps.h>
#include <calculator_panels/panel_transline.h>
#include <common_data.h>
#include <widgets/unit_selector.h>
#include <pcb_calculator_utils.h>


extern double DoubleFromString( const wxString& TextValue );

void PANEL_TRANSLINE::OnTranslineEpsilonR_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardRelativeDielectricConstantList();
    list.Add( "" );  // Add an empty line for no selection

    // Find the previous choice index:
    wxString prevChoiceStr = m_Value_EpsilonR->GetValue();
    int prevChoice = 0;
    findMatch( list, prevChoiceStr, prevChoice );

    int index = wxGetSingleChoiceIndex( wxEmptyString, _( "Relative Dielectric Constants" ),
                                        list, prevChoice );

    if( index >= 0 && !list.Item( index ).IsEmpty() )   // i.e. non canceled.
        m_Value_EpsilonR->SetValue( list.Item( index ).BeforeFirst( ' ' ) );
}


void PANEL_TRANSLINE::OnTranslineTanD_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardLossTangentList();
    list.Add( "" );  // Add an empty line for no selection

    // Find the previous choice index:
    wxString prevChoiceStr = m_Value_TanD->GetValue();
    int prevChoice = 0;
    findMatch( list, prevChoiceStr, prevChoice );

    int index = wxGetSingleChoiceIndex( wxEmptyString, _( "Dielectric Loss Factor" ), list,
                                        prevChoice, nullptr );

    if( index >= 0 && !list.Item( index ).IsEmpty() )   // i.e. non canceled.
        m_Value_TanD->SetValue( list.Item( index ).BeforeFirst( ' ' ) );
}


void PANEL_TRANSLINE::OnTranslineRho_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardResistivityList();
    list.Add( "" );  // Add an empty line for no selection

    // Find the previous choice index:
    wxString prevChoiceStr = m_Value_Rho->GetValue();
    int prevChoice = 0;
    findMatch( list, prevChoiceStr, prevChoice );

    int index = wxGetSingleChoiceIndex( wxEmptyString, _( "Specific Resistance" ), list,
                                        prevChoice, nullptr );

    if( index >= 0 && !list.Item( index ).IsEmpty() )   // i.e. non canceled.
        m_Value_Rho->SetValue( list.Item( index ).BeforeFirst( ' ' ) );
}


// Minor helper struct to handle dialog items for a given parameter
struct DLG_PRM_DATA
{
    wxStaticText*  name;
    wxTextCtrl*    value;
    UNIT_SELECTOR* unit;
};


void PANEL_TRANSLINE::TranslineTypeSelection( enum TRANSLINE_TYPE_ID aType )
{
    m_currTransLineType = aType;

    if( (m_currTransLineType < START_OF_LIST_TYPE )
            || ( m_currTransLineType >= END_OF_LIST_TYPE ) )
    {
        m_currTransLineType = DEFAULT_TYPE;
    }

    m_translineBitmap->SetBitmap( KiBitmapBundle( m_transline_list[m_currTransLineType]->m_BitmapName ) );

    // This helper bitmap is shown for coupled microstrip only:
    m_bmCMicrostripZoddZeven->Show( aType == C_MICROSTRIP_TYPE || aType == C_STRIPLINE_TYPE );
    m_bmCMicrostripZoddZeven->SetBitmap( KiBitmapBundle( BITMAPS::microstrip_zodd_zeven ) );

    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    m_currTransLine = tr_ident->m_TLine;

    m_radioBtnPrm1->Show( tr_ident->m_HasPrmSelection );
    m_radioBtnPrm2->Show( tr_ident->m_HasPrmSelection );

    // Setup messages
    wxStaticText* left_msg_list[] = { m_left_message1, m_left_message2,  m_left_message3, m_left_message4,
                                      m_left_message5, m_left_message6,  m_left_message7, m_left_message8,
                                      m_left_message9, m_left_message10, nullptr };

    wxStaticText* msg_list[] = { m_Message1, m_Message2, m_Message3, m_Message4,  m_Message5, m_Message6,
                                 m_Message7, m_Message8, m_Message9, m_Message10, nullptr };

    unsigned jj = 0;

    for( ; jj < tr_ident->m_Messages.GetCount(); jj++ )
    {
        if( left_msg_list[jj] == nullptr )
            break;

        left_msg_list[jj]->SetLabel( tr_ident->m_Messages[jj] );
        msg_list[jj]->SetLabel( wxEmptyString );
    }

    while( left_msg_list[jj] )
    {
        left_msg_list[jj]->SetLabel( wxEmptyString );
        msg_list[jj]->SetLabel( wxEmptyString );
        jj++;
    }


    // Init parameters dialog items
    struct DLG_PRM_DATA substrateprms[] =
            {
                { m_EpsilonR_label,       m_Value_EpsilonR,       nullptr },
                { m_TanD_label,           m_Value_TanD,           nullptr },
                { m_Rho_label,            m_Value_Rho,            nullptr },
                { m_substrate_prm4_label, m_Substrate_prm4_Value, m_SubsPrm4_choiceUnit },
                { m_substrate_prm5_label, m_Substrate_prm5_Value, m_SubsPrm5_choiceUnit },
                { m_substrate_prm6_label, m_Substrate_prm6_Value, m_SubsPrm6_choiceUnit },
                { m_substrate_prm7_label, m_Substrate_prm7_Value, m_SubsPrm7_choiceUnit },
                { m_substrate_prm8_label, m_Substrate_prm8_Value, m_SubsPrm8_choiceUnit },
                { m_substrate_prm9_label, m_Substrate_prm9_Value, m_SubsPrm9_choiceUnit }
            };

#define substrateprms_cnt (sizeof(substrateprms)/sizeof(substrateprms[0]))

    struct DLG_PRM_DATA physprms[] =
            {
                { m_phys_prm1_label, m_Phys_prm1_Value, m_choiceUnit_Param1 },
                { m_phys_prm2_label, m_Phys_prm2_Value, m_choiceUnit_Param2 },
                { m_phys_prm3_label, m_Phys_prm3_Value, m_choiceUnit_Param3 }
            };

#define physprms_cnt (sizeof(physprms)/sizeof(physprms[0]))

    struct DLG_PRM_DATA elecprms[] =
            {
                { m_elec_prm1_label, m_Elec_prm1_Value, m_choiceUnit_ElecPrm1 },
                { m_elec_prm2_label, m_Elec_prm2_Value, m_choiceUnit_ElecPrm2 },
                { m_elec_prm3_label, m_Elec_prm3_Value, m_choiceUnit_ElecPrm3 }
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
        struct DLG_PRM_DATA * data = nullptr;

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
        data->name->SetLabel( prm->m_DlgLabel != wxS( "" ) ? prm->m_DlgLabel + wxS( ':' ) : wxString( wxS( "" ) ) );
        prm->m_ValueCtrl = data->value;

        if( prm->m_Id != DUMMY_PRM )
        {
            data->value->SetValue( wxString::Format( wxS( "%g" ), prm->m_Value ) );
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


void PANEL_TRANSLINE::TransfDlgDataToTranslineParams()
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


void PANEL_TRANSLINE::OnTranslineSelection( wxCommandEvent& event )
{
    // Ensure parameters from current selection are taken in account before switching to a new selection
    if( m_currTransLine )
        TransfDlgDataToTranslineParams();

    enum TRANSLINE_TYPE_ID id = (enum TRANSLINE_TYPE_ID) event.GetSelection();

    TranslineTypeSelection( id );

    // Texts and units choice widgets can have their size modified:
    // The new size must be taken in account
    GetSizer()->Layout();
    Refresh();
}


void PANEL_TRANSLINE::OnTransLineResetButtonClick( wxCommandEvent& event )
{
    // Initialize param values to default value
    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];

    for( unsigned ii = 0; ii < tr_ident->GetPrmsCount(); ii++ )
    {
        TRANSLINE_PRM* prm = tr_ident->GetPrm( ii );
        prm->m_Value = prm->m_DefaultValue;
        UNIT_SELECTOR* unit_ctrl = (UNIT_SELECTOR*) prm->m_UnitCtrl;

        if( unit_ctrl )
            prm->m_UnitSelection = prm->m_DefaultUnit;
    }

    // Reinit displayed values
    TranslineTypeSelection( m_currTransLineType );

    Refresh();
}
