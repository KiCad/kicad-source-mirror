///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_shim.h"

#include "dialog_track_via_size_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_SIZE_BASE::DIALOG_TRACK_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizes;
	bSizes = new wxBoxSizer( wxVERTICAL );
	
	m_trackWidth = new WX_UNIT_TEXT( this, _("Track width:") );
	bSizes->Add( m_trackWidth, 0, wxALL, 5 );
	
	m_viaDiameter = new WX_UNIT_TEXT( this, _("Via diameter:") );
	bSizes->Add( m_viaDiameter, 0, wxALL, 5 );
	
	m_viaDrill = new WX_UNIT_TEXT( this, _("Via drill:") );
	bSizes->Add( m_viaDrill, 0, wxALL, 5 );
	
	wxBoxSizer* bButtons;
	bButtons = new wxBoxSizer( wxHORIZONTAL );
	
	m_ok = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_ok, 1, wxALL, 5 );
	
	m_cancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtons->Add( m_cancel, 1, wxALL, 5 );
	
	
	bSizes->Add( bButtons, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizes );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onClose ) );
	m_ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onOkClick ), NULL, this );
	m_cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onCancelClick ), NULL, this );
}

DIALOG_TRACK_VIA_SIZE_BASE::~DIALOG_TRACK_VIA_SIZE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onClose ) );
	m_ok->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onOkClick ), NULL, this );
	m_cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE_BASE::onCancelClick ), NULL, this );
	
}
