///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_constraints_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_CONSTRAINTS_BASE::PANEL_SETUP_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bScrolledSizer;
	bScrolledSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sbFeatureRules;
	sbFeatureRules = new wxBoxSizer( wxVERTICAL );

	m_staticText26 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Allowed features"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	sbFeatureRules->Add( m_staticText26, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerViaOpt;
	fgSizerViaOpt = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerViaOpt->SetFlexibleDirection( wxBOTH );
	fgSizerViaOpt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_bitmapBlindBuried = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_bitmapBlindBuried, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_OptAllowBlindBuriedVias = new wxCheckBox( m_scrolledWindow1, wxID_ANY, _("Allow blind/buried vias"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_OptAllowBlindBuriedVias, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_bitmap_uVia = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_bitmap_uVia, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_OptAllowMicroVias = new wxCheckBox( m_scrolledWindow1, wxID_ANY, _("Allow micro vias (uVias)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_OptAllowMicroVias, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbFeatureRules->Add( fgSizerViaOpt, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerArcToPoly;
	bSizerArcToPoly = new wxBoxSizer( wxVERTICAL );

	m_staticline2 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerArcToPoly->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 15 );

	m_stCircleToPolyOpt = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Arc/circle approximated by segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyOpt->Wrap( -1 );
	bSizerArcToPoly->Add( m_stCircleToPolyOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 3, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer2->Add( 15, 0, 0, 0, 5 );

	m_maxErrorTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Max allowed deviation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorTitle->Wrap( -1 );
	m_maxErrorTitle->SetToolTip( _("This is the maximum distance between a circle and the polygonal shape that approximate it.\nThe error max defines the number of segments of this polygon.") );

	fgSizer2->Add( m_maxErrorTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_maxErrorCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_maxErrorCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_maxErrorUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorUnits->Wrap( -1 );
	fgSizer2->Add( m_maxErrorUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerArcToPoly->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 5 );

	m_stCircleToPolyWarning = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Note: zone filling can be slow when < %s."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyWarning->Wrap( -1 );
	bSizerArcToPoly->Add( m_stCircleToPolyWarning, 0, wxLEFT|wxRIGHT, 5 );


	sbFeatureRules->Add( bSizerArcToPoly, 0, wxEXPAND|wxTOP, 5 );

	m_bSizerPolygonFillOption = new wxBoxSizer( wxVERTICAL );

	m_staticline1 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_bSizerPolygonFillOption->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 15 );

	m_stZoneFilledPolysOpt = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Zone fill strategy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stZoneFilledPolysOpt->Wrap( -1 );
	m_bSizerPolygonFillOption->Add( m_stZoneFilledPolysOpt, 0, wxALL, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapZoneFillOpt = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_bitmapZoneFillOpt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizerOutlinesOpts;
	bSizerOutlinesOpts = new wxBoxSizer( wxVERTICAL );

	m_rbOutlinePolygonBestQ = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("Mimic legacy behavior"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbOutlinePolygonBestQ->SetToolTip( _("Produces a slightly smoother outline at the expense of performance, some export fidelity issues, and overly aggressive higher-priority zone knockouts.") );

	bSizerOutlinesOpts->Add( m_rbOutlinePolygonBestQ, 0, wxALL, 4 );

	m_rbOutlinePolygonFastest = new wxRadioButton( m_scrolledWindow1, wxID_ANY, _("Smoothed polygons (best performance)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbOutlinePolygonFastest->SetValue( true );
	m_rbOutlinePolygonFastest->SetToolTip( _("Better performance, exact export fidelity, and more complete filling near higher-priority zones.") );

	bSizerOutlinesOpts->Add( m_rbOutlinePolygonFastest, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );


	bSizer5->Add( bSizerOutlinesOpts, 1, wxEXPAND, 5 );


	m_bSizerPolygonFillOption->Add( bSizer5, 1, wxEXPAND, 15 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_filletBitmap = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_filletBitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_allowExternalFilletsOpt = new wxCheckBox( m_scrolledWindow1, wxID_ANY, _("Allow fillets outside zone outline"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_allowExternalFilletsOpt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_bSizerPolygonFillOption->Add( bSizer9, 1, wxEXPAND, 5 );


	sbFeatureRules->Add( m_bSizerPolygonFillOption, 0, wxEXPAND|wxTOP, 10 );


	bScrolledSizer->Add( sbFeatureRules, 1, wxEXPAND, 5 );


	bScrolledSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxBoxSizer* sbFeatureConstraints;
	sbFeatureConstraints = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgFeatureConstraints;
	fgFeatureConstraints = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgFeatureConstraints->SetFlexibleDirection( wxBOTH );
	fgFeatureConstraints->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText23 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText23, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxLEFT, 4 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmapClearance = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_clearanceTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_clearanceCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_clearanceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 7 );

	m_clearanceUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bitmapMinTrackWidth = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinTrackWidth, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_TrackMinWidthTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_TrackMinWidthCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgFeatureConstraints->Add( m_TrackMinWidthCtrl, 0, wxALIGN_LEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_TrackMinWidthUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinViaAnnulus = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaAnnulus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaMinAnnulusTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum annular width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ViaMinAnnulusCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 7 );

	m_ViaMinAnnulusUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bitmapMinViaDiameter = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDiameter, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaMinTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum via diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_SetViasMinSizeCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetViasMinSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 7 );

	m_ViaMinUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapHoleClearance = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapHoleClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_HoleClearanceLabel = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Copper hole clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleClearanceLabel, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_HoleClearanceCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_HoleClearanceCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_HoleClearanceUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleClearanceUnits, 0, wxALL, 5 );

	m_bitmapEdgeClearance = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapEdgeClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_EdgeClearanceLabel = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Copper edge clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_EdgeClearanceCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_EdgeClearanceCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_EdgeClearanceUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticline3 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline3, 0, wxTOP|wxEXPAND, 10 );

	m_staticline4 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline4, 0, wxEXPAND|wxTOP, 10 );

	m_staticline5 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline5, 0, wxEXPAND|wxTOP, 10 );

	m_staticline6 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline6, 0, wxEXPAND|wxTOP, 10 );

	m_staticText24 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Holes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText24, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 4 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_bitmapMinViaDrill = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDrill, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_MinDrillTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum through hole:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_MinDrillCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_MinDrillCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_MinDrillUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinHoleClearance = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinHoleClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_HoleToHoleTitle = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Hole to hole clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SetHoleToHoleCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetHoleToHoleCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_HoleToHoleUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticline8 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline8, 0, wxEXPAND|wxTOP, 10 );

	m_staticline9 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline9, 0, wxEXPAND|wxTOP, 10 );

	m_staticline10 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline10, 0, wxEXPAND|wxTOP, 10 );

	m_staticline11 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline11, 0, wxEXPAND|wxTOP, 10 );

	m_staticText25 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("uVias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText25, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 4 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_bitmapMinuViaDiameter = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDiameter, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_uviaMinSizeLabel = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum uVia diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_uviaMinSizeCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_uviaMinSizeUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinuViaDrill = new wxStaticBitmap( m_scrolledWindow1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDrill, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_uviaMinDrillLabel = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum uVia hole:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_uviaMinDrillCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinDrillCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 7 );

	m_uviaMinDrillUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticline111 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline111, 0, wxEXPAND|wxTOP, 10 );

	m_staticline12 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline12, 0, wxEXPAND|wxTOP, 10 );

	m_staticline13 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline13, 0, wxEXPAND|wxTOP, 10 );

	m_staticline14 = new wxStaticLine( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline14, 0, wxEXPAND|wxTOP, 10 );

	m_staticText28 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText28, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 4 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_silkClearanceLabel = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Minimum item clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_silkClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_silkClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_silkClearanceCtrl = new wxTextCtrl( m_scrolledWindow1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_silkClearanceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 7 );

	m_silkClearanceUnits = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_silkClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_silkClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	sbFeatureConstraints->Add( fgFeatureConstraints, 1, wxEXPAND|wxLEFT, 5 );


	bScrolledSizer->Add( sbFeatureConstraints, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_scrolledWindow1->SetSizer( bScrolledSizer );
	m_scrolledWindow1->Layout();
	bScrolledSizer->Fit( m_scrolledWindow1 );
	bMainSizer->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_rbOutlinePolygonBestQ->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_rbOutlinePolygonFastest->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
}

PANEL_SETUP_CONSTRAINTS_BASE::~PANEL_SETUP_CONSTRAINTS_BASE()
{
	// Disconnect Events
	m_rbOutlinePolygonBestQ->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_rbOutlinePolygonFastest->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );

}
