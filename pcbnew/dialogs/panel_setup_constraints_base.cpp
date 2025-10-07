///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
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

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 0, 5 );
	wxBoxSizer* bScrolledSizer;
	bScrolledSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sbFeatureConstraints;
	sbFeatureConstraints = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgFeatureConstraints;
	fgFeatureConstraints = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgFeatureConstraints->SetFlexibleDirection( wxBOTH );
	fgFeatureConstraints->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fgFeatureConstraints->SetMinSize( wxSize( -1,0 ) );
	m_staticText23 = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText23, 0, wxTOP|wxLEFT, 13 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticline151 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline151, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline16 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline16, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline17 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline17, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline18 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline18, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_bitmapClearance = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_clearanceTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_clearanceCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceCtrl->SetToolTip( _("The minimum clearance between copper items which do not belong to the same net. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_clearanceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_clearanceUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bitmapMinTrackWidth = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinTrackWidth, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_TrackMinWidthTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_TrackMinWidthCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthCtrl->SetToolTip( _("The minimum track width. If set, this can only be reduced by custom rules.") );
	m_TrackMinWidthCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgFeatureConstraints->Add( m_TrackMinWidthCtrl, 0, wxALIGN_LEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_TrackMinWidthUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinConn = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinConn, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_MinConnTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum connection width:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinConnTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinConnTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_MinConnCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MinConnCtrl->SetToolTip( _("The minimum copper width of connected copper items.") );
	m_MinConnCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgFeatureConstraints->Add( m_MinConnCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_MinConnUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinConnUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinConnUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinViaAnnulus = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaAnnulus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaMinAnnulusTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum annular width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ViaMinAnnulusCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusCtrl->SetToolTip( _("The minimum annular ring width. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_ViaMinAnnulusCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_ViaMinAnnulusUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bitmapMinViaDiameter = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDiameter, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaMinTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum via diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_SetViasMinSizeCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SetViasMinSizeCtrl->SetToolTip( _("The minimum via diameter. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_SetViasMinSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_ViaMinUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapHoleClearance = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapHoleClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_HoleClearanceLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Copper to hole clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_HoleClearanceCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleClearanceCtrl->SetToolTip( _("The minimum clearance between a hole and an unassociated copper item. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_HoleClearanceCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_HoleClearanceUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleClearanceUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapEdgeClearance = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapEdgeClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_EdgeClearanceLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Copper to edge clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_EdgeClearanceCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceCtrl->SetToolTip( _("The minimum clearance between the board edge and any copper item. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_EdgeClearanceCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_EdgeClearanceUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bitmapMinGrooveWidth = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinGrooveWidth, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_minGrooveWidthLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum groove for creepage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minGrooveWidthLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_minGrooveWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_minGrooveWidthCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_minGrooveWidthCtrl->SetToolTip( _("The minimum slot width from DRC creepage checks") );

	fgFeatureConstraints->Add( m_minGrooveWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_minGrooveWidthUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minGrooveWidthUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_minGrooveWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticText24 = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Holes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText24, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxLEFT, 13 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_staticline3 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline4 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline5 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline6 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline6, 0, wxEXPAND, 2 );

	m_bitmapMinViaDrill = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDrill, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_MinDrillTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum through hole:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_MinDrillCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MinDrillCtrl->SetToolTip( _("The minimum through-hole size. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_MinDrillCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_MinDrillUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinHoleClearance = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinHoleClearance, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_HoleToHoleTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Hole to hole clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleTitle, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SetHoleToHoleCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SetHoleToHoleCtrl->SetToolTip( _("The minimum clearance between two drilled holes. If set, this can only be reduced by custom rules. (Note: does not apply to milled holes.)") );

	fgFeatureConstraints->Add( m_SetHoleToHoleCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_HoleToHoleUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_staticText25 = new wxStaticText( m_scrolledWindow, wxID_ANY, _("uVias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText25, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxLEFT, 13 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 2 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_staticline8 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline8, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline9 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline9, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline10 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline10, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline11 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline11, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_bitmapMinuViaDiameter = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDiameter, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_uviaMinSizeLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum uVia diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_uviaMinSizeCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_uviaMinSizeCtrl->SetToolTip( _("The minimum diameter for micro-vias. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_uviaMinSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_uviaMinSizeUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_bitmapMinuViaDrill = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDrill, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_uviaMinDrillLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum uVia hole:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5 );

	m_uviaMinDrillCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_uviaMinDrillCtrl->SetToolTip( _("The minimum micro-via hole size. If set, this can only be reduced by custom rules.") );

	fgFeatureConstraints->Add( m_uviaMinDrillCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_uviaMinDrillUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticText28 = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Silk"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText28, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxLEFT, 13 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticline111 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline111, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline12 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline12, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline13 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline13, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_staticline14 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline14, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_silkClearanceLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum item clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_silkClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_silkClearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_silkClearanceCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_silkClearanceCtrl->SetToolTip( _("Minimum clearance between two items on the same silkscreen layer. If set this can improve legibility.  (Note: does not apply to multiple shapes within a single footprint.)") );

	fgFeatureConstraints->Add( m_silkClearanceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 5 );

	m_silkClearanceUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_silkClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_silkClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textHeightLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textHeightLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_textHeightLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_textHeightCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_textHeightCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_textHeightUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textHeightUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_textHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_textThicknessLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum text thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textThicknessLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_textThicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_textThicknessCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_textThicknessCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_textThicknessUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textThicknessUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_textThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );


	sbFeatureConstraints->Add( fgFeatureConstraints, 1, wxEXPAND, 5 );


	bScrolledSizer->Add( sbFeatureConstraints, 0, wxEXPAND|wxRIGHT, 5 );


	bScrolledSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxBoxSizer* sbFeatureRules;
	sbFeatureRules = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerArcToPoly;
	bSizerArcToPoly = new wxBoxSizer( wxVERTICAL );

	m_stCircleToPolyOpt = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Arc/Circle Approximations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyOpt->Wrap( -1 );
	bSizerArcToPoly->Add( m_stCircleToPolyOpt, 0, wxTOP|wxLEFT, 13 );

	m_staticline19 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerArcToPoly->Add( m_staticline19, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 3, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_maxErrorTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Maximum allowed deviation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorTitle->Wrap( -1 );
	m_maxErrorTitle->SetToolTip( _("This is the maximum distance between a circle and the polygonal shape that approximate it.\nThe error max defines the number of segments of this polygon.") );

	fgSizer2->Add( m_maxErrorTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_maxErrorCtrl = new wxTextCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorCtrl->SetToolTip( _("The maximum allowed deviation between a true arc or circle and segments used to approximate it.  Smaller values produce smoother graphics at the expense of performance.") );

	fgSizer2->Add( m_maxErrorCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_maxErrorUnits = new wxStaticText( m_scrolledWindow, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorUnits->Wrap( -1 );
	fgSizer2->Add( m_maxErrorUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerArcToPoly->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 5 );

	m_stCircleToPolyWarning = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Note: zone filling can be slow when < %s."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyWarning->Wrap( -1 );
	bSizerArcToPoly->Add( m_stCircleToPolyWarning, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbFeatureRules->Add( bSizerArcToPoly, 0, wxEXPAND, 5 );

	m_bSizerPolygonFillOption = new wxBoxSizer( wxVERTICAL );

	m_stZoneFilledPolysOpt = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Zone Fill Strategy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stZoneFilledPolysOpt->Wrap( -1 );
	m_bSizerPolygonFillOption->Add( m_stZoneFilledPolysOpt, 0, wxTOP|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_bSizerPolygonFillOption->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_filletBitmap = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_filletBitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_allowExternalFilletsOpt = new wxCheckBox( m_scrolledWindow, wxID_ANY, _("Allow fillets/chamfers outside zone outline"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_allowExternalFilletsOpt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_bSizerPolygonFillOption->Add( bSizer9, 0, wxEXPAND|wxTOP, 7 );

	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxHORIZONTAL );

	m_spokeBitmap = new wxStaticBitmap( m_scrolledWindow, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer111->Add( m_spokeBitmap, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_minResolvedSpokesLabel = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Minimum thermal relief spoke count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minResolvedSpokesLabel->Wrap( -1 );
	bSizer111->Add( m_minResolvedSpokesLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_minResolvedSpokeCountCtrl = new wxSpinCtrl( m_scrolledWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer111->Add( m_minResolvedSpokeCountCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	m_bSizerPolygonFillOption->Add( bSizer111, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	sbFeatureRules->Add( m_bSizerPolygonFillOption, 0, wxEXPAND|wxTOP, 10 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_staticText33 = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Length Tuning"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText33->Wrap( -1 );
	bSizer11->Add( m_staticText33, 0, wxTOP|wxLEFT, 13 );

	m_staticline15 = new wxStaticLine( m_scrolledWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer11->Add( m_staticline15, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_useHeightForLengthCalcs = new wxCheckBox( m_scrolledWindow, wxID_ANY, _("Include stackup height in track length calculations"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useHeightForLengthCalcs->SetToolTip( _("When enabled, the distance between copper layers will be included in track length calculations for tracks with vias.  When disabled, via stackup height is ignored.") );

	bSizer11->Add( m_useHeightForLengthCalcs, 0, wxALL, 5 );


	sbFeatureRules->Add( bSizer11, 1, wxEXPAND, 5 );


	bScrolledSizer->Add( sbFeatureRules, 0, wxEXPAND|wxRIGHT, 5 );


	bScrolledSizer->Add( 0, 0, 1, wxEXPAND, 0 );


	m_scrolledWindow->SetSizer( bScrolledSizer );
	m_scrolledWindow->Layout();
	bScrolledSizer->Fit( m_scrolledWindow );
	bMainSizer->Add( m_scrolledWindow, 1, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

PANEL_SETUP_CONSTRAINTS_BASE::~PANEL_SETUP_CONSTRAINTS_BASE()
{
}
