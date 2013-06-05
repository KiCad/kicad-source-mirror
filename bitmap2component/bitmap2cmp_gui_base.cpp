///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "bitmap2cmp_gui_base.h"

///////////////////////////////////////////////////////////////////////////

BM2CMP_FRAME_BASE::BM2CMP_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_InitialPicturePanel = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_InitialPicturePanel->SetScrollRate( 5, 5 );
	m_InitialPicturePanel->SetMinSize( wxSize( 400,300 ) );
	
	m_notebook1->AddPage( m_InitialPicturePanel, _("Original Picture"), true );
	m_GreyscalePicturePanel = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_GreyscalePicturePanel->SetScrollRate( 5, 5 );
	m_GreyscalePicturePanel->SetMinSize( wxSize( 400,300 ) );
	
	m_notebook1->AddPage( m_GreyscalePicturePanel, _("Greyscale Picture"), false );
	m_BNPicturePanel = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_BNPicturePanel->SetScrollRate( 5, 5 );
	m_notebook1->AddPage( m_BNPicturePanel, _("Black&&White Picture"), false );
	
	bMainSizer->Add( m_notebook1, 1, wxEXPAND, 5 );
	
	m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* brightSizer;
	brightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( m_panelRight, wxID_ANY, _("Bitmap Info:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerInfo;
	fgSizerInfo = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizerInfo->SetFlexibleDirection( wxBOTH );
	fgSizerInfo->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextSizeX = new wxStaticText( m_panelRight, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextSizeX, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SizeXValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXValue, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SizeXunits = new wxStaticText( m_panelRight, wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXunits->Wrap( -1 );
	fgSizerInfo->Add( m_SizeXunits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextSizeY = new wxStaticText( m_panelRight, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextSizeY, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SizeYValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYValue->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYValue, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SizeYunits = new wxStaticText( m_panelRight, wxID_ANY, _("pixels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYunits->Wrap( -1 );
	fgSizerInfo->Add( m_SizeYunits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextBPP = new wxStaticText( m_panelRight, wxID_ANY, _("BPP:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBPP->Wrap( -1 );
	fgSizerInfo->Add( m_staticTextBPP, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_BPPValue = new wxStaticText( m_panelRight, wxID_ANY, _("0000"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPValue->Wrap( -1 );
	fgSizerInfo->Add( m_BPPValue, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_BPPunits = new wxStaticText( m_panelRight, wxID_ANY, _("bits"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BPPunits->Wrap( -1 );
	fgSizerInfo->Add( m_BPPunits, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizerInfo->Add( fgSizerInfo, 0, wxEXPAND|wxBOTTOM, 5 );
	
	brightSizer->Add( sbSizerInfo, 0, wxEXPAND|wxALL, 5 );
	
	m_buttonLoad = new wxButton( m_panelRight, wxID_ANY, _("Load Bitmap"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonLoad, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonExportEeschema = new wxButton( m_panelRight, wxID_ANY, _("Export to Eeschema"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonExportEeschema->SetToolTip( _("Create a library file for Eeschema\nThis library contains only one component: logo") );
	
	brightSizer->Add( m_buttonExportEeschema, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonExportPcbnew = new wxButton( m_panelRight, wxID_ANY, _("Export to Pcbnew"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonExportPcbnew->SetToolTip( _("Create a footprint file for PcbNew\nThis footprint contains only one footprint: logo") );
	
	brightSizer->Add( m_buttonExportPcbnew, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxString m_rbOptionsChoices[] = { _("Normal"), _("Negative") };
	int m_rbOptionsNChoices = sizeof( m_rbOptionsChoices ) / sizeof( wxString );
	m_rbOptions = new wxRadioBox( m_panelRight, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, m_rbOptionsNChoices, m_rbOptionsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOptions->SetSelection( 0 );
	brightSizer->Add( m_rbOptions, 0, wxALL|wxEXPAND, 5 );
	
	m_ThresholdText = new wxStaticText( m_panelRight, wxID_ANY, _("Threshold Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThresholdText->Wrap( -1 );
	brightSizer->Add( m_ThresholdText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sliderThreshold = new wxSlider( m_panelRight, wxID_ANY, 25, 0, 50, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_TOP );
	m_sliderThreshold->SetToolTip( _("Adjust the level to convert the greyscale picture to a black and white picture.") );
	
	brightSizer->Add( m_sliderThreshold, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
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
	m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExportEeschema->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportEeschema ), NULL, this );
	m_buttonExportPcbnew->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportPcbnew ), NULL, this );
	m_rbOptions->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnOptionsSelection ), NULL, this );
	m_sliderThreshold->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
}

BM2CMP_FRAME_BASE::~BM2CMP_FRAME_BASE()
{
	// Disconnect Events
	m_InitialPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_GreyscalePicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_BNPicturePanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( BM2CMP_FRAME_BASE::OnPaint ), NULL, this );
	m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnLoadFile ), NULL, this );
	m_buttonExportEeschema->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportEeschema ), NULL, this );
	m_buttonExportPcbnew->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnExportPcbnew ), NULL, this );
	m_rbOptions->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( BM2CMP_FRAME_BASE::OnOptionsSelection ), NULL, this );
	m_sliderThreshold->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( BM2CMP_FRAME_BASE::OnThresholdChange ), NULL, this );
	
}
