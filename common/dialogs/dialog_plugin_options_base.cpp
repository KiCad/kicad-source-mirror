///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_plugin_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLUGIN_OPTIONS_BASE::DIALOG_PLUGIN_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_horizontal_sizer;
	m_horizontal_sizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* m_grid_sizer;
	m_grid_sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Plugin Options") ), wxVERTICAL );

	m_grid_sizer->SetMinSize( wxSize( 400,300 ) );
	m_grid = new WX_GRID( m_grid_sizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxVSCROLL );

	// Grid
	m_grid->CreateGrid( 1, 2 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 120 );
	m_grid->SetColSize( 1, 240 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Option") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid_sizer->Add( m_grid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_append_button = new STD_BITMAP_BUTTON( m_grid_sizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bButtonsSizer->Add( m_append_button, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_delete_button = new STD_BITMAP_BUTTON( m_grid_sizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bButtonsSizer->Add( m_delete_button, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_grid_sizer->Add( bButtonsSizer, 0, wxEXPAND|wxTOP, 5 );


	m_horizontal_sizer->Add( m_grid_sizer, 3, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* m_options_sizer;
	m_options_sizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Option Choices") ), wxVERTICAL );

	m_listbox = new wxListBox( m_options_sizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_SINGLE );
	m_listbox->SetToolTip( _("Options supported by current plugin") );

	m_options_sizer->Add( m_listbox, 3, wxALL|wxEXPAND, 5 );

	m_append_choice_button = new wxButton( m_options_sizer->GetStaticBox(), wxID_ANY, _("<<    Append Selected Option"), wxDefaultPosition, wxDefaultSize, 0 );
	m_options_sizer->Add( m_append_choice_button, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_options_sizer->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_html = new HTML_WINDOW( m_options_sizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO|wxBORDER_SIMPLE|wxVSCROLL );
	m_html->SetMinSize( wxSize( 280,100 ) );

	m_options_sizer->Add( m_html, 2, wxALL|wxEXPAND, 5 );


	m_horizontal_sizer->Add( m_options_sizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_horizontal_sizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onGridCellChange ), NULL, this );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onSize ), NULL, this );
	m_grid->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onUpdateUI ), NULL, this );
	m_append_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_listbox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onListBoxItemSelected ), NULL, this );
	m_listbox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onListBoxItemDoubleClicked ), NULL, this );
	m_append_choice_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );
}

DIALOG_PLUGIN_OPTIONS_BASE::~DIALOG_PLUGIN_OPTIONS_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onGridCellChange ), NULL, this );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onSize ), NULL, this );
	m_grid->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onUpdateUI ), NULL, this );
	m_append_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onAppendRow ), NULL, this );
	m_delete_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onDeleteRow ), NULL, this );
	m_listbox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onListBoxItemSelected ), NULL, this );
	m_listbox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onListBoxItemDoubleClicked ), NULL, this );
	m_append_choice_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLUGIN_OPTIONS_BASE::onAppendOption ), NULL, this );

}
