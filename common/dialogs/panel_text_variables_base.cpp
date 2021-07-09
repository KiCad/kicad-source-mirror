///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_text_variables_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_TEXT_VARIABLES_BASE::PANEL_TEXT_VARIABLES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_TextVars = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_TextVars->CreateGrid( 1, 2 );
	m_TextVars->EnableEditing( true );
	m_TextVars->EnableGridLines( true );
	m_TextVars->EnableDragGridSize( false );
	m_TextVars->SetMargins( 0, 0 );

	// Columns
	m_TextVars->SetColSize( 0, 150 );
	m_TextVars->SetColSize( 1, 454 );
	m_TextVars->EnableDragColMove( false );
	m_TextVars->EnableDragColSize( true );
	m_TextVars->SetColLabelSize( 22 );
	m_TextVars->SetColLabelValue( 0, _("Variable Name") );
	m_TextVars->SetColLabelValue( 1, _("Text Substitution") );
	m_TextVars->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_TextVars->EnableDragRowSize( true );
	m_TextVars->SetRowLabelSize( 0 );
	m_TextVars->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_TextVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_TextVars->SetMinSize( wxSize( 604,170 ) );

	bSizer3->Add( m_TextVars, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerEnvVarBtns;
	bSizerEnvVarBtns = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddTextVar = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerEnvVarBtns->Add( m_btnAddTextVar, 0, wxRIGHT, 5 );


	bSizerEnvVarBtns->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_btnDeleteTextVar = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerEnvVarBtns->Add( m_btnDeleteTextVar, 0, wxRIGHT|wxLEFT, 5 );


	bSizer3->Add( bSizerEnvVarBtns, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bPanelSizer->Add( bSizer3, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_TEXT_VARIABLES_BASE::OnUpdateUI ) );
	m_TextVars->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_TEXT_VARIABLES_BASE::OnGridCellChange ), NULL, this );
	m_TextVars->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_TEXT_VARIABLES_BASE::OnGridSize ), NULL, this );
	m_btnAddTextVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEXT_VARIABLES_BASE::OnAddTextVar ), NULL, this );
	m_btnDeleteTextVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEXT_VARIABLES_BASE::OnRemoveTextVar ), NULL, this );
}

PANEL_TEXT_VARIABLES_BASE::~PANEL_TEXT_VARIABLES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_TEXT_VARIABLES_BASE::OnUpdateUI ) );
	m_TextVars->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( PANEL_TEXT_VARIABLES_BASE::OnGridCellChange ), NULL, this );
	m_TextVars->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_TEXT_VARIABLES_BASE::OnGridSize ), NULL, this );
	m_btnAddTextVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEXT_VARIABLES_BASE::OnAddTextVar ), NULL, this );
	m_btnDeleteTextVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_TEXT_VARIABLES_BASE::OnRemoveTextVar ), NULL, this );

}
