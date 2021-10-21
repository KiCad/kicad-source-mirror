///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_symbol_fields_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SYMBOL_FIELDS_TABLE_BASE::DIALOG_SYMBOL_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_splitterMainWindow = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitterMainWindow->SetMinimumPaneSize( 200 );

	m_leftPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bGroupSizer;
	bGroupSizer = new wxBoxSizer( wxHORIZONTAL );

	m_groupSymbolsBox = new wxCheckBox( m_leftPanel, OPT_GROUP_COMPONENTS, _("Group symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupSymbolsBox->SetValue(true);
	m_groupSymbolsBox->SetToolTip( _("Group symbols together based on common properties") );

	bGroupSizer->Add( m_groupSymbolsBox, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bGroupSizer->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_bRefresh = new wxBitmapButton( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefresh->SetMinSize( wxSize( 30,30 ) );

	bGroupSizer->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bLeftSizer->Add( bGroupSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 2 );

	m_fieldsCtrl = new wxDataViewListCtrl( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldsCtrl->SetMinSize( wxSize( -1,220 ) );

	bLeftSizer->Add( m_fieldsCtrl, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_addFieldButton = new wxButton( m_leftPanel, wxID_ANY, _("Add Field..."), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_addFieldButton, 0, wxALL|wxEXPAND, 5 );


	m_leftPanel->SetSizer( bLeftSizer );
	m_leftPanel->Layout();
	bLeftSizer->Fit( m_leftPanel );
	m_rightPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 5, 5 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->EnableDragColMove( true );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 20 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 400,240 ) );

	bRightSizer->Add( m_grid, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	m_rightPanel->SetSizer( bRightSizer );
	m_rightPanel->Layout();
	bRightSizer->Fit( m_rightPanel );
	m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, -1 );
	bMainSizer->Add( m_splitterMainWindow, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonApply = new wxButton( this, wxID_ANY, _("Apply, Save Schematic && Continue"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonApply, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_groupSymbolsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_buttonApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
}

DIALOG_SYMBOL_FIELDS_TABLE_BASE::~DIALOG_SYMBOL_FIELDS_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnClose ) );
	m_groupSymbolsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_buttonApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnSaveAndContinue ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SYMBOL_FIELDS_TABLE_BASE::OnCancel ), NULL, this );

}
