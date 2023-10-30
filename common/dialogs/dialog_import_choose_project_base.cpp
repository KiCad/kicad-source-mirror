///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_import_choose_project_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_CHOOSE_PROJECT_BASE::DIALOG_IMPORT_CHOOSE_PROJECT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_titleText = new wxStaticText( this, wxID_ANY, _("This project file contains multiple PCB+Schematic combinations.\nChoose which one should be imported to KiCad."), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleText->Wrap( -1 );
	bSizerMain->Add( m_titleText, 0, wxALL, 5 );

	m_listCtrl = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	bSizerMain->Add( m_listCtrl, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bSizerBottom->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_IMPORT_CHOOSE_PROJECT_BASE::onClose ) );
	m_listCtrl->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORT_CHOOSE_PROJECT_BASE::onItemActivated ), NULL, this );
}

DIALOG_IMPORT_CHOOSE_PROJECT_BASE::~DIALOG_IMPORT_CHOOSE_PROJECT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_IMPORT_CHOOSE_PROJECT_BASE::onClose ) );
	m_listCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORT_CHOOSE_PROJECT_BASE::onItemActivated ), NULL, this );

}
