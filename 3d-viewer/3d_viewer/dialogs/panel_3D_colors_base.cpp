///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "panel_3D_colors_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_COLORS_BASE::PANEL_3D_COLORS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	backgroundTopLabel = new wxStaticText( this, wxID_ANY, _("Background top:"), wxDefaultPosition, wxDefaultSize, 0 );
	backgroundTopLabel->Wrap( -1 );
	fgSizer1->Add( backgroundTopLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backgroundTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_backgroundTop, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	backgroundBotLabel = new wxStaticText( this, wxID_ANY, _("Background bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	backgroundBotLabel->Wrap( -1 );
	fgSizer1->Add( backgroundBotLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backgroundBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_backgroundBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	silkscreenTopLabel = new wxStaticText( this, wxID_ANY, _("Silkscreen top:"), wxDefaultPosition, wxDefaultSize, 0 );
	silkscreenTopLabel->Wrap( -1 );
	fgSizer1->Add( silkscreenTopLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_silkscreenTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_silkscreenTop, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	silkscreenBottomLabel = new wxStaticText( this, wxID_ANY, _("Silkscreen bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	silkscreenBottomLabel->Wrap( -1 );
	fgSizer1->Add( silkscreenBottomLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_silkscreenBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_silkscreenBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	solderMaskTopLabel = new wxStaticText( this, wxID_ANY, _("Solder mask top:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderMaskTopLabel->Wrap( -1 );
	fgSizer1->Add( solderMaskTopLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_solderMaskTop = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_solderMaskTop, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	solderMaskBottomLabel = new wxStaticText( this, wxID_ANY, _("Solder mask bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderMaskBottomLabel->Wrap( -1 );
	fgSizer1->Add( solderMaskBottomLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_solderMaskBottom = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_solderMaskBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	solderPasteLabel = new wxStaticText( this, wxID_ANY, _("Solder paste:"), wxDefaultPosition, wxDefaultSize, 0 );
	solderPasteLabel->Wrap( -1 );
	fgSizer1->Add( solderPasteLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_solderPaste = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_solderPaste, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	surfaceFinishLabel = new wxStaticText( this, wxID_ANY, _("Copper/surface finish:"), wxDefaultPosition, wxDefaultSize, 0 );
	surfaceFinishLabel->Wrap( -1 );
	fgSizer1->Add( surfaceFinishLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_surfaceFinish = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_surfaceFinish, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 10, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	boardBodyLabel = new wxStaticText( this, wxID_ANY, _("Board body:"), wxDefaultPosition, wxDefaultSize, 0 );
	boardBodyLabel->Wrap( -1 );
	fgSizer1->Add( boardBodyLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_boardBody = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_boardBody, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMargins->Add( fgSizer1, 0, wxTOP, 5 );


	bSizerMargins->Add( 0, 0, 1, wxEXPAND, 5 );

	m_loadStackup = new wxButton( this, wxID_ANY, _("Load Colors from Board Stackup"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_loadStackup, 0, wxALL, 5 );


	bSizerMain->Add( bSizerMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_loadStackup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_3D_COLORS_BASE::OnLoadColorsFromBoardStackup ), NULL, this );
}

PANEL_3D_COLORS_BASE::~PANEL_3D_COLORS_BASE()
{
	// Disconnect Events
	m_loadStackup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_3D_COLORS_BASE::OnLoadColorsFromBoardStackup ), NULL, this );

}
