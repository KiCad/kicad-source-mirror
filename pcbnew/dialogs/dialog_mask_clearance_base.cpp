///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_mask_clearance_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_PADS_MASK_CLEARANCE_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_CANCEL, DIALOG_PADS_MASK_CLEARANCE_BASE::_wxFB_OnButtonCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_PADS_MASK_CLEARANCE_BASE::_wxFB_OnButtonOkClick )
END_EVENT_TABLE()

DIALOG_PADS_MASK_CLEARANCE_BASE::DIALOG_PADS_MASK_CLEARANCE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bMainUpperSizer;
	bMainUpperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Note: For clearance values:\n- a positive value means a mask bigger than a pad\n- a negative value means a mask smaller than a pad\n"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	bMainUpperSizer->Add( m_staticTextInfo, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainUpperSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxFlexGridSizer* fgGridSolderMaskSizer;
	fgGridSolderMaskSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGridSolderMaskSizer->AddGrowableCol( 1 );
	fgGridSolderMaskSizer->SetFlexibleDirection( wxBOTH );
	fgGridSolderMaskSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_MaskClearanceTitle = new wxStaticText( this, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	m_MaskClearanceTitle->SetToolTip( _("This is the global clearance between pads and the solder mask\nThis value can be superseded by local values for a footprint or a pad.") );
	
	fgGridSolderMaskSizer->Add( m_MaskClearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetMaxLength( 0 ); 
	fgGridSolderMaskSizer->Add( m_SolderMaskMarginCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderMaskMarginUnits, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextMinWidth = new wxStaticText( this, wxID_ANY, _("Solder mask min width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMinWidth->Wrap( -1 );
	m_staticTextMinWidth->SetToolTip( _("Min dist between 2 pad areas.\nTwo pad areas nearer than this value will be merged during plotting.\nThis parameter is used only to plot solder mask layers.") );
	
	fgGridSolderMaskSizer->Add( m_staticTextMinWidth, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_SolderMaskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMinWidthCtrl->SetMaxLength( 0 ); 
	fgGridSolderMaskSizer->Add( m_SolderMaskMinWidthCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_solderMaskMinWidthUnit = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMinWidthUnit->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_solderMaskMinWidthUnit, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgGridSolderMaskSizer->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgGridSolderMaskSizer->Add( m_staticline4, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgGridSolderMaskSizer->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( this, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	m_staticTextSolderPaste->SetToolTip( _("This is the global clearance between pads and the solder paste\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value ratio") );
	
	fgGridSolderMaskSizer->Add( m_staticTextSolderPaste, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetMaxLength( 0 ); 
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( this, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginUnits, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextRatio = new wxStaticText( this, wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the global clearance ratio in per cent between pads and the solder paste\nA value of 10 means the clearance value is 10 per cent of the pad size\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value") );
	
	fgGridSolderMaskSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetMaxLength( 0 ); 
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bMainUpperSizer->Add( fgGridSolderMaskSizer, 1, wxEXPAND, 5 );
	
	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainUpperSizer->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );
	
	
	bMainSizer->Add( bMainUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbButtonsSizer = new wxStdDialogButtonSizer();
	m_sdbButtonsSizerOK = new wxButton( this, wxID_OK );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerOK );
	m_sdbButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerCancel );
	m_sdbButtonsSizer->Realize();
	
	bMainSizer->Add( m_sdbButtonsSizer, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
}

DIALOG_PADS_MASK_CLEARANCE_BASE::~DIALOG_PADS_MASK_CLEARANCE_BASE()
{
}
