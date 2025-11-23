///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_design_block_lib_table_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::PANEL_DESIGN_BLOCK_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_CLOSE_ON_ALL_TABS|wxAUI_NB_DEFAULT_STYLE );

	bMainSizer->Add( m_notebook, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_append_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_append_button->SetToolTip( _("Add empty row to table") );

	bButtonsSizer->Add( m_append_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_browseButton = new SPLIT_BUTTON( this, wxID_ANY, _( "Add Existing" ), wxDefaultPosition );
	m_browseButton->SetToolTip( _("Add Existing") );

	bButtonsSizer->Add( m_browseButton, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_move_up_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_move_up_button->SetToolTip( _("Move up") );

	bButtonsSizer->Add( m_move_up_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_move_down_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_move_down_button->SetToolTip( _("Move down") );

	bButtonsSizer->Add( m_move_down_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_delete_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_delete_button->SetToolTip( _("Remove library from table") );

	bButtonsSizer->Add( m_delete_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_migrate_libs_button = new wxButton( this, wxID_ANY, _("Migrate Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_migrate_libs_button, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 8 );

	wxStaticText* stPathsLabel;
	stPathsLabel = new wxStaticText( this, wxID_ANY, _("Available path substitutions:"), wxDefaultPosition, wxDefaultSize, 0 );
	stPathsLabel->Wrap( -1 );
	bMainSizer->Add( stPathsLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 8 );


	bMainSizer->Add( 0, 2, 0, wxEXPAND, 5 );

	m_path_subs_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_path_subs_grid->CreateGrid( 1, 2 );
	m_path_subs_grid->EnableEditing( false );
	m_path_subs_grid->EnableGridLines( true );
	m_path_subs_grid->EnableDragGridSize( false );
	m_path_subs_grid->SetMargins( 0, 0 );

	// Columns
	m_path_subs_grid->SetColSize( 0, 150 );
	m_path_subs_grid->SetColSize( 1, 500 );
	m_path_subs_grid->AutoSizeColumns();
	m_path_subs_grid->EnableDragColMove( false );
	m_path_subs_grid->EnableDragColSize( true );
	m_path_subs_grid->SetColLabelSize( 0 );
	m_path_subs_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_path_subs_grid->EnableDragRowSize( true );
	m_path_subs_grid->SetRowLabelSize( 0 );
	m_path_subs_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_path_subs_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_path_subs_grid->SetToolTip( _("This is a read-only table which shows pertinent environment variables.") );

	bMainSizer->Add( m_path_subs_grid, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_append_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::appendRowHandler ), NULL, this );
	m_move_up_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::moveUpHandler ), NULL, this );
	m_move_down_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::moveDownHandler ), NULL, this );
	m_delete_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::deleteRowHandler ), NULL, this );
	m_migrate_libs_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::onMigrateLibraries ), NULL, this );
	m_path_subs_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::onSizeGrid ), NULL, this );
}

PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::~PANEL_DESIGN_BLOCK_LIB_TABLE_BASE()
{
	// Disconnect Events
	m_append_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::appendRowHandler ), NULL, this );
	m_move_up_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::moveUpHandler ), NULL, this );
	m_move_down_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::moveDownHandler ), NULL, this );
	m_delete_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::deleteRowHandler ), NULL, this );
	m_migrate_libs_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::onMigrateLibraries ), NULL, this );
	m_path_subs_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_DESIGN_BLOCK_LIB_TABLE_BASE::onSizeGrid ), NULL, this );

}
