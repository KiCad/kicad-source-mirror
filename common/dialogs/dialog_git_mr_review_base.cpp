///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_git_mr_review_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GIT_MR_REVIEW_BASE::DIALOG_GIT_MR_REVIEW_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 700,500 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgRefSizer;
	fgRefSizer = new wxFlexGridSizer( 2, 2, 3, 5 );
	fgRefSizer->AddGrowableCol( 1 );
	fgRefSizer->SetFlexibleDirection( wxBOTH );
	fgRefSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_labelBase = new wxStaticText( this, wxID_ANY, _("Base ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelBase->Wrap( -1 );
	fgRefSizer->Add( m_labelBase, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_comboBase = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgRefSizer->Add( m_comboBase, 1, wxEXPAND|wxALL, 5 );

	m_labelHead = new wxStaticText( this, wxID_ANY, _("Head ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelHead->Wrap( -1 );
	fgRefSizer->Add( m_labelHead, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_comboHead = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgRefSizer->Add( m_comboHead, 1, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( fgRefSizer, 0, wxEXPAND|wxALL, 5 );

	m_buttonCompare = new wxButton( this, wxID_ANY, _("Compare"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_buttonCompare, 0, wxALIGN_RIGHT|wxALL, 5 );

	m_separator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_separator, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_labelChanged = new wxStaticText( this, wxID_ANY, _("Changed files (double-click to open):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelChanged->Wrap( -1 );
	bMainSizer->Add( m_labelChanged, 0, wxALL|wxEXPAND, 5 );

	m_listFiles = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_HRULES|wxLC_VRULES|wxLC_SINGLE_SEL );
	m_listFiles->SetMinSize( wxSize( 600,250 ) );

	bMainSizer->Add( m_listFiles, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnClose ) );
	m_buttonCompare->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnCompareClick ), NULL, this );
	m_listFiles->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnFileActivated ), NULL, this );
}

DIALOG_GIT_MR_REVIEW_BASE::~DIALOG_GIT_MR_REVIEW_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnClose ) );
	m_buttonCompare->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnCompareClick ), NULL, this );
	m_listFiles->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_GIT_MR_REVIEW_BASE::OnFileActivated ), NULL, this );

}
