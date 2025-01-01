/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
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

#include <attenuators/attenuator_classes.h>
#include <calculator_panels/panel_rf_attenuators.h>
#include <pcb_calculator_settings.h>

#include <bitmaps.h>
#include <string_utils.h>
#include <widgets/ui_common.h>
#include <widgets/std_bitmap_button.h>

extern double DoubleFromString( const wxString& TextValue );


PANEL_RF_ATTENUATORS::PANEL_RF_ATTENUATORS( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                            const wxSize& size, long style, const wxString& name ) :
        PANEL_RF_ATTENUATORS_BASE( parent, id, pos, size, style, name )
{
    m_CurrAttenuator = nullptr;
    m_bpButtonCalcAtt->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // Populate attenuator list ordered like in dialog menu list
    m_AttenuatorList.push_back( new ATTENUATOR_PI() );
    m_AttenuatorList.push_back( new ATTENUATOR_TEE() );
    m_AttenuatorList.push_back( new ATTENUATOR_BRIDGE() );
    m_AttenuatorList.push_back( new ATTENUATOR_SPLITTER() );
    m_CurrAttenuator = m_AttenuatorList[0];
    SetAttenuator( 0 );     // Ensure an attenuator and especially its bitmap are set

    m_staticTextAttMsg->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_attZinUnit->SetLabel( wxT( "Ω" ) );
    m_attZoutUnit->SetLabel( wxT( "Ω" ) );
    m_attR1Unit->SetLabel( wxT( "Ω" ) );
    m_attR2Unit->SetLabel( wxT( "Ω" ) );
    m_attR3Unit->SetLabel( wxT( "Ω" ) );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_RF_ATTENUATORS::~PANEL_RF_ATTENUATORS()
{
    for( ATTENUATOR* attenuator : m_AttenuatorList )
        delete attenuator;
}


void PANEL_RF_ATTENUATORS::ThemeChanged()
{
    // Update the bitmaps
    m_bpButtonCalcAtt->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_attenuatorBitmap->SetBitmap( KiBitmapBundle( m_CurrAttenuator->m_SchBitmapName ) );

    // Update the font
    m_staticTextAttMsg->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // Update the HTML windows
    m_Attenuator_Messages->ThemeChanged();
    m_panelAttFormula->ThemeChanged();

    Layout();
    Refresh();
}


void PANEL_RF_ATTENUATORS::UpdateUI()
{
    m_attenuatorBitmap->SetBitmap( KiBitmapBundle( m_CurrAttenuator->m_SchBitmapName ) );

    m_attenuatorBitmap->GetParent()->Layout();
    m_attenuatorBitmap->GetParent()->Refresh();
}


void PANEL_RF_ATTENUATORS::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    wxASSERT( aCfg );

    for( ATTENUATOR* attenuator : m_AttenuatorList )
        attenuator->ReadConfig();

    m_AttenuatorsSelection->SetSelection( aCfg->m_Attenuators.type );
    SetAttenuator( m_AttenuatorsSelection->GetSelection() );
}


void PANEL_RF_ATTENUATORS::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    wxASSERT( aCfg );

    aCfg->m_Attenuators.type = m_AttenuatorsSelection->GetSelection();

    for( ATTENUATOR* attenuator : m_AttenuatorList )
        attenuator->WriteConfig();
}


// Called on a attenuator selection
void PANEL_RF_ATTENUATORS::OnAttenuatorSelection( wxCommandEvent& event )
{
    SetAttenuator( (unsigned) event.GetSelection() );
    Refresh();
}


void PANEL_RF_ATTENUATORS::SetAttenuator( unsigned aIdx )
{
    if( aIdx >=m_AttenuatorList.size() )
        aIdx = m_AttenuatorList.size() - 1;

    m_CurrAttenuator = m_AttenuatorList[aIdx];
    TransfAttenuatorDataToPanel();
    m_Attenuator_Messages->SetPage( wxEmptyString );
    m_Att_R1_Value->SetValue( wxEmptyString );
    m_Att_R2_Value->SetValue( wxEmptyString );
    m_Att_R3_Value->SetValue( wxEmptyString );

    // Disable R3 for bridget T attenuator only
    bool enable = aIdx != 2;
    m_attenuatorR3Label->Enable( enable );
    m_Att_R3_Value->Enable( enable );
    m_attR3Unit->Enable( enable );
}


