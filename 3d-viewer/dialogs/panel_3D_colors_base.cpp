///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "panel_3D_colors_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_COLORS_BASE::PANEL_3D_COLORS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_environmentLable = new wxStaticText( this, wxID_ANY, _("Environment Colors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_environmentLable->Wrap( -1 );
	bSizerMain->Add( m_environmentLable, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	backgroundTopLabel = new wxStaticText( this, wxID_ANY, _("Background gradient start:"), wxDefaultPosition, wxDefaultSize, 0 );
	backgroundTopLabel->Wrap( -1 );
	fgSizer1->Add( backgroundTopLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backgroundTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_backgroundTop, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	backgroundBotLabel = new wxStaticText( this, wxID_ANY, _("Background gradient end:"), wxDefaultPosition, wxDefaultSize, 0 );
	backgroundBotLabel->Wrap( -1 );
	fgSizer1->Add( backgroundBotLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backgroundBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_backgroundBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 6, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	solderPasteLabel = new wxStaticText( this, wxID_ANY, _("Solder paste:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderPasteLabel->Wrap( -1 );
	fgSizer1->Add( solderPasteLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_solderPaste = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_solderPaste, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( fgSizer1, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( 0, 15, 0, wxEXPAND, 5 );

	m_boardColorsLabel = new wxStaticText( this, wxID_ANY, _("Board Colors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_boardColorsLabel->Wrap( -1 );
	bSizerMain->Add( m_boardColorsLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	m_boardStackupRB = new wxRadioButton( this, wxID_ANY, _("Use board stackup colors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_boardStackupRB, 0, wxTOP|wxBOTTOM|wxLEFT, 8 );

	m_specificColorsRB = new wxRadioButton( this, wxID_ANY, _("Use colors:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_specificColorsRB, 0, wxTOP|wxBOTTOM|wxLEFT, 8 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	silkscreenTopLabel = new wxStaticText( this, wxID_ANY, _("Silkscreen top:"), wxDefaultPosition, wxDefaultSize, 0 );
	silkscreenTopLabel->Wrap( -1 );
	fgSizer2->Add( silkscreenTopLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 20 );

	m_silkscreenTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_silkscreenTop, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	silkscreenBottomLabel = new wxStaticText( this, wxID_ANY, _("Silkscreen bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	silkscreenBottomLabel->Wrap( -1 );
	fgSizer2->Add( silkscreenBottomLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_silkscreenBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_silkscreenBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( 0, 6, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	solderMaskTopLabel = new wxStaticText( this, wxID_ANY, _("Solder mask top:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderMaskTopLabel->Wrap( -1 );
	fgSizer2->Add( solderMaskTopLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_solderMaskTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_solderMaskTop, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	solderMaskBottomLabel = new wxStaticText( this, wxID_ANY, _("Solder mask bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderMaskBottomLabel->Wrap( -1 );
	fgSizer2->Add( solderMaskBottomLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_solderMaskBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_solderMaskBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( 0, 6, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	surfaceFinishLabel = new wxStaticText( this, wxID_ANY, _("Copper/surface finish:"), wxDefaultPosition, wxDefaultSize, 0 );
	surfaceFinishLabel->Wrap( -1 );
	fgSizer2->Add( surfaceFinishLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_surfaceFinish = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_surfaceFinish, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( 0, 6, 1, wxEXPAND, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	boardBodyLabel = new wxStaticText( this, wxID_ANY, _("Board body:"), wxDefaultPosition, wxDefaultSize, 0 );
	boardBodyLabel->Wrap( -1 );
	fgSizer2->Add( boardBodyLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_boardBody = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_boardBody, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer2, 1, wxEXPAND|wxLEFT, 8 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_3D_COLORS_BASE::~PANEL_3D_COLORS_BASE()
{
}
