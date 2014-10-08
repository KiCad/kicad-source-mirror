///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_netlist_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( NETLIST_DIALOG_BASE, DIALOG_SHIM )
	EVT_NOTEBOOK_PAGE_CHANGED( ID_CHANGE_NOTEBOOK_PAGE, NETLIST_DIALOG_BASE::_wxFB_OnNetlistTypeSelection )
	EVT_BUTTON( ID_CREATE_NETLIST, NETLIST_DIALOG_BASE::_wxFB_GenNetlist )
	EVT_BUTTON( wxID_CANCEL, NETLIST_DIALOG_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( ID_ADD_PLUGIN, NETLIST_DIALOG_BASE::_wxFB_OnAddPlugin )
	EVT_BUTTON( ID_DEL_PLUGIN, NETLIST_DIALOG_BASE::_wxFB_OnDelPlugin )
END_EVENT_TABLE()

NETLIST_DIALOG_BASE::NETLIST_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_CHANGE_NOTEBOOK_PAGE, wxDefaultPosition, wxDefaultSize, 0 );
	
	bRightSizer->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	
	bLeftSizer->Add( 0, 0, 0, wxTOP, 15 );
	
	m_buttonNetlist = new wxButton( this, ID_CREATE_NETLIST, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_buttonNetlist, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonAddPlugin = new wxButton( this, ID_ADD_PLUGIN, _("Add Plugin"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_buttonAddPlugin, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonDelPlugin = new wxButton( this, ID_DEL_PLUGIN, _("Remove Plugin"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_buttonDelPlugin, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bLeftSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_cbUseDefaultNetlistName = new wxCheckBox( this, wxID_ANY, _("Use default netname"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_cbUseDefaultNetlistName, 0, wxALL, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticTextDefaultFN = new wxStaticText( this, wxID_ANY, _("Default Netlist Filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefaultFN->Wrap( -1 );
	bMainSizer->Add( m_staticTextDefaultFN, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlDefaultFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlDefaultFileName->SetMaxLength( 0 ); 
	bMainSizer->Add( m_textCtrlDefaultFileName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

NETLIST_DIALOG_BASE::~NETLIST_DIALOG_BASE()
{
}

BEGIN_EVENT_TABLE( NETLIST_DIALOG_ADD_PLUGIN_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_OK, NETLIST_DIALOG_ADD_PLUGIN_BASE::_wxFB_OnOKClick )
	EVT_BUTTON( wxID_CANCEL, NETLIST_DIALOG_ADD_PLUGIN_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_BROWSE_PLUGINS, NETLIST_DIALOG_ADD_PLUGIN_BASE::_wxFB_OnBrowsePlugins )
END_EVENT_TABLE()

NETLIST_DIALOG_ADD_PLUGIN_BASE::NETLIST_DIALOG_ADD_PLUGIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Netlist command:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bSizerLeft->Add( m_staticTextCmd, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlCommand = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMaxLength( 0 ); 
	m_textCtrlCommand->SetMinSize( wxSize( 300,-1 ) );
	
	bSizerLeft->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	bSizerLeft->Add( m_staticTextName, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlName->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_textCtrlName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerMain->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bSizerRight->Add( m_buttonOK, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonCancel, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPlugin = new wxButton( this, wxID_BROWSE_PLUGINS, _("Browse Plugins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRight->Add( m_buttonPlugin, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerRight, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
}

NETLIST_DIALOG_ADD_PLUGIN_BASE::~NETLIST_DIALOG_ADD_PLUGIN_BASE()
{
}
