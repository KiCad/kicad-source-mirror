///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
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
	m_EnvVars->SetColSize( 0, 100 );
	m_EnvVars->SetColSize( 1, 180 );
	m_EnvVars->EnableDragColMove( false );
	m_EnvVars->EnableDragColSize( true );
	m_EnvVars->SetColLabelValue( 0, _("Name") );
	m_EnvVars->SetColLabelValue( 1, _("Path") );
	m_EnvVars->SetColLabelSize( wxGRID_AUTOSIZE );
	m_EnvVars->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_EnvVars->EnableDragRowSize( true );
	m_EnvVars->SetRowLabelSize( 0 );
	m_EnvVars->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_EnvVars->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_EnvVars->SetMinSize( wxSize( 604,170 ) );

	sbEnvVars->Add( m_EnvVars, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerEnvVarBtns;
	bSizerEnvVarBtns = new wxBoxSizer( wxHORIZONTAL );

	m_btnAddEnvVar = new STD_BITMAP_BUTTON( sbEnvVars->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerEnvVarBtns->Add( m_btnAddEnvVar, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerEnvVarBtns->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_btnDeleteEnvVar = new STD_BITMAP_BUTTON( sbEnvVars->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerEnvVarBtns->Add( m_btnDeleteEnvVar, 0, wxBOTTOM, 5 );


	sbEnvVars->Add( bSizerEnvVarBtns, 0, wxEXPAND, 5 );


	bSizerMain->Add( sbEnvVars, 1, wxALL|wxEXPAND, 5 );

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
	m_btnAddEnvVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddEnvVar ), NULL, this );
	m_btnDeleteEnvVar->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnRemoveEnvVar ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnHelp ), NULL, this );
}

DIALOG_CONFIGURE_PATHS_BASE::~DIALOG_CONFIGURE_PATHS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnUpdateUI ) );
	m_btnAddEnvVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnAddEnvVar ), NULL, this );
	m_btnDeleteEnvVar->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnRemoveEnvVar ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CONFIGURE_PATHS_BASE::OnHelp ), NULL, this );

}
