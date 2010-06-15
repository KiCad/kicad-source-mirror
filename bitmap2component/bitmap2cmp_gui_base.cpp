///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
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
	m_notebook1->AddPage( m_BNPicturePanel, _("Binary Picture"), false );
	
	bMainSizer->Add( m_notebook1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* brightSizer;
	brightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_gridInfo = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridInfo->CreateGrid( 3, 1 );
	m_gridInfo->EnableEditing( false );
	m_gridInfo->EnableGridLines( true );
	m_gridInfo->EnableDragGridSize( false );
	m_gridInfo->SetMargins( 0, 0 );
	
	// Columns
	m_gridInfo->SetColSize( 0, 80 );
	m_gridInfo->EnableDragColMove( false );
	m_gridInfo->EnableDragColSize( true );
	m_gridInfo->SetColLabelSize( 30 );
	m_gridInfo->SetColLabelValue( 0, _("Value") );
	m_gridInfo->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridInfo->AutoSizeRows();
	m_gridInfo->EnableDragRowSize( true );
	m_gridInfo->SetRowLabelSize( 80 );
	m_gridInfo->SetRowLabelValue( 0, _("Size X") );
	m_gridInfo->SetRowLabelValue( 1, _("Size Y") );
	m_gridInfo->SetRowLabelValue( 2, _("BPP") );
	m_gridInfo->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridInfo->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	brightSizer->Add( m_gridInfo, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonLoad = new wxButton( this, wxID_ANY, _("Load Bitmap"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonLoad, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonExportEeschema = new wxButton( this, wxID_ANY, _("Export to eeschema"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportEeschema, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonExportPcbnew = new wxButton( this, wxID_ANY, _("Export to Pcbnew"), wxDefaultPosition, wxDefaultSize, 0 );
	brightSizer->Add( m_buttonExportPcbnew, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxString m_rbOptionsChoices[] = { _("Normal"), _("Negative") };
	int m_rbOptionsNChoices = sizeof( m_rbOptionsChoices ) / sizeof( wxString );
	m_rbOptions = new wxRadioBox( this, wxID_ANY, _("Options"), wxDefaultPosition, wxDefaultSize, m_rbOptionsNChoices, m_rbOptionsChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOptions->SetSelection( 0 );
	brightSizer->Add( m_rbOptions, 0, wxALL|wxEXPAND, 5 );
	
	m_ThresholdText = new wxStaticText( this, wxID_ANY, _("Threshold Value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThresholdText->Wrap( -1 );
	brightSizer->Add( m_ThresholdText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sliderThreshold = new wxSlider( this, wxID_ANY, 5, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP );
	brightSizer->Add( m_sliderThreshold, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( brightSizer, 0, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
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
