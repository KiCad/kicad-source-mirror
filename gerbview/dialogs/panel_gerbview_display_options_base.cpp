///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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
	bRightSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

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
	bRightSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bDrawingModeSizer;
	bDrawingModeSizer = new wxBoxSizer( wxVERTICAL );

	m_OptDisplayFlashedItems = new wxCheckBox( this, wxID_ANY, _("Sketch flashed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayFlashedItems->SetToolTip( _("Display flashed items (items drawn using standard or macro apertures) in outlines mode") );

	bDrawingModeSizer->Add( m_OptDisplayFlashedItems, 0, wxALL, 5 );

	m_OptDisplayLines = new wxCheckBox( this, wxID_ANY, _("Sketch lines"), wxDefaultPosition, wxDefaultSize, 0 );
	bDrawingModeSizer->Add( m_OptDisplayLines, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayPolygons = new wxCheckBox( this, wxID_ANY, _("Sketch polygons"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayPolygons->SetValue(true);
	m_OptDisplayPolygons->SetToolTip( _("Display polygon items in outline mode") );

	bDrawingModeSizer->Add( m_OptDisplayPolygons, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextOpacity = new wxStaticText( this, wxID_ANY, _("Forced opacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOpacity->Wrap( -1 );
	m_staticTextOpacity->SetToolTip( _("Opacity in forced opacity display mode") );

	bSizer9->Add( m_staticTextOpacity, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_spOpacityCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxT("0.6"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0.2, 1, 0.600000, 0.1 );
	m_spOpacityCtrl->SetDigits( 2 );
	bSizer9->Add( m_spOpacityCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bDrawingModeSizer->Add( bSizer9, 1, wxEXPAND|wxRIGHT, 5 );


	bRightSizer->Add( bDrawingModeSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Page Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bRightSizer->Add( m_staticText3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bRightSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bPageSizeSizer;
	bPageSizeSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_pageSizeFull = new wxRadioButton( this, wxID_ANY, _("Full size"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bMargins->Add( m_pageSizeFull, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeA4 = new wxRadioButton( this, wxID_ANY, _("Size A4"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeA4, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeA3 = new wxRadioButton( this, wxID_ANY, _("Size A3"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeA3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeA2 = new wxRadioButton( this, wxID_ANY, _("Size A2"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeA2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeA = new wxRadioButton( this, wxID_ANY, _("Size A"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeA, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeB = new wxRadioButton( this, wxID_ANY, _("Size B"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeB, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_pageSizeC = new wxRadioButton( this, wxID_ANY, _("Size C"), wxDefaultPosition, wxDefaultSize, 0 );
	bMargins->Add( m_pageSizeC, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bPageSizeSizer->Add( bMargins, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bRightSizer->Add( bPageSizeSizer, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	m_UpperSizer->Add( bRightSizer, 0, wxEXPAND|wxLEFT, 5 );


	bDialogSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();
}

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE()
{
}
