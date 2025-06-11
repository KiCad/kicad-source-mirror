///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_teardrops_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TEARDROPS_BASE::PANEL_SETUP_TEARDROPS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_gridSizer = new wxBoxSizer( wxVERTICAL );

	m_roundShapesLabel = new wxStaticText( this, wxID_ANY, _("Default Properties for Round Shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_roundShapesLabel->Wrap( -1 );
	m_gridSizer->Add( m_roundShapesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerShapeColumns;
	bSizerShapeColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol->Add( m_bitmapTeardrop, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns->Add( bSizerLeftCol, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns->Add( 10, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer;
	gbSizer = new wxGridBagSizer( 2, 3 );
	gbSizer->SetFlexibleDirection( wxBOTH );
	gbSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer->SetEmptyCellSize( wxSize( 10,7 ) );

	m_stLenPercentLabel = new wxStaticText( this, wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentLabel->Wrap( -1 );
	gbSizer->Add( m_stLenPercentLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spLenPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spLenPercent->SetDigits( 0 );
	gbSizer->Add( m_spLenPercent, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stLenPercentUnits;
	stLenPercentUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercentUnits->Wrap( -1 );
	bSizer131->Add( stLenPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercentHint;
	stLenPercentHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercentHint->Wrap( -1 );
	stLenPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer131->Add( stLenPercentHint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* staticText76;
	staticText76 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText76->Wrap( -1 );
	bSizer131->Add( staticText76, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer->Add( bSizer131, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen = new wxStaticText( this, wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	gbSizer->Add( m_stMaxLen, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_tcTdMaxLen, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits->Wrap( -1 );
	gbSizer->Add( m_stMaxLenUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidthLabel = new wxStaticText( this, wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthLabel->Wrap( -1 );
	gbSizer->Add( m_stWidthLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spWidthPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spWidthPercent->SetDigits( 0 );
	gbSizer->Add( m_spWidthPercent, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stWidthPercentUnits;
	stWidthPercentUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercentUnits->Wrap( -1 );
	bSizer13->Add( stWidthPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stWidthPercentHint;
	stWidthPercentHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercentHint->Wrap( -1 );
	stWidthPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer13->Add( stWidthPercentHint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stWidthPercentSuffix;
	stWidthPercentSuffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercentSuffix->Wrap( -1 );
	bSizer13->Add( stWidthPercentSuffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer->Add( bSizer13, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthLabel = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthLabel->Wrap( -1 );
	gbSizer->Add( m_stMaxWidthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_tcMaxWidth, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthUnits->Wrap( -1 );
	gbSizer->Add( m_stMaxWidthUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPreferZoneConnection->SetToolTip( _("Do not create teardrops on tracks connected to pads that are also connected to a copper zone.") );

	gbSizer->Add( m_cbPreferZoneConnection, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_stHDRatio = new wxStaticText( this, wxID_ANY, _("Track width limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer->Add( m_stHDRatio, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent->SetDigits( 0 );
	m_spTeardropHDPercent->SetToolTip( _("Tracks which are similar in size to the pad or via do not need teardrops.") );

	gbSizer->Add( m_spTeardropHDPercent, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1311;
	bSizer1311 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* minTrackWidthUnits;
	minTrackWidthUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	minTrackWidthUnits->Wrap( -1 );
	bSizer1311->Add( minTrackWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stMinTrackWidthHint;
	stMinTrackWidthHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthHint->Wrap( -1 );
	stMinTrackWidthHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1311->Add( stMinTrackWidthHint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stMinTrackWidthSuffix;
	stMinTrackWidthSuffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthSuffix->Wrap( -1 );
	bSizer1311->Add( stMinTrackWidthSuffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer->Add( bSizer1311, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack->SetValue(true);
	m_cbTeardropsUseNextTrack->SetToolTip( _("Allows a teardrop to extend over the first 2 connected track segments if the first track segment is too short to accommodate the best length.") );

	gbSizer->Add( m_cbTeardropsUseNextTrack, wxGBPosition( 0, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_cbCurvedEdges = new wxCheckBox( this, wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_cbCurvedEdges, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), 0, 5 );


	bSizerShapeColumns->Add( gbSizer, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns, 1, wxEXPAND|wxTOP, 5 );


	m_gridSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_rectShapesLabel = new wxStaticText( this, wxID_ANY, _("Default Properties for Rectangular Shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectShapesLabel->Wrap( -1 );
	m_gridSizer->Add( m_rectShapesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerShapeColumns1;
	bSizerShapeColumns1 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol1;
	bSizerLeftCol1 = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop1 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol1->Add( m_bitmapTeardrop1, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns1->Add( bSizerLeftCol1, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns1->Add( 10, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 10,7 ) );

	m_stLenPercent1Label = new wxStaticText( this, wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent1Label->Wrap( -1 );
	gbSizer1->Add( m_stLenPercent1Label, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spLenPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spLenPercent1->SetDigits( 0 );
	gbSizer1->Add( m_spLenPercent1, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer122;
	bSizer122 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stLenPercent1Units;
	stLenPercent1Units = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent1Units->Wrap( -1 );
	bSizer122->Add( stLenPercent1Units, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercent1Hint;
	stLenPercent1Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent1Hint->Wrap( -1 );
	stLenPercent1Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer122->Add( stLenPercent1Hint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stLenPercent1Suffix;
	stLenPercent1Suffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent1Suffix->Wrap( -1 );
	bSizer122->Add( stLenPercent1Suffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer122, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen1 = new wxStaticText( this, wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen1->Wrap( -1 );
	gbSizer1->Add( m_stMaxLen1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcTdMaxLen1, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLen1Units = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen1Units->Wrap( -1 );
	gbSizer1->Add( m_stMaxLen1Units, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidth1Label = new wxStaticText( this, wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidth1Label->Wrap( -1 );
	gbSizer1->Add( m_stWidth1Label, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spWidthPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spWidthPercent1->SetDigits( 0 );
	gbSizer1->Add( m_spWidthPercent1, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stWidthPercent1Units;
	stWidthPercent1Units = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercent1Units->Wrap( -1 );
	bSizer12->Add( stWidthPercent1Units, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stSizePercent1Hint;
	stSizePercent1Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stSizePercent1Hint->Wrap( -1 );
	stSizePercent1Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer12->Add( stSizePercent1Hint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stSizePercent1Suffix;
	stSizePercent1Suffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stSizePercent1Suffix->Wrap( -1 );
	bSizer12->Add( stSizePercent1Suffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer12, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidth1Label = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidth1Label->Wrap( -1 );
	gbSizer1->Add( m_stMaxWidth1Label, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcMaxWidth1, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidth1Units = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidth1Units->Wrap( -1 );
	gbSizer1->Add( m_stMaxWidth1Units, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection1 = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPreferZoneConnection1->SetToolTip( _("Do not create teardrops on tracks connected to pads that are also connected to a copper zone.") );

	gbSizer1->Add( m_cbPreferZoneConnection1, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_stHDRatio1 = new wxStaticText( this, wxID_ANY, _("Track width limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio1->Wrap( -1 );
	m_stHDRatio1->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer1->Add( m_stHDRatio1, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent1->SetDigits( 0 );
	m_spTeardropHDPercent1->SetToolTip( _("Tracks which are similar in size to the pad do not need teardrops.") );

	gbSizer1->Add( m_spTeardropHDPercent1, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1221;
	bSizer1221 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* minTrackWidthUnits1;
	minTrackWidthUnits1 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	minTrackWidthUnits1->Wrap( -1 );
	bSizer1221->Add( minTrackWidthUnits1, 0, 0, 5 );

	wxStaticText* stMinTrackWidthHint1;
	stMinTrackWidthHint1 = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthHint1->Wrap( -1 );
	stMinTrackWidthHint1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1221->Add( stMinTrackWidthHint1, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* staticText73;
	staticText73 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText73->Wrap( -1 );
	bSizer1221->Add( staticText73, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer1221, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack1 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack1->SetValue(true);
	m_cbTeardropsUseNextTrack1->SetToolTip( _("Allows a teardrop to extend over the first 2 connected track segments if the first track segment is too short to accommodate the best length.") );

	gbSizer1->Add( m_cbTeardropsUseNextTrack1, wxGBPosition( 0, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_cbCurvedEdges1 = new wxCheckBox( this, wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbCurvedEdges1, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), 0, 5 );


	bSizerShapeColumns1->Add( gbSizer1, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns1, 1, wxEXPAND|wxTOP, 5 );


	m_gridSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_tracksLabel = new wxStaticText( this, wxID_ANY, _("Properties for Track-to-Track Teardrops"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tracksLabel->Wrap( -1 );
	m_gridSizer->Add( m_tracksLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerShapeColumns2;
	bSizerShapeColumns2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol2;
	bSizerLeftCol2 = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop2 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol2->Add( m_bitmapTeardrop2, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns2->Add( bSizerLeftCol2, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns2->Add( 10, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 2, 3 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( 10,7 ) );

	m_stLenPercent2Label = new wxStaticText( this, wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent2Label->Wrap( -1 );
	gbSizer2->Add( m_stLenPercent2Label, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spLenPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spLenPercent2->SetDigits( 0 );
	gbSizer2->Add( m_spLenPercent2, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer1211;
	bSizer1211 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stLenPercent2Units;
	stLenPercent2Units = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent2Units->Wrap( -1 );
	bSizer1211->Add( stLenPercent2Units, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercent2Hint;
	stLenPercent2Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent2Hint->Wrap( -1 );
	stLenPercent2Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1211->Add( stLenPercent2Hint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stLenPercent2Suffix;
	stLenPercent2Suffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent2Suffix->Wrap( -1 );
	bSizer1211->Add( stLenPercent2Suffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->Add( bSizer1211, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen2 = new wxStaticText( this, wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen2->Wrap( -1 );
	gbSizer2->Add( m_stMaxLen2, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcTdMaxLen2, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLen2Units = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen2Units->Wrap( -1 );
	gbSizer2->Add( m_stMaxLen2Units, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidth2Label = new wxStaticText( this, wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidth2Label->Wrap( -1 );
	gbSizer2->Add( m_stWidth2Label, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spWidthPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spWidthPercent2->SetDigits( 0 );
	gbSizer2->Add( m_spWidthPercent2, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stWidthPercent2Units;
	stWidthPercent2Units = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercent2Units->Wrap( -1 );
	bSizer121->Add( stWidthPercent2Units, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stWidthPercent2Hint;
	stWidthPercent2Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercent2Hint->Wrap( -1 );
	stWidthPercent2Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer121->Add( stWidthPercent2Hint, 0, wxALIGN_BOTTOM, 1 );

	wxStaticText* stWidthPercent2Suffix;
	stWidthPercent2Suffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercent2Suffix->Wrap( -1 );
	bSizer121->Add( stWidthPercent2Suffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->Add( bSizer121, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidth2Label = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidth2Label->Wrap( -1 );
	gbSizer2->Add( m_stMaxWidth2Label, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcMaxWidth2, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidth2Units = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidth2Units->Wrap( -1 );
	gbSizer2->Add( m_stMaxWidth2Units, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stHDRatio2 = new wxStaticText( this, wxID_ANY, _("Track width limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio2->Wrap( -1 );
	m_stHDRatio2->SetToolTip( _("Max track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer2->Add( m_stHDRatio2, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent2->SetDigits( 0 );
	m_spTeardropHDPercent2->SetToolTip( _("Tracks which are similar in size do not need teardrops.") );

	gbSizer2->Add( m_spTeardropHDPercent2, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer12211;
	bSizer12211 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* stMinTrackWidth2Units;
	stMinTrackWidth2Units = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidth2Units->Wrap( -1 );
	bSizer12211->Add( stMinTrackWidth2Units, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stMinTrackWidth2Hint;
	stMinTrackWidth2Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidth2Hint->Wrap( -1 );
	stMinTrackWidth2Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer12211->Add( stMinTrackWidth2Hint, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stMinTrackWidth2Suffix;
	stMinTrackWidth2Suffix = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidth2Suffix->Wrap( -1 );
	bSizer12211->Add( stMinTrackWidth2Suffix, 0, wxALIGN_BOTTOM|wxBOTTOM, 1 );


	gbSizer2->Add( bSizer12211, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack2 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack2->SetValue(true);
	m_cbTeardropsUseNextTrack2->SetToolTip( _("Allows a teardrop to extend over the first 2 connected track segments if the first track segment is too short to accommodate the best length.") );

	gbSizer2->Add( m_cbTeardropsUseNextTrack2, wxGBPosition( 0, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_cbCurvedEdges2 = new wxCheckBox( this, wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_cbCurvedEdges2, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), 0, 5 );


	bSizerShapeColumns2->Add( gbSizer2, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns2, 1, wxEXPAND|wxTOP, 5 );


	m_gridSizer->Add( 0, 5, 0, wxEXPAND, 5 );


	mainSizer->Add( m_gridSizer, 0, wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_TEARDROPS_BASE::~PANEL_SETUP_TEARDROPS_BASE()
{
}
