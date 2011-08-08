/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
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
#include "wx/wx.h"
#include "wx/config.h"

#include "pcb_calculator.h"
#include "UnitSelector.h"

#include "pcb_calculator.xpm"

#define KEYWORD_FRAME_POSX                    wxT( "Pcb_calculator_Pos_x" )
#define KEYWORD_FRAME_POSY                    wxT( "Pcb_calculator_Pos_y" )
#define KEYWORD_FRAME_SIZEX                   wxT( "Pcb_calculator_Size_x" )
#define KEYWORD_FRAME_SIZEY                   wxT( "Pcb_calculator_Size_y" )
#define KEYWORD_TRANSLINE_SELECTION           wxT( "Transline_selection" )
#define KEYWORD_PAGE_SELECTION                wxT( "Page_selection" )
#define KEYWORD_COLORCODE_SELECTION           wxT( "CC_selection" )
#define KEYWORD_ATTENUATORS_SELECTION         wxT( "Att_selection" )
#define KEYWORD_BRDCLASS_SELECTION            wxT( "BrdClass_selection" )
#define KEYWORD_ELECTRICAL_SPACING_SELECTION  wxT( "ElectSpacing_selection" )
#define KEYWORD_ELECTRICAL_SPACING_VOLTAGE    wxT( "ElectSpacing_voltage" )
#define KEYWORD_REGUL_R1            wxT( "RegulR1" )
#define KEYWORD_REGUL_R2            wxT( "RegulR2" )
#define KEYWORD_REGUL_VREF          wxT( "RegulVREF" )
#define KEYWORD_REGUL_VOUT          wxT( "RegulVOUT" )

PCB_CALCULATOR_FRAME::PCB_CALCULATOR_FRAME( wxWindow * parent ) :
        PCB_CALCULATOR_FRAME_BASE( parent )
{
    m_currTransLine     = NULL;
    m_currTransLineType = default_type;
    m_currAttenuator = NULL;
    m_Config = new wxConfig();

    // Populate transline list ordered like in dialog menu list
    transline_type_id tltype_list[8] =
    {
        microstrip_type,    cpw_type,  grounded_cpw_type,
        rectwaveguide_type, coax_type, c_microstrip_type,
        stripline_type,     twistedpair_type
    };
    for( int ii = 0; ii < 8; ii++ )
        m_transline_list.push_back( new TRANSLINE_IDENT( tltype_list[ii] ) );

    // Populate attenuator list ordered like in dialog menu list
    m_attenuator_list.push_back( new ATTENUATOR_PI() );
    m_attenuator_list.push_back( new ATTENUATOR_TEE() );
    m_attenuator_list.push_back( new ATTENUATOR_BRIDGE() );
    m_attenuator_list.push_back( new ATTENUATOR_SPLITTER() );
    m_currAttenuator = m_attenuator_list[0];

    ReadConfig();

    TranslineTypeSelection( m_currTransLineType );
    m_TranslineSelection->SetSelection( m_currTransLineType );

    TW_Init();

    SetAttenuator( m_AttenuatorsSelection->GetSelection() );

    ToleranceSelection( m_rbToleranceSelection->GetSelection() );

    BoardClassesUpdateData( m_BoardClassesUnitsSelector->GetUnitScale() );

    ElectricalSpacingUpdateData( m_ElectricalSpacingUnitsSelector->GetUnitScale() );

    #ifdef __WINDOWS__
    SetIcon( wxICON( pcb_calculator_icon ) );
    #else
    SetIcon( wxICON( pcb_calculator ) );
    #endif

    GetSizer()->SetSizeHints( this );

    // Set previous size and position
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if( m_FramePos == wxDefaultPosition )
        Centre();
}


