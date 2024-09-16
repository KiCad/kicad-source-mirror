///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SHAPE_PROPERTIES_BASE::DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_upperSizer = new wxBoxSizer( wxVERTICAL );

	m_sizerStartEnd = new wxGridBagSizer( 3, 3 );
	m_sizerStartEnd->SetFlexibleDirection( wxBOTH );
	m_sizerStartEnd->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_sizerStartEnd->SetEmptyCellSize( wxSize( 5,5 ) );

	m_startPointLabel = new wxStaticText( this, wxID_ANY, _("Start Point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startPointLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_startPointLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_startXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_startXLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_startXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerStartEnd->Add( m_startXCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_startXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXUnits->Wrap( -1 );
	m_sizerStartEnd->Add( m_startXUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 20 );

	m_startYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_startYLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_startYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerStartEnd->Add( m_startYCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_startYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYUnits->Wrap( -1 );
	m_sizerStartEnd->Add( m_startYUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 20 );

	m_endPointLabel = new wxStaticText( this, wxID_ANY, _("End Point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endPointLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_endPointLabel, wxGBPosition( 0, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_endXLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_endXLabel, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_endXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerStartEnd->Add( m_endXCtrl, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_endXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXUnits->Wrap( -1 );
	m_sizerStartEnd->Add( m_endXUnits, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_endYLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYLabel->Wrap( -1 );
	m_sizerStartEnd->Add( m_endYLabel, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_endYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerStartEnd->Add( m_endYCtrl, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_endYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYUnits->Wrap( -1 );
	m_sizerStartEnd->Add( m_endYUnits, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	m_upperSizer->Add( m_sizerStartEnd, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_sizerBezier = new wxGridBagSizer( 3, 3 );
	m_sizerBezier->SetFlexibleDirection( wxBOTH );
	m_sizerBezier->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_sizerBezier->SetEmptyCellSize( wxSize( 5,5 ) );

	m_bezierCtrlPt1Label = new wxStaticText( this, wxID_ANY, _("Bezier Control Point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bezierCtrlPt1Label->Wrap( -1 );
	m_sizerBezier->Add( m_bezierCtrlPt1Label, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_BezierPointC1XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1XLabel->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC1XLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_BezierC1X_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerBezier->Add( m_BezierC1X_Ctrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierPointC1XUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1XUnit->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC1XUnit, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 20 );

	m_BezierPointC1YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1YLabel->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC1YLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierC1Y_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerBezier->Add( m_BezierC1Y_Ctrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierPointC1YUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1YUnit->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC1YUnit, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxRIGHT|wxALIGN_CENTER_VERTICAL, 20 );

	m_bezierCtrlPt2Label = new wxStaticText( this, wxID_ANY, _("Bezier Control Point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bezierCtrlPt2Label->Wrap( -1 );
	m_sizerBezier->Add( m_bezierCtrlPt2Label, wxGBPosition( 0, 3 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );

	m_BezierPointC2XLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2XLabel->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC2XLabel, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierC2X_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerBezier->Add( m_BezierC2X_Ctrl, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierPointC2XUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2XUnit->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC2XUnit, wxGBPosition( 1, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierPointC2YLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2YLabel->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC2YLabel, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierC2Y_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerBezier->Add( m_BezierC2Y_Ctrl, wxGBPosition( 2, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_BezierPointC2YUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2YUnit->Wrap( -1 );
	m_sizerBezier->Add( m_BezierPointC2YUnit, wxGBPosition( 2, 5 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	m_upperSizer->Add( m_sizerBezier, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_segmentLengthLabel = new wxStaticText( this, wxID_ANY, _("Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_segmentLengthLabel->Wrap( -1 );
	fgSizer1->Add( m_segmentLengthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_segmentLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_segmentLengthCtrl->SetMinSize( wxSize( 140,-1 ) );

	fgSizer1->Add( m_segmentLengthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_segmentLengthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_segmentLengthUnits->Wrap( -1 );
	fgSizer1->Add( m_segmentLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_segmentAngleLabel = new wxStaticText( this, wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_segmentAngleLabel->Wrap( -1 );
	fgSizer1->Add( m_segmentAngleLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_segmentAngleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_segmentAngleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_segmentAngleUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_segmentAngleUnits->Wrap( -1 );
	fgSizer1->Add( m_segmentAngleUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_rectangleHeightLabel = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleHeightLabel->Wrap( -1 );
	fgSizer1->Add( m_rectangleHeightLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_rectangleHeightCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_rectangleHeightCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_rectangleHeightUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleHeightUnits->Wrap( -1 );
	fgSizer1->Add( m_rectangleHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_rectangleWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_rectangleWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_rectangleWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_rectangleWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_rectangleWidthUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_rectangleWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_angleLabel = new wxStaticText( this, wxID_ANY, _("Arc angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_angleLabel->Wrap( -1 );
	fgSizer1->Add( m_angleLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_angleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_angleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_angleUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_angleUnits->Wrap( -1 );
	fgSizer1->Add( m_angleUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bMiddleSizer->Add( fgSizer1, 0, wxTOP, 5 );


	m_upperSizer->Add( bMiddleSizer, 0, wxBOTTOM|wxEXPAND, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 5, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,4 ) );

	m_locked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_locked, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 5 );

	m_filledCtrl = new wxCheckBox( this, wxID_ANY, _("Filled shape"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_filledCtrl, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxLEFT, 5 );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	gbSizer2->Add( m_thicknessLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_thicknessCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	gbSizer2->Add( m_thicknessUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_lineStyleLabel = new wxStaticText( this, wxID_ANY, _("Line style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineStyleLabel->Wrap( -1 );
	gbSizer2->Add( m_lineStyleLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lineStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_lineStyleCombo->SetMinSize( wxSize( 210,-1 ) );

	gbSizer2->Add( m_lineStyleCombo, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	gbSizer2->Add( m_LayerLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer2->Add( m_LayerSelectionCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_techLayersLabel = new wxStaticText( this, wxID_ANY, _("Technical Layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	gbSizer2->Add( m_techLayersLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hasSolderMask = new wxCheckBox( this, wxID_ANY, _("Solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_hasSolderMask, 0, wxALL, 5 );

	m_solderMaskMarginLabel = new wxStaticText( this, wxID_ANY, _("Expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginLabel->Wrap( -1 );
	fgSizer2->Add( m_solderMaskMarginLabel, 0, wxALL, 5 );

	m_solderMaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginCtrl->SetToolTip( _("This is the local clearance between the shape and the solder mask opening.\nLeave blank to use the value defined in the Board Setup.") );

	fgSizer2->Add( m_solderMaskMarginCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_solderMaskMarginUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginUnit->Wrap( -1 );
	fgSizer2->Add( m_solderMaskMarginUnit, 0, wxALL, 5 );


	gbSizer2->Add( fgSizer2, wxGBPosition( 8, 0 ), wxGBSpan( 1, 3 ), wxEXPAND, 5 );

	m_netLabel = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netLabel->Wrap( -1 );
	gbSizer2->Add( m_netLabel, wxGBPosition( 9, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_netSelector = new NET_SELECTOR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_netSelector, wxGBPosition( 9, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	gbSizer2->AddGrowableCol( 1 );

	m_upperSizer->Add( gbSizer2, 0, wxEXPAND, 5 );


	bMainSizer->Add( m_upperSizer, 1, wxALL|wxEXPAND, 5 );

	m_StandardButtonsSizer = new wxStdDialogButtonSizer();
	m_StandardButtonsSizerOK = new wxButton( this, wxID_OK );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerOK );
	m_StandardButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerCancel );
	m_StandardButtonsSizer->Realize();

	bMainSizer->Add( m_StandardButtonsSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_filledCtrl->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onFilledCheckbox ), NULL, this );
	m_LayerSelectionCtrl->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onLayerSelection ), NULL, this );
	m_hasSolderMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onTechLayersChanged ), NULL, this );
}

DIALOG_SHAPE_PROPERTIES_BASE::~DIALOG_SHAPE_PROPERTIES_BASE()
{
	// Disconnect Events
	m_filledCtrl->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onFilledCheckbox ), NULL, this );
	m_LayerSelectionCtrl->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onLayerSelection ), NULL, this );
	m_hasSolderMask->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onTechLayersChanged ), NULL, this );

}
