///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_new_component_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_NEW_COMPONENT_BASE::DIALOG_LIB_NEW_COMPONENT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("General Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	m_staticText8->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer16->Add( m_staticText8, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Component &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetToolTip( _("This is the component name in library,\nand also the default component value when loaded in the schematic.") );
	
	fgSizer31->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), 0 );
	m_textName->SetMaxLength( 0 ); 
	fgSizer31->Add( m_textName, 1, wxALL|wxEXPAND, 3 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Default reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer31->Add( m_staticText9, 0, wxALL, 5 );
	
	m_textReference = new wxTextCtrl( this, wxID_ANY, _("U"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_textReference, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Number of units per package:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer31->Add( m_staticText10, 0, wxALL, 5 );
	
	m_spinPartCount = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 26, 0 );
	fgSizer31->Add( m_spinPartCount, 0, wxALL, 5 );
	
	
	bSizer16->Add( fgSizer31, 1, wxEXPAND|wxLEFT|wxRIGHT, 20 );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	m_checkHasConversion = new wxCheckBox( this, wxID_ANY, _("Create component with alternate body style (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkHasConversion, 0, wxALL, 5 );
	
	m_checkIsPowerSymbol = new wxCheckBox( this, wxID_ANY, _("Create component as power symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkIsPowerSymbol, 0, wxALL, 5 );
	
	m_checkLockItems = new wxCheckBox( this, wxID_ANY, _("Units are not interchangeable"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer17->Add( m_checkLockItems, 0, wxALL, 5 );
	
	
	bSizer16->Add( bSizer17, 1, wxEXPAND|wxLEFT|wxRIGHT, 15 );
	
	
	bSizer7->Add( bSizer16, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer18->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("General Pin Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_staticText11->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer18->Add( m_staticText11, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 0, 55 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Pin text position offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer4->Add( m_staticText12, 0, wxALL, 5 );
	
	m_spinPinTextPosition = new wxSpinCtrl( this, wxID_ANY, wxT("40"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 40 );
	fgSizer4->Add( m_spinPinTextPosition, 0, wxALL, 5 );
	
	
	bSizer18->Add( fgSizer4, 0, wxLEFT|wxRIGHT, 20 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_checkShowPinNumber = new wxCheckBox( this, wxID_ANY, _("Show pin number text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNumber->SetValue(true); 
	bSizer19->Add( m_checkShowPinNumber, 0, wxALL, 5 );
	
	m_checkShowPinName = new wxCheckBox( this, wxID_ANY, _("Show pin name text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinName->SetValue(true); 
	bSizer19->Add( m_checkShowPinName, 0, wxALL, 5 );
	
	m_checkShowPinNameInside = new wxCheckBox( this, wxID_ANY, _("Pin name inside"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNameInside->SetValue(true); 
	bSizer19->Add( m_checkShowPinNameInside, 0, wxALL, 5 );
	
	
	bSizer18->Add( bSizer19, 0, wxEXPAND|wxLEFT|wxRIGHT, 15 );
	
	
	bSizer7->Add( bSizer18, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizer7->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 10 );
	
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_NEW_COMPONENT_BASE::~DIALOG_LIB_NEW_COMPONENT_BASE()
{
}
