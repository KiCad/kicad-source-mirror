///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_annotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ANNOTATE_BASE::DIALOG_ANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_userMessage = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_userMessage->Wrap( 1 );
	m_userMessage->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	m_userMessage->Hide();
	
	bupperSizer->Add( m_userMessage, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxString m_rbScopeChoices[] = { _("Use the entire schematic"), _("Use the current page only") };
	int m_rbScopeNChoices = sizeof( m_rbScopeChoices ) / sizeof( wxString );
	m_rbScope = new wxRadioBox( this, wxID_ANY, _("Scope:"), wxDefaultPosition, wxDefaultSize, m_rbScopeNChoices, m_rbScopeChoices, 1, wxRA_SPECIFY_COLS );
	m_rbScope->SetSelection( 0 );
	fgSizer1->Add( m_rbScope, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Order:") ), wxVERTICAL );
	
	wxBoxSizer* bSizerXpos;
	bSizerXpos = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbSortBy_X_Position = new wxRadioButton( sbSizer1->GetStaticBox(), ID_SORT_BY_X_POSITION, _("Sort components by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxBOTTOM|wxTOP, 3 );
	
	
	bSizerXpos->Add( 0, 0, 1, 0, 5 );
	
	annotate_down_right_bitmap = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	
	sbSizer1->Add( bSizerXpos, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbSortBy_Y_Position = new wxRadioButton( sbSizer1->GetStaticBox(), ID_SORT_BY_Y_POSITION, _("Sort components by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxBOTTOM|wxTOP, 3 );
	
	
	bSizerYpos->Add( 0, 0, 1, 0, 5 );
	
	annotate_right_down_bitmap = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	
	sbSizer1->Add( bSizerYpos, 0, wxEXPAND, 5 );
	
	
	fgSizer1->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_rbOptionsChoices[] = { _("Keep existing annotations"), _("Reset existing annotations"), _("Reset, but keep order of multi-unit parts") };
	int m_rbOptionsNChoices = sizeof( m_rbOptionsChoices ) / sizeof( wxString );
	m_rbOptions = new wxRadioBox( this, wxID_ANY, _("Options:"), wxDefaultPosition, wxDefaultSize, m_rbOptionsNChoices, m_rbOptionsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOptions->SetSelection( 0 );
	fgSizer1->Add( m_rbOptions, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Numbering:") ), wxVERTICAL );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_rbFirstFree = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_FIRST_FREE, _("Use first free number after:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizer1->Add( m_rbFirstFree, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxTOP, 2 );
	
	m_textNumberAfter = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), 0 );
	gbSizer1->Add( m_textNumberAfter, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxLEFT, 2 );
	
	m_rbSheetX100 = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_SHEET_X_100, _("First free after sheet number X 100"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbSheetX100, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 4 );
	
	m_rbSheetX1000 = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_SHEET_X_1000, _("First free after sheet number X 1000"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbSheetX1000, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 2 );
	
	
	sbSizer2->Add( gbSizer1, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( sbSizer2, 1, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( fgSizer1, 0, wxBOTTOM|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDldOptions;
	bSizerDldOptions = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerDlgChoices;
	bSizerDlgChoices = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerChoiceClose;
	bSizerChoiceClose = new wxBoxSizer( wxHORIZONTAL );
	
	m_cbKeepDlgOpen = new wxCheckBox( this, wxID_ANY, _("Keep this dialog open"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceClose->Add( m_cbKeepDlgOpen, 0, wxALL, 5 );
	
	
	bSizerChoiceClose->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerDlgChoices->Add( bSizerChoiceClose, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerChoiceSilentMode;
	bSizerChoiceSilentMode = new wxBoxSizer( wxHORIZONTAL );
	
	m_cbSkipConfirmation = new wxCheckBox( this, wxID_ANY, _("Don't ask for confirmation"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceSilentMode->Add( m_cbSkipConfirmation, 0, wxALL, 5 );
	
	
	bSizerChoiceSilentMode->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerDlgChoices->Add( bSizerChoiceSilentMode, 1, wxEXPAND, 5 );
	
	
	bSizerDldOptions->Add( bSizerDlgChoices, 1, wxLEFT, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	bButtonsSizer->SetMinSize( wxSize( 160,-1 ) ); 
	m_btnApply = new wxButton( this, wxID_APPLY, _("Annotate"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnApply, 0, wxALL|wxEXPAND, 5 );
	
	m_btnClear = new wxButton( this, ID_CLEAR_ANNOTATION_CMP, _("Clear Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClear, 0, wxALL|wxEXPAND, 5 );
	
	m_btnClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClose, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerDldOptions->Add( bButtonsSizer, 0, 0, 5 );
	
	
	bupperSizer->Add( bSizerDldOptions, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,120 ) );
	
	bupperSizer->Add( m_MessageWindow, 5, wxEXPAND | wxALL, 5 );
	
	
	bmainSizer->Add( bupperSizer, 1, wxALL|wxEXPAND, 6 );
	
	
	this->SetSizer( bmainSizer );
	this->Layout();
	bmainSizer->Fit( this );
	
	// Connect Events
	m_btnApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnApplyClick ), NULL, this );
	m_btnClear->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationCmpClick ), NULL, this );
}

DIALOG_ANNOTATE_BASE::~DIALOG_ANNOTATE_BASE()
{
	// Disconnect Events
	m_btnApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnApplyClick ), NULL, this );
	m_btnClear->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationCmpClick ), NULL, this );
	
}
