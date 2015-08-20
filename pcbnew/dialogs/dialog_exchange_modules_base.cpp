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
	
	m_staticTextCmpVal = new wxStaticText( this, wxID_ANY, _("Component value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmpVal->Wrap( -1 );
	bLeftSizer->Add( m_staticTextCmpVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CmpValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_CmpValue->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_CmpValue, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_staticTexCmpRef = new wxStaticText( this, wxID_ANY, _("Component reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTexCmpRef->Wrap( -1 );
	bLeftSizer->Add( m_staticTexCmpRef, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CmpReference = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bLeftSizer->Add( m_CmpReference, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_SelectionChoices[] = { _("Change footprint"), _("Change footprints"), _("Change footprints having same value"), _("Update all footprints of the board") };
	int m_SelectionNChoices = sizeof( m_SelectionChoices ) / sizeof( wxString );
	m_Selection = new wxRadioBox( this, ID_SELECTION_CLICKED, _("Options"), wxDefaultPosition, wxDefaultSize, m_SelectionNChoices, m_SelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_Selection->SetSelection( 1 );
	bMiddleSizer->Add( m_Selection, 0, wxALL, 5 );
	
	
	bUpperSizer->Add( bMiddleSizer, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonCmpList = new wxButton( this, wxID_ANY, _("Export Footprint Association File"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCmpList, 0, wxALL|wxEXPAND, 5 );
	
	m_Browsebutton = new wxButton( this, wxID_ANY, _("List Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_Browsebutton, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonFPViewer = new wxButton( this, wxID_ANY, _("View Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonFPViewer, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextCurrFPID = new wxStaticText( this, wxID_ANY, _("Current footprint name (FPID)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCurrFPID->Wrap( -1 );
	bMainSizer->Add( m_staticTextCurrFPID, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CurrentFootprintFPID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_CurrentFootprintFPID->SetMaxLength( 0 ); 
	bMainSizer->Add( m_CurrentFootprintFPID, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextNewFPID = new wxStaticText( this, wxID_ANY, _("New footprint name (FPID)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNewFPID->Wrap( -1 );
	bMainSizer->Add( m_staticTextNewFPID, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NewFootprintFPID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NewFootprintFPID->SetMaxLength( 0 ); 
	bMainSizer->Add( m_NewFootprintFPID, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextMsg = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMsg->Wrap( -1 );
	bMainSizer->Add( m_staticTextMsg, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_WinMessages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_WinMessages->SetMinSize( wxSize( -1,150 ) );
	
	bMainSizer->Add( m_WinMessages, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerButts;
	bSizerButts = new wxBoxSizer( wxHORIZONTAL );
	
	m_Applybutton = new wxButton( this, wxID_OK, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButts->Add( m_Applybutton, 0, wxEXPAND|wxALL, 5 );
	
	m_Quitbutton = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButts->Add( m_Quitbutton, 0, wxALL|wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerButts, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_Selection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnSelectionClicked ), NULL, this );
	m_buttonCmpList->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_Browsebutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::BrowseAndSelectFootprint ), NULL, this );
	m_buttonFPViewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_Applybutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_Quitbutton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
}

DIALOG_EXCHANGE_MODULE_BASE::~DIALOG_EXCHANGE_MODULE_BASE()
{
	// Disconnect Events
	m_Selection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnSelectionClicked ), NULL, this );
	m_buttonCmpList->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::RebuildCmpList ), NULL, this );
	m_Browsebutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::BrowseAndSelectFootprint ), NULL, this );
	m_buttonFPViewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::ViewAndSelectFootprint ), NULL, this );
	m_Applybutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnOkClick ), NULL, this );
	m_Quitbutton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_MODULE_BASE::OnQuit ), NULL, this );
	
}
