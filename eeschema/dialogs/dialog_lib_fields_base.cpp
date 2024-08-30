///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_lib_fields_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_FIELDS_BASE::DIALOG_LIB_FIELDS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bEditSizer;
	bEditSizer = new wxBoxSizer( wxVERTICAL );

	m_splitterMainWindow = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitterMainWindow->SetMinimumPaneSize( 200 );

	m_leftPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_fieldsCtrl = new wxDataViewListCtrl( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldsCtrl->SetMinSize( wxSize( -1,250 ) );

	bLeftSizer->Add( m_fieldsCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bFieldsButtons;
	bFieldsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_addFieldButton, 0, wxBOTTOM|wxLEFT, 5 );

	m_renameFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_renameFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bFieldsButtons->Add( 15, 0, 0, wxEXPAND, 5 );

	m_removeFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_removeFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bLeftSizer->Add( bFieldsButtons, 0, wxEXPAND, 5 );


	m_leftPanel->SetSizer( bLeftSizer );
	m_leftPanel->Layout();
	bLeftSizer->Fit( m_leftPanel );
	m_rightPanel = new wxPanel( m_splitterMainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bControls;
	bControls = new wxBoxSizer( wxHORIZONTAL );

	m_filter = new wxSearchCtrl( m_rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_filter->ShowSearchButton( true );
	#endif
	m_filter->ShowCancelButton( true );
	m_filter->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_filter->SetMinSize( wxSize( 140,-1 ) );

	bControls->Add( m_filter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticline31 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline31, 0, wxEXPAND | wxALL, 3 );

	m_bRefresh = new STD_BITMAP_BUTTON( m_rightPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefresh->SetMinSize( wxSize( 30,30 ) );

	bControls->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bRightSizer->Add( bControls, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

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
	m_grid->SetColLabelSize( 24 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( 400,200 ) );

	bRightSizer->Add( m_grid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	m_rightPanel->SetSizer( bRightSizer );
	m_rightPanel->Layout();
	bRightSizer->Fit( m_rightPanel );
	m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, -1 );
	bEditSizer->Add( m_splitterMainWindow, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bMainSizer->Add( bEditSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bButtonsSizer->Add( 0, 0, 9, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_FIELDS_BASE::OnClose ) );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_LIB_FIELDS_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_LIB_FIELDS_BASE::OnFieldsCtrlSelectionChanged ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_FIELDS_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRemoveField ), NULL, this );
	m_filter->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_LIB_FIELDS_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnFilterText ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableColSize ), NULL, this );
	m_grid->Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnEditorShown ), NULL, this );
	m_grid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableSelectCell ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnOk ), NULL, this );
}

DIALOG_LIB_FIELDS_BASE::~DIALOG_LIB_FIELDS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_FIELDS_BASE::OnClose ) );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_LIB_FIELDS_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_LIB_FIELDS_BASE::OnFieldsCtrlSelectionChanged ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_FIELDS_BASE::OnSizeFieldList ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRemoveField ), NULL, this );
	m_filter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_LIB_FIELDS_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnFilterText ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableColSize ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnEditorShown ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_LIB_FIELDS_BASE::OnTableSelectCell ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_BASE::OnOk ), NULL, this );

}
