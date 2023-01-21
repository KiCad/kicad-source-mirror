///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_sym_lib_table_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYM_LIB_TABLE_BASE::PANEL_SYM_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_global_panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_global_sizer;
	m_global_sizer = new wxBoxSizer( wxVERTICAL );

	m_global_grid = new WX_GRID( m_global_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_global_grid->CreateGrid( 1, 7 );
	m_global_grid->EnableEditing( true );
	m_global_grid->EnableGridLines( true );
	m_global_grid->EnableDragGridSize( false );
	m_global_grid->SetMargins( 0, 0 );

	// Columns
	m_global_grid->SetColSize( 0, 48 );
	m_global_grid->SetColSize( 1, 48 );
	m_global_grid->SetColSize( 2, 100 );
	m_global_grid->SetColSize( 3, 240 );
	m_global_grid->SetColSize( 4, 100 );
	m_global_grid->SetColSize( 5, 80 );
	m_global_grid->SetColSize( 6, 240 );
	m_global_grid->EnableDragColMove( false );
	m_global_grid->EnableDragColSize( true );
	m_global_grid->SetColLabelValue( 0, _("Active") );
	m_global_grid->SetColLabelValue( 1, _("Visible") );
	m_global_grid->SetColLabelValue( 2, _("Nickname") );
	m_global_grid->SetColLabelValue( 3, _("Library Path") );
	m_global_grid->SetColLabelValue( 4, _("Library Format") );
	m_global_grid->SetColLabelValue( 5, _("Options") );
	m_global_grid->SetColLabelValue( 6, _("Description") );
	m_global_grid->SetColLabelSize( 22 );
	m_global_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_global_grid->AutoSizeRows();
	m_global_grid->EnableDragRowSize( false );
	m_global_grid->SetRowLabelSize( 0 );
	m_global_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_global_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_global_sizer->Add( m_global_grid, 5, wxALL|wxEXPAND, 5 );


	m_global_panel->SetSizer( m_global_sizer );
	m_global_panel->Layout();
	m_global_sizer->Fit( m_global_panel );
	m_notebook->AddPage( m_global_panel, _("Global Libraries"), true );
	m_project_panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_project_sizer;
	m_project_sizer = new wxBoxSizer( wxVERTICAL );

	m_project_grid = new WX_GRID( m_project_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_project_grid->CreateGrid( 1, 7 );
	m_project_grid->EnableEditing( true );
	m_project_grid->EnableGridLines( true );
	m_project_grid->EnableDragGridSize( false );
	m_project_grid->SetMargins( 0, 0 );

	// Columns
	m_project_grid->SetColSize( 0, 48 );
	m_project_grid->SetColSize( 1, 48 );
	m_project_grid->SetColSize( 2, 100 );
	m_project_grid->SetColSize( 3, 240 );
	m_project_grid->SetColSize( 4, 100 );
	m_project_grid->SetColSize( 5, 80 );
	m_project_grid->SetColSize( 6, 240 );
	m_project_grid->EnableDragColMove( false );
	m_project_grid->EnableDragColSize( true );
	m_project_grid->SetColLabelValue( 0, _("Active") );
	m_project_grid->SetColLabelValue( 1, _("Visible") );
	m_project_grid->SetColLabelValue( 2, _("Nickname") );
	m_project_grid->SetColLabelValue( 3, _("Library Path") );
	m_project_grid->SetColLabelValue( 4, _("Library Format") );
	m_project_grid->SetColLabelValue( 5, _("Options") );
	m_project_grid->SetColLabelValue( 6, _("Description") );
	m_project_grid->SetColLabelSize( 22 );
	m_project_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_project_grid->EnableDragRowSize( false );
	m_project_grid->SetRowLabelSize( 0 );
	m_project_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_project_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_project_grid->SetMinSize( wxSize( 600,-1 ) );

	m_project_sizer->Add( m_project_grid, 2, wxALL|wxEXPAND, 5 );


	m_project_panel->SetSizer( m_project_sizer );
	m_project_panel->Layout();
	m_project_sizer->Fit( m_project_panel );
	m_notebook->AddPage( m_project_panel, _("Project Specific Libraries"), false );

	bMainSizer->Add( m_notebook, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_append_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_append_button->SetToolTip( _("Add empty row to table") );

	bButtonsSizer->Add( m_append_button, 0, wxALL, 5 );

	m_browse_button = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_browse_button->SetToolTip( _("Add existing library to table") );

	bButtonsSizer->Add( m_browse_button, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

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


	bButtonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_convertLegacy = new wxButton( this, wxID_ANY, _("Migrate Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_convertLegacy, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND|wxALL, 3 );


	bMainSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	wxStaticText* stPathsLabel;
	stPathsLabel = new wxStaticText( this, wxID_ANY, _("Available path substitutions:"), wxDefaultPosition, wxDefaultSize, 0 );
	stPathsLabel->Wrap( -1 );
	bMainSizer->Add( stPathsLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );


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
	m_path_subs_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_path_subs_grid->SetToolTip( _("This is a read-only table which shows pertinent environment variables.") );

	bMainSizer->Add( m_path_subs_grid, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SYM_LIB_TABLE_BASE::OnUpdateUI ) );
	m_append_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::appendRowHandler ), NULL, this );
	m_browse_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::browseLibrariesHandler ), NULL, this );
	m_move_up_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::moveUpHandler ), NULL, this );
	m_move_down_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::moveDownHandler ), NULL, this );
	m_delete_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::deleteRowHandler ), NULL, this );
	m_convertLegacy->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::onConvertLegacyLibraries ), NULL, this );
	m_path_subs_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SYM_LIB_TABLE_BASE::onSizeGrid ), NULL, this );
}

PANEL_SYM_LIB_TABLE_BASE::~PANEL_SYM_LIB_TABLE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SYM_LIB_TABLE_BASE::OnUpdateUI ) );
	m_append_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::appendRowHandler ), NULL, this );
	m_browse_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::browseLibrariesHandler ), NULL, this );
	m_move_up_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::moveUpHandler ), NULL, this );
	m_move_down_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::moveDownHandler ), NULL, this );
	m_delete_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::deleteRowHandler ), NULL, this );
	m_convertLegacy->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYM_LIB_TABLE_BASE::onConvertLegacyLibraries ), NULL, this );
	m_path_subs_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SYM_LIB_TABLE_BASE::onSizeGrid ), NULL, this );

}
