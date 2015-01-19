/**
 * @file attenuators.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#include <pcb_calculator.h>
#include <attenuator_classes.h>

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
    m_Attenuator_Messages->Clear();
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
}


void PCB_CALCULATOR_FRAME::TransfAttenuatorResultsToPanel()
{
    wxString msg;

    m_Attenuator_Messages->Clear();

    if( m_currAttenuator->m_Error )
    {
        msg.Printf( _( "Error!\nSet attenuation more than %f dB" ),
                    m_currAttenuator->m_MinimumATT );
        m_Attenuator_Messages->AppendText( msg );
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
        m_Att_R3_Value->SetValue( wxEmptyString );
    else
    {
        msg.Printf( wxT( "%g" ), m_currAttenuator->m_R3 );
        m_Att_R3_Value->SetValue( msg );
    }
}


void PCB_CALCULATOR_FRAME::OnPaintAttenuatorPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelDisplayAttenuator );

    if( m_currAttenuator && m_currAttenuator->m_SchBitMap )
    {
        wxSize size = m_panelDisplayAttenuator->GetSize();
        size.x -= m_currAttenuator->m_SchBitMap->GetWidth();
        size.y -= m_currAttenuator->m_SchBitMap->GetHeight();
        dc.DrawBitmap( *m_currAttenuator->m_SchBitMap, size.x / 2, size.y / 2 );
    }

    event.Skip();
}


void PCB_CALCULATOR_FRAME::OnPaintAttFormulaPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelAttFormula );

    if( m_currAttenuator && m_currAttenuator->m_FormulaBitMap )
    {
        wxSize size = m_panelAttFormula->GetSize();
        size.x -= m_currAttenuator->m_FormulaBitMap->GetWidth();
        size.y -= m_currAttenuator->m_FormulaBitMap->GetHeight();
        dc.DrawBitmap( *m_currAttenuator->m_FormulaBitMap, size.x / 2, size.y / 2 );
    }

    event.Skip();
}
