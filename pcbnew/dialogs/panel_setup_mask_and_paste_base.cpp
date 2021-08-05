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
	m_staticTextInfoMaskMinWidth->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer4->Add( m_staticTextInfoMaskMinWidth, 0, wxEXPAND, 1 );

	m_staticTextInfoMaskMinWidth1 = new wxStaticText( this, wxID_ANY, _("If none is provided, setting the values to zero is suggested."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoMaskMinWidth1->Wrap( -1 );
	m_staticTextInfoMaskMinWidth1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer4->Add( m_staticTextInfoMaskMinWidth1, 0, wxEXPAND|wxTOP, 1 );


	bMessages->Add( bSizer4, 1, wxEXPAND|wxLEFT, 5 );


	bSizer3->Add( bMessages, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer3->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxFlexGridSizer* fgGridSolderMaskSizer;
	fgGridSolderMaskSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGridSolderMaskSizer->SetFlexibleDirection( wxBOTH );
	fgGridSolderMaskSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_MaskMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMarginLabel->Wrap( -1 );
	m_MaskMarginLabel->SetToolTip( _("Global clearance between pads and the solder mask.\nThis value can be superseded by local values for a footprint or a pad.") );

	fgGridSolderMaskSizer->Add( m_MaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_MaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMarginCtrl->SetToolTip( _("Positive clearance means area bigger than the pad (usual for solder mask clearance).") );

	fgGridSolderMaskSizer->Add( m_MaskMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_MaskMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_MaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MaskMinWidthLabel = new wxStaticText( this, wxID_ANY, _("Solder mask minimum bridge width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMinWidthLabel->Wrap( -1 );
	m_MaskMinWidthLabel->SetToolTip( _("Min. dist between 2 pad areas.\nTwo pad areas nearer than this value will be merged during plotting.\nThis parameter is only used to plot solder mask layers.\nLeave at 0 unless you know what you are doing.") );

	fgGridSolderMaskSizer->Add( m_MaskMinWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_MaskMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMinWidthCtrl->SetToolTip( _("Minimum distance between openings in the solder mask.  Pad openings closer than this distance will be plotted as a single opening.") );

	fgGridSolderMaskSizer->Add( m_MaskMinWidthCtrl, 0, wxEXPAND|wxALL, 5 );

	m_MaskMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskMinWidthUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_MaskMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	fgGridSolderMaskSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_PasteMarginLabel = new wxStaticText( this, wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginLabel->Wrap( -1 );
	m_PasteMarginLabel->SetToolTip( _("Global clearance between pads and the solder paste.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value ratio.") );

	fgGridSolderMaskSizer->Add( m_PasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_PasteMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginCtrl->SetToolTip( _("Negative clearance means area smaller than the pad (usual for solder paste clearance).") );

	fgGridSolderMaskSizer->Add( m_PasteMarginCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_PasteMarginUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PasteMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_PasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextRatio = new wxStaticText( this, wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("Global clearance ratio in percent between pads and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nThis value can be superseded by local values for a footprint or a pad.\nFinal clearance value is the sum of this value and the clearance value.") );

	fgGridSolderMaskSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetToolTip( _("Additional clearance as a percentage of the pad size.") );

	fgGridSolderMaskSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxEXPAND|wxALL, 5 );

	m_SolderPasteRatioMarginUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgGridSolderMaskSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( fgGridSolderMaskSizer, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer3->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

	m_staticTextInfoPaste = new wxStaticText( this, wxID_ANY, _("Note: Solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	m_staticTextInfoPaste->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer3->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( bSizer3, 1, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
}

PANEL_SETUP_MASK_AND_PASTE_BASE::~PANEL_SETUP_MASK_AND_PASTE_BASE()
{
}
