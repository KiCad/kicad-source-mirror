///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_track_via_size_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_SIZE_BASE::DIALOG_TRACK_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 280,240 ), wxDefaultSize );
	
	wxBoxSizer* bSizes;
	bSizes = new wxBoxSizer( wxVERTICAL );
	
	m_trackWidth = new WX_UNIT_TEXT( this, _("Track width:") );
	bSizes->Add( m_trackWidth, 0, wxALL|wxEXPAND, 5 );
	
	m_viaDiameter = new WX_UNIT_TEXT( this, _("Via diameter:") );
	bSizes->Add( m_viaDiameter, 0, wxALL|wxEXPAND, 5 );
	
	m_viaDrill = new WX_UNIT_TEXT( this, _("Via drill:") );
	bSizes->Add( m_viaDrill, 0, wxALL|wxEXPAND, 5 );
	
	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();
	
	bSizes->Add( m_stdButtons, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizes );
	this->Layout();
	bSizes->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onClose ) );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onCancelClick ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onOkClick ), NULL, this );
}

DIALOG_TRACK_VIA_SIZE_BASE::~DIALOG_TRACK_VIA_SIZE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onClose ) );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onCancelClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onOkClick ), NULL, this );
	
}
