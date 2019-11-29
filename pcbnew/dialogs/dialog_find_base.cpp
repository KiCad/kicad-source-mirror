///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_find_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	staticText1 = new wxStaticText( this, wxID_ANY, wxT("Search for :"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText1->Wrap( -1 );
	bSizer8->Add( staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_searchCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_searchCombo->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	bSizer8->Add( m_searchCombo, 1, wxALL|wxEXPAND, 5 );


	bSizer131->Add( bSizer8, 1, wxEXPAND, 5 );

	wxBoxSizer* sizerOptions;
	sizerOptions = new wxBoxSizer( wxHORIZONTAL );

	m_matchCase = new wxCheckBox( this, wxID_ANY, wxT("Match case"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchCase, 0, wxALL, 5 );

	m_matchWords = new wxCheckBox( this, wxID_ANY, wxT("Words"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchWords, 0, wxALL, 5 );

	m_wildcards = new wxCheckBox( this, wxID_ANY, wxT("Wildcards"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_wildcards, 0, wxALL, 5 );

	m_wrap = new wxCheckBox( this, wxID_ANY, wxT("Wrap"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wrap->SetValue(true);
	sizerOptions->Add( m_wrap, 0, wxALL, 5 );


	bSizer131->Add( sizerOptions, 1, wxALL|wxEXPAND, 0 );

	wxBoxSizer* sizerInclude;
	sizerInclude = new wxBoxSizer( wxVERTICAL );

	m_includeTexts = new wxCheckBox( this, wxID_ANY, wxT("Search for texts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeTexts->SetValue(true);
	sizerInclude->Add( m_includeTexts, 0, wxALL, 5 );

	m_includeValues = new wxCheckBox( this, wxID_ANY, wxT("Search for item values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeValues->SetValue(true);
	sizerInclude->Add( m_includeValues, 0, wxALL, 5 );

	m_includeReferences = new wxCheckBox( this, wxID_ANY, wxT("Search for item references"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeReferences->SetValue(true);
	sizerInclude->Add( m_includeReferences, 0, wxALL, 5 );

	m_includeMarkers = new wxCheckBox( this, wxID_ANY, wxT("Search for DRC markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeMarkers->SetValue(true);
	sizerInclude->Add( m_includeMarkers, 0, wxALL, 5 );

	m_includeVias = new wxCheckBox( this, wxID_ANY, wxT("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeVias->SetValue(true);
	m_includeVias->Hide();

	sizerInclude->Add( m_includeVias, 0, wxALL, 5 );


	bSizer131->Add( sizerInclude, 0, wxEXPAND, 5 );


	bSizer13->Add( bSizer131, 1, wxEXPAND, 5 );

	wxBoxSizer* sizerButtons;
	sizerButtons = new wxBoxSizer( wxVERTICAL );

	m_findNext = new wxButton( this, wxID_ANY, wxT("Find Next"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerButtons->Add( m_findNext, 0, wxALIGN_TOP|wxALL|wxEXPAND, 5 );

	m_findPrevious = new wxButton( this, wxID_ANY, wxT("Find Previous"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerButtons->Add( m_findPrevious, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	m_searchAgain = new wxButton( this, wxID_ANY, wxT("Restart Search"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerButtons->Add( m_searchAgain, 0, wxALIGN_RIGHT|wxALL|wxEXPAND, 5 );


	bSizer13->Add( sizerButtons, 0, wxALIGN_RIGHT, 5 );


	bSizer10->Add( bSizer13, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerStatus;
	sizerStatus = new wxBoxSizer( wxHORIZONTAL );

	m_status = new wxStaticText( this, wxID_ANY, wxT("Status"), wxDefaultPosition, wxDefaultSize, 0 );
	m_status->Wrap( -1 );
	sizerStatus->Add( m_status, 0, wxALL, 5 );

	m_cancel = new wxButton( this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cancel->Hide();

	sizerStatus->Add( m_cancel, 0, wxALL, 5 );

	m_gauge = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_gauge->SetValue( 0 );
	m_gauge->Hide();

	sizerStatus->Add( m_gauge, 1, wxALIGN_CENTER_HORIZONTAL|wxALL|wxRIGHT, 5 );


	bSizer10->Add( sizerStatus, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::onClose ) );
	m_searchCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_findNext->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );
}

DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::onClose ) );
	m_searchCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_findNext->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );

}
