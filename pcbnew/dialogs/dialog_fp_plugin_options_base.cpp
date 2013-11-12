///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 30 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_fp_plugin_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_PLUGIN_OPTIONS_BASE::DIALOG_FP_PLUGIN_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_horizontal_sizer;
	m_horizontal_sizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* m_grid_sizer;
	m_grid_sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Plugin Options:") ), wxVERTICAL );
	
	m_grid_sizer->SetMinSize( wxSize( -1,300 ) ); 
	m_grid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	
	// Grid
	m_grid->CreateGrid( 1, 2 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 30 );
	m_grid->SetColLabelValue( 0, _("Option") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 40 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid_sizer->Add( m_grid, 1, wxEXPAND|wxTOP, 5 );
	
	wxBoxSizer* m_button_sizer;
	m_button_sizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_add_row = new wxButton( this, wxID_ANY, _("Append"), wxDefaultPosition, wxDefaultSize, 0 );
	m_add_row->SetToolTip( _("Append a blank row") );
	
	m_button_sizer->Add( m_add_row, 0, wxALL, 5 );
	
	m_delete_row = new wxButton( this, wxID_ANY, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	m_delete_row->SetToolTip( _("Delete the selected row") );
	
	m_button_sizer->Add( m_delete_row, 0, wxALL, 5 );
	
	m_move_up = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_move_up->SetToolTip( _("Move the selected row up one position") );
	
	m_button_sizer->Add( m_move_up, 0, wxALL, 5 );
	
	m_move_down = new wxButton( this, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	m_move_down->SetToolTip( _("Move the selected row down one position") );
	
	m_button_sizer->Add( m_move_down, 0, wxALL, 5 );
	
	
	m_grid_sizer->Add( m_button_sizer, 0, wxALIGN_CENTER, 5 );
	
	
	m_horizontal_sizer->Add( m_grid_sizer, 3, wxEXPAND, 5 );
	
	wxGridSizer* m_choose_size;
	m_choose_size = new wxGridSizer( 1, 1, 0, 0 );
	
	m_button1 = new wxButton( this, wxID_ANY, _("<<"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button1->SetMaxSize( wxSize( 50,-1 ) );
	
	m_choose_size->Add( m_button1, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	
	m_horizontal_sizer->Add( m_choose_size, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_options_sizer;
	m_options_sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Option Choices:") ), wxHORIZONTAL );
	
	m_options_sizer->SetMinSize( wxSize( 200,-1 ) ); 
	m_option_choices = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	
	// Grid
	m_option_choices->CreateGrid( 0, 2 );
	m_option_choices->EnableEditing( false );
	m_option_choices->EnableGridLines( true );
	m_option_choices->EnableDragGridSize( false );
	m_option_choices->SetMargins( 0, 0 );
	
	// Columns
	m_option_choices->AutoSizeColumns();
	m_option_choices->EnableDragColMove( false );
	m_option_choices->EnableDragColSize( true );
	m_option_choices->SetColLabelSize( 30 );
	m_option_choices->SetColLabelValue( 0, _("Option") );
	m_option_choices->SetColLabelValue( 1, _("Description") );
	m_option_choices->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_option_choices->AutoSizeRows();
	m_option_choices->EnableDragRowSize( true );
	m_option_choices->SetRowLabelSize( 0 );
	m_option_choices->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_option_choices->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_options_sizer->Add( m_option_choices, 1, wxEXPAND|wxTOP, 5 );
	
	
	m_horizontal_sizer->Add( m_options_sizer, 4, wxEXPAND, 5 );
	
	
	bSizer4->Add( m_horizontal_sizer, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer4->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer4 );
	this->Layout();
	bSizer4->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ) );
	m_add_row->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_row->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_button1->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
}

DIALOG_FP_PLUGIN_OPTIONS_BASE::~DIALOG_FP_PLUGIN_OPTIONS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ) );
	m_add_row->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_row->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_button1->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
	
}
