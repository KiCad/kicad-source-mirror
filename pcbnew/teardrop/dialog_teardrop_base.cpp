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
	bSizeScopeSize = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerBitmaps;
	bSizerBitmaps = new wxBoxSizer( wxVERTICAL );

	m_bitmapTdCircularInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmaps->Add( m_bitmapTdCircularInfo, 0, wxALL, 5 );

	m_bitmapTdRectangularInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmaps->Add( m_bitmapTdRectangularInfo, 0, wxALL, 5 );

	m_bitmapTdTrackInfo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmaps->Add( m_bitmapTdTrackInfo, 0, wxALL, 5 );


	bSizeScopeSize->Add( bSizerBitmaps, 0, wxEXPAND, 5 );


	bSizeScopeSize->Add( 0, 0, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerSizes;
	fgSizerSizes = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerSizes->SetFlexibleDirection( wxBOTH );
	fgSizerSizes->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stMaxLen = new wxStaticText( this, wxID_ANY, wxT("Max lenght"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	fgSizerSizes->Add( m_stMaxLen, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSizes->Add( m_tcTdMaxLen, 0, wxALL, 5 );

	m_stLenUnit = new wxStaticText( this, wxID_ANY, wxT("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenUnit->Wrap( -1 );
	fgSizerSizes->Add( m_stLenUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_stHsetting = new wxStaticText( this, wxID_ANY, wxT("Best lenght"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsetting->Wrap( -1 );
	fgSizerSizes->Add( m_stHsetting, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropLenPercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 100, 50.000000, 10 );
	m_spTeardropLenPercent->SetDigits( 0 );
	fgSizerSizes->Add( m_spTeardropLenPercent, 0, wxALL|wxEXPAND, 5 );

	m_stLenPercent = new wxStaticText( this, wxID_ANY, wxT("% of d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercent->Wrap( -1 );
	fgSizerSizes->Add( m_stLenPercent, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerSizes->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );

	m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerSizes->Add( m_staticline6, 0, wxEXPAND | wxALL, 5 );

	m_staticline7 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerSizes->Add( m_staticline7, 0, wxEXPAND | wxALL, 5 );

	m_stTdMaxSize = new wxStaticText( this, wxID_ANY, wxT("Max height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSize->Wrap( -1 );
	fgSizerSizes->Add( m_stTdMaxSize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcMaxSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSizes->Add( m_tcMaxSize, 0, wxALL, 5 );

	m_stSizeUnit = new wxStaticText( this, wxID_ANY, wxT("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSizeUnit->Wrap( -1 );
	fgSizerSizes->Add( m_stSizeUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_stVsetting = new wxStaticText( this, wxID_ANY, wxT("Best height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsetting->Wrap( -1 );
	fgSizerSizes->Add( m_stVsetting, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropSizePercent = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercent->SetDigits( 0 );
	fgSizerSizes->Add( m_spTeardropSizePercent, 0, wxALL|wxEXPAND, 5 );

	m_stTdSizePercent = new wxStaticText( this, wxID_ANY, wxT("% of d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdSizePercent->Wrap( -1 );
	fgSizerSizes->Add( m_stTdSizePercent, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizeScopeSize->Add( fgSizerSizes, 0, wxEXPAND|wxALL, 5 );


	bSizerTop->Add( bSizeScopeSize, 0, wxEXPAND, 5 );


	bSizerUpper->Add( bSizerTop, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerShape;
	bSizerShape = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerScope;
	sbSizerScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Scope") ), wxVERTICAL );

	m_cbPadVia = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, wxT("Vias and PTH pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPadVia->SetValue(true);
	sbSizerScope->Add( m_cbPadVia, 0, wxALL, 5 );

	m_cbRoundShapesOnly = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, wxT("Round pads only"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerScope->Add( m_cbRoundShapesOnly, 0, wxALL, 5 );

	m_cbSmdSimilarPads = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, wxT("Not Drilled Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSmdSimilarPads->SetValue(true);
	sbSizerScope->Add( m_cbSmdSimilarPads, 0, wxALL, 5 );

	m_cbTrack2Track = new wxCheckBox( sbSizerScope->GetStaticBox(), wxID_ANY, wxT("Track to track"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerScope->Add( m_cbTrack2Track, 0, wxALL, 5 );


	bSizerShape->Add( sbSizerScope, 0, 0, 5 );


	bSizerShape->Add( 30, 10, 1, wxEXPAND, 5 );

	wxString m_rbShapeChoices[] = { wxT("Straight lines"), wxT("Curved for round pads"), wxT("Curved for all pad shapes") };
	int m_rbShapeNChoices = sizeof( m_rbShapeChoices ) / sizeof( wxString );
	m_rbShape = new wxRadioBox( this, wxID_ANY, wxT("Teardrop style"), wxDefaultPosition, wxDefaultSize, m_rbShapeNChoices, m_rbShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_rbShape->SetSelection( 0 );
	bSizerShape->Add( m_rbShape, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerShape->Add( 30, 0, 0, 0, 5 );

	m_stPoinCount = new wxStaticText( this, wxID_ANY, wxT("Curve points"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPoinCount->Wrap( -1 );
	bSizerShape->Add( m_stPoinCount, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_spPointCount = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 10, 5 );
	bSizerShape->Add( m_spPointCount, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerUpper->Add( bSizerShape, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerOptions;
	sbSizerOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Options") ), wxVERTICAL );

	m_cbOptUseNextTrack = new wxCheckBox( sbSizerOptions->GetStaticBox(), wxID_ANY, wxT("Allows use two tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOptUseNextTrack->SetValue(true);
	m_cbOptUseNextTrack->SetToolTip( wxT("Allows a teardrop to spread over 2 tracks if the first track segment is too short") );

	sbSizerOptions->Add( m_cbOptUseNextTrack, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerUpper->Add( sbSizerOptions, 1, wxEXPAND, 5 );

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
