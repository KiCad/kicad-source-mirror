///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_snapping_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SNAPPING_BASE::PANEL_SNAPPING_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_gridSnappingLabel = new wxStaticText( this, wxID_ANY, _("Grid Snapping"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridSnappingLabel->Wrap( -1 );
	mainSizer->Add( m_gridSnappingLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_gridSnappingLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_gridSnappingLine, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgGridSnapping;
	fgGridSnapping = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgGridSnapping->SetFlexibleDirection( wxVERTICAL );
	fgGridSnapping->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_gridSnapLabel = new wxStaticText( this, wxID_ANY, _("Snap to grid:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridSnapLabel->Wrap( -1 );
	fgGridSnapping->Add( m_gridSnapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_gridSnapChoiceChoices[] = { _("Always"), _("When grid shown"), _("Never") };
	int m_gridSnapChoiceNChoices = sizeof( m_gridSnapChoiceChoices ) / sizeof( wxString );
	m_gridSnapChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridSnapChoiceNChoices, m_gridSnapChoiceChoices, 0 );
	m_gridSnapChoice->SetSelection( 0 );
	m_gridSnapChoice->SetToolTip( _("Snap the cursor to the active grid") );

	fgGridSnapping->Add( m_gridSnapChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	mainSizer->Add( fgGridSnapping, 0, wxEXPAND|wxALL, 5 );

	m_objectSnappingLabel = new wxStaticText( this, wxID_ANY, _("Object Snapping"), wxDefaultPosition, wxDefaultSize, 0 );
	m_objectSnappingLabel->Wrap( -1 );
	mainSizer->Add( m_objectSnappingLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_objectSnappingLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_objectSnappingLine, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_sizerObjectSnapping = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgObjectSnapping;
	fgObjectSnapping = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgObjectSnapping->SetFlexibleDirection( wxVERTICAL );
	fgObjectSnapping->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_padSnapLabel = new wxStaticText( this, wxID_ANY, _("Snap to pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padSnapLabel->Wrap( -1 );
	fgObjectSnapping->Add( m_padSnapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_padSnapChoiceChoices;
	m_padSnapChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_padSnapChoiceChoices, 0 );
	m_padSnapChoice->SetSelection( 0 );
	m_padSnapChoice->SetToolTip( _("Capture cursor when the mouse enters a pad area") );

	fgObjectSnapping->Add( m_padSnapChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_trackSnapLabel = new wxStaticText( this, wxID_ANY, _("Snap to tracks and vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackSnapLabel->Wrap( -1 );
	fgObjectSnapping->Add( m_trackSnapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_trackSnapChoiceChoices;
	m_trackSnapChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trackSnapChoiceChoices, 0 );
	m_trackSnapChoice->SetSelection( 0 );
	m_trackSnapChoice->SetToolTip( _("Capture cursor when the mouse approaches a track") );

	fgObjectSnapping->Add( m_trackSnapChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_graphicsSnapLabel = new wxStaticText( this, wxID_ANY, _("Snap to graphics:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_graphicsSnapLabel->Wrap( -1 );
	fgObjectSnapping->Add( m_graphicsSnapLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_graphicsSnapChoiceChoices;
	m_graphicsSnapChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_graphicsSnapChoiceChoices, 0 );
	m_graphicsSnapChoice->SetSelection( 0 );
	m_graphicsSnapChoice->SetToolTip( _("Capture cursor when the mouse approaches graphical control points") );

	fgObjectSnapping->Add( m_graphicsSnapChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	m_sizerObjectSnapping->Add( fgObjectSnapping, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_snapAllLayers = new wxCheckBox( this, wxID_ANY, _("Snap to objects on all layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapAllLayers->SetToolTip( _("When unchecked, only objects on the active layer are snapped to") );

	m_sizerObjectSnapping->Add( m_snapAllLayers, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( m_sizerObjectSnapping, 0, wxEXPAND, 0 );

	m_snapInferenceLabel = new wxStaticText( this, wxID_ANY, _("Snap Inference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapInferenceLabel->Wrap( -1 );
	mainSizer->Add( m_snapInferenceLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_snapInferenceLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_snapInferenceLine, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_sizerSnapInference = new wxBoxSizer( wxVERTICAL );

	m_snapObjectGeometry = new wxCheckBox( this, wxID_ANY, _("Object geometry"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapObjectGeometry->SetToolTip( _("Snap to endpoints, midpoints, centres and edges of existing objects") );

	m_sizerSnapInference->Add( m_snapObjectGeometry, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_snapConstructionExtensions = new wxCheckBox( this, wxID_ANY, _("Construction extensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapConstructionExtensions->SetToolTip( _("Snap to virtual extensions of lines and arcs") );

	m_sizerSnapInference->Add( m_snapConstructionExtensions, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_snapTangentNormal = new wxCheckBox( this, wxID_ANY, _("Tangent and normal contacts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapTangentNormal->SetToolTip( _("Snap where the dragged geometry meets an arc tangentially or normally") );

	m_sizerSnapInference->Add( m_snapTangentNormal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_snapAlignmentDistribution = new wxCheckBox( this, wxID_ANY, _("Alignment and equal spacing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_snapAlignmentDistribution->SetToolTip( _("Snap to alignment with, and equal spacing between, nearby objects") );

	m_sizerSnapInference->Add( m_snapAlignmentDistribution, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( m_sizerSnapInference, 0, wxEXPAND, 0 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SNAPPING_BASE::~PANEL_SNAPPING_BASE()
{
}
