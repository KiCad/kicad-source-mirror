///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 28 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_eeschema_options_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_EESCHEMA_OPTIONS_BASE, DIALOG_SHIM )
	EVT_SIZE( DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnSize )
	EVT_CHOICE( wxID_ANY, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnChooseUnits )
	EVT_CHECKBOX( xwID_ANY, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnMiddleBtnPanEnbl )
	EVT_BUTTON( wxID_ADD_FIELD, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnAddButtonClick )
	EVT_BUTTON( wxID_DELETE_FIELD, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnDeleteButtonClick )
	EVT_BUTTON( ID_IMP_EXP, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnImpExpClick )
END_EVENT_TABLE()

DIALOG_EESCHEMA_OPTIONS_BASE::DIALOG_EESCHEMA_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panel5 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer82;
	bSizer82 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer32;
	fgSizer32 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer32->AddGrowableCol( 0 );
	fgSizer32->AddGrowableCol( 1 );
	fgSizer32->AddGrowableCol( 2 );
	fgSizer32->SetFlexibleDirection( wxBOTH );
	fgSizer32->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText3 = new wxStaticText( m_panel5, wxID_ANY, _("&Grid size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer32->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	wxArrayString m_choiceGridSizeChoices;
	m_choiceGridSize = new wxChoice( m_panel5, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceGridSizeChoices, 0 );
	m_choiceGridSize->SetSelection( 0 );
	fgSizer32->Add( m_choiceGridSize, 0, wxEXPAND|wxALL, 3 );
	
	m_staticGridUnits = new wxStaticText( m_panel5, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticGridUnits->Wrap( -1 );
	fgSizer32->Add( m_staticGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText51 = new wxStaticText( m_panel5, wxID_ANY, _("&Bus thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizer32->Add( m_staticText51, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinBusWidth = new wxSpinCtrl( m_panel5, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 1 );
	fgSizer32->Add( m_spinBusWidth, 0, wxALL|wxEXPAND, 3 );
	
	m_staticBusWidthUnits = new wxStaticText( m_panel5, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticBusWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_staticBusWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText5 = new wxStaticText( m_panel5, wxID_ANY, _("&Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer32->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinLineWidth = new wxSpinCtrl( m_panel5, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 1 );
	fgSizer32->Add( m_spinLineWidth, 0, wxALL|wxEXPAND, 3 );
	
	m_staticLineWidthUnits = new wxStaticText( m_panel5, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLineWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_staticLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText26 = new wxStaticText( m_panel5, wxID_ANY, _("&Part ID notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer32->Add( m_staticText26, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );
	
	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( m_panel5, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	fgSizer32->Add( m_choiceSeparatorRefId, 0, wxALL|wxEXPAND, 3 );
	
	
	fgSizer32->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer82->Add( fgSizer32, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer92;
	bSizer92 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline3 = new wxStaticLine( m_panel5, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer92->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	m_checkShowGrid = new wxCheckBox( m_panel5, wxID_ANY, _("&Show grid"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer92->Add( m_checkShowGrid, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkHVOrientation = new wxCheckBox( m_panel5, wxID_ANY, _("&Restrict buses and wires to H and V orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer92->Add( m_checkHVOrientation, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkShowHiddenPins = new wxCheckBox( m_panel5, wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer92->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkPageLimits = new wxCheckBox( m_panel5, wxID_ANY, _("Show page limi&ts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkPageLimits->SetValue(true); 
	bSizer92->Add( m_checkPageLimits, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 3 );
	
	
	bSizer82->Add( bSizer92, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panel5->SetSizer( bSizer82 );
	m_panel5->Layout();
	bSizer82->Fit( m_panel5 );
	m_notebook->AddPage( m_panel5, _("Display"), false );
	m_panel3 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableCol( 2 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( m_panel3, wxID_ANY, _("&Measurement units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	wxArrayString m_choiceUnitsChoices;
	m_choiceUnits = new wxChoice( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer3->Add( m_choiceUnits, 0, wxALL|wxEXPAND, 3 );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticText9 = new wxStaticText( m_panel3, wxID_ANY, _("&Horizontal pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer3->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatHorizontal = new wxSpinCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -5000, 5000, 0 );
	fgSizer3->Add( m_spinRepeatHorizontal, 0, wxALL|wxEXPAND, 3 );
	
	m_staticRepeatXUnits = new wxStaticText( m_panel3, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatXUnits->Wrap( -1 );
	fgSizer3->Add( m_staticRepeatXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText12 = new wxStaticText( m_panel3, wxID_ANY, _("&Vertical pitch of repeated items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer3->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatVertical = new wxSpinCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -5000, 5000, 100 );
	fgSizer3->Add( m_spinRepeatVertical, 0, wxALL|wxEXPAND, 3 );
	
	m_staticRepeatYUnits = new wxStaticText( m_panel3, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatYUnits->Wrap( -1 );
	fgSizer3->Add( m_staticRepeatYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText16 = new wxStaticText( m_panel3, wxID_ANY, _("&Increment of repeated labels:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer3->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatLabel = new wxSpinCtrl( m_panel3, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -10, 10, 1 );
	fgSizer3->Add( m_spinRepeatLabel, 0, wxALL|wxEXPAND, 3 );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticText7 = new wxStaticText( m_panel3, wxID_ANY, _("Def&ault text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer3->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinTextSize = new wxSpinCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 1000, 0 );
	fgSizer3->Add( m_spinTextSize, 0, wxALL|wxEXPAND, 3 );
	
	m_staticTextSizeUnits = new wxStaticText( m_panel3, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer3->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_stMaxUndoItems = new wxStaticText( m_panel3, wxID_ANY, _("Ma&ximum undo items:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxUndoItems->Wrap( -1 );
	fgSizer3->Add( m_stMaxUndoItems, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_spinMaxUndoItems = new wxSpinCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65536, 0 );
	fgSizer3->Add( m_spinMaxUndoItems, 0, wxALL|wxEXPAND, 3 );
	
	m_staticText22 = new wxStaticText( m_panel3, wxID_ANY, _("(0 = unlimited)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer3->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3 );
	
	m_staticText221 = new wxStaticText( m_panel3, wxID_ANY, _("&Auto-save time interval"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText221->Wrap( -1 );
	fgSizer3->Add( m_staticText221, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinAutoSaveInterval = new wxSpinCtrl( m_panel3, ID_M_SPINAUTOSAVEINTERVAL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 10 );
	fgSizer3->Add( m_spinAutoSaveInterval, 0, wxALL|wxEXPAND, 3 );
	
	m_staticText23 = new wxStaticText( m_panel3, wxID_ANY, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer3->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	
	bSizer8->Add( fgSizer3, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline2 = new wxStaticLine( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer9->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_checkAutoplaceFields = new wxCheckBox( m_panel3, wxID_ANY, _("A&utomatically place component fields"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_checkAutoplaceFields, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 3 );
	
	m_checkAutoplaceJustify = new wxCheckBox( m_panel3, wxID_ANY, _("A&llow field autoplace to change justification"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_checkAutoplaceJustify, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 3 );
	
	m_checkAutoplaceAlign = new wxCheckBox( m_panel3, wxID_ANY, _("Al&ways align autoplaced fields to the 50 mil grid"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer9->Add( m_checkAutoplaceAlign, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 3 );
	
	
	bSizer8->Add( bSizer9, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panel3->SetSizer( bSizer8 );
	m_panel3->Layout();
	bSizer8->Fit( m_panel3 );
	m_notebook->AddPage( m_panel3, _("Editing"), false );
	m_controlsPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer31->AddGrowableCol( 0 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->AddGrowableCol( 2 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	bSizer81->Add( fgSizer31, 0, wxALL|wxEXPAND, 5 );
	
	m_controlsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText20 = new wxStaticText( m_controlsPanel, wxID_ANY, _("Hotkeys:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	m_controlsSizer->Add( m_staticText20, 0, wxALL, 5 );
	
	m_checkEnableZoomCenter = new wxCheckBox( m_controlsPanel, wxID_ANY, _("Cen&ter and warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnableZoomCenter->SetToolTip( _("Keep the cursor at its current location when zooming") );
	
	m_controlsSizer->Add( m_checkEnableZoomCenter, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkEnableMiddleButtonPan = new wxCheckBox( m_controlsPanel, xwID_ANY, _("&Use middle mouse button to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnableMiddleButtonPan->SetToolTip( _("Use middle mouse button dragging to pan") );
	
	m_controlsSizer->Add( m_checkEnableMiddleButtonPan, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkMiddleButtonPanLimited = new wxCheckBox( m_controlsPanel, wxID_ANY, _("&Limit panning to scroll size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkMiddleButtonPanLimited->SetToolTip( _("Middle mouse button panning limited by current scrollbar size") );
	
	m_controlsSizer->Add( m_checkMiddleButtonPanLimited, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkAutoPan = new wxCheckBox( m_controlsPanel, wxID_ANY, _("&Pan while moving object"), wxDefaultPosition, wxDefaultSize, 0 );
	m_controlsSizer->Add( m_checkAutoPan, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	
	bSizer81->Add( m_controlsSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	m_controlsPanel->SetSizer( bSizer81 );
	m_controlsPanel->Layout();
	bSizer81->Fit( m_controlsPanel );
	m_notebook->AddPage( m_controlsPanel, _("Co&ntrols"), false );
	m_panel2 = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel2->SetToolTip( _("User defined field names for schematic components. ") );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_fieldGrid = new wxGrid( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_fieldGrid->CreateGrid( 0, 3 );
	m_fieldGrid->EnableEditing( true );
	m_fieldGrid->EnableGridLines( true );
	m_fieldGrid->EnableDragGridSize( false );
	m_fieldGrid->SetMargins( 0, 0 );
	
	// Columns
	m_fieldGrid->SetColSize( 0, 150 );
	m_fieldGrid->SetColSize( 1, 150 );
	m_fieldGrid->SetColSize( 2, 75 );
	m_fieldGrid->EnableDragColMove( false );
	m_fieldGrid->EnableDragColSize( true );
	m_fieldGrid->SetColLabelSize( 30 );
	m_fieldGrid->SetColLabelValue( 0, _("Name") );
	m_fieldGrid->SetColLabelValue( 1, _("Default Value") );
	m_fieldGrid->SetColLabelValue( 2, _("Visible") );
	m_fieldGrid->SetColLabelValue( 3, _("Name") );
	m_fieldGrid->SetColLabelValue( 4, wxEmptyString );
	m_fieldGrid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_fieldGrid->EnableDragRowSize( true );
	m_fieldGrid->SetRowLabelSize( 80 );
	m_fieldGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_fieldGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer11->Add( m_fieldGrid, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer11, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	addFieldButton = new wxButton( m_panel2, wxID_ADD_FIELD, _("&Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( addFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	deleteFieldButton = new wxButton( m_panel2, wxID_DELETE_FIELD, _("De&lete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( deleteFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer10->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer10, 0, wxEXPAND, 5 );
	
	
	m_panel2->SetSizer( bSizer6 );
	m_panel2->Layout();
	bSizer6->Fit( m_panel2 );
	m_notebook->AddPage( m_panel2, _("Default &Fields"), true );
	
	bOptionsSizer->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnImpExp = new wxButton( this, ID_IMP_EXP, _("Import/Export..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_btnImpExp, 0, wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizer12->Add( m_sdbSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	bOptionsSizer->Add( bSizer12, 0, wxEXPAND, 5 );
	
	
	mainSizer->Add( bOptionsSizer, 1, wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_EESCHEMA_OPTIONS_BASE::~DIALOG_EESCHEMA_OPTIONS_BASE()
{
}
