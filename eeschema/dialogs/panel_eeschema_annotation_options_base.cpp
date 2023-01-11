///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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

	m_checkAutoAnnotate = new wxCheckBox( this, wxID_ANY, _("Automatically annotate symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkAutoAnnotate->SetValue(true);
	bLeftColumn->Add( m_checkAutoAnnotate, 0, wxTOP|wxLEFT, 12 );


	bLeftColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_orderLabel = new wxStaticText( this, wxID_ANY, _("Order"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orderLabel->Wrap( -1 );
	bLeftColumn->Add( m_orderLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizerXpos;
	bSizerXpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_X_Position = new wxRadioButton( this, ID_SORT_BY_X_POSITION, _("Sort symbols by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	annotate_down_right_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bLeftColumn->Add( bSizerXpos, 0, wxEXPAND|wxLEFT, 10 );

	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );

	m_rbSortBy_Y_Position = new wxRadioButton( this, ID_SORT_BY_Y_POSITION, _("Sort symbols by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxALIGN_CENTER_VERTICAL, 5 );

	annotate_right_down_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bLeftColumn->Add( bSizerYpos, 0, wxEXPAND|wxLEFT, 10 );


	bLeftColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_numberingLabel = new wxStaticText( this, wxID_ANY, _("Numbering"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numberingLabel->Wrap( -1 );
	bLeftColumn->Add( m_numberingLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftColumn->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizerNumbering;
	gbSizerNumbering = new wxGridBagSizer( 0, 0 );
	gbSizerNumbering->SetFlexibleDirection( wxBOTH );
	gbSizerNumbering->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbFirstFree = new wxRadioButton( this, wxID_FIRST_FREE, _("Use first free number after:"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizerNumbering->Add( m_rbFirstFree, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 2 );

	m_textNumberAfter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), 0 );
	gbSizerNumbering->Add( m_textNumberAfter, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 2 );

	m_rbSheetX100 = new wxRadioButton( this, wxID_SHEET_X_100, _("First free after sheet number X 100"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerNumbering->Add( m_rbSheetX100, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_rbSheetX1000 = new wxRadioButton( this, wxID_SHEET_X_1000, _("First free after sheet number X 1000"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerNumbering->Add( m_rbSheetX1000, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxTOP|wxBOTTOM|wxLEFT, 2 );


	bLeftColumn->Add( gbSizerNumbering, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bPanelSizer->Add( bLeftColumn, 0, wxEXPAND, 5 );


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
