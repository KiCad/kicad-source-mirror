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

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );

	m_OptDisplayDCodes = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show D codes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OptDisplayDCodes->SetValue(true);
	sbSizer1->Add( m_OptDisplayDCodes, 0, wxBOTTOM|wxLEFT, 5 );


	bRightSizer->Add( sbSizer1, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Drawing Mode") ), wxVERTICAL );

	m_OptDisplayFlashedItems = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Sketch flashed items"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_OptDisplayFlashedItems, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OptDisplayLines = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Sketch lines"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_OptDisplayLines, 0, wxALL, 5 );

	m_OptDisplayPolygons = new wxCheckBox( sbSizer2->GetStaticBox(), wxID_ANY, _("Sketch polygons"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_OptDisplayPolygons, 0, wxALL, 5 );


	bRightSizer->Add( sbSizer2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_UpperSizer->Add( bRightSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bDialogSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );


	this->SetSizer( bDialogSizer );
	this->Layout();
	bDialogSizer->Fit( this );
}

PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE::~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE()
{
}
