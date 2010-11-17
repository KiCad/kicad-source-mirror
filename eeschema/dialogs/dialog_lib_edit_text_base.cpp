///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_text_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_TEXT_BASE::DIALOG_LIB_EDIT_TEXT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bPropertiesSizer;
	bPropertiesSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperBoxSizer;
	bUpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bTextValueBoxSizer;
	bTextValueBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bTextValueBoxSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TextValue->SetMinSize( wxSize( 200,-1 ) );
	
	bTextValueBoxSizer->Add( m_TextValue, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bUpperBoxSizer->Add( bTextValueBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bTextSizeSizer;
	bTextSizeSizer = new wxBoxSizer( wxVERTICAL );
	
	m_TextSizeText = new wxStaticText( this, wxID_ANY, _("Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeText->Wrap( -1 );
	bTextSizeSizer->Add( m_TextSizeText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bTextSizeSizer->Add( m_TextSize, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bUpperBoxSizer->Add( bTextSizeSizer, 0, 0, 5 );
	
	bPropertiesSizer->Add( bUpperBoxSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bBottomtBoxSizer;
	bBottomtBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sOptionsSizer;
	sOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _(" Text Options : ") ), wxVERTICAL );
	
	m_Orient = new wxCheckBox( this, wxID_ANY, _("Vertical"), wxDefaultPosition, wxDefaultSize, 0 );
	sOptionsSizer->Add( m_Orient, 0, wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sOptionsSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_CommonUnit = new wxCheckBox( this, wxID_ANY, _("Common to Units"), wxDefaultPosition, wxDefaultSize, 0 );
	sOptionsSizer->Add( m_CommonUnit, 0, wxALL, 5 );
	
	m_CommonConvert = new wxCheckBox( this, wxID_ANY, _("Common to convert"), wxDefaultPosition, wxDefaultSize, 0 );
	sOptionsSizer->Add( m_CommonConvert, 0, wxALL|wxEXPAND, 5 );
	
	bBottomtBoxSizer->Add( sOptionsSizer, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_TextShapeOptChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_TextShapeOptNChoices = sizeof( m_TextShapeOptChoices ) / sizeof( wxString );
	m_TextShapeOpt = new wxRadioBox( this, wxID_ANY, _("Text Shape:"), wxDefaultPosition, wxDefaultSize, m_TextShapeOptNChoices, m_TextShapeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShapeOpt->SetSelection( 0 );
	bBottomtBoxSizer->Add( m_TextShapeOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_TextHJustificationOptChoices[] = { _("Align left"), _("Align center"), _("Align right") };
	int m_TextHJustificationOptNChoices = sizeof( m_TextHJustificationOptChoices ) / sizeof( wxString );
	m_TextHJustificationOpt = new wxRadioBox( this, wxID_ANY, _("Horiz. Justify"), wxDefaultPosition, wxDefaultSize, m_TextHJustificationOptNChoices, m_TextHJustificationOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextHJustificationOpt->SetSelection( 1 );
	bBottomtBoxSizer->Add( m_TextHJustificationOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_TextVJustificationOptChoices[] = { _("Align bottom"), _("Align center"), _("Align top") };
	int m_TextVJustificationOptNChoices = sizeof( m_TextVJustificationOptChoices ) / sizeof( wxString );
	m_TextVJustificationOpt = new wxRadioBox( this, wxID_ANY, _("Vert. Justify"), wxDefaultPosition, wxDefaultSize, m_TextVJustificationOptNChoices, m_TextVJustificationOptChoices, 1, wxRA_SPECIFY_COLS );
	m_TextVJustificationOpt->SetSelection( 1 );
	bBottomtBoxSizer->Add( m_TextVJustificationOpt, 0, wxALL|wxEXPAND, 5 );
	
	bPropertiesSizer->Add( bBottomtBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	bPropertiesSizer->Add( bRightSizer, 0, wxEXPAND, 5 );
	
	bMainSizer->Add( bPropertiesSizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 6 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 12 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

DIALOG_LIB_EDIT_TEXT_BASE::~DIALOG_LIB_EDIT_TEXT_BASE()
{
}
