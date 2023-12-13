///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_board_class_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_BOARD_CLASS_BASE::PANEL_BOARD_CLASS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerBoardClass;
	bSizerBoardClass = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerUnitsMargins;
	bSizerUnitsMargins = new wxBoxSizer( wxVERTICAL );

	wxArrayString m_BoardClassesUnitsSelectorChoices;
	m_BoardClassesUnitsSelector = new UNIT_SELECTOR_LEN( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_BoardClassesUnitsSelectorChoices, 0 );
	m_BoardClassesUnitsSelector->SetSelection( 0 );
	bSizerUnitsMargins->Add( m_BoardClassesUnitsSelector, 0, wxTOP|wxBOTTOM|wxRIGHT, 32 );


	bSizerBoardClass->Add( bSizerUnitsMargins, 0, wxLEFT, 10 );

	wxBoxSizer* brdclsSizerRight;
	brdclsSizerRight = new wxBoxSizer( wxVERTICAL );

	m_staticTextBrdClass = new wxStaticText( this, wxID_ANY, _("Note: Values are minimal values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdClass->Wrap( -1 );
	m_staticTextBrdClass->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	brdclsSizerRight->Add( m_staticTextBrdClass, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_gridClassesValuesDisplay = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridClassesValuesDisplay->CreateGrid( 5, 6 );
	m_gridClassesValuesDisplay->EnableEditing( false );
	m_gridClassesValuesDisplay->EnableGridLines( true );
	m_gridClassesValuesDisplay->EnableDragGridSize( false );
	m_gridClassesValuesDisplay->SetMargins( 0, 0 );

	// Columns
	m_gridClassesValuesDisplay->SetColSize( 0, 100 );
	m_gridClassesValuesDisplay->SetColSize( 1, 100 );
	m_gridClassesValuesDisplay->SetColSize( 2, 100 );
	m_gridClassesValuesDisplay->SetColSize( 3, 100 );
	m_gridClassesValuesDisplay->SetColSize( 4, 100 );
	m_gridClassesValuesDisplay->SetColSize( 5, 100 );
	m_gridClassesValuesDisplay->EnableDragColMove( false );
	m_gridClassesValuesDisplay->EnableDragColSize( true );
	m_gridClassesValuesDisplay->SetColLabelValue( 0, _("Class 1") );
	m_gridClassesValuesDisplay->SetColLabelValue( 1, _("Class 2") );
	m_gridClassesValuesDisplay->SetColLabelValue( 2, _("Class 3") );
	m_gridClassesValuesDisplay->SetColLabelValue( 3, _("Class 4") );
	m_gridClassesValuesDisplay->SetColLabelValue( 4, _("Class 5") );
	m_gridClassesValuesDisplay->SetColLabelValue( 5, _("Class 6") );
	m_gridClassesValuesDisplay->SetColLabelSize( 30 );
	m_gridClassesValuesDisplay->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridClassesValuesDisplay->SetRowSize( 0, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 1, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 2, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 3, 24 );
	m_gridClassesValuesDisplay->SetRowSize( 4, 24 );
	m_gridClassesValuesDisplay->EnableDragRowSize( false );
	m_gridClassesValuesDisplay->SetRowLabelValue( 0, _("Lines width") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 1, _("Minimum clearance") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 2, _("Via: (diameter - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 3, _("Plated Pad: (diameter - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelValue( 4, _("NP Pad: (diameter - drill)") );
	m_gridClassesValuesDisplay->SetRowLabelSize( 200 );
	m_gridClassesValuesDisplay->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridClassesValuesDisplay->SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	brdclsSizerRight->Add( m_gridClassesValuesDisplay, 0, wxALL, 5 );

	m_panelShowClassPrms = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	brdclsSizerRight->Add( m_panelShowClassPrms, 1, wxALL|wxEXPAND, 5 );


	bSizerBoardClass->Add( brdclsSizerRight, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerBoardClass );
	this->Layout();
	bSizerBoardClass->Fit( this );

	// Connect Events
	m_BoardClassesUnitsSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_BOARD_CLASS_BASE::OnBoardClassesUnitsSelection ), NULL, this );
}

PANEL_BOARD_CLASS_BASE::~PANEL_BOARD_CLASS_BASE()
{
	// Disconnect Events
	m_BoardClassesUnitsSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_BOARD_CLASS_BASE::OnBoardClassesUnitsSelection ), NULL, this );

}
