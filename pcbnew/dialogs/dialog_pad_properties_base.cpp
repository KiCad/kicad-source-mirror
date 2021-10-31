///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"
#include "widgets/wx_grid.h"

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

	m_locked = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	m_locked->SetToolTip( _("Do not allow position of pad relative to parent footprint to be changed") );

	gbSizerCommon->Add( m_locked, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbSizerCommon->AddGrowableCol( 1 );
	gbSizerCommon->AddGrowableCol( 4 );

	m_LeftBoxSizer->Add( gbSizerCommon, 0, wxEXPAND|wxBOTTOM, 5 );


	m_LeftBoxSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticline5 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline5, 0, wxEXPAND, 5 );


	m_LeftBoxSizer->Add( 0, 0, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizerShape;
	bSizerShape = new wxBoxSizer( wxHORIZONTAL );

	m_shapeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeLabel->Wrap( -1 );
	bSizerShape->Add( m_shapeLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_PadShapeSelectorChoices[] = { _("Circular"), _("Oval"), _("Rectangular"), _("Trapezoidal"), _("Rounded rectangle"), _("Chamfered rectangle"), _("Chamfered with other corners rounded"), _("Custom (circular base)"), _("Custom (rectangular base)") };
	int m_PadShapeSelectorNChoices = sizeof( m_PadShapeSelectorChoices ) / sizeof( wxString );
	m_PadShapeSelector = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadShapeSelectorNChoices, m_PadShapeSelectorChoices, 0 );
	m_PadShapeSelector->SetSelection( 2 );
	bSizerShape->Add( m_PadShapeSelector, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_LeftBoxSizer->Add( bSizerShape, 0, wxEXPAND|wxTOP, 5 );

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
	fgSizerChamferProps = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgSizerChamferProps->AddGrowableCol( 1 );
	fgSizerChamferProps->SetFlexibleDirection( wxBOTH );
	fgSizerChamferProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_chamferRatioLabel = new wxStaticText( m_chamferProps, wxID_ANY, _("Chamfer size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chamferRatioLabel->Wrap( -1 );
	m_chamferRatioLabel->SetToolTip( _("Chamfer size in percent of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgSizerChamferProps->Add( m_chamferRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_chamferRatioCtrl = new TEXT_CTRL_EVAL( m_chamferProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerChamferProps->Add( m_chamferRatioCtrl, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_chamferRatioUnits = new wxStaticText( m_chamferProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_chamferRatioUnits->Wrap( -1 );
	fgSizerChamferProps->Add( m_chamferRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticTextChamferCorner = new wxStaticText( m_chamferProps, wxID_ANY, _("Chamfer corners:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextChamferCorner->Wrap( -1 );
	m_staticTextChamferCorner->SetToolTip( _("Chamfered corners. The position is relative to a pad orientation 0 degree.") );

	fgSizerChamferProps->Add( m_staticTextChamferCorner, 0, 0, 5 );

	wxGridSizer* gCornersSizer;
	gCornersSizer = new wxGridSizer( 0, 2, 2, 6 );

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
	fgMixedProps = new wxFlexGridSizer( 0, 3, 4, 0 );
	fgMixedProps->AddGrowableCol( 1 );
	fgMixedProps->SetFlexibleDirection( wxBOTH );
	fgMixedProps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_mixedChamferRatioLabel = new wxStaticText( m_mixedProps, wxID_ANY, _("Chamfer size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedChamferRatioLabel->Wrap( -1 );
	m_mixedChamferRatioLabel->SetToolTip( _("Chamfer size in percent of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgMixedProps->Add( m_mixedChamferRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_mixedChamferRatioCtrl = new TEXT_CTRL_EVAL( m_mixedProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMixedProps->Add( m_mixedChamferRatioCtrl, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_mixedChamferRatioUnits = new wxStaticText( m_mixedProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedChamferRatioUnits->Wrap( -1 );
	fgMixedProps->Add( m_mixedChamferRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

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


	fgMixedProps->Add( 0, 0, 1, wxEXPAND, 5 );

	m_mixedCornerRatioLabel = new wxStaticText( m_mixedProps, wxID_ANY, _("Corner size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedCornerRatioLabel->Wrap( -1 );
	m_mixedCornerRatioLabel->SetToolTip( _("Corner radius in percent  of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );

	fgMixedProps->Add( m_mixedCornerRatioLabel, 0, wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_mixedCornerRatioCtrl = new TEXT_CTRL_EVAL( m_mixedProps, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgMixedProps->Add( m_mixedCornerRatioCtrl, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_mixedCornerRatioUnits = new wxStaticText( m_mixedProps, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mixedCornerRatioUnits->Wrap( -1 );
	fgMixedProps->Add( m_mixedCornerRatioUnits, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	m_mixedProps->SetSizer( fgMixedProps );
	m_mixedProps->Layout();
	fgMixedProps->Fit( m_mixedProps );
	m_shapePropsBook->AddPage( m_mixedProps, _("a page"), false );

	m_LeftBoxSizer->Add( m_shapePropsBook, 0, wxEXPAND|wxLEFT, 20 );

	wxGridBagSizer* gbSizerPadOrientation;
	gbSizerPadOrientation = new wxGridBagSizer( 3, 0 );
	gbSizerPadOrientation->SetFlexibleDirection( wxBOTH );
	gbSizerPadOrientation->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sizeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXLabel->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeXLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_sizeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	gbSizerPadOrientation->Add( m_sizeXCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeXUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeXUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_sizeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYLabel->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeYLabel, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_sizeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerPadOrientation->Add( m_sizeYCtrl, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_sizeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeYUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_sizeYUnits, wxGBPosition( 0, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_PadOrientText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Angle:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_PadOrientText->Wrap( -1 );
	gbSizerPadOrientation->Add( m_PadOrientText, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_cb_padrotation = new wxComboBox( m_panelGeneral, wxID_ANY, _("0"), wxDefaultPosition, wxSize( 100,-1 ), 0, NULL, 0 );
	m_cb_padrotation->Append( _("0") );
	m_cb_padrotation->Append( _("90") );
	m_cb_padrotation->Append( _("-90") );
	m_cb_padrotation->Append( _("180") );
	m_cb_padrotation->SetSelection( 0 );
	gbSizerPadOrientation->Add( m_cb_padrotation, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_orientationUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationUnits->Wrap( -1 );
	gbSizerPadOrientation->Add( m_orientationUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	m_LeftBoxSizer->Add( gbSizerPadOrientation, 0, wxEXPAND|wxTOP, 5 );


	m_LeftBoxSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticline6 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline6, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxGridBagSizer* gbSizerHole;
	gbSizerHole = new wxGridBagSizer( 4, 0 );
	gbSizerHole->SetFlexibleDirection( wxBOTH );
	gbSizerHole->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_holeShapeLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeShapeLabel->Wrap( -1 );
	gbSizerHole->Add( m_holeShapeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_holeShapeCtrlChoices[] = { _("Circular"), _("Oval") };
	int m_holeShapeCtrlNChoices = sizeof( m_holeShapeCtrlChoices ) / sizeof( wxString );
	m_holeShapeCtrl = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_holeShapeCtrlNChoices, m_holeShapeCtrlChoices, 0 );
	m_holeShapeCtrl->SetSelection( 1 );
	gbSizerHole->Add( m_holeShapeCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 5 ), wxEXPAND|wxRIGHT, 5 );

	m_holeXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Hole size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXLabel->Wrap( -1 );
	gbSizerHole->Add( m_holeXLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_holeXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerHole->Add( m_holeXCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_holeXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeXUnits->Wrap( -1 );
	gbSizerHole->Add( m_holeXUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_holeYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYLabel->Wrap( -1 );
	gbSizerHole->Add( m_holeYLabel, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_holeYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizerHole->Add( m_holeYCtrl, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_holeYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_holeYUnits->Wrap( -1 );
	gbSizerHole->Add( m_holeYUnits, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	m_LeftBoxSizer->Add( gbSizerHole, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_LeftBoxSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticline7 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_LeftBoxSizer->Add( m_staticline7, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer35;
	bSizer35 = new wxBoxSizer( wxHORIZONTAL );

	m_offsetShapeOpt = new wxCheckBox( m_panelGeneral, wxID_ANY, _("Offset shape from hole"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer35->Add( m_offsetShapeOpt, 0, wxLEFT, 5 );

	m_offsetShapeOptLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetShapeOptLabel->Wrap( -1 );
	bSizer35->Add( m_offsetShapeOptLabel, 0, wxRIGHT, 5 );


	m_LeftBoxSizer->Add( bSizer35, 0, wxEXPAND|wxTOP, 5 );

	m_offsetCtrls = new wxFlexGridSizer( 0, 6, 0, 0 );
	m_offsetCtrls->AddGrowableCol( 1 );
	m_offsetCtrls->AddGrowableCol( 4 );
	m_offsetCtrls->SetFlexibleDirection( wxBOTH );
	m_offsetCtrls->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_offsetXLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXLabel->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetXLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );

	m_offsetXCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetCtrls->Add( m_offsetXCtrl, 0, wxBOTTOM|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_offsetXUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXUnits->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetXUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );

	m_offsetYLabel = new wxStaticText( m_panelGeneral, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYLabel->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetYLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_offsetYCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetCtrls->Add( m_offsetYCtrl, 0, wxBOTTOM|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_offsetYUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYUnits->Wrap( -1 );
	m_offsetCtrls->Add( m_offsetYUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );


	m_LeftBoxSizer->Add( m_offsetCtrls, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 25 );

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
	bSizer34->Add( m_padToDieLabel, 0, wxALIGN_CENTER_VERTICAL|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT|wxTOP, 5 );


	fgSizerPadToDie->Add( bSizer34, 0, wxEXPAND|wxRIGHT, 5 );

	m_padToDieCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPadToDie->Add( m_padToDieCtrl, 0, wxEXPAND|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_padToDieUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_padToDieUnits->Wrap( -1 );
	fgSizerPadToDie->Add( m_padToDieUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRESERVE_SPACE_EVEN_IF_HIDDEN|wxRIGHT, 5 );


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


	m_LayersSizer->Add( m_FlippedWarningSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	m_copperLayersLabel = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_copperLayersLabel->Wrap( -1 );
	m_LayersSizer->Add( m_copperLayersLabel, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	wxArrayString m_rbCopperLayersSelChoices;
	m_rbCopperLayersSel = new wxChoice( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rbCopperLayersSelChoices, 0 );
	m_rbCopperLayersSel->SetSelection( 0 );
	m_LayersSizer->Add( m_rbCopperLayersSel, 1, wxALL|wxEXPAND|wxTOP, 4 );

	m_techLayersLabel = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Technical layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	m_LayersSizer->Add( m_techLayersLabel, 0, wxALL, 5 );

	m_PadLayerAdhCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerAdhCmp, 0, wxLEFT|wxRIGHT, 4 );

	m_PadLayerAdhCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerAdhCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerPateCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerPateCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerPateCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerPateCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerSilkCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerSilkCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerSilkCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerSilkCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerMaskCmp = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Front solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerMaskCmp, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerMaskCu = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Back solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerMaskCu, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerDraft = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Drafting notes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerDraft, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerECO1 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerECO1, 0, wxTOP|wxRIGHT|wxLEFT, 4 );

	m_PadLayerECO2 = new wxCheckBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("E.C.O.2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayersSizer->Add( m_PadLayerECO2, 0, wxALL, 4 );


	m_middleBoxSizer->Add( m_LayersSizer, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );

	m_staticTextFabProperty = new wxStaticText( m_panelGeneral, wxID_ANY, _("Fabrication Property:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFabProperty->Wrap( -1 );
	m_staticTextFabProperty->SetToolTip( _("Optional property to specify a special purpose or constraint in fabrication files:\nBGA attribute is for pads in BGA footprints\nFiducial local is a fiducial for the parent footprint\nFiducial global is a fiducial for the whole board\nTest Point pad  is useful to specify test points in Gerber files\nHeatsink pad  specify a thermal pad\nCastellated specify castellated through hole pads on a board edge\nThese properties are specified in Gerber X2  files.") );

	m_middleBoxSizer->Add( m_staticTextFabProperty, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_middleBoxSizer->Add( 0, 2, 0, wxEXPAND, 5 );

	wxString m_choiceFabPropertyChoices[] = { _("None"), _("BGA pad"), _("Fiducial, local to footprint"), _("Fiducial, global to board"), _("Test point pad"), _("Heatsink pad"), _("Castellated pad (through hole only)") };
	int m_choiceFabPropertyNChoices = sizeof( m_choiceFabPropertyChoices ) / sizeof( wxString );
	m_choiceFabProperty = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceFabPropertyNChoices, m_choiceFabPropertyChoices, 0 );
	m_choiceFabProperty->SetSelection( 0 );
	m_middleBoxSizer->Add( m_choiceFabProperty, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bGeneralSizer->Add( m_middleBoxSizer, 0, wxEXPAND|wxALL, 3 );


	m_panelGeneral->SetSizer( bGeneralSizer );
	m_panelGeneral->Layout();
	bGeneralSizer->Fit( m_panelGeneral );
	m_notebook->AddPage( m_panelGeneral, _("General"), true );
	m_localSettingsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerPanelClearance;
	bSizerPanelClearance = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerClearance;
	bSizerClearance = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbClearancesSizer;
	sbClearancesSizer = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Clearances") ), wxVERTICAL );

	wxStaticText* m_staticTextHint;
	m_staticTextHint = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Set values to 0 to use parent footprint or netclass values."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHint->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextHint, 0, wxLEFT|wxRIGHT, 5 );

	m_staticTextInfoPosValue = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Positive clearance means area bigger than the pad (usual for mask clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoPosValue->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextInfoPosValue, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoNegVal = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Negative clearance means area smaller than the pad (usual for paste clearance)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfoNegVal->Wrap( -1 );
	sbClearancesSizer->Add( m_staticTextInfoNegVal, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer* fgClearancesGridSizer;
	fgClearancesGridSizer = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgClearancesGridSizer->AddGrowableCol( 1 );
	fgClearancesGridSizer->SetFlexibleDirection( wxBOTH );
	fgClearancesGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_clearanceLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceLabel->Wrap( -1 );
	m_clearanceLabel->SetToolTip( _("This is the local net clearance for this pad.\nIf 0, the footprint local value or the Netclass value is used.") );

	fgClearancesGridSizer->Add( m_clearanceLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_clearanceCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_clearanceCtrl, 0, wxEXPAND|wxTOP|wxLEFT, 5 );

	m_clearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_maskMarginLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginLabel->Wrap( -1 );
	m_maskMarginLabel->SetToolTip( _("This is the local clearance between this pad and the solder mask.\nIf 0, the footprint local value or the global value is used.") );

	fgClearancesGridSizer->Add( m_maskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_maskMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_maskMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_maskMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maskMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_maskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_pasteMarginLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste absolute clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginLabel->Wrap( -1 );
	m_pasteMarginLabel->SetToolTip( _("This is the local clearance between this pad and the solder paste.\nIf 0, the footprint value or the global value is used.\nThe final clearance value is the sum of this value and the clearance value ratio.\nA negative value means a smaller mask size than pad size.") );

	fgClearancesGridSizer->Add( m_pasteMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_pasteMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_pasteMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

	m_pasteMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_pasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_pasteMarginRatioLabel = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste relative clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioLabel->Wrap( -1 );
	m_pasteMarginRatioLabel->SetToolTip( _("This is the local clearance ratio in percent between this pad and the solder paste.\nA value of 10 means the clearance value is 10 percent of the pad size.\nIf 0, the footprint value or the global value is used.\nThe final clearance value is the sum of this value and the clearance value.\nA negative value means a smaller mask size than pad size.") );

	fgClearancesGridSizer->Add( m_pasteMarginRatioLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );

	m_pasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_pasteMarginRatioCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );

	m_pasteMarginRatioUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pasteMarginRatioUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_pasteMarginRatioUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbClearancesSizer->Add( fgClearancesGridSizer, 0, wxEXPAND, 5 );

	m_nonCopperWarningBook = new wxSimplebook( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* notePanel;
	notePanel = new wxPanel( m_nonCopperWarningBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bNoteSizer;
	bNoteSizer = new wxBoxSizer( wxVERTICAL );

	m_nonCopperNote = new wxStaticText( notePanel, wxID_ANY, _("Note: solder mask and paste values are used only for pads on copper layers."), wxDefaultPosition, wxDefaultSize, 0 );
	m_nonCopperNote->Wrap( -1 );
	bNoteSizer->Add( m_nonCopperNote, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextInfoPaste = new wxStaticText( notePanel, wxID_ANY, _("Note: solder paste clearances (absolute and relative) are added to determine the final clearance."), wxDefaultPosition, wxDefaultSize, 0 );
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

	m_sbSizerZonesSettings = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Connection to Copper Zones") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerCopperZonesOpts;
	fgSizerCopperZonesOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerCopperZonesOpts->AddGrowableCol( 1 );
	fgSizerCopperZonesOpts->SetFlexibleDirection( wxBOTH );
	fgSizerCopperZonesOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText40 = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticText40, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_ZoneConnectionChoiceChoices[] = { _("From parent footprint"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneConnectionChoice, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerCopperZonesOpts->Add( 0, 0, 1, wxEXPAND, 5 );

	m_spokeWidthLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief spoke width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_spokeWidthCtrl = new wxTextCtrl( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthCtrl, 0, wxEXPAND|wxLEFT|wxTOP|wxALIGN_CENTER_VERTICAL, 5 );

	m_spokeWidthUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spokeWidthUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_spokeWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_thermalGapLabel = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapLabel->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_thermalGapLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );

	m_thermalGapCtrl = new wxTextCtrl( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_thermalGapCtrl, 0, wxEXPAND|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_thermalGapUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thermalGapUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_thermalGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticTextcps = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Custom pad shape in zone:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextcps->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticTextcps, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxString m_ZoneCustomPadShapeChoices[] = { _("Use pad shape"), _("Use pad convex hull") };
	int m_ZoneCustomPadShapeNChoices = sizeof( m_ZoneCustomPadShapeChoices ) / sizeof( wxString );
	m_ZoneCustomPadShape = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneCustomPadShapeNChoices, m_ZoneCustomPadShapeChoices, 0 );
	m_ZoneCustomPadShape->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneCustomPadShape, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	m_sbSizerZonesSettings->Add( fgSizerCopperZonesOpts, 0, wxEXPAND, 5 );


	bSizerClearance->Add( m_sbSizerZonesSettings, 0, wxALL|wxEXPAND, 5 );


	bSizerPanelClearance->Add( bSizerClearance, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_localSettingsPanel->SetSizer( bSizerPanelClearance );
	m_localSettingsPanel->Layout();
	bSizerPanelClearance->Fit( m_localSettingsPanel );
	m_notebook->AddPage( m_localSettingsPanel, _("Clearance Overrides and Settings"), false );
	m_panelCustomShapePrimitives = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_bSizerPanelPrimitives = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerAboveList;
	bSizerAboveList = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPrimitivesList = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Primitives list:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitivesList->Wrap( -1 );
	bSizerAboveList->Add( m_staticTextPrimitivesList, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticTextPrimitiveListWarning = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Note: coordinates are relative to anchor pad, orientation 0."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitiveListWarning->Wrap( -1 );
	bSizerAboveList->Add( m_staticTextPrimitiveListWarning, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 7 );


	m_bSizerPanelPrimitives->Add( bSizerAboveList, 0, wxEXPAND|wxBOTTOM, 3 );

	m_listCtrlPrimitives = new wxListView( m_panelCustomShapePrimitives, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_NO_HEADER|wxLC_REPORT );
	m_bSizerPanelPrimitives->Add( m_listCtrlPrimitives, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerButtonsUpper;
	bSizerButtonsUpper = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Add Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonAddShape, 0, wxALL, 5 );

	m_buttonEditShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Edit Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonEditShape, 0, wxALL, 5 );

	m_buttonDup = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Duplicate Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDup, 0, wxALL, 5 );

	m_buttonGeometry = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Transform Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonGeometry, 0, wxALL, 5 );


	bSizerButtonsUpper->Add( 0, 0, 1, wxEXPAND, 5 );

	m_buttonDel = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Delete Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDel, 0, wxALL, 5 );


	bSizerButtons->Add( bSizerButtonsUpper, 0, wxEXPAND, 5 );


	m_bSizerPanelPrimitives->Add( bSizerButtons, 0, wxEXPAND, 5 );


	m_panelCustomShapePrimitives->SetSizer( m_bSizerPanelPrimitives );
	m_panelCustomShapePrimitives->Layout();
	m_bSizerPanelPrimitives->Fit( m_panelCustomShapePrimitives );
	m_notebook->AddPage( m_panelCustomShapePrimitives, _("Custom Shape Primitives"), false );

	bSizerUpper->Add( m_notebook, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );

	bSizerDisplayPad->SetMinSize( wxSize( 290,-1 ) );

	bSizerDisplayPad->Add( 0, 0, 0, wxBOTTOM|wxEXPAND|wxTOP, 3 );


	bSizerDisplayPad->Add( 0, 10, 0, wxEXPAND, 5 );

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

	bSizerDisplayPad->Add( m_stackupImagesBook, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 6 );

	m_boardViewPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_padPreviewSizer = new wxBoxSizer( wxVERTICAL );

	m_padPreviewSizer->SetMinSize( wxSize( 280,-1 ) );

	m_boardViewPanel->SetSizer( m_padPreviewSizer );
	m_boardViewPanel->Layout();
	m_padPreviewSizer->Fit( m_boardViewPanel );
	bSizerDisplayPad->Add( m_boardViewPanel, 1, wxEXPAND|wxALL, 2 );


	bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );


	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );

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

	bottomSizer->Add( m_sdbSizer, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


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
	m_holeShapeCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetShapeOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnOffsetCheckbox ), NULL, this );
	m_offsetXCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_padToDieOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieCheckbox ), NULL, this );
	m_rbCopperLayersSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetCopperLayers ), NULL, this );
	m_PadLayerAdhCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCmp->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCu->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerDraft->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO1->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO2->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_clearanceCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonAddShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonEditShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonDup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_buttonDel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
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
	m_holeShapeCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_holeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_holeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetShapeOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnOffsetCheckbox ), NULL, this );
	m_offsetXCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_offsetYCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_padToDieOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadToDieCheckbox ), NULL, this );
	m_rbCopperLayersSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetCopperLayers ), NULL, this );
	m_PadLayerAdhCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerAdhCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerPateCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerSilkCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCmp->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerMaskCu->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerDraft->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO1->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_PadLayerECO2->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_clearanceCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_nonCopperWarningBook->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnUpdateUINonCopperWarning ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonAddShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonEditShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonDup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_buttonDel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
	m_cbShowPadOutline->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onChangePadMode ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );

}

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerShapeProperties;
	fgSizerShapeProperties = new wxFlexGridSizer( 0, 7, 5, 0 );
	fgSizerShapeProperties->AddGrowableCol( 2 );
	fgSizerShapeProperties->AddGrowableCol( 4 );
	fgSizerShapeProperties->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextPosStart = new wxStaticText( this, wxID_ANY, _("Start point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosStart->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosStart, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_startXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startXLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_startXCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_startXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_startXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_startYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startYLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_startYCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_startYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_startYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_startYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextPosCtrl1 = new wxStaticText( this, wxID_ANY, _("Control point 1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl1->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ctrl1XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl1XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1XCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl1XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ctrl1YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl1YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl1YCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl1YUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl1YUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl1YUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextPosCtrl2 = new wxStaticText( this, wxID_ANY, _("Control point 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosCtrl2->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosCtrl2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ctrl2XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl2XCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2XCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl2XUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2XUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2XUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ctrl2YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2YLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2YLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_ctrl2YCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_ctrl2YCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ctrl2YUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ctrl2YUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_ctrl2YUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticTextPosEnd = new wxStaticText( this, wxID_ANY, _("End point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosEnd->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosEnd, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_endXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endXLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_endXCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_endXCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_endXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_endYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endYLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_endYCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_endYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_endYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_endYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_radiusLabel = new wxStaticText( this, wxID_ANY, _("Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_radiusLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_radiusCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_radiusCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_radiusUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radiusUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_radiusUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerShapeProperties->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_filledCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizermain->Add( fgSizerShapeProperties, 1, wxEXPAND|wxALL, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizermain->Add( m_sdbSizer, 0, wxALL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( bSizermain );
	this->Layout();
	bSizermain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::~DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE()
{
}

DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE::DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerShapeProperties1;
	fgSizerShapeProperties1 = new wxFlexGridSizer( 0, 7, 3, 0 );
	fgSizerShapeProperties1->AddGrowableCol( 2 );
	fgSizerShapeProperties1->AddGrowableCol( 4 );
	fgSizerShapeProperties1->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextMove = new wxStaticText( this, wxID_ANY, _("Move vector"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMove->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_staticTextMove, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_xCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_xCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_xUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_xUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_yLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_yCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_yCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_yUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_yUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_rotationLabel = new wxStaticText( this, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_rotationLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_rotationCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_rotationCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_rotationUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotationUnits->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_rotationUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_scaleLabel = new wxStaticText( this, wxID_ANY, _("Scaling factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_scaleLabel->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_scaleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_scaleCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties1->Add( m_scaleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextDupCnt = new wxStaticText( this, wxID_ANY, _("Duplicate:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDupCnt->Wrap( -1 );
	fgSizerShapeProperties1->Add( m_staticTextDupCnt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_spinCtrlDuplicateCount = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1 );
	fgSizerShapeProperties1->Add( m_spinCtrlDuplicateCount, 0, wxALL|wxEXPAND, 5 );


	fgSizerShapeProperties1->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizermain->Add( fgSizerShapeProperties1, 1, wxALL|wxEXPAND, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizermain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizermain );
	this->Layout();
	bSizermain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE::~DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE()
{
}

DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	m_gridCornersList = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE );

	// Grid
	m_gridCornersList->CreateGrid( 1, 2 );
	m_gridCornersList->EnableEditing( true );
	m_gridCornersList->EnableGridLines( true );
	m_gridCornersList->EnableDragGridSize( false );
	m_gridCornersList->SetMargins( 0, 0 );

	// Columns
	m_gridCornersList->SetColSize( 0, 124 );
	m_gridCornersList->SetColSize( 1, 124 );
	m_gridCornersList->EnableDragColMove( false );
	m_gridCornersList->EnableDragColSize( true );
	m_gridCornersList->SetColLabelSize( 22 );
	m_gridCornersList->SetColLabelValue( 0, _("Pos X") );
	m_gridCornersList->SetColLabelValue( 1, _("Pos Y") );
	m_gridCornersList->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridCornersList->AutoSizeRows();
	m_gridCornersList->EnableDragRowSize( false );
	m_gridCornersList->SetRowLabelSize( 80 );
	m_gridCornersList->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridCornersList->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bLeftSizer->Add( m_gridCornersList, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerRightButts;
	bSizerRightButts = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addButton->SetMinSize( wxSize( 30,30 ) );

	bSizerRightButts->Add( m_addButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerRightButts->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_deleteButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteButton->SetMinSize( wxSize( 30,30 ) );

	bSizerRightButts->Add( m_deleteButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bLeftSizer->Add( bSizerRightButts, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerThickness;
	fgSizerThickness = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerThickness->AddGrowableCol( 1 );
	fgSizerThickness->SetFlexibleDirection( wxBOTH );
	fgSizerThickness->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_thicknessCtrl = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizerThickness->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_filledCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );


	bLeftSizer->Add( fgSizerThickness, 0, wxALL|wxEXPAND, 10 );


	bSizerUpper->Add( bLeftSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	m_panelPoly = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelPoly->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	m_panelPoly->SetMinSize( wxSize( 290,290 ) );

	bRightSizer->Add( m_panelPoly, 1, wxEXPAND|wxTOP|wxRIGHT, 10 );

	wxBoxSizer* m_warningSizer;
	m_warningSizer = new wxBoxSizer( wxHORIZONTAL );

	m_warningIcon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_warningIcon->SetMinSize( wxSize( 50,50 ) );

	m_warningSizer->Add( m_warningIcon, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_warningText = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_warningText->Wrap( -1 );
	m_warningSizer->Add( m_warningText, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );


	m_warningSizer->Add( 5, 88, 0, 0, 5 );


	bRightSizer->Add( m_warningSizer, 0, wxEXPAND|wxRIGHT, 10 );


	bSizerUpper->Add( bRightSizer, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxHORIZONTAL );

	m_statusLine1 = new wxStaticText( this, wxID_ANY, _("Coordinates are relative to anchor pad, rotated 0.0 deg."), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine1->Wrap( -1 );
	bSizer24->Add( m_statusLine1, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer24->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	bSizerMain->Add( bSizer24, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_gridCornersList->Connect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonAdd ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );
}

DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE()
{
	// Disconnect Events
	m_gridCornersList->Disconnect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonAdd ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );

}
