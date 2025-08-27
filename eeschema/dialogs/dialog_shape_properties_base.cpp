///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"
#include "widgets/wx_infobar.h"

#include "dialog_shape_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_SHAPE_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_OnCheckBox )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_OnCheckBox )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_OnCheckBox )
	EVT_CHECKBOX( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onBorderChecked )
	EVT_CHOICE( wxID_ANY, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillChoice )
	EVT_RADIOBUTTON( NO_FILL, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillRadioButton )
	EVT_RADIOBUTTON( FILLED_SHAPE, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillRadioButton )
	EVT_RADIOBUTTON( FILLED_WITH_BG_BODYCOLOR, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillRadioButton )
	EVT_RADIOBUTTON( FILLED_WITH_COLOR, DIALOG_SHAPE_PROPERTIES_BASE::_wxFB_onFillRadioButton )
END_EVENT_TABLE()

DIALOG_SHAPE_PROPERTIES_BASE::DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	mainSizer->Add( m_infoBar, 0, wxBOTTOM|wxEXPAND, 5 );

	m_ruleAreaSizer = new wxBoxSizer( wxVERTICAL );

	m_cbExcludeFromSim = new wxCheckBox( this, wxID_ANY, _("Exclude from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ruleAreaSizer->Add( m_cbExcludeFromSim, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	m_ruleAreaSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	m_cbExcludeFromBom = new wxCheckBox( this, wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBom->SetToolTip( _("This is useful for adding symbols for board footprints such as fiducials\nand logos that you do not want to appear in the bill of materials export") );

	m_ruleAreaSizer->Add( m_cbExcludeFromBom, 0, wxALL, 5 );

	m_cbExcludeFromBoard = new wxCheckBox( this, wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBoard->SetToolTip( _("This is useful for adding symbols that only get exported to the bill of materials but\nnot required to layout the board such as mechanical fasteners and enclosures") );

	m_ruleAreaSizer->Add( m_cbExcludeFromBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDNP = new wxCheckBox( this, wxID_ANY, _("Do not populate"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ruleAreaSizer->Add( m_cbDNP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_ruleAreaSizer->Add( 0, 5, 1, wxEXPAND, 5 );


	mainSizer->Add( m_ruleAreaSizer, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bColumns;
	bColumns = new wxBoxSizer( wxHORIZONTAL );

	m_borderSizer = new wxGridBagSizer( 3, 3 );
	m_borderSizer->SetFlexibleDirection( wxBOTH );
	m_borderSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_borderSizer->SetEmptyCellSize( wxSize( 36,-1 ) );

	m_borderCheckbox = new wxCheckBox( this, wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderSizer->Add( m_borderCheckbox, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM, 2 );

	m_borderWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	m_borderSizer->Add( m_borderWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer7->Add( m_borderWidthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_borderWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer7->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );

	m_borderColorLabel = new wxStaticText( this, wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer7->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );


	bSizer7->Add( 5, 0, 0, 0, 5 );

	m_panelBorderColor = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxVERTICAL );

	m_borderColorSwatch = new COLOR_SWATCH( m_panelBorderColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer23->Add( m_borderColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelBorderColor->SetSizer( bSizer23 );
	m_panelBorderColor->Layout();
	bSizer23->Fit( m_panelBorderColor );
	bSizer7->Add( m_panelBorderColor, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_borderSizer->Add( bSizer7, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_borderStyleLabel = new wxStaticText( this, wxID_ANY, _("Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderStyleLabel->Wrap( -1 );
	m_borderSizer->Add( m_borderStyleLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_borderStyleCombo = new wxBitmapComboBox( this, wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_borderStyleCombo->SetMinSize( wxSize( 240,-1 ) );

	m_borderSizer->Add( m_borderStyleCombo, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_helpLabel1 = new wxStaticText( this, wxID_ANY, _("Set border width to 0 to use schematic's default line width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel1->Wrap( 320 );
	m_borderSizer->Add( m_helpLabel1, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxTOP, 8 );


	bColumns->Add( m_borderSizer, 9, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bColumns->Add( 15, 0, 0, wxEXPAND, 5 );

	m_fillBook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_schematicPage = new wxPanel( m_fillBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_fillSizer = new wxGridBagSizer( 3, 3 );
	m_fillSizer->SetFlexibleDirection( wxBOTH );
	m_fillSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_fillSizer->SetEmptyCellSize( wxSize( 36,12 ) );

	m_fillLabel = new wxStaticText( m_schematicPage, wxID_ANY, _("Fill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillLabel->Wrap( -1 );
	m_fillSizer->Add( m_fillLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_fillCtrlChoices[] = { _("None"), _("Solid"), _("Hatch"), _("Reverse Hatch"), _("Cross-hatch") };
	int m_fillCtrlNChoices = sizeof( m_fillCtrlChoices ) / sizeof( wxString );
	m_fillCtrl = new wxChoice( m_schematicPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_fillCtrlNChoices, m_fillCtrlChoices, 0 );
	m_fillCtrl->SetSelection( 0 );
	m_fillSizer->Add( m_fillCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_fillColorLabel = new wxStaticText( m_schematicPage, wxID_ANY, _("Fill color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillColorLabel->Wrap( -1 );
	m_fillSizer->Add( m_fillColorLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_panelFillColor = new wxPanel( m_schematicPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_fillColorSwatch = new COLOR_SWATCH( m_panelFillColor, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer21->Add( m_fillColorSwatch, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


	m_panelFillColor->SetSizer( bSizer21 );
	m_panelFillColor->Layout();
	bSizer21->Fit( m_panelFillColor );
	m_fillSizer->Add( m_panelFillColor, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_helpLabel2 = new wxStaticText( m_schematicPage, wxID_ANY, _("Clear colors to use Schematic Editor colors."), wxDefaultPosition, wxDefaultSize, 0 );
	m_helpLabel2->Wrap( -1 );
	m_fillSizer->Add( m_helpLabel2, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxTOP|wxRIGHT, 8 );


	bSizer8->Add( m_fillSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	m_schematicPage->SetSizer( bSizer8 );
	m_schematicPage->Layout();
	bSizer8->Fit( m_schematicPage );
	m_fillBook->AddPage( m_schematicPage, _("a page"), false );
	m_symbolEditorPage = new wxPanel( m_fillBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* bSizerFill;
	bSizerFill = new wxStaticBoxSizer( new wxStaticBox( m_symbolEditorPage, wxID_ANY, _("Fill Style") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_rbFillNone = new wxRadioButton( bSizerFill->GetStaticBox(), NO_FILL, _("Do not fill"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	gbSizer1->Add( m_rbFillNone, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillOutline = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_SHAPE, _("Fill with body outline color"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillOutline, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillBackground = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_WITH_BG_BODYCOLOR, _("Fill with body background color"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillBackground, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_rbFillCustom = new wxRadioButton( bSizerFill->GetStaticBox(), FILLED_WITH_COLOR, _("Fill with:"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_rbFillCustom, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_customColorSwatch = new COLOR_SWATCH( bSizerFill->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_customColorSwatch, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerFill->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM, 5 );


	bSizer9->Add( bSizerFill, 1, wxEXPAND, 5 );


	m_symbolEditorPage->SetSizer( bSizer9 );
	m_symbolEditorPage->Layout();
	bSizer9->Fit( m_symbolEditorPage );
	m_fillBook->AddPage( m_symbolEditorPage, _("a page"), false );

	bColumns->Add( m_fillBook, 8, wxEXPAND | wxALL, 10 );


	mainSizer->Add( bColumns, 0, wxEXPAND, 5 );

	m_symbolEditorSizer = new wxBoxSizer( wxVERTICAL );


	m_symbolEditorSizer->Add( 0, 10, 0, wxEXPAND, 5 );

	wxBoxSizer* bColumns2;
	bColumns2 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeft2;
	bLeft2 = new wxBoxSizer( wxVERTICAL );

	m_privateCheckbox = new wxCheckBox( this, wxID_ANY, _("Private to Symbol Editor"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeft2->Add( m_privateCheckbox, 0, wxTOP, 5 );


	bColumns2->Add( bLeft2, 9, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bColumns2->Add( 15, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bRight2;
	bRight2 = new wxBoxSizer( wxVERTICAL );

	m_checkApplyToAllUnits = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	bRight2->Add( m_checkApplyToAllUnits, 0, wxTOP|wxRIGHT, 5 );

	m_checkApplyToAllBodyStyles = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles"), wxDefaultPosition, wxDefaultSize, 0 );
	bRight2->Add( m_checkApplyToAllBodyStyles, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bColumns2->Add( bRight2, 8, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	m_symbolEditorSizer->Add( bColumns2, 1, wxEXPAND, 5 );


	mainSizer->Add( m_symbolEditorSizer, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_SHAPE_PROPERTIES_BASE::~DIALOG_SHAPE_PROPERTIES_BASE()
{
}
