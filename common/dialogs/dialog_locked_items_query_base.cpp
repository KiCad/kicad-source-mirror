///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_locked_items_query_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LOCKED_ITEMS_QUERY_BASE::DIALOG_LOCKED_ITEMS_QUERY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 10, 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_icon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_icon, 0, wxALL|wxALIGN_CENTER_VERTICAL, 10 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_messageLine1 = new wxStaticText( this, wxID_ANY, _("The selection contains %d locked items."), wxDefaultPosition, wxDefaultSize, 0 );
	m_messageLine1->Wrap( -1 );
	bSizer4->Add( m_messageLine1, 0, wxALL, 5 );

	m_messageLine2 = new wxStaticText( this, wxID_ANY, _("These items will be skipped unless you override the locks."), wxDefaultPosition, wxDefaultSize, 0 );
	m_messageLine2->Wrap( -1 );
	bSizer4->Add( m_messageLine2, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizer4->Add( bSizer4, 1, wxEXPAND|wxRIGHT, 5 );


	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );

	m_doNotShowBtn = new wxCheckBox( this, wxID_ANY, _("Remember decision for this session."), wxDefaultPosition, wxDefaultSize, 0 );
	m_doNotShowBtn->SetToolTip( _("Remember the option selected for the remainder of this session.\nThis dialog will not be shown again until KiCad is restarted.") );

	fgSizer4->Add( m_doNotShowBtn, 0, wxALL, 5 );


	bSizerMain->Add( fgSizer4, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_overrideBtn = new wxButton( this, wxID_ANY, _("Override Locks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_overrideBtn->SetToolTip( _("Override locks and apply the operation on all the items selected.\nAny locked items will remain locked after the operation is complete.") );

	bButtonSizer->Add( m_overrideBtn, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonSizer->Add( m_sdbSizer, 1, wxBOTTOM|wxTOP, 5 );


	bSizerMain->Add( bButtonSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_LOCKED_ITEMS_QUERY_BASE::OnInitDlg ) );
	m_overrideBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LOCKED_ITEMS_QUERY_BASE::onOverrideLocks ), NULL, this );
}

DIALOG_LOCKED_ITEMS_QUERY_BASE::~DIALOG_LOCKED_ITEMS_QUERY_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_LOCKED_ITEMS_QUERY_BASE::OnInitDlg ) );
	m_overrideBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LOCKED_ITEMS_QUERY_BASE::onOverrideLocks ), NULL, this );

}
