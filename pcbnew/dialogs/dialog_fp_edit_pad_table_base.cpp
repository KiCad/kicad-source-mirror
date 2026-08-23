///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_fp_edit_pad_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_EDIT_PAD_TABLE_BASE::DIALOG_FP_EDIT_PAD_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_contentSizer;
	m_contentSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* m_dataSizer;
	m_dataSizer = new wxBoxSizer( wxVERTICAL );

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


	m_dataSizer->Add( bSummarySizer, 0, wxTOP|wxBOTTOM|wxEXPAND, 5 );

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
	m_dataSizer->Add( m_grid, 1, wxEXPAND|wxALL, 5 );


	m_contentSizer->Add( m_dataSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* m_sideBarSizer;
	m_sideBarSizer = new wxBoxSizer( wxVERTICAL );


	m_sideBarSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* m_exportSizer;
	m_exportSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Export") ), wxVERTICAL );

	wxBoxSizer* m_exportBtnSizer;
	m_exportBtnSizer = new wxBoxSizer( wxHORIZONTAL );

	m_btnExportToFile = new wxButton( m_exportSizer->GetStaticBox(), wxID_ANY, _("To File..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_exportBtnSizer->Add( m_btnExportToFile, 1, wxALL, 5 );


	m_exportBtnSizer->Add( 5, 0, 0, wxEXPAND, 5 );

	m_btnExportToClipboard = new wxButton( m_exportSizer->GetStaticBox(), wxID_ANY, _("To Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exportBtnSizer->Add( m_btnExportToClipboard, 1, wxALL, 5 );


	m_exportSizer->Add( m_exportBtnSizer, 1, wxEXPAND, 5 );


	m_sideBarSizer->Add( m_exportSizer, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* m_importSizer;
	m_importSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Import") ), wxVERTICAL );

	m_rbReplaceExisting = new wxRadioButton( m_importSizer->GetStaticBox(), wxID_ANY, _("Replace all existing pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importSizer->Add( m_rbReplaceExisting, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rbAppendToExisiting = new wxRadioButton( m_importSizer->GetStaticBox(), wxID_ANY, _("Append to existing pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importSizer->Add( m_rbAppendToExisiting, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* m_importBtnSizer;
	m_importBtnSizer = new wxBoxSizer( wxHORIZONTAL );

	m_btnImportFromFile = new wxButton( m_importSizer->GetStaticBox(), wxID_ANY, _("From File..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_importBtnSizer->Add( m_btnImportFromFile, 1, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_importBtnSizer->Add( 5, 0, 0, wxEXPAND, 5 );

	m_btnImportFromClipboard = new wxButton( m_importSizer->GetStaticBox(), wxID_ANY, _("From Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importBtnSizer->Add( m_btnImportFromClipboard, 1, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_importSizer->Add( m_importBtnSizer, 1, wxEXPAND|wxBOTTOM, 5 );


	m_sideBarSizer->Add( m_importSizer, 0, wxALL|wxEXPAND, 5 );


	m_contentSizer->Add( m_sideBarSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	topSizer->Add( m_contentSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAdd = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_buttonAdd, 0, wxRIGHT|wxLEFT, 2 );

	m_buttonDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_buttonDelete, 0, wxRIGHT|wxLEFT, 25 );


	bSizer9->Add( bSizer91, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bSizer9->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer9->Add( m_sdbSizer, 0, wxALL, 5 );


	topSizer->Add( bSizer9, 0, wxEXPAND, 5 );


	this->SetSizer( topSizer );
	this->Layout();
	topSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCellChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSelectCell ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSize ), NULL, this );
	m_btnExportToFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnExportButtonClick ), NULL, this );
	m_btnExportToClipboard->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnExportButtonClick ), NULL, this );
	m_btnImportFromFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnImportButtonClick ), NULL, this );
	m_btnImportFromClipboard->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnImportButtonClick ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnAddRow ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_FP_EDIT_PAD_TABLE_BASE::~DIALOG_FP_EDIT_PAD_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCellChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSelectCell ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnSize ), NULL, this );
	m_btnExportToFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnExportButtonClick ), NULL, this );
	m_btnExportToClipboard->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnExportButtonClick ), NULL, this );
	m_btnImportFromFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnImportButtonClick ), NULL, this );
	m_btnImportFromClipboard->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnImportButtonClick ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnAddRow ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnDeleteRow ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_EDIT_PAD_TABLE_BASE::OnCancel ), NULL, this );

}
