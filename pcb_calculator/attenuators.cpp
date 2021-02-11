/**
 * @file attenuators.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 jean-pierre.charras
 * Copyright (C) 2015 Kicad Developers, see change_log.txt for contributors.
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

#include <dialog_helpers.h>

#include "pcb_calculator_frame.h"

extern double DoubleFromString( const wxString& TextValue );

// Called on a attenuator selection
void PCB_CALCULATOR_FRAME::OnAttenuatorSelection( wxCommandEvent& event )
{
    SetAttenuator( (unsigned) event.GetSelection() );
    Refresh();
}


void PCB_CALCULATOR_FRAME::SetAttenuator( unsigned aIdx )
{
    if( aIdx >=m_attenuator_list.size() )
        aIdx = m_attenuator_list.size() - 1;

    m_currAttenuator = m_attenuator_list[aIdx];
    TransfAttenuatorDataToPanel();
    m_Attenuator_Messages->SetPage( wxEmptyString );
    m_Att_R1_Value->SetValue( wxEmptyString );
    m_Att_R2_Value->SetValue( wxEmptyString );
    m_Att_R3_Value->SetValue( wxEmptyString );
}


void PCB_CALCULATOR_FRAME::OnCalculateAttenuator( wxCommandEvent& event )
{
    TransfPanelDataToAttenuator();
    m_currAttenuator->Calculate();
    TransfAttenuatorResultsToPanel();
}


void PCB_CALCULATOR_FRAME::TransfPanelDataToAttenuator()
{
    wxString msg;

    msg = m_AttValueCtrl->GetValue();
    m_currAttenuator->m_Attenuation = DoubleFromString(msg);
    msg = m_ZinValueCtrl->GetValue();
    m_currAttenuator->m_Zin = DoubleFromString(msg);
    msg = m_ZoutValueCtrl->GetValue();
    m_currAttenuator->m_Zout = DoubleFromString(msg);
}


void PCB_CALCULATOR_FRAME::TransfAttenuatorDataToPanel()
{
    // TODO: make attenuator bitmaps transparent so we can remove this
    m_attenuatorPanel->SetBackgroundColour( *wxWHITE );

    m_attenuatorBitmap->SetBitmap( *m_currAttenuator->m_SchBitMap );

    wxString msg;

    msg.Printf( wxT( "%g" ), m_currAttenuator->m_Attenuation );
    m_AttValueCtrl->SetValue( msg );
    m_AttValueCtrl->Enable( m_currAttenuator->m_Attenuation_Enable );

    m_ZinValueCtrl->Enable( m_currAttenuator->m_Zin_Enable );

    if( m_currAttenuator->m_Zin_Enable )
        msg.Printf( wxT( "%g" ), m_currAttenuator->m_Zin );
    else
        msg.Clear();

    m_ZinValueCtrl->SetValue( msg );

    msg.Printf( wxT( "%g" ), m_currAttenuator->m_Zout );
    m_ZoutValueCtrl->SetValue( msg );

    if( m_currAttenuator->m_FormulaName )
    {
        if( m_currAttenuator->m_FormulaName->StartsWith( "<!" ) )
        {
            m_panelAttFormula->SetPage( *m_currAttenuator->m_FormulaName );
        }
        else
        {
            wxString html_txt;
            ConvertMarkdown2Html( wxGetTranslation( *m_currAttenuator->m_FormulaName ), html_txt );
            m_panelAttFormula->SetPage( html_txt );
        }
    }
    else
    {
        m_panelAttFormula->SetPage( wxEmptyString );
    }
}


void PCB_CALCULATOR_FRAME::TransfAttenuatorResultsToPanel()
{
    wxString msg;

    m_Attenuator_Messages->SetPage( wxEmptyString );

    if( m_currAttenuator->m_Error )
    {
        msg.Printf( _( "Attenuation more than %f dB" ),
                    m_currAttenuator->m_MinimumATT );
        m_Attenuator_Messages->AppendToPage( wxT( "<br><b>Error!</b></br><br><em>" ) );
        m_Attenuator_Messages->AppendToPage( msg );
        m_Attenuator_Messages->AppendToPage( wxT( "</em></br>" ) );

        // Display -- as resistor values:
        msg = wxT( "--" );
        m_Att_R1_Value->SetValue( msg );
        m_Att_R2_Value->SetValue( msg );
        if( m_currAttenuator->m_ResultCount  >= 3 )
            m_Att_R3_Value->SetValue( msg );

        return;
    }

    msg.Printf( wxT( "%g" ), m_currAttenuator->m_R1 );
    m_Att_R1_Value->SetValue( msg );
    msg.Printf( wxT( "%g" ), m_currAttenuator->m_R2 );
    m_Att_R2_Value->SetValue( msg );

    if( m_currAttenuator->m_ResultCount  < 3 )
    {
        m_Att_R3_Value->SetValue( wxEmptyString );
    }
    else
    {
        msg.Printf( wxT( "%g" ), m_currAttenuator->m_R3 );
        m_Att_R3_Value->SetValue( msg );
    }
}


