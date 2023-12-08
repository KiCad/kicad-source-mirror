///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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

	m_roundShapesLabel = new wxStaticText( this, wxID_ANY, _("Default properties for round shapes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_roundShapesLabel->Wrap( -1 );
	m_gridSizer->Add( m_roundShapesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 10 );

	wxBoxSizer* bSizerShapeColumns;
	bSizerShapeColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol->Add( m_bitmapTeardrop, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );

	m_edgesLabel = new wxStaticText( this, wxID_ANY, _("Edges:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_edgesLabel->Wrap( -1 );
	bSizer41->Add( m_edgesLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rbStraightLines = new wxRadioButton( this, wxID_ANY, _("Straight lines"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer41->Add( m_rbStraightLines, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rbCurved = new wxRadioButton( this, wxID_ANY, _("Curved"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer41->Add( m_rbCurved, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerLeftCol->Add( bSizer41, 0, wxEXPAND|wxBOTTOM, 4 );


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

	m_stLenPercentUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentUnits->Wrap( -1 );
	bSizer131->Add( m_stLenPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercentHint;
	stLenPercentHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercentHint->Wrap( -1 );
	stLenPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer131->Add( stLenPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText76 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText76->Wrap( -1 );
	bSizer131->Add( m_staticText76, 0, wxALIGN_CENTER_VERTICAL, 5 );


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

	m_stWidthPercentUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentUnits->Wrap( -1 );
	bSizer13->Add( m_stWidthPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stWidthPercentHint;
	stWidthPercentHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercentHint->Wrap( -1 );
	stWidthPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer13->Add( stWidthPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText77 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText77->Wrap( -1 );
	bSizer13->Add( m_staticText77, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer->Add( bSizer13, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthLabel = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthLabel->Wrap( -1 );
	gbSizer->Add( m_stMaxWidthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_tcMaxWidth, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthUnits->Wrap( -1 );
	gbSizer->Add( m_stMaxWidthUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel->Wrap( -1 );
	gbSizer->Add( m_curvePointsLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer->Add( m_curvePointsCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_cbPreferZoneConnection, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_stHDRatio = new wxStaticText( this, wxID_ANY, _("Maximum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer->Add( m_stHDRatio, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent->SetDigits( 0 );
	m_spTeardropHDPercent->SetToolTip( _("Tracks which are similar in size to the pad or via do not need teardrops.") );

	gbSizer->Add( m_spTeardropHDPercent, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1311;
	bSizer1311 = new wxBoxSizer( wxHORIZONTAL );

	m_minTrackWidthUnits = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits->Wrap( -1 );
	bSizer1311->Add( m_minTrackWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stMinTrackWidthHint;
	stMinTrackWidthHint = new wxStaticText( this, wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthHint->Wrap( -1 );
	stMinTrackWidthHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1311->Add( stMinTrackWidthHint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText78 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText78->Wrap( -1 );
	bSizer1311->Add( m_staticText78, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer->Add( bSizer1311, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack->SetValue(true);
	gbSizer->Add( m_cbTeardropsUseNextTrack, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns->Add( gbSizer, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns, 1, wxEXPAND, 5 );


	m_gridSizer->Add( 0, 30, 0, wxEXPAND, 5 );

	m_rectShapesLabel = new wxStaticText( this, wxID_ANY, _("Default properties for rectangular shapes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectShapesLabel->Wrap( -1 );
	m_gridSizer->Add( m_rectShapesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 10 );

	wxBoxSizer* bSizerShapeColumns1;
	bSizerShapeColumns1 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol1;
	bSizerLeftCol1 = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop1 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol1->Add( m_bitmapTeardrop1, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer411;
	bSizer411 = new wxBoxSizer( wxHORIZONTAL );

	m_edgesLabel1 = new wxStaticText( this, wxID_ANY, _("Edges:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_edgesLabel1->Wrap( -1 );
	bSizer411->Add( m_edgesLabel1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rbStraightLines1 = new wxRadioButton( this, wxID_ANY, _("Straight lines"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer411->Add( m_rbStraightLines1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rbCurved1 = new wxRadioButton( this, wxID_ANY, _("Curved"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer411->Add( m_rbCurved1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerLeftCol1->Add( bSizer411, 0, wxEXPAND|wxBOTTOM, 4 );


	bSizerShapeColumns1->Add( bSizerLeftCol1, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns1->Add( 10, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 10,7 ) );

	m_stLenPercentLabel1 = new wxStaticText( this, wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentLabel1->Wrap( -1 );
	gbSizer1->Add( m_stLenPercentLabel1, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spLenPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spLenPercent1->SetDigits( 0 );
	gbSizer1->Add( m_spLenPercent1, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer122;
	bSizer122 = new wxBoxSizer( wxHORIZONTAL );

	m_stLenPercentUnits1 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentUnits1->Wrap( -1 );
	bSizer122->Add( m_stLenPercentUnits1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercent1Hint;
	stLenPercent1Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercent1Hint->Wrap( -1 );
	stLenPercent1Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer122->Add( stLenPercent1Hint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText75 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText75->Wrap( -1 );
	bSizer122->Add( m_staticText75, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer122, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen1 = new wxStaticText( this, wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen1->Wrap( -1 );
	gbSizer1->Add( m_stMaxLen1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcTdMaxLen1, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits1 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits1->Wrap( -1 );
	gbSizer1->Add( m_stMaxLenUnits1, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidthLabel1 = new wxStaticText( this, wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthLabel1->Wrap( -1 );
	gbSizer1->Add( m_stWidthLabel1, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spWidthPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spWidthPercent1->SetDigits( 0 );
	gbSizer1->Add( m_spWidthPercent1, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_stWidthPercentUnits1 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentUnits1->Wrap( -1 );
	bSizer12->Add( m_stWidthPercentUnits1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stSizePercent1Hint;
	stSizePercent1Hint = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stSizePercent1Hint->Wrap( -1 );
	stSizePercent1Hint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer12->Add( stSizePercent1Hint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText74 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText74->Wrap( -1 );
	bSizer12->Add( m_staticText74, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer12, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthLabel1 = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthLabel1->Wrap( -1 );
	gbSizer1->Add( m_stMaxWidthLabel1, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcMaxWidth1, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidthUnits1 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthUnits1->Wrap( -1 );
	gbSizer1->Add( m_stMaxWidthUnits1, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel1 = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel1->Wrap( -1 );
	gbSizer1->Add( m_curvePointsLabel1, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl1 = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer1->Add( m_curvePointsCtrl1, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection1 = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbPreferZoneConnection1, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_stHDRatio1 = new wxStaticText( this, wxID_ANY, _("Maximum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio1->Wrap( -1 );
	m_stHDRatio1->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer1->Add( m_stHDRatio1, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent1->SetDigits( 0 );
	m_spTeardropHDPercent1->SetToolTip( _("Tracks which are similar in size to the pad do not need teardrops.") );

	gbSizer1->Add( m_spTeardropHDPercent1, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1221;
	bSizer1221 = new wxBoxSizer( wxHORIZONTAL );

	m_minTrackWidthUnits1 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits1->Wrap( -1 );
	bSizer1221->Add( m_minTrackWidthUnits1, 0, 0, 5 );

	wxStaticText* stMinTrackWidthHint1;
	stMinTrackWidthHint1 = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthHint1->Wrap( -1 );
	stMinTrackWidthHint1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1221->Add( stMinTrackWidthHint1, 0, wxALIGN_BOTTOM, 1 );

	m_staticText73 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText73->Wrap( -1 );
	bSizer1221->Add( m_staticText73, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->Add( bSizer1221, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack1 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack1->SetValue(true);
	gbSizer1->Add( m_cbTeardropsUseNextTrack1, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns1->Add( gbSizer1, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns1, 1, wxEXPAND, 5 );


	m_gridSizer->Add( 0, 30, 0, wxEXPAND, 5 );

	m_tracksLabel = new wxStaticText( this, wxID_ANY, _("Properties for track-to-track teardrops:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tracksLabel->Wrap( -1 );
	m_gridSizer->Add( m_tracksLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_gridSizer->Add( m_staticline3, 0, wxEXPAND|wxBOTTOM, 10 );

	wxBoxSizer* bSizerShapeColumns2;
	bSizerShapeColumns2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol2;
	bSizerLeftCol2 = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop2 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol2->Add( m_bitmapTeardrop2, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer412;
	bSizer412 = new wxBoxSizer( wxHORIZONTAL );

	m_edgesLabel2 = new wxStaticText( this, wxID_ANY, _("Edges:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_edgesLabel2->Wrap( -1 );
	bSizer412->Add( m_edgesLabel2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rbStraightLines2 = new wxRadioButton( this, wxID_ANY, _("Straight lines"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer412->Add( m_rbStraightLines2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rbCurved2 = new wxRadioButton( this, wxID_ANY, _("Curved"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer412->Add( m_rbCurved2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerLeftCol2->Add( bSizer412, 0, wxEXPAND|wxBOTTOM, 4 );


	bSizerShapeColumns2->Add( bSizerLeftCol2, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns2->Add( 10, 0, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 2, 3 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( 10,7 ) );

	m_stLenPercentLabel2 = new wxStaticText( this, wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentLabel2->Wrap( -1 );
	gbSizer2->Add( m_stLenPercentLabel2, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spLenPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spLenPercent2->SetDigits( 0 );
	gbSizer2->Add( m_spLenPercent2, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer1211;
	bSizer1211 = new wxBoxSizer( wxHORIZONTAL );

	m_stLenPercentUnits2 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentUnits2->Wrap( -1 );
	bSizer1211->Add( m_stLenPercentUnits2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stLenPercentHint2;
	stLenPercentHint2 = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stLenPercentHint2->Wrap( -1 );
	stLenPercentHint2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1211->Add( stLenPercentHint2, 0, wxALIGN_BOTTOM, 1 );

	m_staticText70 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText70->Wrap( -1 );
	bSizer1211->Add( m_staticText70, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->Add( bSizer1211, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen2 = new wxStaticText( this, wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen2->Wrap( -1 );
	gbSizer2->Add( m_stMaxLen2, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcTdMaxLen2, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits2 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits2->Wrap( -1 );
	gbSizer2->Add( m_stMaxLenUnits2, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidthLabel2 = new wxStaticText( this, wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthLabel2->Wrap( -1 );
	gbSizer2->Add( m_stWidthLabel2, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spWidthPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spWidthPercent2->SetDigits( 0 );
	gbSizer2->Add( m_spWidthPercent2, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxHORIZONTAL );

	m_stWidthPercentUnits2 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentUnits2->Wrap( -1 );
	bSizer121->Add( m_stWidthPercentUnits2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stWidthPercentHint2;
	stWidthPercentHint2 = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stWidthPercentHint2->Wrap( -1 );
	stWidthPercentHint2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer121->Add( stWidthPercentHint2, 0, wxALIGN_BOTTOM, 1 );

	m_staticText72 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText72->Wrap( -1 );
	bSizer121->Add( m_staticText72, 0, wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->Add( bSizer121, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthLabel2 = new wxStaticText( this, wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthLabel2->Wrap( -1 );
	gbSizer2->Add( m_stMaxWidthLabel2, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxWidth2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcMaxWidth2, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxWidthUnits2 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthUnits2->Wrap( -1 );
	gbSizer2->Add( m_stMaxWidthUnits2, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel2 = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel2->Wrap( -1 );
	gbSizer2->Add( m_curvePointsLabel2, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl2 = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer2->Add( m_curvePointsCtrl2, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stHDRatio2 = new wxStaticText( this, wxID_ANY, _("Maximum track width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio2->Wrap( -1 );
	m_stHDRatio2->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer2->Add( m_stHDRatio2, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 40 );

	m_spTeardropHDPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent2->SetDigits( 0 );
	m_spTeardropHDPercent2->SetToolTip( _("Tracks which are similar in size do not need teardrops.") );

	gbSizer2->Add( m_spTeardropHDPercent2, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer12211;
	bSizer12211 = new wxBoxSizer( wxHORIZONTAL );

	m_minTrackWidthUnits2 = new wxStaticText( this, wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits2->Wrap( -1 );
	bSizer12211->Add( m_minTrackWidthUnits2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* stMinTrackWidthHint2;
	stMinTrackWidthHint2 = new wxStaticText( this, wxID_ANY, _("w"), wxDefaultPosition, wxDefaultSize, 0 );
	stMinTrackWidthHint2->Wrap( -1 );
	stMinTrackWidthHint2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer12211->Add( stMinTrackWidthHint2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText71 = new wxStaticText( this, wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText71->Wrap( -1 );
	bSizer12211->Add( m_staticText71, 0, wxALIGN_BOTTOM|wxBOTTOM, 1 );


	gbSizer2->Add( bSizer12211, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_cbTeardropsUseNextTrack2 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack2->SetValue(true);
	gbSizer2->Add( m_cbTeardropsUseNextTrack2, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns2->Add( gbSizer2, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns2, 1, wxEXPAND, 5 );


	m_gridSizer->Add( 0, 5, 0, wxEXPAND, 5 );


	mainSizer->Add( m_gridSizer, 0, wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_TEARDROPS_BASE::~PANEL_SETUP_TEARDROPS_BASE()
{
}
