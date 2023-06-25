///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_data_collection_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DATA_COLLECTION_BASE::PANEL_DATA_COLLECTION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_stExplanation = new wxStaticText( this, wxID_ANY, _("KiCad can anonymously report crashes and special event data to developers in order to aid identifying critical bugs across the user base more effectively and help profile functionality to guide improvements.\n\nTo link automatic reports from the same KiCad install, a unique identifier is generated that is completely random, and is only used for the purposes of crash reporting. No personally identifiable information (PII) including IP address is stored or connected to this identifier. You may reset this id at anytime on the Preferences page for Data Collection.\n\nIf you choose to voluntarily participate, KiCad will automatically handle sending said reports when crashes or events occur. Your design files such as schematic or PCB are never shared in this process."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stExplanation->Wrap( 500 );
	bSizer8->Add( m_stExplanation, 0, wxALL, 5 );

	m_cbOptIn = new wxCheckBox( this, wxID_ANY, _("I agree to provide anonymous reports"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_cbOptIn, 0, wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_sentryUid = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_sentryUid->SetMinSize( wxSize( 340,-1 ) );

	bSizer3->Add( m_sentryUid, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_buttonResetId = new wxButton( this, wxID_ANY, _("Reset Unique Id"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_buttonResetId, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );


	bSizer8->Add( bSizer3, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bPanelSizer->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_buttonResetId->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DATA_COLLECTION_BASE::OnResetIdClick ), NULL, this );
}

PANEL_DATA_COLLECTION_BASE::~PANEL_DATA_COLLECTION_BASE()
{
	// Disconnect Events
	m_buttonResetId->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DATA_COLLECTION_BASE::OnResetIdClick ), NULL, this );

}
