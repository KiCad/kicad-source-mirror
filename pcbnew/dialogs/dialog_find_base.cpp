///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Nov 22 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_find_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxVERTICAL );

	staticText1 = new wxStaticText( this, wxID_ANY, wxT("Search for :"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText1->Wrap( -1 );
	staticText1->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	bSizer131->Add( staticText1, 0, wxALL, 5 );

	m_searchCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_searchCombo->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
	m_searchCombo->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );

	bSizer131->Add( m_searchCombo, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* sizerOptions;
	sizerOptions = new wxBoxSizer( wxHORIZONTAL );

	m_matchCase = new wxCheckBox( this, wxID_ANY, wxT("Case"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchCase->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerOptions->Add( m_matchCase, 0, wxALL, 5 );

	m_matchWords = new wxCheckBox( this, wxID_ANY, wxT("Words"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchWords->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerOptions->Add( m_matchWords, 0, wxALL, 5 );

	m_wildcards = new wxCheckBox( this, wxID_ANY, wxT("Wildcards"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wildcards->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerOptions->Add( m_wildcards, 0, wxALL, 5 );

	m_wrap = new wxCheckBox( this, wxID_ANY, wxT("Wrap"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wrap->SetValue(true);
	m_wrap->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerOptions->Add( m_wrap, 0, wxALL, 5 );


	bSizer131->Add( sizerOptions, 1, wxALL|wxEXPAND, 0 );

	staticText2 = new wxStaticText( this, wxID_ANY, wxT("Include :"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText2->Wrap( -1 );
	staticText2->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	bSizer131->Add( staticText2, 0, wxALL, 5 );

	wxBoxSizer* sizerInclude;
	sizerInclude = new wxBoxSizer( wxHORIZONTAL );

	m_includeTexts = new wxCheckBox( this, wxID_ANY, wxT("Texts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeTexts->SetValue(true);
	m_includeTexts->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerInclude->Add( m_includeTexts, 0, wxALL, 5 );

	m_includeValues = new wxCheckBox( this, wxID_ANY, wxT("Values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeValues->SetValue(true);
	m_includeValues->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerInclude->Add( m_includeValues, 0, wxALL, 5 );

	m_includeReferences = new wxCheckBox( this, wxID_ANY, wxT("References"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeReferences->SetValue(true);
	m_includeReferences->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );

	sizerInclude->Add( m_includeReferences, 0, wxALL, 5 );

	m_includeVias = new wxCheckBox( this, wxID_ANY, wxT("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeVias->SetValue(true);
	m_includeVias->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
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

	m_searchAgain = new wxButton( this, wxID_ANY, wxT("Search Again"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerButtons->Add( m_searchAgain, 0, wxALIGN_RIGHT|wxALL|wxEXPAND, 5 );


	bSizer13->Add( sizerButtons, 0, wxALIGN_RIGHT, 5 );


	bSizer10->Add( bSizer13, 0, wxEXPAND, 5 );

	wxBoxSizer* sizerStatus;
	sizerStatus = new wxBoxSizer( wxHORIZONTAL );

	m_status = new wxTextCtrl( this, wxID_ANY, wxT("No hit"), wxDefaultPosition, wxDefaultSize, 0|wxBORDER_NONE );
	m_status->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
	m_status->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );

	sizerStatus->Add( m_status, 0, wxALL|wxEXPAND, 5 );

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
