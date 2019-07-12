///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_feature_constraints_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::PANEL_SETUP_FEATURE_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* sbFeatureRules;
	sbFeatureRules = new wxBoxSizer( wxVERTICAL );
	
	m_OptAllowBlindBuriedVias = new wxCheckBox( this, wxID_ANY, _("Allow blind/buried vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFeatureRules->Add( m_OptAllowBlindBuriedVias, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptAllowMicroVias = new wxCheckBox( this, wxID_ANY, _("Allow micro vias (uVias)"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFeatureRules->Add( m_OptAllowMicroVias, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	sbFeatureRules->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_OptRequireCourtyards = new wxCheckBox( this, wxID_ANY, _("Require courtyard definitions in footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFeatureRules->Add( m_OptRequireCourtyards, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptOverlappingCourtyards = new wxCheckBox( this, wxID_ANY, _("Prohibit overlapping courtyards"), wxDefaultPosition, wxDefaultSize, 0 );
	sbFeatureRules->Add( m_OptOverlappingCourtyards, 0, wxALL, 5 );
	
	
	sbFeatureRules->Add( 0, 0, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizerArcToPoly;
	bSizerArcToPoly = new wxBoxSizer( wxVERTICAL );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerArcToPoly->Add( m_staticline2, 0, wxEXPAND | wxALL, 2 );
	
	m_stCircleToPolyOpt = new wxStaticText( this, wxID_ANY, _("Arc/circle drawing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyOpt->Wrap( -1 );
	m_stCircleToPolyOpt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizerArcToPoly->Add( m_stCircleToPolyOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 3, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer2->Add( 15, 0, 1, wxEXPAND, 5 );
	
	m_maxErrorTitle = new wxStaticText( this, wxID_ANY, _("Maximum deviation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorTitle->Wrap( -1 );
	m_maxErrorTitle->SetToolTip( _("This is the maximum distance between a circle and the polygonal shape that approximate it.\nThe error max defines the number of segments of this polygon.") );
	
	fgSizer2->Add( m_maxErrorTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );
	
	m_maxErrorCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_maxErrorCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_maxErrorUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorUnits->Wrap( -1 );
	fgSizer2->Add( m_maxErrorUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerArcToPoly->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 5 );
	
	
	sbFeatureRules->Add( bSizerArcToPoly, 0, wxEXPAND|wxTOP, 5 );
	
	m_bSizerPolygonFillOption = new wxBoxSizer( wxVERTICAL );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_bSizerPolygonFillOption->Add( m_staticline1, 0, wxEXPAND | wxALL, 2 );
	
	m_stZoneFilledPolysOpt = new wxStaticText( this, wxID_ANY, _("Zone fill strategy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stZoneFilledPolysOpt->Wrap( -1 );
	m_stZoneFilledPolysOpt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	m_bSizerPolygonFillOption->Add( m_stZoneFilledPolysOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_cbOutlinePolygonBestQ = new wxCheckBox( this, wxID_ANY, _("Stroked outlines (legacy)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_cbOutlinePolygonBestQ, 0, wxALL, 4 );
	
	m_cbOutlinePolygonFastest = new wxCheckBox( this, wxID_ANY, _("Smoothed polygons (best performance)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOutlinePolygonFastest->SetValue(true); 
	bSizer5->Add( m_cbOutlinePolygonFastest, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	
	m_bSizerPolygonFillOption->Add( bSizer5, 1, wxEXPAND|wxLEFT, 15 );
	
	
	sbFeatureRules->Add( m_bSizerPolygonFillOption, 0, wxEXPAND|wxTOP, 5 );
	
	
	bMainSizer->Add( sbFeatureRules, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );
	
	wxBoxSizer* sbFeatureConstraints;
	sbFeatureConstraints = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgFeatureConstraints;
	fgFeatureConstraints = new wxFlexGridSizer( 0, 3, 2, 0 );
	fgFeatureConstraints->AddGrowableCol( 1 );
	fgFeatureConstraints->SetFlexibleDirection( wxBOTH );
	fgFeatureConstraints->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_TrackMinWidthTitle = new wxStaticText( this, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM, 5 );
	
	m_TrackMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthCtrl->SetMinSize( wxSize( 120,-1 ) );
	
	fgFeatureConstraints->Add( m_TrackMinWidthCtrl, 0, wxALIGN_LEFT|wxALIGN_TOP|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TrackMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxALL|wxEXPAND, 5 );
	
	m_ViaMinTitle = new wxStaticText( this, wxID_ANY, _("Minimum via diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_SetViasMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetViasMinSizeCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_ViaMinUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_ViaMinDrillTitle = new wxStaticText( this, wxID_ANY, _("Minimum via drill:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinDrillTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinDrillTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_SetViasMinDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetViasMinDrillCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_ViaMinDrillUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );
	
	m_uviaMinSizeLabel = new wxStaticText( this, wxID_ANY, _("Minimum uVia diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_uviaMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinSizeCtrl, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_uviaMinSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_uviaMinDrillLabel = new wxStaticText( this, wxID_ANY, _("Minimum uVia drill:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	m_uviaMinDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinDrillCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_uviaMinDrillUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_HoleToHoleTitle = new wxStaticText( this, wxID_ANY, _("Minimum hole to hole:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleTitle, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	m_SetHoleToHoleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetHoleToHoleCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_HoleToHoleUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_EdgeClearanceLabel = new wxStaticText( this, wxID_ANY, _("Copper edge clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceLabel, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_EdgeClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_EdgeClearanceCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_EdgeClearanceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceUnits, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	sbFeatureConstraints->Add( fgFeatureConstraints, 1, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	
	bMainSizer->Add( sbFeatureConstraints, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	m_cbOutlinePolygonBestQ->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_cbOutlinePolygonFastest->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
}

PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::~PANEL_SETUP_FEATURE_CONSTRAINTS_BASE()
{
	// Disconnect Events
	m_cbOutlinePolygonBestQ->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_cbOutlinePolygonFastest->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	
}
