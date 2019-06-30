///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	
	bPanelSizer->Add( m_galOptionsSizer, 1, wxEXPAND|wxLEFT, 5 );
	
	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimensions") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer32;
	fgSizer32 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer32->AddGrowableCol( 1 );
	fgSizer32->SetFlexibleDirection( wxBOTH );
	fgSizer32->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_busWidthLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("&Bus thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_busWidthLabel->Wrap( -1 );
	fgSizer32->Add( m_busWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_busWidthCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer32->Add( m_busWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_busWidthUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_busWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_busWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_wireWidthLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("&Wire thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wireWidthLabel->Wrap( -1 );
	fgSizer32->Add( m_wireWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_wireWidthCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxSP_WRAP );
	fgSizer32->Add( m_wireWidthCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_wireWidthUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_wireWidthUnits->Wrap( -1 );
	fgSizer32->Add( m_wireWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_jctSizeLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Junction size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_jctSizeLabel->Wrap( -1 );
	fgSizer32->Add( m_jctSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_jctSizeCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer32->Add( m_jctSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_jctSizeUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_jctSizeUnits->Wrap( -1 );
	fgSizer32->Add( m_jctSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( fgSizer32, 0, wxBOTTOM|wxRIGHT|wxEXPAND, 5 );
	
	
	bRightColumn->Add( sbSizer2, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Annotations") ), wxVERTICAL );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText26 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Symbol unit notation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	bSizer6->Add( m_staticText26, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_choiceSeparatorRefIdChoices[] = { _("A"), _(".A"), _("-A"), _("_A"), _(".1"), _("-1"), _("_1") };
	int m_choiceSeparatorRefIdNChoices = sizeof( m_choiceSeparatorRefIdChoices ) / sizeof( wxString );
	m_choiceSeparatorRefId = new wxChoice( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSeparatorRefIdNChoices, m_choiceSeparatorRefIdChoices, 0 );
	m_choiceSeparatorRefId->SetSelection( 0 );
	bSizer6->Add( m_choiceSeparatorRefId, 1, wxEXPAND|wxRIGHT, 5 );
	
	
	sbSizer1->Add( bSizer6, 0, wxEXPAND|wxRIGHT, 5 );
	
	
	sbSizer1->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );
	
	m_checkShowHiddenPins = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxALL, 5 );
	
	m_checkPageLimits = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show page limi&ts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkPageLimits->SetValue(true); 
	sbSizer1->Add( m_checkPageLimits, 0, wxEXPAND|wxALL, 5 );
	
	
	bRightColumn->Add( sbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	
	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	
	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::~PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE()
{
}
