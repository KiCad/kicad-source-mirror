///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_kicad_launcher_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_KICAD_LAUNCHER_BASE::PANEL_KICAD_LAUNCHER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_toolsSizer = new wxGridBagSizer( 5, 20 );
	m_toolsSizer->SetFlexibleDirection( wxBOTH );
	m_toolsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );


	m_mainSizer->Add( m_toolsSizer, 0, wxALL|wxEXPAND, 5 );


	m_mainSizer->Add( 0, 20, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerMessages;
	sbSizerMessages = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Messages") ), wxVERTICAL );

	m_messagesBox = new wxTextCtrl( sbSizerMessages->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_messagesBox->SetMinSize( wxSize( -1,60 ) );

	sbSizerMessages->Add( m_messagesBox, 1, wxALL|wxEXPAND, 5 );


	m_mainSizer->Add( sbSizerMessages, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );
}

PANEL_KICAD_LAUNCHER_BASE::~PANEL_KICAD_LAUNCHER_BASE()
{
}
