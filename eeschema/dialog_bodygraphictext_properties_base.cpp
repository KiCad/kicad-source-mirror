///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_bodygraphictext_properties_base.h"

///////////////////////////////////////////////////////////////////////////

Dialog_BodyGraphicText_Properties_base::Dialog_BodyGraphicText_Properties_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bLeftSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextValue->SetMinSize( wxSize( 200,-1 ) );
	
	bLeftSizer->Add( m_TextValue, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sOptionsSizer;
	sOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _(" Text Options : ") ), wxVERTICAL );
	
	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to Units"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sOptionsSizer->Add( m_CommonUnit, 0, wxALL, 5 );
	
	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to convert"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sOptionsSizer->Add( m_CommonConvert, 0, wxALL, 5 );
	
	m_Orient = new wxCheckBox( this, wxID_ANY, _("Vertical"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sOptionsSizer->Add( m_Orient, 0, wxALL, 5 );
	
	bLeftSizer->Add( sOptionsSizer, 0, 0, 5 );
	
	bMainSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_TextSizeText = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeText->Wrap( -1 );
	bRightSizer->Add( m_TextSizeText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_TextSize, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_TextShapeOptChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_TextShapeOptNChoices = sizeof( m_TextShapeOptChoices ) / sizeof( wxString );
	m_TextShapeOpt = new wxRadioBox( this, wxID_ANY, _("Text Shape:"), wxDefaultPosition, wxDefaultSize, m_TextShapeOptNChoices, m_TextShapeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShapeOpt->SetSelection( 3 );
	bRightSizer->Add( m_TextShapeOpt, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( bRightSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_buttonOK, 0, wxALL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_buttonCANCEL, 0, wxALL, 5 );
	
	bMainSizer->Add( bSizer4, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( Dialog_BodyGraphicText_Properties_base::OnInitDialog ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( Dialog_BodyGraphicText_Properties_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( Dialog_BodyGraphicText_Properties_base::OnCancelClick ), NULL, this );
}

Dialog_BodyGraphicText_Properties_base::~Dialog_BodyGraphicText_Properties_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( Dialog_BodyGraphicText_Properties_base::OnInitDialog ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( Dialog_BodyGraphicText_Properties_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( Dialog_BodyGraphicText_Properties_base::OnCancelClick ), NULL, this );
}
