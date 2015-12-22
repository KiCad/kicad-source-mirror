///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
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
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextRef = new wxStaticText( this, wxID_ANY, _("Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	fgSizer1->Add( m_staticTextRef, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SearchTextCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	m_SearchTextCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_SearchTextCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextRef1 = new wxStaticText( this, wxID_ANY, _("Available:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef1->Wrap( -1 );
	fgSizer1->Add( m_staticTextRef1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxArrayString m_choiceFpListChoices;
	m_choiceFpList = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFpListChoices, 0 );
	m_choiceFpList->SetSelection( 0 );
	fgSizer1->Add( m_choiceFpList, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerUpper->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::onClose ) );
	m_choiceFpList->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSelectFootprint ), NULL, this );
}

DIALOG_GET_FOOTPRINT_BY_NAME_BASE::~DIALOG_GET_FOOTPRINT_BY_NAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::onClose ) );
	m_choiceFpList->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_GET_FOOTPRINT_BY_NAME_BASE::OnSelectFootprint ), NULL, this );
	
}
