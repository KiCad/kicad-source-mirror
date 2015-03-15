///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sch_edit_sheet_pin_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_EDIT_SHEET_PIN_BASE::DIALOG_SCH_EDIT_SHEET_PIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 350,-1 ), wxDefaultSize );
	
	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_nameSizer;
	m_nameSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 6 );
	
	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textName->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 6 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 6 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Text height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	m_textHeight = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textHeight->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textHeight, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );
	
	m_staticHeightUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticHeightUnits->Wrap( -1 );
	fgSizer1->Add( m_staticHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Text width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	m_textWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textWidth->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textWidth, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 6 );
	
	m_staticWidthUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticWidthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Connection type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	wxArrayString m_choiceConnectionTypeChoices;
	m_choiceConnectionType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceConnectionTypeChoices, 0 );
	m_choiceConnectionType->SetSelection( 0 );
	fgSizer1->Add( m_choiceConnectionType, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 6 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 6 );
	
	
	m_nameSizer->Add( fgSizer1, 1, wxEXPAND, 6 );
	
	
	m_mainSizer->Add( m_nameSizer, 1, wxALL|wxEXPAND, 6 );
	
	
	m_mainSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 3 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	m_mainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 6 );
	
	
	this->SetSizer( m_mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_SCH_EDIT_SHEET_PIN_BASE::~DIALOG_SCH_EDIT_SHEET_PIN_BASE()
{
}
