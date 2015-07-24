///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_freeroute_exchange_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FREEROUTE_BASE::DIALOG_FREEROUTE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextMsg = new wxStaticText( this, wxID_ANY, _("Export/Import to/from FreeRoute:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMsg->Wrap( -1 );
	m_staticTextMsg->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bLeftSizer->Add( m_staticTextMsg, 0, wxALL, 5 );
	
	wxBoxSizer* bLeftSubSizerSizer;
	bLeftSubSizerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	bLeftSubSizerSizer->Add( 20, 20, 0, 0, 5 );
	
	wxBoxSizer* bLeftButtonsSizer;
	bLeftButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ExportDSN = new wxButton( this, wxID_ANY, _("Export a Specctra Design (*.dsn) File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportDSN->SetToolTip( _("Export a Specctra DSN file (to FreeRouter)") );
	
	bLeftButtonsSizer->Add( m_ExportDSN, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonLaunchFreeroute = new wxButton( this, wxID_ANY, _("Export a Specctra Design and Launch FreeRoute"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonLaunchFreeroute->SetToolTip( _("FreeRouter can be run only if freeroute.jar is found in Kicad binaries folder") );
	
	bLeftButtonsSizer->Add( m_buttonLaunchFreeroute, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonImport = new wxButton( this, wxID_ANY, _("Back Import the Specctra Session (*.ses) File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonImport->SetToolTip( _("Merge a session file created by FreeRouter with the current board.") );
	
	bLeftButtonsSizer->Add( m_buttonImport, 0, wxALL|wxEXPAND, 5 );
	
	
	bLeftSubSizerSizer->Add( bLeftButtonsSizer, 1, wxEXPAND, 5 );
	
	
	bLeftSizer->Add( bLeftSubSizerSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizerHelp = new wxButton( this, wxID_HELP );
	m_sdbSizer->AddButton( m_sdbSizerHelp );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_ExportDSN->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnExportButtonClick ), NULL, this );
	m_buttonLaunchFreeroute->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnLaunchButtonClick ), NULL, this );
	m_buttonImport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnImportButtonClick ), NULL, this );
	m_sdbSizerHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnHelpButtonClick ), NULL, this );
}

DIALOG_FREEROUTE_BASE::~DIALOG_FREEROUTE_BASE()
{
	// Disconnect Events
	m_ExportDSN->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnExportButtonClick ), NULL, this );
	m_buttonLaunchFreeroute->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnLaunchButtonClick ), NULL, this );
	m_buttonImport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnImportButtonClick ), NULL, this );
	m_sdbSizerHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnHelpButtonClick ), NULL, this );
	
}
