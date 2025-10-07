/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/net_inspector_panel.h>

#include <eda_base_frame.h>

#include <widgets/ui_common.h>
#include <wx/statline.h>

NET_INSPECTOR_PANEL::NET_INSPECTOR_PANEL( wxWindow* parent, EDA_BASE_FRAME* aFrame, wxWindowID id,
                                          const wxPoint& pos, const wxSize& size, long style,
                                          const wxString& name ) :
        wxPanel( parent, id, pos, size, style, name ),
        m_frame( aFrame )
{
    m_sizerOuter = new wxGridBagSizer( 0, 0 );
    m_sizerOuter->SetFlexibleDirection( wxBOTH );
    m_sizerOuter->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_searchCtrl = new wxSearchCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
#ifndef __WXMAC__
    m_searchCtrl->ShowSearchButton( true );
#endif
    m_searchCtrl->ShowCancelButton( false );
    m_searchCtrl->SetDescriptiveText( _( "Filter" ) );
    m_sizerOuter->Add( m_searchCtrl, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ),
                       wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 2 );

    wxStaticLine* separator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    m_sizerOuter->Add( separator, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 3 );

    m_configureBtn = new BITMAP_BUTTON( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
    m_configureBtn->SetToolTip( _( "Configure netlist inspector" ) );
    m_configureBtn->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_configureBtn->SetPadding( 2 );
    m_sizerOuter->Add( m_configureBtn, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

    m_netsList = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE );
    m_netsList->SetFont( KIUI::GetDockedPaneFont( this ) );
    m_sizerOuter->Add( m_netsList, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxEXPAND, 5 );

    m_sizerOuter->AddGrowableCol( 0 );
    m_sizerOuter->AddGrowableRow( 1 );

    SetFont( KIUI::GetDockedPaneFont( this ) );
    SetSizer( m_sizerOuter );
    Layout();

    m_sizerOuter->Fit( this );

    // Connect Events
    m_frame->Bind( EDA_LANG_CHANGED, &NET_INSPECTOR_PANEL::OnLanguageChanged, this );

    Bind( wxEVT_SET_FOCUS, &NET_INSPECTOR_PANEL::OnSetFocus, this );
    Bind( wxEVT_SIZE, &NET_INSPECTOR_PANEL::OnSize, this );

    m_searchCtrl->Bind( wxEVT_COMMAND_TEXT_UPDATED, &NET_INSPECTOR_PANEL::OnSearchTextChanged, this );
    m_netsList->Bind( wxEVT_SET_FOCUS, &NET_INSPECTOR_PANEL::OnSetFocus, this );
    m_configureBtn->Bind( wxEVT_BUTTON, &NET_INSPECTOR_PANEL::OnConfigButton, this );
}


NET_INSPECTOR_PANEL::~NET_INSPECTOR_PANEL()
{
    // Disconnect Events
    m_frame->Unbind( EDA_LANG_CHANGED, &NET_INSPECTOR_PANEL::OnLanguageChanged, this );

    Unbind( wxEVT_SET_FOCUS, &NET_INSPECTOR_PANEL::OnSetFocus, this );
    Unbind( wxEVT_SIZE, &NET_INSPECTOR_PANEL::OnSize, this );

    m_searchCtrl->Unbind( wxEVT_COMMAND_TEXT_UPDATED, &NET_INSPECTOR_PANEL::OnSearchTextChanged, this );
    m_netsList->Unbind( wxEVT_SET_FOCUS, &NET_INSPECTOR_PANEL::OnSetFocus, this );
    m_configureBtn->Unbind( wxEVT_BUTTON, &NET_INSPECTOR_PANEL::OnConfigButton, this );
}


void NET_INSPECTOR_PANEL::OnLanguageChanged( wxCommandEvent& event )
{
    m_searchCtrl->SetDescriptiveText( _( "Filter" ) );
    m_configureBtn->SetToolTip( _( "Configure netlist inspector" ) );

    OnLanguageChangedImpl();

    event.Skip();
}
