///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pcb_text_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PCB_TEXT_PROPERTIES_BASE::DIALOG_PCB_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_TextLabel = new wxStaticText( this, wxID_ANY, _("Text content:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextLabel->Wrap( -1 );
	bMainSizer->Add( m_TextLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextContentCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_TextContentCtrl->SetToolTip( _("Enter the text placed on selected layer.") );
	m_TextContentCtrl->SetMinSize( wxSize( 400,60 ) );
	
	bMainSizer->Add( m_TextContentCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxBoxSizer* bSizerLower;
	bSizerLower = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_SizeXLabel = new wxStaticText( this, wxID_ANY, _("Size X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXLabel->Wrap( -1 );
	bSizerLeft->Add( m_SizeXLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_SizeXCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_SizeYLabel = new wxStaticText( this, wxID_ANY, _("Size Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYLabel->Wrap( -1 );
	bSizerLeft->Add( m_SizeYLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_SizeYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	bSizerLeft->Add( m_ThicknessLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_ThicknessCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_PositionXLabel = new wxStaticText( this, wxID_ANY, _("Position X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionXLabel->Wrap( -1 );
	bSizerLeft->Add( m_PositionXLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	m_PositionXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_PositionXCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_PositionYLabel = new wxStaticText( this, wxID_ANY, _("Position Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionYLabel->Wrap( -1 );
	bSizerLeft->Add( m_PositionYLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	m_PositionYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeft->Add( m_PositionYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	bSizerLeft->Add( m_LayerLabel, 0, wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_LayerSelectionCtrlChoices;
	m_LayerSelectionCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_LayerSelectionCtrlChoices, 0 );
	m_LayerSelectionCtrl->SetSelection( 0 );
	m_LayerSelectionCtrl->SetToolTip( _("Select the layer on which text should lay.") );
	
	bSizerLeft->Add( m_LayerSelectionCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizerLower->Add( bSizerLeft, 1, 0, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	wxString m_OrientationCtrlChoices[] = { _("0"), _("90"), _("180"), _("-90") };
	int m_OrientationCtrlNChoices = sizeof( m_OrientationCtrlChoices ) / sizeof( wxString );
	m_OrientationCtrl = new wxRadioBox( this, wxID_ANY, _("Orientation"), wxDefaultPosition, wxDefaultSize, m_OrientationCtrlNChoices, m_OrientationCtrlChoices, 2, wxRA_SPECIFY_COLS );
	m_OrientationCtrl->SetSelection( 0 );
	bSizerRight->Add( m_OrientationCtrl, 0, wxALL|wxEXPAND, 3 );
	
	wxString m_StyleCtrlChoices[] = { _("Normal"), _("Italic") };
	int m_StyleCtrlNChoices = sizeof( m_StyleCtrlChoices ) / sizeof( wxString );
	m_StyleCtrl = new wxRadioBox( this, wxID_ANY, _("Style"), wxDefaultPosition, wxDefaultSize, m_StyleCtrlNChoices, m_StyleCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_StyleCtrl->SetSelection( 0 );
	bSizerRight->Add( m_StyleCtrl, 0, wxALL|wxEXPAND, 3 );
	
	wxString m_DisplayCtrlChoices[] = { _("Normal"), _("Mirror") };
	int m_DisplayCtrlNChoices = sizeof( m_DisplayCtrlChoices ) / sizeof( wxString );
	m_DisplayCtrl = new wxRadioBox( this, wxID_ANY, _("Display"), wxDefaultPosition, wxDefaultSize, m_DisplayCtrlNChoices, m_DisplayCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_DisplayCtrl->SetSelection( 0 );
	bSizerRight->Add( m_DisplayCtrl, 0, wxALL|wxEXPAND, 3 );
	
	bSizerLower->Add( bSizerRight, 0, 0, 5 );
	
	bMainSizer->Add( bSizerLower, 0, wxEXPAND, 5 );
	
	m_StandardSizer = new wxStdDialogButtonSizer();
	m_StandardSizerOK = new wxButton( this, wxID_OK );
	m_StandardSizer->AddButton( m_StandardSizerOK );
	m_StandardSizerCancel = new wxButton( this, wxID_CANCEL );
	m_StandardSizer->AddButton( m_StandardSizerCancel );
	m_StandardSizer->Realize();
	bMainSizer->Add( m_StandardSizer, 0, wxALIGN_BOTTOM|wxALIGN_RIGHT|wxALL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_StandardSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCB_TEXT_PROPERTIES_BASE::OnCancelClick ), NULL, this );
	m_StandardSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCB_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
}

DIALOG_PCB_TEXT_PROPERTIES_BASE::~DIALOG_PCB_TEXT_PROPERTIES_BASE()
{
	// Disconnect Events
	m_StandardSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCB_TEXT_PROPERTIES_BASE::OnCancelClick ), NULL, this );
	m_StandardSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PCB_TEXT_PROPERTIES_BASE::OnOkClick ), NULL, this );
	
}
