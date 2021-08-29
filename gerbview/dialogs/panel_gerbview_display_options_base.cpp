///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_gerbview_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );

	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	m_UpperSizer->Add( m_galOptionsSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbAnnotations;
	sbAnnotations = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );

	m_OptDisplayDCodes = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show D codes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDCodes->SetValue(true);
	sbAnnotations->Add( m_OptDisplayDCodes, 0, wxBOTTOM|wxLEFT, 5 );

	m_ShowPageLimitsOpt = new wxCheckBox( sbAnnotations->GetStaticBox(), wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAnnotations->Add( m_ShowPageLimitsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( sbAnnotations, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbDrawingMode;
	sbDrawingMode = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Drawing Mode") ), wxVERTICAL );

	m_OptDisplayFlashedItems = new wxCheckBox( sbDrawingMode->GetStaticBox(), wxID_ANY, _("Sketch flashed items"), wxDefaultPosition, wxDefaultSize, 0 );
	sbDrawingMode->Add( m_OptDisplayFlashedItems, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayLines = new wxCheckBox( sbDrawingMode->GetStaticBox(), wxID_ANY, _("Sketch lines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbDrawingMode->Add( m_OptDisplayLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPolygons = new wxCheckBox( sbDrawingMode->GetStaticBox(), wxID_ANY, _("Sketch polygons"), wxDefaultPosition, wxDefaultSize, 0 );
	sbDrawingMode->Add( m_OptDisplayPolygons, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( sbDrawingMode, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bPageSize;
	bPageSize = new wxBoxSizer( wxVERTICAL );

	wxString m_PageSizeChoices[] = { _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_PageSizeNChoices = sizeof( m_PageSizeChoices ) / sizeof( wxString );
	m_PageSize = new wxRadioBox( this, wxID_ANY, _("Page Size"), wxDefaultPosition, wxDefaultSize, m_PageSizeNChoices, m_PageSizeChoices, 1, wxRA_SPECIFY_COLS );
	m_PageSize->SetSelection( 0 );
	bPageSize->Add( m_PageSize, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bRightSizer->Add( bPageSize, 1, wxEXPAND, 5 );


	m_UpperSizer->Add( bRightSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bDialogSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );
}

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE()
{
}
