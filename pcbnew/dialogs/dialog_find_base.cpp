///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	m_matchCase = new wxCheckBox( this, wxID_ANY, _("Match &case"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchCase, 0, wxALL, 5 );


	sizerOptions->Add( 0, 0, 1, wxEXPAND, 5 );

	m_matchWords = new wxCheckBox( this, wxID_ANY, _("Whole &words only"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_matchWords, 0, wxALL, 5 );


	sizerOptions->Add( 0, 0, 1, wxEXPAND, 5 );

	m_wildcards = new wxCheckBox( this, wxID_ANY, _("Wi&ldcards"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerOptions->Add( m_wildcards, 0, wxALL, 5 );


	sizerOptions->Add( 0, 0, 1, wxEXPAND, 5 );

	m_wrap = new wxCheckBox( this, wxID_ANY, _("Wra&p"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wrap->SetValue(true);
	sizerOptions->Add( m_wrap, 0, wxALL, 5 );


	leftSizer->Add( sizerOptions, 1, wxALL|wxEXPAND, 0 );


	leftSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* sizerInclude;
	sizerInclude = new wxFlexGridSizer( 0, 2, 6, 20 );
	sizerInclude->SetFlexibleDirection( wxBOTH );
	sizerInclude->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_includeReferences = new wxCheckBox( this, wxID_ANY, _("Search footprint reference &designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeReferences->SetValue(true);
	sizerInclude->Add( m_includeReferences, 0, 0, 5 );

	m_includeMarkers = new wxCheckBox( this, wxID_ANY, _("Search DRC &markers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeMarkers->SetValue(true);
	sizerInclude->Add( m_includeMarkers, 0, 0, 5 );

	m_includeValues = new wxCheckBox( this, wxID_ANY, _("Search footprint &values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeValues->SetValue(true);
	sizerInclude->Add( m_includeValues, 0, 0, 5 );

	m_includeNets = new wxCheckBox( this, wxID_ANY, _("Search &net names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeNets->SetValue(true);
	sizerInclude->Add( m_includeNets, 0, 0, 5 );

	m_checkAllFields = new wxCheckBox( this, wxID_ANY, _("Include &hidden fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sizerInclude->Add( m_checkAllFields, 0, 0, 5 );


	sizerInclude->Add( 0, 0, 1, wxEXPAND, 5 );

	m_includeTexts = new wxCheckBox( this, wxID_ANY, _("Search &other text items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_includeTexts->SetValue(true);
	sizerInclude->Add( m_includeTexts, 0, 0, 5 );


	leftSizer->Add( sizerInclude, 0, wxEXPAND|wxALL, 5 );


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


	bSizer10->Add( topSizer, 0, wxALL|wxEXPAND, 5 );

	wxStaticLine* staticline;
	staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer10->Add( staticline, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* sizerStatus;
	sizerStatus = new wxBoxSizer( wxHORIZONTAL );

	m_status = new wxStaticText( this, wxID_ANY, _("Status"), wxDefaultPosition, wxDefaultSize, 0 );
	m_status->Wrap( -1 );
	sizerStatus->Add( m_status, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sizerStatus->Add( 0, 0, 1, wxEXPAND, 5 );

	m_searchPanelLink = new wxHyperlinkCtrl( this, wxID_ANY, _("Show search panel"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	sizerStatus->Add( m_searchPanelLink, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer10->Add( sizerStatus, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_searchCombo->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_matchCase->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_matchWords->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_wildcards->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeReferences->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeMarkers->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeValues->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeNets->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_checkAllFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeTexts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_findNext->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::OnCloseButtonClick ), NULL, this );
	m_searchPanelLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_FIND_BASE::onShowSearchPanel ), NULL, this );
}

DIALOG_FIND_BASE::~DIALOG_FIND_BASE()
{
	// Disconnect Events
	m_searchCombo->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_FIND_BASE::onTextEnter ), NULL, this );
	m_matchCase->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_matchWords->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_wildcards->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeReferences->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeMarkers->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeValues->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeNets->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_checkAllFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_includeTexts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onOptionChanged ), NULL, this );
	m_findNext->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindNextClick ), NULL, this );
	m_findPrevious->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onFindPreviousClick ), NULL, this );
	m_searchAgain->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::onSearchAgainClick ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIND_BASE::OnCloseButtonClick ), NULL, this );
	m_searchPanelLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_FIND_BASE::onShowSearchPanel ), NULL, this );

}
