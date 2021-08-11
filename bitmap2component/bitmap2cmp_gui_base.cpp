///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "bitmap2cmp_gui_base.h"

///////////////////////////////////////////////////////////////////////////

BM2CMP_FRAME_BASE::BM2CMP_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : KIWAY_PLAYER( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_InitialPicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_InitialPicturePanel->SetScrollRate( 5, 5 );
	m_InitialPicturePanel->SetMinSize( wxSize( 400,300 ) );

	m_Notebook->AddPage( m_InitialPicturePanel, _("Original Picture"), true );
	m_GreyscalePicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_GreyscalePicturePanel->SetScrollRate( 5, 5 );
	m_GreyscalePicturePanel->SetMinSize( wxSize( 400,300 ) );

	m_Notebook->AddPage( m_GreyscalePicturePanel, _("Greyscale Picture"), false );
	m_BNPicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_BNPicturePanel->SetScrollRate( 5, 5 );
	m_Notebook->AddPage( m_BNPicturePanel, _("Black&&White Picture"), false );

	bMainSizer->Add( m_Notebook, 1, wxEXPAND, 5 );

	m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* brightSizer;
	brightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( m_panelRight, wxID_ANY, _("Bitmap Information") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerInfo;
	fgSizerInfo = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerInfo->AddGrowableCol( 1 );
	fgSizerInfo->AddGrowableCol( 2 );
	fgSizerInfo->SetFlexibleDirection( wxBOTH );
	fgSizerInfo->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextISize = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Bitmap size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextISize->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextISize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_SizeXValue = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_SizeYValue = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_SizePixUnits = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizePixUnits->Wrap( -1 );
	fgSizerInfo->Add( m_SizePixUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextDPI = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Bitmap PPI:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDPI->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextDPI, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_InputXValueDPI = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_InputXValueDPI->Wrap( -1 );
	fgSizerInfo->Add( m_InputXValueDPI, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_InputYValueDPI = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_InputYValueDPI->Wrap( -1 );
	fgSizerInfo->Add( m_InputYValueDPI, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_DPIUnit = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("PPI"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DPIUnit->Wrap( -1 );
	fgSizerInfo->Add( m_DPIUnit, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextBPP = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("BPP:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBPP->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextBPP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_BPPValue = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPValue->Wrap( -1 );
	fgSizerInfo->Add( m_BPPValue, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_BPPunits = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("bits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPunits->Wrap( -1 );
	fgSizerInfo->Add( m_BPPunits, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerInfo->Add( 0, 0, 0, 0, 5 );


	sbSizerInfo->Add( fgSizerInfo, 0, wxEXPAND, 5 );


	brightSizer->Add( sbSizerInfo, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizerImgPrms;
	sbSizerImgPrms = new wxStaticBoxSizer( new wxStaticBox( m_panelRight, wxID_ANY, _("Output Parameters") ), wxVERTICAL );

	wxBoxSizer* bSizerLock;
	bSizerLock = new wxBoxSizer( wxHORIZONTAL );

	m_textLock = new wxStaticText( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("Lock height/width ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textLock->Wrap( -1 );
	bSizerLock->Add( m_textLock, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_AspectRatioLockButton = new wxBitmapButton( sbSizerImgPrms->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerLock->Add( m_AspectRatioLockButton, 0, wxALL, 5 );


	sbSizerImgPrms->Add( bSizerLock, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRes;
	bSizerRes = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextOSize = new wxStaticText( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOSize->Wrap( -1 );
	bSizerRes->Add( m_staticTextOSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_UnitSizeX = new wxTextCtrl( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitSizeX->SetMinSize( wxSize( 60,-1 ) );

	bSizerRes->Add( m_UnitSizeX, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_UnitSizeY = new wxTextCtrl( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitSizeY->SetMinSize( wxSize( 60,-1 ) );

	bSizerRes->Add( m_UnitSizeY, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxArrayString m_PixelUnitChoices;
	m_PixelUnit = new wxChoice( sbSizerImgPrms->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PixelUnitChoices, 0 );
	m_PixelUnit->SetSelection( 0 );
	m_PixelUnit->SetMinSize( wxSize( 80,-1 ) );

	bSizerRes->Add( m_PixelUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerImgPrms->Add( bSizerRes, 0, wxEXPAND, 5 );


	brightSizer->Add( sbSizerImgPrms, 0, wxEXPAND|wxALL, 5 );

	m_buttonLoad = new wxButton( m_panelRight, wxID_ANY, _("Load Bitmap"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonLoad, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_buttonExportFile = new wxButton( m_panelRight, wxID_ANY, _("Export to File"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportFile, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_buttonExportClipboard = new wxButton( m_panelRight, wxID_ANY, _("Export to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportClipboard, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_rbOutputFormatChoices[] = { _("Symbol (.kicad_sym file)"), _("Footprint (.kicad_mod file)"), _("Postscript (.ps file)"), _("Drawing Sheet (.kicad_wks file)") };
	int m_rbOutputFormatNChoices = sizeof( m_rbOutputFormatChoices ) / sizeof( wxString );
	m_rbOutputFormat = new wxRadioBox( m_panelRight, wxID_ANY, _("Output Format"), wxDefaultPosition, wxDefaultSize, m_rbOutputFormatNChoices, m_rbOutputFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOutputFormat->SetSelection( 0 );
	brightSizer->Add( m_rbOutputFormat, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_panelRight, wxID_ANY, _("Image Options") ), wxVERTICAL );

	m_ThresholdText = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Black / White Threshold:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThresholdText->Wrap( -1 );
	sbSizer2->Add( m_ThresholdText, 0, wxTOP|wxLEFT, 5 );

	m_sliderThreshold = new wxSlider( sbSizer2->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderThreshold->SetToolTip( _("Adjust the level to convert the greyscale picture to a black and white picture.") );

	sbSizer2->Add( m_sliderThreshold, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkNegative = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Negative"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_checkNegative, 0, wxTOP|wxBOTTOM, 10 );


	brightSizer->Add( sbSizer2, 0, wxALL|wxEXPAND, 5 );

	wxString m_rbPCBLayerChoices[] = { _("Front silk screen"), _("Front solder mask"), _("User layer Eco1"), _("User layer Eco2") };
	int m_rbPCBLayerNChoices = sizeof( m_rbPCBLayerChoices ) / sizeof( wxString );
	m_rbPCBLayer = new wxRadioBox( m_panelRight, wxID_ANY, _("Board Layer for Outline"), wxDefaultPosition, wxDefaultSize, m_rbPCBLayerNChoices, m_rbPCBLayerChoices, 1, wxRA_SPECIFY_COLS );
	m_rbPCBLayer->SetSelection( 1 );
	m_rbPCBLayer->SetToolTip( _("Choose the board layer to place the outline.\nThe reference designator and value are always placed on the silk screen layer (but will be marked invisible).") );

	brightSizer->Add( m_rbPCBLayer, 0, wxALL|wxEXPAND, 5 );


	m_panelRight->SetSizer( brightSizer );
	m_panelRight->Layout();
	brightSizer->Fit( m_panelRight );
	bMainSizer->Add( m_panelRight, 0, wxEXPAND, 0 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	m_statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	// Connect Events
	m_InitialPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintInit ), NULL, this );
	m_GreyscalePicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintGreyscale ), NULL, this );
	m_BNPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintBW ), NULL, this );
	m_AspectRatioLockButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::ToggleAspectRatioLock ), NULL, this );
	m_UnitSizeX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeChangeX ), NULL, this );
	m_UnitSizeY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeChangeY ), NULL, this );
	m_PixelUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeUnitChange ), NULL, this );
	m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExportFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportToFile ), NULL, this );
	m_buttonExportClipboard->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportToClipboard ), NULL, this );
	m_rbOutputFormat->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnFormatChange ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	m_checkNegative->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnNegativeClicked ), NULL, this );
}

BM2CMP_FRAME_BASE::~BM2CMP_FRAME_BASE()
{
	// Disconnect Events
	m_InitialPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintInit ), NULL, this );
	m_GreyscalePicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintGreyscale ), NULL, this );
	m_BNPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaintBW ), NULL, this );
	m_AspectRatioLockButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::ToggleAspectRatioLock ), NULL, this );
	m_UnitSizeX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeChangeX ), NULL, this );
	m_UnitSizeY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeChangeY ), NULL, this );
	m_PixelUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnSizeUnitChange ), NULL, this );
	m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExportFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportToFile ), NULL, this );
	m_buttonExportClipboard->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportToClipboard ), NULL, this );
	m_rbOutputFormat->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnFormatChange ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	m_checkNegative->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnNegativeClicked ), NULL, this );

}
