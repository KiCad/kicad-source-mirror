///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_module_for_Modedit_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODULE_MODULE_EDITOR_BASE::DIALOG_MODULE_MODULE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelProperties = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelPropertiesBoxSizer;
	m_PanelPropertiesBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* PropLeftSizer;
	PropLeftSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Fields") ), wxVERTICAL );
	
	m_staticTextDoc = new wxStaticText( m_PanelProperties, wxID_ANY, _("Doc"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDoc->Wrap( -1 );
	PropLeftSizer->Add( m_staticTextDoc, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DocCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DocCtrl->SetMaxLength( 0 ); 
	PropLeftSizer->Add( m_DocCtrl, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_staticTextKeywords = new wxStaticText( m_PanelProperties, wxID_ANY, _("Keywords"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKeywords->Wrap( -1 );
	PropLeftSizer->Add( m_staticTextKeywords, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_KeywordCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_KeywordCtrl->SetMaxLength( 0 ); 
	PropLeftSizer->Add( m_KeywordCtrl, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_staticTextRef = new wxStaticText( m_PanelProperties, wxID_ANY, _("Reference"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	PropLeftSizer->Add( m_staticTextRef, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerRef;
	bSizerRef = new wxBoxSizer( wxHORIZONTAL );
	
	m_ReferenceCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_ReferenceCtrl->SetMaxLength( 0 ); 
	bSizerRef->Add( m_ReferenceCtrl, 1, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_button4 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerRef->Add( m_button4, 0, wxBOTTOM|wxRIGHT, 5 );
	
	
	PropLeftSizer->Add( bSizerRef, 0, wxEXPAND, 5 );
	
	m_staticTextVal = new wxStaticText( m_PanelProperties, wxID_ANY, _("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVal->Wrap( -1 );
	PropLeftSizer->Add( m_staticTextVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerVal;
	bSizerVal = new wxBoxSizer( wxHORIZONTAL );
	
	m_ValueCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_ValueCtrl->SetMaxLength( 0 ); 
	bSizerVal->Add( m_ValueCtrl, 1, wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_button5 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerVal->Add( m_button5, 0, wxBOTTOM|wxRIGHT, 5 );
	
	
	PropLeftSizer->Add( bSizerVal, 0, wxEXPAND, 5 );
	
	
	PropLeftSizer->Add( 0, 20, 0, 0, 5 );
	
	m_staticTextFp = new wxStaticText( m_PanelProperties, wxID_ANY, _("Footprint Name in Library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFp->Wrap( -1 );
	PropLeftSizer->Add( m_staticTextFp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FootprintNameCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FootprintNameCtrl->SetMaxLength( 0 ); 
	PropLeftSizer->Add( m_FootprintNameCtrl, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	
	PropLeftSizer->Add( 0, 0, 0, wxEXPAND, 5 );
	
	
	m_PanelPropertiesBoxSizer->Add( PropLeftSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_PropRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerAttrib;
	bSizerAttrib = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_AttributsCtrlChoices[] = { _("Normal"), _("Normal+Insert"), _("Virtual") };
	int m_AttributsCtrlNChoices = sizeof( m_AttributsCtrlChoices ) / sizeof( wxString );
	m_AttributsCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Attributes"), wxDefaultPosition, wxDefaultSize, m_AttributsCtrlNChoices, m_AttributsCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AttributsCtrl->SetSelection( 0 );
	bSizerAttrib->Add( m_AttributsCtrl, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_AutoPlaceCtrlChoices[] = { _("Free"), _("Locked") };
	int m_AutoPlaceCtrlNChoices = sizeof( m_AutoPlaceCtrlChoices ) / sizeof( wxString );
	m_AutoPlaceCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Move and Place"), wxDefaultPosition, wxDefaultSize, m_AutoPlaceCtrlNChoices, m_AutoPlaceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AutoPlaceCtrl->SetSelection( 0 );
	bSizerAttrib->Add( m_AutoPlaceCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( bSizerAttrib, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerAutoplace;
	sbSizerAutoplace = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Auto Place") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizerRot90;
	bSizerRot90 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText11 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 90 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	bSizerRot90->Add( m_staticText11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_CostRot90Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxSize( -1,-1 ), wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerRot90->Add( m_CostRot90Ctrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerAutoplace->Add( bSizerRot90, 1, 0, 5 );
	
	wxBoxSizer* bSizerRot180;
	bSizerRot180 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText12 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 180 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizerRot180->Add( m_staticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_CostRot180Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerRot180->Add( m_CostRot180Ctrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerAutoplace->Add( bSizerRot180, 1, 0, 5 );
	
	
	m_PropRightSizer->Add( sbSizerAutoplace, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Local Clearance Values") ), wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( m_PanelProperties, wxID_ANY, _("Set clearances to 0 to use global values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sbSizer8->Add( m_staticTextInfo, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 5, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextNetClearance = new wxStaticText( m_PanelProperties, wxID_ANY, _("Pad clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetClearance->Wrap( -1 );
	fgSizer1->Add( m_staticTextNetClearance, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_NetClearanceValueCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceValueCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_NetClearanceValueCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_NetClearanceUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetClearanceUnits->Wrap( -1 );
	fgSizer1->Add( m_NetClearanceUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline1 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline1, 0, wxEXPAND, 5 );
	
	m_staticline2 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline2, 0, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( m_PanelProperties, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline3, 0, wxEXPAND, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	m_MaskClearanceTitle->SetToolTip( _("This is the local clearance between pads and the solder mask\nfor this footprint\nThis value can be superseded by a pad local value.\nIf 0, the global value is used") );
	
	fgSizer1->Add( m_MaskClearanceTitle, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_SolderMaskMarginCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderMaskMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	m_staticTextSolderPaste->SetToolTip( _("This is the local clearance between pads and the solder paste\nfor this footprint.\nThis value can be superseded by a pad local values.\nThe final clearance value is the sum of this value and the clearance value ratio\nA negative value means a smaller mask size than pad size") );
	
	fgSizer1->Add( m_staticTextSolderPaste, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_SolderPasteMarginCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderPasteMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextRatio = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder paste ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	m_staticTextRatio->SetToolTip( _("This is the local clearance ratio in per cent between pads and the solder paste\nfor this footprint.\nA value of 10 means the clearance value is 10 per cent of the pad size\nThis value can be superseded by a pad local value.\nThe final clearance value is the sum of this value and the clearance value\nA negative value means a smaller mask size than pad size.") );
	
	fgSizer1->Add( m_staticTextRatio, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetMaxLength( 0 ); 
	fgSizer1->Add( m_SolderPasteMarginRatioCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderPasteRatioMarginUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	sbSizer8->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( sbSizer8, 0, wxEXPAND|wxALL, 5 );
	
	
	m_PanelPropertiesBoxSizer->Add( m_PropRightSizer, 0, 0, 5 );
	
	
	m_PanelProperties->SetSizer( m_PanelPropertiesBoxSizer );
	m_PanelProperties->Layout();
	m_PanelPropertiesBoxSizer->Fit( m_PanelProperties );
	m_NoteBook->AddPage( m_PanelProperties, _("Properties"), true );
	m_Panel3D = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerMain3D;
	bSizerMain3D = new wxBoxSizer( wxVERTICAL );
	
	m_staticText3Dname = new wxStaticText( m_Panel3D, wxID_ANY, _("3D Shape Names"), wxDefaultPosition, wxDefaultSize, 0 );
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
	
	
	bLowerSizer3D->Add( m_Sizer3DValues, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonBrowse = new wxButton( m_Panel3D, ID_BROWSE_3D_LIB, _("Add 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonBrowse, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonRemove = new wxButton( m_Panel3D, ID_REMOVE_3D_SHAPE, _("Remove 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonRemove, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_buttonEdit = new wxButton( m_Panel3D, wxID_ANY, _("Edit Filename"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonEdit, 0, wxALL|wxEXPAND, 5 );
	
	
	bLowerSizer3D->Add( bSizer3DButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain3D->Add( bLowerSizer3D, 0, wxALL|wxEXPAND, 5 );
	
	
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
	
	m_GeneralBoxSizer->Add( m_sdbSizerStdButtons, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	
	// Connect Events
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditValue ), NULL, this );
	m_3D_ShapeNameListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_buttonEdit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Edit3DShapeFilename ), NULL, this );
	m_sdbSizerStdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_MODULE_MODULE_EDITOR_BASE::~DIALOG_MODULE_MODULE_EDITOR_BASE()
{
	// Disconnect Events
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditValue ), NULL, this );
	m_3D_ShapeNameListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_buttonEdit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Edit3DShapeFilename ), NULL, this );
	m_sdbSizerStdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnOkClick ), NULL, this );
	
}
