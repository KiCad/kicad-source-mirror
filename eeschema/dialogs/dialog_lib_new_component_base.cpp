///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 11 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_lib_new_component_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LIB_NEW_COMPONENT_BASE::DIALOG_LIB_NEW_COMPONENT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("General Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer5->Add( m_staticText6, 0, wxALIGN_LEFT, 3 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer2->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Component &name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetToolTip( _("This is the component name in library,\nand also the default component value when loaded in the schematic.") );
	
	bSizer2->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer2->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), 0 );
	bSizer2->Add( m_textName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer2->Add( 30, 0, 0, wxEXPAND, 3 );
	
	
	bSizer5->Add( bSizer2, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer3->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Default &reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->SetToolTip( _("This is the reference used in schematic for annotation.\nDo not use digits in reference.") );
	
	bSizer3->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textReference = new wxTextCtrl( this, wxID_ANY, _("U"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	bSizer3->Add( m_textReference, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer3->Add( 30, 0, 0, wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer3, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer4->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Number of &parts per package:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	m_staticText4->SetToolTip( _("This is the number of parts in this component package.\nA 74LS00 gate has 4 parts per packages.") );
	
	bSizer4->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer4->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_spinPartCount = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 1, 26, 0 );
	bSizer4->Add( m_spinPartCount, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer4->Add( 30, 0, 0, wxEXPAND, 3 );
	
	
	bSizer5->Add( bSizer4, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer7->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkHasConversion = new wxCheckBox( this, wxID_ANY, _("Create component with &alternate body style (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkHasConversion->SetToolTip( _("Check this option for components that have a De Morgan representation.\nThis is usual for gates.") );
	
	bSizer7->Add( m_checkHasConversion, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer7, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer8->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkIsPowerSymbol = new wxCheckBox( this, wxID_ANY, _("Create component as power &symbol"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkIsPowerSymbol->SetToolTip( _("Check this option for power symbols.\nPower symbols have specific properties for Eeschema:\n- Value cannot be edited (to avoid mistakes) because this is the pin name that is important for a power symbol\n- Reference is updated automatically when a netlist is created (no need to run Annotate)") );
	
	bSizer8->Add( m_checkIsPowerSymbol, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer8, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer9->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkLockItems = new wxCheckBox( this, wxID_ANY, _("Parts in package locked (cannot be swapped)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkLockItems->SetToolTip( _("Check this option if Eeschema cannot change parts selections inside a given package\nThis happens when parts are different in this package.\nWhen this option is not checked, Eeschema automatically choose the parts in packages to minimize packages count") );
	
	bSizer9->Add( m_checkLockItems, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer9, 0, wxALL|wxEXPAND, 0 );
	
	
	bSizer5->Add( 0, 0, 0, wxALL|wxEXPAND, 10 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Global Pin Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer5->Add( m_staticText7, 0, wxALIGN_LEFT|wxBOTTOM, 3 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer6->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_staticText41 = new wxStaticText( this, wxID_ANY, _("Pin text position &offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	m_staticText41->SetToolTip( _("Margin (in 0.001 inches) between a pin name position and the component body.\nA value from 10 to 40 is usually good.") );
	
	bSizer6->Add( m_staticText41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer6->Add( 0, 0, 1, wxEXPAND, 3 );
	
	m_spinPinTextPosition = new wxSpinCtrl( this, wxID_ANY, wxT("40"), wxDefaultPosition, wxSize( 100,-1 ), wxSP_ARROW_KEYS, 1, 100, 40 );
	bSizer6->Add( m_spinPinTextPosition, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText5->Wrap( -1 );
	bSizer6->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL, 3 );
	
	
	bSizer5->Add( bSizer6, 1, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer10->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkShowPinNumber = new wxCheckBox( this, wxID_ANY, _("Show pin n&umber text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNumber->SetValue(true); 
	bSizer10->Add( m_checkShowPinNumber, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer10, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer12->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkShowPinName = new wxCheckBox( this, wxID_ANY, _("Show pin name te&xt"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinName->SetValue(true); 
	bSizer12->Add( m_checkShowPinName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer12, 0, wxALL|wxEXPAND, 0 );
	
	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer121->Add( 12, 0, 0, wxEXPAND, 3 );
	
	m_checkShowPinNameInside = new wxCheckBox( this, wxID_ANY, _("Pin name &inside"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkShowPinNameInside->SetValue(true); 
	bSizer121->Add( m_checkShowPinNameInside, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	bSizer5->Add( bSizer121, 1, wxEXPAND, 5 );
	
	
	bSizer5->Add( 0, 5, 0, wxALL|wxEXPAND, 10 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizer5->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 0 );
	
	
	mainSizer->Add( bSizer5, 1, wxALL|wxEXPAND, 12 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_LIB_NEW_COMPONENT_BASE::~DIALOG_LIB_NEW_COMPONENT_BASE()
{
}
