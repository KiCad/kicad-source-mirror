///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_module_for_BoardEditor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODULE_BOARD_EDITOR_BASE::DIALOG_MODULE_BOARD_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_NoteBook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelProperties = new wxPanel( m_NoteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* m_PanelPropertiesBoxSizer;
	m_PanelPropertiesBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* PropLeftSizer;
	PropLeftSizer = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Fields:") ), wxVERTICAL );
	
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
	
	wxString m_LayerCtrlChoices[] = { _("Top"), _("Bottom") };
	int m_LayerCtrlNChoices = sizeof( m_LayerCtrlChoices ) / sizeof( wxString );
	m_LayerCtrl = new wxRadioBox( m_PanelProperties, wxID_ANY, _("Side Select"), wxDefaultPosition, wxDefaultSize, m_LayerCtrlNChoices, m_LayerCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_LayerCtrl->SetSelection( 0 );
	PropLeftSizer->Add( m_LayerCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerOrientation;
	sbSizerOrientation = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Orientation") ), wxVERTICAL );
	
	wxString m_OrientCtrlChoices[] = { _("Normal"), _("+ 90.0"), _("- 90.0"), _("180.0"), _("User") };
	int m_OrientCtrlNChoices = sizeof( m_OrientCtrlChoices ) / sizeof( wxString );
	m_OrientCtrl = new wxRadioBox( m_PanelProperties, ID_LISTBOX_ORIENT_SELECT, _("Orientation"), wxDefaultPosition, wxDefaultSize, m_OrientCtrlNChoices, m_OrientCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientCtrl->SetSelection( 0 );
	sbSizerOrientation->Add( m_OrientCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText4 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Orientation (in 0.1 degrees)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	sbSizerOrientation->Add( m_staticText4, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OrientValue = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerOrientation->Add( m_OrientValue, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	PropLeftSizer->Add( sbSizerOrientation, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerPosition;
	sbSizerPosition = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Position") ), wxVERTICAL );
	
	XPositionStatic = new wxStaticText( m_PanelProperties, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
	XPositionStatic->Wrap( -1 );
	sbSizerPosition->Add( XPositionStatic, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ModPositionX = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPosition->Add( m_ModPositionX, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	YPositionStatic = new wxStaticText( m_PanelProperties, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
	YPositionStatic->Wrap( -1 );
	sbSizerPosition->Add( YPositionStatic, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ModPositionY = new wxTextCtrl( m_PanelProperties, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPosition->Add( m_ModPositionY, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	PropLeftSizer->Add( sbSizerPosition, 0, wxEXPAND, 5 );
	
	m_PanelPropertiesBoxSizer->Add( PropLeftSizer, 1, wxEXPAND, 5 );
	
	m_PropRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonExchange = new wxButton( m_PanelProperties, ID_MODULE_PROPERTIES_EXCHANGE, _("Change Module(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PropRightSizer->Add( m_buttonExchange, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonModuleEditor = new wxButton( m_PanelProperties, ID_GOTO_MODULE_EDITOR, _("Module Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PropRightSizer->Add( m_buttonModuleEditor, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	m_PropRightSizer->Add( 0, 20, 0, 0, 5 );
	
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
	sbSizerAutoplace = new wxStaticBoxSizer( new wxStaticBox( m_PanelProperties, wxID_ANY, _("Auto Move and Place") ), wxVERTICAL );
	
	m_staticText11 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 90 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	sbSizerAutoplace->Add( m_staticText11, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CostRot90Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	sbSizerAutoplace->Add( m_CostRot90Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText12 = new wxStaticText( m_PanelProperties, wxID_ANY, _("Rotation 180 degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	sbSizerAutoplace->Add( m_staticText12, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_CostRot180Ctrl = new wxSlider( m_PanelProperties, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	sbSizerAutoplace->Add( m_CostRot180Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_PropRightSizer->Add( sbSizerAutoplace, 1, wxEXPAND, 5 );
	
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
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditReference ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnEditValue ), NULL, this );
	m_OrientCtrl->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ModuleOrientEvent ), NULL, this );
	m_buttonExchange->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::ExchangeModule ), NULL, this );
	m_buttonModuleEditor->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::GotoModuleEditor ), NULL, this );
	m_3D_ShapeNameListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::On3DShapeNameSelected ), NULL, this );
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Browse3DLib ), NULL, this );
	m_buttonAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Remove3DShape ), NULL, this );
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
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Browse3DLib ), NULL, this );
	m_buttonAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Add3DShape ), NULL, this );
	m_buttonRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::Remove3DShape ), NULL, this );
	m_sdbSizerStdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerStdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODULE_BOARD_EDITOR_BASE::OnOkClick ), NULL, this );
}
