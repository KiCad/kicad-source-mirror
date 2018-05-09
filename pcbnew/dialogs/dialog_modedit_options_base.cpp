///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 20 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_modedit_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODEDIT_OPTIONS_BASE::DIALOG_MODEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticBoxSizer* sbSizerNewGraphicItems;
	sbSizerNewGraphicItems = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Default Values for New Graphic Items") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_EdgeModEWidthTitle = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("&Graphic line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeModEWidthTitle->Wrap( -1 );
	fgSizer1->Add( m_EdgeModEWidthTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleGrLineWidth = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_OptModuleGrLineWidth, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextGrLineUnit = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrLineUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextGrLineUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModWidthTitle = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("&Text line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModWidthTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModWidthTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleTextWidth = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_OptModuleTextWidth, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextTextWidthUnit = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextWidthUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextWidthUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModSizeVTitle = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("Text &height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeVTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModSizeVTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleTextVSize = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_OptModuleTextVSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextTextVSizeUnit = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextVSizeUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextVSizeUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModSizeHTitle = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("Text &width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeHTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModSizeHTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleTextHSize = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_OptModuleTextHSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextTextHSizeUnit = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextHSizeUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextHSizeUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerNewGraphicItems->Add( fgSizer1, 0, wxEXPAND, 20 );
	
	
	fgSizer3->Add( sbSizerNewGraphicItems, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDisplayOptions;
	bSizerDisplayOptions = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("Cartesian coordinates"), _("Polar coordinates") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_POLAR_CTRL, _("Coordinates"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 0 );
	m_PolarDisplay->SetToolTip( _("Set display of relative (dx/dy) coordinates to Cartesian (rectangular) or polar (angle/distance).") );
	
	bSizerDisplayOptions->Add( m_PolarDisplay, 1, wxALL|wxEXPAND, 5 );
	
	wxString m_UnitsSelectionChoices[] = { _("Inches"), _("Millimeters") };
	int m_UnitsSelectionNChoices = sizeof( m_UnitsSelectionChoices ) / sizeof( wxString );
	m_UnitsSelection = new wxRadioBox( this, wxID_UNITS, _("Units"), wxDefaultPosition, wxDefaultSize, m_UnitsSelectionNChoices, m_UnitsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitsSelection->SetSelection( 0 );
	m_UnitsSelection->SetToolTip( _("Set units used to display dimensions and positions.") );
	
	bSizerDisplayOptions->Add( m_UnitsSelection, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizer3->Add( bSizerDisplayOptions, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerNewFootprints;
	sbSizerNewFootprints = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Default Values for New Footprints") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 5, 5 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextRef = new wxStaticText( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, _("&Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	fgSizer2->Add( m_staticTextRef, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlRefText = new wxTextCtrl( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlRefText->SetToolTip( _("Default text for reference\nLeave blank to use the footprint name") );
	
	fgSizer2->Add( m_textCtrlRefText, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceLayerReferenceChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerReferenceNChoices = sizeof( m_choiceLayerReferenceChoices ) / sizeof( wxString );
	m_choiceLayerReference = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerReferenceNChoices, m_choiceLayerReferenceChoices, 0 );
	m_choiceLayerReference->SetSelection( 0 );
	fgSizer2->Add( m_choiceLayerReference, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceVisibleReferenceChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleReferenceNChoices = sizeof( m_choiceVisibleReferenceChoices ) / sizeof( wxString );
	m_choiceVisibleReference = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleReferenceNChoices, m_choiceVisibleReferenceChoices, 0 );
	m_choiceVisibleReference->SetSelection( 0 );
	fgSizer2->Add( m_choiceVisibleReference, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextValue = new wxStaticText( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, _("V&alue:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValue->Wrap( -1 );
	fgSizer2->Add( m_staticTextValue, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlValueText = new wxTextCtrl( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlValueText->SetToolTip( _("Default text for value\nLeave blank to use the footprint name") );
	
	fgSizer2->Add( m_textCtrlValueText, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceLayerValueChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerValueNChoices = sizeof( m_choiceLayerValueChoices ) / sizeof( wxString );
	m_choiceLayerValue = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerValueNChoices, m_choiceLayerValueChoices, 0 );
	m_choiceLayerValue->SetSelection( 1 );
	fgSizer2->Add( m_choiceLayerValue, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceVisibleValueChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleValueNChoices = sizeof( m_choiceVisibleValueChoices ) / sizeof( wxString );
	m_choiceVisibleValue = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleValueNChoices, m_choiceVisibleValueChoices, 0 );
	m_choiceVisibleValue->SetSelection( 0 );
	fgSizer2->Add( m_choiceVisibleValue, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerNewFootprints->Add( fgSizer2, 0, wxEXPAND, 20 );
	
	m_staticTextInfo = new wxStaticText( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, _("Leave reference and/or value blank to use footprint name."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbSizerNewFootprints->Add( m_staticTextInfo, 0, wxTOP, 5 );
	
	
	fgSizer3->Add( sbSizerNewFootprints, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerEditOptions;
	sbSizerEditOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing Options") ), wxVERTICAL );
	
	m_MagneticPads = new wxCheckBox( sbSizerEditOptions->GetStaticBox(), wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerEditOptions->Add( m_MagneticPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizerEditOptions->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_Segments_45_Only_Ctrl = new wxCheckBox( sbSizerEditOptions->GetStaticBox(), wxID_SEGMENTS45, _("L&imit graphic lines to 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Segments_45_Only_Ctrl->SetToolTip( _("Force line segment directions to H, V or 45 degrees when drawing on technical layers.") );
	
	sbSizerEditOptions->Add( m_Segments_45_Only_Ctrl, 0, wxALL, 5 );
	
	m_dragSelects = new wxCheckBox( sbSizerEditOptions->GetStaticBox(), wxID_ANY, _("Prefer selection to dragging"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dragSelects->SetToolTip( _("When enabled and nothing is selected, drag gesture will draw a selection box, even if there are items under the cursor that could be immediately dragged.") );
	
	sbSizerEditOptions->Add( m_dragSelects, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	fgSizer3->Add( sbSizerEditOptions, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( fgSizer3, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerColumns;
	bSizerColumns = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizerMain->Add( bSizerColumns, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	// Connect Events
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODEDIT_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODEDIT_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_MODEDIT_OPTIONS_BASE::~DIALOG_MODEDIT_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODEDIT_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MODEDIT_OPTIONS_BASE::OnOkClick ), NULL, this );
	
}
