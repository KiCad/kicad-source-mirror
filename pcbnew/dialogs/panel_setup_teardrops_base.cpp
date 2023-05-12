///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
	gbSizer = new wxGridBagSizer( 2, 5 );
	gbSizer->SetFlexibleDirection( wxBOTH );
	gbSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer->SetEmptyCellSize( wxSize( 10,9 ) );

	m_stHsetting = new wxStaticText( this, wxID_ANY, _("Best length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsetting->Wrap( -1 );
	gbSizer->Add( m_stHsetting, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropLenPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spTeardropLenPercent->SetDigits( 0 );
	gbSizer->Add( m_spTeardropLenPercent, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stTeardropLenUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTeardropLenUnits->Wrap( -1 );
	gbSizer->Add( m_stTeardropLenUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen = new wxStaticText( this, wxID_ANY, _("Max length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	gbSizer->Add( m_stMaxLen, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_tcTdMaxLen, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits->Wrap( -1 );
	gbSizer->Add( m_stMaxLenUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stVsetting = new wxStaticText( this, wxID_ANY, _("Best height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsetting->Wrap( -1 );
	gbSizer->Add( m_stVsetting, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropSizePercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercent->SetDigits( 0 );
	gbSizer->Add( m_spTeardropSizePercent, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stLenPercent = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent->Wrap( -1 );
	gbSizer->Add( m_stLenPercent, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stTdMaxSize = new wxStaticText( this, wxID_ANY, _("Max height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSize->Wrap( -1 );
	gbSizer->Add( m_stTdMaxSize, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxHeight = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_tcMaxHeight, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxHeightUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeightUnits->Wrap( -1 );
	gbSizer->Add( m_stMaxHeightUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel->Wrap( -1 );
	gbSizer->Add( m_curvePointsLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer->Add( m_curvePointsCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer->Add( m_cbPreferZoneConnection, wxGBPosition( 3, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 60 );

	m_stHDRatio = new wxStaticText( this, wxID_ANY, _("Maximum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer->Add( m_stHDRatio, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 60 );

	m_spTeardropHDPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent->SetDigits( 0 );
	m_spTeardropHDPercent->SetToolTip( _("Tracks which are similar in size to the pad or via do not need teardrops.") );

	gbSizer->Add( m_spTeardropHDPercent, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthUnits = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits->Wrap( -1 );
	gbSizer->Add( m_minTrackWidthUnits, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthHint = new wxStaticText( this, wxID_ANY, _("(as a percentage of pad/via size)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint->Wrap( -1 );
	gbSizer->Add( m_minTrackWidthHint, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxLEFT, 65 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack->SetValue(true);
	gbSizer->Add( m_cbTeardropsUseNextTrack, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizerShapeColumns->Add( gbSizer, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns, 1, wxEXPAND, 5 );


	m_gridSizer->Add( 0, 25, 0, wxEXPAND, 5 );

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
	gbSizer1 = new wxGridBagSizer( 2, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 10,9 ) );

	m_stHsetting1 = new wxStaticText( this, wxID_ANY, _("Best length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsetting1->Wrap( -1 );
	gbSizer1->Add( m_stHsetting1, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropLenPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spTeardropLenPercent1->SetDigits( 0 );
	gbSizer1->Add( m_spTeardropLenPercent1, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stTeardropLenUnits1 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTeardropLenUnits1->Wrap( -1 );
	gbSizer1->Add( m_stTeardropLenUnits1, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen1 = new wxStaticText( this, wxID_ANY, _("Max length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen1->Wrap( -1 );
	gbSizer1->Add( m_stMaxLen1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcTdMaxLen1, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits1 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits1->Wrap( -1 );
	gbSizer1->Add( m_stMaxLenUnits1, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stVsetting1 = new wxStaticText( this, wxID_ANY, _("Best height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsetting1->Wrap( -1 );
	gbSizer1->Add( m_stVsetting1, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropSizePercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercent1->SetDigits( 0 );
	gbSizer1->Add( m_spTeardropSizePercent1, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stLenPercent1 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent1->Wrap( -1 );
	gbSizer1->Add( m_stLenPercent1, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stTdMaxSize1 = new wxStaticText( this, wxID_ANY, _("Max height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSize1->Wrap( -1 );
	gbSizer1->Add( m_stTdMaxSize1, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxHeight1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_tcMaxHeight1, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxHeightUnits1 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeightUnits1->Wrap( -1 );
	gbSizer1->Add( m_stMaxHeightUnits1, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel1 = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel1->Wrap( -1 );
	gbSizer1->Add( m_curvePointsLabel1, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl1 = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer1->Add( m_curvePointsCtrl1, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_cbPreferZoneConnection1 = new wxCheckBox( this, wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbPreferZoneConnection1, wxGBPosition( 3, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 60 );

	m_stHDRatio1 = new wxStaticText( this, wxID_ANY, _("Maximum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio1->Wrap( -1 );
	m_stHDRatio1->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer1->Add( m_stHDRatio1, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 60 );

	m_spTeardropHDPercent1 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent1->SetDigits( 0 );
	m_spTeardropHDPercent1->SetToolTip( _("Tracks which are similar in size to the pad do not need teardrops.") );

	gbSizer1->Add( m_spTeardropHDPercent1, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthUnits1 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits1->Wrap( -1 );
	gbSizer1->Add( m_minTrackWidthUnits1, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthHint1 = new wxStaticText( this, wxID_ANY, _("(as a percentage of pad size)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint1->Wrap( -1 );
	gbSizer1->Add( m_minTrackWidthHint1, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxLEFT, 65 );

	m_cbTeardropsUseNextTrack1 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack1->SetValue(true);
	gbSizer1->Add( m_cbTeardropsUseNextTrack1, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizerShapeColumns1->Add( gbSizer1, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns1, 1, wxEXPAND, 5 );


	m_gridSizer->Add( 0, 25, 0, wxEXPAND, 5 );

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
	gbSizer2 = new wxGridBagSizer( 2, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( 10,9 ) );

	m_stHsetting2 = new wxStaticText( this, wxID_ANY, _("Best length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsetting2->Wrap( -1 );
	gbSizer2->Add( m_stHsetting2, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropLenPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 40.000000, 10 );
	m_spTeardropLenPercent2->SetDigits( 0 );
	gbSizer2->Add( m_spTeardropLenPercent2, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stTeardropLenUnits2 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTeardropLenUnits2->Wrap( -1 );
	gbSizer2->Add( m_stTeardropLenUnits2, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen2 = new wxStaticText( this, wxID_ANY, _("Max length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen2->Wrap( -1 );
	gbSizer2->Add( m_stMaxLen2, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcTdMaxLen2, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxLenUnits2 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits2->Wrap( -1 );
	gbSizer2->Add( m_stMaxLenUnits2, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stVsetting2 = new wxStaticText( this, wxID_ANY, _("Best height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsetting2->Wrap( -1 );
	gbSizer2->Add( m_stVsetting2, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropSizePercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercent2->SetDigits( 0 );
	gbSizer2->Add( m_spTeardropSizePercent2, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stLenPercent2 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent2->Wrap( -1 );
	gbSizer2->Add( m_stLenPercent2, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stTdMaxSize2 = new wxStaticText( this, wxID_ANY, _("Max height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSize2->Wrap( -1 );
	gbSizer2->Add( m_stTdMaxSize2, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxHeight2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_tcMaxHeight2, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_stMaxHeightUnits2 = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeightUnits2->Wrap( -1 );
	gbSizer2->Add( m_stMaxHeightUnits2, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsLabel2 = new wxStaticText( this, wxID_ANY, _("Curve points:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_curvePointsLabel2->Wrap( -1 );
	gbSizer2->Add( m_curvePointsLabel2, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_curvePointsCtrl2 = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	gbSizer2->Add( m_curvePointsCtrl2, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_stHDRatio2 = new wxStaticText( this, wxID_ANY, _("Maximum track width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio2->Wrap( -1 );
	m_stHDRatio2->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	gbSizer2->Add( m_stHDRatio2, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 60 );

	m_spTeardropHDPercent2 = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent2->SetDigits( 0 );
	m_spTeardropHDPercent2->SetToolTip( _("Tracks which are similar in size do not need teardrops.") );

	gbSizer2->Add( m_spTeardropHDPercent2, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthUnits2 = new wxStaticText( this, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits2->Wrap( -1 );
	gbSizer2->Add( m_minTrackWidthUnits2, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthHint2 = new wxStaticText( this, wxID_ANY, _("(as a percentage of larger track width)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint2->Wrap( -1 );
	gbSizer2->Add( m_minTrackWidthHint2, wxGBPosition( 1, 3 ), wxGBSpan( 1, 3 ), wxLEFT, 65 );

	m_cbTeardropsUseNextTrack2 = new wxCheckBox( this, wxID_ANY, _("Allow teardrop to span two track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack2->SetValue(true);
	gbSizer2->Add( m_cbTeardropsUseNextTrack2, wxGBPosition( 6, 2 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizerShapeColumns2->Add( gbSizer2, 0, wxEXPAND|wxLEFT, 20 );


	m_gridSizer->Add( bSizerShapeColumns2, 1, wxEXPAND, 5 );


	mainSizer->Add( m_gridSizer, 0, wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
}

PANEL_SETUP_TEARDROPS_BASE::~PANEL_SETUP_TEARDROPS_BASE()
{
}
