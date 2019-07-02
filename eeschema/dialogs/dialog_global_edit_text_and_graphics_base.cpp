///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_edit_text_and_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbScope;
	sbScope = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scope") ), wxVERTICAL );
	
	m_references = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Symbol references"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_references, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_values = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Symbol values"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_values, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_otherFields = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Other symbol fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_otherFields, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	
	sbScope->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_wires = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Wires && wire labels"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_wires, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_busses = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Busses && bus labels"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_busses, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_globalLabels = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Global labels"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_globalLabels, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	
	sbScope->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_hierLabels = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Hierarchical labels"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_hierLabels, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_sheetTitles = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Sheet titles"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_sheetTitles, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	m_sheetPins = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Sheet pins"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_sheetPins, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	
	sbScope->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_schTextAndGraphics = new wxCheckBox( sbScope->GetStaticBox(), wxID_ANY, _("Schematic text && graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	sbScope->Add( m_schTextAndGraphics, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );
	
	
	bSizerTop->Add( sbScope, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Filters") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_fieldnameFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter fields by name:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldnameFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_fieldnameFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_fieldnameFilter, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	m_referenceFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by parent symbol reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_referenceFilterOpt, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_referenceFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_referenceFilter->SetMinSize( wxSize( 150,-1 ) );
	
	fgSizer2->Add( m_referenceFilter, 0, wxEXPAND|wxLEFT, 5 );
	
	m_symbolFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by parent symbol library id:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_symbolFilterOpt, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_symbolFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_symbolFilter, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_netFilterOpt = new wxCheckBox( sbFilters->GetStaticBox(), wxID_ANY, _("Filter items by net:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_netFilterOpt, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_netFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_netFilter, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );
	
	
	sbFilters->Add( fgSizer2, 1, wxEXPAND|wxRIGHT, 5 );
	
	
	bSizerTop->Add( sbFilters, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );
	
	
	bMainSizer->Add( bSizerTop, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( 0, 0, 0, wxTOP, 5 );
	
	
	bMainSizer->Add( 0, 0, 0, wxTOP, 5 );
	
	wxStaticBoxSizer* sbAction;
	sbAction = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Set To") ), wxVERTICAL );
	
	m_specifiedValues = new wxPanel( sbAction->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer1->AddGrowableCol( 4 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_textSizeLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	fgSizer1->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_textSizeCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeCtrl->SetMinSize( wxSize( 120,-1 ) );
	
	fgSizer1->Add( m_textSizeCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_textSizeUnits = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	
	fgSizer1->Add( 10, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );
	
	m_Bold = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	fgSizer1->Add( m_Bold, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	orientationLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	orientationLabel->Wrap( -1 );
	fgSizer1->Add( orientationLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_orientationChoices[] = { _("Right"), _("Up"), _("Left"), _("Down"), _("...") };
	int m_orientationNChoices = sizeof( m_orientationChoices ) / sizeof( wxString );
	m_orientation = new wxChoice( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_orientationNChoices, m_orientationChoices, 0 );
	m_orientation->SetSelection( 0 );
	fgSizer1->Add( m_orientation, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );
	
	m_Italic = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	fgSizer1->Add( m_Italic, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	hAlignLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("H Alignment (fields only):"), wxDefaultPosition, wxDefaultSize, 0 );
	hAlignLabel->Wrap( -1 );
	fgSizer1->Add( hAlignLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_hAlignChoices[] = { _("Left"), _("Center"), _("Right"), _("...") };
	int m_hAlignNChoices = sizeof( m_hAlignChoices ) / sizeof( wxString );
	m_hAlign = new wxChoice( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_hAlignNChoices, m_hAlignChoices, 0 );
	m_hAlign->SetSelection( 0 );
	fgSizer1->Add( m_hAlign, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_Visible = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Visible (fields only)"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	fgSizer1->Add( m_Visible, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	vAlignLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("V Alignment (fields only):"), wxDefaultPosition, wxDefaultSize, 0 );
	vAlignLabel->Wrap( -1 );
	fgSizer1->Add( vAlignLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_vAlignChoices[] = { _("Top"), _("Center"), _("Bottom"), _("...") };
	int m_vAlignNChoices = sizeof( m_vAlignChoices ) / sizeof( wxString );
	m_vAlign = new wxChoice( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_vAlignNChoices, m_vAlignChoices, 0 );
	m_vAlign->SetSelection( 0 );
	fgSizer1->Add( m_vAlign, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 7 );
	
	m_staticline2 = new wxStaticLine( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 7 );
	
	m_staticline3 = new wxStaticLine( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 7 );
	
	m_staticline4 = new wxStaticLine( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 7 );
	
	m_staticline5 = new wxStaticLine( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline5, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 7 );
	
	m_lineWidthLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LineWidthCtrl = new wxTextCtrl( m_specifiedValues, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_LineWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_lineWidthUnits = new wxStaticText( m_specifiedValues, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	lineStyleLabel = new wxStaticText( m_specifiedValues, wxID_ANY, _("Line style (graphics only):"), wxDefaultPosition, wxDefaultSize, 0 );
	lineStyleLabel->Wrap( -1 );
	fgSizer1->Add( lineStyleLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_lineStyleChoices[] = { _("Solid"), _("Dashed"), _("Dotted"), _("Dash-Dot"), _("...") };
	int m_lineStyleNChoices = sizeof( m_lineStyleChoices ) / sizeof( wxString );
	m_lineStyle = new wxChoice( m_specifiedValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_lineStyleNChoices, m_lineStyleChoices, 0 );
	m_lineStyle->SetSelection( 0 );
	fgSizer1->Add( m_lineStyle, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	m_setColor = new wxCheckBox( m_specifiedValues, wxID_ANY, _("Color (graphics only):"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_setColor, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_color = new wxColourPickerCtrl( m_specifiedValues, wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_DEFAULT_STYLE );
	bSizer4->Add( m_color, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer1->Add( bSizer4, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer2->Add( fgSizer1, 1, wxEXPAND|wxTOP, 2 );
	
	
	m_specifiedValues->SetSizer( bSizer2 );
	m_specifiedValues->Layout();
	bSizer2->Fit( m_specifiedValues );
	sbAction->Add( m_specifiedValues, 1, wxEXPAND|wxBOTTOM, 12 );
	
	
	bMainSizer->Add( sbAction, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	bMainSizer->Add( m_sdbSizerButtons, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnUpdateUI ) );
	m_fieldnameFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_referenceFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_symbolFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnSymbolFilterText ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnUpdateUI ) );
	m_fieldnameFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_referenceFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnReferenceFilterText ), NULL, this );
	m_symbolFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE::OnSymbolFilterText ), NULL, this );
	
}
