///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "panel_modedit_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_MODEDIT_DEFAULTS_BASE::PANEL_MODEDIT_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerNewGraphicItems;
	sbSizerNewGraphicItems = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Default Values for New Graphic Items") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_lineWidthLabel = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("&Graphic line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_lineWidthCtrl = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_lineWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_lineWidthUnits = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textThickLabel = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("&Text thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textThickLabel->Wrap( -1 );
	fgSizer1->Add( m_textThickLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textThickCtrl = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textThickCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_textThickUnits = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textThickUnits->Wrap( -1 );
	fgSizer1->Add( m_textThickUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textHeightLabel = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("Text &height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textHeightLabel->Wrap( -1 );
	fgSizer1->Add( m_textHeightLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textHeightCtrl = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textHeightCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_textHeightUnits = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textHeightUnits->Wrap( -1 );
	fgSizer1->Add( m_textHeightUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textWidthLabel = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("Text &width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textWidthLabel->Wrap( -1 );
	fgSizer1->Add( m_textWidthLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textWidthCtrl = new TEXT_CTRL_EVAL( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_textWidthUnits = new wxStaticText( sbSizerNewGraphicItems->GetStaticBox(), wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_textWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerNewGraphicItems->Add( fgSizer1, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );
	
	
	bSizerMargins->Add( sbSizerNewGraphicItems, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
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
	
	fgSizer2->Add( m_textCtrlRefText, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );
	
	wxString m_choiceLayerReferenceChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerReferenceNChoices = sizeof( m_choiceLayerReferenceChoices ) / sizeof( wxString );
	m_choiceLayerReference = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerReferenceNChoices, m_choiceLayerReferenceChoices, 0 );
	m_choiceLayerReference->SetSelection( 0 );
	fgSizer2->Add( m_choiceLayerReference, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
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
	m_textCtrlValueText->SetMinSize( wxSize( 160,-1 ) );
	
	fgSizer2->Add( m_textCtrlValueText, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );
	
	wxString m_choiceLayerValueChoices[] = { _("SilkScreen"), _("Fab. Layer") };
	int m_choiceLayerValueNChoices = sizeof( m_choiceLayerValueChoices ) / sizeof( wxString );
	m_choiceLayerValue = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceLayerValueNChoices, m_choiceLayerValueChoices, 0 );
	m_choiceLayerValue->SetSelection( 1 );
	fgSizer2->Add( m_choiceLayerValue, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxString m_choiceVisibleValueChoices[] = { _("Visible"), _("Invisible") };
	int m_choiceVisibleValueNChoices = sizeof( m_choiceVisibleValueChoices ) / sizeof( wxString );
	m_choiceVisibleValue = new wxChoice( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVisibleValueNChoices, m_choiceVisibleValueChoices, 0 );
	m_choiceVisibleValue->SetSelection( 0 );
	fgSizer2->Add( m_choiceVisibleValue, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerNewFootprints->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 5 );
	
	m_staticTextInfo = new wxStaticText( sbSizerNewFootprints->GetStaticBox(), wxID_ANY, _("Leave reference and/or value blank to use footprint name."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextInfo->Wrap( -1 );
	sbSizerNewFootprints->Add( m_staticTextInfo, 0, wxTOP|wxBOTTOM, 5 );
	
	
	bSizerMargins->Add( sbSizerNewFootprints, 0, wxEXPAND|wxTOP|wxLEFT, 5 );
	
	
	bSizerMain->Add( bSizerMargins, 0, wxEXPAND|wxTOP|wxLEFT, 10 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_MODEDIT_DEFAULTS_BASE::~PANEL_MODEDIT_DEFAULTS_BASE()
{
}
