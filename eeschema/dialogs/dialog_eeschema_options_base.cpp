///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
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
	EVT_LIST_ITEM_DESELECTED( wxID_ANY, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnTemplateFieldDeselected )
	EVT_LIST_ITEM_SELECTED( wxID_ANY, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnTemplateFieldSelected )
	EVT_BUTTON( wxID_ADD_FIELD, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnAddButtonClick )
	EVT_BUTTON( wxID_DELETE_FIELD, DIALOG_EESCHEMA_OPTIONS_BASE::_wxFB_OnDeleteButtonClick )
END_EVENT_TABLE()

DIALOG_EESCHEMA_OPTIONS_BASE::DIALOG_EESCHEMA_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bOptionsSizer;
	bOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_notebook1->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	m_panel1 = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableCol( 2 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( m_panel1, wxID_ANY, _("Measurement &units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	wxArrayString m_choiceUnitsChoices;
	m_choiceUnits = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	fgSizer1->Add( m_choiceUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	m_staticText3 = new wxStaticText( m_panel1, wxID_ANY, _("&Grid size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	wxArrayString m_choiceGridSizeChoices;
	m_choiceGridSize = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceGridSizeChoices, 0 );
	m_choiceGridSize->SetSelection( 0 );
	fgSizer1->Add( m_choiceGridSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 3 );
	
	m_staticGridUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticGridUnits->Wrap( -1 );
	fgSizer1->Add( m_staticGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText51 = new wxStaticText( m_panel1, wxID_ANY, _("Default &bus width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizer1->Add( m_staticText51, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinBusWidth = new wxSpinCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 1 );
	fgSizer1->Add( m_spinBusWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticBusWidthUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticBusWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticBusWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText5 = new wxStaticText( m_panel1, wxID_ANY, _("Default &line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinLineWidth = new wxSpinCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 1, 100, 1 );
	fgSizer1->Add( m_spinLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticLineWidthUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText7 = new wxStaticText( m_panel1, wxID_ANY, _("Default text &size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer1->Add( m_staticText7, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinTextSize = new wxSpinCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 1000, 0 );
	fgSizer1->Add( m_spinTextSize, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticTextSizeUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText9 = new wxStaticText( m_panel1, wxID_ANY, _("Repeat draw item &horizontal displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer1->Add( m_staticText9, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatHorizontal = new wxSpinCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -5000, 5000, 0 );
	fgSizer1->Add( m_spinRepeatHorizontal, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticRepeatXUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatXUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText12 = new wxStaticText( m_panel1, wxID_ANY, _("Repeat draw item &vertical displacement:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer1->Add( m_staticText12, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatVertical = new wxSpinCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, -5000, 5000, 100 );
	fgSizer1->Add( m_spinRepeatVertical, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticRepeatYUnits = new wxStaticText( m_panel1, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticRepeatYUnits->Wrap( -1 );
	fgSizer1->Add( m_staticRepeatYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText16 = new wxStaticText( m_panel1, wxID_ANY, _("&Repeat label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer1->Add( m_staticText16, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinRepeatLabel = new wxSpinCtrl( m_panel1, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP, 0, 10, 1 );
	fgSizer1->Add( m_spinRepeatLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );
	
	m_staticText221 = new wxStaticText( m_panel1, wxID_ANY, _("Auto save &time interval:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText221->Wrap( -1 );
	fgSizer1->Add( m_staticText221, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_spinAutoSaveInterval = new wxSpinCtrl( m_panel1, ID_M_SPINAUTOSAVEINTERVAL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000, 10 );
	fgSizer1->Add( m_spinAutoSaveInterval, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticText23 = new wxStaticText( m_panel1, wxID_ANY, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer1->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );
	
	m_staticText26 = new wxStaticText( m_panel1, wxID_ANY, _("Part id notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer1->Add( m_staticText26, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	fgSizer1->Add( m_choiceSeparatorRefId, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer3->Add( fgSizer1, 0, wxALIGN_CENTER|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline1 = new wxStaticLine( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer2->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_checkShowGrid = new wxCheckBox( m_panel1, wxID_ANY, _("Show gr&id"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkShowGrid, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkShowHiddenPins = new wxCheckBox( m_panel1, wxID_ANY, _("Show hi&dden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkEnableZoomNoCenter = new wxCheckBox( m_panel1, wxID_ANY, _("Do not center and &warp cursor on zoom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnableZoomNoCenter->SetToolTip( _("Keep the cursor at its current location when zooming") );
	
	bSizer2->Add( m_checkEnableZoomNoCenter, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkEnableMiddleButtonPan = new wxCheckBox( m_panel1, xwID_ANY, _("Use &middle mouse button to pan"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkEnableMiddleButtonPan->SetToolTip( _("Use middle mouse button dragging to pan") );
	
	bSizer2->Add( m_checkEnableMiddleButtonPan, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkMiddleButtonPanLimited = new wxCheckBox( m_panel1, wxID_ANY, _("&Limit panning to scroll size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkMiddleButtonPanLimited->SetToolTip( _("Middle mouse button panning limited by current scrollbar size") );
	
	bSizer2->Add( m_checkMiddleButtonPanLimited, 0, wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkAutoPan = new wxCheckBox( m_panel1, wxID_ANY, _("Pan while moving ob&ject"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkAutoPan, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkHVOrientation = new wxCheckBox( m_panel1, wxID_ANY, _("Allow buses and wires to be placed in H or V &orientation only"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkHVOrientation, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 3 );
	
	m_checkPageLimits = new wxCheckBox( m_panel1, wxID_ANY, _("Show p&age limits"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_checkPageLimits, 0, wxALL|wxEXPAND, 3 );
	
	
	bSizer3->Add( bSizer2, 0, wxEXPAND, 0 );
	
	
	p1mainSizer->Add( bSizer3, 1, wxALL|wxEXPAND, 6 );
	
	
	m_panel1->SetSizer( p1mainSizer );
	m_panel1->Layout();
	p1mainSizer->Fit( m_panel1 );
	m_notebook1->AddPage( m_panel1, _("General Options"), false );
	m_panel2 = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel2->SetToolTip( _("User defined field names for schematic components. ") );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	templateFieldListCtrl = new wxListCtrl( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	templateFieldListCtrl->SetMinSize( wxSize( 220,-1 ) );
	
	bSizer6->Add( templateFieldListCtrl, 1, wxALIGN_TOP|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 8 );
	
	wxStaticBoxSizer* fieldSizer;
	fieldSizer = new wxStaticBoxSizer( new wxStaticBox( m_panel2, wxID_ANY, _("Field Settings") ), wxVERTICAL );
	
	fieldNameLabel = new wxStaticText( m_panel2, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldNameLabel->Wrap( -1 );
	fieldSizer->Add( fieldNameLabel, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	fieldNameTextCtrl = new wxTextCtrl( m_panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldSizer->Add( fieldNameTextCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	fieldDefaultValueLabel = new wxStaticText( m_panel2, wxID_ANY, _("Default Value"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldDefaultValueLabel->Wrap( -1 );
	fieldSizer->Add( fieldDefaultValueLabel, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	fieldDefaultValueTextCtrl = new wxTextCtrl( m_panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fieldSizer->Add( fieldDefaultValueTextCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	fieldVisibleCheckbox = new wxCheckBox( m_panel2, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	fieldSizer->Add( fieldVisibleCheckbox, 0, wxALL, 5 );
	
	
	bSizer6->Add( fieldSizer, 0, wxEXPAND, 5 );
	
	addFieldButton = new wxButton( m_panel2, wxID_ADD_FIELD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( addFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	deleteFieldButton = new wxButton( m_panel2, wxID_DELETE_FIELD, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( deleteFieldButton, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panel2->SetSizer( bSizer6 );
	m_panel2->Layout();
	bSizer6->Fit( m_panel2 );
	m_notebook1->AddPage( m_panel2, _("Template Field Names"), true );
	
	bOptionsSizer->Add( m_notebook1, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bOptionsSizer->Add( m_sdbSizer1, 0, wxALIGN_BOTTOM|wxALL|wxEXPAND, 6 );
	
	
	mainSizer->Add( bOptionsSizer, 1, wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_EESCHEMA_OPTIONS_BASE::~DIALOG_EESCHEMA_OPTIONS_BASE()
{
}
