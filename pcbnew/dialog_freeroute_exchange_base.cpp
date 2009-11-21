///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_freeroute_exchange_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FREEROUTE_BASE::DIALOG_FREEROUTE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, wxT("Export/Import to/from FreeRoute:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bLeftSizer->Add( m_staticText2, 0, wxALL, 5 );
	
	wxBoxSizer* bLeftSubSizerSizer;
	bLeftSubSizerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	bLeftSubSizerSizer->Add( 20, 20, 0, 0, 5 );
	
	wxBoxSizer* bLeftButtonsSizer;
	bLeftButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ExportDSN = new wxButton( this, ID_BUTTON_EXPORT_DSN, wxT("Export a Specctra Design (*.dsn) File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExportDSN->SetToolTip( wxT("Export a Specctra DSN file (to FreeRouter)") );
	
	bLeftButtonsSizer->Add( m_ExportDSN, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonLaunchFreeroute = new wxButton( this, wxID_BUTTON_LAUNCH, wxT("Launch FreeRouter via Java Web Start"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonLaunchFreeroute->SetToolTip( wxT("Use Java Web Start function to run FreeRouter via Internet (or your Browser if not found)") );
	
	bLeftButtonsSizer->Add( m_buttonLaunchFreeroute, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonImport = new wxButton( this, wxID_BUTTON_IMPORT, wxT("Back Import the Specctra Session (*.ses) File"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonImport->SetToolTip( wxT("Merge a session file created by FreeRouter with the current board.") );
	
	bLeftButtonsSizer->Add( m_buttonImport, 0, wxALL|wxEXPAND, 5 );
	
	bLeftSubSizerSizer->Add( bLeftButtonsSizer, 1, wxEXPAND, 5 );
	
	bLeftSizer->Add( bLeftSubSizerSizer, 1, wxEXPAND, 5 );
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bUpperSizer->Add( m_staticline2, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, wxT("FreeRoute Info:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bRightSizer->Add( m_staticText3, 0, wxALL, 5 );
	
	wxBoxSizer* bRightSubSizer;
	bRightSubSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	bRightSubSizer->Add( 20, 20, 0, 0, 5 );
	
	wxBoxSizer* bRightButtonsSizer;
	bRightButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonVisit = new wxButton( this, wxID_BUTTON_VISIT, wxT("Visit the FreeRouting.net Website with your Browser"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightButtonsSizer->Add( m_buttonVisit, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("FreeRouting.net URL"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bRightButtonsSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FreerouteURLName = new wxTextCtrl( this, wxID_TEXT_URL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FreerouteURLName->SetToolTip( wxT("The URL of the FreeRouting.net website") );
	
	bRightButtonsSizer->Add( m_FreerouteURLName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonHelp = new wxButton( this, wxID_BUTTON_HELP, wxT("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightButtonsSizer->Add( m_buttonHelp, 0, wxALL|wxEXPAND, 5 );
	
	bRightSubSizer->Add( bRightButtonsSizer, 1, wxEXPAND, 5 );
	
	bRightSizer->Add( bRightSubSizer, 1, wxEXPAND, 5 );
	
	bUpperSizer->Add( bRightSizer, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bMainSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_ExportDSN->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnExportButtonClick ), NULL, this );
	m_buttonLaunchFreeroute->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnLaunchButtonClick ), NULL, this );
	m_buttonImport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnImportButtonClick ), NULL, this );
	m_buttonVisit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnVisitButtonClick ), NULL, this );
	m_FreerouteURLName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnTextEditFrUrlUpdated ), NULL, this );
	m_buttonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnHelpButtonClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnOKButtonClick ), NULL, this );
}

DIALOG_FREEROUTE_BASE::~DIALOG_FREEROUTE_BASE()
{
	// Disconnect Events
	m_ExportDSN->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnExportButtonClick ), NULL, this );
	m_buttonLaunchFreeroute->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnLaunchButtonClick ), NULL, this );
	m_buttonImport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnImportButtonClick ), NULL, this );
	m_buttonVisit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnVisitButtonClick ), NULL, this );
	m_FreerouteURLName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnTextEditFrUrlUpdated ), NULL, this );
	m_buttonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnHelpButtonClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FREEROUTE_BASE::OnOKButtonClick ), NULL, this );
}
