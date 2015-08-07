///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_exchange_modules_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXCHANGE_MODULE_BASE::DIALOG_EXCHANGE_MODULE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Current footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bLeftSizer->Add( m_staticText6, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OldModule = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_OldModule->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_OldModule, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Current value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bLeftSizer->Add( m_staticText7, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CurrValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_CurrValue->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_CurrValue, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Current reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bLeftSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CurrReference = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bLeftSizer->Add( m_CurrReference, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("New footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bLeftSizer->Add( m_staticText8, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NewModule = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NewModule->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_NewModule, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, 0, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_SelectionChoices[] = { _("Change footprint"), _("Change footprints"), _("Change footprints having same value"), _("Update all footprints of the board") };
	int m_SelectionNChoices = sizeof( m_SelectionChoices ) / sizeof( wxString );
	m_Selection = new wxRadioBox( this, ID_SELECTION_CLICKED, _("Options"), wxDefaultPosition, wxDefaultSize, m_SelectionNChoices, m_SelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_Selection->SetSelection( 1 );
	bMiddleSizer->Add( m_Selection, 0, wxALL, 5 );
	
	
	bUpperSizer->Add( bMiddleSizer, 0, wxALL, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_OKbutton = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_OKbutton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Quitbutton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_Quitbutton, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonCmpList = new wxButton( this, wxID_ANY, _("Export Footprint Association File"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCmpList, 0, wxALL|wxEXPAND, 5 );
	
	m_Browsebutton = new wxButton( this, wxID_ANY, _("List Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_Browsebutton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonFPViewer = new wxButton( this, wxID_ANY, _("View Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonFPViewer, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextMsg = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMsg->Wrap( -1 );
	bMainSizer->Add( m_staticTextMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_WinMessages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_WinMessages->SetMinSize( wxSize( -1,75 ) );
	
	bMainSizer->Add( m_WinMessages, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_Selection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnSelectionClicked ), NULL, this );
	m_OKbutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_Quitbutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
	m_buttonCmpList->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_Browsebutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::BrowseAndSelectFootprint ), NULL, this );
	m_buttonFPViewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
}

DIALOG_EXCHANGE_MODULE_BASE::~DIALOG_EXCHANGE_MODULE_BASE()
{
	// Disconnect Events
	m_Selection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnSelectionClicked ), NULL, this );
	m_OKbutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_Quitbutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
	m_buttonCmpList->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_Browsebutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::BrowseAndSelectFootprint ), NULL, this );
	m_buttonFPViewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	
}
