///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_configure_paths_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CONFIGURE_PATHS_BASE::DIALOG_CONFIGURE_PATHS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbEnvVars;
	sbEnvVars = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Environment Variables") ), wxVERTICAL );
	
	m_EnvVars = new WX_GRID( sbEnvVars->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_EnvVars->CreateGrid( 1, 2 );
	m_EnvVars->EnableEditing( true );
	m_EnvVars->EnableGridLines( true );
	m_EnvVars->EnableDragGridSize( false );
	m_EnvVars->SetMargins( 0, 0 );
	
	// Columns
	m_EnvVars->SetColSize( 0, 150 );
	m_EnvVars->SetColSize( 1, 454 );
	m_EnvVars->EnableDragColMove( false );
	m_EnvVars->EnableDragColSize( true );
	m_EnvVars->SetColLabelSize( 22 );
	m_EnvVars->SetColLabelValue( 0, _("Name") );
	m_EnvVars->SetColLabelValue( 1, _("Path") );
	m_EnvVars->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_EnvVars->EnableDragRowSize( true );
	m_EnvVars->SetRowLabelSize( 0 );
	m_EnvVars->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_EnvVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_EnvVars->SetMinSize( wxSize( 604,170 ) );
	
	sbEnvVars->Add( m_EnvVars, 1, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerEnvVarBtns;
	bSizerEnvVarBtns = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnAddEnvVar = new wxBitmapButton( sbEnvVars->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnAddEnvVar->SetMinSize( wxSize( 30,29 ) );
	
	bSizerEnvVarBtns->Add( m_btnAddEnvVar, 0, wxRIGHT, 5 );
	
	
	bSizerEnvVarBtns->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_btnDeleteEnvVar = new wxBitmapButton( sbEnvVars->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnDeleteEnvVar->SetMinSize( wxSize( 30,29 ) );
	
	bSizerEnvVarBtns->Add( m_btnDeleteEnvVar, 0, wxRIGHT|wxLEFT, 5 );
	
	
	sbEnvVars->Add( bSizerEnvVarBtns, 0, wxEXPAND, 5 );
	
	
	bSizerMain->Add( sbEnvVars, 1, wxEXPAND|wxALL, 5 );
	
	m_sb3DSearchPaths = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("3D Search Paths") ), wxVERTICAL );
	
	m_SearchPaths = new WX_GRID( m_sb3DSearchPaths->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_SearchPaths->CreateGrid( 1, 3 );
	m_SearchPaths->EnableEditing( true );
	m_SearchPaths->EnableGridLines( true );
	m_SearchPaths->EnableDragGridSize( false );
	m_SearchPaths->SetMargins( 0, 0 );
	
	// Columns
	m_SearchPaths->SetColSize( 0, 150 );
	m_SearchPaths->SetColSize( 1, 300 );
	m_SearchPaths->SetColSize( 2, 154 );
	m_SearchPaths->EnableDragColMove( false );
	m_SearchPaths->EnableDragColSize( true );
	m_SearchPaths->SetColLabelSize( 22 );
	m_SearchPaths->SetColLabelValue( 0, _("Alias") );
	m_SearchPaths->SetColLabelValue( 1, _("Path") );
	m_SearchPaths->SetColLabelValue( 2, _("Description") );
	m_SearchPaths->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_SearchPaths->AutoSizeRows();
	m_SearchPaths->EnableDragRowSize( false );
	m_SearchPaths->SetRowLabelSize( 0 );
	m_SearchPaths->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_SearchPaths->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_SearchPaths->SetMinSize( wxSize( 604,150 ) );
	
	m_sb3DSearchPaths->Add( m_SearchPaths, 1, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerSearchPathBtns;
	bSizerSearchPathBtns = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnAddSearchPath = new wxBitmapButton( m_sb3DSearchPaths->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnAddSearchPath->SetMinSize( wxSize( 30,29 ) );
	
	bSizerSearchPathBtns->Add( m_btnAddSearchPath, 0, wxRIGHT, 5 );
	
	m_btnMoveUp = new wxBitmapButton( m_sb3DSearchPaths->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnMoveUp->SetMinSize( wxSize( 30,29 ) );
	
	bSizerSearchPathBtns->Add( m_btnMoveUp, 0, wxRIGHT, 5 );
	
	m_btnMoveDown = new wxBitmapButton( m_sb3DSearchPaths->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnMoveDown->SetMinSize( wxSize( 30,29 ) );
	
	bSizerSearchPathBtns->Add( m_btnMoveDown, 0, wxRIGHT, 5 );
	
	
	bSizerSearchPathBtns->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_btnDeleteSearchPath = new wxBitmapButton( m_sb3DSearchPaths->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnDeleteSearchPath->SetMinSize( wxSize( 30,29 ) );
	
	bSizerSearchPathBtns->Add( m_btnDeleteSearchPath, 0, wxRIGHT|wxLEFT, 5 );
	
	
	m_sb3DSearchPaths->Add( bSizerSearchPathBtns, 0, wxEXPAND, 5 );
	
	
	bSizerMain->Add( m_sb3DSearchPaths, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizerHelp = new wxButton( this, wxID_HELP );
	m_sdbSizer->AddButton( m_sdbSizerHelp );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnUpdateUI ) );
	m_EnvVars->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellChange ), NULL, this );
	m_EnvVars->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridSize ), NULL, this );
	m_btnAddEnvVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddEnvVar ), NULL, this );
	m_btnDeleteEnvVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnRemoveEnvVar ), NULL, this );
	m_SearchPaths->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellChange ), NULL, this );
	m_SearchPaths->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellRightClick ), NULL, this );
	m_btnAddSearchPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddSearchPath ), NULL, this );
	m_btnMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnSearchPathMoveUp ), NULL, this );
	m_btnMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnSearchPathMoveDown ), NULL, this );
	m_btnDeleteSearchPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnDeleteSearchPath ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnHelp ), NULL, this );
}

DIALOG_CONFIGURE_PATHS_BASE::~DIALOG_CONFIGURE_PATHS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnUpdateUI ) );
	m_EnvVars->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellChange ), NULL, this );
	m_EnvVars->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridSize ), NULL, this );
	m_btnAddEnvVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddEnvVar ), NULL, this );
	m_btnDeleteEnvVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnRemoveEnvVar ), NULL, this );
	m_SearchPaths->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellChange ), NULL, this );
	m_SearchPaths->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnGridCellRightClick ), NULL, this );
	m_btnAddSearchPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddSearchPath ), NULL, this );
	m_btnMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnSearchPathMoveUp ), NULL, this );
	m_btnMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnSearchPathMoveDown ), NULL, this );
	m_btnDeleteSearchPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnDeleteSearchPath ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnHelp ), NULL, this );
	
}
