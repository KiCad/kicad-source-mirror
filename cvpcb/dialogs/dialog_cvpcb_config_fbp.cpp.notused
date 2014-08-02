///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_cvpcb_config_fbp.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CVPCB_CONFIG_FBP::DIALOG_CVPCB_CONFIG_FBP( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbLibsChoiceSizer;
	sbLibsChoiceSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprint library files") ), wxHORIZONTAL );
	
	m_ListLibr = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListLibr->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Pcbnew.\nThe order of this list is important:\nPcbnew searchs for a given footprint using this list order priority.") );
	m_ListLibr->SetMinSize( wxSize( 450,90 ) );
	
	sbLibsChoiceSizer->Add( m_ListLibr, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerLibButtons;
	bSizerLibButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddLib = new wxButton( this, ID_ADD_LIB, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddLib->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bSizerLibButtons->Add( m_buttonAddLib, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT, 5 );
	
	m_buttonInsLib = new wxButton( this, ID_INSERT_LIB, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonInsLib->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bSizerLibButtons->Add( m_buttonInsLib, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT, 5 );
	
	m_buttonRemoveLib = new wxButton( this, ID_REMOVE_LIB, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveLib->SetToolTip( _("Unload the selected library") );
	
	bSizerLibButtons->Add( m_buttonRemoveLib, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT, 5 );
	
	m_buttonLibUp = new wxButton( this, ID_LIB_UP, _("Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLibButtons->Add( m_buttonLibUp, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonLibDown = new wxButton( this, ID_LIB_DOWN, _("Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLibButtons->Add( m_buttonLibDown, 0, wxRIGHT|wxLEFT, 5 );
	
	
	sbLibsChoiceSizer->Add( bSizerLibButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( sbLibsChoiceSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbEquivChoiceSizer;
	sbEquivChoiceSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprint alias files") ), wxHORIZONTAL );
	
	m_ListEquiv = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SINGLE ); 
	m_ListEquiv->SetToolTip( _("List of active library files.\nOnly library files in this list are loaded by Pcbnew.\nThe order of this list is important:\nPcbnew searchs for a given footprint using this list order priority.") );
	m_ListEquiv->SetMinSize( wxSize( 400,-1 ) );
	
	sbEquivChoiceSizer->Add( m_ListEquiv, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerEquButtons;
	bSizerEquButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddEqu = new wxButton( this, ID_ADD_EQU, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonAddEqu->SetToolTip( _("Add a new library after the selected library, and load it") );
	
	bSizerEquButtons->Add( m_buttonAddEqu, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonInsEqu = new wxButton( this, ID_INSERT_EQU, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonInsEqu->SetToolTip( _("Add a new library before the selected library, and load it") );
	
	bSizerEquButtons->Add( m_buttonInsEqu, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT, 5 );
	
	m_buttonRemoveEqu = new wxButton( this, ID_REMOVE_EQU, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonRemoveEqu->SetToolTip( _("Unload the selected library") );
	
	bSizerEquButtons->Add( m_buttonRemoveEqu, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonEquUp = new wxButton( this, ID_EQU_UP, _("Up"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerEquButtons->Add( m_buttonEquUp, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonEquDown = new wxButton( this, ID_EQU_DOWN, _("Down"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerEquButtons->Add( m_buttonEquDown, 0, wxRIGHT|wxLEFT, 5 );
	
	
	sbEquivChoiceSizer->Add( bSizerEquButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( sbEquivChoiceSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbModulesDocSizer;
	sbModulesDocSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprint documentation file") ), wxHORIZONTAL );
	
	m_TextHelpModulesFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbModulesDocSizer->Add( m_TextHelpModulesFileName, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonModDoc = new wxButton( this, ID_BROWSE_MOD_DOC, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	sbModulesDocSizer->Add( m_buttonModDoc, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( sbModulesDocSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User defined search paths") ), wxHORIZONTAL );
	
	wxBoxSizer* bUserListSizer;
	bUserListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_listUserPaths = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_listUserPaths->SetToolTip( _("Additional paths used in this project. The priority is higher than default KiCad paths.") );
	m_listUserPaths->SetMinSize( wxSize( -1,70 ) );
	
	bUserListSizer->Add( m_listUserPaths, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer4->Add( bUserListSizer, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bUserPathsButtonsSizer;
	bUserPathsButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddPath = new wxButton( this, ID_LIB_PATH_SEL, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonAddPath, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonInsPath = new wxButton( this, ID_INSERT_PATH, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonInsPath, 0, wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemovePath = new wxButton( this, ID_REMOVE_PATH, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bUserPathsButtonsSizer->Add( m_buttonRemovePath, 0, wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer4->Add( bUserPathsButtonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainSizer->Add( sbSizer4, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Current search path list") ), wxHORIZONTAL );
	
	m_DefaultLibraryPathslistBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB ); 
	m_DefaultLibraryPathslistBox->SetToolTip( _("System and user paths used to search and load library files and component doc files.\nSorted by decreasing priority order.") );
	m_DefaultLibraryPathslistBox->SetMinSize( wxSize( -1,70 ) );
	
	sbSizer6->Add( m_DefaultLibraryPathslistBox, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( sbSizer6, 1, wxALL|wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();
	
	bMainSizer->Add( m_sdbSizer2, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnCloseWindow ) );
	m_buttonAddLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonInsLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonLibUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonLibDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonAddEqu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonInsEqu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveEqu->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonEquUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonEquDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonModDoc->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer2Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnOkClick ), NULL, this );
}

DIALOG_CVPCB_CONFIG_FBP::~DIALOG_CVPCB_CONFIG_FBP()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnCloseWindow ) );
	m_buttonAddLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonInsLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonLibUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonLibDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonAddEqu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonInsEqu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertLibClick ), NULL, this );
	m_buttonRemoveEqu->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveLibClick ), NULL, this );
	m_buttonEquUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonUpClick ), NULL, this );
	m_buttonEquDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnButtonDownClick ), NULL, this );
	m_buttonModDoc->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnBrowseModDocFile ), NULL, this );
	m_buttonAddPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonInsPath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnAddOrInsertPath ), NULL, this );
	m_buttonRemovePath->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnRemoveUserPath ), NULL, this );
	m_sdbSizer2Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnCancelClick ), NULL, this );
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CVPCB_CONFIG_FBP::OnOkClick ), NULL, this );
	
}
