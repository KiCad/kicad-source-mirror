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
	bMessages = new wxBoxSizer( wxHORIZONTAL );


	bMessages->Add( 4, 0, 0, wxEXPAND, 5 );

	m_bitmapWarning = new wxStaticBitmap( this, wxID_ANY, wxArtProvider::GetBitmap( wxART_WARNING, wxART_OTHER ), wxDefaultPosition, wxDefaultSize, 0 );
	bMessages->Add( m_bitmapWarning, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_staticTextInfoMaskMinWidth = new wxStaticText( this, wxID_ANY, _("Use your board house's recommendation for solder mask clearance and minimum bridge width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth, 0, wxEXPAND, 1 );

	m_staticTextInfoMaskMinWidth1 = new wxStaticText( this, wxID_ANY, _("If none is provided, setting the values to zero is suggested."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth1->Wrap( -1 );
	bSizer4->Add( m_staticTextInfoMaskMinWidth1, 0, wxEXPAND|wxTOP, 1 );


	bMessages->Add( bSizer4, 1, wxEXPAND|wxLEFT, 5 );


	bSizer3->Add( bMessages, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer3->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgGridSolderMaskSizer;
	fgGridSolderMaskSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGridSolderMaskSizer->SetFlexibleDirection( wxBOTH );
	fgGridSolderMaskSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_maskMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginLabel->Wrap( -1 );
	m_maskMarginLabel->SetToolTip( _("Global clearance between pads and the solder mask.\nThis value can be superseded by local values for a footprint or a pad.") );

	fgGridSolderMaskSizer->Add( m_maskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_maskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginCtrl->SetToolTip( _("Positive clearance means area bigger than the pad (usual for solder mask clearance).") );

	fgGridSolderMaskSizer->Add( m_maskMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_maskMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_maskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_maskMinWidthLabel = new wxStaticText( this, wxID_ANY, _("Solder mask minimum web width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthLabel->Wrap( -1 );
	m_maskMinWidthLabel->SetToolTip( _("Min. dist between 2 pad areas.\nTwo pad areas nearer than this value will be merged during plotting.\nThis parameter is only used to plot solder mask layers.\nLeave at 0 unless you know what you are doing.") );

	fgGridSolderMaskSizer->Add( m_maskMinWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_maskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthCtrl->SetToolTip( _("Minimum distance between openings in the solder mask.  Pad openings closer than this distance will be plotted as a single opening.") );

	fgGridSolderMaskSizer->Add( m_maskMinWidthCtrl, 0, wxEXPAND|wxALL, 5 );

	m_maskMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMinWidthUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_maskMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_pasteMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginLabel->Wrap( -1 );
	m_pasteMarginLabel->SetToolTip( _("Global clearance between pads and the solder paste.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value ratio.") );

	fgGridSolderMaskSizer->Add( m_pasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginCtrl->SetToolTip( _("Negative clearance means area smaller than the pad (usual for solder paste clearance).") );

	fgGridSolderMaskSizer->Add( m_pasteMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_pasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_pasteMarginRatioLabel = new wxStaticText( this, wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioLabel->Wrap( -1 );
	m_pasteMarginRatioLabel->SetToolTip( _("Global clearance ratio in percent between pads and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value.") );

	fgGridSolderMaskSizer->Add( m_pasteMarginRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_pasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioCtrl->SetToolTip( _("Additional clearance as a percentage of the pad size.") );

	fgGridSolderMaskSizer->Add( m_pasteMarginRatioCtrl, 0, wxEXPAND|wxALL, 5 );

	m_pasteMarginRatioUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_pasteMarginRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( fgGridSolderMaskSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticTextInfoPaste = new wxStaticText( this, wxID_ANY, _("Note: Solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	bSizer3->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( bSizer3, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
}

PANEL_SETUP_MASK_AND_PASTE_BASE::~PANEL_SETUP_MASK_AND_PASTE_BASE()
{
}
