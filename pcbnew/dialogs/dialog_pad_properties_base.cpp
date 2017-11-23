///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 17 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_pad_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PAD_PROPERTIES_BASE::DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_notebook->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	m_panelGeneral = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bGeneralSizer;
	bGeneralSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizerPadType;
	fgSizerPadType = new wxFlexGridSizer( 4, 2, 0, 0 );
	fgSizerPadType->AddGrowableCol( 1 );
	fgSizerPadType->SetFlexibleDirection( wxBOTH );
	fgSizerPadType->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_PadNumText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNumText->Wrap( -1 );
	fgSizerPadType->Add( m_PadNumText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadNumCtrl = new wxTextCtrl( m_panelGeneral, wxID_PADNUMCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPadType->Add( m_PadNumCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadNameText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Net name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNameText->Wrap( -1 );
	fgSizerPadType->Add( m_PadNameText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadNetNameCtrl = new wxTextCtrl( m_panelGeneral, wxID_PADNETNAMECTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPadType->Add( m_PadNetNameCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText44 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	fgSizerPadType->Add( m_staticText44, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_PadTypeChoices[] = { _("Through-hole"), _("SMD"), _("Connector"), _("NPTH, Mechanical") };
	int m_PadTypeNChoices = sizeof( m_PadTypeChoices ) / sizeof( wxString );
	m_PadType = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadTypeNChoices, m_PadTypeChoices, 0 );
	m_PadType->SetSelection( 0 );
	fgSizerPadType->Add( m_PadType, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText45 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	fgSizerPadType->Add( m_staticText45, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	wxString m_PadShapeChoices[] = { _("Circular"), _("Oval"), _("Rectangular"), _("Trapezoidal"), _("Rounded Rectangle"), _("Custom Shape (Circular Anchor)"), _("Custom Shape (Rectangular Anchor)") };
	int m_PadShapeNChoices = sizeof( m_PadShapeChoices ) / sizeof( wxString );
	m_PadShape = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadShapeNChoices, m_PadShapeChoices, 0 );
	m_PadShape->SetSelection( 0 );
	fgSizerPadType->Add( m_PadShape, 0, wxALL|wxEXPAND, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizerPadType, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerShapeType;
	fgSizerShapeType = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerShapeType->AddGrowableCol( 1 );
	fgSizerShapeType->SetFlexibleDirection( wxBOTH );
	fgSizerShapeType->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText4 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadPosition_X_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_PadPosition_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadPosX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadPosX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText41 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText41, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadPosition_Y_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_PadPosition_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadPosY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadPosY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText12 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeSize_X_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_ShapeSize_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeSizeX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeSizeX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeSizeX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText15 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeSize_Y_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_ShapeSize_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeSizeY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeSizeY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeSizeY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadOrientText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadOrientText->Wrap( -1 );
	fgSizerShapeType->Add( m_PadOrientText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_PadOrientChoices[] = { _("0"), _("90"), _("-90"), _("180"), _("Custom") };
	int m_PadOrientNChoices = sizeof( m_PadOrientChoices ) / sizeof( wxString );
	m_PadOrient = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadOrientNChoices, m_PadOrientChoices, 0 );
	m_PadOrient->SetSelection( 4 );
	fgSizerShapeType->Add( m_PadOrient, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_staticText491 = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText491->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText491, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_PadOrientCtrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_PadOrientCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_customOrientUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_customOrientUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_customOrientUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText17 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeOffset_X_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_ShapeOffset_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeOffsetX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeOffsetX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeOffsetX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText19 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeOffset_Y_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_ShapeOffset_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeOffsetY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeOffsetY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeOffsetY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText38 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad to die length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	m_staticText38->SetToolTip( _("Wire length from pad to die on chip ( used to calculate actual track length)") );
	
	fgSizerShapeType->Add( m_staticText38, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_LengthPadToDieCtrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_LengthPadToDieCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadLengthDie_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadLengthDie_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadLengthDie_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticline4 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline4, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline5 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline6 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline6, 0, wxEXPAND | wxALL, 5 );
	
	m_staticText21 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trapezoid delta:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );
	
	m_ShapeDelta_Ctrl = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_ShapeDelta_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeDelta_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeDelta_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeDelta_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText23 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trapezoid direction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	wxString m_trapDeltaDirChoiceChoices[] = { _("Horizontal"), _("Vertical") };
	int m_trapDeltaDirChoiceNChoices = sizeof( m_trapDeltaDirChoiceChoices ) / sizeof( wxString );
	m_trapDeltaDirChoice = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trapDeltaDirChoiceNChoices, m_trapDeltaDirChoiceChoices, 0 );
	m_trapDeltaDirChoice->SetSelection( 0 );
	fgSizerShapeType->Add( m_trapDeltaDirChoice, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	
	fgSizerShapeType->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline7 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline7, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline8 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline8, 0, wxEXPAND | wxALL, 5 );
	
	m_staticline9 = new wxStaticLine( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerShapeType->Add( m_staticline9, 0, wxEXPAND | wxALL, 5 );
	
	m_staticTextCornerSizeRatio = new wxStaticText( m_panelGeneral, wxID_ANY, _("Corner size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerSizeRatio->Wrap( -1 );
	m_staticTextCornerSizeRatio->SetToolTip( _("Corner radius in percent  of the pad width.\nThe width is the smaller value between size X and size Y.\nThe max value is 50 percent.") );
	
	fgSizerShapeType->Add( m_staticTextCornerSizeRatio, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_tcCornerSizeRatio = new TEXT_CTRL_EVAL( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeType->Add( m_tcCornerSizeRatio, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_staticTextCornerSizeRatioUnit = new wxStaticText( m_panelGeneral, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerSizeRatioUnit->Wrap( -1 );
	fgSizerShapeType->Add( m_staticTextCornerSizeRatioUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextCornerRadius = new wxStaticText( m_panelGeneral, wxID_ANY, _("Corner radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerRadius->Wrap( -1 );
	m_staticTextCornerRadius->SetToolTip( _("Corner radius.\nCan be no more than half pad width.\nThe width is the smaller value between size X and size Y.\nNote: IPC norm gives a max value = 0.25mm.") );
	
	fgSizerShapeType->Add( m_staticTextCornerRadius, 0, wxALL, 5 );
	
	m_staticTextCornerRadiusValue = new wxStaticText( m_panelGeneral, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerRadiusValue->Wrap( -1 );
	m_staticTextCornerRadiusValue->SetToolTip( _("Corner radius.\nCan be no more than half pad width.\nThe width is the smaller value between size X and size Y\nNote: IPC norm gives a max value = 0.25mm") );
	
	fgSizerShapeType->Add( m_staticTextCornerRadiusValue, 0, wxALL, 5 );
	
	m_staticTextCornerSizeUnit = new wxStaticText( m_panelGeneral, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerSizeUnit->Wrap( -1 );
	fgSizerShapeType->Add( m_staticTextCornerSizeUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizerShapeType, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bGeneralSizer->Add( m_LeftBoxSizer, 3, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_panelGeneral, wxID_ANY, _("Drill") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerGeometry;
	fgSizerGeometry = new wxFlexGridSizer( 14, 3, 0, 0 );
	fgSizerGeometry->AddGrowableCol( 1 );
	fgSizerGeometry->SetFlexibleDirection( wxBOTH );
	fgSizerGeometry->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText47 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText47->Wrap( -1 );
	fgSizerGeometry->Add( m_staticText47, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_DrillShapeCtrlChoices[] = { _("Circular hole"), _("Oval hole") };
	int m_DrillShapeCtrlNChoices = sizeof( m_DrillShapeCtrlChoices ) / sizeof( wxString );
	m_DrillShapeCtrl = new wxChoice( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DrillShapeCtrlNChoices, m_DrillShapeCtrlChoices, 0 );
	m_DrillShapeCtrl->SetSelection( 0 );
	fgSizerGeometry->Add( m_DrillShapeCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_staticText51 = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizerGeometry->Add( m_staticText51, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_textPadDrillX = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPadDrillX->Wrap( -1 );
	fgSizerGeometry->Add( m_textPadDrillX, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_X_Ctrl = new TEXT_CTRL_EVAL( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGeometry->Add( m_PadDrill_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_X_Unit = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadDrill_X_Unit->Wrap( -1 );
	fgSizerGeometry->Add( m_PadDrill_X_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_textPadDrillY = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPadDrillY->Wrap( -1 );
	fgSizerGeometry->Add( m_textPadDrillY, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_Y_Ctrl = new TEXT_CTRL_EVAL( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGeometry->Add( m_PadDrill_Y_Ctrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_Y_Unit = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadDrill_Y_Unit->Wrap( -1 );
	fgSizerGeometry->Add( m_PadDrill_Y_Unit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbSizer2->Add( fgSizerGeometry, 0, wxEXPAND, 5 );
	
	
	bSizer10->Add( sbSizer2, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_LayersSizer;
	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelGeneral, wxID_ANY, _("Layers") ), wxVERTICAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText511 = new wxStaticText( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Copper:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText511->Wrap( -1 );
	bSizer11->Add( m_staticText511, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_rbCopperLayersSelChoices[] = { _("Front layer"), _("Back layer"), _("All copper layers"), _("None") };
	int m_rbCopperLayersSelNChoices = sizeof( m_rbCopperLayersSelChoices ) / sizeof( wxString );
	m_rbCopperLayersSel = new wxChoice( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_rbCopperLayersSelNChoices, m_rbCopperLayersSelChoices, 0 );
	m_rbCopperLayersSel->SetSelection( 0 );
	bSizer11->Add( m_rbCopperLayersSel, 1, wxALL|wxEXPAND, 5 );
	
	
	m_LayersSizer->Add( bSizer11, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerTechlayers;
	sbSizerTechlayers = new wxStaticBoxSizer( new wxStaticBox( m_LayersSizer->GetStaticBox(), wxID_ANY, _("Technical Layers") ), wxVERTICAL );
	
	m_PadLayerAdhCmp = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Front adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerAdhCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerAdhCu = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Back adhesive"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerAdhCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerPateCmp = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Front solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerPateCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerPateCu = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Back solder paste"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerPateCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerSilkCmp = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Front silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerSilkCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerSilkCu = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Back silk screen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerSilkCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerMaskCmp = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Front solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerMaskCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerMaskCu = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Back solder mask"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerMaskCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerDraft = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("Drafting notes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerDraft, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerECO1 = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("E.C.O.1"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerECO1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerECO2 = new wxCheckBox( sbSizerTechlayers->GetStaticBox(), wxID_ANY, _("E.C.O.2"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerTechlayers->Add( m_PadLayerECO2, 0, wxALL, 5 );
	
	
	m_LayersSizer->Add( sbSizerTechlayers, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer10->Add( m_LayersSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizeModuleInfo;
	sbSizeModuleInfo = new wxStaticBoxSizer( new wxStaticBox( m_panelGeneral, wxID_ANY, _("Parent footprint orientation") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTitleModuleRot = new wxStaticText( sbSizeModuleInfo->GetStaticBox(), wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTitleModuleRot->Wrap( -1 );
	fgSizer4->Add( m_staticTitleModuleRot, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_staticModuleRotValue = new wxStaticText( sbSizeModuleInfo->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticModuleRotValue->Wrap( -1 );
	fgSizer4->Add( m_staticModuleRotValue, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTitleModuleSide = new wxStaticText( sbSizeModuleInfo->GetStaticBox(), wxID_ANY, _("Board side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTitleModuleSide->Wrap( -1 );
	fgSizer4->Add( m_staticTitleModuleSide, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_staticModuleSideValue = new wxStaticText( sbSizeModuleInfo->GetStaticBox(), wxID_ANY, _("Front side"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticModuleSideValue->Wrap( -1 );
	fgSizer4->Add( m_staticModuleSideValue, 0, wxALL|wxEXPAND, 5 );
	
	
	sbSizeModuleInfo->Add( fgSizer4, 0, 0, 5 );
	
	
	bSizer10->Add( sbSizeModuleInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bGeneralSizer->Add( bSizer10, 2, wxALL|wxEXPAND, 5 );
	
	
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
	
	wxFlexGridSizer* fgClearancesGridSizer;
	fgClearancesGridSizer = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgClearancesGridSizer->AddGrowableCol( 1 );
	fgClearancesGridSizer->SetFlexibleDirection( wxBOTH );
	fgClearancesGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextNetClearance = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Net pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetClearance->Wrap( -1 );
	m_staticTextNetClearance->SetToolTip( _("This is the local net clearance for  pad.\nIf 0, the footprint local value or the Netclass value is used") );
	
	fgClearancesGridSizer->Add( m_staticTextNetClearance, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_NetClearanceValueCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_NetClearanceValueCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_NetClearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_NetClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	m_MaskClearanceTitle->SetToolTip( _("This is the local clearance between this  pad and the solder mask\nIf 0, the footprint local value or the global value is used") );
	
	fgClearancesGridSizer->Add( m_MaskClearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_SolderMaskMarginCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_SolderMaskMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	m_staticTextSolderPaste->SetToolTip( _("This is the local clearance between this pad and the solder paste.\nIf 0 the footprint value or the global value is used..\nThe final clearance value is the sum of this value and the clearance value ratio\nA negative value means a smaller mask size than pad size") );
	
	fgClearancesGridSizer->Add( m_staticTextSolderPaste, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_SolderPasteMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextRatio = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in per cent between this pad and the solder paste.\nA value of 10 means the clearance value is 10 per cent of the pad size\nIf 0 the footprint value or the global value is used..\nThe final clearance value is the sum of this value and the clearance value\nA negative value means a smaller mask size than pad size.") );
	
	fgClearancesGridSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginRatioCtrl = new TEXT_CTRL_EVAL( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgClearancesGridSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbClearancesSizer->Add( fgClearancesGridSizer, 0, wxEXPAND, 5 );
	
	
	bSizerClearance->Add( sbClearancesSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_sbSizerZonesSettings = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Copper Zones") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerCopperZonesOpts;
	fgSizerCopperZonesOpts = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerCopperZonesOpts->AddGrowableCol( 1 );
	fgSizerCopperZonesOpts->SetFlexibleDirection( wxBOTH );
	fgSizerCopperZonesOpts->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText40 = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticText40, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_ZoneConnectionChoiceChoices[] = { _("From parent footprint"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneConnectionChoice, 0, wxLEFT|wxTOP|wxEXPAND, 5 );
	
	
	fgSizerCopperZonesOpts->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText49 = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText49->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticText49, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ThermalWidthCtrl = new TEXT_CTRL_EVAL( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_ThermalWidthCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_ThermalWidthUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalWidthUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_ThermalWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText52 = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticText52, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_ThermalGapCtrl = new TEXT_CTRL_EVAL( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerCopperZonesOpts->Add( m_ThermalGapCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_ThermalGapUnits = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalGapUnits->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_ThermalGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextcps = new wxStaticText( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Custom pad shape in zone:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextcps->Wrap( -1 );
	fgSizerCopperZonesOpts->Add( m_staticTextcps, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_ZoneCustomPadShapeChoices[] = { _("Use pad shape"), _("Use pad convex hull") };
	int m_ZoneCustomPadShapeNChoices = sizeof( m_ZoneCustomPadShapeChoices ) / sizeof( wxString );
	m_ZoneCustomPadShape = new wxChoice( m_sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneCustomPadShapeNChoices, m_ZoneCustomPadShapeChoices, 0 );
	m_ZoneCustomPadShape->SetSelection( 0 );
	fgSizerCopperZonesOpts->Add( m_ZoneCustomPadShape, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );
	
	
	fgSizerCopperZonesOpts->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_sbSizerZonesSettings->Add( fgSizerCopperZonesOpts, 0, wxEXPAND, 5 );
	
	
	bSizerClearance->Add( m_sbSizerZonesSettings, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextWarning = new wxStaticText( m_localSettingsPanel, wxID_ANY, _("Set fields to 0 to use parent or global values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarning->Wrap( -1 );
	m_staticTextWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerClearance->Add( m_staticTextWarning, 0, wxALL, 5 );
	
	
	bSizerPanelClearance->Add( bSizerClearance, 0, wxALL|wxEXPAND, 5 );
	
	
	m_localSettingsPanel->SetSizer( bSizerPanelClearance );
	m_localSettingsPanel->Layout();
	bSizerPanelClearance->Fit( m_localSettingsPanel );
	m_notebook->AddPage( m_localSettingsPanel, _("Local Clearance and Settings"), false );
	m_panelCustomShapePrimitives = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_bSizerPanelPrimitives = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextPrimitivesList = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Primitives list"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitivesList->Wrap( -1 );
	m_staticTextPrimitivesList->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	m_bSizerPanelPrimitives->Add( m_staticTextPrimitivesList, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextPrimitiveListWarning = new wxStaticText( m_panelCustomShapePrimitives, wxID_ANY, _("Coordinates are relative to anchor pad, orientation 0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPrimitiveListWarning->Wrap( -1 );
	m_staticTextPrimitiveListWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	m_bSizerPanelPrimitives->Add( m_staticTextPrimitiveListWarning, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_listCtrlPrimitives = new wxListView( m_panelCustomShapePrimitives, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_NO_HEADER|wxLC_REPORT );
	m_bSizerPanelPrimitives->Add( m_listCtrlPrimitives, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerButtonsUpper;
	bSizerButtonsUpper = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonDel = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Delete Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDel, 0, wxALL, 5 );
	
	m_buttonEditShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Edit Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonEditShape, 0, wxALL, 5 );
	
	m_buttonAddShape = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Add Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonAddShape, 0, wxALL, 5 );
	
	m_buttonDup = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Duplicate Primitive"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsUpper->Add( m_buttonDup, 0, wxALL, 5 );
	
	
	bSizerButtons->Add( bSizerButtonsUpper, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizerButtonsLower;
	bSizerButtonsLower = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonGeometry = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Geometry Transform"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsLower->Add( m_buttonGeometry, 0, wxALL, 5 );
	
	m_buttonImport = new wxButton( m_panelCustomShapePrimitives, wxID_ANY, _("Import Primitives"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtonsLower->Add( m_buttonImport, 0, wxALL, 5 );
	
	
	bSizerButtons->Add( bSizerButtonsLower, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_bSizerPanelPrimitives->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_panelCustomShapePrimitives->SetSizer( m_bSizerPanelPrimitives );
	m_panelCustomShapePrimitives->Layout();
	m_bSizerPanelPrimitives->Fit( m_panelCustomShapePrimitives );
	m_notebook->AddPage( m_panelCustomShapePrimitives, _("Custom Shape Primitives"), false );
	
	bSizerUpper->Add( m_notebook, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );
	
	bSizerDisplayPad->SetMinSize( wxSize( 200,-1 ) ); 
	m_panelShowPad = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 200,-1 ), wxFULL_REPAINT_ON_RESIZE|wxSIMPLE_BORDER );
	m_panelShowPad->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	
	bSizerDisplayPad->Add( m_panelShowPad, 4, wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_panelShowPadGal = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, m_galOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO);
	bSizerDisplayPad->Add( m_panelShowPadGal, 4, wxEXPAND|wxRIGHT|wxTOP, 5 );
	
	
	bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticTextWarningPadFlipped = new wxStaticText( this, wxID_ANY, _("Warning:\nThis pad is flipped on board.\nBack and front layers will be swapped."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarningPadFlipped->Wrap( -1 );
	m_staticTextWarningPadFlipped->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	m_MainSizer->Add( m_staticTextWarningPadFlipped, 0, wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	m_MainSizer->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_PadNumCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadNetNameCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_PadShape->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_ShapeSize_X_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeSize_Y_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadOrient->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_PadOrientCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeOffset_X_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeOffset_Y_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeDelta_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapDeltaDirChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_tcCornerSizeRatio->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_DrillShapeCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_PadDrill_X_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadDrill_Y_Ctrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_rbCopperLayersSel->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
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
	m_NetClearanceValueCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonDel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
	m_buttonEditShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonAddShape->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonDup->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_buttonImport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onImportPrimitives ), NULL, this );
	m_panelShowPad->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );
}

DIALOG_PAD_PROPERTIES_BASE::~DIALOG_PAD_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnInitDialog ) );
	m_PadNumCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadNetNameCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadTypeSelected ), NULL, this );
	m_PadShape->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPadShapeSelection ), NULL, this );
	m_ShapeSize_X_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeSize_Y_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadOrient->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadOrientEvent ), NULL, this );
	m_PadOrientCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeOffset_X_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeOffset_Y_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_ShapeDelta_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_trapDeltaDirChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
	m_tcCornerSizeRatio->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onCornerSizePercentChange ), NULL, this );
	m_DrillShapeCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnDrillShapeSelected ), NULL, this );
	m_PadDrill_X_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_PadDrill_Y_Ctrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_rbCopperLayersSel->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnSetLayers ), NULL, this );
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
	m_NetClearanceValueCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnValuesChanged ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DIALOG_PAD_PROPERTIES_BASE::onPrimitiveDClick ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_listCtrlPrimitives->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPrimitiveSelection ), NULL, this );
	m_buttonDel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDeletePrimitive ), NULL, this );
	m_buttonEditShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onEditPrimitive ), NULL, this );
	m_buttonAddShape->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onAddPrimitive ), NULL, this );
	m_buttonDup->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onDuplicatePrimitive ), NULL, this );
	m_buttonGeometry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onGeometryTransform ), NULL, this );
	m_buttonImport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::onImportPrimitives ), NULL, this );
	m_panelShowPad->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancel ), NULL, this );
	
}

DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE::DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizermain;
	bSizermain = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Filled circle: set thickness to 0\nRing:  set thickness to the width of the ring"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizermain->Add( m_staticTextInfo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxFlexGridSizer* fgSizerShapeProperties;
	fgSizerShapeProperties = new wxFlexGridSizer( 0, 6, 0, 0 );
	fgSizerShapeProperties->AddGrowableCol( 2 );
	fgSizerShapeProperties->AddGrowableCol( 4 );
	fgSizerShapeProperties->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextPosStart = new wxStaticText( this, wxID_ANY, _("Start point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosStart->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosStart, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStartX = new wxStaticText( this, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStartX->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextStartX, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrPosX = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrPosX, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextStartY = new wxStaticText( this, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStartY->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextStartY, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrPosY = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrPosY, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextPosUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextPosEnd = new wxStaticText( this, wxID_ANY, _("End point"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosEnd->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextPosEnd, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextEndX = new wxStaticText( this, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndX->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextEndX, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrEndX = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrEndX, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextEndY = new wxStaticText( this, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndY->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextEndY, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrEndY = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrEndY, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextEndUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEndUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextEndUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextAngle = new wxStaticText( this, wxID_ANY, _("Angle"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAngle->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextAngle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textCtrAngle = new TEXT_CTRL_EVAL( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrAngle, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextAngleUnit = new wxStaticText( this, wxID_ANY, _("degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAngleUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextAngleUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextThickness = new wxStaticText( this, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThickness->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextThickness, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textCtrlThickness = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrlThickness, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextThicknessUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThicknessUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextThicknessUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizermain->Add( fgSizerShapeProperties, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizermain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizermain );
	this->Layout();
	
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
	
	wxFlexGridSizer* fgSizerShapeProperties;
	fgSizerShapeProperties = new wxFlexGridSizer( 0, 6, 0, 0 );
	fgSizerShapeProperties->AddGrowableCol( 2 );
	fgSizerShapeProperties->AddGrowableCol( 4 );
	fgSizerShapeProperties->SetFlexibleDirection( wxBOTH );
	fgSizerShapeProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextMove = new wxStaticText( this, wxID_ANY, _("Move vector"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMove->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextMove, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextMoveX = new wxStaticText( this, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMoveX->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextMoveX, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrMoveX = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrMoveX, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextMoveY = new wxStaticText( this, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMoveY->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextMoveY, 0, wxALL, 5 );
	
	m_textCtrMoveY = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrMoveY, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextMoveUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMoveUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextMoveUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextAngle = new wxStaticText( this, wxID_ANY, _("Rotation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAngle->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextAngle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textCtrAngle = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrAngle, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextAngleUnit = new wxStaticText( this, wxID_ANY, _("degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAngleUnit->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextAngleUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextSF = new wxStaticText( this, wxID_ANY, _("Scaling factor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSF->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextSF, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textCtrlScalingFactor = new TEXT_CTRL_EVAL( this, wxID_ANY, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerShapeProperties->Add( m_textCtrlScalingFactor, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextDupCnt = new wxStaticText( this, wxID_ANY, _("Duplicate count"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDupCnt->Wrap( -1 );
	fgSizerShapeProperties->Add( m_staticTextDupCnt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_spinCtrlDuplicateCount = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1 );
	fgSizerShapeProperties->Add( m_spinCtrlDuplicateCount, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizerShapeProperties->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizermain->Add( fgSizerShapeProperties, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizermain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizermain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizermain );
	this->Layout();
	
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
	
	m_staticTextCornerListWarning = new wxStaticText( this, wxID_ANY, _("Coordinates are relative to anchor pad, orientation 0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCornerListWarning->Wrap( -1 );
	m_staticTextCornerListWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerMain->Add( m_staticTextCornerListWarning, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextValidate = new wxStaticText( this, wxID_ANY, _("Incorrect polygon"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValidate->Wrap( -1 );
	m_staticTextValidate->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	
	bSizerMain->Add( m_staticTextValidate, 0, wxALL, 5 );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerCornerlist;
	bSizerCornerlist = new wxBoxSizer( wxVERTICAL );
	
	m_gridCornersList = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridCornersList->CreateGrid( 1, 2 );
	m_gridCornersList->EnableEditing( true );
	m_gridCornersList->EnableGridLines( true );
	m_gridCornersList->EnableDragGridSize( false );
	m_gridCornersList->SetMargins( 0, 0 );
	
	// Columns
	m_gridCornersList->EnableDragColMove( false );
	m_gridCornersList->EnableDragColSize( true );
	m_gridCornersList->SetColLabelSize( 30 );
	m_gridCornersList->SetColLabelValue( 0, _("Pos X") );
	m_gridCornersList->SetColLabelValue( 1, _("Pos Y") );
	m_gridCornersList->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridCornersList->AutoSizeRows();
	m_gridCornersList->EnableDragRowSize( false );
	m_gridCornersList->SetRowLabelSize( 80 );
	m_gridCornersList->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridCornersList->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerCornerlist->Add( m_gridCornersList, 1, wxALL, 5 );
	
	wxBoxSizer* bSizerRightButts;
	bSizerRightButts = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonAdd = new wxButton( this, wxID_ANY, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRightButts->Add( m_buttonAdd, 0, wxALL, 5 );
	
	m_buttonDelete = new wxButton( this, wxID_ANY, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerRightButts->Add( m_buttonDelete, 0, wxALL, 5 );
	
	
	bSizerCornerlist->Add( bSizerRightButts, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer23->Add( bSizerCornerlist, 0, wxEXPAND, 5 );
	
	m_panelPoly = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	m_panelPoly->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	m_panelPoly->SetMinSize( wxSize( 300,300 ) );
	
	bSizer23->Add( m_panelPoly, 1, wxEXPAND | wxALL, 5 );
	
	
	bSizerUpper->Add( bSizer23, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizerThickness;
	fgSizerThickness = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerThickness->AddGrowableCol( 1 );
	fgSizerThickness->SetFlexibleDirection( wxBOTH );
	fgSizerThickness->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextThickness = new wxStaticText( this, wxID_ANY, _("Outline thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThickness->Wrap( -1 );
	fgSizerThickness->Add( m_staticTextThickness, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlThickness = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerThickness->Add( m_textCtrlThickness, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextThicknessUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextThicknessUnit->Wrap( -1 );
	fgSizerThickness->Add( m_staticTextThicknessUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizerThickness->Add( 10, 10, 0, 0, 5 );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("(Thickness outline is usually set to 0)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	fgSizerThickness->Add( m_staticTextInfo, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerUpper->Add( fgSizerThickness, 0, 0, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_gridCornersList->Connect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onButtonAdd ), NULL, this );
	m_buttonDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );
}

DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE()
{
	// Disconnect Events
	m_gridCornersList->Disconnect( wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onGridSelect ), NULL, this );
	m_gridCornersList->Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onCellSelect ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onButtonAdd ), NULL, this );
	m_buttonDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::OnButtonDelete ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPaintPolyPanel ), NULL, this );
	m_panelPoly->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE::onPolyPanelResize ), NULL, this );
	
}
