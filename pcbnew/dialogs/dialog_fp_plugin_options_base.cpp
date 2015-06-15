///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
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
	
	
	m_horizontal_sizer->Add( m_choose_size, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_options_sizer;
	m_options_sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Option Choices:") ), wxVERTICAL );
	
	m_options_sizer->SetMinSize( wxSize( 200,-1 ) ); 
	m_listbox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_SINGLE ); 
	m_listbox->SetToolTip( _("Options supported by current plugin") );
	
	m_options_sizer->Add( m_listbox, 2, wxALL|wxEXPAND, 5 );
	
	m_append_choice_button = new wxButton( this, wxID_ANY, _("<< Append Selected Option"), wxDefaultPosition, wxDefaultSize, 0 );
	m_options_sizer->Add( m_append_choice_button, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Option Specific Help:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_options_sizer->Add( m_staticText1, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_html = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxVSCROLL );
	m_html->SetMinSize( wxSize( 300,300 ) );
	
	m_options_sizer->Add( m_html, 3, wxALL|wxEXPAND, 5 );
	
	
	m_horizontal_sizer->Add( m_options_sizer, 2, wxEXPAND, 5 );
	
	
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
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelCaptionButtonClick ) );
	m_add_row->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_row->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_listbox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onListBoxItemSelected ), NULL, this );
	m_listbox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onListBoxItemDoubleClicked ), NULL, this );
	m_append_choice_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
}

DIALOG_FP_PLUGIN_OPTIONS_BASE::~DIALOG_FP_PLUGIN_OPTIONS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelCaptionButtonClick ) );
	m_add_row->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_row->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_listbox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onListBoxItemSelected ), NULL, this );
	m_listbox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onListBoxItemDoubleClicked ), NULL, this );
	m_append_choice_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
	
}
