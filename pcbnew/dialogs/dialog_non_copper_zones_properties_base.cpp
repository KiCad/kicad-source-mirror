///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_non_copper_zones_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );

	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	bSizerLeft->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_layers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER|wxBORDER_SIMPLE );
	m_layers->SetMinSize( wxSize( 100,-1 ) );

	bSizerLeft->Add( m_layers, 1, wxALL|wxEXPAND, 5 );


	bSizerLeft->Add( 0, 5, 0, wxEXPAND, 5 );


	m_UpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbShape;
	sbShape = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Shape") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ConstrainOpt = new wxCheckBox( sbShape->GetStaticBox(), wxID_ANY, _("Constrain outline to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_ConstrainOpt, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), wxALL, 5 );

	m_cbLocked = new wxCheckBox( sbShape->GetStaticBox(), wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbLocked, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextStyle = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("Outline style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyle->Wrap( -1 );
	gbSizer1->Add( m_staticTextStyle, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_OutlineDisplayCtrlChoices[] = { _("Line"), _("Hatched"), _("Fully hatched") };
	int m_OutlineDisplayCtrlNChoices = sizeof( m_OutlineDisplayCtrlChoices ) / sizeof( wxString );
	m_OutlineDisplayCtrl = new wxChoice( sbShape->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_OutlineDisplayCtrlNChoices, m_OutlineDisplayCtrlChoices, 0 );
	m_OutlineDisplayCtrl->SetSelection( 0 );
	gbSizer1->Add( m_OutlineDisplayCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_MinWidthLabel = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("Minimum width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_MinWidthLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_MinWidthCtrl = new wxTextCtrl( sbShape->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_MinWidthCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_MinWidthUnits = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_MinWidthUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextSmoothing = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("Outline smooth:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSmoothing->Wrap( -1 );
	gbSizer1->Add( m_staticTextSmoothing, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_cornerSmoothingChoiceChoices[] = { _("None"), _("Chamfer"), _("Fillet") };
	int m_cornerSmoothingChoiceNChoices = sizeof( m_cornerSmoothingChoiceChoices ) / sizeof( wxString );
	m_cornerSmoothingChoice = new wxChoice( sbShape->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cornerSmoothingChoiceNChoices, m_cornerSmoothingChoiceChoices, 0 );
	m_cornerSmoothingChoice->SetSelection( 0 );
	gbSizer1->Add( m_cornerSmoothingChoice, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cornerRadiusLabel = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("Chamfer distance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusLabel->Wrap( -1 );
	gbSizer1->Add( m_cornerRadiusLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_cornerRadiusCtrl = new wxTextCtrl( sbShape->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cornerRadiusCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cornerRadiusUnits = new wxStaticText( sbShape->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cornerRadiusUnits->Wrap( -1 );
	gbSizer1->Add( m_cornerRadiusUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbShape->Add( gbSizer1, 0, wxEXPAND, 5 );


	bSizerMiddle->Add( sbShape, 1, wxEXPAND, 5 );

	wxStaticBoxSizer* sbFill;
	sbFill = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fill") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGridFillType = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Fill type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridFillType->Wrap( -1 );
	fgSizer1->Add( m_staticTextGridFillType, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxString m_GridStyleCtrlChoices[] = { _("Solid shape"), _("HatchBorder pattern") };
	int m_GridStyleCtrlNChoices = sizeof( m_GridStyleCtrlChoices ) / sizeof( wxString );
	m_GridStyleCtrl = new wxChoice( sbFill->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_GridStyleCtrlNChoices, m_GridStyleCtrlChoices, 0 );
	m_GridStyleCtrl->SetSelection( 0 );
	fgSizer1->Add( m_GridStyleCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextGrindOrient = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrindOrient->Wrap( -1 );
	fgSizer1->Add( m_staticTextGrindOrient, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_tcGridStyleOrientation = new wxTextCtrl( sbFill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_tcGridStyleOrientation, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_staticTextRotUnits = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("degree"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRotUnits->Wrap( -1 );
	fgSizer1->Add( m_staticTextRotUnits, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextStyleThickness = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Hatch width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStyleThickness->Wrap( -1 );
	fgSizer1->Add( m_staticTextStyleThickness, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_tcGridStyleThickness = new wxTextCtrl( sbFill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_tcGridStyleThickness, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_GridStyleThicknessUnits = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleThicknessUnits->Wrap( -1 );
	fgSizer1->Add( m_GridStyleThicknessUnits, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridGap = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Hatch gap:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridGap->Wrap( -1 );
	fgSizer1->Add( m_staticTextGridGap, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_tcGridStyleGap = new wxTextCtrl( sbFill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_tcGridStyleGap, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_GridStyleGapUnits = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GridStyleGapUnits->Wrap( -1 );
	fgSizer1->Add( m_GridStyleGapUnits, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextGridSmoothingLevel = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Smoothing effort:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmoothingLevel->Wrap( -1 );
	m_staticTextGridSmoothingLevel->SetToolTip( _("Value of smoothing effort\n0 = no smoothing\n1 = chamfer\n2 = round corners\n3 = round corners (finer shape)") );

	fgSizer1->Add( m_staticTextGridSmoothingLevel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlSmoothLevel = new wxSpinCtrl( sbFill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 3, 0 );
	fgSizer1->Add( m_spinCtrlSmoothLevel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextGridSmootingVal = new wxStaticText( sbFill->GetStaticBox(), wxID_ANY, _("Smooth amount:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridSmootingVal->Wrap( -1 );
	m_staticTextGridSmootingVal->SetToolTip( _("Ratio between smoothed corners size and the gap between lines\n0 = no smoothing\n1.0 = max radius/chamfer size (half gap value)") );

	fgSizer1->Add( m_staticTextGridSmootingVal, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_spinCtrlSmoothValue = new wxSpinCtrlDouble( sbFill->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0.1, 0.1 );
	m_spinCtrlSmoothValue->SetDigits( 2 );
	fgSizer1->Add( m_spinCtrlSmoothValue, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbFill->Add( fgSizer1, 0, wxEXPAND, 5 );


	bSizerMiddle->Add( sbFill, 1, wxEXPAND|wxTOP|wxBOTTOM, 10 );


	m_UpperSizer->Add( bSizerMiddle, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	m_MainSizer->Add( m_UpperSizer, 1, wxEXPAND|wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();

	m_MainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnUpdateUI ) );
	m_layers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_GridStyleCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnStyleSelection ), NULL, this );
}

DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::~DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnUpdateUI ) );
	m_layers->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnLayerSelection ), NULL, this );
	m_GridStyleCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::OnStyleSelection ), NULL, this );

}
