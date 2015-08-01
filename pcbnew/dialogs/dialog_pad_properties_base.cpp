///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

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
	m_PadNumCtrl->SetMaxLength( 0 ); 
	fgSizerPadType->Add( m_PadNumCtrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadNameText = new wxStaticText( m_panelGeneral, wxID_ANY, _("Net name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNameText->Wrap( -1 );
	fgSizerPadType->Add( m_PadNameText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadNetNameCtrl = new wxTextCtrl( m_panelGeneral, wxID_PADNETNAMECTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNetNameCtrl->SetMaxLength( 0 ); 
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
	
	wxString m_PadShapeChoices[] = { _("Circular shape"), _("Oval shape"), _("Rectangular shape"), _("Trapezoidal shape") };
	int m_PadShapeNChoices = sizeof( m_PadShapeChoices ) / sizeof( wxString );
	m_PadShape = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadShapeNChoices, m_PadShapeChoices, 0 );
	m_PadShape->SetSelection( 0 );
	fgSizerPadType->Add( m_PadShape, 0, wxALL|wxEXPAND, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizerPadType, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerShapeType;
	fgSizerShapeType = new wxFlexGridSizer( 11, 3, 0, 0 );
	fgSizerShapeType->AddGrowableCol( 1 );
	fgSizerShapeType->SetFlexibleDirection( wxBOTH );
	fgSizerShapeType->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText4 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadPosition_X_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosition_X_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_PadPosition_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadPosX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadPosX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText41 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText41, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadPosition_Y_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosition_Y_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_PadPosition_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadPosY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadPosY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadPosY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText12 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeSize_X_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeSize_X_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_ShapeSize_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeSizeX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeSizeX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeSizeX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText15 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeSize_Y_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeSize_Y_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_ShapeSize_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeSizeY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeSizeY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeSizeY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText48 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText48->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText48, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_PadOrientChoices[] = { _("0"), _("90"), _("-90"), _("180"), _("Custom") };
	int m_PadOrientNChoices = sizeof( m_PadOrientChoices ) / sizeof( wxString );
	m_PadOrient = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PadOrientNChoices, m_PadOrientChoices, 0 );
	m_PadOrient->SetSelection( 4 );
	fgSizerShapeType->Add( m_PadOrient, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_staticText491 = new wxStaticText( m_panelGeneral, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText491->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText491, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PadOrientText = new wxStaticText( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadOrientText->Wrap( -1 );
	fgSizerShapeType->Add( m_PadOrientText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_PadOrientCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadOrientCtrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_PadOrientCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_customOrientUnits = new wxStaticText( m_panelGeneral, wxID_ANY, _("0.1 deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_customOrientUnits->Wrap( -1 );
	fgSizerShapeType->Add( m_customOrientUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText17 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeOffset_X_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeOffset_X_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_ShapeOffset_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeOffsetX_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeOffsetX_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeOffsetX_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText19 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Shape offset Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ShapeOffset_Y_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeOffset_Y_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_ShapeOffset_Y_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeOffsetY_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeOffsetY_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeOffsetY_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText38 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Pad to die length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	m_staticText38->SetToolTip( _("Wire length from pad to die on chip ( used to calculate actual track length)") );
	
	fgSizerShapeType->Add( m_staticText38, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_LengthPadToDieCtrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LengthPadToDieCtrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_LengthPadToDieCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadLengthDie_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadLengthDie_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadLengthDie_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText21 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trap. delta dim:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT, 5 );
	
	m_ShapeDelta_Ctrl = new wxTextCtrl( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeDelta_Ctrl->SetMaxLength( 0 ); 
	fgSizerShapeType->Add( m_ShapeDelta_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadShapeDelta_Unit = new wxStaticText( m_panelGeneral, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadShapeDelta_Unit->Wrap( -1 );
	fgSizerShapeType->Add( m_PadShapeDelta_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText23 = new wxStaticText( m_panelGeneral, wxID_ANY, _("Trap. direction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	wxString m_trapDeltaDirChoiceChoices[] = { _("Horiz."), _("Vert.") };
	int m_trapDeltaDirChoiceNChoices = sizeof( m_trapDeltaDirChoiceChoices ) / sizeof( wxString );
	m_trapDeltaDirChoice = new wxChoice( m_panelGeneral, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_trapDeltaDirChoiceNChoices, m_trapDeltaDirChoiceChoices, 0 );
	m_trapDeltaDirChoice->SetSelection( 0 );
	fgSizerShapeType->Add( m_trapDeltaDirChoice, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_staticText521 = new wxStaticText( m_panelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText521->Wrap( -1 );
	fgSizerShapeType->Add( m_staticText521, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_LeftBoxSizer->Add( fgSizerShapeType, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
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
	
	
	m_LeftBoxSizer->Add( sbSizeModuleInfo, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
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
	
	m_PadDrill_X_Ctrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadDrill_X_Ctrl->SetMaxLength( 0 ); 
	fgSizerGeometry->Add( m_PadDrill_X_Ctrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_X_Unit = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadDrill_X_Unit->Wrap( -1 );
	fgSizerGeometry->Add( m_PadDrill_X_Unit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_textPadDrillY = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPadDrillY->Wrap( -1 );
	fgSizerGeometry->Add( m_textPadDrillY, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_PadDrill_Y_Ctrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PadDrill_Y_Ctrl->SetMaxLength( 0 ); 
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
	
	m_NetClearanceValueCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceValueCtrl->SetMaxLength( 0 ); 
	fgClearancesGridSizer->Add( m_NetClearanceValueCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_NetClearanceUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_NetClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	m_MaskClearanceTitle->SetToolTip( _("This is the local clearance between this  pad and the solder mask\nIf 0, the footprint local value or the global value is used") );
	
	fgClearancesGridSizer->Add( m_MaskClearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetMaxLength( 0 ); 
	fgClearancesGridSizer->Add( m_SolderMaskMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	m_staticTextSolderPaste->SetToolTip( _("This is the local clearance between this pad and the solder paste.\nIf 0 the footprint value or the global value is used..\nThe final clearance value is the sum of this value and the clearance value ratio\nA negative value means a smaller mask size than pad size") );
	
	fgClearancesGridSizer->Add( m_staticTextSolderPaste, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetMaxLength( 0 ); 
	fgClearancesGridSizer->Add( m_SolderPasteMarginCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextRatio = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in per cent between this pad and the solder paste.\nA value of 10 means the clearance value is 10 per cent of the pad size\nIf 0 the footprint value or the global value is used..\nThe final clearance value is the sum of this value and the clearance value\nA negative value means a smaller mask size than pad size.") );
	
	fgClearancesGridSizer->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( sbClearancesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetMaxLength( 0 ); 
	fgClearancesGridSizer->Add( m_SolderPasteMarginRatioCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( sbClearancesSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgClearancesGridSizer->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbClearancesSizer->Add( fgClearancesGridSizer, 0, wxEXPAND, 5 );
	
	
	bSizerClearance->Add( sbClearancesSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerZonesSettings;
	sbSizerZonesSettings = new wxStaticBoxSizer( new wxStaticBox( m_localSettingsPanel, wxID_ANY, _("Copper Zones") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer41;
	fgSizer41 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer41->AddGrowableCol( 1 );
	fgSizer41->SetFlexibleDirection( wxBOTH );
	fgSizer41->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText40 = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Pad connection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	fgSizer41->Add( m_staticText40, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	wxString m_ZoneConnectionChoiceChoices[] = { _("From parent footprint"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	fgSizer41->Add( m_ZoneConnectionChoice, 0, wxLEFT|wxTOP|wxEXPAND, 5 );
	
	m_staticText53 = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText53->Wrap( -1 );
	fgSizer41->Add( m_staticText53, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText49 = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText49->Wrap( -1 );
	fgSizer41->Add( m_staticText49, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5 );
	
	m_ThermalWidthCtrl = new wxTextCtrl( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalWidthCtrl->SetMaxLength( 0 ); 
	fgSizer41->Add( m_ThermalWidthCtrl, 0, wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_ThermalWidthUnits = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalWidthUnits->Wrap( -1 );
	fgSizer41->Add( m_ThermalWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticText52 = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Thermal relief gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52->Wrap( -1 );
	fgSizer41->Add( m_staticText52, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_ThermalGapCtrl = new wxTextCtrl( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalGapCtrl->SetMaxLength( 0 ); 
	fgSizer41->Add( m_ThermalGapCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_ThermalGapUnits = new wxStaticText( sbSizerZonesSettings->GetStaticBox(), wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThermalGapUnits->Wrap( -1 );
	fgSizer41->Add( m_ThermalGapUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	sbSizerZonesSettings->Add( fgSizer41, 0, wxEXPAND, 5 );
	
	
	bSizerClearance->Add( sbSizerZonesSettings, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextWarning = new wxStaticText( m_localSettingsPanel, wxID_ANY, _("Set fields to 0 to use parent or global values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarning->Wrap( -1 );
	m_staticTextWarning->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerClearance->Add( m_staticTextWarning, 0, wxALL, 5 );
	
	
	bSizerPanelClearance->Add( bSizerClearance, 0, wxALL|wxEXPAND, 5 );
	
	
	m_localSettingsPanel->SetSizer( bSizerPanelClearance );
	m_localSettingsPanel->Layout();
	bSizerPanelClearance->Fit( m_localSettingsPanel );
	m_notebook->AddPage( m_localSettingsPanel, _("Local Clearance and Settings"), false );
	
	bSizerUpper->Add( m_notebook, 0, wxALL, 5 );
	
	wxBoxSizer* bSizerDisplayPad;
	bSizerDisplayPad = new wxBoxSizer( wxVERTICAL );
	
	bSizerDisplayPad->SetMinSize( wxSize( 100,-1 ) ); 
	m_panelShowPad = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 200,200 ), wxFULL_REPAINT_ON_RESIZE|wxSIMPLE_BORDER );
	m_panelShowPad->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	
	bSizerDisplayPad->Add( m_panelShowPad, 4, wxRIGHT|wxTOP|wxEXPAND, 5 );
	
	m_panelShowPadGal = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), wxDefaultSize, EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );
	bSizerDisplayPad->Add( m_panelShowPadGal, 4, wxEXPAND|wxRIGHT|wxTOP, 5 );
	
	
	bSizerUpper->Add( bSizerDisplayPad, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	m_MainSizer->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticTextWarningPadFlipped = new wxStaticText( this, wxID_ANY, _("Warning:\nThis pad is flipped on board.\nBack and front layers will be swapped."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWarningPadFlipped->Wrap( -1 );
	m_staticTextWarningPadFlipped->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	m_MainSizer->Add( m_staticTextWarningPadFlipped, 0, wxALL, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	m_MainSizer->Add( m_sdbSizer1, 0, wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
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
	m_panelShowPad->Connect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );
}

DIALOG_PAD_PROPERTIES_BASE::~DIALOG_PAD_PROPERTIES_BASE()
{
	// Disconnect Events
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
	m_panelShowPad->Disconnect( wxEVT_PAINT, wxPaintEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnPaintShowPanel ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );
	
}
