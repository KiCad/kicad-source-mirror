///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_module_for_BoardEditor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODULE_BOARD_EDITOR_BASE::DIALOG_MODULE_BOARD_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelProperties = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelPropertiesBoxSizer;
	m_PanelPropertiesBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextRef = new wxStaticText( m_PanelProperties, wxID_ANY, _("Reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	bSizerLeft->Add( m_staticTextRef, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerRef;
	bSizerRef = new wxBoxSizer( wxHORIZONTAL );
	
	m_ReferenceCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_ReferenceCtrl->SetMaxLength( 0 ); 
	bSizerRef->Add( m_ReferenceCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	m_button4 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerRef->Add( m_button4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerLeft->Add( bSizerRef, 0, wxEXPAND, 5 );
	
	m_staticTextVal = new wxStaticText( m_PanelProperties, wxID_ANY, _("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVal->Wrap( -1 );
	bSizerLeft->Add( m_staticTextVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerVal;
	bSizerVal = new wxBoxSizer( wxHORIZONTAL );
	
	m_ValueCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_ValueCtrl->SetMaxLength( 0 ); 
	bSizerVal->Add( m_ValueCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	m_button5 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerVal->Add( m_button5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerLeft->Add( bSizerVal, 0, wxEXPAND, 5 );
	
	wxString m_LayerCtrlChoices[] = { _("Front"), _("Back") };
	int m_LayerCtrlNChoices = sizeof( m_LayerCtrlChoices ) / sizeof( wxString );
	m_LayerCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Board Side"), wxDefaultPosition, wxDefaultSize, m_LayerCtrlNChoices, m_LayerCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_LayerCtrl->SetSelection( 0 );
	bSizerLeft->Add( m_LayerCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_OrientCtrlChoices[] = { _("0.0"), _("+90.0"), _("-90.0"), _("180.0"), _("Other") };
	int m_OrientCtrlNChoices = sizeof( m_OrientCtrlChoices ) / sizeof( wxString );
	m_OrientCtrl = new wxRadioBox( m_PanelProperties, ID_LISTBOX_ORIENT_SELECT, _("Rotation"), wxDefaultPosition, wxDefaultSize, m_OrientCtrlNChoices, m_OrientCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientCtrl->SetSelection( 0 );
	bSizerLeft->Add( m_OrientCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextRotation = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation (in 0.1 degrees):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotation->Wrap( -1 );
	bSizerLeft->Add( m_staticTextRotation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OrientValue = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OrientValue->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_OrientValue, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextPos = new wxStaticText( m_PanelProperties, wxID_ANY, _("Position"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPos->Wrap( -1 );
	bSizerLeft->Add( m_staticTextPos, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerPos;
	fgSizerPos = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizerPos->AddGrowableCol( 1 );
	fgSizerPos->SetFlexibleDirection( wxHORIZONTAL );
	fgSizerPos->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_XPosLabel = new wxStaticText( m_PanelProperties, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_XPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_ModPositionX = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ModPositionX->SetMaxLength( 0 ); 
	fgSizerPos->Add( m_ModPositionX, 1, wxEXPAND|wxRIGHT, 5 );
	
	m_XPosUnit = new wxStaticText( m_PanelProperties, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_XPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_YPosLabel = new wxStaticText( m_PanelProperties, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_YPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_ModPositionY = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ModPositionY->SetMaxLength( 0 ); 
	fgSizerPos->Add( m_ModPositionY, 1, wxEXPAND|wxRIGHT, 5 );
	
	m_YPosUnit = new wxStaticText( m_PanelProperties, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_YPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizerLeft->Add( fgSizerPos, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_TextSheetPath = new wxStaticText( m_PanelProperties, wxID_ANY, _("Sheet path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSheetPath->Wrap( -1 );
	bSizerLeft->Add( m_TextSheetPath, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlSheetPath = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlSheetPath->SetToolTip( _("An unique ID (a time stamp) to identify the component.\nThis is an alternate identifier to the reference.") );
	
	bSizerLeft->Add( m_textCtrlSheetPath, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	m_PanelPropertiesBoxSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	m_PropRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonExchange = new wxButton( m_PanelProperties, ID_MODULE_PROPERTIES_EXCHANGE, _("Change Footprint(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PropRightSizer->Add( m_buttonExchange, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonModuleEditor = new wxButton( m_PanelProperties, ID_GOTO_MODULE_EDITOR, _("Footprint Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PropRightSizer->Add( m_buttonModuleEditor, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerAttrib;
	bSizerAttrib = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_AttributsCtrlChoices[] = { _("Normal"), _("Normal+Insert"), _("Virtual") };
	int m_AttributsCtrlNChoices = sizeof( m_AttributsCtrlChoices ) / sizeof( wxString );
	m_AttributsCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Attributes"), wxDefaultPosition, wxDefaultSize, m_AttributsCtrlNChoices, m_AttributsCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AttributsCtrl->SetSelection( 0 );
	bSizerAttrib->Add( m_AttributsCtrl, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_AutoPlaceCtrlChoices[] = { _("Free"), _("Lock pads"), _("Lock footprint") };
	int m_AutoPlaceCtrlNChoices = sizeof( m_AutoPlaceCtrlChoices ) / sizeof( wxString );
	m_AutoPlaceCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Move and Place"), wxDefaultPosition, wxDefaultSize, m_AutoPlaceCtrlNChoices, m_AutoPlaceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AutoPlaceCtrl->SetSelection( 0 );
	bSizerAttrib->Add( m_AutoPlaceCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( bSizerAttrib, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerAP;
	sbSizerAP = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Auto Place") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizerRotOpt;
	bSizerRotOpt = new wxBoxSizer( wxVERTICAL );
	
	m_staticText11 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotate 90 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	bSizerRotOpt->Add( m_staticText11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_CostRot90Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerRotOpt->Add( m_CostRot90Ctrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerAP->Add( bSizerRotOpt, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerMoveOpt;
	bSizerMoveOpt = new wxBoxSizer( wxVERTICAL );
	
	m_staticText12 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotate 180 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizerMoveOpt->Add( m_staticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_CostRot180Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerMoveOpt->Add( m_CostRot180Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbSizerAP->Add( bSizerMoveOpt, 1, wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( sbSizerAP, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerLocalProperties;
	sbSizerLocalProperties = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Local Settings") ), wxVERTICAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText16 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Pad connection to zones:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bSizer10->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_ZoneConnectionChoiceChoices[] = { _("Use zone setting"), _("Solid"), _("Thermal relief"), _("None") };
	int m_ZoneConnectionChoiceNChoices = sizeof( m_ZoneConnectionChoiceChoices ) / sizeof( wxString );
	m_ZoneConnectionChoice = new wxChoice( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ZoneConnectionChoiceNChoices, m_ZoneConnectionChoiceChoices, 0 );
	m_ZoneConnectionChoice->SetSelection( 0 );
	bSizer10->Add( m_ZoneConnectionChoice, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer11->Add( bSizer10, 1, wxEXPAND, 5 );
	
	m_staticTextInfo = new wxStaticText( m_PanelProperties, wxID_ANY, _("Set clearances to 0 to use global values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer11->Add( m_staticTextInfo, 0, wxALL, 5 );
	
	
	sbSizerLocalProperties->Add( bSizer11, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizerClearances;
	fgSizerClearances = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizerClearances->AddGrowableCol( 1 );
	fgSizerClearances->SetFlexibleDirection( wxBOTH );
	fgSizerClearances->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextNetClearance = new wxStaticText( m_PanelProperties, wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetClearance->Wrap( -1 );
	m_staticTextNetClearance->SetToolTip( _("This is the local net clearance for all pad of this footprint\nIf 0, the Netclass values are used\nThis value can be superseded by a pad local value.") );
	
	fgSizerClearances->Add( m_staticTextNetClearance, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_NetClearanceValueCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceValueCtrl->SetMaxLength( 0 ); 
	fgSizerClearances->Add( m_NetClearanceValueCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_NetClearanceUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgSizerClearances->Add( m_NetClearanceUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticline1 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerClearances->Add( m_staticline1, 1, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerClearances->Add( m_staticline2, 1, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizerClearances->Add( m_staticline3, 1, wxEXPAND, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	m_MaskClearanceTitle->SetToolTip( _("This is the local clearance between pads and the solder mask\nfor this footprint\nThis value can be superseded by a pad local value.\nIf 0, the global value is used") );
	
	fgSizerClearances->Add( m_MaskClearanceTitle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetMaxLength( 0 ); 
	fgSizerClearances->Add( m_SolderMaskMarginCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	m_staticTextSolderPaste->SetToolTip( _("This is the local clearance between pads and the solder paste\nfor this footprint.\nThis value can be superseded by a pad local values.\nThe final clearance value is the sum of this value and the clearance value ratio\nA negative value means a smaller mask size than pad size") );
	
	fgSizerClearances->Add( m_staticTextSolderPaste, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetMaxLength( 0 ); 
	fgSizerClearances->Add( m_SolderPasteMarginCtrl, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextRatio = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in per cent between pads and the solder paste\nfor this footprint.\nA value of 10 means the clearance value is 10 per cent of the pad size\nThis value can be superseded by a pad local value.\nThe final clearance value is the sum of this value and the clearance value\nA negative value means a smaller mask size than pad size.") );
	
	fgSizerClearances->Add( m_staticTextRatio, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetMaxLength( 0 ); 
	fgSizerClearances->Add( m_SolderPasteMarginRatioCtrl, 1, wxALL|wxEXPAND, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgSizerClearances->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	sbSizerLocalProperties->Add( fgSizerClearances, 1, wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( sbSizerLocalProperties, 0, wxALL|wxEXPAND, 5 );
	
	
	m_PanelPropertiesBoxSizer->Add( m_PropRightSizer, 0, 0, 5 );
	
	
	m_PanelProperties->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelProperties->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelProperties );
	m_NoteBook->AddPage( m_PanelProperties, _("Properties"), true );
	m_Panel3D = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerMain3D;
	bSizerMain3D = new wxBoxSizer( wxVERTICAL );
	
	m_staticText3Dname = new wxStaticText( m_Panel3D, wxID_ANY, _("3D Shape Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3Dname->Wrap( -1 );
	bSizerMain3D->Add( m_staticText3Dname, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_3D_ShapeNameListBox = new wxListBox( m_Panel3D, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE ); 
	bSizerMain3D->Add( m_3D_ShapeNameListBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextDefault3DPath = new wxStaticText( m_Panel3D, wxID_ANY, _("Default Path (from KISYS3DMOD environment variable)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDefault3DPath->Wrap( -1 );
	bSizerMain3D->Add( m_staticTextDefault3DPath, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrl3DDefaultPath = new wxTextCtrl( m_Panel3D, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizerMain3D->Add( m_textCtrl3DDefaultPath, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxBoxSizer* bLowerSizer3D;
	bLowerSizer3D = new wxBoxSizer( wxHORIZONTAL );
	
	m_Sizer3DValues = new wxStaticBoxSizer( new wxStaticBox( m_Panel3D, wxID_ANY, _("3D Scale and Position") ), wxVERTICAL );
	
	m_bSizerShapeScale = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextShapeScale = new wxStaticText( m_Panel3D, wxID_ANY, _("Shape Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShapeScale->Wrap( -1 );
	m_bSizerShapeScale->Add( m_staticTextShapeScale, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	m_Sizer3DValues->Add( m_bSizerShapeScale, 0, wxEXPAND, 5 );
	
	m_bSizerShapeOffset = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextShapeOffset = new wxStaticText( m_Panel3D, wxID_ANY, _("Shape Offset (inch):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShapeOffset->Wrap( -1 );
	m_bSizerShapeOffset->Add( m_staticTextShapeOffset, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	m_Sizer3DValues->Add( m_bSizerShapeOffset, 0, wxEXPAND, 5 );
	
	m_bSizerShapeRotation = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextShapeRotation = new wxStaticText( m_Panel3D, wxID_ANY, _("Shape Rotation (degrees):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShapeRotation->Wrap( -1 );
	m_bSizerShapeRotation->Add( m_staticTextShapeRotation, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	m_Sizer3DValues->Add( m_bSizerShapeRotation, 0, wxEXPAND, 5 );
	
	
	bLowerSizer3D->Add( m_Sizer3DValues, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAdd = new wxButton( m_Panel3D, ID_ADD_3D_SHAPE, _("Add 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonAdd, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_buttonRemove = new wxButton( m_Panel3D, ID_REMOVE_3D_SHAPE, _("Remove 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonRemove, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_buttonEdit = new wxButton( m_Panel3D, wxID_ANY, _("Edit Filename"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonEdit, 0, wxALL|wxEXPAND, 5 );
	
	
	bLowerSizer3D->Add( bSizer3DButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain3D->Add( bLowerSizer3D, 0, wxEXPAND, 5 );
	
	
	m_Panel3D->SetSizer( bSizerMain3D );
	m_Panel3D->Layout();
	bSizerMain3D->Fit( m_Panel3D );
	m_NoteBook->AddPage( m_Panel3D, _("3D settings"), false );
	
	m_GeneralBoxSizer->Add( m_NoteBook, 1, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();
	
	m_GeneralBoxSizer->Add( m_sdbSizerStdButtons, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	m_GeneralBoxSizer->Fit( this );
	
	// Connect Events
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditValue ), NULL, this );
	m_OrientCtrl->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ModuleOrientEvent ), NULL, this );
	m_buttonExchange->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ExchangeModule ), NULL, this );
	m_buttonModuleEditor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::GotoModuleEditor ), NULL, this );
	m_3D_ShapeNameListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Edit3DShapeFilename ), NULL, this );
	m_sdbSizerStdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_MODULE_BOARD_EDITOR_BASE::~DIALOG_MODULE_BOARD_EDITOR_BASE()
{
	// Disconnect Events
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditValue ), NULL, this );
	m_OrientCtrl->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ModuleOrientEvent ), NULL, this );
	m_buttonExchange->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ExchangeModule ), NULL, this );
	m_buttonModuleEditor->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::GotoModuleEditor ), NULL, this );
	m_3D_ShapeNameListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Edit3DShapeFilename ), NULL, this );
	m_sdbSizerStdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnOkClick ), NULL, this );
	
}
