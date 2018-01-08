///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "gerbview_dialog_display_options_frame_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DISPLAY_OPTIONS_BASE::DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );
	
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("Cartesian coordinates"), _("Polar coordinates") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_ANY, _("Coordinates"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 0 );
	bLeftSizer->Add( m_PolarDisplay, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_BoxUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_BoxUnitsNChoices = sizeof( m_BoxUnitsChoices ) / sizeof( wxString );
	m_BoxUnits = new wxRadioBox( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_BoxUnitsNChoices, m_BoxUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_BoxUnits->SetSelection( 0 );
	bLeftSizer->Add( m_BoxUnits, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayFlashedItemsChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayFlashedItemsNChoices = sizeof( m_OptDisplayFlashedItemsChoices ) / sizeof( wxString );
	m_OptDisplayFlashedItems = new wxRadioBox( this, wxID_ANY, _("Flashed items"), wxDefaultPosition, wxDefaultSize, m_OptDisplayFlashedItemsNChoices, m_OptDisplayFlashedItemsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayFlashedItems->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayFlashedItems, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayLinesChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayLinesNChoices = sizeof( m_OptDisplayLinesChoices ) / sizeof( wxString );
	m_OptDisplayLines = new wxRadioBox( this, wxID_ANY, _("Lines"), wxDefaultPosition, wxDefaultSize, m_OptDisplayLinesNChoices, m_OptDisplayLinesChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayLines->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayLines, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OptDisplayPolygonsChoices[] = { _("Sketch"), _("Filled") };
	int m_OptDisplayPolygonsNChoices = sizeof( m_OptDisplayPolygonsChoices ) / sizeof( wxString );
	m_OptDisplayPolygons = new wxRadioBox( this, wxID_ANY, _("Polygons"), wxDefaultPosition, wxDefaultSize, m_OptDisplayPolygonsNChoices, m_OptDisplayPolygonsChoices, 1, wxRA_SPECIFY_COLS );
	m_OptDisplayPolygons->SetSelection( 1 );
	bLeftSizer->Add( m_OptDisplayPolygons, 0, wxALL|wxEXPAND, 5 );
	
	m_OptDisplayDCodes = new wxCheckBox( this, wxID_ANY, _("Show D codes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDCodes->SetValue(true); 
	bLeftSizer->Add( m_OptDisplayDCodes, 0, wxALL, 5 );
	
	
	m_UpperSizer->Add( bLeftSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_ShowPageLimitsChoices[] = { _("Full size without limits"), _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_ShowPageLimitsNChoices = sizeof( m_ShowPageLimitsChoices ) / sizeof( wxString );
	m_ShowPageLimits = new wxRadioBox( this, wxID_ANY, _("Page"), wxDefaultPosition, wxDefaultSize, m_ShowPageLimitsNChoices, m_ShowPageLimitsChoices, 1, wxRA_SPECIFY_COLS );
	m_ShowPageLimits->SetSelection( 0 );
	bRightSizer->Add( m_ShowPageLimits, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* bLeftBottomSizer;
	bLeftBottomSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pan and Zoom") ), wxVERTICAL );
	
	m_OptZoomNoCenter = new wxCheckBox( bLeftBottomSizer->GetStaticBox(), wxID_ANY, _("Do not center and warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptZoomNoCenter->SetToolTip( _("Keep the cursor at its current location when zooming") );
	
	bLeftBottomSizer->Add( m_OptZoomNoCenter, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_OptMousewheelPan = new wxCheckBox( bLeftBottomSizer->GetStaticBox(), wxID_ANY, _("Use touchpad to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptMousewheelPan->SetToolTip( _("Use touchpad to pan canvas") );
	
	bLeftBottomSizer->Add( m_OptMousewheelPan, 0, wxALL, 5 );
	
	
	bRightSizer->Add( bLeftBottomSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Interface:") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Icon scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_scaleSlider = new STEPPED_SLIDER( sbSizer2->GetStaticBox(), wxID_ANY, 50, 50, 275, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer1->Add( m_scaleSlider, 0, wxALL|wxEXPAND, 3 );
	
	m_staticText2 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_scaleAuto = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_scaleAuto, 0, wxALL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbSizer2->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );
	
	
	bRightSizer->Add( sbSizer2, 1, wxALL|wxEXPAND, 5 );
	
	
	m_UpperSizer->Add( bRightSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	bDialogSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bDialogSizer->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bDialogSizer->Add( m_sdbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );
	
	// Connect Events
	m_scaleSlider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleAuto ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOKBUttonClick ), NULL, this );
}

DIALOG_DISPLAY_OPTIONS_BASE::~DIALOG_DISPLAY_OPTIONS_BASE()
{
	// Disconnect Events
	m_scaleSlider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleSlider ), NULL, this );
	m_scaleAuto->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnScaleAuto ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DISPLAY_OPTIONS_BASE::OnOKBUttonClick ), NULL, this );
	
}
