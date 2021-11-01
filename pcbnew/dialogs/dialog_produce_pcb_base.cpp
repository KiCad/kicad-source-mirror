///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_produce_pcb_base.h"

///////////////////////////////////////////////////////////////////////////

PRODUCE_PCB_BASE::PRODUCE_PCB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerContent;
	bSizerContent = new wxBoxSizer( wxVERTICAL );

	m_label = new wxStaticText( this, wxID_ANY, _("Select Manufacturer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label->Wrap( -1 );
	bSizerContent->Add( m_label, 0, wxALL|wxEXPAND, 5 );

	m_label1 = new wxStaticText( this, wxID_ANY, _("from %s"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label1->Wrap( -1 );
	bSizerContent->Add( m_label1, 0, wxALL, 5 );

	m_label2 = new wxStaticText( this, wxID_ANY, _("Details: %s"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label2->Wrap( -1 );
	bSizerContent->Add( m_label2, 0, wxALL, 5 );

	m_label3 = new wxStaticText( this, wxID_ANY, _("Send project directly"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label3->Wrap( -1 );
	bSizerContent->Add( m_label3, 0, wxALL, 5 );

	m_label31 = new wxStaticText( this, wxID_ANY, _("Sends production files to manufacturer from KiCad"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label31->Wrap( -1 );
	bSizerContent->Add( m_label31, 0, wxALL, 5 );

	m_label32 = new wxStaticText( this, wxID_ANY, _("Produce PCB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label32->Wrap( -1 );
	bSizerContent->Add( m_label32, 0, wxALL, 5 );


	m_mainSizer->Add( bSizerContent, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

PRODUCE_PCB_BASE::~PRODUCE_PCB_BASE()
{
}
