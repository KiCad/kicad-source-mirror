///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_select_dirlist_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SELECT_DIRLIST_BASE::DIALOG_SELECT_DIRLIST_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,300 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_staticText = new wxStaticText( this, wxID_ANY, _("The footprint library is a folder.\nFootprints are files inside this folder."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	m_staticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerMain->Add( m_staticText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_dirCtrl = new wxGenericDirCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDIRCTRL_3D_INTERNAL|wxDIRCTRL_DIR_ONLY|wxDIRCTRL_MULTIPLE|wxDIRCTRL_SHOW_FILTERS|wxSUNKEN_BORDER, _("*.pretty|*"), 0 );
	
	m_dirCtrl->ShowHidden( false );
	m_dirCtrl->SetMinSize( wxSize( 400,350 ) );
	
	bSizerMain->Add( m_dirCtrl, 1, wxEXPAND | wxALL, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_dirCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnSelectFolder ), NULL, this );
	m_dirCtrl->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnSelectFolder ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnOKClick ), NULL, this );
}

DIALOG_SELECT_DIRLIST_BASE::~DIALOG_SELECT_DIRLIST_BASE()
{
	// Disconnect Events
	m_dirCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnSelectFolder ), NULL, this );
	m_dirCtrl->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnSelectFolder ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_DIRLIST_BASE::OnOKClick ), NULL, this );
	
}
