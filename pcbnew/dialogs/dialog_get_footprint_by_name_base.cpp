///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_get_footprint_by_name_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GET_FOOTPRINT_BY_NAME_BASE::DIALOG_GET_FOOTPRINT_BY_NAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,12 ) );

	m_staticTextRef = new wxStaticText( this, wxID_ANY, _("Reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	gbSizer1->Add( m_staticTextRef, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_SearchTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	gbSizer1->Add( m_SearchTextCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_multipleHint = new wxStaticText( this, wxID_ANY, _("(specify multiple items separated by spaces for successive placement)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_multipleHint->Wrap( -1 );
	gbSizer1->Add( m_multipleHint, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextRef1 = new wxStaticText( this, wxID_ANY, _("Available footprints:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef1->Wrap( -1 );
	gbSizer1->Add( m_staticTextRef1, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_choiceFpListChoices;
	m_choiceFpList = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFpListChoices, 0 );
	m_choiceFpList->SetSelection( 0 );
	gbSizer1->Add( m_choiceFpList, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizerUpper->Add( gbSizer1, 1, wxEXPAND|wxALL, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::onClose ) );
	m_SearchTextCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSearchInputChanged ), NULL, this );
	m_choiceFpList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSelectFootprint ), NULL, this );
}

DIALOG_GET_FOOTPRINT_BY_NAME_BASE::~DIALOG_GET_FOOTPRINT_BY_NAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::onClose ) );
	m_SearchTextCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSearchInputChanged ), NULL, this );
	m_choiceFpList->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSelectFootprint ), NULL, this );

}