PCB_CALCULATOR_FRAME::~PCB_CALCULATOR_FRAME()
{
    WriteConfig();
    for( unsigned ii = 0; ii < m_transline_list.size(); ii++ )
        delete m_transline_list[ii];

    for( unsigned ii = 0; ii < m_attenuator_list.size(); ii++ )
        delete m_attenuator_list[ii];

    delete m_Config;

    /* This needed for OSX: avoids furter OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze();
}


void PCB_CALCULATOR_FRAME::ReadConfig()
{
    if( m_Config == NULL )
        return;

    long     ltmp;
    wxString msg;
    m_Config->Read( KEYWORD_FRAME_POSX, &m_FramePos.x, -1 );
    m_Config->Read( KEYWORD_FRAME_POSY, &m_FramePos.y, -1 );
    m_Config->Read( KEYWORD_FRAME_SIZEX, &m_FrameSize.x, -1 );
    m_Config->Read( KEYWORD_FRAME_SIZEY, &m_FrameSize.y, -1 );
    m_Config->Read( KEYWORD_TRANSLINE_SELECTION, &ltmp, (long) default_type );
    m_currTransLineType = (enum transline_type_id) ltmp;
    m_Config->Read( KEYWORD_PAGE_SELECTION, &ltmp, 0 );
    m_Notebook->ChangeSelection( ltmp );
    m_Config->Read( KEYWORD_COLORCODE_SELECTION, &ltmp, 1 );
    m_rbToleranceSelection->SetSelection( ltmp );
    m_Config->Read( KEYWORD_ATTENUATORS_SELECTION, &ltmp, 0 );
    m_AttenuatorsSelection->SetSelection( ltmp );
    m_Config->Read( KEYWORD_BRDCLASS_SELECTION, &ltmp, 0 );
    m_BoardClassesUnitsSelector->SetSelection( ltmp );
    m_Config->Read( KEYWORD_REGUL_R1, &msg, wxT("10") );
    m_RegulR1Value->SetValue( msg );
    m_Config->Read( KEYWORD_REGUL_R2, &msg, wxT("10") );
    m_RegulR2Value->SetValue( msg );
    m_Config->Read( KEYWORD_REGUL_VREF, &msg, wxT("3") );
    m_RegulVrefValue->SetValue( msg );
    m_Config->Read( KEYWORD_REGUL_VOUT, &msg, wxT("12") );
    m_RegulVoutValue->SetValue( msg );
    m_Config->Read( KEYWORD_ELECTRICAL_SPACING_SELECTION, &ltmp, 0 );
    m_ElectricalSpacingUnitsSelector->SetSelection( ltmp );
    m_Config->Read( KEYWORD_ELECTRICAL_SPACING_VOLTAGE, &msg, wxT("500") );
    m_ElectricalSpacingVoltage->SetValue( msg );

    for( unsigned ii = 0; ii < m_transline_list.size(); ii++ )
        m_transline_list[ii]->ReadConfig( m_Config );
    for( unsigned ii = 0; ii < m_attenuator_list.size(); ii++ )
        m_attenuator_list[ii]->ReadConfig( m_Config );
}


void PCB_CALCULATOR_FRAME::WriteConfig()
{
    if( m_Config == NULL )
        return;

    if( !IsIconized() )
    {
        m_FrameSize = GetSize();
        m_FramePos  = GetPosition();

        m_Config->Write( KEYWORD_FRAME_POSX, (long) m_FramePos.x );
        m_Config->Write( KEYWORD_FRAME_POSY, (long) m_FramePos.y );
        m_Config->Write( KEYWORD_FRAME_SIZEX, (long) m_FrameSize.x );
        m_Config->Write( KEYWORD_FRAME_SIZEY, (long) m_FrameSize.y );
    }
    m_Config->Write( KEYWORD_TRANSLINE_SELECTION, (long) m_currTransLineType );
    m_Config->Write( KEYWORD_PAGE_SELECTION, m_Notebook->GetSelection() );
    m_Config->Write( KEYWORD_COLORCODE_SELECTION, m_rbToleranceSelection->GetSelection() );
    m_Config->Write( KEYWORD_ATTENUATORS_SELECTION, m_AttenuatorsSelection->GetSelection());
    m_Config->Write( KEYWORD_BRDCLASS_SELECTION, m_BoardClassesUnitsSelector->GetSelection() );
    m_Config->Write( KEYWORD_REGUL_R1, m_RegulR1Value->GetValue() );
    m_Config->Write( KEYWORD_REGUL_R2, m_RegulR2Value->GetValue() );
    m_Config->Write( KEYWORD_REGUL_VREF, m_RegulVrefValue->GetValue() );
    m_Config->Write( KEYWORD_REGUL_VOUT, m_RegulVoutValue->GetValue() );
    m_Config->Write( KEYWORD_ELECTRICAL_SPACING_SELECTION,
                     m_ElectricalSpacingUnitsSelector->GetSelection() );
    m_Config->Write( KEYWORD_ELECTRICAL_SPACING_VOLTAGE,
                     m_ElectricalSpacingVoltage->GetValue() );

    TW_WriteConfig();

    for( unsigned ii = 0; ii < m_transline_list.size(); ii++ )
        m_transline_list[ii]->WriteConfig( m_Config );
    for( unsigned ii = 0; ii < m_attenuator_list.size(); ii++ )
        m_attenuator_list[ii]->WriteConfig( m_Config );
}



/**
 * Function OnTranslineAnalyse
 * Run a new analyse for the current transline with current parameters
 * and displays the electrical parmeters
 */
void PCB_CALCULATOR_FRAME::OnTranslineAnalyse( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->analyze();
    }
}

/**
 * Function OnTranslineSynthetize
 * Run a new synthezis for the current transline with current parameters
 * and displays the geometrical parmeters
 */
void PCB_CALCULATOR_FRAME::OnTranslineSynthetize( wxCommandEvent& event )
{
    if( m_currTransLine )
    {
        TransfDlgDataToTranslineParams();
        m_currTransLine->synthesize();
    }
}


void PCB_CALCULATOR_FRAME::OnPaintTranslinePanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelDisplayshape );

    TRANSLINE_IDENT* tr_ident = m_transline_list[m_currTransLineType];
    if( tr_ident )
    {
        wxSize size = m_panelDisplayshape->GetSize();
        size.x -= tr_ident->m_Icon->GetWidth();
        size.y -= tr_ident->m_Icon->GetHeight();
        dc.DrawBitmap( *tr_ident->m_Icon, size.x / 2, size.y / 2 );
    }

    event.Skip();
}
