///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_change_symbols_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CHANGE_SYMBOLS_BASE::DIALOG_CHANGE_SYMBOLS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* matchSizerMargins;
	matchSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_matchSizer = new wxGridBagSizer( 3, 0 );
	m_matchSizer->SetFlexibleDirection( wxBOTH );
	m_matchSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_matchAll = new wxRadioButton( this, wxID_ANY, _("Update all symbols in schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchAll, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_matchBySelection = new wxRadioButton( this, wxID_ANY, _("Update selected symbol(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchBySelection, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_matchByReference = new wxRadioButton( this, wxID_ANY, _("Update symbols matching reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchByReference, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 2 );

	m_specifiedReference = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), wxTE_PROCESS_ENTER );
	m_matchSizer->Add( m_specifiedReference, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );

	m_matchByValue = new wxRadioButton( this, wxID_ANY, _("Update symbols matching value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchByValue, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_matchSizer->Add( m_specifiedValue, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_matchById = new wxRadioButton( this, wxID_ANY, _("Update symbols matching library identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_matchSizer->Add( m_matchById, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 6 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_specifiedId = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer10->Add( m_specifiedId, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_matchIdBrowserButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer10->Add( m_matchIdBrowserButton, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_matchSizer->Add( bSizer10, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT, 5 );


	m_matchSizer->AddGrowableCol( 1 );
	m_matchSizer->AddGrowableRow( 1 );

	matchSizerMargins->Add( m_matchSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_mainSizer->Add( matchSizerMargins, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	m_mainSizer->Add( 0, 8, 0, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline1, 0, wxEXPAND|wxALL, 4 );

	m_newIdSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_newIdLabel;
	m_newIdLabel = new wxStaticText( this, wxID_ANY, _("New library identifier:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_newIdLabel->Wrap( -1 );
	m_newIdSizer->Add( m_newIdLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_newId = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_newIdSizer->Add( m_newId, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_newIdBrowserButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_newIdSizer->Add( m_newIdBrowserButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_mainSizer->Add( m_newIdSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerUpdate;
	bSizerUpdate = new wxBoxSizer( wxHORIZONTAL );

	m_updateFieldsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update/Reset Fields") ), wxVERTICAL );

	wxArrayString m_fieldsBoxChoices;
	m_fieldsBox = new wxCheckListBox( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fieldsBoxChoices, wxLB_NEEDED_SB );
	m_fieldsBox->SetMinSize( wxSize( -1,120 ) );

	m_updateFieldsSizer->Add( m_fieldsBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* m_selBtnSizer;
	m_selBtnSizer = new wxBoxSizer( wxHORIZONTAL );

	m_selAllBtn = new wxButton( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selAllBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_selNoneBtn = new wxButton( m_updateFieldsSizer->GetStaticBox(), wxID_ANY, _("Select None"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selBtnSizer->Add( m_selNoneBtn, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_updateFieldsSizer->Add( m_selBtnSizer, 0, wxEXPAND, 5 );


	bSizerUpdate->Add( m_updateFieldsSizer, 2, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerUpdate->Add( 5, 0, 0, wxEXPAND, 5 );

	m_updateOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update Options") ), wxHORIZONTAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_removeExtraBox = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Remove fields if not in library symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeExtraBox->SetToolTip( _("Removes fields that do not occur in the original library symbols") );

	bSizer8->Add( m_removeExtraBox, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetEmptyFields = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Reset fields if empty in library symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_resetEmptyFields, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer8->Add( 0, 10, 1, wxEXPAND, 5 );

	m_resetFieldText = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field text"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_resetFieldText, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetFieldVisibilities = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_resetFieldVisibilities, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetFieldEffects = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field text sizes and styles"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_resetFieldEffects, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetFieldPositions = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset field positions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_resetFieldPositions, 0, wxBOTTOM|wxRIGHT, 10 );

	m_checkAll = new wxButton( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Check All Update Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_checkAll, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_updateOptionsSizer->Add( bSizer8, 1, wxEXPAND|wxRIGHT, 10 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxCheckBox* updateShapeAndPins;
	updateShapeAndPins = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update symbol shape and pins"), wxDefaultPosition, wxDefaultSize, 0 );
	updateShapeAndPins->SetValue(true);
	updateShapeAndPins->Enable( false );

	bSizer9->Add( updateShapeAndPins, 0, wxBOTTOM|wxRIGHT, 5 );

	wxCheckBox* updateKeywordsAndFootprintFilters;
	updateKeywordsAndFootprintFilters = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update keywords and footprint filters"), wxDefaultPosition, wxDefaultSize, 0 );
	updateKeywordsAndFootprintFilters->SetValue(true);
	updateKeywordsAndFootprintFilters->Enable( false );

	bSizer9->Add( updateKeywordsAndFootprintFilters, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer9->Add( 0, 10, 1, wxEXPAND, 5 );

	m_resetPinTextVisibility = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset pin name/number visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_resetPinTextVisibility, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetAlternatePin = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Reset alternate pin functions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_resetAlternatePin, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer9->Add( 0, 10, 1, wxEXPAND, 5 );

	m_resetAttributes = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset symbol attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_resetAttributes, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetCustomPower = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Reset custom power symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_resetCustomPower, 0, wxBOTTOM|wxRIGHT, 10 );

	m_uncheckAll = new wxButton( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Uncheck All Update Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_uncheckAll, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_updateOptionsSizer->Add( bSizer9, 1, wxEXPAND, 5 );


	bSizerUpdate->Add( m_updateOptionsSizer, 4, wxEXPAND|wxTOP|wxRIGHT, 10 );


	m_mainSizer->Add( bSizerUpdate, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_messagePanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagePanel->SetMinSize( wxSize( -1,200 ) );

	bSizer2->Add( m_messagePanel, 1, wxEXPAND | wxALL, 5 );


	m_mainSizer->Add( bSizer2, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_matchAll->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByAll ), NULL, this );
	m_matchBySelection->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchBySelected ), NULL, this );
	m_matchByReference->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByReference ), NULL, this );
	m_specifiedReference->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchTextKillFocus ), NULL, this );
	m_matchByValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByValue ), NULL, this );
	m_specifiedValue->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchTextKillFocus ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::OnMatchText ), NULL, this );
	m_matchById->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchById ), NULL, this );
	m_specifiedId->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchIDKillFocus ), NULL, this );
	m_matchIdBrowserButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchMatchIdSymbolBrowser ), NULL, this );
	m_newId->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onNewLibIDKillFocus ), NULL, this );
	m_newIdBrowserButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchNewIdSymbolBrowser ), NULL, this );
	m_selAllBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onSelectNone ), NULL, this );
	m_checkAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onCheckAll ), NULL, this );
	m_uncheckAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onUncheckAll ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onOkButtonClicked ), NULL, this );
}

DIALOG_CHANGE_SYMBOLS_BASE::~DIALOG_CHANGE_SYMBOLS_BASE()
{
	// Disconnect Events
	m_matchAll->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByAll ), NULL, this );
	m_matchBySelection->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchBySelected ), NULL, this );
	m_matchByReference->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByReference ), NULL, this );
	m_specifiedReference->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchTextKillFocus ), NULL, this );
	m_matchByValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchByValue ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchTextKillFocus ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::OnMatchText ), NULL, this );
	m_matchById->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchById ), NULL, this );
	m_specifiedId->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onMatchIDKillFocus ), NULL, this );
	m_matchIdBrowserButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchMatchIdSymbolBrowser ), NULL, this );
	m_newId->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onNewLibIDKillFocus ), NULL, this );
	m_newIdBrowserButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::launchNewIdSymbolBrowser ), NULL, this );
	m_selAllBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onSelectAll ), NULL, this );
	m_selNoneBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onSelectNone ), NULL, this );
	m_checkAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onCheckAll ), NULL, this );
	m_uncheckAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onUncheckAll ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CHANGE_SYMBOLS_BASE::onOkButtonClicked ), NULL, this );

}
