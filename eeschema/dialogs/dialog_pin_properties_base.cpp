///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pinshape_combobox.h"
#include "pintype_combobox.h"
#include "widgets/std_bitmap_button.h"
#include "widgets/wx_bitmap_combobox.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "dialog_pin_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PIN_PROPERTIES_BASE::DIALOG_PIN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	mainSizer->Add( m_infoBar, 0, wxEXPAND, 5 );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pinNameLabel = new wxStaticText( this, wxID_ANY, _("Pin &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameLabel->Wrap( -1 );
	gbSizer1->Add( m_pinNameLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textPinName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textPinName, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pinNumberLabel = new wxStaticText( this, wxID_ANY, _("Pin num&ber:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumberLabel->Wrap( -1 );
	m_pinNumberLabel->SetToolTip( _("Pin number: 1 to 4 ASCII letters and/or digits") );

	gbSizer1->Add( m_pinNumberLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textPinNumber = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textPinNumber, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXLabel = new wxStaticText( this, wxID_ANY, _("&X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXLabel->Wrap( -1 );
	gbSizer1->Add( m_posXLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_posXCtrl->SetMinSize( wxSize( 110,-1 ) );

	gbSizer1->Add( m_posXCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posYLabel = new wxStaticText( this, wxID_ANY, _("&Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYLabel->Wrap( -1 );
	gbSizer1->Add( m_posYLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_posYCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_pinLengthLabel = new wxStaticText( this, wxID_ANY, _("&Pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthLabel->Wrap( -1 );
	gbSizer1->Add( m_pinLengthLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pinLengthCtrl, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_nameSizeLabel = new wxStaticText( this, wxID_ANY, _("N&ame text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_nameSizeLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_nameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_nameSizeCtrl, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_numberSizeLabel = new wxStaticText( this, wxID_ANY, _("Number text si&ze:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numberSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_numberSizeLabel, wxGBPosition( 9, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_numberSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_numberSizeCtrl, wxGBPosition( 9, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextEType = new wxStaticText( this, wxID_ANY, _("Electrical type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextEType->Wrap( -1 );
	m_staticTextEType->SetToolTip( _("Used by the ERC.") );

	gbSizer1->Add( m_staticTextEType, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceElectricalType = new PINTYPE_COMBOBOX( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceElectricalType, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextGstyle = new wxStaticText( this, wxID_ANY, _("Graphic style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGstyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextGstyle, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceStyle = new PINSHAPE_COMBOBOX( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceStyle, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_staticTextOrient = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrient->Wrap( -1 );
	gbSizer1->Add( m_staticTextOrient, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_choiceOrientation = new WX_BITMAP_COMBOBOX( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY );
	gbSizer1->Add( m_choiceOrientation, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_posXUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posXUnits->Wrap( -1 );
	gbSizer1->Add( m_posXUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_posYUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_posYUnits->Wrap( -1 );
	gbSizer1->Add( m_posYUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinLengthUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_pinLengthUnits, wxGBPosition( 7, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_nameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_nameSizeUnits, wxGBPosition( 8, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_numberSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_numberSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_numberSizeUnits, wxGBPosition( 9, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bLeftSizer->Add( gbSizer1, 1, wxEXPAND, 5 );


	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND|wxALL, 15 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* checkboxesSizer;
	checkboxesSizer = new wxBoxSizer( wxVERTICAL );

	m_checkApplyToAllParts = new wxCheckBox( this, wxID_ANY, _("Common to all &units in symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	checkboxesSizer->Add( m_checkApplyToAllParts, 0, wxBOTTOM, 3 );

	m_checkApplyToAllBodyStyles = new wxCheckBox( this, wxID_ANY, _("Common to all body &styles"), wxDefaultPosition, wxDefaultSize, 0 );
	checkboxesSizer->Add( m_checkApplyToAllBodyStyles, 0, wxBOTTOM, 3 );


	checkboxesSizer->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_checkShow = new wxCheckBox( this, wxID_ANY, _("&Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShow->SetValue(true);
	checkboxesSizer->Add( m_checkShow, 0, wxBOTTOM, 3 );


	bRightSizer->Add( checkboxesSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Preview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	bRightSizer->Add( m_staticText16, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_panelShowPin = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );
	m_panelShowPin->SetMinSize( wxSize( 150,150 ) );

	bRightSizer->Add( m_panelShowPin, 1, wxEXPAND|wxLEFT, 5 );


	bUpperSizer->Add( bRightSizer, 1, wxEXPAND|wxALL, 15 );


	mainSizer->Add( bUpperSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	m_alternatesTurndown = new wxCollapsiblePane( this, wxID_ANY, _("Alternate pin function definitions"), wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE|wxCP_NO_TLW_RESIZE );
	m_alternatesTurndown->Collapse( true );

	wxBoxSizer* bAlternatesSizer;
	bAlternatesSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bRightMargin;
	bRightMargin = new wxBoxSizer( wxVERTICAL );

	m_alternatesGrid = new WX_GRID( m_alternatesTurndown->GetPane(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_alternatesGrid->CreateGrid( 1, 3 );
	m_alternatesGrid->EnableEditing( true );
	m_alternatesGrid->EnableGridLines( true );
	m_alternatesGrid->EnableDragGridSize( false );
	m_alternatesGrid->SetMargins( 0, 0 );

	// Columns
	m_alternatesGrid->SetColSize( 0, 260 );
	m_alternatesGrid->SetColSize( 1, 140 );
	m_alternatesGrid->SetColSize( 2, 140 );
	m_alternatesGrid->EnableDragColMove( false );
	m_alternatesGrid->EnableDragColSize( false );
	m_alternatesGrid->SetColLabelValue( 0, _("Alternate Pin Name") );
	m_alternatesGrid->SetColLabelValue( 1, _("Electrical Type") );
	m_alternatesGrid->SetColLabelValue( 2, _("Graphic Style") );
	m_alternatesGrid->SetColLabelSize( 22 );
	m_alternatesGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_alternatesGrid->EnableDragRowSize( false );
	m_alternatesGrid->SetRowLabelSize( 0 );
	m_alternatesGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_alternatesGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_alternatesGrid->SetMinSize( wxSize( -1,100 ) );

	bRightMargin->Add( m_alternatesGrid, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bAlternatesSizer->Add( bRightMargin, 1, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addAlternate = new STD_BITMAP_BUTTON( m_alternatesTurndown->GetPane(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_addAlternate, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bButtonSizer->Add( 20, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_deleteAlternate = new STD_BITMAP_BUTTON( m_alternatesTurndown->GetPane(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_deleteAlternate, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bAlternatesSizer->Add( bButtonSizer, 0, wxTOP, 5 );


	m_alternatesTurndown->GetPane()->SetSizer( bAlternatesSizer );
	m_alternatesTurndown->GetPane()->Layout();
	bAlternatesSizer->Fit( m_alternatesTurndown->GetPane() );
	bLowerSizer->Add( m_alternatesTurndown, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( bLowerSizer, 1, wxEXPAND|wxLEFT, 10 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	mainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnSize ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnUpdateUI ) );
	m_textPinName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNumber->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_pinLengthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_nameSizeCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_numberSizeCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllBodyStyles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_alternatesTurndown->Connect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnCollapsiblePaneChange ), NULL, this );
	m_addAlternate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnAddAlternate ), NULL, this );
	m_deleteAlternate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnDeleteAlternate ), NULL, this );
}

DIALOG_PIN_PROPERTIES_BASE::~DIALOG_PIN_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnSize ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnUpdateUI ) );
	m_textPinName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_textPinNumber->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_pinLengthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_nameSizeCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_numberSizeCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceElectricalType->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceStyle->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_choiceOrientation->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllParts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkApplyToAllBodyStyles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_checkShow->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnPropertiesChange ), NULL, this );
	m_alternatesTurndown->Disconnect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnCollapsiblePaneChange ), NULL, this );
	m_addAlternate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnAddAlternate ), NULL, this );
	m_deleteAlternate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PIN_PROPERTIES_BASE::OnDeleteAlternate ), NULL, this );

}
