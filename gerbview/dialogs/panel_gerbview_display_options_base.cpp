///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_gerbview_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bDialogSizer;
	bDialogSizer = new wxBoxSizer( wxVERTICAL );

	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	m_UpperSizer->Add( m_galOptionsSizer, 0, wxEXPAND|wxRIGHT, 20 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Annotations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bRightSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bAnnotationsSizer;
	bAnnotationsSizer = new wxBoxSizer( wxVERTICAL );

	m_OptDisplayDCodes = new wxCheckBox( this, wxID_ANY, _("Show D codes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDCodes->SetValue(true);
	bAnnotationsSizer->Add( m_OptDisplayDCodes, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_ShowPageLimitsOpt = new wxCheckBox( this, wxID_ANY, _("Show page limits"), wxDefaultPosition, wxDefaultSize, 0 );
	bAnnotationsSizer->Add( m_ShowPageLimitsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( bAnnotationsSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bRightSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Drawing Mode"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bRightSizer->Add( m_staticText2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bDrawingModeSizer;
	bDrawingModeSizer = new wxBoxSizer( wxVERTICAL );

	m_OptDisplayFlashedItems = new wxCheckBox( this, wxID_ANY, _("Sketch flashed items"), wxDefaultPosition, wxDefaultSize, 0 );
	bDrawingModeSizer->Add( m_OptDisplayFlashedItems, 0, wxALL, 5 );

	m_OptDisplayLines = new wxCheckBox( this, wxID_ANY, _("Sketch lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bDrawingModeSizer->Add( m_OptDisplayLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPolygons = new wxCheckBox( this, wxID_ANY, _("Sketch polygons"), wxDefaultPosition, wxDefaultSize, 0 );
	bDrawingModeSizer->Add( m_OptDisplayPolygons, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( bDrawingModeSizer, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bRightSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Page Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bRightSizer->Add( m_staticText3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bPageSizeSizer;
	bPageSizeSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	wxString m_PageSizeChoices[] = { _("Full size"), _("Size A4"), _("Size A3"), _("Size A2"), _("Size A"), _("Size B"), _("Size C") };
	int m_PageSizeNChoices = sizeof( m_PageSizeChoices ) / sizeof( wxString );
	m_PageSize = new wxRadioBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_PageSizeNChoices, m_PageSizeChoices, 1, wxRA_SPECIFY_COLS );
	m_PageSize->SetSelection( 0 );
	bMargins->Add( m_PageSize, 0, wxEXPAND|wxALL, 5 );


	bPageSizeSizer->Add( bMargins, 0, wxEXPAND|wxRIGHT, 10 );


	bRightSizer->Add( bPageSizeSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	m_UpperSizer->Add( bRightSizer, 0, wxEXPAND|wxLEFT, 5 );


	bDialogSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );
}

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE()
{
}
