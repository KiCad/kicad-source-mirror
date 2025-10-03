///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_lib_fields_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_FIELDS_TABLE_BASE::DIALOG_LIB_FIELDS_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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

	m_viewControlsGrid = new WX_GRID( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_viewControlsGrid->CreateGrid( 1, 4 );
	m_viewControlsGrid->EnableEditing( true );
	m_viewControlsGrid->EnableGridLines( false );
	m_viewControlsGrid->EnableDragGridSize( false );
	m_viewControlsGrid->SetMargins( 0, 0 );

	// Columns
	m_viewControlsGrid->SetColSize( 0, 60 );
	m_viewControlsGrid->SetColSize( 1, 60 );
	m_viewControlsGrid->SetColSize( 2, 46 );
	m_viewControlsGrid->SetColSize( 3, 56 );
	m_viewControlsGrid->EnableDragColMove( false );
	m_viewControlsGrid->EnableDragColSize( false );
	m_viewControlsGrid->SetColLabelValue( 0, _("Field") );
	m_viewControlsGrid->SetColLabelValue( 1, _("BOM Name") );
	m_viewControlsGrid->SetColLabelValue( 2, _("Include") );
	m_viewControlsGrid->SetColLabelValue( 3, _("Group By") );
	m_viewControlsGrid->SetColLabelSize( 24 );
	m_viewControlsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viewControlsGrid->EnableDragRowSize( false );
	m_viewControlsGrid->SetRowLabelSize( 0 );
	m_viewControlsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viewControlsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_viewControlsGrid->SetMinSize( wxSize( -1,250 ) );

	bLeftSizer->Add( m_viewControlsGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bFieldsButtons;
	bFieldsButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_addFieldButton, 0, wxBOTTOM|wxLEFT, 5 );

	m_renameFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_renameFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bFieldsButtons->Add( 15, 0, 0, wxEXPAND, 5 );

	m_removeFieldButton = new STD_BITMAP_BUTTON( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bFieldsButtons->Add( m_removeFieldButton, 0, wxBOTTOM|wxLEFT, 5 );


	bLeftSizer->Add( bFieldsButtons, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


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

	bControls->Add( m_filter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticline31 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline31, 0, wxEXPAND | wxALL, 3 );

	wxString m_choiceScopeChoices[] = { _("Whole Library"), _("Related Symbols Only") };
	int m_choiceScopeNChoices = sizeof( m_choiceScopeChoices ) / sizeof( wxString );
	m_choiceScope = new wxChoice( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceScopeNChoices, m_choiceScopeChoices, 0 );
	m_choiceScope->SetSelection( 0 );
	bControls->Add( m_choiceScope, 0, wxALL, 5 );

	m_staticline311 = new wxStaticLine( m_rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bControls->Add( m_staticline311, 0, wxEXPAND | wxALL, 5 );

	m_groupSymbolsBox = new wxCheckBox( m_rightPanel, wxID_ANY, _("Group symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupSymbolsBox->SetValue(true);
	m_groupSymbolsBox->SetToolTip( _("Group symbols together based on common properties") );

	bControls->Add( m_groupSymbolsBox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bRefresh = new STD_BITMAP_BUTTON( m_rightPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bRefresh->SetMinSize( wxSize( 30,30 ) );

	bControls->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bRightSizer->Add( bControls, 0, wxEXPAND|wxALL, 5 );

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

	bRightSizer->Add( m_grid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sidebarButton = new STD_BITMAP_BUTTON( m_rightPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonsSizer->Add( m_sidebarButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	bButtonsSizer->Add( 0, 0, 9, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( m_rightPanel, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( m_rightPanel, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( m_rightPanel, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bRightSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	m_rightPanel->SetSizer( bRightSizer );
	m_rightPanel->Layout();
	bRightSizer->Fit( m_rightPanel );
	m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, -1 );
	bEditSizer->Add( m_splitterMainWindow, 1, wxEXPAND, 5 );


	bMainSizer->Add( bEditSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnClose ) );
	m_viewControlsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnViewControlsCellChanged ), NULL, this );
	m_viewControlsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnSizeViewControlsGrid ), NULL, this );
	m_addFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_filter->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_choiceScope->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnScope ), NULL, this );
	m_groupSymbolsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Connect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_grid->Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnEditorShown ), NULL, this );
	m_grid->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableSelectCell ), NULL, this );
	m_sidebarButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnSidebarToggle ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnOk ), NULL, this );
}

DIALOG_LIB_FIELDS_TABLE_BASE::~DIALOG_LIB_FIELDS_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnClose ) );
	m_viewControlsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnViewControlsCellChanged ), NULL, this );
	m_viewControlsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnSizeViewControlsGrid ), NULL, this );
	m_addFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnAddField ), NULL, this );
	m_renameFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRenameField ), NULL, this );
	m_removeFieldButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRemoveField ), NULL, this );
	m_filter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnFilterMouseMoved ), NULL, this );
	m_filter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnFilterText ), NULL, this );
	m_choiceScope->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnScope ), NULL, this );
	m_groupSymbolsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnGroupSymbolsToggled ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnRegroupSymbols ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableItemContextMenu ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableCmdCellClick ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableColSize ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnEditorShown ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnTableSelectCell ), NULL, this );
	m_sidebarButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnSidebarToggle ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnCancel ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_FIELDS_TABLE_BASE::OnOk ), NULL, this );

}
