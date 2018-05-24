///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_text_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_TEXT_BASE::DIALOG_LIB_EDIT_TEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bPropertiesSizer;
	bPropertiesSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperBoxSizer;
	bUpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bTextValueBoxSizer;
	bTextValueBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bTextValueBoxSizer->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_TextValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextValue->SetMinSize( wxSize( 200,-1 ) );
	
	bTextValueBoxSizer->Add( m_TextValue, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_TextValueSelectButton = new wxButton( this, wxID_ANY, _("Select"), wxDefaultPosition, wxDefaultSize, 0 );
	bTextValueBoxSizer->Add( m_TextValueSelectButton, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bUpperBoxSizer->Add( bTextValueBoxSizer, 1, wxEXPAND, 5 );
	
	
	bPropertiesSizer->Add( bUpperBoxSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_PowerComponentValues = new wxStaticText( this, wxID_ANY, _("Note: power symbol value text cannot be modified."), wxDefaultPosition, wxDefaultSize, 0 );
	m_PowerComponentValues->Wrap( -1 );
	m_PowerComponentValues->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer9->Add( m_PowerComponentValues, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bPropertiesSizer->Add( bSizer9, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bBottomtBoxSizer;
	bBottomtBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer8->Add( 0, 0, 1, wxEXPAND|wxTOP, 2 );
	
	wxBoxSizer* bTextSizeSizer;
	bTextSizeSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	bTextSizeSizer->Add( m_textSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bTextSizeSizer->Add( m_textSizeCtrl, 1, wxEXPAND|wxALL, 5 );
	
	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	bTextSizeSizer->Add( m_textSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	bSizer8->Add( bTextSizeSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_visible = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_visible, 0, wxALIGN_LEFT|wxBOTTOM|wxLEFT, 5 );
	
	m_Orient = new wxCheckBox( this, wxID_ANY, _("Vertical"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_Orient, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to all units"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_CommonUnit, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to all body styles"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_CommonConvert, 0, wxALIGN_RIGHT|wxBOTTOM|wxEXPAND|wxLEFT, 5 );
	
	
	bBottomtBoxSizer->Add( bSizer8, 4, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bBottomtBoxSizer->Add( 0, 0, 0, wxEXPAND|wxLEFT, 5 );
	
	wxString m_TextShapeOptChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold and italic") };
	int m_TextShapeOptNChoices = sizeof( m_TextShapeOptChoices ) / sizeof( wxString );
	m_TextShapeOpt = new wxRadioBox( this, wxID_ANY, _("Text Style"), wxDefaultPosition, wxDefaultSize, m_TextShapeOptNChoices, m_TextShapeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShapeOpt->SetSelection( 3 );
	bBottomtBoxSizer->Add( m_TextShapeOpt, 3, wxALL|wxEXPAND, 5 );
	
	wxString m_TextHJustificationOptChoices[] = { _("Align left"), _("Align center"), _("Align right") };
	int m_TextHJustificationOptNChoices = sizeof( m_TextHJustificationOptChoices ) / sizeof( wxString );
	m_TextHJustificationOpt = new wxRadioBox( this, wxID_ANY, _("Horizontal Align"), wxDefaultPosition, wxDefaultSize, m_TextHJustificationOptNChoices, m_TextHJustificationOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextHJustificationOpt->SetSelection( 0 );
	bBottomtBoxSizer->Add( m_TextHJustificationOpt, 3, wxALL|wxEXPAND, 5 );
	
	wxString m_TextVJustificationOptChoices[] = { _("Align top"), _("Align center"), _("Align bottom") };
	int m_TextVJustificationOptNChoices = sizeof( m_TextVJustificationOptChoices ) / sizeof( wxString );
	m_TextVJustificationOpt = new wxRadioBox( this, wxID_ANY, _("Vertical Align"), wxDefaultPosition, wxDefaultSize, m_TextVJustificationOptNChoices, m_TextVJustificationOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextVJustificationOpt->SetSelection( 0 );
	bBottomtBoxSizer->Add( m_TextVJustificationOpt, 3, wxALL|wxEXPAND, 5 );
	
	
	bPropertiesSizer->Add( bBottomtBoxSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bPropertiesSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 6 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	bMainSizer->Add( m_sdbSizerButtons, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnCloseDialog ) );
	m_TextValueSelectButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextValueSelectButtonClick ), NULL, this );
}

DIALOG_LIB_EDIT_TEXT_BASE::~DIALOG_LIB_EDIT_TEXT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnCloseDialog ) );
	m_TextValueSelectButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LIB_EDIT_TEXT_BASE::OnTextValueSelectButtonClick ), NULL, this );
	
}
