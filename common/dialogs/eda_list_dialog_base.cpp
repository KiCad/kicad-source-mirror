///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "eda_list_dialog_base.h"

///////////////////////////////////////////////////////////////////////////

EDA_LIST_DIALOG_BASE::EDA_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_listLabel = new wxStaticText( this, wxID_ANY, _("Items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_listLabel->Wrap( -1 );
	bMargins->Add( m_listLabel, 0, wxALL, 5 );

	m_listBox = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES|wxBORDER_SIMPLE|wxVSCROLL );
	m_listBox->SetMinSize( wxSize( 280,150 ) );

	bMargins->Add( m_listBox, 3, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_filterBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_filterBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExtrasSizer = new wxBoxSizer( wxVERTICAL );


	bMargins->Add( m_ExtrasSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_ButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_ButtonsSizer->Add( m_sdbSizer, 1, wxALL, 5 );


	bSizerMain->Add( m_ButtonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_LIST_DIALOG_BASE::onSize ) );
	m_listBox->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemActivated ), NULL, this );
	m_filterBox->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::textChangeInFilterBox ), NULL, this );
}

EDA_LIST_DIALOG_BASE::~EDA_LIST_DIALOG_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( EDA_LIST_DIALOG_BASE::onSize ) );
	m_listBox->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( EDA_LIST_DIALOG_BASE::onListItemActivated ), NULL, this );
	m_filterBox->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( EDA_LIST_DIALOG_BASE::textChangeInFilterBox ), NULL, this );

}
