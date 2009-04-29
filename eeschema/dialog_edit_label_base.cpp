///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_edit_label_base.h"

///////////////////////////////////////////////////////////////////////////

DialogLabelEditor_Base::DialogLabelEditor_Base( wxWindow* parent, wxWindowID id, bool multiline, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
  m_staticText1 = new wxStaticText( this, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
  m_staticText1->Wrap( -1 );
	bSizer2->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	if (multiline)
  {
   	m_TextLabel = new wxTextCtrl( this, wxID_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxTE_MULTILINE );
  }
   else
  {
 	  m_TextLabel = new wxTextCtrl( this, wxID_VALUE, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
 	}
	m_TextLabel->SetToolTip( _("Enter the text to be used within the schematic") );
	
	bSizer2->Add( m_TextLabel, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxBoxSizer* m_OptionsSizer;
	m_OptionsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_TextOrientChoices[] = { _("Right"), _("Up"), _("Left"), _("Down") };
	int m_TextOrientNChoices = sizeof( m_TextOrientChoices ) / sizeof( wxString );
	m_TextOrient = new wxRadioBox( this, wxID_ANY, _("Direction"), wxDefaultPosition, wxDefaultSize, m_TextOrientNChoices, m_TextOrientChoices, 1, wxRA_SPECIFY_COLS );
	m_TextOrient->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextOrient, 1, wxALL, 5 );
	
	wxString m_TextStyleChoices[] = { _("Normal"), _("Italic"), _("Bold"), _("Bold Italic") };
	int m_TextStyleNChoices = sizeof( m_TextStyleChoices ) / sizeof( wxString );
	m_TextStyle = new wxRadioBox( this, wxID_ANY, _("Style"), wxDefaultPosition, wxDefaultSize, m_TextStyleNChoices, m_TextStyleChoices, 1, wxRA_SPECIFY_COLS );
	m_TextStyle->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextStyle, 1, wxALL, 5 );
	
	wxString m_TextShapeChoices[] = { _("Input"), _("Output"), _("Bidi"), _("TriState"), _("Passive") };
	int m_TextShapeNChoices = sizeof( m_TextShapeChoices ) / sizeof( wxString );
	m_TextShape = new wxRadioBox( this, wxID_ANY, _("Glabel Shape"), wxDefaultPosition, wxDefaultSize, m_TextShapeNChoices, m_TextShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_TextShape->SetSelection( 0 );
	m_OptionsSizer->Add( m_TextShape, 1, wxALL, 5 );
	
	bSizer2->Add( m_OptionsSizer, 1, wxEXPAND, 5 );
	
	bMainSizer->Add( bSizer2, 5, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_SizeTitle = new wxStaticText( this, wxID_ANY, _("Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeTitle->Wrap( -1 );
	bSizer4->Add( m_SizeTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TextSize = new wxTextCtrl( this, wxID_SIZE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_TextSize, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer4->Add( 8, 8, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_buttonOK, 1, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_buttonCANCEL, 1, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bMainSizer->Add( bSizer4, 1, 0, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_TextLabel->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DialogLabelEditor_Base::onEnterKey ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLabelEditor_Base::OnButtonOKClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLabelEditor_Base::OnButtonCANCEL_Click ), NULL, this );
}

DialogLabelEditor_Base::~DialogLabelEditor_Base()
{
	// Disconnect Events
	m_TextLabel->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DialogLabelEditor_Base::onEnterKey ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLabelEditor_Base::OnButtonOKClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLabelEditor_Base::OnButtonCANCEL_Click ), NULL, this );
}
