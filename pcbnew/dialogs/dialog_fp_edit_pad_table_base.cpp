///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-75-g9786507b-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_fp_edit_pad_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_EDIT_PAD_TABLE_BASE::DIALOG_FP_EDIT_PAD_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSummarySizer;
	bSummarySizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPinNumbers = new wxStaticText( this, wxID_ANY, _("Pad numbers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinNumbers->Wrap( -1 );
	bSummarySizer->Add( m_staticTextPinNumbers, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pin_numbers_summary = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_pin_numbers_summary->Wrap( -1 );
	bSummarySizer->Add( m_pin_numbers_summary, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSummarySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextPinCount = new wxStaticText( this, wxID_ANY, _("Pad count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPinCount->Wrap( -1 );
	bSummarySizer->Add( m_staticTextPinCount, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_pin_count = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pin_count->Wrap( -1 );
	bSummarySizer->Add( m_pin_count, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSummarySizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextDuplicatePins = new wxStaticText( this, wxID_ANY, _("Duplicate pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDuplicatePins->Wrap( -1 );
	bSummarySizer->Add( m_staticTextDuplicatePins, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_duplicate_pins = new wxStaticText( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
	m_duplicate_pins->Wrap( -1 );
	bSummarySizer->Add( m_duplicate_pins, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	topSizer->Add( bSummarySizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( 800,400 ), 0 );

	// Grid
	m_grid->CreateGrid( 1, 11 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 60 );
	m_grid->SetColSize( 1, 110 );
	m_grid->SetColSize( 2, 140 );
	m_grid->SetColSize( 3, 84 );
	m_grid->SetColSize( 4, 84 );
	m_grid->SetColSize( 5, 84 );
	m_grid->SetColSize( 6, 84 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 110 );
	m_grid->SetColSize( 10, 110 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Number") );
	m_grid->SetColLabelValue( 1, _("Type") );
	m_grid->SetColLabelValue( 2, _("Shape") );
	m_grid->SetColLabelValue( 3, _("X Position") );
	m_grid->SetColLabelValue( 4, _("Y Position") );
	m_grid->SetColLabelValue( 5, _("Size X") );
	m_grid->SetColLabelValue( 6, _("Size Y") );
	m_grid->SetColLabelValue( 7, _("Drill X") );
	m_grid->SetColLabelValue( 8, _("Drill Y") );
	m_grid->SetColLabelValue( 9, _("Pad->Die Length") );
	m_grid->SetColLabelValue( 10, _("Pad->Die Delay") );
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	topSizer->Add( m_grid, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	topSizer->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );


	this->SetSizer( topSizer );
	this->Layout();
	topSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCellChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSelectCell ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSize ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_FP_EDIT_PAD_TABLE_BASE::~DIALOG_FP_EDIT_PAD_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCellChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSelectCell ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSize ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCancel ), NULL, this );

}
