///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
	bMessages = new wxBoxSizer( wxHORIZONTAL );


	bMessages->Add( 4, 0, 0, wxEXPAND, 5 );

	m_bitmapWarning = new wxStaticBitmap( this, wxID_ANY, wxArtProvider::GetBitmap( wxART_WARNING, wxART_OTHER ), wxDefaultPosition, wxDefaultSize, 0 );
	bMessages->Add( m_bitmapWarning, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_staticTextInfoMaskMinWidth = new wxStaticText( this, wxID_ANY, _("Use your board manufacturer's recommendations for solder mask expansion and minimum web width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth, 0, wxEXPAND, 1 );

	m_staticTextInfoMaskMinWidth1 = new wxStaticText( this, wxID_ANY, _("If none are provided, setting the values to zero is suggested."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth1->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth1, 0, wxEXPAND|wxTOP, 1 );


	bMessages->Add( bSizer4, 1, wxEXPAND|wxLEFT, 5 );


	bSizer3->Add( bMessages, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer3->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_maskMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder mask expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginLabel->Wrap( -1 );
	m_maskMarginLabel->SetToolTip( _("Global clearance between pads and the solder mask.\nThis value can be superseded by local values for a footprint or a pad.") );

	gbSizer1->Add( m_maskMarginLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginCtrl->SetToolTip( _("Positive clearance means area bigger than the pad (usual for solder mask clearance).") );

	gbSizer1->Add( m_maskMarginCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_maskMarginUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maskMinWidthLabel = new wxStaticText( this, wxID_ANY, _("Solder mask minimum web width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthLabel->Wrap( -1 );
	m_maskMinWidthLabel->SetToolTip( _("Min. dist between 2 pad areas.\nTwo pad areas nearer than this value will be merged during plotting.\nThis parameter is only used to plot solder mask layers.\nLeave at 0 unless you know what you are doing.") );

	gbSizer1->Add( m_maskMinWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthCtrl->SetToolTip( _("Minimum distance between openings in the solder mask.  Pad openings closer than this distance will be plotted as a single opening.") );

	gbSizer1->Add( m_maskMinWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_maskMinWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_maskToCopperClearanceLabel = new wxStaticText( this, wxID_ANY, _("Solder mask to copper clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskToCopperClearanceLabel->Wrap( -1 );
	gbSizer1->Add( m_maskToCopperClearanceLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maskToCopperClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_maskToCopperClearanceCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskToCopperClearanceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskToCopperClearanceUnits->Wrap( -1 );
	gbSizer1->Add( m_maskToCopperClearanceUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_allowBridges = new wxCheckBox( this, wxID_ANY, _("Allow bridged solder mask apertures between pads within footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_allowBridges, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxALL, 5 );

	m_tentVias = new wxCheckBox( this, wxID_ANY, _("Tent vias"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tentVias, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginLabel->Wrap( -1 );
	m_pasteMarginLabel->SetToolTip( _("Global clearance between pads and the solder paste.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value ratio.") );

	gbSizer1->Add( m_pasteMarginLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginCtrl->SetToolTip( _("Negative clearance means area smaller than the pad (usual for solder paste clearance).") );

	gbSizer1->Add( m_pasteMarginCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_pasteMarginUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginRatioLabel = new wxStaticText( this, wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioLabel->Wrap( -1 );
	m_pasteMarginRatioLabel->SetToolTip( _("Global clearance ratio in percent between pads and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value.") );

	gbSizer1->Add( m_pasteMarginRatioLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioCtrl->SetToolTip( _("Additional clearance as a percentage of the pad size.") );

	gbSizer1->Add( m_pasteMarginRatioCtrl, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginRatioUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioUnits->Wrap( -1 );
	gbSizer1->Add( m_pasteMarginRatioUnits, wxGBPosition( 7, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer3->Add( gbSizer1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_staticTextInfoPaste = new wxStaticText( this, wxID_ANY, _("Note: Solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	bSizer3->Add( m_staticTextInfoPaste, 0, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizer3, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_MASK_AND_PASTE_BASE::~PANEL_SETUP_MASK_AND_PASTE_BASE()
{
}
