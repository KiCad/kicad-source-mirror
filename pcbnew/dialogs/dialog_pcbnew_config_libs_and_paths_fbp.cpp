///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pcbnew_config_libs_and_paths_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PCBNEW_CONFIG_LIBS_FBP::DIALOG_PCBNEW_CONFIG_LIBS_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bLibsChoiceSizer;
	bLibsChoiceSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLibs = new wxStaticText( this, wxID_ANY, _("Footprint library files"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLibs->Wrap( -1 );
	bLibsChoiceSizer->Add( m_staticTextLibs, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bLibsChoiceListSizer;
	bLibsChoiceListSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_ListLibr = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListLibr->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Pcbnew.\nThe order of this list is important:\nPcbnew searchs for a given footprint using this list order priority.") );
	m_ListLibr->SetMinSize( wxSize( 400,90 ) );
	
	bLibsChoiceListSizer->Add( m_ListLibr, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddLib = new wxButton( this, ID_ADD_LIB, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddLib->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bRightSizer->Add( m_buttonAddLib, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonIns = new wxButton( this, wxID_ANY, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonIns->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bRightSizer->Add( m_buttonIns, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemoveLib = new wxButton( this, ID_REMOVE_LIB, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveLib->SetToolTip( _("Unload the selected library") );
	
	bRightSizer->Add( m_buttonRemoveLib, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonUp = new wxButton( this, wxID_ANY, _("Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonUp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonDown = new wxButton( this, wxID_ANY, _("Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonDown, 0, wxALL, 5 );
	
	
	bLibsChoiceListSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bLibsChoiceSizer->Add( bLibsChoiceListSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bLibsChoiceSizer, 2, wxEXPAND, 5 );
	
	wxBoxSizer* bModulesDocSizer;
	bModulesDocSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextModulesDoc = new wxStaticText( this, wxID_ANY, _("Footprint documentation file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextModulesDoc->Wrap( -1 );
	bModulesDocSizer->Add( m_staticTextModulesDoc, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDoc;
	bSizerDoc = new wxBoxSizer( wxHORIZONTAL );
	
	m_TextHelpModulesFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextHelpModulesFileName->SetMaxLength( 0 ); 
	bSizerDoc->Add( m_TextHelpModulesFileName, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_buttonModDoc = new wxButton( this, wxID_BROWSE_MOD_DOC, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDoc->Add( m_buttonModDoc, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );
	
	
	bModulesDocSizer->Add( bSizerDoc, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bModulesDocSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerPaths;
	bSizerPaths = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPaths = new wxStaticText( this, wxID_ANY, _("User defined search paths"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPaths->Wrap( -1 );
	bSizerPaths->Add( m_staticTextPaths, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerPathsChoice;
	bSizerPathsChoice = new wxBoxSizer( wxHORIZONTAL );
	
	m_listUserPaths = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_listUserPaths->SetToolTip( _("Additional paths used in this project. The priority is higher than default KiCad paths.") );
	
	bSizerPathsChoice->Add( m_listUserPaths, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bUserPathsButtonsSizer;
	bUserPathsButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddPath = new wxButton( this, ID_LIB_PATH_SEL, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonAddPath, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonInsPath = new wxButton( this, wxID_INSERT_PATH, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonInsPath, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_buttonRemovePath = new wxButton( this, wxID_REMOVE_PATH, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonRemovePath, 0, wxALL|wxBOTTOM, 5 );
	
	
	bSizerPathsChoice->Add( bUserPathsButtonsSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bSizerPaths->Add( bSizerPathsChoice, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerPaths, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bLibPathSizer;
	bLibPathSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPathList = new wxStaticText( this, wxID_ANY, _("Current search path list"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPathList->Wrap( -1 );
	bLibPathSizer->Add( m_staticTextPathList, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultLibraryPathslistBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_DefaultLibraryPathslistBox->SetToolTip( _("System and user paths used to search and load library files and component doc files.\nSorted by decreasing priority order.") );
	m_DefaultLibraryPathslistBox->SetMinSize( wxSize( -1,70 ) );
	
	bLibPathSizer->Add( m_DefaultLibraryPathslistBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bLibPathSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCloseWindow ) );
	m_buttonAddLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnButtonUpClick ), NULL, this );
	m_buttonDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnButtonDownClick ), NULL, this );
	m_buttonModDoc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnOkClick ), NULL, this );
}

DIALOG_PCBNEW_CONFIG_LIBS_FBP::~DIALOG_PCBNEW_CONFIG_LIBS_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCloseWindow ) );
	m_buttonAddLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnButtonUpClick ), NULL, this );
	m_buttonDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnButtonDownClick ), NULL, this );
	m_buttonModDoc->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCBNEW_CONFIG_LIBS_FBP::OnOkClick ), NULL, this );
	
}
