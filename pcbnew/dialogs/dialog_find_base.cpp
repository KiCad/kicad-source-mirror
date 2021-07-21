///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun 18 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_find_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FIND_BASE::DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* leftSizer;
	leftSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	searchStringLabel = new wxStaticText( this, wxID_ANY, _("Search for:"), wxDefaultPosition, wxDefaultSize, 0 );
	searchStringLabel->Wrap( -1 );
	bSizer8->Add( searchStringLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_searchCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxTE_PROCESS_ENTER );
	m_searchCombo->SetToolTip( _("Text with optional wildcards") );

	bSizer8->Add( m_searchCombo, 1, wxALL|wxEXPAND, 5 );


	leftSizer->Add( bSizer8, 1, wxEXPAND, 5 );

	wxBoxSizer* sizerOptions;
	sizerOptions = new wxBoxSizer( wxHORIZONTAL );

	m_matchCase = new wxCheckBox( this, wxID_ANY, _("Match case"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchCase, 0, wxALL, 5 );

	m_matchWords = new wxCheckBox( this, wxID_ANY, _("Words"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchWords, 0, wxALL, 5 );

	m_wildcards = new wxCheckBox( this, wxID_ANY, _("Wildcards"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_wildcards, 0, wxALL, 5 );

	m_wrap = new wxCheckBox( this, wxID_ANY, _("Wrap"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wrap->SetValue(true);
	sizerOptions->Add( m_wrap, 0, wxALL, 5 );


	leftSizer->Add( sizerOptions, 1, wxALL|wxEXPAND, 0 );

	wxBoxSizer* sizerInclude;
	sizerInclude = new wxBoxSizer( wxVERTICAL );

	m_includeReferences = new wxCheckBox( this, wxID_ANY, _("Search footprint reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeReferences->SetValue(true);
	sizerInclude->Add( m_includeReferences, 0, wxALL, 5 );

	m_includeValues = new wxCheckBox( this, wxID_ANY, _("Search footprint values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeValues->SetValue(true);
	sizerInclude->Add( m_includeValues, 0, wxALL, 5 );

	m_includeTexts = new wxCheckBox( this, wxID_ANY, _("Search other text items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeTexts->SetValue(true);
	sizerInclude->Add( m_includeTexts, 0, wxALL, 5 );

	m_includeMarkers = new wxCheckBox( this, wxID_ANY, _("Search DRC markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeMarkers->SetValue(true);
	sizerInclude->Add( m_includeMarkers, 0, wxALL, 5 );

	m_includeVias = new wxCheckBox( this, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeVias->SetValue(true);
	m_includeVias->Hide();

	sizerInclude->Add( m_includeVias, 0, wxALL, 5 );


	leftSizer->Add( sizerInclude, 0, wxEXPAND, 5 );


	topSizer->Add( leftSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* buttonSizer;
	buttonSizer = new wxBoxSizer( wxVERTICAL );

	m_findNext = new wxButton( this, wxID_ANY, _("Find Next"), wxDefaultPosition, wxDefaultSize, 0 );

	m_findNext->SetDefault();
	buttonSizer->Add( m_findNext, 0, wxALIGN_TOP|wxALL|wxEXPAND, 5 );

	m_findPrevious = new wxButton( this, wxID_ANY, _("Find Previous"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( m_findPrevious, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	m_searchAgain = new wxButton( this, wxID_ANY, _("Restart Search"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( m_searchAgain, 0, wxALL|wxEXPAND, 5 );

	m_closeButton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( m_closeButton, 0, wxALL|wxEXPAND, 5 );


	topSizer->Add( buttonSizer, 0, 0, 5 );


	bSizer10->Add( topSizer, 0, wxEXPAND, 5 );

	wxStaticLine* staticline;
	staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer10->Add( staticline, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* sizerStatus;
	sizerStatus = new wxBoxSizer( wxHORIZONTAL );

	m_status = new wxStaticText( this, wxID_ANY, _("Status"), wxDefaultPosition, wxDefaultSize, 0 );
	m_status->Wrap( -1 );
	sizerStatus->Add( m_status, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer10->Add( sizerStatus, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::OnClose ) );
	m_searchCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_findNext->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::OnCloseButtonClick ), NULL, this );
}

DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FIND_BASE::OnClose ) );
	m_searchCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_findNext->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::OnCloseButtonClick ), NULL, this );

}
