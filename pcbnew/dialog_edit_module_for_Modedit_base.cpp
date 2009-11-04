///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_module_for_Modedit_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODULE_MODULE_EDITOR_BASE::DIALOG_MODULE_MODULE_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelProperties = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelPropertiesBoxSizer;
	m_PanelPropertiesBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* PropLeftSizer;
	PropLeftSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Fields:") ), wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerDoc;
	sbSizerDoc = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Doc") ), wxHORIZONTAL );
	
	sbSizerDoc->SetMinSize( wxSize( 300,-1 ) ); 
	m_DocCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerDoc->Add( m_DocCtrl, 1, 0, 5 );
	
	PropLeftSizer->Add( sbSizerDoc, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerKeysW;
	sbSizerKeysW = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Keywords") ), wxHORIZONTAL );
	
	m_KeywordCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerKeysW->Add( m_KeywordCtrl, 1, 0, 5 );
	
	PropLeftSizer->Add( sbSizerKeysW, 0, wxEXPAND, 5 );
	
	
	PropLeftSizer->Add( 0, 20, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizerRef;
	sbSizerRef = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Reference") ), wxHORIZONTAL );
	
	m_ReferenceCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	sbSizerRef->Add( m_ReferenceCtrl, 1, 0, 5 );
	
	m_button4 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sbSizerRef->Add( m_button4, 0, 0, 5 );
	
	PropLeftSizer->Add( sbSizerRef, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerValue;
	sbSizerValue = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Value") ), wxHORIZONTAL );
	
	m_ValueCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	sbSizerValue->Add( m_ValueCtrl, 1, 0, 5 );
	
	m_button5 = new wxButton( m_PanelProperties, wxID_ANY, _("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	sbSizerValue->Add( m_button5, 0, 0, 5 );
	
	PropLeftSizer->Add( sbSizerValue, 0, wxEXPAND, 5 );
	
	m_PanelPropertiesBoxSizer->Add( PropLeftSizer, 1, wxEXPAND, 5 );
	
	m_PropRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_AttributsCtrlChoices[] = { _("Normal"), _("Normal+Insert"), _("Virtual") };
	int m_AttributsCtrlNChoices = sizeof( m_AttributsCtrlChoices ) / sizeof( wxString );
	m_AttributsCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Attributs:"), wxDefaultPosition, wxDefaultSize, m_AttributsCtrlNChoices, m_AttributsCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AttributsCtrl->SetSelection( 0 );
	m_PropRightSizer->Add( m_AttributsCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_AutoPlaceCtrlChoices[] = { _("Free"), _("Locked") };
	int m_AutoPlaceCtrlNChoices = sizeof( m_AutoPlaceCtrlChoices ) / sizeof( wxString );
	m_AutoPlaceCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Move and Auto Place"), wxDefaultPosition, wxDefaultSize, m_AutoPlaceCtrlNChoices, m_AutoPlaceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_AutoPlaceCtrl->SetSelection( 0 );
	m_PropRightSizer->Add( m_AutoPlaceCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerAutoplace;
	sbSizerAutoplace = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Auto Move and Place") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizerRot90;
	bSizerRot90 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText11 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 90 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	bSizerRot90->Add( m_staticText11, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CostRot90Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerRot90->Add( m_CostRot90Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizerAutoplace->Add( bSizerRot90, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRot180;
	bSizerRot180 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText12 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 180 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizerRot180->Add( m_staticText12, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CostRot180Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerRot180->Add( m_CostRot180Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizerAutoplace->Add( bSizerRot180, 1, wxEXPAND, 5 );
	
	m_PropRightSizer->Add( sbSizerAutoplace, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Masks clearances local values:") ), wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( m_PanelProperties, wxID_ANY, _("Set these values to 0 to use global values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	m_staticTextInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sbSizer8->Add( m_staticTextInfo, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_MaskClearanceTitle = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder mask clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	fgSizer1->Add( m_MaskClearanceTitle, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_SolderMaskMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginCtrl->SetToolTip( _("This is the global clearance between pads and the solder mask\nThis value can be superseded by a pad local value.") );
	
	fgSizer1->Add( m_SolderMaskMarginCtrl, 0, wxALL, 5 );
	
	m_SolderMaskMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderMaskMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderMaskMarginUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextSolderPaste = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder paste clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSolderPaste->Wrap( -1 );
	fgSizer1->Add( m_staticTextSolderPaste, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SolderPasteMarginCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginCtrl->SetToolTip( _("This is the global clearance between pads and the solder paste\nThis value can be superseded by a pad local values.\nThe final clearance value is the sum of this value and the clearance value ratio\nA negative value means a smaller mask size than pad size") );
	
	fgSizer1->Add( m_SolderPasteMarginCtrl, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SolderPasteMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("Inch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderPasteMarginUnits, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextRatio = new wxStaticText( m_PanelProperties, wxID_ANY, _("Solder mask ratio clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRatio->Wrap( -1 );
	fgSizer1->Add( m_staticTextRatio, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SolderPasteMarginRatioCtrl = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteMarginRatioCtrl->SetToolTip( _("This is the global clearance ratio in per cent between pads and the solder paste\nA value of 10 means the clearance value is 10% of the pad size\nThis value can be superseded by a pad local value.\nThe final clearance value is the sum of this value and the clearance value\nA negative value means a smaller mask size than pad size") );
	
	fgSizer1->Add( m_SolderPasteMarginRatioCtrl, 0, wxALL, 5 );
	
	m_SolderPasteRatioMarginUnits = new wxStaticText( m_PanelProperties, wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SolderPasteRatioMarginUnits->Wrap( -1 );
	fgSizer1->Add( m_SolderPasteRatioMarginUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer8->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	m_PropRightSizer->Add( sbSizer8, 1, wxEXPAND, 5 );
	
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
	bSizerMain3D->Add( m_3D_ShapeNameListBox, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bLowerSizer3D;
	bLowerSizer3D = new wxBoxSizer( wxHORIZONTAL );
	
	m_Sizer3DValues = new wxStaticBoxSizer( new wxStaticBox( m_Panel3D, wxID_ANY, _("3D Scale and Pos") ), wxVERTICAL );
	
	bLowerSizer3D->Add( m_Sizer3DValues, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3DButtons;
	bSizer3DButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonBrowse = new wxButton( m_Panel3D, ID_BROWSE_3D_LIB, _("Browse Shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonBrowse, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonAdd = new wxButton( m_Panel3D, ID_ADD_3D_SHAPE, _("Add 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonAdd, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonRemove = new wxButton( m_Panel3D, ID_REMOVE_3D_SHAPE, _("Remove 3D Shape"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3DButtons->Add( m_buttonRemove, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bLowerSizer3D->Add( bSizer3DButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerMain3D->Add( bLowerSizer3D, 1, wxEXPAND, 5 );
	
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
	m_GeneralBoxSizer->Add( m_sdbSizerStdButtons, 0, wxEXPAND|wxALIGN_RIGHT, 5 );
	
	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	
	// Connect Events
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditValue ), NULL, this );
	m_3D_ShapeNameListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Browse3DLib ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_sdbSizerStdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnOkClick ), NULL, this );
}

DIALOG_MODULE_MODULE_EDITOR_BASE::~DIALOG_MODULE_MODULE_EDITOR_BASE()
{
	// Disconnect Events
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnEditValue ), NULL, this );
	m_3D_ShapeNameListBox->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Browse3DLib ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_sdbSizerStdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_MODULE_EDITOR_BASE::OnOkClick ), NULL, this );
}
