///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_rescue_summary_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RESCUE_SUMMARY_BASE::DIALOG_RESCUE_SUMMARY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("The symbols of the following components were changed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bupperSizer->Add( m_staticText5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ListOfChanges = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_ListOfChanges->SetMinSize( wxSize( -1,200 ) );
	
	bupperSizer->Add( m_ListOfChanges, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();
	
	bupperSizer->Add( m_sdbSizer, 0, wxEXPAND, 5 );
	
	
	bmainSizer->Add( bupperSizer, 1, wxALL|wxEXPAND, 6 );
	
	
	this->SetSizer( bmainSizer );
	this->Layout();
}

DIALOG_RESCUE_SUMMARY_BASE::~DIALOG_RESCUE_SUMMARY_BASE()
{
}
