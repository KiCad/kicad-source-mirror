///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_spice_model_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SPICE_MODEL_BASE::DIALOG_SPICE_MODEL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_passive = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText2 = new wxStaticText( m_passive, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pasType = new wxComboBox( m_passive, wxID_ANY, _("Resistor"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_pasType->Append( _("Resistor") );
	m_pasType->Append( _("Capacitor") );
	m_pasType->Append( _("Inductor") );
	m_pasType->SetSelection( 0 );
	fgSizer1->Add( m_pasType, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText62 = new wxStaticText( m_passive, wxID_ANY, _("Passive type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText62->Wrap( -1 );
	fgSizer1->Add( m_staticText62, 0, wxALL, 5 );
	
	m_staticText3 = new wxStaticText( m_passive, wxID_ANY, _("Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pasValue = new wxTextCtrl( m_passive, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pasValue->SetMinSize( wxSize( 200,-1 ) );
	
	fgSizer1->Add( m_pasValue, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText63 = new wxStaticText( m_passive, wxID_ANY, _("Spice value in simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText63->Wrap( -1 );
	fgSizer1->Add( m_staticText63, 0, wxALL, 5 );
	
	
	bSizer41->Add( fgSizer1, 0, wxEXPAND|wxALL, 5 );
	
	m_staticline1 = new wxStaticLine( m_passive, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer41->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText32 = new wxStaticText( m_passive, wxID_ANY, _("In Spice values,the decimal separator is the point.\nValues can use Spice unit symbols."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	bSizer5->Add( m_staticText32, 0, wxALL, 5 );
	
	m_staticText321 = new wxStaticText( m_passive, wxID_ANY, _("Spice unit symbols in values (case insensitive):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	bSizer5->Add( m_staticText321, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerUnitSymbols;
	fgSizerUnitSymbols = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerUnitSymbols->AddGrowableCol( 1 );
	fgSizerUnitSymbols->SetFlexibleDirection( wxBOTH );
	fgSizerUnitSymbols->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText341 = new wxStaticText( m_passive, wxID_ANY, _("f"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText341->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText341, 0, wxALL, 5 );
	
	m_staticText351 = new wxStaticText( m_passive, wxID_ANY, _("fempto"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText351->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText351, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText36 = new wxStaticText( m_passive, wxID_ANY, _("1e-15"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText36->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText36, 0, wxALL, 5 );
	
	m_staticText37 = new wxStaticText( m_passive, wxID_ANY, _("p"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText37, 0, wxALL, 5 );
	
	m_staticText38 = new wxStaticText( m_passive, wxID_ANY, _("pico"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText38, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText39 = new wxStaticText( m_passive, wxID_ANY, _("1e-12"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText39->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText39, 0, wxALL, 5 );
	
	m_staticText40 = new wxStaticText( m_passive, wxID_ANY, _("n"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText40->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText40, 0, wxALL, 5 );
	
	m_staticText41 = new wxStaticText( m_passive, wxID_ANY, _("nano"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText41, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText42 = new wxStaticText( m_passive, wxID_ANY, _("1e-9"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText42->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText42, 0, wxALL, 5 );
	
	m_staticText43 = new wxStaticText( m_passive, wxID_ANY, _("u"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText43->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText43, 0, wxALL, 5 );
	
	m_staticText44 = new wxStaticText( m_passive, wxID_ANY, _("micro"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText44, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText46 = new wxStaticText( m_passive, wxID_ANY, _("1e-6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText46->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText46, 0, wxALL, 5 );
	
	m_staticText47 = new wxStaticText( m_passive, wxID_ANY, _("m"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText47->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText47, 0, wxALL, 5 );
	
	m_staticText48 = new wxStaticText( m_passive, wxID_ANY, _("milli"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText48->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText48, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText45 = new wxStaticText( m_passive, wxID_ANY, _("1e-3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText45, 0, wxALL, 5 );
	
	m_staticText49 = new wxStaticText( m_passive, wxID_ANY, _("k"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText49->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText49, 0, wxALL, 5 );
	
	m_staticText50 = new wxStaticText( m_passive, wxID_ANY, _("kilo"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText50->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText50, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText51 = new wxStaticText( m_passive, wxID_ANY, _("1e3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText51, 0, wxALL, 5 );
	
	m_staticText52 = new wxStaticText( m_passive, wxID_ANY, _("meg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText52, 0, wxALL, 5 );
	
	m_staticText53 = new wxStaticText( m_passive, wxID_ANY, _("mega"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText53->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText53, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText54 = new wxStaticText( m_passive, wxID_ANY, _("1e6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText54->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText54, 0, wxALL, 5 );
	
	m_staticText55 = new wxStaticText( m_passive, wxID_ANY, _("g"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText55->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText55, 0, wxALL, 5 );
	
	m_staticText56 = new wxStaticText( m_passive, wxID_ANY, _("giga"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText56, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText57 = new wxStaticText( m_passive, wxID_ANY, _("1e9"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText57->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText57, 0, wxALL, 5 );
	
	m_staticText58 = new wxStaticText( m_passive, wxID_ANY, _("t"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText58->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText58, 0, wxALL, 5 );
	
	m_staticText59 = new wxStaticText( m_passive, wxID_ANY, _("tera"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText59->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText59, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText60 = new wxStaticText( m_passive, wxID_ANY, _("1e12"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText60->Wrap( -1 );
	fgSizerUnitSymbols->Add( m_staticText60, 0, wxALL, 5 );
	
	
	bSizer5->Add( fgSizerUnitSymbols, 1, wxEXPAND, 5 );
	
	
	bSizer41->Add( bSizer5, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_passive->SetSizer( bSizer41 );
	m_passive->Layout();
	bSizer41->Fit( m_passive );
	m_notebook->AddPage( m_passive, _("Passive"), true );
	m_semiconductor = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText4 = new wxStaticText( m_semiconductor, wxID_ANY, _("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer3->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_semiType = new wxComboBox( m_semiconductor, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_semiType->Append( _("Diode") );
	m_semiType->Append( _("Bipolar transistor (BJT)") );
	m_semiType->Append( _("MOSFET") );
	fgSizer3->Add( m_semiType, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer3->Add( 0, 0, 0, 0, 5 );
	
	m_staticText5 = new wxStaticText( m_semiconductor, wxID_ANY, _("Model"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer3->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_semiModel = new wxComboBox( m_semiconductor, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer3->Add( m_semiModel, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer3->Add( 0, 0, 0, 0, 5 );
	
	m_staticText7 = new wxStaticText( m_semiconductor, wxID_ANY, _("Library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer3->Add( m_staticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_semiLib = new wxTextCtrl( m_semiconductor, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer3->Add( m_semiLib, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	m_semiSelectLib = new wxButton( m_semiconductor, wxID_ANY, _("Select file..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_semiSelectLib, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_semiconductor->SetSizer( fgSizer3 );
	m_semiconductor->Layout();
	fgSizer3->Fit( m_semiconductor );
	m_notebook->AddPage( m_semiconductor, _("Semiconductor"), false );
	m_ic = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText8 = new wxStaticText( m_ic, wxID_ANY, _("Model"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer4->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_icModel = new wxComboBox( m_ic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_icModel->SetMinSize( wxSize( 200,-1 ) );
	
	fgSizer4->Add( m_icModel, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer4->Add( 0, 0, 0, 0, 5 );
	
	m_staticText9 = new wxStaticText( m_ic, wxID_ANY, _("Library"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizer4->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_icLib = new wxTextCtrl( m_ic, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer4->Add( m_icLib, 0, wxALL|wxEXPAND, 5 );
	
	m_icSelectLib = new wxButton( m_ic, wxID_ANY, _("Select file..."), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( m_icSelectLib, 0, wxALL, 5 );
	
	
	m_ic->SetSizer( fgSizer4 );
	m_ic->Layout();
	fgSizer4->Fit( m_ic );
	m_notebook->AddPage( m_ic, _("Integrated circuit"), false );
	m_power = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_power->SetMinSize( wxSize( 650,-1 ) );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_power, wxID_ANY, _("DC/AC analysis") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer6->AddGrowableCol( 1 );
	fgSizer6->AddGrowableCol( 3 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText10 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("DC [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	fgSizer6->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_genDc = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_genDc->SetMinSize( wxSize( 60,-1 ) );
	
	fgSizer6->Add( m_genDc, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText11 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("AC magnitude [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer6->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_genAcMag = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_genAcMag->SetMinSize( wxSize( 60,-1 ) );
	
	fgSizer6->Add( m_genAcMag, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText12 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("AC phase [rad]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer6->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_genAcPhase = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_genAcPhase->SetMinSize( wxSize( 60,-1 ) );
	
	fgSizer6->Add( m_genAcPhase, 0, wxALL|wxEXPAND, 5 );
	
	
	sbSizer1->Add( fgSizer6, 1, wxEXPAND, 5 );
	
	
	bSizer4->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_power, wxID_ANY, _("Transient analysis") ), wxVERTICAL );
	
	m_powerNotebook = new wxNotebook( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_pwrPulse = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer7->AddGrowableCol( 1 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText13 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Initial value [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	fgSizer7->Add( m_staticText13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseInit = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pulseInit->SetMinSize( wxSize( 100,-1 ) );
	
	fgSizer7->Add( m_pulseInit, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText14 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Pulsed value [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer7->Add( m_staticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseNominal = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulseNominal, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText15 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Delay time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer7->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseDelay = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulseDelay, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText16 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Rise time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer7->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseRise = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulseRise, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText17 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Fall time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText17->Wrap( -1 );
	fgSizer7->Add( m_staticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseFall = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulseFall, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText18 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Pulse width [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	fgSizer7->Add( m_staticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulseWidth = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulseWidth, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText20 = new wxStaticText( m_pwrPulse, wxID_ANY, _("Period [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer7->Add( m_staticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pulsePeriod = new wxTextCtrl( m_pwrPulse, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer7->Add( m_pulsePeriod, 0, wxALL|wxEXPAND, 5 );
	
	
	m_pwrPulse->SetSizer( fgSizer7 );
	m_pwrPulse->Layout();
	fgSizer7->Fit( m_pwrPulse );
	m_powerNotebook->AddPage( m_pwrPulse, _("Pulse"), true );
	m_pwrSin = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer8->AddGrowableCol( 1 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText21 = new wxStaticText( m_pwrSin, wxID_ANY, _("DC offset [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer8->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_sinOffset = new wxTextCtrl( m_pwrSin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_sinOffset->SetMinSize( wxSize( 100,-1 ) );
	
	fgSizer8->Add( m_sinOffset, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText22 = new wxStaticText( m_pwrSin, wxID_ANY, _("Amplitude [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer8->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_sinAmplitude = new wxTextCtrl( m_pwrSin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_sinAmplitude, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText23 = new wxStaticText( m_pwrSin, wxID_ANY, _("Frequency [Hz]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer8->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_sinFreq = new wxTextCtrl( m_pwrSin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_sinFreq, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText24 = new wxStaticText( m_pwrSin, wxID_ANY, _("Delay [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgSizer8->Add( m_staticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_sinDelay = new wxTextCtrl( m_pwrSin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_sinDelay, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText25 = new wxStaticText( m_pwrSin, wxID_ANY, _("Damping factor [1/s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgSizer8->Add( m_staticText25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_sinDampFactor = new wxTextCtrl( m_pwrSin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_sinDampFactor, 0, wxALL|wxEXPAND, 5 );
	
	
	m_pwrSin->SetSizer( fgSizer8 );
	m_pwrSin->Layout();
	fgSizer8->Fit( m_pwrSin );
	m_powerNotebook->AddPage( m_pwrSin, _("Sinusoidal"), false );
	m_pwrExp = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->AddGrowableCol( 1 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText26 = new wxStaticText( m_pwrExp, wxID_ANY, _("Initial value [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer9->Add( m_staticText26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expInit = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_expInit->SetMinSize( wxSize( 100,-1 ) );
	
	fgSizer9->Add( m_expInit, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText27 = new wxStaticText( m_pwrExp, wxID_ANY, _("Pulsed value [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	fgSizer9->Add( m_staticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expPulsed = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_expPulsed, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText28 = new wxStaticText( m_pwrExp, wxID_ANY, _("Rise delay time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	fgSizer9->Add( m_staticText28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expRiseDelay = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_expRiseDelay, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText29 = new wxStaticText( m_pwrExp, wxID_ANY, _("Rise time constant [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText29->Wrap( -1 );
	fgSizer9->Add( m_staticText29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expRiseConst = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_expRiseConst, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText30 = new wxStaticText( m_pwrExp, wxID_ANY, _("Fall delay time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	fgSizer9->Add( m_staticText30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expFallDelay = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_expFallDelay, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText31 = new wxStaticText( m_pwrExp, wxID_ANY, _("Fall time constant [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizer9->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_expFallConst = new wxTextCtrl( m_pwrExp, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_expFallConst, 0, wxALL|wxEXPAND, 5 );
	
	
	m_pwrExp->SetSizer( fgSizer9 );
	m_pwrExp->Layout();
	fgSizer9->Fit( m_pwrExp );
	m_powerNotebook->AddPage( m_pwrExp, _("Exponential"), false );
	m_pwrPwl = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrPwl->SetToolTip( _("Piece-wise linear") );
	
	wxFlexGridSizer* fgSizer15;
	fgSizer15 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer15->AddGrowableCol( 0 );
	fgSizer15->AddGrowableRow( 1 );
	fgSizer15->SetFlexibleDirection( wxBOTH );
	fgSizer15->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer10->AddGrowableCol( 1 );
	fgSizer10->SetFlexibleDirection( wxBOTH );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText34 = new wxStaticText( m_pwrPwl, wxID_ANY, _("Time [s]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText34->Wrap( -1 );
	fgSizer10->Add( m_staticText34, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pwlTime = new wxTextCtrl( m_pwrPwl, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_pwlTime->SetMinSize( wxSize( 100,-1 ) );
	
	fgSizer10->Add( m_pwlTime, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText35 = new wxStaticText( m_pwrPwl, wxID_ANY, _("Value [V/A]"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText35->Wrap( -1 );
	fgSizer10->Add( m_staticText35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pwlValue = new wxTextCtrl( m_pwrPwl, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer10->Add( m_pwlValue, 0, wxALL|wxEXPAND, 5 );
	
	
	fgSizer15->Add( fgSizer10, 1, wxEXPAND, 5 );
	
	m_pwlAddButton = new wxButton( m_pwrPwl, wxID_ANY, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_pwlAddButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_pwlValList = new wxListCtrl( m_pwrPwl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	m_pwlValList->SetMinSize( wxSize( 200,-1 ) );
	
	fgSizer15->Add( m_pwlValList, 0, wxALL|wxEXPAND, 5 );
	
	m_pwlRemoveBtn = new wxButton( m_pwrPwl, wxID_ANY, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer15->Add( m_pwlRemoveBtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_pwrPwl->SetSizer( fgSizer15 );
	m_pwrPwl->Layout();
	fgSizer15->Fit( m_pwrPwl );
	m_powerNotebook->AddPage( m_pwrPwl, _("Piece-wise Linear"), false );
	m_pwrFm = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrFm->Hide();
	
	m_powerNotebook->AddPage( m_pwrFm, _("FM"), false );
	m_pwrAm = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrAm->Hide();
	
	m_powerNotebook->AddPage( m_pwrAm, _("AM"), false );
	m_pwrTransNoise = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrTransNoise->Hide();
	
	m_powerNotebook->AddPage( m_pwrTransNoise, _("Transient noise"), false );
	m_pwrRandom = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrRandom->Hide();
	
	m_powerNotebook->AddPage( m_pwrRandom, _("Random"), false );
	m_pwrExtData = new wxPanel( m_powerNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pwrExtData->Hide();
	
	m_powerNotebook->AddPage( m_pwrExtData, _("External data"), false );
	
	sbSizer3->Add( m_powerNotebook, 0, wxEXPAND | wxALL, 5 );
	
	
	bSizer4->Add( sbSizer3, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_pwrTypeChoices[] = { _("Voltage"), _("Current") };
	int m_pwrTypeNChoices = sizeof( m_pwrTypeChoices ) / sizeof( wxString );
	m_pwrType = new wxRadioBox( m_power, wxID_ANY, _("Source type"), wxDefaultPosition, wxDefaultSize, m_pwrTypeNChoices, m_pwrTypeChoices, 1, wxRA_SPECIFY_ROWS );
	m_pwrType->SetSelection( 0 );
	bSizer4->Add( m_pwrType, 0, wxALL|wxEXPAND, 5 );
	
	
	m_power->SetSizer( bSizer4 );
	m_power->Layout();
	bSizer4->Fit( m_power );
	m_notebook->AddPage( m_power, _("Source"), false );
	
	bSizer1->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_disabled = new wxCheckBox( this, wxID_ANY, _("Disable component for simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_disabled, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	m_nodeSeqCheck = new wxCheckBox( this, wxID_ANY, _("Alternate node sequence:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_nodeSeqCheck, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_nodeSeqVal = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_nodeSeqVal->SetMinSize( wxSize( 200,-1 ) );
	
	bSizer2->Add( m_nodeSeqVal, 1, wxALL, 5 );
	
	
	bSizer1->Add( bSizer2, 0, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizer1->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SPICE_MODEL_BASE::onInitDlg ) );
	m_semiSelectLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onSemiSelectLib ), NULL, this );
	m_icSelectLib->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onSelectIcLib ), NULL, this );
	m_pwlAddButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onPwlAdd ), NULL, this );
	m_pwlRemoveBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onPwlRemove ), NULL, this );
}

DIALOG_SPICE_MODEL_BASE::~DIALOG_SPICE_MODEL_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SPICE_MODEL_BASE::onInitDlg ) );
	m_semiSelectLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onSemiSelectLib ), NULL, this );
	m_icSelectLib->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onSelectIcLib ), NULL, this );
	m_pwlAddButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onPwlAdd ), NULL, this );
	m_pwlRemoveBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SPICE_MODEL_BASE::onPwlRemove ), NULL, this );
	
}
