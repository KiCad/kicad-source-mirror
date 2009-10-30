///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_mask_clearance_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_PADS_MASK_CLEARANCE_BASE, wxDialog )
	EVT_BUTTON( wxID_CANCEL, DIALOG_PADS_MASK_CLEARANCE_BASE::_wxFB_OnButtonCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_PADS_MASK_CLEARANCE_BASE::_wxFB_OnButtonOkClick )
END_EVENT_TABLE()

DIALOG_PADS_MASK_CLEARANCE_BASE::DIALOG_PADS_MASK_CLEARANCE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bMainUpperSizer;
	bMainUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbMiddleRightSizer;
	sbMiddleRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimensions:") ), wxVERTICAL );
	
	m_MaskClearanceTitle = new wxStaticText( this, wxID_ANY, _("Pads Mask Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	sbMiddleRightSizer->Add( m_MaskClearanceTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptMaskMargin = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptMaskMargin->SetToolTip( _("This is the clearance between pads and the mask") );
	
	sbMiddleRightSizer->Add( m_OptMaskMargin, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainUpperSizer->Add( sbMiddleRightSizer, 1, wxEXPAND, 5 );
	
	bMainSizer->Add( bMainUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbButtonsSizer = new wxStdDialogButtonSizer();
	m_sdbButtonsSizerOK = new wxButton( this, wxID_OK );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerOK );
	m_sdbButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerCancel );
	m_sdbButtonsSizer->Realize();
	bMainSizer->Add( m_sdbButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
}

DIALOG_PADS_MASK_CLEARANCE_BASE::~DIALOG_PADS_MASK_CLEARANCE_BASE()
{
}
