///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
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
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( m_panelRight, wxID_ANY, _("Bitmap Info:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerInfo;
	fgSizerInfo = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerInfo->AddGrowableCol( 1 );
	fgSizerInfo->AddGrowableCol( 2 );
	fgSizerInfo->SetFlexibleDirection( wxBOTH );
	fgSizerInfo->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextSize = new wxStaticText( m_panelRight, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSize->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextSize, 0, wxALIGN_RIGHT|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SizeXValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_SizeYValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_SizePixUnits = new wxStaticText( m_panelRight, wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizePixUnits->Wrap( -1 );
	fgSizerInfo->Add( m_SizePixUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticTextSize1 = new wxStaticText( m_panelRight, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSize1->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextSize1, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );
	
	m_SizeXValue_mm = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXValue_mm->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXValue_mm, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_SizeYValue_mm = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYValue_mm->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYValue_mm, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_Size_mmxUnits = new wxStaticText( m_panelRight, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Size_mmxUnits->Wrap( -1 );
	fgSizerInfo->Add( m_Size_mmxUnits, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_staticTextBPP = new wxStaticText( m_panelRight, wxID_ANY, _("BPP:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBPP->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextBPP, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_BPPValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPValue->Wrap( -1 );
	fgSizerInfo->Add( m_BPPValue, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	m_BPPunits = new wxStaticText( m_panelRight, wxID_ANY, _("bits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPunits->Wrap( -1 );
	fgSizerInfo->Add( m_BPPunits, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerInfo->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextBPI = new wxStaticText( m_panelRight, wxID_ANY, _("Resolution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBPI->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextBPI, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_DPIValueX = new wxTextCtrl( m_panelRight, wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DPIValueX->SetMinSize( wxSize( 40,-1 ) );
	
	fgSizerInfo->Add( m_DPIValueX, 0, wxBOTTOM|wxRIGHT, 5 );
	
	m_DPIValueY = new wxTextCtrl( m_panelRight, wxID_ANY, _("300"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DPIValueY->SetMinSize( wxSize( 40,-1 ) );
	
	fgSizerInfo->Add( m_DPIValueY, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	m_DPI_Units = new wxStaticText( m_panelRight, wxID_ANY, _("DPI"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DPI_Units->Wrap( -1 );
	fgSizerInfo->Add( m_DPI_Units, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	
	sbSizerInfo->Add( fgSizerInfo, 0, wxEXPAND|wxBOTTOM, 5 );
	
	
	brightSizer->Add( sbSizerInfo, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonLoad = new wxButton( m_panelRight, wxID_ANY, _("Load Bitmap"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonLoad, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_buttonExport = new wxButton( m_panelRight, wxID_ANY, _("Export"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonExport->SetToolTip( _("Create a library file for Eeschema\nThis library contains only one component: logo") );
	
	brightSizer->Add( m_buttonExport, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_radioBoxFormatChoices[] = { _("Eeschema (.lib file)"), _("Pcbnew (.kicad_mod file)"), _("Postscript (.ps file)"), _("Logo for title block (.kicad_wks file)") };
	int m_radioBoxFormatNChoices = sizeof( m_radioBoxFormatChoices ) / sizeof( wxString );
	m_radioBoxFormat = new wxRadioBox( m_panelRight, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, m_radioBoxFormatNChoices, m_radioBoxFormatChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxFormat->SetSelection( 1 );
	brightSizer->Add( m_radioBoxFormat, 0, wxEXPAND|wxALL, 5 );
	
	wxString m_rbOptionsChoices[] = { _("Normal"), _("Negative") };
	int m_rbOptionsNChoices = sizeof( m_rbOptionsChoices ) / sizeof( wxString );
	m_rbOptions = new wxRadioBox( m_panelRight, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, m_rbOptionsNChoices, m_rbOptionsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOptions->SetSelection( 0 );
	brightSizer->Add( m_rbOptions, 0, wxEXPAND|wxALL, 5 );
	
	m_ThresholdText = new wxStaticText( m_panelRight, wxID_ANY, _("Threshold Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThresholdText->Wrap( -1 );
	brightSizer->Add( m_ThresholdText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sliderThreshold = new wxSlider( m_panelRight, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	m_sliderThreshold->SetToolTip( _("Adjust the level to convert the greyscale picture to a black and white picture.") );
	
	brightSizer->Add( m_sliderThreshold, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_radio_PCBLayerChoices[] = { _("Front silk screen"), _("Front solder mask"), _("User layer Eco1"), _("User Layer Eco2") };
	int m_radio_PCBLayerNChoices = sizeof( m_radio_PCBLayerChoices ) / sizeof( wxString );
	m_radio_PCBLayer = new wxRadioBox( m_panelRight, wxID_ANY, _("Board Layer for Outline:"), wxDefaultPosition, wxDefaultSize, m_radio_PCBLayerNChoices, m_radio_PCBLayerChoices, 1, wxRA_SPECIFY_COLS );
	m_radio_PCBLayer->SetSelection( 0 );
	m_radio_PCBLayer->SetToolTip( _("Choose the board layer to place the outline.\nThe 2 invisible fields reference and value are always placed on the silk screen layer.") );
	
	brightSizer->Add( m_radio_PCBLayer, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panelRight->SetSizer( brightSizer );
	m_panelRight->Layout();
	brightSizer->Fit( m_panelRight );
	bMainSizer->Add( m_panelRight, 0, wxEXPAND, 0 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	m_statusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	
	// Connect Events
	m_InitialPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_GreyscalePicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_BNPicturePanel->Connect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_DPIValueX->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BM2CMP_FRAME_BASE::UpdatePPITextValueX ), NULL, this );
	m_DPIValueX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnResolutionChange ), NULL, this );
	m_DPIValueY->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BM2CMP_FRAME_BASE::UpdatePPITextValueY ), NULL, this );
	m_DPIValueY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnResolutionChange ), NULL, this );
	m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExport ), NULL, this );
	m_radioBoxFormat->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnFormatChange ), NULL, this );
	m_rbOptions->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnOptionsSelection ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
}

BM2CMP_FRAME_BASE::~BM2CMP_FRAME_BASE()
{
	// Disconnect Events
	m_InitialPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_GreyscalePicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_BNPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_DPIValueX->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BM2CMP_FRAME_BASE::UpdatePPITextValueX ), NULL, this );
	m_DPIValueX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnResolutionChange ), NULL, this );
	m_DPIValueY->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BM2CMP_FRAME_BASE::UpdatePPITextValueY ), NULL, this );
	m_DPIValueY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnResolutionChange ), NULL, this );
	m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExport ), NULL, this );
	m_radioBoxFormat->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnFormatChange ), NULL, this );
	m_rbOptions->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnOptionsSelection ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	
}
