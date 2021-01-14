///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 21 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_lib_edit_pin_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_PIN_TABLE_BASE::DIALOG_LIB_EDIT_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* top_sizer;
	top_sizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( 800,400 ), 0 );

	// Grid
	m_grid->CreateGrid( 5, 11 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 66 );
	m_grid->SetColSize( 1, 84 );
	m_grid->SetColSize( 2, 140 );
	m_grid->SetColSize( 3, 140 );
	m_grid->SetColSize( 4, 100 );
	m_grid->SetColSize( 5, 110 );
	m_grid->SetColSize( 6, 110 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
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
	m_grid->SetColLabelValue( 10, _("Visible") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 690,200 ) );

	top_sizer->Add( m_grid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 15 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_addButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer2->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_deleteButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer2->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_cbGroup = new wxCheckBox( this, wxID_ANY, _("Group by name"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_cbGroup, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_refreshButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer2->Add( m_refreshButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer2->Add( m_staticline2, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 10 );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Pin numbers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer2->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_summary = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_summary->Wrap( -1 );
	bSizer2->Add( m_summary, 1, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer2->Add( 10, 0, 0, wxEXPAND, 5 );

	m_Buttons = new wxStdDialogButtonSizer();
	m_ButtonsOK = new wxButton( this, wxID_OK );
	m_Buttons->AddButton( m_ButtonsOK );
	m_ButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_Buttons->AddButton( m_ButtonsCancel );
	m_Buttons->Realize();

	bSizer2->Add( m_Buttons, 0, wxEXPAND|wxALL, 5 );


	top_sizer->Add( bSizer2, 0, wxLEFT|wxEXPAND, 5 );


	this->SetSizer( top_sizer );
	this->Layout();
	top_sizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnAddRow ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_cbGroup->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_ButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_LIB_EDIT_PIN_TABLE_BASE::~DIALOG_LIB_EDIT_PIN_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnAddRow ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_cbGroup->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_ButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCancel ), NULL, this );

}
