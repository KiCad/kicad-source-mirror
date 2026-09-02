///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_project_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PROJECT_DEFAULTS_BASE::PANEL_PROJECT_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );


	bupperSizer->Add( 15, 0, 0, 0, 5 );

	m_optionsBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* pcbPage;
	pcbPage = new wxPanel( m_optionsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* pcbOptionsSizer;
	pcbOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );


	bMargins->Add( 0, 8, 0, 0, 5 );

	m_defaultOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Default Opacity"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultOpacityLabel->Wrap( -1 );
	bMargins->Add( m_defaultOpacityLabel, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 13 );

	m_staticline6 = new wxStaticLine( pcbPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMargins->Add( m_staticline6, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_trackOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Tracks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_trackOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptTrackOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 0, 100, 100 );
    fgSizer1->Add( m_OptTrackOpacity, 0, wxALL, 5 );

	m_opacityUnit = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit, 0, wxALIGN_CENTER|wxALL, 5 );

	m_viaOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_viaOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptViaOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 0, 100, 100 );
    fgSizer1->Add( m_OptViaOpacity, 0, wxALL, 5 );

	m_opacityUnit1 = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit1->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit1, 0, wxALIGN_CENTER|wxALL, 5 );

	m_padOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_padOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptPadOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 0, 100, 100 );
    fgSizer1->Add( m_OptPadOpacity, 0, wxALL, 5 );

	m_opacityUnit2 = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit2->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit2, 0, wxALIGN_CENTER|wxALL, 5 );

	m_zoneOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoneOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_zoneOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptZoneOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                       wxSP_ARROW_KEYS, 0, 100, 60 );
    fgSizer1->Add( m_OptZoneOpacity, 0, wxALL, 5 );

	m_opacityUnit3 = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit3->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit3, 0, wxALIGN_CENTER, 5 );

	m_filledShapesOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Shapes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filledShapesOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_filledShapesOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptFilledShapeOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                              wxSP_ARROW_KEYS, 0, 100, 100 );
    fgSizer1->Add( m_OptFilledShapeOpacity, 0, wxALL, 5 );

	m_opacityUnit4 = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit4->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit4, 0, wxALIGN_CENTER|wxALL, 5 );

	m_imageOpacityLabel = new wxStaticText( pcbPage, wxID_ANY, _("Images:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_imageOpacityLabel->Wrap( -1 );
	fgSizer1->Add( m_imageOpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_OptImageOpacity = new wxSpinCtrl( pcbPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 0, 100, 60 );
    fgSizer1->Add( m_OptImageOpacity, 0, wxALL, 5 );

	m_opacityUnit5 = new wxStaticText( pcbPage, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_opacityUnit5->Wrap( -1 );
	fgSizer1->Add( m_opacityUnit5, 0, wxALIGN_CENTER|wxALL, 5 );


	bMargins->Add( fgSizer1, 1, wxEXPAND, 5 );


	bMargins->Add( 0, 0, 1, wxEXPAND, 5 );


	pcbOptionsSizer->Add( bMargins, 0, wxEXPAND, 5 );


	pcbPage->SetSizer( pcbOptionsSizer );
	pcbPage->Layout();
	pcbOptionsSizer->Fit( pcbPage );
	m_optionsBook->AddPage( pcbPage, _("PCB Editor"), true );

	bupperSizer->Add( m_optionsBook, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_PROJECT_DEFAULTS_BASE::~PANEL_PROJECT_DEFAULTS_BASE()
{
}
