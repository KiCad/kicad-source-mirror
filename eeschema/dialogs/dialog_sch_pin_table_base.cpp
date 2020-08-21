///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_sch_pin_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_PIN_TABLE_BASE::DIALOG_SCH_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* top_sizer;
	top_sizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );

	// Grid
	m_grid->CreateGrid( 5, 4 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 84 );
	m_grid->SetColSize( 1, 140 );
	m_grid->SetColSize( 2, 140 );
	m_grid->SetColSize( 3, 140 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelValue( 0, _("Number") );
	m_grid->SetColLabelValue( 1, _("Name") );
	m_grid->SetColLabelValue( 2, _("Electrical Type") );
	m_grid->SetColLabelValue( 3, _("Graphic Style") );
	m_grid->SetColLabelValue( 4, _("Orientation") );
	m_grid->SetColLabelValue( 5, _("Number Text Size") );
	m_grid->SetColLabelValue( 6, _("Name Text Size") );
	m_grid->SetColLabelValue( 7, _("Length") );
	m_grid->SetColLabelValue( 8, _("X Position") );
	m_grid->SetColLabelValue( 9, _("Y Position") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 512,320 ) );

	top_sizer->Add( m_grid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 15 );

	m_Buttons = new wxStdDialogButtonSizer();
	m_ButtonsOK = new wxButton( this, wxID_OK );
	m_Buttons->AddButton( m_ButtonsOK );
	m_ButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_Buttons->AddButton( m_ButtonsCancel );
	m_Buttons->Realize();

	top_sizer->Add( m_Buttons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( top_sizer );
	this->Layout();
	top_sizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnClose ) );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_ButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_SCH_PIN_TABLE_BASE::~DIALOG_SCH_PIN_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnClose ) );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_ButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_PIN_TABLE_BASE::OnCancel ), NULL, this );

}
