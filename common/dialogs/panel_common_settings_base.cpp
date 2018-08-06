///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_common_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COMMON_SETTINGS_BASE::PANEL_COMMON_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );
	
	m_staticTextautosave = new wxStaticText( this, wxID_ANY, _("&Auto save:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextautosave->Wrap( -1 );
	gbSizer1->Add( m_staticTextautosave, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	m_SaveTime = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer6->Add( m_SaveTime, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxStaticText* minutesLabel;
	minutesLabel = new wxStaticText( this, wxID_ANY, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
	minutesLabel->Wrap( -1 );
	bSizer6->Add( minutesLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	gbSizer1->Add( bSizer6, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );
	
	m_staticTextFileHistorySize = new wxStaticText( this, wxID_ANY, _("File history size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFileHistorySize->Wrap( -1 );
	gbSizer1->Add( m_staticTextFileHistorySize, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_fileHistorySize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 0 );
	gbSizer1->Add( m_fileHistorySize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxStaticText* textEditorLabel;
	textEditorLabel = new wxStaticText( this, wxID_ANY, _("Text editor:"), wxDefaultPosition, wxDefaultSize, 0 );
	textEditorLabel->Wrap( -1 );
	gbSizer1->Add( textEditorLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	m_textEditorPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textEditorPath->Enable( false );
	m_textEditorPath->SetMinSize( wxSize( 300,-1 ) );
	
	gbSizer1->Add( m_textEditorPath, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_textEditorBtn = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_textEditorBtn->SetMinSize( wxSize( 29,29 ) );
	
	gbSizer1->Add( m_textEditorBtn, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );
	
	m_defaultPDFViewer = new wxRadioButton( this, wxID_ANY, _("System default PDF viewer"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_defaultPDFViewer, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_otherPDFViewer = new wxRadioButton( this, wxID_ANY, _("Other:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_otherPDFViewer, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	m_PDFViewerPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PDFViewerPath->Enable( false );
	m_PDFViewerPath->SetMinSize( wxSize( 300,-1 ) );
	
	gbSizer1->Add( m_PDFViewerPath, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_pdfViewerBtn = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_pdfViewerBtn->SetMinSize( wxSize( 29,29 ) );
	
	gbSizer1->Add( m_pdfViewerBtn, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );
	
	
	gbSizer1->AddGrowableCol( 1 );
	
	bLeftSizer->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Interface") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer11->AddGrowableCol( 1 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("Icon scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer11->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_scaleSlider = new STEPPED_SLIDER( sbSizer5->GetStaticBox(), wxID_ANY, 50, 50, 275, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer11->Add( m_scaleSlider, 0, wxBOTTOM|wxEXPAND, 4 );
	
	m_staticText2 = new wxStaticText( sbSizer5->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer11->Add( m_staticText2, 0, wxALIGN_BOTTOM|wxBOTTOM, 2 );
	
	
	fgSizer11->Add( 0, 0, 0, wxEXPAND, 5 );
	
	m_scaleAuto = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_scaleAuto, 0, wxTOP|wxLEFT, 9 );
	
	
	fgSizer11->Add( 0, 0, 0, wxEXPAND, 5 );
	
	
	sbSizer5->Add( fgSizer11, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer5->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_checkBoxIconsInMenus = new wxCheckBox( sbSizer5->GetStaticBox(), wxID_ANY, _("Show icons in menus"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer5->Add( m_checkBoxIconsInMenus, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bLeftSizer->Add( sbSizer5, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer51;
	sbSizer51 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan and Zoom") ), wxVERTICAL );
	
	m_ZoomCenterOpt = new wxCheckBox( sbSizer51->GetStaticBox(), wxID_ANY, _("Ce&nter and warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoomCenterOpt->SetToolTip( _("Center the cursor on screen when zooming.") );
	
	sbSizer51->Add( m_ZoomCenterOpt, 0, wxRIGHT|wxLEFT, 5 );
	
	m_MousewheelPANOpt = new wxCheckBox( sbSizer51->GetStaticBox(), wxID_ANY, _("Use touchpad to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MousewheelPANOpt->SetToolTip( _("Enable touchpad-friendly controls (pan with scroll action, zoom with Ctrl+scroll).") );
	
	sbSizer51->Add( m_MousewheelPANOpt, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_AutoPANOpt = new wxCheckBox( sbSizer51->GetStaticBox(), wxID_AUTOPAN, _("&Pan while moving object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AutoPANOpt->SetToolTip( _("When drawing a track or moving an item, pan when approaching the edge of the display.") );
	
	sbSizer51->Add( m_AutoPANOpt, 0, wxALL, 5 );
	
	
	bLeftSizer->Add( sbSizer51, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bPanelSizer->Add( bLeftSizer, 0, 0, 5 );
	
	
	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
	
	// Connect Events
	m_textEditorBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_pdfViewerBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleAuto ), NULL, this );
}

PANEL_COMMON_SETTINGS_BASE::~PANEL_COMMON_SETTINGS_BASE()
{
	// Disconnect Events
	m_textEditorBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_pdfViewerBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COMMON_SETTINGS_BASE::OnScaleAuto ), NULL, this );
	
}
