///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_mask_and_paste_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_MASK_AND_PASTE_BASE::PANEL_SETUP_MASK_AND_PASTE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMessages;
	bMessages = new wxBoxSizer( wxVERTICAL );

	m_staticTextInfoValPos = new wxStaticText( this, wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValPos->Wrap( 500 );
	m_staticTextInfoValPos->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bMessages->Add( m_staticTextInfoValPos, 0, wxBOTTOM, 5 );

	m_staticTextInfoValNeg = new wxStaticText( this, wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoValNeg->Wrap( 500 );
	m_staticTextInfoValNeg->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bMessages->Add( m_staticTextInfoValNeg, 0, wxBOTTOM, 12 );


	bSizer3->Add( bMessages, 0, wxEXPAND|wxALL, 5 );

	wxFlexGridSizer* fgGridSolderMaskSizer;
	fgGridSolderMaskSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGridSolderMaskSizer->AddGrowableCol( 1 );
	fgGridSolderMaskSizer->SetFlexibleDirection( wxBOTH );
	fgGridSolderMaskSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_MaskMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMarginLabel->Wrap( -1 );
	m_MaskMarginLabel->SetToolTip( _("This is the global clearance between pads and the solder mask\nThis value can be superseded by local values for a footprint or a pad.") );

	fgGridSolderMaskSizer->Add( m_MaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_MaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridSolderMaskSizer->Add( m_MaskMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_MaskMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_MaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MaskMinWidthLabel = new wxStaticText( this, wxID_ANY, _("Solder mask minimum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMinWidthLabel->Wrap( -1 );
	m_MaskMinWidthLabel->SetToolTip( _("Min dist between 2 pad areas.\nTwo pad areas nearer than this value will be merged during plotting.\nThis parameter is used only to plot solder mask layers.") );

	fgGridSolderMaskSizer->Add( m_MaskMinWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_MaskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridSolderMaskSizer->Add( m_MaskMinWidthCtrl, 0, wxEXPAND|wxALL, 5 );

	m_MaskMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMinWidthUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_MaskMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_PasteMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginLabel->Wrap( -1 );
	m_PasteMarginLabel->SetToolTip( _("This is the global clearance between pads and the solder paste\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value ratio") );

	fgGridSolderMaskSizer->Add( m_PasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_PasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridSolderMaskSizer->Add( m_PasteMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_PasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextRatio = new wxStaticText( this, wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the global clearance ratio in per cent between pads and the solder paste\nA value of 10 means the clearance value is 10 per cent of the pad size\nThis value can be superseded by local values for a footprint or a pad.\nThe final clearance value is the sum of this value and the clearance value") );

	fgGridSolderMaskSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridSolderMaskSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxEXPAND|wxALL, 5 );

	m_SolderPasteRatioMarginUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( fgGridSolderMaskSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticTextInfoCopper = new wxStaticText( this, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoCopper->Wrap( -1 );
	m_staticTextInfoCopper->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticTextInfoCopper, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextInfoPaste = new wxStaticText( this, wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	m_staticTextInfoPaste->SetFont( wxFont( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizer3, 1, wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_MASK_AND_PASTE_BASE::~PANEL_SETUP_MASK_AND_PASTE_BASE()
{
}
