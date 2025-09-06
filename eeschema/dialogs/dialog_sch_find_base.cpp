///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sch_find_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_FIND_BASE::DIALOG_SCH_FIND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* leftSizer;
	leftSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* leftGridSizer;
	leftGridSizer = new wxFlexGridSizer( 3, 2, 3, 3 );
	leftGridSizer->AddGrowableCol( 1 );
	leftGridSizer->SetFlexibleDirection( wxBOTH );
	leftGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("&Search for:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	leftGridSizer->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL, 6 );

	m_comboFind = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxTE_PROCESS_ENTER );
	m_comboFind->SetToolTip( _("Text with optional wildcards") );
	m_comboFind->SetMinSize( wxSize( 220,-1 ) );

	leftGridSizer->Add( m_comboFind, 0, wxEXPAND, 6 );

	m_staticReplace = new wxStaticText( this, wxID_ANY, _("Replace &with:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticReplace->Wrap( -1 );
	m_staticReplace->Hide();

	leftGridSizer->Add( m_staticReplace, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_comboReplace = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_comboReplace->Hide();

	leftGridSizer->Add( m_comboReplace, 0, wxEXPAND, 5 );

	m_staticDirection = new wxStaticText( this, wxID_ANY, _("Direction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticDirection->Wrap( -1 );
	m_staticDirection->Hide();

	leftGridSizer->Add( m_staticDirection, 0, 0, 5 );

	wxBoxSizer* directionSizer;
	directionSizer = new wxBoxSizer( wxVERTICAL );

	m_radioForward = new wxRadioButton( this, wxID_ANY, _("F&orward"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_radioForward->Hide();

	directionSizer->Add( m_radioForward, 0, wxALL, 3 );

	m_radioBackward = new wxRadioButton( this, wxID_ANY, _("&Backward"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBackward->Hide();

	directionSizer->Add( m_radioBackward, 0, wxALL, 3 );


	leftGridSizer->Add( directionSizer, 1, wxEXPAND, 5 );


	leftSizer->Add( leftGridSizer, 0, wxALL|wxEXPAND, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 20 );
	gbSizer2->SetFlexibleDirection( wxVERTICAL );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );
	gbSizer2->SetEmptyCellSize( wxSize( -1,8 ) );

	m_checkMatchCase = new wxCheckBox( this, wxID_ANY, _("Match &case"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkMatchCase, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_checkWholeWord = new wxCheckBox( this, wxID_ANY, _("Whole &words only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkWholeWord->SetValue(true);
	gbSizer2->Add( m_checkWholeWord, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_checkRegexMatch = new wxCheckBox( this, wxID_ANY, _("Regular Expression"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkRegexMatch, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_cbSearchPins = new wxCheckBox( this, wxID_ANY, _("Search pin &names and numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbSearchPins, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbSearchHiddenFields = new wxCheckBox( this, wxID_ANY, _("Include &hidden fields"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbSearchHiddenFields, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbCurrentSheetOnly = new wxCheckBox( this, wxID_ANY, _("Search the current &sheet only"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbCurrentSheetOnly, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbSelectedOnly = new wxCheckBox( this, wxID_ANY, _("Search the current selection &only"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbSelectedOnly, wxGBPosition( 5, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbReplaceReferences = new wxCheckBox( this, wxID_ANY, _("Replace matches in reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbReplaceReferences->Hide();

	gbSizer2->Add( m_cbReplaceReferences, wxGBPosition( 6, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbSearchNetNames = new wxCheckBox( this, wxID_ANY, _("Search &net names"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbSearchNetNames, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	leftSizer->Add( gbSizer2, 1, wxEXPAND|wxTOP, 5 );


	topSizer->Add( leftSizer, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* rightSizer;
	rightSizer = new wxBoxSizer( wxVERTICAL );

	m_buttonFind = new wxButton( this, wxID_FIND, _("Find"), wxDefaultPosition, wxDefaultSize, 0 );

	m_buttonFind->SetDefault();
	rightSizer->Add( m_buttonFind, 0, wxALL|wxEXPAND, 6 );

	m_buttonReplace = new wxButton( this, wxID_REPLACE, _("Replace"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonReplace->Hide();

	rightSizer->Add( m_buttonReplace, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );

	m_buttonReplaceAll = new wxButton( this, wxID_REPLACE_ALL, _("Replace All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonReplaceAll->Hide();

	rightSizer->Add( m_buttonReplaceAll, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );

	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	rightSizer->Add( m_buttonCancel, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 6 );


	topSizer->Add( rightSizer, 0, wxALL|wxEXPAND, 6 );


	mainSizer->Add( topSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer6->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_searchPanelLink = new wxHyperlinkCtrl( this, wxID_ANY, _("Show search panel"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bSizer6->Add( m_searchPanelLink, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );


	mainSizer->Add( bSizer6, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_FIND_BASE::OnClose ) );
	this->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_SCH_FIND_BASE::OnIdle ) );
	m_comboFind->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForSelect ), NULL, this );
	m_comboFind->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForText ), NULL, this );
	m_comboFind->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForEnter ), NULL, this );
	m_comboFind->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_comboReplace->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithSelect ), NULL, this );
	m_comboReplace->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithText ), NULL, this );
	m_comboReplace->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithEnter ), NULL, this );
	m_comboReplace->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_checkMatchCase->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWholeWord->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkRegexMatch->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchHiddenFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbCurrentSheetOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSelectedOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchNetNames->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_buttonFind->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnFind ), NULL, this );
	m_buttonReplace->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplace->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceUI ), NULL, this );
	m_buttonReplaceAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplaceAll->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceAllUI ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnCancel ), NULL, this );
	m_searchPanelLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_SCH_FIND_BASE::onShowSearchPanel ), NULL, this );
}

DIALOG_SCH_FIND_BASE::~DIALOG_SCH_FIND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_FIND_BASE::OnClose ) );
	this->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_SCH_FIND_BASE::OnIdle ) );
	m_comboFind->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForSelect ), NULL, this );
	m_comboFind->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForText ), NULL, this );
	m_comboFind->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForEnter ), NULL, this );
	m_comboFind->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithSelect ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithText ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplaceWithEnter ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_checkMatchCase->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWholeWord->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkRegexMatch->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchHiddenFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbCurrentSheetOnly->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSelectedOnly->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_cbSearchNetNames->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_buttonFind->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnFind ), NULL, this );
	m_buttonReplace->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplace->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceUI ), NULL, this );
	m_buttonReplaceAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplaceAll->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceAllUI ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnCancel ), NULL, this );
	m_searchPanelLink->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_SCH_FIND_BASE::onShowSearchPanel ), NULL, this );

}
