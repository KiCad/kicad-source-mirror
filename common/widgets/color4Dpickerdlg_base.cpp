///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  4 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "color4Dpickerdlg_base.h"

///////////////////////////////////////////////////////////////////////////

COLOR4D_PICKER_DLG_BASE::COLOR4D_PICKER_DLG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	sbSizerViewRGB = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, wxT("RGB") ), wxVERTICAL );
	
	
	sbSizerViewRGB->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_RgbBitmap = new wxStaticBitmap( sbSizerViewRGB->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 264,264 ), 0 );
	m_RgbBitmap->SetMinSize( wxSize( 264,264 ) );
	
	sbSizerViewRGB->Add( m_RgbBitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	sbSizerViewRGB->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerPanels->Add( sbSizerViewRGB, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbSizerViewHSV;
	sbSizerViewHSV = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, wxT("Hue and Saturation") ), wxVERTICAL );
	
	
	sbSizerViewHSV->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_HsvBitmap = new wxStaticBitmap( sbSizerViewHSV->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 264,264 ), 0 );
	m_HsvBitmap->SetMinSize( wxSize( 264,264 ) );
	
	sbSizerViewHSV->Add( m_HsvBitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	sbSizerViewHSV->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerPanels->Add( sbSizerViewHSV, 1, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerBright;
	bSizerBright = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextBright = new wxStaticText( m_panelFreeColors, wxID_ANY, wxT("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBright->Wrap( -1 );
	bSizerBright->Add( m_staticTextBright, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_sliderBrightness = new wxSlider( m_panelFreeColors, wxID_ANY, 255, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_LABELS|wxSL_LEFT|wxSL_VERTICAL );
	bSizerBright->Add( m_sliderBrightness, 1, wxALL, 5 );
	
	
	bSizerPanels->Add( bSizerBright, 0, wxEXPAND, 5 );
	
	
	bSizerUpperFreeColors->Add( bSizerPanels, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLowerFreeColors;
	bSizerLowerFreeColors = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerSetRGB;
	sbSizerSetRGB = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, wxT("RGB Values") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizerRGB;
	fgSizerRGB = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerRGB->AddGrowableCol( 0 );
	fgSizerRGB->AddGrowableCol( 1 );
	fgSizerRGB->AddGrowableCol( 2 );
	fgSizerRGB->SetFlexibleDirection( wxBOTH );
	fgSizerRGB->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextR = new wxStaticText( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxT("Red"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextR->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextR, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextG = new wxStaticText( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxT("Green"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextG->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextG, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextB = new wxStaticText( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxT("Blue"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextB->Wrap( -1 );
	fgSizerRGB->Add( m_staticTextB, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_spinCtrlRed = new wxSpinCtrl( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	m_spinCtrlRed->SetMinSize( wxSize( 80,-1 ) );
	
	fgSizerRGB->Add( m_spinCtrlRed, 0, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlGreen = new wxSpinCtrl( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	m_spinCtrlGreen->SetMinSize( wxSize( 80,-1 ) );
	
	fgSizerRGB->Add( m_spinCtrlGreen, 0, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlBlue = new wxSpinCtrl( sbSizerSetRGB->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	m_spinCtrlBlue->SetMinSize( wxSize( 80,-1 ) );
	
	fgSizerRGB->Add( m_spinCtrlBlue, 0, wxALL|wxEXPAND, 5 );
	
	
	sbSizerSetRGB->Add( fgSizerRGB, 1, wxEXPAND, 5 );
	
	
	bSizerLowerFreeColors->Add( sbSizerSetRGB, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerSetHSV;
	sbSizerSetHSV = new wxStaticBoxSizer( new wxStaticBox( m_panelFreeColors, wxID_ANY, wxT("HS Values") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizerHSV;
	fgSizerHSV = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerHSV->AddGrowableCol( 0 );
	fgSizerHSV->AddGrowableCol( 1 );
	fgSizerHSV->SetFlexibleDirection( wxBOTH );
	fgSizerHSV->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextHue = new wxStaticText( sbSizerSetHSV->GetStaticBox(), wxID_ANY, wxT("Hue"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHue->Wrap( -1 );
	fgSizerHSV->Add( m_staticTextHue, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextSat = new wxStaticText( sbSizerSetHSV->GetStaticBox(), wxID_ANY, wxT("Saturation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSat->Wrap( -1 );
	fgSizerHSV->Add( m_staticTextSat, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_spinCtrlHue = new wxSpinCtrl( sbSizerSetHSV->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 359, 0 );
	m_spinCtrlHue->SetMinSize( wxSize( 80,-1 ) );
	
	fgSizerHSV->Add( m_spinCtrlHue, 0, wxALL|wxEXPAND, 5 );
	
	m_spinCtrlSaturation = new wxSpinCtrl( sbSizerSetHSV->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 128 );
	m_spinCtrlSaturation->SetMinSize( wxSize( 80,-1 ) );
	
	fgSizerHSV->Add( m_spinCtrlSaturation, 0, wxALL|wxEXPAND, 5 );
	
	
	sbSizerSetHSV->Add( fgSizerHSV, 1, wxEXPAND, 5 );
	
	
	bSizerLowerFreeColors->Add( sbSizerSetHSV, 1, wxEXPAND, 5 );
	
	
	bSizerUpperFreeColors->Add( bSizerLowerFreeColors, 0, wxEXPAND, 5 );
	
	
	m_panelFreeColors->SetSizer( bSizerUpperFreeColors );
	m_panelFreeColors->Layout();
	bSizerUpperFreeColors->Fit( m_panelFreeColors );
	m_notebook->AddPage( m_panelFreeColors, wxT("Color Picker"), true );
	m_panelDefinedColors = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_SizerDefinedColors = new wxBoxSizer( wxVERTICAL );
	
	
	m_SizerDefinedColors->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_fgridColor = new wxFlexGridSizer( 0, 10, 0, 0 );
	m_fgridColor->AddGrowableCol( 1 );
	m_fgridColor->AddGrowableCol( 3 );
	m_fgridColor->AddGrowableCol( 5 );
	m_fgridColor->AddGrowableCol( 7 );
	m_fgridColor->AddGrowableCol( 9 );
	m_fgridColor->SetFlexibleDirection( wxBOTH );
	m_fgridColor->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	m_SizerDefinedColors->Add( m_fgridColor, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_SizerDefinedColors->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panelDefinedColors->SetSizer( m_SizerDefinedColors );
	m_panelDefinedColors->Layout();
	m_SizerDefinedColors->Fit( m_panelDefinedColors );
	m_notebook->AddPage( m_panelDefinedColors, wxT("Defined Colors"), false );
	
	bSizerUpperMain->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );
	
	m_SizerTransparency = new wxBoxSizer( wxVERTICAL );
	
	
	m_SizerTransparency->Add( 0, 20, 0, 0, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("Opacity %"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	m_SizerTransparency->Add( m_staticText9, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_sliderTransparency = new wxSlider( this, wxID_ANY, 80, 20, 100, wxDefaultPosition, wxDefaultSize, wxSL_LABELS|wxSL_LEFT|wxSL_VERTICAL );
	m_SizerTransparency->Add( m_sliderTransparency, 1, wxALL, 5 );
	
	
	m_SizerTransparency->Add( 0, 20, 0, 0, 5 );
	
	
	bSizerUpperMain->Add( m_SizerTransparency, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerShowColors;
	bSizerShowColors = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextOldColor = new wxStaticText( this, wxID_ANY, wxT("Old Color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOldColor->Wrap( -1 );
	bSizerShowColors->Add( m_staticTextOldColor, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OldColorRect = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 64,64 ), 0 );
	bSizerShowColors->Add( m_OldColorRect, 0, wxALL, 5 );
	
	m_staticTextNewColor = new wxStaticText( this, wxID_ANY, wxT("New Color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNewColor->Wrap( -1 );
	bSizerShowColors->Add( m_staticTextNewColor, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_NewColorRect = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 64,64 ), 0 );
	bSizerShowColors->Add( m_NewColorRect, 0, wxALL, 5 );
	
	
	bSizerUpperMain->Add( bSizerShowColors, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerMain->Add( bSizerUpperMain, 1, wxEXPAND, 5 );
	
	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_RgbBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onRGBMouseClick ), NULL, this );
	m_RgbBitmap->Connect( wxEVT_MOTION, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onRGBMouseDrag ), NULL, this );
	m_HsvBitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onHSVMouseClick ), NULL, this );
	m_HsvBitmap->Connect( wxEVT_MOTION, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onHSVMouseDrag ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_spinCtrlRed->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditRed ), NULL, this );
	m_spinCtrlGreen->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditGreen ), NULL, this );
	m_spinCtrlBlue->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditBlue ), NULL, this );
	m_spinCtrlHue->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditHue ), NULL, this );
	m_spinCtrlSaturation->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditSat ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
}

COLOR4D_PICKER_DLG_BASE::~COLOR4D_PICKER_DLG_BASE()
{
	// Disconnect Events
	m_RgbBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onRGBMouseClick ), NULL, this );
	m_RgbBitmap->Disconnect( wxEVT_MOTION, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onRGBMouseDrag ), NULL, this );
	m_HsvBitmap->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onHSVMouseClick ), NULL, this );
	m_HsvBitmap->Disconnect( wxEVT_MOTION, wxMouseEventHandler( COLOR4D_PICKER_DLG_BASE::onHSVMouseDrag ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_sliderBrightness->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeBrightness ), NULL, this );
	m_spinCtrlRed->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditRed ), NULL, this );
	m_spinCtrlGreen->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditGreen ), NULL, this );
	m_spinCtrlBlue->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditBlue ), NULL, this );
	m_spinCtrlHue->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditHue ), NULL, this );
	m_spinCtrlSaturation->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeEditSat ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	m_sliderTransparency->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( COLOR4D_PICKER_DLG_BASE::OnChangeAlpha ), NULL, this );
	
}
