///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 22 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_components_libid_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_COMPONENTS_LIBID_BASE::DIALOG_EDIT_COMPONENTS_LIBID_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_panelGrid = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerGrid;
	bSizerGrid = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new wxGrid( m_panelGrid, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid->CreateGrid( 5, 3 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->SetColSize( 0, 300 );
	m_grid->SetColSize( 1, 200 );
	m_grid->SetColSize( 2, 200 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 30 );
	m_grid->SetColLabelValue( 0, _("Symbols") );
	m_grid->SetColLabelValue( 1, _("Current Symbol") );
	m_grid->SetColLabelValue( 2, _("New Symbol") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->AutoSizeRows();
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 30 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( 700,-1 ) );
	
	bSizerGrid->Add( m_grid, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panelGrid->SetSizer( bSizerGrid );
	m_panelGrid->Layout();
	bSizerGrid->Fit( m_panelGrid );
	bSizerMain->Add( m_panelGrid, 1, wxEXPAND | wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerMsgWarning;
	bSizerMsgWarning = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextWarning = new wxStaticText( this, wxID_ANY, _("Warning: Changes made from this dialog cannot be undone, after closing it."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarning->Wrap( -1 );
	m_staticTextWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerMsgWarning->Add( m_staticTextWarning, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerMsgWarning, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerButtons->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_buttonUndo = new wxButton( this, wxID_ANY, _("Undo Changes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonUndo, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonBrowseLibs = new wxButton( this, wxID_ANY, _("Browse Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_buttonBrowseLibs, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonOrphanItems = new wxButton( this, wxID_ANY, _("Map Orphans"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOrphanItems->SetToolTip( _("If  some components are orphan (the linked symbol is found nowhere),\ntry to find a candidate having the same name in one of loaded symbol libraries") );
	
	bSizerButtons->Add( m_buttonOrphanItems, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerButtons, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Connect( wxEVT_GRID_LABEL_LEFT_DCLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onApplyButton ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCancel ), NULL, this );
	m_buttonUndo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onUndoChangesButton ), NULL, this );
	m_buttonUndo->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::updateUIChangesButton ), NULL, this );
	m_buttonBrowseLibs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onButtonBrowseLibraries ), NULL, this );
	m_buttonBrowseLibs->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::updateUIBrowseButton ), NULL, this );
	m_buttonOrphanItems->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onClickOrphansButton ), NULL, this );
}

DIALOG_EDIT_COMPONENTS_LIBID_BASE::~DIALOG_EDIT_COMPONENTS_LIBID_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_LABEL_LEFT_DCLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_grid->Disconnect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCellBrowseLib ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onApplyButton ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onCancel ), NULL, this );
	m_buttonUndo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onUndoChangesButton ), NULL, this );
	m_buttonUndo->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::updateUIChangesButton ), NULL, this );
	m_buttonBrowseLibs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onButtonBrowseLibraries ), NULL, this );
	m_buttonBrowseLibs->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::updateUIBrowseButton ), NULL, this );
	m_buttonOrphanItems->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENTS_LIBID_BASE::onClickOrphansButton ), NULL, this );
	
}
