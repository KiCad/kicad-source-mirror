///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_block_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BLOCK_OPTIONS_BASE::DIALOG_BLOCK_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 4, 2, 0, 0 );
	
	m_Include_Modules = new wxCheckBox( this, wxID_ANY, _("Include &footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Modules, 0, wxALL, 5 );
	
	m_Include_PcbTextes = new wxCheckBox( this, wxID_ANY, _("Include t&ext items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Include_PcbTextes->SetValue(true); 
	gSizer1->Add( m_Include_PcbTextes, 0, wxALL, 5 );
	
	m_IncludeLockedModules = new wxCheckBox( this, wxID_ANY, _("Include &locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_IncludeLockedModules, 0, wxALL, 5 );
	
	m_Include_Draw_Items = new wxCheckBox( this, wxID_ANY, _("Include &drawings"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Draw_Items, 0, wxALL, 5 );
	
	m_Include_Tracks = new wxCheckBox( this, wxID_ANY, _("Include &tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Tracks, 0, wxALL, 5 );
	
	m_Include_Edges_Items = new wxCheckBox( this, wxID_ANY, _("Include &board outline layer"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Edges_Items, 0, wxALL, 5 );
	
	m_Include_Zones = new wxCheckBox( this, wxID_ANY, _("Include &zones"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Zones, 0, wxALL, 5 );
	
	m_DrawBlockItems = new wxCheckBox( this, wxID_ANY, _("Draw &selected items while moving"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_DrawBlockItems, 0, wxALL, 5 );
	
	
	sbSizer1->Add( gSizer1, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizer1->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_checkBoxIncludeInvisible = new wxCheckBox( this, wxID_ANY, _("Include &items on invisible layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkBoxIncludeInvisible, 0, wxALL, 5 );
	
	
	bSizerMain->Add( sbSizer1, 1, wxALL|wxEXPAND, 5 );
	
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
	m_checkBoxIncludeInvisible->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
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
	m_checkBoxIncludeInvisible->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::checkBoxClicked ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::OnCancel ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BLOCK_OPTIONS_BASE::ExecuteCommand ), NULL, this );
	
}
