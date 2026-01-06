///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"
#include "widgets/wx_infobar.h"

#include "dialog_annotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ANNOTATE_BASE::DIALOG_ANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bmainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	m_rbScope_Schematic = new wxRadioButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Entire schematic"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbScope_Schematic->SetValue( true );
	sbSizer3->Add( m_rbScope_Schematic, 0, wxLEFT|wxRIGHT, 5 );

	m_rbScope_Sheet = new wxRadioButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Current sheet only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_rbScope_Sheet, 0, wxLEFT|wxRIGHT, 5 );

	m_rbScope_Selection = new wxRadioButton( sbSizer3->GetStaticBox(), wxID_ANY, _("Selection"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_rbScope_Selection, 0, wxLEFT|wxRIGHT|wxBOTTOM, 5 );

	m_checkRecursive = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Recurse into subsheets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkRecursive->SetValue(true);
	sbSizer3->Add( m_checkRecursive, 0, wxALL, 5 );


	fgSizer1->Add( sbSizer3, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Order") ), wxVERTICAL );

	wxBoxSizer* bSizerXpos;
	bSizerXpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_X_Position = new wxRadioButton( sbSizer1->GetStaticBox(), ID_SORT_BY_X_POSITION, _("Sort symbols by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbSortBy_X_Position->SetValue( true );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	annotate_down_right_bitmap = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizer1->Add( bSizerXpos, 0, wxBOTTOM|wxEXPAND, 5 );

	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_Y_Position = new wxRadioButton( sbSizer1->GetStaticBox(), ID_SORT_BY_Y_POSITION, _("Sort symbols by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	annotate_right_down_bitmap = new wxStaticBitmap( sbSizer1->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizer1->Add( bSizerYpos, 0, wxEXPAND, 5 );


	fgSizer1->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_rbKeep_Annotations = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, _("Keep existing annotations"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	sbSizer4->Add( m_rbKeep_Annotations, 0, wxLEFT|wxRIGHT, 5 );

	m_rbReset_Annotations = new wxRadioButton( sbSizer4->GetStaticBox(), wxID_ANY, _("Reset existing annotations"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer4->Add( m_rbReset_Annotations, 0, wxLEFT|wxRIGHT, 5 );

	m_checkRegroupUnits = new wxCheckBox( sbSizer4->GetStaticBox(), wxID_ANY, _("Regroup symbol units"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer4->Add( m_checkRegroupUnits, 0, wxALL|wxEXPAND, 5 );


	fgSizer1->Add( sbSizer4, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Numbering") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbFirstFree = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_FIRST_FREE, _("Use first free number after:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbFirstFree->SetValue( true );
	gbSizer1->Add( m_rbFirstFree, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 2 );

	m_textNumberAfter = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), 0 );
	gbSizer1->Add( m_textNumberAfter, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 2 );

	m_rbSheetX100 = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_SHEET_X_100, _("First free after sheet number X 100"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbSheetX100, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 2 );

	m_rbSheetX1000 = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_SHEET_X_1000, _("First free after sheet number X 1000"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbSheetX1000, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 2 );


	sbSizer2->Add( gbSizer1, 1, wxEXPAND, 5 );


	fgSizer1->Add( sbSizer2, 1, wxALL|wxEXPAND, 5 );


	bupperSizer->Add( fgSizer1, 0, wxBOTTOM|wxEXPAND, 5 );

	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,120 ) );

	bupperSizer->Add( m_MessageWindow, 5, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bmainSizer->Add( bupperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 6 );

	wxBoxSizer* m_buttonsSizer;
	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_btnClear = new wxButton( this, ID_CLEAR_ANNOTATION_CMP, _("Clear Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_btnClear, 0, wxEXPAND|wxALL, 10 );


	m_buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_buttonsSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	bmainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();
	bmainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ANNOTATE_BASE::OnClose ) );
	m_rbSortBy_X_Position->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSortBy_Y_Position->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbReset_Annotations->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_checkRegroupUnits->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbFirstFree->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_textNumberAfter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX100->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX1000->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_btnClear->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnCloseClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnAnnotateClick ), NULL, this );
}

DIALOG_ANNOTATE_BASE::~DIALOG_ANNOTATE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_ANNOTATE_BASE::OnClose ) );
	m_rbSortBy_X_Position->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSortBy_Y_Position->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbReset_Annotations->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_checkRegroupUnits->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbFirstFree->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_textNumberAfter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX100->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX1000->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnOptionChanged ), NULL, this );
	m_btnClear->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnCloseClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnAnnotateClick ), NULL, this );

}
