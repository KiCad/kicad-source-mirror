///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_select_pretty_lib_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SELECT_PRETTY_LIB_BASE::DIALOG_SELECT_PRETTY_LIB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_staticText = new wxStaticText( this, wxID_ANY, _("The footprint library is a folder with a name ending with .pretty\nFootprints are .kicad_mod files inside this folder."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText->Wrap( -1 );
	m_staticText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerMain->Add( m_staticText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Path base:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizerMain->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_dirCtrl = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE );
	bSizerMain->Add( m_dirCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SizerNewLibName = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDirname = new wxStaticText( this, wxID_ANY, _("Library folder (.pretty will be added to name, if missing)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDirname->Wrap( -1 );
	m_SizerNewLibName->Add( m_staticTextDirname, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_libName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SizerNewLibName->Add( m_libName, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerMain->Add( m_SizerNewLibName, 0, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_dirCtrl->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_SELECT_PRETTY_LIB_BASE::OnSelectFolder ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_PRETTY_LIB_BASE::OnOKButton ), NULL, this );
}

DIALOG_SELECT_PRETTY_LIB_BASE::~DIALOG_SELECT_PRETTY_LIB_BASE()
{
	// Disconnect Events
	m_dirCtrl->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( DIALOG_SELECT_PRETTY_LIB_BASE::OnSelectFolder ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SELECT_PRETTY_LIB_BASE::OnOKButton ), NULL, this );
	
}
