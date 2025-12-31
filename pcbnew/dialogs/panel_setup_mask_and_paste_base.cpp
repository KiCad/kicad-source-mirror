///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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

	m_bitmapWarning = new wxStaticBitmap( this, wxID_ANY, wxArtProvider::GetBitmap( wxASCII_STR(wxART_WARNING), wxASCII_STR(wxART_OTHER) ), wxDefaultPosition, wxDefaultSize, 0 );
	bMessages->Add( m_bitmapWarning, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_staticTextInfoMaskMinWidth = new wxStaticText( this, wxID_ANY, _("Consult your PCB manufacturer's specifications for solder mask expansion, web width, and clearance settings."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth, 0, wxEXPAND, 5 );

	m_staticTextInfoMaskMinWidth1 = new wxStaticText( this, wxID_ANY, _("If no specifications are provided, setting these values to zero is recommended."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth1->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth1, 0, wxEXPAND, 5 );


	bMessages->Add( bSizer4, 1, wxEXPAND|wxLEFT, 5 );


	bSizer3->Add( bMessages, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	bSizer3->Add( 0, 5, 0, wxEXPAND, 5 );

	m_stSolderMaskSettings = new wxStaticText( this, wxID_ANY, _("Solder Mask Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSolderMaskSettings->Wrap( -1 );
	m_stSolderMaskSettings->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_stSolderMaskSettings, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer3->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_maskMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder mask expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginLabel->Wrap( -1 );
	m_maskMarginLabel->SetToolTip( _("Global clearance between pads and the solder mask.\nThis value can be superseded by local values for a footprint or a pad.\nPositive clearance means a solder mask opening bigger than the pad (typical for solder mask clearance).") );

	gbSizer1->Add( m_maskMarginLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_maskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginCtrl->SetToolTip( _("Global clearance between pads and the solder mask.\nThis value can be superseded by local values for a footprint or a pad.\nPositive clearance means a solder mask opening bigger than the pad (typical for solder mask clearance).") );

	gbSizer1->Add( m_maskMarginCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginUnits->Wrap( -1 );
	gbSizer1->Add( m_maskMarginUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maskMinWidthLabel = new wxStaticText( this, wxID_ANY, _("Solder mask minimum web width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthLabel->Wrap( -1 );
	m_maskMinWidthLabel->SetToolTip( _("Minimum distance between openings in the solder mask.\nPad openings closer than this distance will be plotted as a single opening.") );

	gbSizer1->Add( m_maskMinWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthCtrl->SetToolTip( _("Minimum distance between openings in the solder mask.\nPad openings closer than this distance will be plotted as a single opening.") );

	gbSizer1->Add( m_maskMinWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_maskMinWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_maskToCopperClearanceLabel = new wxStaticText( this, wxID_ANY, _("Solder mask to copper clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskToCopperClearanceLabel->Wrap( -1 );
	m_maskToCopperClearanceLabel->SetToolTip( _("Minimum distance between a solder mask opening and a copper item with a different net than the solder mask opening's parent.\nDistances smaller than this minimum will create a DRC error.") );

	gbSizer1->Add( m_maskToCopperClearanceLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maskToCopperClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskToCopperClearanceCtrl->SetToolTip( _("Minimum distance between a solder mask opening and a copper item with a different net than the solder mask opening's parent.\nDistances smaller than this minimum will create a DRC error.") );

	gbSizer1->Add( m_maskToCopperClearanceCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_maskToCopperClearanceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskToCopperClearanceUnits->Wrap( -1 );
	gbSizer1->Add( m_maskToCopperClearanceUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_allowBridges = new wxCheckBox( this, wxID_ANY, _("Allow bridged solder mask apertures between pads within footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_allowBridges->SetToolTip( _("Disable DRC error checking for solder mask aperture bridging between pads in the same footprint.") );

	gbSizer1->Add( m_allowBridges, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxTOP|wxLEFT, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_stTenting = new wxStaticText( this, wxID_ANY, _("Tent vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTenting->Wrap( -1 );
	bSizer6->Add( m_stTenting, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_tentViasFront = new wxCheckBox( this, wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tentViasFront->SetToolTip( _("Tented: vias are covered with solder mask.\nNot tented: vias are not covered with solder mask.") );

	bSizer6->Add( m_tentViasFront, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_tentViasBack = new wxCheckBox( this, wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tentViasBack->SetToolTip( _("Tented: vias are covered with solder mask.\nNot tented: vias are not covered with solder mask.") );

	bSizer6->Add( m_tentViasBack, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	gbSizer1->Add( bSizer6, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bSizer3->Add( gbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizer3->Add( 0, 10, 0, wxEXPAND, 5 );

	m_stSolderPasteSettings = new wxStaticText( this, wxID_ANY, _("Solder Paste Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSolderPasteSettings->Wrap( -1 );
	m_stSolderPasteSettings->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_stSolderPasteSettings, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer3->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 3, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pasteMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginLabel->Wrap( -1 );
	m_pasteMarginLabel->SetToolTip( _("Solder paste clearance relative to pad size.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nThis value can be superseded by local values for a footprint or a pad.") );

	gbSizer2->Add( m_pasteMarginLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginCtrl->SetToolTip( _("Solder paste clearance relative to pad size.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nThis value can be superseded by local values for a footprint or a pad.") );

	gbSizer2->Add( m_pasteMarginCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginUnits->Wrap( -1 );
	gbSizer2->Add( m_pasteMarginUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginRatioLabel = new wxStaticText( this, wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioLabel->Wrap( -1 );
	m_pasteMarginRatioLabel->SetToolTip( _("Global clearance ratio between pads and the solder paste as a percentage of the pad size.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the absolute clearance value.") );

	gbSizer2->Add( m_pasteMarginRatioLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioCtrl->SetToolTip( _("Global clearance ratio between pads and the solder paste as a percentage of the pad size.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the absolute clearance value.") );

	gbSizer2->Add( m_pasteMarginRatioCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginRatioUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioUnits->Wrap( -1 );
	gbSizer2->Add( m_pasteMarginRatioUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	bSizer3->Add( gbSizer2, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizer3->Add( 0, 0, 2, wxEXPAND, 5 );

	m_staticTextInfoPaste = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	bSizer3->Add( m_staticTextInfoPaste, 0, wxALL|wxEXPAND, 5 );


	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );


	bMainSizer->Add( bSizer3, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_MASK_AND_PASTE_BASE::~PANEL_SETUP_MASK_AND_PASTE_BASE()
{
}
