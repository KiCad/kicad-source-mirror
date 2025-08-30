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

	m_notebookShapeDefs = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleByCorners = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCorners = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCorners->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCorners->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer18->Add( m_gbsRectangleByCorners, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCorners->SetSizer( bSizer18 );
	m_rectangleByCorners->Layout();
	bSizer18->Fit( m_rectangleByCorners );
	m_notebookShapeDefs->AddPage( m_rectangleByCorners, _("By Corners"), false );
	m_rectangleByCornerSize = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCornerSize = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCornerSize->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCornerSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer19->Add( m_gbsRectangleByCornerSize, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCornerSize->SetSizer( bSizer19 );
	m_rectangleByCornerSize->Layout();
	bSizer19->Fit( m_rectangleByCornerSize );
	m_notebookShapeDefs->AddPage( m_rectangleByCornerSize, _("By Corner and Size"), false );
	m_rectangleByCenterSize = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCenterSize = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCenterSize->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCenterSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer20->Add( m_gbsRectangleByCenterSize, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCenterSize->SetSizer( bSizer20 );
	m_rectangleByCenterSize->Layout();
	bSizer20->Fit( m_rectangleByCenterSize );
	m_notebookShapeDefs->AddPage( m_rectangleByCenterSize, _("By Center and Size"), true );
	m_lineByEnds = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_gbsLineByEnds = new wxGridBagSizer( 4, 5 );
	m_gbsLineByEnds->SetFlexibleDirection( wxBOTH );
	m_gbsLineByEnds->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer6->Add( m_gbsLineByEnds, 1, wxALL|wxEXPAND, 5 );


	m_lineByEnds->SetSizer( bSizer6 );
	m_lineByEnds->Layout();
	bSizer6->Fit( m_lineByEnds );
	m_notebookShapeDefs->AddPage( m_lineByEnds, _("By Endpoints"), false );
	m_lineByLengthAngle = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	m_gbsLineByLengthAngle = new wxGridBagSizer( 4, 5 );
	m_gbsLineByLengthAngle->SetFlexibleDirection( wxBOTH );
	m_gbsLineByLengthAngle->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer7->Add( m_gbsLineByLengthAngle, 1, wxALL|wxEXPAND, 5 );


	m_lineByLengthAngle->SetSizer( bSizer7 );
	m_lineByLengthAngle->Layout();
	bSizer7->Fit( m_lineByLengthAngle );
	m_notebookShapeDefs->AddPage( m_lineByLengthAngle, _("By Length and Angle"), false );
	m_lineByStartMid = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxVERTICAL );

	m_gbsLineByStartMid = new wxGridBagSizer( 4, 5 );
	m_gbsLineByStartMid->SetFlexibleDirection( wxBOTH );
	m_gbsLineByStartMid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer71->Add( m_gbsLineByStartMid, 1, wxALL|wxEXPAND, 5 );


	m_lineByStartMid->SetSizer( bSizer71 );
	m_lineByStartMid->Layout();
	bSizer71->Fit( m_lineByStartMid );
	m_notebookShapeDefs->AddPage( m_lineByStartMid, _("By Start/Midpoint"), false );
	m_arcByCSA = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_gbsArcByCSA = new wxGridBagSizer( 4, 5 );
	m_gbsArcByCSA->SetFlexibleDirection( wxBOTH );
	m_gbsArcByCSA->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer8->Add( m_gbsArcByCSA, 1, wxALL|wxEXPAND, 5 );


	m_arcByCSA->SetSizer( bSizer8 );
	m_arcByCSA->Layout();
	bSizer8->Fit( m_arcByCSA );
	m_notebookShapeDefs->AddPage( m_arcByCSA, _("By Center/Start/Angle"), false );
	m_arcBySME = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_gbsArcBySME = new wxGridBagSizer( 4, 5 );
	m_gbsArcBySME->SetFlexibleDirection( wxBOTH );
	m_gbsArcBySME->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer9->Add( m_gbsArcBySME, 1, wxALL|wxEXPAND, 5 );


	m_arcBySME->SetSizer( bSizer9 );
	m_arcBySME->Layout();
	bSizer9->Fit( m_arcBySME );
	m_notebookShapeDefs->AddPage( m_arcBySME, _("By Start/Mid/End"), false );
	m_circle = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	m_gbsCircleCenterRadius = new wxGridBagSizer( 4, 5 );
	m_gbsCircleCenterRadius->SetFlexibleDirection( wxBOTH );
	m_gbsCircleCenterRadius->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer10->Add( m_gbsCircleCenterRadius, 1, wxALL|wxEXPAND, 5 );


	m_circle->SetSizer( bSizer10 );
	m_circle->Layout();
	bSizer10->Fit( m_circle );
	m_notebookShapeDefs->AddPage( m_circle, _("By Center/Radius"), false );
	m_circleCenterPoint = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_gbsCircleCenterPoint = new wxGridBagSizer( 4, 5 );
	m_gbsCircleCenterPoint->SetFlexibleDirection( wxBOTH );
	m_gbsCircleCenterPoint->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer11->Add( m_gbsCircleCenterPoint, 1, wxALL|wxEXPAND, 5 );


	m_circleCenterPoint->SetSizer( bSizer11 );
	m_circleCenterPoint->Layout();
	bSizer11->Fit( m_circleCenterPoint );
	m_notebookShapeDefs->AddPage( m_circleCenterPoint, _("By Center/Point"), false );
	m_bezier = new wxPanel( m_notebookShapeDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_gbsBezier = new wxGridBagSizer( 4, 5 );
	m_gbsBezier->SetFlexibleDirection( wxBOTH );
	m_gbsBezier->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer4->Add( m_gbsBezier, 1, wxALL|wxEXPAND, 5 );


	m_bezier->SetSizer( bSizer4 );
	m_bezier->Layout();
	bSizer4->Fit( m_bezier );
	m_notebookShapeDefs->AddPage( m_bezier, _("Bezier Control Points"), false );

	bMainSizer->Add( m_notebookShapeDefs, 0, wxEXPAND | wxALL, 5 );

	m_locked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_locked, 0, wxALL, 10 );

	wxBoxSizer* bSizerRoundRect;
	bSizerRoundRect = new wxBoxSizer( wxHORIZONTAL );

	m_cbRoundRect = new wxCheckBox( this, wxID_ANY, _("Rounded rectangle"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRoundRect->Add( m_cbRoundRect, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRoundRect->Add( 15, 0, 1, wxEXPAND, 5 );

	m_cornerRadiusLabel = new wxStaticText( this, wxID_ANY, _("Corner radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	bSizerRoundRect->Add( m_cornerRadiusLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_cornerRadiusCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRoundRect->Add( m_cornerRadiusCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_cornerRadiusUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	bSizerRoundRect->Add( m_cornerRadiusUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bMainSizer->Add( bSizerRoundRect, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_upperSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 2, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,6 ) );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	gbSizer2->Add( m_thicknessLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_thicknessCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	gbSizer2->Add( m_thicknessUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_lineStyleLabel = new wxStaticText( this, wxID_ANY, _("Line style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineStyleLabel->Wrap( -1 );
	gbSizer2->Add( m_lineStyleLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lineStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_lineStyleCombo->SetMinSize( wxSize( 210,-1 ) );

	gbSizer2->Add( m_lineStyleCombo, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_fillLabel = new wxStaticText( this, wxID_ANY, _("Fill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillLabel->Wrap( -1 );
	gbSizer2->Add( m_fillLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_fillCtrlChoices[] = { _("None"), _("Solid"), _("Hatch"), _("Reverse Hatch"), _("Cross-hatch") };
	int m_fillCtrlNChoices = sizeof( m_fillCtrlChoices ) / sizeof( wxString );
	m_fillCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fillCtrlNChoices, m_fillCtrlChoices, 0 );
	m_fillCtrl->SetSelection( 0 );
	gbSizer2->Add( m_fillCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_netLabel = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netLabel->Wrap( -1 );
	gbSizer2->Add( m_netLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_netSelector = new NET_SELECTOR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_netSelector, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	gbSizer2->Add( m_LayerLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer2->Add( m_LayerSelectionCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	gbSizer2->AddGrowableCol( 1 );

	m_upperSizer->Add( gbSizer2, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_techLayersLabel = new wxStaticText( this, wxID_ANY, _("Technical Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	bSizer14->Add( m_techLayersLabel, 0, wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer14->Add( m_staticline1, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 4 );


	m_upperSizer->Add( bSizer14, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->AddGrowableCol( 3 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_hasSolderMask = new wxCheckBox( this, wxID_ANY, _("Solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_hasSolderMask, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgSizer2->Add( 10, 0, 1, wxEXPAND, 5 );

	m_solderMaskMarginLabel = new wxStaticText( this, wxID_ANY, _("Expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginLabel->Wrap( -1 );
	fgSizer2->Add( m_solderMaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_solderMaskMarginCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginCtrl->SetToolTip( _("This is the local clearance between the shape and the solder mask opening.\nLeave blank to use the value defined in the Board Setup.") );

	fgSizer2->Add( m_solderMaskMarginCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_solderMaskMarginUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_solderMaskMarginUnit->Wrap( -1 );
	fgSizer2->Add( m_solderMaskMarginUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	m_upperSizer->Add( fgSizer2, 1, wxEXPAND, 5 );


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
	m_cbRoundRect->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onRoundedRectChanged ), NULL, this );
	m_cornerRadiusCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onCornerRadius ), NULL, this );
	m_LayerSelectionCtrl->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onLayerSelection ), NULL, this );
	m_hasSolderMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onTechLayersChanged ), NULL, this );
}

DIALOG_SHAPE_PROPERTIES_BASE::~DIALOG_SHAPE_PROPERTIES_BASE()
{
	// Disconnect Events
	m_cbRoundRect->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onRoundedRectChanged ), NULL, this );
	m_cornerRadiusCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onCornerRadius ), NULL, this );
	m_LayerSelectionCtrl->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onLayerSelection ), NULL, this );
	m_hasSolderMask->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHAPE_PROPERTIES_BASE::onTechLayersChanged ), NULL, this );

}
