///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Nov  1 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_unused_pad_layers_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_UNUSED_PAD_LAYERS_BASE::DIALOG_UNUSED_PAD_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );


	bSizer5->Add( 0, 0, 0, wxEXPAND, 5 );

	wxString m_rbScopeChoices[] = { _("&Vias"), _("&Pads") };
	int m_rbScopeNChoices = sizeof( m_rbScopeChoices ) / sizeof( wxString );
	m_rbScope = new wxRadioBox( this, wxID_ANY, _("Scope"), wxDefaultPosition, wxDefaultSize, m_rbScopeNChoices, m_rbScopeChoices, 1, wxRA_SPECIFY_COLS );
	m_rbScope->SetSelection( 0 );
	bSizer5->Add( m_rbScope, 0, wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxString m_rbActionChoices[] = { _("&Remove Unused"), _("Reset &Unused") };
	int m_rbActionNChoices = sizeof( m_rbActionChoices ) / sizeof( wxString );
	m_rbAction = new wxRadioBox( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, m_rbActionNChoices, m_rbActionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbAction->SetSelection( 0 );
	bSizer5->Add( m_rbAction, 0, wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer5->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizer4->Add( bSizer5, 2, 0, 5 );

	m_cbSelectedOnly = new wxCheckBox( this, wxID_ANY, _("&Selection only"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbSelectedOnly, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_cbPreservePads = new wxCheckBox( this, wxID_ANY, _("&Keep pads at first and last layers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbPreservePads, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer6->Add( bSizer4, 1, wxEXPAND, 0 );


	bSizer2->Add( bSizer6, 4, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer2->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );


	bSizer3->Add( 0, 0, 0, wxEXPAND, 5 );

	m_image = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_image, 0, wxALL, 10 );


	bSizer3->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizer2->Add( bSizer3, 2, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_MainSizer->Add( bSizer2, 6, wxEXPAND, 5 );

	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();

	m_MainSizer->Add( m_StdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_rbScope->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onScopeChange ), NULL, this );
	m_rbAction->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreservePads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
}

DIALOG_UNUSED_PAD_LAYERS_BASE::~DIALOG_UNUSED_PAD_LAYERS_BASE()
{
	// Disconnect Events
	m_rbScope->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::onScopeChange ), NULL, this );
	m_rbAction->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );
	m_cbPreservePads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_UNUSED_PAD_LAYERS_BASE::syncImages ), NULL, this );

}
