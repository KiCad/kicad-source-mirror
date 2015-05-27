///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_bom_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_BOM_BASE, DIALOG_SHIM )
	EVT_LISTBOX( wxID_ANY, DIALOG_BOM_BASE::_wxFB_OnPluginSelected )
	EVT_TEXT( IN_NAMELINE, DIALOG_BOM_BASE::_wxFB_OnNameEdited )
	EVT_BUTTON( ID_CREATE_BOM, DIALOG_BOM_BASE::_wxFB_OnRunPlugin )
	EVT_BUTTON( wxID_CANCEL, DIALOG_BOM_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( ID_HELP, DIALOG_BOM_BASE::_wxFB_OnHelp )
	EVT_BUTTON( ID_ADD_PLUGIN, DIALOG_BOM_BASE::_wxFB_OnAddPlugin )
	EVT_BUTTON( ID_REMOVEL_PLUGIN, DIALOG_BOM_BASE::_wxFB_OnRemovePlugin )
	EVT_BUTTON( wxID_ANY, DIALOG_BOM_BASE::_wxFB_OnEditPlugin )
	EVT_TEXT( ID_CMDLINE, DIALOG_BOM_BASE::_wxFB_OnCommandLineEdited )
END_EVENT_TABLE()

DIALOG_BOM_BASE::DIALOG_BOM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPluginTitle = new wxStaticText( this, wxID_ANY, _("Plugins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPluginTitle->Wrap( -1 );
	bLeftSizer->Add( m_staticTextPluginTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_lbPlugins = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bLeftSizer->Add( m_lbPlugins, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	bLeftSizer->Add( m_staticTextName, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlName = new wxTextCtrl( this, IN_NAMELINE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlName->SetMaxLength( 0 ); 
	bLeftSizer->Add( m_textCtrlName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonNetlist = new wxButton( this, ID_CREATE_BOM, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonNetlist->SetDefault(); 
	bRightSizer->Add( m_buttonNetlist, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonHelp = new wxButton( this, ID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonHelp, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline2, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonAddPlugin = new wxButton( this, ID_ADD_PLUGIN, _("Add Plugin"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonAddPlugin, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonDelPlugin = new wxButton( this, ID_REMOVEL_PLUGIN, _("Remove Plugin"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonDelPlugin, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonEdit = new wxButton( this, wxID_ANY, _("Edit Plugin File"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonEdit, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bbottomSizer;
	bbottomSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCmd = new wxStaticText( this, wxID_ANY, _("Command line:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmd->Wrap( -1 );
	bbottomSizer->Add( m_staticTextCmd, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlCommand = new wxTextCtrl( this, ID_CMDLINE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCommand->SetMaxLength( 0 ); 
	m_textCtrlCommand->SetMinSize( wxSize( 380,-1 ) );
	
	bbottomSizer->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bbottomSizer, 0, wxEXPAND, 5 );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Plugin Info:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	bMainSizer->Add( m_staticTextInfo, 0, wxRIGHT|wxLEFT, 5 );
	
	m_Messages = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	bMainSizer->Add( m_Messages, 1, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_BOM_BASE::~DIALOG_BOM_BASE()
{
}
