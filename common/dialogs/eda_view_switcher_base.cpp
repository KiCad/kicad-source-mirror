///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "eda_view_switcher_base.h"

///////////////////////////////////////////////////////////////////////////

EDA_VIEW_SWITCHER_BASE::EDA_VIEW_SWITCHER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_stTitle = new wxStaticText( this, wxID_ANY, _("View Preset Switcher"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTitle->Wrap( -1 );
	m_stTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizerMain->Add( m_stTitle, 0, wxALL|wxEXPAND, 5 );

	m_listBox = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxBORDER_NONE );
	bSizerMain->Add( m_listBox, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

EDA_VIEW_SWITCHER_BASE::~EDA_VIEW_SWITCHER_BASE()
{
}
