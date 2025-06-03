///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sync_sheet_pins_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SYNC_SHEET_PINS_BASE::DIALOG_SYNC_SHEET_PINS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_sizerMain = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	m_sizerMain->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );


	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_labelTip = new wxStaticText( this, wxID_ANY, _("Changes made in this dialog occur immediately, use Undo in each affected document to undo them"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTip->Wrap( -1 );
	bSizer8->Add( m_labelTip, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );

	m_btnClose->SetDefault();
	bSizer8->Add( m_btnClose, 0, wxALL, 5 );


	m_sizerMain->Add( bSizer8, 0, wxEXPAND, 5 );


	this->SetSizer( m_sizerMain );
	this->Layout();
	m_sizerMain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_SYNC_SHEET_PINS_BASE::~DIALOG_SYNC_SHEET_PINS_BASE()
{
}
