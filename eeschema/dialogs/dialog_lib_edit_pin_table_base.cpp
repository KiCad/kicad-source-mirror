///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_lib_edit_pin_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_PIN_TABLE_BASE::DIALOG_LIB_EDIT_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* top_sizer;
	top_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSummarySizer;
	bSummarySizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPinNumbers = new wxStaticText( this, wxID_ANY, _("Pin numbers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinNumbers->Wrap( -1 );
	bSummarySizer->Add( m_staticTextPinNumbers, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pin_numbers_summary = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pin_numbers_summary->Wrap( -1 );
	bSummarySizer->Add( m_pin_numbers_summary, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSummarySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextPinCount = new wxStaticText( this, wxID_ANY, _("Pin count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinCount->Wrap( -1 );
	bSummarySizer->Add( m_staticTextPinCount, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_pin_count = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pin_count->Wrap( -1 );
	bSummarySizer->Add( m_pin_count, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSummarySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextDuplicatePins = new wxStaticText( this, wxID_ANY, _("Duplicate pins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDuplicatePins->Wrap( -1 );
	bSummarySizer->Add( m_staticTextDuplicatePins, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_duplicate_pins = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_duplicate_pins->Wrap( -1 );
	bSummarySizer->Add( m_duplicate_pins, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	top_sizer->Add( bSummarySizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bGridMarginsSizer;
	bGridMarginsSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( 800,400 ), 0 );

	// Grid
	m_grid->CreateGrid( 5, 14 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 60 );
	m_grid->SetColSize( 1, 66 );
	m_grid->SetColSize( 2, 84 );
	m_grid->SetColSize( 3, 140 );
	m_grid->SetColSize( 4, 140 );
	m_grid->SetColSize( 5, 100 );
	m_grid->SetColSize( 6, 110 );
	m_grid->SetColSize( 7, 110 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->SetColSize( 11, 84 );
	m_grid->SetColSize( 12, 66 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Count") );
	m_grid->SetColLabelValue( 1, _("Number") );
	m_grid->SetColLabelValue( 2, _("Name") );
	m_grid->SetColLabelValue( 3, _("Electrical Type") );
	m_grid->SetColLabelValue( 4, _("Graphic Style") );
	m_grid->SetColLabelValue( 5, _("Orientation") );
	m_grid->SetColLabelValue( 6, _("Number Text Size") );
	m_grid->SetColLabelValue( 7, _("Name Text Size") );
	m_grid->SetColLabelValue( 8, _("Length") );
	m_grid->SetColLabelValue( 9, _("X Position") );
	m_grid->SetColLabelValue( 10, _("Y Position") );
	m_grid->SetColLabelValue( 11, _("Visible") );
	m_grid->SetColLabelValue( 12, _("Unit") );
	m_grid->SetColLabelValue( 13, _("De Morgan") );
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 690,200 ) );

	bGridMarginsSizer->Add( m_grid, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );


	top_sizer->Add( bGridMarginsSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bBottomSizer;
	bBottomSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_addButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_deleteButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_deleteButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_divider1 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_divider1->Enable( false );

	bSizer5->Add( m_divider1, 0, wxEXPAND|wxALL, 4 );

	m_cbGroup = new wxCheckBox( this, wxID_ANY, _("Group by name"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_cbGroup, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_groupSelected = new wxButton( this, wxID_ANY, _("Group Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_groupSelected, 0, wxALL|wxALIGN_CENTER_VERTICAL, 7 );

	m_refreshButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer5->Add( m_refreshButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_divider2 = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_divider2->Enable( false );

	bSizer5->Add( m_divider2, 0, wxEXPAND|wxALL, 4 );

	m_cbFilterByUnit = new wxCheckBox( this, wxID_ANY, _("Filter by unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_cbFilterByUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	wxArrayString m_unitFilterChoices;
	m_unitFilter = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitFilterChoices, 0 );
	m_unitFilter->SetSelection( 0 );
	bSizer5->Add( m_unitFilter, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bBottomSizer->Add( bSizer5, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bBottomSizer->Add( 30, 0, 1, wxEXPAND, 5 );

	m_Buttons = new wxStdDialogButtonSizer();
	m_ButtonsOK = new wxButton( this, wxID_OK );
	m_Buttons->AddButton( m_ButtonsOK );
	m_ButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_Buttons->AddButton( m_ButtonsCancel );
	m_Buttons->Realize();

	bBottomSizer->Add( m_Buttons, 0, wxEXPAND|wxALL, 5 );


	top_sizer->Add( bBottomSizer, 0, wxLEFT|wxEXPAND, 10 );


	this->SetSizer( top_sizer );
	this->Layout();
	top_sizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellSelected ), NULL, this );
	m_grid->Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellSelected ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnAddRow ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_cbGroup->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_groupSelected->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnGroupSelected ), NULL, this );
	m_refreshButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_cbFilterByUnit->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnFilterCheckBox ), NULL, this );
	m_unitFilter->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnFilterChoice ), NULL, this );
	m_ButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_LIB_EDIT_PIN_TABLE_BASE::~DIALOG_LIB_EDIT_PIN_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellEdited ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellSelected ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCellSelected ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnSize ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnAddRow ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_cbGroup->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_groupSelected->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnGroupSelected ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnRebuildRows ), NULL, this );
	m_cbFilterByUnit->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnFilterCheckBox ), NULL, this );
	m_unitFilter->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnFilterChoice ), NULL, this );
	m_ButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_PIN_TABLE_BASE::OnCancel ), NULL, this );

}
