///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "bitmap2cmp_panel_base.h"

///////////////////////////////////////////////////////////////////////////

BITMAP2CMP_PANEL_BASE::BITMAP2CMP_PANEL_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_Notebook->SetMinSize( wxSize( 500,-1 ) );

	m_InitialPicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_InitialPicturePanel->SetScrollRate( 5, 5 );
	m_Notebook->AddPage( m_InitialPicturePanel, _("Original Picture"), false );
	m_GreyscalePicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_GreyscalePicturePanel->SetScrollRate( 5, 5 );
	m_Notebook->AddPage( m_GreyscalePicturePanel, _("Greyscale Picture"), false );
	m_BNPicturePanel = new wxScrolledWindow( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_BNPicturePanel->SetScrollRate( 5, 5 );
	m_Notebook->AddPage( m_BNPicturePanel, _("Black && White Picture"), true );

	bMainSizer->Add( m_Notebook, 1, wxEXPAND|wxBOTTOM|wxLEFT, 10 );

	wxBoxSizer* brightSizer;
	brightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Image Information") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerInfo;
	fgSizerInfo = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerInfo->AddGrowableCol( 1 );
	fgSizerInfo->AddGrowableCol( 2 );
	fgSizerInfo->SetFlexibleDirection( wxBOTH );
	fgSizerInfo->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextISize = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Image size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextISize->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextISize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_SizeXValue = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_SizeYValue = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_SizePixUnits = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizePixUnits->Wrap( -1 );
	fgSizerInfo->Add( m_SizePixUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextDPI = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("Image PPI:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDPI->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextDPI, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_InputXValueDPI = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_InputXValueDPI->Wrap( -1 );
	fgSizerInfo->Add( m_InputXValueDPI, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

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
	fgSizerInfo->Add( m_BPPValue, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_BPPunits = new wxStaticText( sbSizerInfo->GetStaticBox(), wxID_ANY, _("bits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPunits->Wrap( -1 );
	fgSizerInfo->Add( m_BPPunits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	fgSizerInfo->Add( 0, 0, 0, 0, 5 );


	sbSizerInfo->Add( fgSizerInfo, 0, wxEXPAND, 5 );


	brightSizer->Add( sbSizerInfo, 0, wxEXPAND|wxALL, 5 );

	m_buttonLoad = new wxButton( this, wxID_ANY, _("Load Source Image"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonLoad, 0, wxEXPAND|wxALL, 5 );


	brightSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerImgPrms;
	sbSizerImgPrms = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Output Size") ), wxVERTICAL );

	wxBoxSizer* bSizerRes;
	bSizerRes = new wxBoxSizer( wxHORIZONTAL );

	m_sizeLabel = new wxStaticText( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeLabel->Wrap( -1 );
	bSizerRes->Add( m_sizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_UnitSizeX = new wxTextCtrl( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitSizeX->SetMinSize( wxSize( 60,-1 ) );

	bSizerRes->Add( m_UnitSizeX, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_UnitSizeY = new wxTextCtrl( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_UnitSizeY->SetMinSize( wxSize( 60,-1 ) );

	bSizerRes->Add( m_UnitSizeY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_PixelUnitChoices;
	m_PixelUnit = new wxChoice( sbSizerImgPrms->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PixelUnitChoices, 0 );
	m_PixelUnit->SetSelection( 0 );
	m_PixelUnit->SetMinSize( wxSize( 80,-1 ) );

	bSizerRes->Add( m_PixelUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerImgPrms->Add( bSizerRes, 0, wxEXPAND|wxBOTTOM, 5 );

	m_aspectRatioCheckbox = new wxCheckBox( sbSizerImgPrms->GetStaticBox(), wxID_ANY, _("Lock height / width ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerImgPrms->Add( m_aspectRatioCheckbox, 0, wxALL, 5 );


	brightSizer->Add( sbSizerImgPrms, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_ThresholdText = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Black / white threshold:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThresholdText->Wrap( -1 );
	sbSizer2->Add( m_ThresholdText, 0, wxRIGHT|wxLEFT, 5 );

	m_sliderThreshold = new wxSlider( sbSizer2->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderThreshold->SetToolTip( _("Adjust the level to convert the greyscale picture to a black and white picture.") );

	sbSizer2->Add( m_sliderThreshold, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_checkNegative = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Negative"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_checkNegative, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	brightSizer->Add( sbSizer2, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbOutputFormat;
	sbOutputFormat = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Output Format") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 5, 1, 2, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbSymbol = new wxRadioButton( sbOutputFormat->GetStaticBox(), wxID_ANY, _("Symbol (.kicad_sym file)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbSymbol, 0, wxBOTTOM|wxRIGHT, 5 );

	m_rbFootprint = new wxRadioButton( sbOutputFormat->GetStaticBox(), wxID_ANY, _("Footprint (.kicad_mod file)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbFootprint, 0, wxTOP|wxRIGHT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_layerLabel = new wxStaticText( sbOutputFormat->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layerLabel->Wrap( -1 );
	bSizer4->Add( m_layerLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 28 );

	wxString m_layerCtrlChoices[] = { _("F.Cu"), _("F.Silkscreen"), _("F.Mask"), _("User.Drawings"), _("User.Comments"), _("User.Eco1"), _("User.Eco2"), _("F.Fab") };
	int m_layerCtrlNChoices = sizeof( m_layerCtrlChoices ) / sizeof( wxString );
	m_layerCtrl = new wxChoice( sbOutputFormat->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_layerCtrlNChoices, m_layerCtrlChoices, 0 );
	m_layerCtrl->SetSelection( 0 );
	bSizer4->Add( m_layerCtrl, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( bSizer4, 1, wxEXPAND, 5 );

	m_rbPostscript = new wxRadioButton( sbOutputFormat->GetStaticBox(), wxID_ANY, _("Postscript (.ps file)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbPostscript, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_rbWorksheet = new wxRadioButton( sbOutputFormat->GetStaticBox(), wxID_ANY, _("Drawing Sheet (.kicad_wks file)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_rbWorksheet, 0, wxTOP|wxRIGHT, 5 );


	sbOutputFormat->Add( fgSizer2, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	brightSizer->Add( sbOutputFormat, 1, wxEXPAND|wxALL, 5 );

	m_buttonExportFile = new wxButton( this, wxID_ANY, _("Export to File..."), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportFile, 0, wxEXPAND|wxALL, 5 );

	m_buttonExportClipboard = new wxButton( this, wxID_ANY, _("Export to Clipboard"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportClipboard, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( brightSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_InitialPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintInit ), NULL, this );
	m_GreyscalePicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintGreyscale ), NULL, this );
	m_BNPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintBW ), NULL, this );
	m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnLoadFile ), NULL, this );
	m_UnitSizeX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeChangeX ), NULL, this );
	m_UnitSizeY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeChangeY ), NULL, this );
	m_PixelUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeUnitChange ), NULL, this );
	m_aspectRatioCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::ToggleAspectRatioLock ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( BITMAP2CMP_PANEL_BASE::OnThresholdChange ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BITMAP2CMP_PANEL_BASE::OnThresholdChange ), NULL, this );
	m_checkNegative->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnNegativeClicked ), NULL, this );
	m_rbSymbol->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbFootprint->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbPostscript->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbWorksheet->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_buttonExportFile->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnExportToFile ), NULL, this );
	m_buttonExportClipboard->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnExportToClipboard ), NULL, this );
}

BITMAP2CMP_PANEL_BASE::~BITMAP2CMP_PANEL_BASE()
{
	// Disconnect Events
	m_InitialPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintInit ), NULL, this );
	m_GreyscalePicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintGreyscale ), NULL, this );
	m_BNPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BITMAP2CMP_PANEL_BASE::OnPaintBW ), NULL, this );
	m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnLoadFile ), NULL, this );
	m_UnitSizeX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeChangeX ), NULL, this );
	m_UnitSizeY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeChangeY ), NULL, this );
	m_PixelUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnSizeUnitChange ), NULL, this );
	m_aspectRatioCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::ToggleAspectRatioLock ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( BITMAP2CMP_PANEL_BASE::OnThresholdChange ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BITMAP2CMP_PANEL_BASE::OnThresholdChange ), NULL, this );
	m_checkNegative->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnNegativeClicked ), NULL, this );
	m_rbSymbol->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbFootprint->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbPostscript->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_rbWorksheet->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnFormatChange ), NULL, this );
	m_buttonExportFile->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnExportToFile ), NULL, this );
	m_buttonExportClipboard->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BITMAP2CMP_PANEL_BASE::OnExportToClipboard ), NULL, this );

}
