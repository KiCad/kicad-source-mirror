///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_mdw_release_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MDW_RELEASE_BASE::DIALOG_MDW_RELEASE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bContentSizer;
	bContentSizer = new wxBoxSizer( wxVERTICAL );

	m_intro = new wxStaticText( this, wxID_ANY, _("Release these parts as the revisions shown:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intro->Wrap( -1 );
	bContentSizer->Add( m_intro, 0, wxEXPAND|wxBOTTOM, 5 );

	m_partsList = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_partsList->SetMinSize( wxSize( 560,120 ) );

	bContentSizer->Add( m_partsList, 1, wxEXPAND, 0 );

	m_note = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_note->Wrap( -1 );
	bContentSizer->Add( m_note, 0, wxEXPAND|wxTOP, 5 );

	m_commentLabel = new wxStaticText( this, wxID_ANY, _("Comment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_commentLabel->Wrap( -1 );
	bContentSizer->Add( m_commentLabel, 0, wxTOP, 10 );

	m_comment = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_comment->SetMinSize( wxSize( -1,60 ) );

	bContentSizer->Add( m_comment, 0, wxEXPAND|wxTOP, 2 );


	bMainSizer->Add( bContentSizer, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_MDW_RELEASE_BASE::~DIALOG_MDW_RELEASE_BASE()
{
}
