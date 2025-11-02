///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/html_window.h"

#include "panel_electrical_spacing_iec60664_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ELECTRICAL_SPACING_IEC60664_BASE::PANEL_ELECTRICAL_SPACING_IEC60664_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerWindow;
	bSizerWindow = new wxBoxSizer( wxVERTICAL );

	m_scrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_stTitle = new wxStaticText( m_scrolledWindow, wxID_ANY, _("Insulation for equipment within low-voltage supply systems"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTitle->Wrap( -1 );
	m_stTitle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerMain->Add( m_stTitle, 0, wxALIGN_CENTER|wxALL, 5 );

	wxStaticBoxSizer* sbSizerTop;
	sbSizerTop = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Determine the transient impulse voltage to withstand") ), wxHORIZONTAL );

	wxBoxSizer* bSizerTopLeft;
	bSizerTopLeft = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer111;
	fgSizer111 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer111->SetFlexibleDirection( wxBOTH );
	fgSizer111->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText5211 = new wxStaticText( sbSizerTop->GetStaticBox(), wxID_ANY, _("Rated Voltage (RMS or DC):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5211->Wrap( -1 );
	m_staticText5211->SetToolTip( _("Voltage of the mains supply") );

	fgSizer111->Add( m_staticText5211, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ratedVoltage = new wxTextCtrl( sbSizerTop->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer111->Add( m_ratedVoltage, 0, wxALL|wxEXPAND, 5 );

	m_staticText52112 = new wxStaticText( sbSizerTop->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52112->Wrap( -1 );
	fgSizer111->Add( m_staticText52112, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText52111 = new wxStaticText( sbSizerTop->GetStaticBox(), wxID_ANY, _("Overvoltage category:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52111->Wrap( -1 );
	m_staticText52111->SetToolTip( _("OVC I: Equipment with no direct connection to mains supply\n\nOVC II: Energy-consuming equipment to be supplied from the fixed installation. (eg: appliances, portable tools, household loads). OVCIII applies if there are reliability and availability requirements\n\nOVC III :  Equipment in fixed installations with reliability and availability requirements. (eg: electrical switches, equipment for industrial use)\n\nOVC IV: Equipment at the origin of the installation (eg: electricity meters, primary overcurrent protection devices)") );

	fgSizer111->Add( m_staticText52111, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_OVCchoiceChoices[] = { _("OVC I"), _("OVC II"), _("OVC III"), _("OVC IV") };
	int m_OVCchoiceNChoices = sizeof( m_OVCchoiceChoices ) / sizeof( wxString );
	m_OVCchoice = new wxChoice( sbSizerTop->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_OVCchoiceNChoices, m_OVCchoiceChoices, 0 );
	m_OVCchoice->SetSelection( 0 );
	fgSizer111->Add( m_OVCchoice, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerTopLeft->Add( fgSizer111, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerTopLeft->Add( 0, 0, 1, wxEXPAND, 5 );


	sbSizerTop->Add( bSizerTopLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerTopRight;
	bSizerTopRight = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerTopRight;
	fgSizerTopRight = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizerTopRight->SetFlexibleDirection( wxBOTH );
	fgSizerTopRight->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText111111 = new wxStaticText( sbSizerTop->GetStaticBox(), wxID_ANY, _("Impulse voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111111->Wrap( -1 );
	m_staticText111111->SetToolTip( _("Given the rated voltage and the overvoltage category, a device should withstand this value without a breakdown of insulation. This impulse voltage is a standard 1.2/50µs wave") );

	fgSizerTopRight->Add( m_staticText111111, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_impulseVotlage1TxtCtrl = new wxTextCtrl( sbSizerTop->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_impulseVotlage1TxtCtrl->Enable( false );
	m_impulseVotlage1TxtCtrl->SetMinSize( wxSize( 100,-1 ) );

	fgSizerTopRight->Add( m_impulseVotlage1TxtCtrl, 0, wxALL, 5 );

	static_textkV = new wxStaticText( sbSizerTop->GetStaticBox(), wxID_ANY, _("kV"), wxDefaultPosition, wxDefaultSize, 0 );
	static_textkV->Wrap( -1 );
	fgSizerTopRight->Add( static_textkV, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizerTopRight->Add( fgSizerTopRight, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	sbSizerTop->Add( bSizerTopRight, 1, wxEXPAND, 5 );


	bSizerMain->Add( sbSizerTop, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerMiddle;
	sbSizerMiddle = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Compute the clearance and creepage distances") ), wxHORIZONTAL );

	wxBoxSizer* bSizerMiddleLeft;
	bSizerMiddleLeft = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText11 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("RMS Voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_staticText11->SetToolTip( _("Expected RMS voltage.") );

	fgSizer11->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_RMSVoltage = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_RMSVoltage->SetMinSize( wxSize( 100,-1 ) );

	fgSizer11->Add( m_RMSVoltage, 0, wxALL|wxEXPAND, 5 );

	m_staticText11212 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11212->Wrap( -1 );
	fgSizer11->Add( m_staticText11212, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText114 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Transient overvoltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText114->Wrap( -1 );
	m_staticText114->SetToolTip( _("Transient overvoltages due to:\n\n- Atmospheric disturbances transmitted by the mains supply (eg: a lightning strike)\n- Switching loads in the main supplys\n- External circuits\n- Internal generation\n\nEvents that last for a few milliseconds or less.") );

	fgSizer11->Add( m_staticText114, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_transientOvervoltage = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_transientOvervoltage->SetMinSize( wxSize( 100,-1 ) );

	fgSizer11->Add( m_transientOvervoltage, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText1121 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("kV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1121->Wrap( -1 );
	fgSizer11->Add( m_staticText1121, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText113 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Recurring peak voltage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText113->Wrap( -1 );
	m_staticText113->SetToolTip( _("- Steady-state voltage value\n- Temporary overvoltage\n- Recurring peak voltage\n\nEvents of relatively long duration.") );

	fgSizer11->Add( m_staticText113, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_peakVoltage = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_peakVoltage->SetMinSize( wxSize( 100,-1 ) );

	fgSizer11->Add( m_peakVoltage, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText11211 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("kV"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11211->Wrap( -1 );
	fgSizer11->Add( m_staticText11211, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText112 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Type of insulation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText112->Wrap( -1 );
	m_staticText112->SetToolTip( _("Functional: insulation is necessary only for the functioning of the equipment\n\nBasic: Insulation of hazardous-live parts.\n\nReinforced: Single insulation that provides a degree of protection equivalent to a double insulation. ( which is two separate basic insulations, in case one of them fails  ).") );

	fgSizer11->Add( m_staticText112, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_insulationTypeChoices[] = { _("Functional"), _("Basic"), _("Reinforced") };
	int m_insulationTypeNChoices = sizeof( m_insulationTypeChoices ) / sizeof( wxString );
	m_insulationType = new wxChoice( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_insulationTypeNChoices, m_insulationTypeChoices, 0 );
	m_insulationType->SetSelection( 0 );
	fgSizer11->Add( m_insulationType, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText52 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Pollution Degree:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText52->Wrap( -1 );
	m_staticText52->SetToolTip( _("PD1: No pollution or only dry, non-conductive pollution occurs\n\nPD2: Only non-conductive pollution occurs . Condensation may occur.\n\nPD3: Conductive pollution occurs, or non-conductive pollution occurs which becomes conductive due to expected condensation.\n\nPD4: Continuous conductivity occurs due to conductive dust, rain, ...") );

	fgSizer11->Add( m_staticText52, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_pollutionDegreeChoices[] = { _("PD1"), _("PD2"), _("PD3"), _("PD4") };
	int m_pollutionDegreeNChoices = sizeof( m_pollutionDegreeChoices ) / sizeof( wxString );
	m_pollutionDegree = new wxChoice( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pollutionDegreeNChoices, m_pollutionDegreeChoices, 0 );
	m_pollutionDegree->SetSelection( 1 );
	fgSizer11->Add( m_pollutionDegree, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );

	m_materialGroupTxt = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Material group:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_materialGroupTxt->Wrap( -1 );
	m_materialGroupTxt->SetToolTip( _("Materials with a high comparative tracking index (CTI) are better at providing isolation.\n\nMaterial group I: 600 <= CTI\nMaterial group II: 400 <= CTI < 600\nMaterial group IIIa: 175 <= CTI < 400\nMaterial group IIIb: 100 <= CTI < 175") );

	fgSizer11->Add( m_materialGroupTxt, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_materialGroupChoices[] = { _("I"), _("II"), _("IIIa"), _("IIIb") };
	int m_materialGroupNChoices = sizeof( m_materialGroupChoices ) / sizeof( wxString );
	m_materialGroup = new wxChoice( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_materialGroupNChoices, m_materialGroupChoices, 0 );
	m_materialGroup->SetSelection( 0 );
	fgSizer11->Add( m_materialGroup, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText1112 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("PCB material:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1112->Wrap( -1 );
	m_staticText1112->SetToolTip( _("Printed wiring material can benefit of a creepage distance reduction for RMS voltages lower than 1000V") );

	fgSizer11->Add( m_staticText1112, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_pcbMaterial = new wxCheckBox( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_pcbMaterial, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText1112121 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Max altitude:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1112121->Wrap( -1 );
	m_staticText1112121->SetToolTip( _("Coating and potting allows for clearance and creepage distances reduction. Not supported by the calculator.\n\nA coating that could easily delaminate in the lifespan of the product (such as a soldermask) should not be considered for a reduction.") );

	fgSizer11->Add( m_staticText1112121, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_altitude = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_altitude->SetMinSize( wxSize( 100,-1 ) );

	fgSizer11->Add( m_altitude, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_staticText11121211 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("m"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11121211->Wrap( -1 );
	fgSizer11->Add( m_staticText11121211, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bSizerMiddleLeft->Add( fgSizer11, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	sbSizerMiddle->Add( bSizerMiddleLeft, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerMiddleRight;
	bSizerMiddleRight = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMiddleRightInfo;
	bSizerMiddleRightInfo = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer6->AddGrowableCol( 1 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText11111 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11111->Wrap( -1 );
	fgSizer6->Add( m_staticText11111, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_clearance = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_clearance->Enable( false );
	m_clearance->SetMinSize( wxSize( 100,-1 ) );

	fgSizer6->Add( m_clearance, 0, wxALL, 5 );

	m_staticText71111 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText71111->Wrap( -1 );
	fgSizer6->Add( m_staticText71111, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticText1111 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Creepage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1111->Wrap( -1 );
	fgSizer6->Add( m_staticText1111, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_creepage = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_creepage->Enable( false );
	m_creepage->SetMinSize( wxSize( 100,-1 ) );

	fgSizer6->Add( m_creepage, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticText7111 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7111->Wrap( -1 );
	fgSizer6->Add( m_staticText7111, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticText111 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("Min groove width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	m_staticText111->SetToolTip( _("A groove which width is smaller has no effect on the path considered for creepage") );

	fgSizer6->Add( m_staticText111, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_minGrooveWidth = new wxTextCtrl( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_minGrooveWidth->Enable( false );
	m_minGrooveWidth->SetMinSize( wxSize( 100,-1 ) );

	fgSizer6->Add( m_minGrooveWidth, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_staticText711 = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText711->Wrap( -1 );
	fgSizer6->Add( m_staticText711, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );


	bSizerMiddleRightInfo->Add( fgSizer6, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );


	bSizerMiddleRight->Add( bSizerMiddleRightInfo, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerBitmapHelp;
	bSizerBitmapHelp = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerBitmap;
	bSizerBitmap = new wxBoxSizer( wxHORIZONTAL );

	m_creepageclearanceBitmap = new wxStaticBitmap( sbSizerMiddle->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBitmap->Add( m_creepageclearanceBitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_stBitmapLegend = new wxStaticText( sbSizerMiddle->GetStaticBox(), wxID_ANY, _("solid: clearance\ndashed: creepage"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stBitmapLegend->Wrap( -1 );
	bSizerBitmap->Add( m_stBitmapLegend, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );


	bSizerBitmapHelp->Add( bSizerBitmap, 1, wxEXPAND, 5 );


	bSizerMiddleRight->Add( bSizerBitmapHelp, 0, wxEXPAND, 5 );


	sbSizerMiddle->Add( bSizerMiddleRight, 1, wxEXPAND, 5 );


	bSizerMain->Add( sbSizerMiddle, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbSizerBottom;
	sbSizerBottom = new wxStaticBoxSizer( new wxStaticBox( m_scrolledWindow, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelHelp = new HTML_WINDOW( sbSizerBottom->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbSizerBottom->Add( m_panelHelp, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( sbSizerBottom, 1, wxEXPAND|wxBOTTOM, 5 );


	m_scrolledWindow->SetSizer( bSizerMain );
	m_scrolledWindow->Layout();
	bSizerMain->Fit( m_scrolledWindow );
	bSizerWindow->Add( m_scrolledWindow, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizerWindow );
	this->Layout();
	bSizerWindow->Fit( this );

	// Connect Events
	m_ratedVoltage->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateTransientImpulse ), NULL, this );
	m_OVCchoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateTransientImpulse ), NULL, this );
	m_RMSVoltage->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_transientOvervoltage->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_peakVoltage->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_insulationType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_pollutionDegree->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_materialGroup->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_pcbMaterial->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_altitude->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
}

PANEL_ELECTRICAL_SPACING_IEC60664_BASE::~PANEL_ELECTRICAL_SPACING_IEC60664_BASE()
{
	// Disconnect Events
	m_ratedVoltage->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateTransientImpulse ), NULL, this );
	m_OVCchoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateTransientImpulse ), NULL, this );
	m_RMSVoltage->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_transientOvervoltage->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_peakVoltage->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_insulationType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_pollutionDegree->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_materialGroup->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_pcbMaterial->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );
	m_altitude->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_ELECTRICAL_SPACING_IEC60664_BASE::UpdateClearanceCreepage ), NULL, this );

}
