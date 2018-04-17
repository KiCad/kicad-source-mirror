///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_fields_editor_global_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FIELDS_EDITOR_GLOBAL_BASE::DIALOG_FIELDS_EDITOR_GLOBAL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bHorizontalSizer;
	bHorizontalSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );
	
	m_panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 120 );
	
	m_leftPanel = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );
	
	m_groupComponentsBox = new wxCheckBox( m_leftPanel, OPT_GROUP_COMPONENTS, _("Group symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_groupComponentsBox->SetValue(true); 
	m_groupComponentsBox->SetToolTip( _("Group components together based on common properties") );
	
	bSizer8->Add( m_groupComponentsBox, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bRefresh = new wxBitmapButton( m_leftPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bRefresh->SetMinSize( wxSize( 30,28 ) );
	
	bSizer8->Add( m_bRefresh, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer6->Add( bSizer8, 0, wxALL|wxBOTTOM|wxEXPAND|wxTOP, 3 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_fieldsCtrl = new wxDataViewListCtrl( m_leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldsCtrl->SetMinSize( wxSize( -1,250 ) );
	
	bSizer9->Add( m_fieldsCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer9, 5, wxEXPAND, 5 );
	
	
	m_leftPanel->SetSizer( bSizer6 );
	m_leftPanel->Layout();
	bSizer6->Fit( m_leftPanel );
	m_panel4 = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new wxGrid( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid->CreateGrid( 5, 5 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 20 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer5->Add( m_grid, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer5 );
	m_panel4->Layout();
	bSizer5->Fit( m_panel4 );
	m_splitter1->SplitVertically( m_leftPanel, m_panel4, 200 );
	bSizer7->Add( m_splitter1, 1, wxEXPAND, 5 );
	
	
	m_panel->SetSizer( bSizer7 );
	m_panel->Layout();
	bSizer7->Fit( m_panel );
	bSizer61->Add( m_panel, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer61->Add( m_sdbSizer1, 0, wxEXPAND, 5 );
	
	
	bHorizontalSizer->Add( bSizer61, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bHorizontalSizer );
	this->Layout();
	bHorizontalSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_groupComponentsBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnGroupComponentsToggled ), NULL, this );
	m_bRefresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnRegroupComponents ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnSizeFieldList ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnTableItemContextMenu ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnCancel ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnOK ), NULL, this );
}

DIALOG_FIELDS_EDITOR_GLOBAL_BASE::~DIALOG_FIELDS_EDITOR_GLOBAL_BASE()
{
	// Disconnect Events
	m_groupComponentsBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnGroupComponentsToggled ), NULL, this );
	m_bRefresh->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnRegroupComponents ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnColumnItemToggled ), NULL, this );
	m_fieldsCtrl->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnSizeFieldList ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnTableValueChanged ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnTableItemContextMenu ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnCancel ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FIELDS_EDITOR_GLOBAL_BASE::OnOK ), NULL, this );
	
}
