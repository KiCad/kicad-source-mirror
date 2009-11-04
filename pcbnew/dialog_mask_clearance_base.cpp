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
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Note:\n- a positive value means a mask bigger than a pad\n- a negative value means a mask smaller than a pad\n"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbMiddleRightSizer->Add( m_staticTextInfo, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbMiddleRightSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxFlexGridSizer* fgGridSolderMaskSizer;
	fgGridSolderMaskSizer = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgGridSolderMaskSizer->SetFlexibleDirection( wxBOTH );
	fgGridSolderMaskSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_MaskClearanceTitle = new wxStaticText( this, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_MaskClearanceTitle, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetToolTip( _("This is the global clearance between pads and the solder mask\nThis value can be superseded by local values for a footprint or a pad.") );
	
	fgGridSolderMaskSizer->Add( m_SolderMaskMarginCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderMaskMarginUnits, 0, wxALL, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( this, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_staticTextSolderPaste, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetToolTip( _("This is the global clearance between pads and the solder paste\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value ratio") );
	
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginCtrl, 0, wxALL, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginUnits, 0, wxALL, 5 );
	
	m_staticTextRatio = new wxStaticText( this, wxID_ANY, _("Solder mask ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_staticTextRatio, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetToolTip( _("This is the global clearance ratio in per cent between pads and the solder paste\nA value of 10 means the clearance value is 10% of the pad size\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value") );
	
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxALL, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALL, 5 );
	
	sbMiddleRightSizer->Add( fgGridSolderMaskSizer, 1, wxEXPAND, 5 );
	
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
