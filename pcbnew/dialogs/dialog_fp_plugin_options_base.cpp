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
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Plugin Options:") ), wxVERTICAL );
	
	m_grid1 = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid1->CreateGrid( 5, 2 );
	m_grid1->EnableEditing( true );
	m_grid1->EnableGridLines( true );
	m_grid1->EnableDragGridSize( false );
	m_grid1->SetMargins( 0, 0 );
	
	// Columns
	m_grid1->EnableDragColMove( false );
	m_grid1->EnableDragColSize( true );
	m_grid1->SetColLabelSize( 30 );
	m_grid1->SetColLabelValue( 0, _("Option") );
	m_grid1->SetColLabelValue( 1, _("Value") );
	m_grid1->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid1->EnableDragRowSize( false );
	m_grid1->SetRowLabelSize( 80 );
	m_grid1->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid1->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer1->Add( m_grid1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	m_add_row = new wxButton( this, wxID_ANY, _("Add Row"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_add_row, 0, wxALL, 5 );
	
	m_delete_row = new wxButton( this, wxID_ANY, _("Delete Row"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_delete_row, 0, wxALL, 5 );
	
	m_move_up = new wxButton( this, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_move_up, 0, wxALL, 5 );
	
	m_move_down = new wxButton( this, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_move_down, 0, wxALL, 5 );
	
	
	sbSizer1->Add( bSizer6, 0, wxALIGN_CENTER, 5 );
	
	
	m_horizontal_sizer->Add( sbSizer1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button1 = new wxButton( this, wxID_ANY, _("<<"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_button1, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	m_horizontal_sizer->Add( bSizer3, 0, wxALIGN_CENTER|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Option Choices:") ), wxHORIZONTAL );
	
	m_listCtrl1 = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxVSCROLL );
	sbSizer3->Add( m_listCtrl1, 1, wxEXPAND, 5 );
	
	
	m_horizontal_sizer->Add( sbSizer3, 1, wxEXPAND, 5 );
	
	
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
	m_add_row->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAddRow ), NULL, this );
	m_delete_row->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
}

DIALOG_FP_PLUGIN_OPTIONS_BASE::~DIALOG_FP_PLUGIN_OPTIONS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ) );
	m_add_row->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onAddRow ), NULL, this );
	m_delete_row->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_move_up->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveUp ), NULL, this );
	m_move_down->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onMoveDown ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_PLUGIN_OPTIONS_BASE::onOKButtonClick ), NULL, this );
	
}
