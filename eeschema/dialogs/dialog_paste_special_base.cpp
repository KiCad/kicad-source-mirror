///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_paste_special_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PASTE_SPECIAL_BASE::DIALOG_PASTE_SPECIAL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* optionsSizer;
	optionsSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Paste Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	optionsSizer->Add( m_staticText7, 0, wxALL, 5 );

	m_keepAnnotations = new wxCheckBox( this, wxID_ANY, _("Keep existing annotations, even if they are duplicated"), wxDefaultPosition, wxDefaultSize, 0 );
	optionsSizer->Add( m_keepAnnotations, 0, wxALL, 5 );

	m_dropAnnotations = new wxCheckBox( this, wxID_ANY, _("Clear annotations on pasted items"), wxDefaultPosition, wxDefaultSize, 0 );
	optionsSizer->Add( m_dropAnnotations, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_mainSizer->Add( optionsSizer, 1, wxALL|wxEXPAND, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 6 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_keepAnnotations->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::OnKeepAnnotations ), NULL, this );
	m_dropAnnotations->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::OnDropAnnotations ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::onOKButton ), NULL, this );
}

DIALOG_PASTE_SPECIAL_BASE::~DIALOG_PASTE_SPECIAL_BASE()
{
	// Disconnect Events
	m_keepAnnotations->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::OnKeepAnnotations ), NULL, this );
	m_dropAnnotations->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::OnDropAnnotations ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PASTE_SPECIAL_BASE::onOKButton ), NULL, this );

}
