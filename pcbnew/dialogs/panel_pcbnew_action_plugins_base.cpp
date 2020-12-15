///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_pcbnew_action_plugins_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_ACTION_PLUGINS_BASE::PANEL_PCBNEW_ACTION_PLUGINS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bGridSizer;
	bGridSizer = new wxBoxSizer( wxVERTICAL );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 3, 6 );
	m_grid->EnableEditing( false );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->AutoSizeColumns();
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, _("Icon") );
	m_grid->SetColLabelValue( 1, _("Show button") );
	m_grid->SetColLabelValue( 2, _("Name") );
	m_grid->SetColLabelValue( 3, _("Category") );
	m_grid->SetColLabelValue( 4, _("Description") );
	m_grid->SetColLabelValue( 5, _("Path") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bGridSizer->Add( m_grid, 1, wxALL|wxEXPAND, 5 );


	bPanelSizer->Add( bGridSizer, 1, wxALIGN_LEFT|wxEXPAND|wxLEFT, 0 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_moveUpButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_moveUpButton->SetToolTip( _("Move Up") );

	bButtonsSizer->Add( m_moveUpButton, 0, wxLEFT|wxRIGHT, 5 );

	m_moveDownButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_moveDownButton->SetToolTip( _("Move Down") );

	bButtonsSizer->Add( m_moveDownButton, 0, wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_openDirectoryButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_openDirectoryButton->SetToolTip( _("Open Plugin Directory") );

	bButtonsSizer->Add( m_openDirectoryButton, 0, wxLEFT|wxRIGHT, 5 );

	m_reloadButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_reloadButton->SetToolTip( _("Reload Plugins") );

	bButtonsSizer->Add( m_reloadButton, 0, wxRIGHT, 5 );

	m_showErrorsButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_showErrorsButton->SetToolTip( _("Show Plugin Errors") );

	bButtonsSizer->Add( m_showErrorsButton, 0, wxRIGHT, 5 );


	bPanelSizer->Add( bButtonsSizer, 0, wxEXPAND, 0 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnGridCellClick ), NULL, this );
	m_moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveUpButtonClick ), NULL, this );
	m_moveDownButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveDownButtonClick ), NULL, this );
	m_openDirectoryButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnOpenDirectoryButtonClick ), NULL, this );
	m_reloadButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnReloadButtonClick ), NULL, this );
	m_showErrorsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnShowErrorsButtonClick ), NULL, this );
}

PANEL_PCBNEW_ACTION_PLUGINS_BASE::~PANEL_PCBNEW_ACTION_PLUGINS_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnGridCellClick ), NULL, this );
	m_moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveUpButtonClick ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnMoveDownButtonClick ), NULL, this );
	m_openDirectoryButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnOpenDirectoryButtonClick ), NULL, this );
	m_reloadButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnReloadButtonClick ), NULL, this );
	m_showErrorsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_ACTION_PLUGINS_BASE::OnShowErrorsButtonClick ), NULL, this );

}
