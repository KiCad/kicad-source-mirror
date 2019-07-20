///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_schematic_find_base.h"

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
	gbSizer2->SetFlexibleDirection( wxHORIZONTAL );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );
	
	m_checkMatchCase = new wxCheckBox( this, wxID_ANY, _("&Match case"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkMatchCase, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_checkWholeWord = new wxCheckBox( this, wxID_ANY, _("Words"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkWholeWord->SetValue(true); 
	gbSizer2->Add( m_checkWholeWord, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_checkWildcardMatch = new wxCheckBox( this, wxID_ANY, _("Wildcards"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkWildcardMatch, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_checkAllFields = new wxCheckBox( this, wxID_ANY, _("Search all com&ponent fields"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkAllFields, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_checkAllPins = new wxCheckBox( this, wxID_ANY, _("Search all pin &names and numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkAllPins, wxGBPosition( 2, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_checkCurrentSheetOnly = new wxCheckBox( this, wxID_ANY, _("Search the current &sheet only"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_checkCurrentSheetOnly, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_checkReplaceReferences = new wxCheckBox( this, wxID_ANY, _("Replace componen&t reference designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkReplaceReferences->Hide();
	
	gbSizer2->Add( m_checkReplaceReferences, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	leftSizer->Add( gbSizer2, 1, wxEXPAND, 5 );
	
	
	topSizer->Add( leftSizer, 1, wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* rightSizer;
	rightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonFind = new wxButton( this, wxID_FIND, _("&Find"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonFind->SetDefault(); 
	rightSizer->Add( m_buttonFind, 0, wxALL|wxEXPAND, 6 );
	
	m_buttonReplace = new wxButton( this, wxID_REPLACE, _("&Replace"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonReplace->Hide();
	
	rightSizer->Add( m_buttonReplace, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );
	
	m_buttonReplaceAll = new wxButton( this, wxID_REPLACE_ALL, _("Replace &All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonReplaceAll->Hide();
	
	rightSizer->Add( m_buttonReplaceAll, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	rightSizer->Add( m_buttonCancel, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 6 );
	
	
	topSizer->Add( rightSizer, 0, wxALL|wxEXPAND, 6 );
	
	
	mainSizer->Add( topSizer, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_FIND_BASE::OnClose ) );
	m_comboFind->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForText ), NULL, this );
	m_comboFind->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnTextEnter ), NULL, this );
	m_comboFind->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_comboReplace->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnTextEnter ), NULL, this );
	m_comboReplace->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_checkMatchCase->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWholeWord->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWildcardMatch->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkAllFields->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkAllPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkCurrentSheetOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_buttonFind->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnFind ), NULL, this );
	m_buttonReplace->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplace->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceUI ), NULL, this );
	m_buttonReplaceAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplaceAll->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceAllUI ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnCancel ), NULL, this );
}

DIALOG_SCH_FIND_BASE::~DIALOG_SCH_FIND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_FIND_BASE::OnClose ) );
	m_comboFind->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnSearchForText ), NULL, this );
	m_comboFind->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnTextEnter ), NULL, this );
	m_comboFind->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnTextEnter ), NULL, this );
	m_comboReplace->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateDrcUI ), NULL, this );
	m_checkMatchCase->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWholeWord->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkWildcardMatch->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkAllFields->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkAllPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_checkCurrentSheetOnly->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnOptions ), NULL, this );
	m_buttonFind->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnFind ), NULL, this );
	m_buttonReplace->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplace->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceUI ), NULL, this );
	m_buttonReplaceAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnReplace ), NULL, this );
	m_buttonReplaceAll->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SCH_FIND_BASE::OnUpdateReplaceAllUI ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_FIND_BASE::OnCancel ), NULL, this );
	
}
