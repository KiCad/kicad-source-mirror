///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_unit_entry_base.h"

///////////////////////////////////////////////////////////////////////////

WX_UNIT_ENTRY_DIALOG_BASE::WX_UNIT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerContent;
	bSizerContent = new wxBoxSizer( wxHORIZONTAL );

	m_label = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_label->Wrap( -1 );
	bSizerContent->Add( m_label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_textCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerContent->Add( m_textCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_unit_label = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unit_label->Wrap( -1 );
	bSizerContent->Add( m_unit_label, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerContent, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );


	bSizerButtons->Add( 100, 0, 1, 0, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerButtons->Add( m_sdbSizer1, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

WX_UNIT_ENTRY_DIALOG_BASE::~WX_UNIT_ENTRY_DIALOG_BASE()
{
}

WX_PT_ENTRY_DIALOG_BASE::WX_PT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelX = new wxStaticText( this, wxID_ANY, _("X label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelX->Wrap( -1 );
	fgSizer->Add( m_labelX, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textCtrlX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_textCtrlX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_unitsX = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsX->Wrap( -1 );
	fgSizer->Add( m_unitsX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_labelY = new wxStaticText( this, wxID_ANY, _("Y label:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelY->Wrap( -1 );
	fgSizer->Add( m_labelY, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_textCtrlY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_textCtrlY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_unitsY = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsY->Wrap( -1 );
	fgSizer->Add( m_unitsY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_ButtonReset = new wxButton( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_ButtonReset, 0, wxALL|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );


	bSizerButtons->Add( 30, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerButtons->Add( m_sdbSizer1, 0, wxALL, 5 );


	bSizerMain->Add( bSizerButtons, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_ButtonReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_PT_ENTRY_DIALOG_BASE::ResetValues ), NULL, this );
}

WX_PT_ENTRY_DIALOG_BASE::~WX_PT_ENTRY_DIALOG_BASE()
{
	// Disconnect Events
	m_ButtonReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WX_PT_ENTRY_DIALOG_BASE::ResetValues ), NULL, this );

}
