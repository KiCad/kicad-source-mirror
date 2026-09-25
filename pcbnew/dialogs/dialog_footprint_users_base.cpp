///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_footprint_users_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FOOTPRINT_USERS_BASE::DIALOG_FOOTPRINT_USERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerMatchType;
	sbSizerMatchType = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Match type") ), wxVERTICAL );

	m_cbShowFilterMatches = new wxCheckBox( sbSizerMatchType->GetStaticBox(), wxID_ANY, _("Show footprint filter matches"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerMatchType->Add( m_cbShowFilterMatches, 0, wxLEFT|wxTOP, 5 );

	m_cbShowFootprintFieldMatches = new wxCheckBox( sbSizerMatchType->GetStaticBox(), wxID_ANY, _("Show footprint field matches"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerMatchType->Add( m_cbShowFootprintFieldMatches, 0, wxLEFT, 5 );

	m_cbShowPinMapMatches = new wxCheckBox( sbSizerMatchType->GetStaticBox(), wxID_ANY, _("Show pin map matches"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerMatchType->Add( m_cbShowPinMapMatches, 0, wxBOTTOM|wxLEFT, 5 );


	bMainSizer->Add( sbSizerMatchType, 0, wxALL|wxEXPAND, 5 );

	m_statusText = new wxStaticText( this, wxID_ANY, _("This footprint is referenced by %zu symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusText->Wrap( -1 );
	bMainSizer->Add( m_statusText, 0, wxLEFT|wxRIGHT, 5 );

	m_footprintUserList = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES );
	m_footprintUserList->SetMinSize( wxSize( -1,100 ) );

	bMainSizer->Add( m_footprintUserList, 1, wxALL|wxEXPAND, 5 );

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
}

DIALOG_FOOTPRINT_USERS_BASE::~DIALOG_FOOTPRINT_USERS_BASE()
{
}