void PANEL_RF_ATTENUATORS::OnCalculateAttenuator( wxCommandEvent& event )
{
    TransfPanelDataToAttenuator();
    m_CurrAttenuator->Calculate();
    TransfAttenuatorResultsToPanel();
}


void PANEL_RF_ATTENUATORS::TransfPanelDataToAttenuator()
{
    wxString msg;

    msg = m_AttValueCtrl->GetValue();
    m_CurrAttenuator->m_Attenuation = DoubleFromString(msg);
    msg = m_ZinValueCtrl->GetValue();
    m_CurrAttenuator->m_Zin = DoubleFromString(msg);
    msg = m_ZoutValueCtrl->GetValue();
    m_CurrAttenuator->m_Zout = DoubleFromString(msg);
}


void PANEL_RF_ATTENUATORS::TransfAttenuatorDataToPanel()
{
    m_attenuatorBitmap->SetBitmap( KiBitmapBundle( m_CurrAttenuator->m_SchBitmapName ) );

    wxString msg;

    msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_Attenuation );
    m_AttValueCtrl->SetValue( msg );
    m_AttValueCtrl->Enable( m_CurrAttenuator->m_Attenuation_Enable );

    m_ZinValueCtrl->Enable( m_CurrAttenuator->m_Zin_Enable );

    if( m_CurrAttenuator->m_Zin_Enable )
        msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_Zin );
    else
        msg.Clear();

    m_ZinValueCtrl->SetValue( msg );

    msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_Zout );
    m_ZoutValueCtrl->SetValue( msg );

    if( m_CurrAttenuator->m_FormulaName )
    {
        if( m_CurrAttenuator->m_FormulaName->StartsWith( "<!" ) )
        {
            m_panelAttFormula->SetPage( *m_CurrAttenuator->m_FormulaName );
        }
        else
        {
            wxString html_txt;
            ConvertMarkdown2Html( wxGetTranslation( *m_CurrAttenuator->m_FormulaName ), html_txt );
            m_panelAttFormula->SetPage( html_txt );
        }
    }
    else
    {
        m_panelAttFormula->SetPage( wxEmptyString );
    }
}


void PANEL_RF_ATTENUATORS::TransfAttenuatorResultsToPanel()
{
    wxString msg;

    m_Attenuator_Messages->SetPage( wxEmptyString );

    if( m_CurrAttenuator->m_Error )
    {
        msg.Printf( _( "Attenuation more than %f dB" ),
                    m_CurrAttenuator->m_MinimumATT );
        m_Attenuator_Messages->AppendToPage( wxT( "<br><b>Error!</b></br><br><em>" ) );
        m_Attenuator_Messages->AppendToPage( msg );
        m_Attenuator_Messages->AppendToPage( wxT( "</em></br>" ) );

        // Display -- as resistor values:
        msg = wxT( "--" );
        m_Att_R1_Value->SetValue( msg );
        m_Att_R2_Value->SetValue( msg );

        if( m_CurrAttenuator->m_ResultCount  >= 3 )
            m_Att_R3_Value->SetValue( msg );

        return;
    }

    msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_R1 );
    m_Att_R1_Value->SetValue( msg );
    msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_R2 );
    m_Att_R2_Value->SetValue( msg );

    if( m_CurrAttenuator->m_ResultCount  < 3 )
    {
        m_Att_R3_Value->SetValue( wxEmptyString );
    }
    else
    {
        msg.Printf( wxT( "%g" ), m_CurrAttenuator->m_R3 );
        m_Att_R3_Value->SetValue( msg );
    }
}
