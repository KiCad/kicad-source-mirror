///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_eeschema_config_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EESCHEMA_CONFIG_FBP::DIALOG_EESCHEMA_CONFIG_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLibsList = new wxStaticText( this, wxID_ANY, _("Component library files"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLibsList->Wrap( -1 );
	bSizerUpper->Add( m_staticTextLibsList, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerLibsChoice;
	bSizerLibsChoice = new wxBoxSizer( wxHORIZONTAL );
	
	m_ListLibr = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListLibr->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Eeschema.\nThe order of this list is important:\nEeschema searchs for a given component using this list order priority.") );
	m_ListLibr->SetMinSize( wxSize( 400,250 ) );
	
	bSizerLibsChoice->Add( m_ListLibr, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddLib = new wxButton( this, ID_ADD_LIB, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddLib->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bRightSizer->Add( m_buttonAddLib, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_buttonIns = new wxButton( this, wxID_ANY, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonIns->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bRightSizer->Add( m_buttonIns, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_buttonRemoveLib = new wxButton( this, ID_REMOVE_LIB, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveLib->SetToolTip( _("Unload the selected library") );
	
	bRightSizer->Add( m_buttonRemoveLib, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonUp = new wxButton( this, wxID_ANY, _("Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonUp, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonDown = new wxButton( this, wxID_ANY, _("Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonDown, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerLibsChoice->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizerUpper->Add( bSizerLibsChoice, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerUpper, 2, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPaths = new wxStaticText( this, wxID_ANY, _("User defined search path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPaths->Wrap( -1 );
	bSizerMiddle->Add( m_staticTextPaths, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerPathsChoice;
	bSizerPathsChoice = new wxBoxSizer( wxHORIZONTAL );
	
	m_listUserPaths = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_listUserPaths->SetToolTip( _("Additional paths used in this project. The priority is higher than default KiCad paths.") );
	m_listUserPaths->SetMinSize( wxSize( 400,90 ) );
	
	bSizerPathsChoice->Add( m_listUserPaths, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bUserPathsButtonsSizer;
	bUserPathsButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddPath = new wxButton( this, ID_LIB_PATH_SEL, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonAddPath, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_buttonInsPath = new wxButton( this, wxID_INSERT_PATH, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonInsPath, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_buttonRemovePath = new wxButton( this, wxID_REMOVE_PATH, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonRemovePath, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerPathsChoice->Add( bUserPathsButtonsSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizerMiddle->Add( bSizerPathsChoice, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bSizerMiddle, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPathlist = new wxStaticText( this, wxID_ANY, _("Current search path list"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPathlist->Wrap( -1 );
	bSizerLower->Add( m_staticTextPathlist, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultLibraryPathslistBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_DefaultLibraryPathslistBox->SetToolTip( _("System and user paths used to search and load library files and component doc files.\nSorted by decreasing priority order.") );
	
	bSizerLower->Add( m_DefaultLibraryPathslistBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( bSizerLower, 1, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline3, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_cbRescue = new wxCheckBox( this, wxID_ANY, _("Check for cache/library conflicts at schematic load"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_cbRescue, 0, wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCloseWindow ) );
	m_ListLibr->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnFilesListClick ), NULL, this );
	m_ListLibr->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnFilesListClick ), NULL, this );
	m_buttonAddLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonAddPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnOkClick ), NULL, this );
}

DIALOG_EESCHEMA_CONFIG_FBP::~DIALOG_EESCHEMA_CONFIG_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCloseWindow ) );
	m_ListLibr->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnFilesListClick ), NULL, this );
	m_ListLibr->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnFilesListClick ), NULL, this );
	m_buttonAddLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonIns->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonAddPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EESCHEMA_CONFIG_FBP::OnOkClick ), NULL, this );
	
}
