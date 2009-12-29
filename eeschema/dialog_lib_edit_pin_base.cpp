///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_edit_pin_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_EDIT_PIN_BASE::DIALOG_LIB_EDIT_PIN_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 5, 6, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Pin &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("N&ame text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textNameTextSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textNameTextSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticNameTextSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNameTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticNameTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Pin n&umber:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	m_staticText4->SetToolTip( _("Pin number: 1 to 4 ASCII letters and/or digits") );
	
	fgSizer1->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textNumber = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textNumber, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Number te&xt size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer1->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textNumberTextSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textNumberTextSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticNumberTextSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticNumberTextSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticNumberTextSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("&Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxArrayString m_choiceOrientationChoices;
	m_choiceOrientation = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceOrientationChoices, 0 );
	m_choiceOrientation->SetSelection( 0 );
	fgSizer1->Add( m_choiceOrientation, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 15, 0, 1, wxEXPAND, 3 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("&Length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer1->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textLength = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textLength, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticLengthUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticLengthUnits->Wrap( -1 );
	fgSizer1->Add( m_staticLengthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("&Electrical type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Used by the ERC.") );
	
	fgSizer1->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxArrayString m_choiceElectricalTypeChoices;
	m_choiceElectricalType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceElectricalTypeChoices, 0 );
	m_choiceElectricalType->SetSelection( 0 );
	fgSizer1->Add( m_choiceElectricalType, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 0, wxEXPAND, 3 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Graphic &Style:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer1->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	wxArrayString m_choiceStyleChoices;
	m_choiceStyle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceStyleChoices, 0 );
	m_choiceStyle->SetSelection( 0 );
	fgSizer1->Add( m_choiceStyle, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 3 );
	
	mainSizer->Add( fgSizer1, 0, wxALL|wxEXPAND, 12 );
	
	wxBoxSizer* boarderSizer;
	boarderSizer = new wxBoxSizer( wxVERTICAL );
	
	m_checkApplyToAllParts = new wxCheckBox( this, wxID_ANY, _("Add to all &parts in package"), wxDefaultPosition, wxDefaultSize, 0 );
	
	boarderSizer->Add( m_checkApplyToAllParts, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_checkApplyToAllConversions = new wxCheckBox( this, wxID_ANY, _("Add to all alternate &body styles (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	
	boarderSizer->Add( m_checkApplyToAllConversions, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_checkShow = new wxCheckBox( this, wxID_ANY, _("&Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShow->SetValue(true);
	
	boarderSizer->Add( m_checkShow, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	boarderSizer->Add( 0, 5, 0, wxALL|wxEXPAND, 10 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	boarderSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 0 );
	
	mainSizer->Add( boarderSizer, 0, wxALL|wxEXPAND, 12 );
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_EDIT_PIN_BASE::~DIALOG_LIB_EDIT_PIN_BASE()
{
}
