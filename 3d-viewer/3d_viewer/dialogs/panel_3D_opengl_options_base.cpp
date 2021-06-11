///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"

#include "panel_3D_opengl_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_3D_OPENGL_OPTIONS_BASE::PANEL_3D_OPENGL_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerOpenGLRenderoptions;
	sbSizerOpenGLRenderoptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Rendering Options") ), wxVERTICAL );

	m_checkBoxBoundingBoxes = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show model bounding boxes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOpenGLRenderoptions->Add( m_checkBoxBoundingBoxes, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxCuThickness = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Show copper thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOpenGLRenderoptions->Add( m_checkBoxCuThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxHighlightOnRollOver = new wxCheckBox( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Highlight items on rollover"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOpenGLRenderoptions->Add( m_checkBoxHighlightOnRollOver, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 0, 0 );

	m_staticText221 = new wxStaticText( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Anti-aliasing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText221->Wrap( -1 );
	gSizer1->Add( m_staticText221, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_choiceAntiAliasingChoices[] = { _("Disabled"), _("2x"), _("4x"), _("8x") };
	int m_choiceAntiAliasingNChoices = sizeof( m_choiceAntiAliasingChoices ) / sizeof( wxString );
	m_choiceAntiAliasing = new wxChoice( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceAntiAliasingNChoices, m_choiceAntiAliasingChoices, 0 );
	m_choiceAntiAliasing->SetSelection( 0 );
	m_choiceAntiAliasing->SetToolTip( _("3D-Viewer must be closed and re-opened to apply this setting") );

	gSizer1->Add( m_choiceAntiAliasing, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_selectionColorLabel = new wxStaticText( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, _("Selection color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selectionColorLabel->Wrap( -1 );
	gSizer1->Add( m_selectionColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_selectionColorSwatch = new COLOR_SWATCH( sbSizerOpenGLRenderoptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_selectionColorSwatch, 0, wxALL|wxEXPAND, 5 );


	sbSizerOpenGLRenderoptions->Add( gSizer1, 0, 0, 5 );


	bSizerMargins->Add( sbSizerOpenGLRenderoptions, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerWhileMoving;
	sbSizerWhileMoving = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("While Moving") ), wxVERTICAL );

	m_checkBoxDisableAAMove = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable anti-aliasing"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableAAMove, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveThickness = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveVias = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveVias, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkBoxDisableMoveHoles = new wxCheckBox( sbSizerWhileMoving->GetStaticBox(), wxID_ANY, _("Disable holes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerWhileMoving->Add( m_checkBoxDisableMoveHoles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( sbSizerWhileMoving, 0, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerMargins, 1, wxEXPAND|wxRIGHT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_3D_OPENGL_OPTIONS_BASE::~PANEL_3D_OPENGL_OPTIONS_BASE()
{
}
