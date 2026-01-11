///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_pad_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAD_PROPERTIES_BASE::DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelGeneral = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGeneralSizer;
	bGeneralSizer = new wxBoxSizer( wxHORIZONTAL );

	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizerCommon;
	gbSizerCommon = new wxGridBagSizer( 4, 0 );
	gbSizerCommon->SetFlexibleDirection( wxBOTH );
	gbSizerCommon->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_padTypeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padTypeLabel->Wrap( -1 );
	gbSizerCommon->Add( m_padTypeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_padTypeChoices[] = { _("Through-hole"), _("SMD"), _("Edge Connector"), _("NPTH, Mechanical"), _("SMD Aperture") };
	int m_padTypeNChoices = sizeof( m_padTypeChoices ) / sizeof( wxString );
	m_padType = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_padTypeNChoices, m_padTypeChoices, 0 );
	m_padType->SetSelection( 0 );
	gbSizerCommon->Add( m_padType, wxGBPosition( 0, 1 ), wxGBSpan( 1, 5 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );

	m_padNumLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padNumLabel->Wrap( -1 );
	gbSizerCommon->Add( m_padNumLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_padNumCtrl = new wxTextCtrl( m_panelGeneral, wxID_PADNUMCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerCommon->Add( m_padNumCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 5 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_padNetLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Net name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padNetLabel->Wrap( -1 );
	gbSizerCommon->Add( m_padNetLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_padNetSelector = new NET_SELECTOR( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerCommon->Add( m_padNetSelector, wxGBPosition( 2, 1 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_posXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	gbSizerCommon->Add( m_posXLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_posXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerCommon->Add( m_posXCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	gbSizerCommon->Add( m_posXUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_posYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	gbSizerCommon->Add( m_posYLabel, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_posYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerCommon->Add( m_posYCtrl, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	gbSizerCommon->Add( m_posYUnits, wxGBPosition( 3, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	gbSizerCommon->AddGrowableCol( 1 );
	gbSizerCommon->AddGrowableCol( 4 );

	m_LeftBoxSizer->Add( gbSizerCommon, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline5 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM, 7 );

	m_padstackControls = new wxBoxSizer( wxHORIZONTAL );

	m_staticText891 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Padstack mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText891->Wrap( -1 );
	m_padstackControls->Add( m_staticText891, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_cbPadstackModeChoices[] = { _("Normal"), _("Front/Inner/Bottom"), _("Custom") };
	int m_cbPadstackModeNChoices = sizeof( m_cbPadstackModeChoices ) / sizeof( wxString );
	m_cbPadstackMode = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbPadstackModeNChoices, m_cbPadstackModeChoices, 0 );
	m_cbPadstackMode->SetSelection( 2 );
	m_padstackControls->Add( m_cbPadstackMode, 0, wxALL, 5 );


	m_padstackControls->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText90 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Edit layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText90->Wrap( -1 );
	m_padstackControls->Add( m_staticText90, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	wxString m_cbEditLayerChoices[] = { _("Inner Layers") };
	int m_cbEditLayerNChoices = sizeof( m_cbEditLayerChoices ) / sizeof( wxString );
	m_cbEditLayer = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbEditLayerNChoices, m_cbEditLayerChoices, 0 );
	m_cbEditLayer->SetSelection( 0 );
	m_padstackControls->Add( m_cbEditLayer, 0, wxALL, 5 );


	m_LeftBoxSizer->Add( m_padstackControls, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizerPadOrientation;
	gbSizerPadOrientation = new wxGridBagSizer( 4, 5 );
	gbSizerPadOrientation->SetFlexibleDirection( wxBOTH );
	gbSizerPadOrientation->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_shapeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeLabel->Wrap( -1 );
	gbSizerPadOrientation->Add( m_shapeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_PadShapeSelectorChoices[] = { _("Circular"), _("Oval"), _("Rectangular"), _("Trapezoidal"), _("Rounded rectangle"), _("Chamfered rectangle"), _("Chamfered with other corners rounded"), _("Custom (circular base)"), _("Custom (rectangular base)") };
	int m_PadShapeSelectorNChoices = sizeof( m_PadShapeSelectorChoices ) / sizeof( wxString );
	m_PadShapeSelector = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadShapeSelectorNChoices, m_PadShapeSelectorChoices, 0 );
	m_PadShapeSelector->SetSelection( 4 );
	gbSizerPadOrientation->Add( m_PadShapeSelector, wxGBPosition( 0, 1 ), wxGBSpan( 1, 5 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_shapePropsBook = new wxSimplebook( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_emptyProps = new wxPanel( m_shapePropsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_shapePropsBook->AddPage( m_emptyProps, _("a page"), false );
	m_trapProps = new wxPanel( m_shapePropsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	fgSizerTrapProps = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgSizerTrapProps->AddGrowableCol( 1 );
	fgSizerTrapProps->SetFlexibleDirection( wxBOTH );
	fgSizerTrapProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_trapDeltaLabel = new wxStaticText( m_trapProps, wxID_ANY, _("Trapezoid delta:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapDeltaLabel->Wrap( -1 );
	fgSizerTrapProps->Add( m_trapDeltaLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_trapDeltaCtrl = new wxTextCtrl( m_trapProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerTrapProps->Add( m_trapDeltaCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_trapDeltaUnits = new wxStaticText( m_trapProps, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapDeltaUnits->Wrap( -1 );
	fgSizerTrapProps->Add( m_trapDeltaUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_trapAxisLabel = new wxStaticText( m_trapProps, wxID_ANY, _("Trapezoid axis:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trapAxisLabel->Wrap( -1 );
	fgSizerTrapProps->Add( m_trapAxisLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_trapAxisCtrlChoices[] = { _("Horizontal"), _("Vertical") };
	int m_trapAxisCtrlNChoices = sizeof( m_trapAxisCtrlChoices ) / sizeof( wxString );
	m_trapAxisCtrl = new wxChoice( m_trapProps, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trapAxisCtrlNChoices, m_trapAxisCtrlChoices, 0 );
	m_trapAxisCtrl->SetSelection( 0 );
	fgSizerTrapProps->Add( m_trapAxisCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	m_trapProps->SetSizer( fgSizerTrapProps );
	m_trapProps->Layout();
	fgSizerTrapProps->Fit( m_trapProps );
	m_shapePropsBook->AddPage( m_trapProps, _("a page"), false );
	m_roudingProps = new wxPanel( m_shapePropsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	fgSizerRoundingProps = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgSizerRoundingProps->AddGrowableCol( 1 );
	fgSizerRoundingProps->SetFlexibleDirection( wxBOTH );
	fgSizerRoundingProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cornerRatioLabel = new wxStaticText( m_roudingProps, wxID_ANY, _("Corner size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRatioLabel->Wrap( -1 );
	m_cornerRatioLabel->SetToolTip( _("Corner radius in percent  of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgSizerRoundingProps->Add( m_cornerRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_cornerRatioCtrl = new TEXT_CTRL_EVAL( m_roudingProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRoundingProps->Add( m_cornerRatioCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_cornerRatioUnits = new wxStaticText( m_roudingProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRatioUnits->Wrap( -1 );
	fgSizerRoundingProps->Add( m_cornerRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_cornerRadiusLabel = new wxStaticText( m_roudingProps, wxID_ANY, _("Corner radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	m_cornerRadiusLabel->SetToolTip( _("Corner radius.\nCan be no more than half pad width.\nThe width is the smaller value between size X and size Y.\nNote: IPC norm gives a max value = 0.25mm.") );

	fgSizerRoundingProps->Add( m_cornerRadiusLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_cornerRadiusCtrl = new wxTextCtrl( m_roudingProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRoundingProps->Add( m_cornerRadiusCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_cornerRadiusUnits = new wxStaticText( m_roudingProps, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	fgSizerRoundingProps->Add( m_cornerRadiusUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	m_roudingProps->SetSizer( fgSizerRoundingProps );
	m_roudingProps->Layout();
	fgSizerRoundingProps->Fit( m_roudingProps );
	m_shapePropsBook->AddPage( m_roudingProps, _("a page"), false );
	m_chamferProps = new wxPanel( m_shapePropsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizerChamferProps;
	fgSizerChamferProps = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgSizerChamferProps->AddGrowableCol( 1 );
	fgSizerChamferProps->SetFlexibleDirection( wxBOTH );
	fgSizerChamferProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_chamferRatioLabel = new wxStaticText( m_chamferProps, wxID_ANY, _("Chamfer size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chamferRatioLabel->Wrap( -1 );
	m_chamferRatioLabel->SetToolTip( _("Chamfer size in percent of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgSizerChamferProps->Add( m_chamferRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxHORIZONTAL );

	m_chamferRatioCtrl = new TEXT_CTRL_EVAL( m_chamferProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer29->Add( m_chamferRatioCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_chamferRatioUnits = new wxStaticText( m_chamferProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chamferRatioUnits->Wrap( -1 );
	bSizer29->Add( m_chamferRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizerChamferProps->Add( bSizer29, 1, wxEXPAND, 5 );

	m_staticTextChamferCorner = new wxStaticText( m_chamferProps, wxID_ANY, _("Chamfer corners:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferCorner->Wrap( -1 );
	m_staticTextChamferCorner->SetToolTip( _("Chamfered corners. The position is relative to a pad orientation 0 degree.") );

	fgSizerChamferProps->Add( m_staticTextChamferCorner, 0, 0, 5 );

	wxGridSizer* gCornersSizer;
	gCornersSizer = new wxGridSizer( 0, 2, 3, 6 );

	m_cbTopLeft = new wxCheckBox( m_chamferProps, wxID_ANY, _("Top left"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTopLeft->SetValue(true);
	gCornersSizer->Add( m_cbTopLeft, 0, 0, 5 );

	m_cbTopRight = new wxCheckBox( m_chamferProps, wxID_ANY, _("Top right"), wxDefaultPosition, wxDefaultSize, 0 );
	gCornersSizer->Add( m_cbTopRight, 0, 0, 5 );

	m_cbBottomLeft = new wxCheckBox( m_chamferProps, wxID_ANY, _("Bottom left"), wxDefaultPosition, wxDefaultSize, 0 );
	gCornersSizer->Add( m_cbBottomLeft, 0, 0, 5 );

	m_cbBottomRight = new wxCheckBox( m_chamferProps, wxID_ANY, _("Bottom right"), wxDefaultPosition, wxDefaultSize, 0 );
	gCornersSizer->Add( m_cbBottomRight, 0, 0, 5 );


	fgSizerChamferProps->Add( gCornersSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_chamferProps->SetSizer( fgSizerChamferProps );
	m_chamferProps->Layout();
	fgSizerChamferProps->Fit( m_chamferProps );
	m_shapePropsBook->AddPage( m_chamferProps, _("a page"), false );
	m_mixedProps = new wxPanel( m_shapePropsBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgMixedProps;
	fgMixedProps = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgMixedProps->AddGrowableCol( 1 );
	fgMixedProps->SetFlexibleDirection( wxBOTH );
	fgMixedProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_mixedChamferRatioLabel = new wxStaticText( m_mixedProps, wxID_ANY, _("Chamfer size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedChamferRatioLabel->Wrap( -1 );
	m_mixedChamferRatioLabel->SetToolTip( _("Chamfer size in percent of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgMixedProps->Add( m_mixedChamferRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxHORIZONTAL );

	m_mixedChamferRatioCtrl = new TEXT_CTRL_EVAL( m_mixedProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer30->Add( m_mixedChamferRatioCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_mixedChamferRatioUnits = new wxStaticText( m_mixedProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedChamferRatioUnits->Wrap( -1 );
	bSizer30->Add( m_mixedChamferRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgMixedProps->Add( bSizer30, 1, wxEXPAND, 5 );

	m_staticTextChamferCorner1 = new wxStaticText( m_mixedProps, wxID_ANY, _("Chamfer corners:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferCorner1->Wrap( -1 );
	m_staticTextChamferCorner1->SetToolTip( _("Chamfered corners. The position is relative to a pad orientation 0 degree.") );

	fgMixedProps->Add( m_staticTextChamferCorner1, 0, 0, 5 );

	wxGridSizer* gMixedCornersSizer;
	gMixedCornersSizer = new wxGridSizer( 0, 2, 2, 6 );

	m_cbTopLeft1 = new wxCheckBox( m_mixedProps, wxID_ANY, _("Top left"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTopLeft1->SetValue(true);
	gMixedCornersSizer->Add( m_cbTopLeft1, 0, 0, 5 );

	m_cbTopRight1 = new wxCheckBox( m_mixedProps, wxID_ANY, _("Top right"), wxDefaultPosition, wxDefaultSize, 0 );
	gMixedCornersSizer->Add( m_cbTopRight1, 0, 0, 5 );

	m_cbBottomLeft1 = new wxCheckBox( m_mixedProps, wxID_ANY, _("Bottom left"), wxDefaultPosition, wxDefaultSize, 0 );
	gMixedCornersSizer->Add( m_cbBottomLeft1, 0, 0, 5 );

	m_cbBottomRight1 = new wxCheckBox( m_mixedProps, wxID_ANY, _("Bottom right"), wxDefaultPosition, wxDefaultSize, 0 );
	gMixedCornersSizer->Add( m_cbBottomRight1, 0, 0, 5 );


	fgMixedProps->Add( gMixedCornersSizer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_mixedCornerRatioLabel = new wxStaticText( m_mixedProps, wxID_ANY, _("Corner size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedCornerRatioLabel->Wrap( -1 );
	m_mixedCornerRatioLabel->SetToolTip( _("Corner radius in percent  of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgMixedProps->Add( m_mixedCornerRatioLabel, 0, wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );

	m_mixedCornerRatioCtrl = new TEXT_CTRL_EVAL( m_mixedProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_mixedCornerRatioCtrl, 1, wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_mixedCornerRatioUnits = new wxStaticText( m_mixedProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedCornerRatioUnits->Wrap( -1 );
	bSizer31->Add( m_mixedCornerRatioUnits, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgMixedProps->Add( bSizer31, 1, wxEXPAND, 5 );


	m_mixedProps->SetSizer( fgMixedProps );
	m_mixedProps->Layout();
	fgMixedProps->Fit( m_mixedProps );
	m_shapePropsBook->AddPage( m_mixedProps, _("a page"), false );

	gbSizerPadOrientation->Add( m_shapePropsBook, wxGBPosition( 1, 0 ), wxGBSpan( 1, 6 ), wxEXPAND|wxLEFT, 25 );

	m_sizeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXLabel->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeXLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_sizeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	gbSizerPadOrientation->Add( m_sizeXCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeXUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_sizeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYLabel->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeYLabel, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_sizeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerPadOrientation->Add( m_sizeYCtrl, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeYUnits, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PadOrientText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Angle:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_PadOrientText->Wrap( -1 );
	gbSizerPadOrientation->Add( m_PadOrientText, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_cb_padrotation = new wxComboBox( m_panelGeneral, wxID_ANY, _("0"), wxDefaultPosition, wxSize( 100,-1 ), 0, NULL, 0 );
	m_cb_padrotation->Append( _("0") );
	m_cb_padrotation->Append( _("90") );
	m_cb_padrotation->Append( _("-90") );
	m_cb_padrotation->Append( _("180") );
	m_cb_padrotation->SetSelection( 0 );
	gbSizerPadOrientation->Add( m_cb_padrotation, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_orientationUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_orientationUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizerPadOrientation->AddGrowableCol( 1 );
	gbSizerPadOrientation->AddGrowableCol( 4 );

	m_LeftBoxSizer->Add( gbSizerPadOrientation, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer35;
	bSizer35 = new wxBoxSizer( wxHORIZONTAL );

	m_offsetShapeOpt = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Offset shape from hole"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer35->Add( m_offsetShapeOpt, 0, wxTOP|wxLEFT, 5 );

	m_offsetShapeOptLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetShapeOptLabel->Wrap( -1 );
	bSizer35->Add( m_offsetShapeOptLabel, 0, wxTOP|wxRIGHT, 5 );


	m_LeftBoxSizer->Add( bSizer35, 0, wxEXPAND|wxTOP, 5 );


	m_LeftBoxSizer->Add( 0, 2, 0, wxEXPAND, 5 );

	m_offsetCtrls = new wxFlexGridSizer( 0, 6, 0, 5 );
	m_offsetCtrls->AddGrowableCol( 1 );
	m_offsetCtrls->AddGrowableCol( 4 );
	m_offsetCtrls->SetFlexibleDirection( wxBOTH );
	m_offsetCtrls->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_offsetXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXLabel->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetXLabel, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxLEFT, 5 );

	m_offsetXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetCtrls->Add( m_offsetXCtrl, 0, wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxALIGN_CENTER_VERTICAL, 5 );

	m_offsetXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXUnits->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );

	m_offsetYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYLabel->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_offsetYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetCtrls->Add( m_offsetYCtrl, 0, wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxALIGN_CENTER_VERTICAL, 5 );

	m_offsetYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYUnits->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );


	m_LeftBoxSizer->Add( m_offsetCtrls, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 25 );

	m_staticline7 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline7, 0, wxEXPAND|wxTOP|wxBOTTOM, 12 );

	m_gbSizerHole = new wxGridBagSizer( 4, 5 );
	m_gbSizerHole->SetFlexibleDirection( wxBOTH );
	m_gbSizerHole->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_holeShapeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeShapeLabel->Wrap( -1 );
	m_gbSizerHole->Add( m_holeShapeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_holeShapeCtrlChoices[] = { _("Round"), _("Oblong") };
	int m_holeShapeCtrlNChoices = sizeof( m_holeShapeCtrlChoices ) / sizeof( wxString );
	m_holeShapeCtrl = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_holeShapeCtrlNChoices, m_holeShapeCtrlChoices, 0 );
	m_holeShapeCtrl->SetSelection( 1 );
	m_gbSizerHole->Add( m_holeShapeCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 5 ), wxEXPAND|wxRIGHT, 5 );

	m_holeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXLabel->Wrap( -1 );
	m_gbSizerHole->Add( m_holeXLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_holeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_gbSizerHole->Add( m_holeXCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_holeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXUnits->Wrap( -1 );
	m_gbSizerHole->Add( m_holeXUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_holeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYLabel->Wrap( -1 );
	m_gbSizerHole->Add( m_holeYLabel, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_holeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_gbSizerHole->Add( m_holeYCtrl, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_holeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYUnits->Wrap( -1 );
	m_gbSizerHole->Add( m_holeYUnits, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_gbSizerHole->AddGrowableCol( 1 );
	m_gbSizerHole->AddGrowableCol( 4 );

	m_LeftBoxSizer->Add( m_gbSizerHole, 0, wxEXPAND, 5 );

	m_staticline71 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline71, 0, wxEXPAND|wxTOP|wxBOTTOM, 12 );

	wxFlexGridSizer* fgSizerPadToDie;
	fgSizerPadToDie = new wxFlexGridSizer( 14, 3, 0, 0 );
	fgSizerPadToDie->AddGrowableCol( 1 );
	fgSizerPadToDie->SetFlexibleDirection( wxBOTH );
	fgSizerPadToDie->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxBoxSizer* bSizer34;
	bSizer34 = new wxBoxSizer( wxHORIZONTAL );

	m_padToDieOpt = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Specify pad to die length"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer34->Add( m_padToDieOpt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_padToDieLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieLabel->Wrap( -1 );
	bSizer34->Add( m_padToDieLabel, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );


	fgSizerPadToDie->Add( bSizer34, 0, wxEXPAND|wxRIGHT, 5 );

	m_padToDieCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPadToDie->Add( m_padToDieCtrl, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_padToDieUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieUnits->Wrap( -1 );
	fgSizerPadToDie->Add( m_padToDieUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );

	wxBoxSizer* bSizer341;
	bSizer341 = new wxBoxSizer( wxHORIZONTAL );

	m_padToDieDelayOpt = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Specify pad to die delay"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer341->Add( m_padToDieDelayOpt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_padToDieDelayLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieDelayLabel->Wrap( -1 );
	bSizer341->Add( m_padToDieDelayLabel, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );


	fgSizerPadToDie->Add( bSizer341, 1, wxEXPAND, 5 );

	m_padToDieDelayCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPadToDie->Add( m_padToDieDelayCtrl, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxTOP, 5 );

	m_padToDieDelayUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieDelayUnits->Wrap( -1 );
	fgSizerPadToDie->Add( m_padToDieDelayUnits, 0, wxALL, 5 );


	m_LeftBoxSizer->Add( fgSizerPadToDie, 0, wxEXPAND, 5 );


	bGeneralSizer->Add( m_LeftBoxSizer, 0, wxEXPAND|wxALL, 5 );

	m_middleBoxSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* m_LayersSizer;
	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelGeneral, wxID_ANY, wxEmptyString ), wxVERTICAL );

	m_FlippedWarningSizer = new wxBoxSizer( wxHORIZONTAL );

	m_FlippedWarningIcon = new wxStaticBitmap( m_LayersSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_FlippedWarningIcon->SetMinSize( wxSize( 48,48 ) );

	m_FlippedWarningSizer->Add( m_FlippedWarningIcon, 0, wxALIGN_TOP|wxBOTTOM|wxTOP, 4 );

	m_staticText86 = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Parent footprint on board is flipped.\nLayers will be reversed."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText86->Wrap( 150 );
	m_FlippedWarningSizer->Add( m_staticText86, 1, wxALIGN_TOP|wxBOTTOM|wxLEFT|wxRIGHT, 8 );


	m_LayersSizer->Add( m_FlippedWarningSizer, 0, wxEXPAND|wxBOTTOM, 10 );

	m_copperLayersLabel = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_copperLayersLabel->Wrap( -1 );
	m_LayersSizer->Add( m_copperLayersLabel, 0, wxRIGHT|wxLEFT, 4 );

	wxArrayString m_rbCopperLayersSelChoices;
	m_rbCopperLayersSel = new wxChoice( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rbCopperLayersSelChoices, 0 );
	m_rbCopperLayersSel->SetSelection( 0 );
	m_LayersSizer->Add( m_rbCopperLayersSel, 1, wxALL|wxEXPAND|wxTOP, 4 );

	wxBoxSizer* bSizer444;
	bSizer444 = new wxBoxSizer( wxHORIZONTAL );

	m_techLayersLabel = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Technical Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	bSizer444->Add( m_techLayersLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline52 = new wxStaticLine( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer444->Add( m_staticline52, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 3 );


	m_LayersSizer->Add( bSizer444, 1, wxEXPAND|wxTOP, 5 );

	m_layerFrontAdhesive = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerFrontAdhesive, 0, wxLEFT|wxRIGHT, 4 );

	m_layerBackAdhesive = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerBackAdhesive, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerFrontPaste = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerFrontPaste, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerBackPaste = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerBackPaste, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerFrontSilk = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerFrontSilk, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerBackSilk = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerBackSilk, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerFrontMask = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerFrontMask, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerBackMask = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerBackMask, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerUserDwgs = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Drafting notes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerUserDwgs, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerECO1 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerECO1, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_layerECO2 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_layerECO2, 0, wxALL, 4 );


	m_middleBoxSizer->Add( m_LayersSizer, 0, wxEXPAND|wxALL, 5 );

	m_staticTextFabProperty = new wxStaticText( m_panelGeneral, wxID_ANY, _("Fabrication property:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFabProperty->Wrap( -1 );
	m_staticTextFabProperty->SetToolTip( _("Optional property to specify a special purpose or constraint in fabrication files:\nBGA attribute is for pads in BGA footprints\nFiducial local is a fiducial for the parent footprint\nFiducial global is a fiducial for the whole board\nTest Point specifies an electrical test point\nHeatsink specifies a thermal pad\nCastellated specifies a through hole pad on a board edge\nMechanical specifies a through-hole pad that is used for mechanical support") );

	m_middleBoxSizer->Add( m_staticTextFabProperty, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_middleBoxSizer->Add( 0, 2, 0, wxEXPAND, 5 );

	wxString m_choiceFabPropertyChoices[] = { _("None"), _("BGA pad"), _("Fiducial, local to footprint"), _("Fiducial, global to board"), _("Test point pad"), _("Heatsink pad"), _("Mechanical"), _("Castellated pad (through hole only)"), _("Press-fit (round through hole only)") };
	int m_choiceFabPropertyNChoices = sizeof( m_choiceFabPropertyChoices ) / sizeof( wxString );
	m_choiceFabProperty = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFabPropertyNChoices, m_choiceFabPropertyChoices, 0 );
	m_choiceFabProperty->SetSelection( 0 );
	m_middleBoxSizer->Add( m_choiceFabProperty, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bGeneralSizer->Add( m_middleBoxSizer, 0, wxEXPAND|wxALL, 3 );


	m_panelGeneral->SetSizer( bGeneralSizer );
	m_panelGeneral->Layout();
	bGeneralSizer->Fit( m_panelGeneral );
	m_notebook->AddPage( m_panelGeneral, _("General"), true );
	m_connectionsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelConnections;
	bSizerPanelConnections = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerConnectionsMargins;
	bSizerConnectionsMargins = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* bSizerTeardrops;
	bSizerTeardrops = new wxStaticBoxSizer( new wxStaticBox( m_connectionsPanel, wxID_ANY, _("Teardrops") ), wxVERTICAL );

	m_legacyTeardropsWarning = new wxBoxSizer( wxHORIZONTAL );

	m_legacyTeardropsIcon = new wxStaticBitmap( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_legacyTeardropsWarning->Add( m_legacyTeardropsIcon, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxVERTICAL );

	m_staticText85 = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Board contains legacy teardrops."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText85->Wrap( -1 );
	bSizer42->Add( m_staticText85, 0, wxRIGHT|wxLEFT, 5 );

	m_staticText851 = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Use Edit > Edit Teardrops to apply automatic teardrops."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText851->Wrap( -1 );
	bSizer42->Add( m_staticText851, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_legacyTeardropsWarning->Add( bSizer42, 1, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerTeardrops->Add( m_legacyTeardropsWarning, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizerCols11;
	bSizerCols11 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol11;
	bSizerLeftCol11 = new wxBoxSizer( wxVERTICAL );

	m_cbTeardrops = new wxCheckBox( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Add teardrops on pad's track connections"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol11->Add( m_cbTeardrops, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbPreferZoneConnection = new wxCheckBox( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Prefer zone connection"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPreferZoneConnection->SetValue(true);
	m_cbPreferZoneConnection->SetToolTip( _("Do not create teardrops on tracks connected to pads that are also connected to a copper zone.") );

	bSizerLeftCol11->Add( m_cbPreferZoneConnection, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Allow teardrops to span 2 track segments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTeardropsUseNextTrack->SetValue(true);
	m_cbTeardropsUseNextTrack->SetToolTip( _("Allows a teardrop to spread over 2 tracks if the first track segment is too short") );

	bSizerLeftCol11->Add( m_cbTeardropsUseNextTrack, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerCols11->Add( bSizerLeftCol11, 1, wxEXPAND|wxTOP, 3 );


	bSizerCols11->Add( 15, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol11;
	bSizerRightCol11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer39;
	bSizer39 = new wxBoxSizer( wxHORIZONTAL );

	m_stHDRatio = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Track width limit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	bSizer39->Add( m_stHDRatio, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropHDPercent = new wxSpinCtrlDouble( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 90, 10 );
	m_spTeardropHDPercent->SetDigits( 0 );
	m_spTeardropHDPercent->SetToolTip( _("Tracks which are similar in size to the pad do not need teardrops.") );

	bSizer39->Add( m_spTeardropHDPercent, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_minTrackWidthUnits = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthUnits->Wrap( -1 );
	bSizer39->Add( m_minTrackWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_minTrackWidthHint = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint->Wrap( -1 );
	m_minTrackWidthHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer39->Add( m_minTrackWidthHint, 0, wxBOTTOM|wxALIGN_BOTTOM, 3 );

	m_staticText87 = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText87->Wrap( -1 );
	bSizer39->Add( m_staticText87, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRightCol11->Add( bSizer39, 0, 0, 3 );


	bSizerCols11->Add( bSizerRightCol11, 1, wxEXPAND|wxLEFT, 10 );


	bSizerTeardrops->Add( bSizerCols11, 0, wxEXPAND, 5 );


	bSizerTeardrops->Add( 0, 5, 0, wxEXPAND, 5 );

	m_teardropShapeLabel = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Teardrop Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_teardropShapeLabel->Wrap( -1 );
	bSizerTeardrops->Add( m_teardropShapeLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline51 = new wxStaticLine( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerTeardrops->Add( m_staticline51, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerShapeColumns;
	bSizerShapeColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop = new wxStaticBitmap( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol->Add( m_bitmapTeardrop, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


	bSizerShapeColumns->Add( bSizerLeftCol, 1, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns->Add( 10, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer43;
	bSizer43 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerRightCol;
	fgSizerRightCol = new wxFlexGridSizer( 0, 3, 2, 0 );
	fgSizerRightCol->AddGrowableCol( 1 );
	fgSizerRightCol->SetFlexibleDirection( wxBOTH );
	fgSizerRightCol->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stHsetting = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHsetting->Wrap( -1 );
	fgSizerRightCol->Add( m_stHsetting, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_spTeardropLenPercent = new wxSpinCtrlDouble( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 20, 200, 40.000000, 10 );
	m_spTeardropLenPercent->SetDigits( 0 );
	fgSizerRightCol->Add( m_spTeardropLenPercent, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );

	m_stLenPercentUnits = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentUnits->Wrap( -1 );
	bSizer131->Add( m_stLenPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stLenPercentHint = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentHint->Wrap( -1 );
	m_stLenPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer131->Add( m_stLenPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText88 = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText88->Wrap( -1 );
	bSizer131->Add( m_staticText88, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRightCol->Add( bSizer131, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLen, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcTdMaxLen = new wxTextCtrl( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcTdMaxLen, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_stMaxLenUnits = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLenUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerRightCol->Add( 0, 5, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );

	m_stVsetting = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stVsetting->Wrap( -1 );
	fgSizerRightCol->Add( m_stVsetting, 0, wxALIGN_CENTER_VERTICAL, 10 );

	m_spTeardropSizePercent = new wxSpinCtrlDouble( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 60, 100, 100.000000, 10 );
	m_spTeardropSizePercent->SetDigits( 0 );
	fgSizerRightCol->Add( m_spTeardropSizePercent, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	m_stWidthPercentUnits = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentUnits->Wrap( -1 );
	bSizer13->Add( m_stWidthPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidthPercentHint = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentHint->Wrap( -1 );
	m_stWidthPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer13->Add( m_stWidthPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_staticText89 = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText89->Wrap( -1 );
	bSizer13->Add( m_staticText89, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRightCol->Add( bSizer13, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stTdMaxSize = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTdMaxSize->Wrap( -1 );
	fgSizerRightCol->Add( m_stTdMaxSize, 0, wxALIGN_CENTER_VERTICAL, 10 );

	m_tcMaxHeight = new wxTextCtrl( bSizerTeardrops->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcMaxHeight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_stMaxHeightUnits = new wxStaticText( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxHeightUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer43->Add( fgSizerRightCol, 1, wxEXPAND|wxTOP|wxLEFT, 10 );

	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxHORIZONTAL );

	m_curvedEdges = new wxCheckBox( bSizerTeardrops->GetStaticBox(), wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer44->Add( m_curvedEdges, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer43->Add( bSizer44, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 8 );


	bSizerShapeColumns->Add( bSizer43, 1, wxEXPAND, 5 );


	bSizerTeardrops->Add( bSizerShapeColumns, 1, wxBOTTOM|wxEXPAND|wxRIGHT, 5 );


	bSizerConnectionsMargins->Add( bSizerTeardrops, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerConnectionsLower;
	bSizerConnectionsLower = new wxBoxSizer( wxHORIZONTAL );

	m_sbSizerZonesSettings = new wxStaticBoxSizer( new wxStaticBox( m_connectionsPanel, wxID_ANY, _("Connection to Copper Zones") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerCopperZonesOpts;
	fgSizerCopperZonesOpts = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgSizerCopperZonesOpts->AddGrowableCol( 1 );
	fgSizerCopperZonesOpts->SetFlexibleDirection( wxBOTH );
	fgSizerCopperZonesOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_padConnectionLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padConnectionLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_padConnectionLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("From parent footprint"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneConnectionChoice, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_zoneKnockoutLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Zone knockout:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoneKnockoutLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_zoneKnockoutLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_ZoneCustomPadShapeChoices[] = { _("Pad shape"), _("Pad convex hull") };
	int m_ZoneCustomPadShapeNChoices = sizeof( m_ZoneCustomPadShapeChoices ) / sizeof( wxString );
	m_ZoneCustomPadShape = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneCustomPadShapeNChoices, m_ZoneCustomPadShapeChoices, 0 );
	m_ZoneCustomPadShape->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneCustomPadShape, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	m_sbSizerZonesSettings->Add( fgSizerCopperZonesOpts, 0, 0, 5 );


	bSizerConnectionsLower->Add( m_sbSizerZonesSettings, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerThermalReliefs;
	sbSizerThermalReliefs = new wxStaticBoxSizer( new wxStaticBox( m_connectionsPanel, wxID_ANY, _("Thermal Relief Overrides") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerThermalReliefs;
	fgSizerThermalReliefs = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizerThermalReliefs->AddGrowableCol( 1 );
	fgSizerThermalReliefs->SetFlexibleDirection( wxBOTH );
	fgSizerThermalReliefs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_thermalGapLabel = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("Relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapLabel->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_thermalGapLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_thermalGapCtrl = new wxTextCtrl( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThermalReliefs->Add( m_thermalGapCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 3 );

	m_thermalGapUnits = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapUnits->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_thermalGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_spokeWidthLabel = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("Spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_spokeWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spokeWidthCtrl = new wxTextCtrl( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThermalReliefs->Add( m_spokeWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_spokeWidthUnits = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_spokeWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_spokeAngleLabel = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("Spoke angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeAngleLabel->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_spokeAngleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_spokeAngleCtrl = new wxTextCtrl( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThermalReliefs->Add( m_spokeAngleCtrl, 0, wxEXPAND|wxLEFT, 3 );

	m_spokeAngleUnits = new wxStaticText( sbSizerThermalReliefs->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeAngleUnits->Wrap( -1 );
	fgSizerThermalReliefs->Add( m_spokeAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizerThermalReliefs->Add( fgSizerThermalReliefs, 1, wxBOTTOM|wxEXPAND, 5 );


	bSizerConnectionsLower->Add( sbSizerThermalReliefs, 1, wxEXPAND|wxALL, 5 );


	bSizerConnectionsMargins->Add( bSizerConnectionsLower, 0, wxEXPAND|wxTOP, 10 );


	bSizerPanelConnections->Add( bSizerConnectionsMargins, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_connectionsPanel->SetSizer( bSizerPanelConnections );
	m_connectionsPanel->Layout();
	bSizerPanelConnections->Fit( m_connectionsPanel );
	m_notebook->AddPage( m_connectionsPanel, _("Connections"), false );
	m_localSettingsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearance;
	bSizerPanelClearance = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerClearance;
	bSizerClearance = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbClearancesSizer;
	sbClearancesSizer = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Clearance Overrides") ), wxVERTICAL );

	wxStaticText* m_staticTextHint;
	m_staticTextHint = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Leave values blank to use parent footprint or netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHint->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextHint, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticTextInfoPosValue = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPosValue->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextInfoPosValue, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoNegVal = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoNegVal->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextInfoNegVal, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgClearancesGridSizer;
	fgClearancesGridSizer = new wxFlexGridSizer( 4, 3, 4, 0 );
	fgClearancesGridSizer->AddGrowableCol( 1 );
	fgClearancesGridSizer->SetFlexibleDirection( wxBOTH );
	fgClearancesGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_clearanceLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	m_clearanceLabel->SetToolTip( _("This is the local net clearance for this pad.\nIf 0, the footprint local value or the Netclass value is used.") );

	fgClearancesGridSizer->Add( m_clearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_clearanceCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_clearanceCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_clearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maskMarginLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder mask expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginLabel->Wrap( -1 );
	m_maskMarginLabel->SetToolTip( _("This is the local clearance between this pad and the solder mask.\nIf 0, the footprint local value or the global value is used.") );

	fgClearancesGridSizer->Add( m_maskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_maskMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_maskMarginCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 15 );

	m_maskMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_maskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginLabel->Wrap( -1 );
	m_pasteMarginLabel->SetToolTip( _("Solder paste clearance relative to pad size.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nThis value can be superseded by local values for a footprint or a pad.") );

	fgClearancesGridSizer->Add( m_pasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_pasteMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginCtrl->SetToolTip( _("Local solder paste clearance for this pad.\nEnter an absolute value (e.g., -0.1mm), a percentage (e.g., -5%), or both (e.g., -0.1mm - 5%).\nIf blank, the footprint or global value is used.") );

	fgClearancesGridSizer->Add( m_pasteMarginCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_pasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pasteMarginRatioLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioLabel->Wrap( -1 );
	m_pasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio in percent between this pad and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nIf 0, the footprint value or the global value is used.\nThe final clearance value is the sum of this value and the clearance value.\nA negative value means a smaller mask size than pad size.") );

	fgClearancesGridSizer->Add( m_pasteMarginRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pasteMarginRatioCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_pasteMarginRatioCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_pasteMarginRatioUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_pasteMarginRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbClearancesSizer->Add( fgClearancesGridSizer, 0, wxTOP|wxBOTTOM, 10 );

	m_nonCopperWarningBook = new wxSimplebook( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* notePanel;
	notePanel = new wxPanel( m_nonCopperWarningBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bNoteSizer;
	bNoteSizer = new wxBoxSizer( wxVERTICAL );

	m_nonCopperNote = new wxStaticText( notePanel, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_nonCopperNote->Wrap( -1 );
	bNoteSizer->Add( m_nonCopperNote, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( notePanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPaste->Wrap( -1 );
	bNoteSizer->Add( m_staticTextInfoPaste, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	notePanel->SetSizer( bNoteSizer );
	notePanel->Layout();
	bNoteSizer->Fit( notePanel );
	m_nonCopperWarningBook->AddPage( notePanel, _("a page"), false );
	wxPanel* warningPanel;
	warningPanel = new wxPanel( m_nonCopperWarningBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bWarningSizer;
	bWarningSizer = new wxBoxSizer( wxHORIZONTAL );

	bWarningSizer->SetMinSize( wxSize( -1,50 ) );
	m_nonCopperWarningIcon = new wxStaticBitmap( warningPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_nonCopperWarningIcon->SetMinSize( wxSize( 48,48 ) );

	bWarningSizer->Add( m_nonCopperWarningIcon, 0, wxALL, 5 );

	m_nonCopperWarningText = new wxStaticText( warningPanel, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_nonCopperWarningText->Wrap( -1 );
	bWarningSizer->Add( m_nonCopperWarningText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	warningPanel->SetSizer( bWarningSizer );
	warningPanel->Layout();
	bWarningSizer->Fit( warningPanel );
	m_nonCopperWarningBook->AddPage( warningPanel, _("a page"), false );

	sbClearancesSizer->Add( m_nonCopperWarningBook, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizerClearance->Add( sbClearancesSizer, 0, wxALL|wxEXPAND, 5 );


	bSizerPanelClearance->Add( bSizerClearance, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_localSettingsPanel->SetSizer( bSizerPanelClearance );
	m_localSettingsPanel->Layout();
	bSizerPanelClearance->Fit( m_localSettingsPanel );
	m_notebook->AddPage( m_localSettingsPanel, _("Clearance Overrides"), false );
	m_backDrillPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer45;
	bSizer45 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer46;
	bSizer46 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbPostMachining;
	sbPostMachining = new wxStaticBoxSizer( new wxStaticBox( m_backDrillPanel, wxID_ANY, _("Hole Post Machining") ), wxVERTICAL );

	wxBoxSizer* bPostMachiningColumns;
	bPostMachiningColumns = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgPostMachiningTop;
	fgPostMachiningTop = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgPostMachiningTop->AddGrowableCol( 1 );
	fgPostMachiningTop->SetFlexibleDirection( wxBOTH );
	fgPostMachiningTop->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_topPostMachiningLabel = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Top:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topPostMachiningLabel->Wrap( -1 );
	fgPostMachiningTop->Add( m_topPostMachiningLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_topPostMachiningChoices[] = { _("None"), _("Countersink"), _("Counterbore") };
	int m_topPostMachiningNChoices = sizeof( m_topPostMachiningChoices ) / sizeof( wxString );
	m_topPostMachining = new wxChoice( sbPostMachining->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_topPostMachiningNChoices, m_topPostMachiningChoices, 0 );
	m_topPostMachining->SetSelection( 0 );
	fgPostMachiningTop->Add( m_topPostMachining, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgPostMachiningTop->Add( 0, 0, 1, wxEXPAND, 5 );

	m_topPostMachineSize1Label = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topPostMachineSize1Label->Wrap( -1 );
	fgPostMachiningTop->Add( m_topPostMachineSize1Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_topPostmachineSize1 = new wxTextCtrl( sbPostMachining->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgPostMachiningTop->Add( m_topPostmachineSize1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_topPostMachineSize1Units = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topPostMachineSize1Units->Wrap( -1 );
	fgPostMachiningTop->Add( m_topPostMachineSize1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_topPostMachineSize2Label = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topPostMachineSize2Label->Wrap( -1 );
	fgPostMachiningTop->Add( m_topPostMachineSize2Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_topPostMachineSize2 = new wxTextCtrl( sbPostMachining->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgPostMachiningTop->Add( m_topPostMachineSize2, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_topPostMachineSize2Units = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_topPostMachineSize2Units->Wrap( -1 );
	fgPostMachiningTop->Add( m_topPostMachineSize2Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bPostMachiningColumns->Add( fgPostMachiningTop, 1, wxEXPAND, 5 );


	bPostMachiningColumns->Add( 15, 0, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgPostMachiningBottom;
	fgPostMachiningBottom = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgPostMachiningBottom->AddGrowableCol( 1 );
	fgPostMachiningBottom->SetFlexibleDirection( wxBOTH );
	fgPostMachiningBottom->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_bottomPostMachiningLabel = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomPostMachiningLabel->Wrap( -1 );
	fgPostMachiningBottom->Add( m_bottomPostMachiningLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_bottomPostMachiningChoices[] = { _("None"), _("Countersink"), _("Counterbore") };
	int m_bottomPostMachiningNChoices = sizeof( m_bottomPostMachiningChoices ) / sizeof( wxString );
	m_bottomPostMachining = new wxChoice( sbPostMachining->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_bottomPostMachiningNChoices, m_bottomPostMachiningChoices, 0 );
	m_bottomPostMachining->SetSelection( 0 );
	fgPostMachiningBottom->Add( m_bottomPostMachining, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgPostMachiningBottom->Add( 0, 0, 1, wxALL|wxEXPAND, 5 );

	m_bottomPostMachineSize1Label = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomPostMachineSize1Label->Wrap( -1 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize1Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bottomPostMachineSize1 = new wxTextCtrl( sbPostMachining->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bottomPostMachineSize1Units = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomPostMachineSize1Units->Wrap( -1 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bottomPostMachineSize2Label = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomPostMachineSize2Label->Wrap( -1 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize2Label, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_bottomPostMachineSize2 = new wxTextCtrl( sbPostMachining->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize2, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bottomPostMachineSize2Units = new wxStaticText( sbPostMachining->GetStaticBox(), wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bottomPostMachineSize2Units->Wrap( -1 );
	fgPostMachiningBottom->Add( m_bottomPostMachineSize2Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bPostMachiningColumns->Add( fgPostMachiningBottom, 1, wxEXPAND, 5 );


	sbPostMachining->Add( bPostMachiningColumns, 1, wxEXPAND, 5 );


	bSizer46->Add( sbPostMachining, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbBackdrill;
	sbBackdrill = new wxStaticBoxSizer( new wxStaticBox( m_backDrillPanel, wxID_ANY, _("Backdrill") ), wxVERTICAL );

	wxString m_backDrillChoiceChoices[] = { _("None"), _("Top"), _("Bottom"), _("Top & bottom") };
	int m_backDrillChoiceNChoices = sizeof( m_backDrillChoiceChoices ) / sizeof( wxString );
	m_backDrillChoice = new wxChoice( sbBackdrill->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_backDrillChoiceNChoices, m_backDrillChoiceChoices, 0 );
	m_backDrillChoice->SetSelection( 0 );
	sbBackdrill->Add( m_backDrillChoice, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bBackdrillColumns;
	bBackdrillColumns = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgBackdrillTop;
	fgBackdrillTop = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgBackdrillTop->AddGrowableCol( 1 );
	fgBackdrillTop->SetFlexibleDirection( wxBOTH );
	fgBackdrillTop->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_backDrillTopLayerLabel = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("Top backdrill must-cut:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillTopLayerLabel->Wrap( -1 );
	m_backDrillTopLayerLabel->SetToolTip( _("The backdrill must pass through this layer") );

	fgBackdrillTop->Add( m_backDrillTopLayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backDrillTopLayer = new wxBitmapComboBox( sbBackdrill->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_backDrillTopLayer->SetToolTip( _("The backdrill must pass through this layer") );

	fgBackdrillTop->Add( m_backDrillTopLayer, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	fgBackdrillTop->Add( 0, 0, 1, wxEXPAND, 5 );

	m_backDrillTopSizeLabel = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillTopSizeLabel->Wrap( -1 );
	fgBackdrillTop->Add( m_backDrillTopSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backDrillTopSize = new wxTextCtrl( sbBackdrill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgBackdrillTop->Add( m_backDrillTopSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_backDrillTopSizeUnits = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillTopSizeUnits->Wrap( -1 );
	fgBackdrillTop->Add( m_backDrillTopSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bBackdrillColumns->Add( fgBackdrillTop, 1, wxEXPAND, 5 );


	bBackdrillColumns->Add( 15, 0, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgBackdrillBottom;
	fgBackdrillBottom = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgBackdrillBottom->AddGrowableCol( 1 );
	fgBackdrillBottom->SetFlexibleDirection( wxBOTH );
	fgBackdrillBottom->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_backDrillBottomLayerLabel = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("Bottom backdrill must-cut:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillBottomLayerLabel->Wrap( -1 );
	m_backDrillBottomLayerLabel->SetToolTip( _("The backdrill must pass through this layer") );

	fgBackdrillBottom->Add( m_backDrillBottomLayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backDrillBottomLayer = new wxBitmapComboBox( sbBackdrill->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_backDrillBottomLayer->SetToolTip( _("The backdrill must pass through this layer") );

	fgBackdrillBottom->Add( m_backDrillBottomLayer, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	fgBackdrillBottom->Add( 0, 0, 1, wxEXPAND, 5 );

	m_backDrillBottomSizeLabel = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillBottomSizeLabel->Wrap( -1 );
	fgBackdrillBottom->Add( m_backDrillBottomSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_backDrillBottomSize = new wxTextCtrl( sbBackdrill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgBackdrillBottom->Add( m_backDrillBottomSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_backDrillBottomSizeUnits = new wxStaticText( sbBackdrill->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backDrillBottomSizeUnits->Wrap( -1 );
	fgBackdrillBottom->Add( m_backDrillBottomSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bBackdrillColumns->Add( fgBackdrillBottom, 1, wxEXPAND, 5 );


	sbBackdrill->Add( bBackdrillColumns, 1, wxEXPAND|wxTOP, 5 );


	bSizer46->Add( sbBackdrill, 0, wxEXPAND|wxTOP, 5 );


	bSizer45->Add( bSizer46, 1, wxALL|wxEXPAND, 5 );


	m_backDrillPanel->SetSizer( bSizer45 );
	m_backDrillPanel->Layout();
	bSizer45->Fit( m_backDrillPanel );
	m_notebook->AddPage( m_backDrillPanel, _("Backdrill"), false );

	bSizerUpper->Add( m_notebook, 0, wxEXPAND|wxTOP|wxBOTTOM, 12 );

	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );

	bSizerDisplayPad->SetMinSize( wxSize( 290,-1 ) );

	bSizerDisplayPad->Add( 0, 25, 0, wxEXPAND, 5 );

	m_stackupImagesBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );
	page0 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page0Sizer;
	page0Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage0 = new wxStaticBitmap( page0, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page0Sizer->Add( m_stackupImage0, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page0->SetSizer( page0Sizer );
	page0->Layout();
	page0Sizer->Fit( page0 );
	m_stackupImagesBook->AddPage( page0, _("a page"), false );
	page1 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page1Sizer;
	page1Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage1 = new wxStaticBitmap( page1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page1Sizer->Add( m_stackupImage1, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page1->SetSizer( page1Sizer );
	page1->Layout();
	page1Sizer->Fit( page1 );
	m_stackupImagesBook->AddPage( page1, _("a page"), false );
	page2 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page2Sizer;
	page2Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage2 = new wxStaticBitmap( page2, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page2Sizer->Add( m_stackupImage2, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page2->SetSizer( page2Sizer );
	page2->Layout();
	page2Sizer->Fit( page2 );
	m_stackupImagesBook->AddPage( page2, _("a page"), false );
	page3 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page3Sizer;
	page3Sizer = new wxBoxSizer( wxVERTICAL );


	page3->SetSizer( page3Sizer );
	page3->Layout();
	page3Sizer->Fit( page3 );
	m_stackupImagesBook->AddPage( page3, _("a page"), false );
	page4 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page4Sizer;
	page4Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage4 = new wxStaticBitmap( page4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page4Sizer->Add( m_stackupImage4, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page4->SetSizer( page4Sizer );
	page4->Layout();
	page4Sizer->Fit( page4 );
	m_stackupImagesBook->AddPage( page4, _("a page"), false );
	page5 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page5Sizer;
	page5Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage5 = new wxStaticBitmap( page5, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page5Sizer->Add( m_stackupImage5, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page5->SetSizer( page5Sizer );
	page5->Layout();
	page5Sizer->Fit( page5 );
	m_stackupImagesBook->AddPage( page5, _("a page"), false );
	page6 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page6Sizer;
	page6Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage6 = new wxStaticBitmap( page6, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page6Sizer->Add( m_stackupImage6, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page6->SetSizer( page6Sizer );
	page6->Layout();
	page6Sizer->Fit( page6 );
	m_stackupImagesBook->AddPage( page6, _("a page"), false );
	page7 = new wxPanel( m_stackupImagesBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* page7Sizer;
	page7Sizer = new wxBoxSizer( wxVERTICAL );

	m_stackupImage7 = new wxStaticBitmap( page7, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	page7Sizer->Add( m_stackupImage7, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	page7->SetSizer( page7Sizer );
	page7->Layout();
	page7Sizer->Fit( page7 );
	m_stackupImagesBook->AddPage( page7, _("a page"), false );

	bSizerDisplayPad->Add( m_stackupImagesBook, 0, wxEXPAND|wxRIGHT|wxLEFT, 7 );


	bSizerDisplayPad->Add( 0, 4, 0, wxEXPAND, 5 );

	m_boardViewPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_padPreviewSizer = new wxBoxSizer( wxVERTICAL );

	m_padPreviewSizer->SetMinSize( wxSize( 280,-1 ) );

	m_boardViewPanel->SetSizer( m_padPreviewSizer );
	m_boardViewPanel->Layout();
	m_padPreviewSizer->Fit( m_boardViewPanel );
	bSizerDisplayPad->Add( m_boardViewPanel, 1, wxEXPAND|wxRIGHT|wxLEFT, 2 );


	bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP, 5 );


	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND|wxLEFT, 8 );

	wxBoxSizer* bottomSizer;
	bottomSizer = new wxBoxSizer( wxHORIZONTAL );

	m_parentInfo = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_parentInfo->Wrap( -1 );
	bottomSizer->Add( m_parentInfo, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 8 );


	bottomSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_cbShowPadOutline = new wxCheckBox( this, wxID_ANY, _("Preview pad in sketch mode"), wxDefaultPosition, wxDefaultSize, 0 );
	bottomSizer->Add( m_cbShowPadOutline, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bottomSizer->Add( 40, 0, 0, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bottomSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	m_MainSizer->Add( bottomSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_panelGeneral->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUI ), NULL, this );
	m_padType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_padNumCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbPadstackMode->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadstackModeChanged ), NULL, this );
	m_cbEditLayer->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnEditLayerChanged ), NULL, this );
	m_PadShapeSelector->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_trapDeltaCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapAxisCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_cornerRatioCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cornerRadiusCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerRadiusChange ), NULL, this );
	m_chamferRatioCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_mixedChamferRatioCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_mixedCornerRatioCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_sizeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cb_padrotation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_cb_padrotation->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_offsetShapeOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnOffsetCheckbox ), NULL, this );
	m_offsetXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeShapeCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_padToDieOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieCheckbox ), NULL, this );
	m_padToDieDelayOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieDelayCheckbox ), NULL, this );
	m_rbCopperLayersSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetCopperLayers ), NULL, this );
	m_layerFrontAdhesive->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackAdhesive->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontPaste->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackPaste->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontSilk->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackSilk->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerUserDwgs->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerECO1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerECO2->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_cbTeardrops->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbTeardrops->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbPreferZoneConnection->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbPreferZoneConnection->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbTeardropsUseNextTrack->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatio->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropHDPercent->Connect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropHDPercent->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropHDPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_minTrackWidthUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_teardropShapeLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_bitmapTeardrop->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHsetting->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropLenPercent->Connect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropLenPercent->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropLenPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcTdMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLenUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stVsetting->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropSizePercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stTdMaxSize->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcMaxHeight->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxHeightUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_ZoneConnectionChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_ZoneCustomPadShape->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_thermalGapCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spokeWidthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spokeAngleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_clearanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_maskMarginCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pasteMarginCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pasteMarginRatioCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_topPostMachining->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTopPostMachining ), NULL, this );
	m_bottomPostMachining->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onBottomPostMachining ), NULL, this );
	m_backDrillChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onBackDrillChoice ), NULL, this );
	m_cbShowPadOutline->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onChangePadMode ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_PAD_PROPERTIES_BASE::~DIALOG_PAD_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_panelGeneral->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUI ), NULL, this );
	m_padType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_padNumCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbPadstackMode->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadstackModeChanged ), NULL, this );
	m_cbEditLayer->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnEditLayerChanged ), NULL, this );
	m_PadShapeSelector->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_trapDeltaCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapAxisCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_cornerRatioCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cornerRadiusCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerRadiusChange ), NULL, this );
	m_chamferRatioCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_mixedChamferRatioCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_cbTopLeft1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbTopRight1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomLeft1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cbBottomRight1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_mixedCornerRatioCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_sizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_sizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_cb_padrotation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_cb_padrotation->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_offsetShapeOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnOffsetCheckbox ), NULL, this );
	m_offsetXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeShapeCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_padToDieOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieCheckbox ), NULL, this );
	m_padToDieDelayOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieDelayCheckbox ), NULL, this );
	m_rbCopperLayersSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetCopperLayers ), NULL, this );
	m_layerFrontAdhesive->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackAdhesive->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontPaste->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackPaste->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontSilk->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackSilk->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerFrontMask->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerBackMask->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerUserDwgs->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerECO1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_layerECO2->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_cbTeardrops->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbTeardrops->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbPreferZoneConnection->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbPreferZoneConnection->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_cbTeardropsUseNextTrack->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatio->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropHDPercent->Disconnect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropHDPercent->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropHDPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_minTrackWidthUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_teardropShapeLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_bitmapTeardrop->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHsetting->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropLenPercent->Disconnect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropLenPercent->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spTeardropLenPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcTdMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLenUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stVsetting->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_spTeardropSizePercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stTdMaxSize->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcMaxHeight->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxHeightUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_ZoneConnectionChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_ZoneCustomPadShape->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_thermalGapCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spokeWidthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_spokeAngleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onModify ), NULL, this );
	m_clearanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_maskMarginCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pasteMarginCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_pasteMarginRatioCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_topPostMachining->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onTopPostMachining ), NULL, this );
	m_bottomPostMachining->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onBottomPostMachining ), NULL, this );
	m_backDrillChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onBackDrillChoice ), NULL, this );
	m_cbShowPadOutline->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onChangePadMode ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );

}
