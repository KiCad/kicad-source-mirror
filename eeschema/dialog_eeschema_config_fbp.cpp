///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_eeschema_config_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EESCHEMA_CONFIG_FBP::DIALOG_EESCHEMA_CONFIG_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextNetListFormats = new wxStaticText( this, wxID_ANY, _("NetList Formats:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetListFormats->Wrap( -1 );
	bLeftSizer->Add( m_staticTextNetListFormats, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NetFormatBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE ); 
	bLeftSizer->Add( m_NetFormatBox, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bLeftSizer->Add( 0, 20, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sFileExtBox;
	sFileExtBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Files ext:") ), wxVERTICAL );
	
	m_InfoCmpFileExt = new wxStaticText( this, wxID_ANY, _("Cmp file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoCmpFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoCmpFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoNetFileExt = new wxStaticText( this, wxID_ANY, _("Net file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoNetFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoNetFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoLibFileExt = new wxStaticText( this, wxID_ANY, _("Library file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoLibFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoLibFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoSymbFileExt = new wxStaticText( this, wxID_ANY, _("Symbol file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoSymbFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoSymbFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoSchFileExt = new wxStaticText( this, wxID_ANY, _("Schematic file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoSchFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoSchFileExt, 0, wxALL, 5 );
	
	bLeftSizer->Add( sFileExtBox, 0, wxEXPAND, 5 );
	
	bUpperSizer->Add( bLeftSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbLibsChoiceSizer;
	sbLibsChoiceSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Libraries") ), wxVERTICAL );
	
	wxBoxSizer* bLibsButtonsSizer;
	bLibsButtonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	sbLibsChoiceSizer->Add( bLibsButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextlibList = new wxStaticText( this, wxID_ANY, _("Active Libraries:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextlibList->Wrap( -1 );
	sbLibsChoiceSizer->Add( m_staticTextlibList, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListLibr = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListLibr->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Eeschema.\nThe order of this list is important:\nEeschema searchs for a given component using this list order priority.") );
	m_ListLibr->SetMinSize( wxSize( 300,-1 ) );
	
	sbLibsChoiceSizer->Add( m_ListLibr, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bUpperSizer->Add( sbLibsChoiceSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonRemove = new wxButton( this, ID_REMOVE_LIB, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemove->SetForegroundColour( wxColour( 186, 1, 38 ) );
	m_buttonRemove->SetToolTip( _("Unload the selected library") );
	
	bRightSizer->Add( m_buttonRemove, 0, wxALL, 5 );
	
	m_buttonAdd = new wxButton( this, ID_ADD_LIB, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAdd->SetForegroundColour( wxColour( 13, 118, 1 ) );
	m_buttonAdd->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bRightSizer->Add( m_buttonAdd, 0, wxALL, 5 );
	
	m_buttonIns = new wxButton( this, wxID_ANY, _("Ins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonIns->SetForegroundColour( wxColour( 0, 65, 130 ) );
	m_buttonIns->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bRightSizer->Add( m_buttonIns, 0, wxALL, 5 );
	
	
	bRightSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_buttonOk = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonOk, 0, wxALL, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonCancel->SetForegroundColour( wxColour( 14, 0, 179 ) );
	
	bRightSizer->Add( m_buttonCancel, 0, wxALL, 5 );
	
	m_buttonSave = new wxButton( this, ID_SAVE_CFG, _("Save Cfg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSave->SetToolTip( _("Accept and save current configuration setting in the local .pro file") );
	
	bRightSizer->Add( m_buttonSave, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxStaticBoxSizer* sbLibPathSizer;
	sbLibPathSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Libraries Files Main Default  Path:") ), wxVERTICAL );
	
	wxBoxSizer* bUserLibPathSizer;
	bUserLibPathSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_LibDirCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LibDirCtrl->SetToolTip( _("Default path to search libraries which have no absolute path in name,\nor a name which does not start by ./ or ../\nIf void, the default path is kicad/share/library") );
	
	bUserLibPathSizer->Add( m_LibDirCtrl, 1, wxALL, 5 );
	
	m_buttonBrowse = new wxButton( this, ID_LIB_PATH_SEL, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserLibPathSizer->Add( m_buttonBrowse, 0, wxALL, 5 );
	
	sbLibPathSizer->Add( bUserLibPathSizer, 1, wxEXPAND, 5 );
	
	m_staticTextcurrenpaths = new wxStaticText( this, wxID_ANY, _("Current Libraries Full Paths in Use:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextcurrenpaths->Wrap( -1 );
	sbLibPathSizer->Add( m_staticTextcurrenpaths, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultLibraryPathslistBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_DefaultLibraryPathslistBox->SetMinSize( wxSize( -1,70 ) );
	
	sbLibPathSizer->Add( m_DefaultLibraryPathslistBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( sbLibPathSizer, 0, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCloseWindow ) );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnOkClick ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnSaveCfgClick ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnLibPathSelClick ), NULL, this );
}

DIALOG_EESCHEMA_CONFIG_FBP::~DIALOG_EESCHEMA_CONFIG_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCloseWindow ) );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnOkClick ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnSaveCfgClick ), NULL, this );
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnLibPathSelClick ), NULL, this );
}
