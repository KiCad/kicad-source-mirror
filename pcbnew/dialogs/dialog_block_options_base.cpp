///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_block_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BLOCK_OPTIONS_BASE::DIALOG_BLOCK_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 4, 2, 0, 0 );
	
	m_Include_Modules = new wxCheckBox( this, wxID_ANY, _("Include Modules"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Modules, 0, wxALL, 5 );
	
	m_Include_PcbTextes = new wxCheckBox( this, wxID_ANY, _("Include Texts on Copper Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_PcbTextes, 0, wxALL, 5 );
	
	m_IncludeLockedModules = new wxCheckBox( this, wxID_ANY, _("Include Locked Modules"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_IncludeLockedModules, 0, wxALL, 5 );
	
	m_Include_Draw_Items = new wxCheckBox( this, wxID_ANY, _("Include Drawings"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Draw_Items, 0, wxALL, 5 );
	
	m_Include_Tracks = new wxCheckBox( this, wxID_ANY, _("Include Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Tracks, 0, wxALL, 5 );
	
	m_Include_Edges_Items = new wxCheckBox( this, wxID_ANY, _("Include Board Outline Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Edges_Items, 0, wxALL, 5 );
	
	m_Include_Zones = new wxCheckBox( this, wxID_ANY, _("Include Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Zones, 0, wxALL, 5 );
	
	m_DrawBlockItems = new wxCheckBox( this, wxID_ANY, _("Draw Block Items while Moving"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_DrawBlockItems, 0, wxALL, 5 );
	
	bSizerMain->Add( gSizer1, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizerMain->Add( m_sdbSizer1, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_Include_Modules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_PcbTextes->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_IncludeLockedModules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Draw_Items->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Tracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Edges_Items->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Zones->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_DrawBlockItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::OnCancel ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::ExecuteCommand ), NULL, this );
}

DIALOG_BLOCK_OPTIONS_BASE::~DIALOG_BLOCK_OPTIONS_BASE()
{
	// Disconnect Events
	m_Include_Modules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_PcbTextes->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_IncludeLockedModules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Draw_Items->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Tracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Edges_Items->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_Include_Zones->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_DrawBlockItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::OnCancel ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::ExecuteCommand ), NULL, this );
	
}
