///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_color_picker_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_COLOR_PICKER_BASE::DIALOG_COLOR_PICKER_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpperMain;
	bSizerUpperMain = new wxBoxSizer( wxHORIZONTAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelFreeColors = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerUpperFreeColors;
	bSizerUpperFreeColors = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerPanels;
	bSizerPanels = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerViewRGB;
	sbSizerViewRGB = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, _("RGB") ), wxVERTICAL );

	m_RgbBitmap = new wxStaticBitmap( sbSizerViewRGB->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 264,264 ), 0 );
	m_RgbBitmap->SetMinSize( wxSize( 264,264 ) );

	sbSizerViewRGB->Add( m_RgbBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND|wxSHAPED, 5 );


	sbSizerViewRGB->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerRGB;
	fgSizerRGB = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRGB->AddGrowableCol( 0 );
	fgSizerRGB->AddGrowableCol( 1 );
	fgSizerRGB->AddGrowableCol( 2 );
	fgSizerRGB->SetFlexibleDirection( wxBOTH );
	fgSizerRGB->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextR = new wxStaticText( sbSizerViewRGB->GetStaticBox(), wxID_ANY, _("Red:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextR->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextR, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextG = new wxStaticText( sbSizerViewRGB->GetStaticBox(), wxID_ANY, _("Green:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextG->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextG, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextB = new wxStaticText( sbSizerViewRGB->GetStaticBox(), wxID_ANY, _("Blue:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextB->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextB, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlRed = new wxSpinCtrl( sbSizerViewRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	fgSizerRGB->Add( m_spinCtrlRed, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlGreen = new wxSpinCtrl( sbSizerViewRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	fgSizerRGB->Add( m_spinCtrlGreen, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlBlue = new wxSpinCtrl( sbSizerViewRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	fgSizerRGB->Add( m_spinCtrlBlue, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbSizerViewRGB->Add( fgSizerRGB, 0, wxEXPAND, 5 );


	bSizerPanels->Add( sbSizerViewRGB, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizerViewHSV;
	sbSizerViewHSV = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, _("HSV") ), wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_HsvBitmap = new wxStaticBitmap( sbSizerViewHSV->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 264,264 ), 0 );
	m_HsvBitmap->SetMinSize( wxSize( 264,264 ) );

	bSizerLeftCol->Add( m_HsvBitmap, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND|wxSHAPED, 5 );


	bSizerLeftCol->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerHSV;
	fgSizerHSV = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerHSV->AddGrowableCol( 0 );
	fgSizerHSV->AddGrowableCol( 1 );
	fgSizerHSV->SetFlexibleDirection( wxBOTH );
	fgSizerHSV->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextHue = new wxStaticText( sbSizerViewHSV->GetStaticBox(), wxID_ANY, _("Hue:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHue->Wrap( -1 );
	fgSizerHSV->Add( m_staticTextHue, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextSat = new wxStaticText( sbSizerViewHSV->GetStaticBox(), wxID_ANY, _("Saturation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSat->Wrap( -1 );
	fgSizerHSV->Add( m_staticTextSat, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlHue = new wxSpinCtrl( sbSizerViewHSV->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	fgSizerHSV->Add( m_spinCtrlHue, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlSaturation = new wxSpinCtrl( sbSizerViewHSV->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	fgSizerHSV->Add( m_spinCtrlSaturation, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerLeftCol->Add( fgSizerHSV, 0, wxEXPAND, 5 );


	sbSizerViewHSV->Add( bSizerLeftCol, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerBright;
	bSizerBright = new wxBoxSizer( wxVERTICAL );

	m_staticTextBright = new wxStaticText( sbSizerViewHSV->GetStaticBox(), wxID_ANY, _("Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBright->Wrap( -1 );
	bSizerBright->Add( m_staticTextBright, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_sliderBrightness = new wxSlider( sbSizerViewHSV->GetStaticBox(), wxID_ANY, 255, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_INVERSE|wxSL_LABELS|wxSL_LEFT|wxSL_VERTICAL );
	bSizerBright->Add( m_sliderBrightness, 1, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT, 5 );


	sbSizerViewHSV->Add( bSizerBright, 0, wxEXPAND, 5 );


	bSizerPanels->Add( sbSizerViewHSV, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerUpperFreeColors->Add( bSizerPanels, 1, wxEXPAND, 5 );


	m_panelFreeColors->SetSizer( bSizerUpperFreeColors );
	m_panelFreeColors->Layout();
	bSizerUpperFreeColors->Fit( m_panelFreeColors );
	m_notebook->AddPage( m_panelFreeColors, _("Color Picker"), true );
	m_panelDefinedColors = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_SizerDefinedColors = new wxBoxSizer( wxVERTICAL );

	m_fgridColor = new wxFlexGridSizer( 0, 10, 25, 5 );
	m_fgridColor->AddGrowableCol( 1 );
	m_fgridColor->AddGrowableCol( 3 );
	m_fgridColor->AddGrowableCol( 5 );
	m_fgridColor->AddGrowableCol( 7 );
	m_fgridColor->AddGrowableCol( 9 );
	m_fgridColor->SetFlexibleDirection( wxBOTH );
	m_fgridColor->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_SizerDefinedColors->Add( m_fgridColor, 1, wxALL|wxEXPAND, 10 );


	m_panelDefinedColors->SetSizer( m_SizerDefinedColors );
	m_panelDefinedColors->Layout();
	m_SizerDefinedColors->Fit( m_panelDefinedColors );
	m_notebook->AddPage( m_panelDefinedColors, _("Defined Colors"), false );

	bSizerUpperMain->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );

	m_SizerTransparency = new wxBoxSizer( wxVERTICAL );


	m_SizerTransparency->Add( 0, 20, 0, wxTOP, 5 );

	m_opacityLabel = new wxStaticText( this, wxID_ANY, _("Opacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityLabel->Wrap( -1 );
	m_SizerTransparency->Add( m_opacityLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxLEFT, 5 );

	m_sliderTransparency = new wxSlider( this, wxID_ANY, 80, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_INVERSE|wxSL_LABELS|wxSL_LEFT|wxSL_VERTICAL );
	m_SizerTransparency->Add( m_sliderTransparency, 1, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizerUpperMain->Add( m_SizerTransparency, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerUpperMain, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextOldColor = new wxStaticText( this, wxID_ANY, _("Preview (old/new):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOldColor->Wrap( -1 );
	bButtonsSizer->Add( m_staticTextOldColor, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_OldColorRect = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_OldColorRect->SetMinSize( wxSize( 24,24 ) );

	bButtonsSizer->Add( m_OldColorRect, 0, wxALIGN_CENTER_VERTICAL|wxSHAPED, 5 );

	m_NewColorRect = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_NewColorRect->SetMinSize( wxSize( 24,24 ) );

	bButtonsSizer->Add( m_NewColorRect, 0, wxALIGN_CENTER_VERTICAL|wxSHAPED, 5 );

	m_colorValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_colorValue->SetMinSize( wxSize( 176,-1 ) );

	bButtonsSizer->Add( m_colorValue, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bButtonsSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_resetToDefault = new wxButton( this, wxID_ANY, _("Reset to Default"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_resetToDefault, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonsSizer->Add( m_sdbSizer, 1, wxALL, 5 );


	bSizerMain->Add( bButtonsSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_RgbBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onRGBMouseClick ), NULL, this );
	m_RgbBitmap->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onRGBMouseDrag ), NULL, this );
	m_spinCtrlRed->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditRed ), NULL, this );
	m_spinCtrlGreen->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditGreen ), NULL, this );
	m_spinCtrlBlue->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditBlue ), NULL, this );
	m_HsvBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onHSVMouseClick ), NULL, this );
	m_HsvBitmap->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onHSVMouseDrag ), NULL, this );
	m_HsvBitmap->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_COLOR_PICKER_BASE::onSize ), NULL, this );
	m_spinCtrlHue->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditHue ), NULL, this );
	m_spinCtrlSaturation->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditSat ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_colorValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_COLOR_PICKER_BASE::OnColorValueText ), NULL, this );
	m_resetToDefault->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_PICKER_BASE::OnResetButton ), NULL, this );
}

DIALOG_COLOR_PICKER_BASE::~DIALOG_COLOR_PICKER_BASE()
{
	// Disconnect Events
	m_RgbBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onRGBMouseClick ), NULL, this );
	m_RgbBitmap->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onRGBMouseDrag ), NULL, this );
	m_spinCtrlRed->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditRed ), NULL, this );
	m_spinCtrlGreen->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditGreen ), NULL, this );
	m_spinCtrlBlue->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditBlue ), NULL, this );
	m_HsvBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onHSVMouseClick ), NULL, this );
	m_HsvBitmap->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_COLOR_PICKER_BASE::onHSVMouseDrag ), NULL, this );
	m_HsvBitmap->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_COLOR_PICKER_BASE::onSize ), NULL, this );
	m_spinCtrlHue->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditHue ), NULL, this );
	m_spinCtrlSaturation->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeEditSat ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeBrightness ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_COLOR_PICKER_BASE::OnChangeAlpha ), NULL, this );
	m_colorValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_COLOR_PICKER_BASE::OnColorValueText ), NULL, this );
	m_resetToDefault->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_COLOR_PICKER_BASE::OnResetButton ), NULL, this );

}
