///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_config_equfiles_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CONFIG_EQUFILES_BASE::DIALOG_CONFIG_EQUFILES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* listLabel;
	listLabel = new wxStaticText( this, wxID_ANY, _("Footprint association files:"), wxDefaultPosition, wxDefaultSize, 0 );
	listLabel->Wrap( -1 );
	bUpperSizer->Add( listLabel, 0, wxBOTTOM, 2 );

	m_filesListBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE );
	m_filesListBox->SetMinSize( wxSize( 500,100 ) );

	bUpperSizer->Add( m_filesListBox, 2, wxEXPAND|wxBOTTOM, 3 );

	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add association file") );

	bButtonsSizer->Add( m_bpAdd, 0, wxRIGHT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonsSizer->Add( m_bpMoveUp, 0, wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonsSizer->Add( m_bpMoveDown, 0, wxRIGHT, 5 );

	m_bpEdit = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpEdit->SetToolTip( _("Edit association file") );

	bButtonsSizer->Add( m_bpEdit, 0, wxRIGHT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Remove association file") );

	bButtonsSizer->Add( m_bpDelete, 0, wxRIGHT|wxLEFT, 10 );


	bUpperSizer->Add( bButtonsSizer, 0, wxEXPAND, 5 );


	bUpperSizer->Add( 0, 25, 0, wxEXPAND, 5 );

	wxStaticText* envVarsLabel;
	envVarsLabel = new wxStaticText( this, wxID_ANY, _("Available path substitutions:"), wxDefaultPosition, wxDefaultSize, 0 );
	envVarsLabel->Wrap( -1 );
	bUpperSizer->Add( envVarsLabel, 0, wxBOTTOM, 2 );

	m_gridEnvVars = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridEnvVars->CreateGrid( 2, 2 );
	m_gridEnvVars->EnableEditing( true );
	m_gridEnvVars->EnableGridLines( true );
	m_gridEnvVars->EnableDragGridSize( false );
	m_gridEnvVars->SetMargins( 0, 0 );

	// Columns
	m_gridEnvVars->EnableDragColMove( false );
	m_gridEnvVars->EnableDragColSize( true );
	m_gridEnvVars->SetColLabelValue( 0, _("Name") );
	m_gridEnvVars->SetColLabelValue( 1, _("Value") );
	m_gridEnvVars->SetColLabelSize( 0 );
	m_gridEnvVars->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridEnvVars->AutoSizeRows();
	m_gridEnvVars->EnableDragRowSize( true );
	m_gridEnvVars->SetRowLabelSize( 0 );
	m_gridEnvVars->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridEnvVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bUpperSizer->Add( m_gridEnvVars, 1, wxEXPAND, 5 );


	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCloseWindow ) );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnAddFiles ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveDown ), NULL, this );
	m_bpEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnEditEquFile ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnRemoveFiles ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnOkClick ), NULL, this );
}

DIALOG_CONFIG_EQUFILES_BASE::~DIALOG_CONFIG_EQUFILES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnCloseWindow ) );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnAddFiles ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnButtonMoveDown ), NULL, this );
	m_bpEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnEditEquFile ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnRemoveFiles ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIG_EQUFILES_BASE::OnOkClick ), NULL, this );

}
