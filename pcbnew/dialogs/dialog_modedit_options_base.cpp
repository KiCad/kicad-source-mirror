///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_modedit_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MODEDIT_OPTIONS_BASE::DIALOG_MODEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );
	
	m_staticText281 = new wxStaticText( this, wxID_ANY, _("On new graphic item creation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText281->Wrap( -1 );
	m_staticText281->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerUpper->Add( m_staticText281, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_EdgeModEWidthTitle = new wxStaticText( this, wxID_ANY, _("Graphic line width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeModEWidthTitle->Wrap( -1 );
	fgSizer1->Add( m_EdgeModEWidthTitle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleGrLineWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleGrLineWidth->SetMaxLength( 0 ); 
	fgSizer1->Add( m_OptModuleGrLineWidth, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextGrLineUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrLineUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextGrLineUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModWidthTitle = new wxStaticText( this, wxID_ANY, _("Text width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModWidthTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModWidthTitle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleTextWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextWidth->SetMaxLength( 0 ); 
	fgSizer1->Add( m_OptModuleTextWidth, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextTextWidthUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextWidthUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextWidthUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModSizeVTitle = new wxStaticText( this, wxID_ANY, _("Text size V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeVTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModSizeVTitle, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_OptModuleTextVSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextVSize->SetMaxLength( 0 ); 
	fgSizer1->Add( m_OptModuleTextVSize, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextTextVSizeUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextVSizeUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextVSizeUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextModSizeHTitle = new wxStaticText( this, wxID_ANY, _("Text size H"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeHTitle->Wrap( -1 );
	fgSizer1->Add( m_TextModSizeHTitle, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_OptModuleTextHSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextHSize->SetMaxLength( 0 ); 
	fgSizer1->Add( m_OptModuleTextHSize, 0, wxEXPAND|wxALL, 5 );
	
	m_staticTextTextHSizeUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTextHSizeUnit->Wrap( -1 );
	fgSizer1->Add( m_staticTextTextHSizeUnit, 0, wxALL, 5 );
	
	
	bSizerUpper->Add( fgSizer1, 0, wxEXPAND|wxLEFT, 20 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerUpper->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_staticText28 = new wxStaticText( this, wxID_ANY, _("Default values on new footprint creation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	m_staticText28->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerUpper->Add( m_staticText28, 0, wxALL, 5 );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Leave ref or value blank to use the footprint name as default text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	bSizerUpper->Add( m_staticTextInfo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 6, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->AddGrowableCol( 3 );
	fgSizer2->AddGrowableCol( 5 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextRef = new wxStaticText( this, wxID_ANY, _("Ref"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRef->Wrap( -1 );
	fgSizer2->Add( m_staticTextRef, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlRefText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlRefText->SetToolTip( _("Default text for reference\nLeave blank to use the footprint name") );
	
	fgSizer2->Add( m_textCtrlRefText, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextRefLayer = new wxStaticText( this, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRefLayer->Wrap( -1 );
	fgSizer2->Add( m_staticTextRefLayer, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceLayerReferenceChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerReferenceNChoices = sizeof( m_choiceLayerReferenceChoices ) / sizeof( wxString );
	m_choiceLayerReference = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerReferenceNChoices, m_choiceLayerReferenceChoices, 0 );
	m_choiceLayerReference->SetSelection( 0 );
	fgSizer2->Add( m_choiceLayerReference, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText32 = new wxStaticText( this, wxID_ANY, _("Visibility"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	fgSizer2->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceVisibleReferenceChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleReferenceNChoices = sizeof( m_choiceVisibleReferenceChoices ) / sizeof( wxString );
	m_choiceVisibleReference = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleReferenceNChoices, m_choiceVisibleReferenceChoices, 0 );
	m_choiceVisibleReference->SetSelection( 0 );
	fgSizer2->Add( m_choiceVisibleReference, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextValue = new wxStaticText( this, wxID_ANY, _("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValue->Wrap( -1 );
	fgSizer2->Add( m_staticTextValue, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlValueText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlValueText->SetToolTip( _("Default text for value\nLeave blank to use the footprint name") );
	
	fgSizer2->Add( m_textCtrlValueText, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextValLayer = new wxStaticText( this, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValLayer->Wrap( -1 );
	fgSizer2->Add( m_staticTextValLayer, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceLayerValueChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerValueNChoices = sizeof( m_choiceLayerValueChoices ) / sizeof( wxString );
	m_choiceLayerValue = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerValueNChoices, m_choiceLayerValueChoices, 0 );
	m_choiceLayerValue->SetSelection( 1 );
	fgSizer2->Add( m_choiceLayerValue, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextValVisibility = new wxStaticText( this, wxID_ANY, _("Visibility"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextValVisibility->Wrap( -1 );
	fgSizer2->Add( m_staticTextValVisibility, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	wxString m_choiceVisibleValueChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleValueNChoices = sizeof( m_choiceVisibleValueChoices ) / sizeof( wxString );
	m_choiceVisibleValue = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleValueNChoices, m_choiceVisibleValueChoices, 0 );
	m_choiceVisibleValue->SetSelection( 0 );
	fgSizer2->Add( m_choiceVisibleValue, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( fgSizer2, 0, wxEXPAND|wxLEFT, 20 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerUpper->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_stGeneral = new wxStaticText( this, wxID_ANY, _("General options:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stGeneral->Wrap( -1 );
	m_stGeneral->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerUpper->Add( m_stGeneral, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgGeneral;
	fgGeneral = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgGeneral->AddGrowableCol( 1 );
	fgGeneral->SetFlexibleDirection( wxBOTH );
	fgGeneral->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_stMaxUndoItems = new wxStaticText( this, wxID_ANY, _("Maximum undo items (0 = unlimited):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxUndoItems->Wrap( -1 );
	fgGeneral->Add( m_stMaxUndoItems, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_spinMaxUndoItems = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65536, 0 );
	fgGeneral->Add( m_spinMaxUndoItems, 0, wxALL|wxEXPAND, 5 );
	
	m_stMaxUndoItemsUnit = new wxStaticText( this, wxID_ANY, _("actions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxUndoItemsUnit->Wrap( -1 );
	fgGeneral->Add( m_stMaxUndoItemsUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizerUpper->Add( fgGeneral, 0, wxEXPAND|wxLEFT, 20 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
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
