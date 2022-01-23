///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_teardrop_base.h"

///////////////////////////////////////////////////////////////////////////

TEARDROP_DIALOG_BASE::TEARDROP_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizeScopeSize;
	bSizeScopeSize = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerBitmaps;
	fgSizerBitmaps = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerBitmaps->AddGrowableCol( 2 );
	fgSizerBitmaps->SetFlexibleDirection( wxBOTH );
	fgSizerBitmaps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextRndS = new wxStaticText( this, wxID_ANY, _("Round shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRndS->Wrap( -1 );
	fgSizerBitmaps->Add( m_staticTextRndS, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapTdCircularInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBitmaps->Add( m_bitmapTdCircularInfo, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxFlexGridSizer* fgSizerParmRound;
	fgSizerParmRound = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerParmRound->AddGrowableCol( 1 );
	fgSizerParmRound->AddGrowableCol( 3 );
	fgSizerParmRound->SetFlexibleDirection( wxBOTH );
	fgSizerParmRound->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stMaxLenRound = new wxStaticText( this, wxID_ANY, _("Max length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stMaxLenRound, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcTdMaxLenRound = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmRound->Add( m_tcTdMaxLenRound, 0, wxALL|wxEXPAND, 5 );

	m_stTdMaxSizeRound = new wxStaticText( this, wxID_ANY, _("Max height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSizeRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stTdMaxSizeRound, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcMaxHeightRound = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmRound->Add( m_tcMaxHeightRound, 0, wxALL|wxEXPAND, 5 );

	m_stLenUnitRound = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenUnitRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stLenUnitRound, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_stHsettingRound = new wxStaticText( this, wxID_ANY, _("Best length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsettingRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stHsettingRound, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropLenPercentRound = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 50.000000, 10 );
	m_spTeardropLenPercentRound->SetDigits( 0 );
	fgSizerParmRound->Add( m_spTeardropLenPercentRound, 0, wxALL|wxEXPAND, 5 );

	m_stVsettingRound = new wxStaticText( this, wxID_ANY, _("Best height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsettingRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stVsettingRound, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropSizePercentRound = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercentRound->SetDigits( 0 );
	fgSizerParmRound->Add( m_spTeardropSizePercentRound, 0, wxALL|wxEXPAND, 5 );

	m_stLenPercentRound = new wxStaticText( this, wxID_ANY, _("percent of d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentRound->Wrap( -1 );
	fgSizerParmRound->Add( m_stLenPercentRound, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgSizerBitmaps->Add( fgSizerParmRound, 1, wxEXPAND, 5 );

	m_staticTextRectS = new wxStaticText( this, wxID_ANY, _("Rect shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRectS->Wrap( -1 );
	fgSizerBitmaps->Add( m_staticTextRectS, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapTdRectangularInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBitmaps->Add( m_bitmapTdRectangularInfo, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxFlexGridSizer* fgSizerParmRect;
	fgSizerParmRect = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerParmRect->AddGrowableCol( 1 );
	fgSizerParmRect->AddGrowableCol( 3 );
	fgSizerParmRect->SetFlexibleDirection( wxBOTH );
	fgSizerParmRect->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stMaxLenRect = new wxStaticText( this, wxID_ANY, _("Max length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stMaxLenRect, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcTdMaxLenRect = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmRect->Add( m_tcTdMaxLenRect, 0, wxALL|wxEXPAND, 5 );

	m_stTdMaxSizeRect = new wxStaticText( this, wxID_ANY, _("Max height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSizeRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stTdMaxSizeRect, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcMaxHeightRect = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmRect->Add( m_tcMaxHeightRect, 0, wxALL|wxEXPAND, 5 );

	m_stLenUnitRect = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenUnitRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stLenUnitRect, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_stHsettingRect = new wxStaticText( this, wxID_ANY, _("Best length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsettingRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stHsettingRect, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropLenPercentRect = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 50.000000, 10 );
	m_spTeardropLenPercentRect->SetDigits( 0 );
	fgSizerParmRect->Add( m_spTeardropLenPercentRect, 0, wxALL|wxEXPAND, 5 );

	m_stVsettingRect = new wxStaticText( this, wxID_ANY, _("Best height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsettingRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stVsettingRect, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropSizePercentRect = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercentRect->SetDigits( 0 );
	fgSizerParmRect->Add( m_spTeardropSizePercentRect, 0, wxALL|wxEXPAND, 5 );

	m_stLenPercentRect = new wxStaticText( this, wxID_ANY, _("percent of d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentRect->Wrap( -1 );
	fgSizerParmRect->Add( m_stLenPercentRect, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgSizerBitmaps->Add( fgSizerParmRect, 1, wxEXPAND, 5 );

	m_staticTextTrck = new wxStaticText( this, wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTrck->Wrap( -1 );
	fgSizerBitmaps->Add( m_staticTextTrck, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapTdTrackInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBitmaps->Add( m_bitmapTdTrackInfo, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxFlexGridSizer* fgSizerParmTrack;
	fgSizerParmTrack = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerParmTrack->AddGrowableCol( 1 );
	fgSizerParmTrack->AddGrowableCol( 3 );
	fgSizerParmTrack->SetFlexibleDirection( wxBOTH );
	fgSizerParmTrack->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stMaxLenTrack = new wxStaticText( this, wxID_ANY, _("Max length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenTrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stMaxLenTrack, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcTdMaxLenTrack = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmTrack->Add( m_tcTdMaxLenTrack, 0, wxALL|wxEXPAND, 5 );

	m_stTdMaxSizeTrack = new wxStaticText( this, wxID_ANY, _("Max height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSizeTrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stTdMaxSizeTrack, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_tcMaxHeightTrack = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerParmTrack->Add( m_tcMaxHeightTrack, 0, wxALL|wxEXPAND, 5 );

	m_stLenUnitTrack = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenUnitTrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stLenUnitTrack, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_stHsettingtrack = new wxStaticText( this, wxID_ANY, _("Best length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsettingtrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stHsettingtrack, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropLenPercentTrack = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 100.000000, 10 );
	m_spTeardropLenPercentTrack->SetDigits( 0 );
	fgSizerParmTrack->Add( m_spTeardropLenPercentTrack, 0, wxALL|wxEXPAND, 5 );

	m_stVsettingtrack = new wxStaticText( this, wxID_ANY, _("Best height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsettingtrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stVsettingtrack, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spTeardropSizePercentTrack = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercentTrack->SetDigits( 0 );
	fgSizerParmTrack->Add( m_spTeardropSizePercentTrack, 0, wxALL|wxEXPAND, 5 );

	m_stLenPercentTrack = new wxStaticText( this, wxID_ANY, _("percent of d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentTrack->Wrap( -1 );
	fgSizerParmTrack->Add( m_stLenPercentTrack, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	fgSizerBitmaps->Add( fgSizerParmTrack, 1, wxEXPAND, 5 );


	bSizeScopeSize->Add( fgSizerBitmaps, 1, wxLEFT|wxEXPAND, 5 );


	bSizerTop->Add( bSizeScopeSize, 0, wxEXPAND, 5 );


	bSizerUpper->Add( bSizerTop, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerShape;
	bSizerShape = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerScope;
	sbSizerScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );

	m_cbPadVia = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Vias and PTH pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadVia->SetValue(true);
	m_cbPadVia->SetToolTip( _("Add teardrops to vias and pads with holes") );

	sbSizerScope->Add( m_cbPadVia, 0, wxALL, 5 );

	m_cbRoundShapesOnly = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Round pads only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRoundShapesOnly->SetToolTip( _("Add teardrops to round shapes only ") );

	sbSizerScope->Add( m_cbRoundShapesOnly, 0, wxALL, 5 );

	m_cbSmdSimilarPads = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Not drilled pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSmdSimilarPads->SetValue(true);
	m_cbSmdSimilarPads->SetToolTip( _("Add teardrops to not driiled pads, like SMD") );

	sbSizerScope->Add( m_cbSmdSimilarPads, 0, wxALL, 5 );

	m_cbTrack2Track = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, _("Track to track"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTrack2Track->SetValue(true);
	m_cbTrack2Track->SetToolTip( _("Add teardrops to two connected tracks of different widths") );

	sbSizerScope->Add( m_cbTrack2Track, 0, wxALL, 5 );


	bSizerShape->Add( sbSizerScope, 0, wxTOP|wxBOTTOM, 5 );


	bSizerShape->Add( 30, 10, 0, wxEXPAND, 5 );

	wxString m_rbShapeRoundChoices[] = { _("Straight lines"), _("Curved") };
	int m_rbShapeRoundNChoices = sizeof( m_rbShapeRoundChoices ) / sizeof( wxString );
	m_rbShapeRound = new wxRadioBox( this, wxID_ANY, _("Style for round shapes"), wxDefaultPosition, wxDefaultSize, m_rbShapeRoundNChoices, m_rbShapeRoundChoices, 1, wxRA_SPECIFY_COLS );
	m_rbShapeRound->SetSelection( 0 );
	bSizerShape->Add( m_rbShapeRound, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_rbShapeRectChoices[] = { _("Straight lines"), _("Curved") };
	int m_rbShapeRectNChoices = sizeof( m_rbShapeRectChoices ) / sizeof( wxString );
	m_rbShapeRect = new wxRadioBox( this, wxID_ANY, _("Style for rect shapes"), wxDefaultPosition, wxDefaultSize, m_rbShapeRectNChoices, m_rbShapeRectChoices, 1, wxRA_SPECIFY_COLS );
	m_rbShapeRect->SetSelection( 0 );
	bSizerShape->Add( m_rbShapeRect, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_rbShapeTrackChoices[] = { _("Straight lines"), _("Curved") };
	int m_rbShapeTrackNChoices = sizeof( m_rbShapeTrackChoices ) / sizeof( wxString );
	m_rbShapeTrack = new wxRadioBox( this, wxID_ANY, _("Style for tracks"), wxDefaultPosition, wxDefaultSize, m_rbShapeTrackNChoices, m_rbShapeTrackChoices, 1, wxRA_SPECIFY_COLS );
	m_rbShapeTrack->SetSelection( 0 );
	bSizerShape->Add( m_rbShapeTrack, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerShape->Add( 30, 0, 0, 0, 5 );


	bSizerUpper->Add( bSizerShape, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerOptions;
	sbSizerOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxHORIZONTAL );

	m_cbOptUseNextTrack = new wxCheckBox( sbSizerOptions->GetStaticBox(), wxID_ANY, _("Allows use two tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOptUseNextTrack->SetValue(true);
	m_cbOptUseNextTrack->SetToolTip( _("Allows a teardrop to spread over 2 tracks if the first track segment is too short") );

	sbSizerOptions->Add( m_cbOptUseNextTrack, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizerOptions->Add( 30, 0, 0, 0, 5 );

	m_stPointCount = new wxStaticText( sbSizerOptions->GetStaticBox(), wxID_ANY, _("Curve points"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPointCount->Wrap( -1 );
	m_stPointCount->SetToolTip( _("Number of segments to build a teardrop with curved shape") );

	sbSizerOptions->Add( m_stPointCount, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_spPointCount = new wxSpinCtrl( sbSizerOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	sbSizerOptions->Add( m_spPointCount, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerUpper->Add( sbSizerOptions, 0, wxEXPAND|wxTOP, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerUpper->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );
}

TEARDROP_DIALOG_BASE::~TEARDROP_DIALOG_BASE()
{
}
