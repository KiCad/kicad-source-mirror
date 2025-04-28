///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "panel_3D_opengl_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_OPENGL_OPTIONS_BASE::PANEL_3D_OPENGL_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_renderingLabel = new wxStaticText( this, wxID_ANY, _("Rendering Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_renderingLabel->Wrap( -1 );
	bSizerMain->Add( m_renderingLabel, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 13 );


	bSizerMain->Add( 0, 3, 0, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	m_checkBoxBoundingBoxes = new wxCheckBox( this, wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_checkBoxBoundingBoxes, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxCuThickness = new wxCheckBox( this, wxID_ANY, _("Show copper and tech layers thickness (slow)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_checkBoxCuThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxHighlightOnRollOver = new wxCheckBox( this, wxID_ANY, _("Highlight items on rollover"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMargins->Add( m_checkBoxHighlightOnRollOver, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 0, 0 );

	m_staticText221 = new wxStaticText( this, wxID_ANY, _("Anti-aliasing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText221->Wrap( -1 );
	gSizer1->Add( m_staticText221, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_choiceAntiAliasingChoices[] = { _("Disabled"), _("2x"), _("4x"), _("8x") };
	int m_choiceAntiAliasingNChoices = sizeof( m_choiceAntiAliasingChoices ) / sizeof( wxString );
	m_choiceAntiAliasing = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntiAliasingNChoices, m_choiceAntiAliasingChoices, 0 );
	m_choiceAntiAliasing->SetSelection( 0 );
	m_choiceAntiAliasing->SetToolTip( _("3D-Viewer must be closed and re-opened to apply this setting") );

	gSizer1->Add( m_choiceAntiAliasing, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_selectionColorLabel = new wxStaticText( this, wxID_ANY, _("Selection color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selectionColorLabel->Wrap( -1 );
	gSizer1->Add( m_selectionColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_selectionColorSwatch = new COLOR_SWATCH( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_selectionColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );


	bSizerMargins->Add( gSizer1, 0, 0, 5 );


	bSizerMain->Add( bSizerMargins, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( 0, 15, 0, wxEXPAND, 5 );

	m_movingLabel = new wxStaticText( this, wxID_ANY, _("While Moving"), wxDefaultPosition, wxDefaultSize, 0 );
	m_movingLabel->Wrap( -1 );
	bSizerMain->Add( m_movingLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );


	bSizerMain->Add( 0, 3, 0, wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_checkBoxDisableAAMove = new wxCheckBox( this, wxID_ANY, _("Disable anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBoxDisableAAMove, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveThickness = new wxCheckBox( this, wxID_ANY, _("Disable thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBoxDisableMoveThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveVias = new wxCheckBox( this, wxID_ANY, _("Disable uVia holes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBoxDisableMoveVias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveHoles = new wxCheckBox( this, wxID_ANY, _("Disable all plated holes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBoxDisableMoveHoles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizer3, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_3D_OPENGL_OPTIONS_BASE::~PANEL_3D_OPENGL_OPTIONS_BASE()
{
}
