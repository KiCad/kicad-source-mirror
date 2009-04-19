///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pcbnew_config_libs_and_paths_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PCBNEW_CONFIG_LIBS_FBP::DIALOG_PCBNEW_CONFIG_LIBS_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sFileExtBox;
	sFileExtBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Files ext:") ), wxVERTICAL );
	
	m_InfoBoardFileExt = new wxStaticText( this, wxID_ANY, _("Board file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoBoardFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoBoardFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoCmpFileExt = new wxStaticText( this, wxID_ANY, _("Cmp file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoCmpFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoCmpFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoLibFileExt = new wxStaticText( this, wxID_ANY, _("Library file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoLibFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoLibFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_InfoNetlistFileExt = new wxStaticText( this, wxID_ANY, _("Netlist file Ext: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_InfoNetlistFileExt->Wrap( -1 );
	sFileExtBox->Add( m_InfoNetlistFileExt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	bLeftSizer->Add( sFileExtBox, 0, wxEXPAND, 5 );
	
	bUpperSizer->Add( bLeftSizer, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxStaticBoxSizer* sbLibsChoiceSizer;
	sbLibsChoiceSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Libraries") ), wxVERTICAL );
	
	wxBoxSizer* bLibsButtonsSizer;
	bLibsButtonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	sbLibsChoiceSizer->Add( bLibsButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextlibList = new wxStaticText( this, wxID_ANY, _("Active Libraries:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextlibList->Wrap( -1 );
	sbLibsChoiceSizer->Add( m_staticTextlibList, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListLibr = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListLibr->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Pcbnew.\nThe order of this list is important:\nPcbnew searchs for a given footprint using this list order priority.") );
	m_ListLibr->SetMinSize( wxSize( 300,-1 ) );
	
	sbLibsChoiceSizer->Add( m_ListLibr, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bUpperSizer->Add( sbLibsChoiceSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddLib = new wxButton( this, ID_ADD_LIB, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddLib->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bRightSizer->Add( m_buttonAddLib, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonIns = new wxButton( this, wxID_ANY, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonIns->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bRightSizer->Add( m_buttonIns, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemoveLib = new wxButton( this, ID_REMOVE_LIB, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveLib->SetToolTip( _("Unload the selected library") );
	
	bRightSizer->Add( m_buttonRemoveLib, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bRightSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_buttonOk = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonOk, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCancel, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonSave = new wxButton( this, ID_SAVE_CFG, _("Save Cfg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSave->SetToolTip( _("Accept and save current configuration setting in the local .pro file") );
	
	bRightSizer->Add( m_buttonSave, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bUpperSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxStaticBoxSizer* sbModulesDocSizer;
	sbModulesDocSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Module Doc File:") ), wxHORIZONTAL );
	
	m_TextHelpModulesFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbModulesDocSizer->Add( m_TextHelpModulesFileName, 1, wxALL, 5 );
	
	m_buttonModDoc = new wxButton( this, wxID_BROWSE_MOD_DOC, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModulesDocSizer->Add( m_buttonModDoc, 0, wxALL, 5 );
	
	bMainSizer->Add( sbModulesDocSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbLibPathSizer;
	sbLibPathSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Path for Libraries Files:") ), wxVERTICAL );
	
	wxBoxSizer* bUserLibPathSizer;
	bUserLibPathSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Path:") ), wxHORIZONTAL );
	
	wxBoxSizer* bUserListSizer;
	bUserListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_listUserPaths = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_listUserPaths->SetToolTip( _("Additional paths used in this project. The priority is highter than default Kicad paths.") );
	
	bUserListSizer->Add( m_listUserPaths, 1, wxEXPAND, 5 );
	
	sbSizer4->Add( bUserListSizer, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxBoxSizer* bUserPathsButtonsSizer;
	bUserPathsButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddPath = new wxButton( this, ID_LIB_PATH_SEL, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonAddPath, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonInsPath = new wxButton( this, wxID_INSERT_PATH, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonInsPath, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemovePath = new wxButton( this, wxID_REMOVE_PATH, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonRemovePath, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizer4->Add( bUserPathsButtonsSizer, 0, wxEXPAND, 5 );
	
	bUserLibPathSizer->Add( sbSizer4, 1, wxEXPAND, 5 );
	
	sbLibPathSizer->Add( bUserLibPathSizer, 1, wxEXPAND, 5 );
	
	m_staticTextcurrenpaths = new wxStaticText( this, wxID_ANY, _("Current Full Paths for  Libraries  Files  in Use:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextcurrenpaths->Wrap( -1 );
	sbLibPathSizer->Add( m_staticTextcurrenpaths, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultLibraryPathslistBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_DefaultLibraryPathslistBox->SetToolTip( _("Paths  (system paths and user paths) used to search and load libraries files and component doc files.\nSorted by decreasing priority order.") );
	m_DefaultLibraryPathslistBox->SetMinSize( wxSize( -1,70 ) );
	
	sbLibPathSizer->Add( m_DefaultLibraryPathslistBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( sbLibPathSizer, 0, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCloseWindow ) );
	m_buttonAddLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnOkClick ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCancelClick ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnSaveCfgClick ), NULL, this );
	m_buttonModDoc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveUserPath ), NULL, this );
}

DIALOG_PCBNEW_CONFIG_LIBS_FBP::~DIALOG_PCBNEW_CONFIG_LIBS_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCloseWindow ) );
	m_buttonAddLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnOkClick ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCancelClick ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnSaveCfgClick ), NULL, this );
	m_buttonModDoc->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveUserPath ), NULL, this );
}
