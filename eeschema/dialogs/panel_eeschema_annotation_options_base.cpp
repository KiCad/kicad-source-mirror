///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-133-g388db8e4)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_annotation_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftColumn;
	bLeftColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerAutomatic;
	sbSizerAutomatic = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbol Annotation") ), wxVERTICAL );

	m_checkAutoAnnotate = new wxCheckBox( sbSizerAutomatic->GetStaticBox(), wxID_ANY, _("Automatically annotate symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkAutoAnnotate->SetValue(true);
	sbSizerAutomatic->Add( m_checkAutoAnnotate, 0, wxLEFT|wxRIGHT|wxBOTTOM, 5 );

	m_checkRecursive = new wxCheckBox( sbSizerAutomatic->GetStaticBox(), wxID_ANY, _("Recursively annotate subsheets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkRecursive->SetValue(true);
	sbSizerAutomatic->Add( m_checkRecursive, 0, wxLEFT|wxRIGHT|wxBOTTOM, 5 );


	bLeftColumn->Add( sbSizerAutomatic, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerOrder;
	sbSizerOrder = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Order") ), wxVERTICAL );

	wxBoxSizer* bSizerXpos;
	bSizerXpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_X_Position = new wxRadioButton( sbSizerOrder->GetStaticBox(), ID_SORT_BY_X_POSITION, _("Sort symbols by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxALIGN_CENTER_VERTICAL, 5 );


	annotate_down_right_bitmap = new wxStaticBitmap( sbSizerOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizerOrder->Add( bSizerXpos, 0, wxEXPAND, 5 );


	sbSizerOrder->Add( 0, 5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_Y_Position = new wxRadioButton( sbSizerOrder->GetStaticBox(), ID_SORT_BY_Y_POSITION, _("Sort symbols by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxALIGN_CENTER_VERTICAL, 5 );


	annotate_right_down_bitmap = new wxStaticBitmap( sbSizerOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizerOrder->Add( bSizerYpos, 0, wxEXPAND, 5 );


	bLeftColumn->Add( sbSizerOrder, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerNumbering;
	fgSizerNumbering = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizerNumbering->AddGrowableCol( 0 );
	fgSizerNumbering->AddGrowableCol( 1 );
	fgSizerNumbering->SetFlexibleDirection( wxBOTH );
	fgSizerNumbering->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticBoxSizer* sbSizerNumbering;
	sbSizerNumbering = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Numbering") ), wxVERTICAL );

	wxGridBagSizer* gbSizerNumbering;
	gbSizerNumbering = new wxGridBagSizer( 0, 0 );
	gbSizerNumbering->SetFlexibleDirection( wxBOTH );
	gbSizerNumbering->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbFirstFree = new wxRadioButton( sbSizerNumbering->GetStaticBox(), wxID_FIRST_FREE, _("Use first free number after:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizerNumbering->Add( m_rbFirstFree, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 2 );

	m_textNumberAfter = new wxTextCtrl( sbSizerNumbering->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), 0 );
	gbSizerNumbering->Add( m_textNumberAfter, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 2 );

	m_rbSheetX100 = new wxRadioButton( sbSizerNumbering->GetStaticBox(), wxID_SHEET_X_100, _("First free after sheet number X 100"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerNumbering->Add( m_rbSheetX100, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 2 );

	m_rbSheetX1000 = new wxRadioButton( sbSizerNumbering->GetStaticBox(), wxID_SHEET_X_1000, _("First free after sheet number X 1000"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerNumbering->Add( m_rbSheetX1000, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxTOP, 2 );


	sbSizerNumbering->Add( gbSizerNumbering, 1, wxEXPAND, 5 );


	fgSizerNumbering->Add( sbSizerNumbering, 1, wxALL|wxEXPAND, 5 );


	bLeftColumn->Add( fgSizerNumbering, 0, wxEXPAND, 5 );


	bPanelSizer->Add( bLeftColumn, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_rbSortBy_X_Position->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSortBy_Y_Position->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbFirstFree->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_textNumberAfter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX100->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX1000->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
}

PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::~PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE()
{
	// Disconnect Events
	m_rbSortBy_X_Position->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSortBy_Y_Position->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbFirstFree->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_textNumberAfter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX100->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );
	m_rbSheetX1000->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE::OnOptionChanged ), NULL, this );

}
