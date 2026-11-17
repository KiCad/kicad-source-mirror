///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sim_command_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SIM_COMMAND_BASE::DIALOG_SIM_COMMAND_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_commandTypeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_commandTypeLabel = new wxStaticText( this, wxID_ANY, _("Analysis type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_commandTypeLabel->Wrap( -1 );
	m_commandTypeSizer->Add( m_commandTypeLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_commandTypeChoices[] = { _("AC"), _("DC"), _("OP"), _("TRAN"), _("FFT"), _("NOISE"), _("SP"), _("Custom") };
	int m_commandTypeNChoices = sizeof( m_commandTypeChoices ) / sizeof( wxString );
	m_commandType = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_commandTypeNChoices, m_commandTypeChoices, 0 );
	m_commandType->SetSelection( 6 );
	m_commandTypeSizer->Add( m_commandType, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer1->Add( m_commandTypeSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizer1->Add( 0, 5, 0, wxEXPAND, 5 );

	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelCommand = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bCommandSizer;
	bCommandSizer = new wxBoxSizer( wxVERTICAL );

	m_analysisParamsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelCommand, wxID_ANY, _("Analysis Parameters") ), wxVERTICAL );

	m_simPages = new wxSimplebook( m_analysisParamsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_pgAC = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	wxString m_acScaleChoices[] = { _("Decade"), _("Octave"), _("Linear") };
	int m_acScaleNChoices = sizeof( m_acScaleChoices ) / sizeof( wxString );
	m_acScale = new wxRadioBox( m_pgAC, wxID_ANY, _("Frequency scale"), wxDefaultPosition, wxDefaultSize, m_acScaleNChoices, m_acScaleChoices, 1, wxRA_SPECIFY_COLS );
	m_acScale->SetSelection( 0 );
	m_acScale->Hide();

	bSizer3->Add( m_acScale, 0, wxEXPAND|wxALL, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText1 = new wxStaticText( m_pgAC, wxID_ANY, _("Number of points per decade:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_acPointsNumber = new wxTextCtrl( m_pgAC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_acPointsNumber, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText2 = new wxStaticText( m_pgAC, wxID_ANY, _("Start frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_acFreqStart = new wxTextCtrl( m_pgAC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_acFreqStart->SetMinSize( wxSize( 100,-1 ) );

	fgSizer1->Add( m_acFreqStart, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText19 = new wxStaticText( m_pgAC, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	fgSizer1->Add( m_staticText19, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_staticText3 = new wxStaticText( m_pgAC, wxID_ANY, _("Stop frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_acFreqStop = new wxTextCtrl( m_pgAC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_acFreqStop, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText110 = new wxStaticText( m_pgAC, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText110->Wrap( -1 );
	fgSizer1->Add( m_staticText110, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );


	bSizer3->Add( fgSizer1, 0, wxEXPAND|wxALL, 5 );


	m_pgAC->SetSizer( bSizer3 );
	m_pgAC->Layout();
	bSizer3->Fit( m_pgAC );
	m_simPages->AddPage( m_pgAC, _("a page"), false );
	m_pgDC = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer82;
	bSizer82 = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_dcEnable2 = new wxCheckBox( m_pgDC, wxID_ANY, _("Source 2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dcEnable2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	gbSizer1->Add( m_dcEnable2, wxGBPosition( 0, 3 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_dcSourceType1Choices[] = { _("V"), _("I"), _("R"), _("TEMP") };
	int m_dcSourceType1NChoices = sizeof( m_dcSourceType1Choices ) / sizeof( wxString );
	m_dcSourceType1 = new wxChoice( m_pgDC, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dcSourceType1NChoices, m_dcSourceType1Choices, 0 );
	m_dcSourceType1->SetSelection( 0 );
	gbSizer1->Add( m_dcSourceType1, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxString m_dcSourceType2Choices[] = { _("V"), _("I"), _("R"), _("TEMP") };
	int m_dcSourceType2NChoices = sizeof( m_dcSourceType2Choices ) / sizeof( wxString );
	m_dcSourceType2 = new wxChoice( m_pgDC, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dcSourceType2NChoices, m_dcSourceType2Choices, 0 );
	m_dcSourceType2->SetSelection( 0 );
	gbSizer1->Add( m_dcSourceType2, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText4 = new wxStaticText( m_pgDC, wxID_ANY, _("Source:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	gbSizer1->Add( m_staticText4, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText41 = new wxStaticText( m_pgDC, wxID_ANY, _("Sweep type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText41->Wrap( -1 );
	gbSizer1->Add( m_staticText41, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT, 5 );

	m_staticText411 = new wxStaticText( m_pgDC, wxID_ANY, _("Source 1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText411->Wrap( -1 );
	m_staticText411->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	gbSizer1->Add( m_staticText411, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_dcSource1Choices;
	m_dcSource1 = new wxChoice( m_pgDC, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dcSource1Choices, 0 );
	m_dcSource1->SetSelection( 0 );
	gbSizer1->Add( m_dcSource1, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_dcSource2Choices;
	m_dcSource2 = new wxChoice( m_pgDC, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dcSource2Choices, 0 );
	m_dcSource2->SetSelection( 0 );
	gbSizer1->Add( m_dcSource2, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticText5 = new wxStaticText( m_pgDC, wxID_ANY, _("Starting value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	gbSizer1->Add( m_staticText5, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_dcStart1 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_dcStart1->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_dcStart1, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src1DCStartValUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src1DCStartValUnit->Wrap( -1 );
	m_src1DCStartValUnit->SetMinSize( wxSize( 80,-1 ) );

	gbSizer1->Add( m_src1DCStartValUnit, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_dcStart2 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_dcStart2->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_dcStart2, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src2DCStartValUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src2DCStartValUnit->Wrap( -1 );
	gbSizer1->Add( m_src2DCStartValUnit, wxGBPosition( 3, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText6 = new wxStaticText( m_pgDC, wxID_ANY, _("Final value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	gbSizer1->Add( m_staticText6, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_dcStop1 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dcStop1, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src1DCEndValUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src1DCEndValUnit->Wrap( -1 );
	m_src1DCEndValUnit->SetMinSize( wxSize( 80,-1 ) );

	gbSizer1->Add( m_src1DCEndValUnit, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_dcStop2 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dcStop2, wxGBPosition( 4, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src2DCEndValUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src2DCEndValUnit->Wrap( -1 );
	gbSizer1->Add( m_src2DCEndValUnit, wxGBPosition( 4, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_staticText7 = new wxStaticText( m_pgDC, wxID_ANY, _("Increment step:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbSizer1->Add( m_staticText7, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_dcIncr1 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dcIncr1, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src1DCStepUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src1DCStepUnit->Wrap( -1 );
	m_src1DCStepUnit->SetMinSize( wxSize( 80,-1 ) );

	gbSizer1->Add( m_src1DCStepUnit, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_dcIncr2 = new wxTextCtrl( m_pgDC, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_dcIncr2, wxGBPosition( 5, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_src2DCStepUnit = new wxStaticText( m_pgDC, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_src2DCStepUnit->Wrap( -1 );
	gbSizer1->Add( m_src2DCStepUnit, wxGBPosition( 5, 4 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer82->Add( gbSizer1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer82->Add( 0, 10, 0, wxEXPAND, 5 );

	m_swapDCSources = new wxButton( m_pgDC, wxID_ANY, _("Swap sources"), wxDefaultPosition, wxDefaultSize, 0 );
	m_swapDCSources->SetMinSize( wxSize( 132,-1 ) );

	bSizer82->Add( m_swapDCSources, 0, wxALL, 10 );


	m_pgDC->SetSizer( bSizer82 );
	m_pgDC->Layout();
	bSizer82->Fit( m_pgDC );
	m_simPages->AddPage( m_pgDC, _("a page"), false );
	m_pgOP = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );


	m_pgOP->SetSizer( bSizer8 );
	m_pgOP->Layout();
	bSizer8->Fit( m_pgOP );
	m_simPages->AddPage( m_pgOP, _("a page"), false );
	m_pgTRAN = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 4, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,8 ) );

	m_timeLabel = new wxStaticText( m_pgTRAN, wxID_ANY, _("Output time step:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeLabel->Wrap( -1 );
	m_timeLabel->SetToolTip( _("How often results are recorded and plotted.  It does not limit the step size the solver takes internally.") );

	gbSizer2->Add( m_timeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transStep = new wxTextCtrl( m_pgTRAN, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_transStep->SetToolTip( _("How often results are recorded and plotted.  It does not limit the step size the solver takes internally.") );
	m_transStep->SetMinSize( wxSize( 100,-1 ) );

	gbSizer2->Add( m_transStep, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_timeUnits = new wxStaticText( m_pgTRAN, wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeUnits->Wrap( -1 );
	gbSizer2->Add( m_timeUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transFinalLabel = new wxStaticText( m_pgTRAN, wxID_ANY, _("Stop time:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transFinalLabel->Wrap( -1 );
	gbSizer2->Add( m_transFinalLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transFinal = new wxTextCtrl( m_pgTRAN, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_transFinal, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_transFinalUnits = new wxStaticText( m_pgTRAN, wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transFinalUnits->Wrap( -1 );
	gbSizer2->Add( m_transFinalUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transInitialLabel = new wxStaticText( m_pgTRAN, wxID_ANY, _("Start time:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transInitialLabel->Wrap( -1 );
	gbSizer2->Add( m_transInitialLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transInitial = new wxTextCtrl( m_pgTRAN, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_transInitial, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_transInitialUnits = new wxStaticText( m_pgTRAN, wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transInitialUnits->Wrap( -1 );
	gbSizer2->Add( m_transInitialUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transInitialHelp = new wxStaticText( m_pgTRAN, wxID_ANY, _("(optional; default 0)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transInitialHelp->Wrap( -1 );
	gbSizer2->Add( m_transInitialHelp, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_maxStepLabel = new wxStaticText( m_pgTRAN, wxID_ANY, _("Max solver time step:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxStepLabel->Wrap( -1 );
	m_maxStepLabel->SetToolTip( _("Upper bound on the solver's own internal time step.  Lower it when a fast transient is being stepped over.  Defaults to whichever is smaller: the output time step, or 1/50 of the simulated time span.") );

	gbSizer2->Add( m_maxStepLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transMaxStep = new wxTextCtrl( m_pgTRAN, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_transMaxStep->SetToolTip( _("Upper bound on the solver's own internal time step.  Lower it when a fast transient is being stepped over.  Defaults to whichever is smaller: the output time step, or 1/50 of the simulated time span.") );

	gbSizer2->Add( m_transMaxStep, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_transMaxStepUnit = new wxStaticText( m_pgTRAN, wxID_ANY, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transMaxStepUnit->Wrap( -1 );
	gbSizer2->Add( m_transMaxStepUnit, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_transMaxHelp = new wxStaticText( m_pgTRAN, wxID_ANY, _("(optional; default: smaller of output step and 1/50 of time span)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_transMaxHelp->Wrap( -1 );
	gbSizer2->Add( m_transMaxHelp, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_useInitialConditions = new wxCheckBox( m_pgTRAN, wxID_ANY, _("Use initial conditions"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_useInitialConditions, wxGBPosition( 5, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer81->Add( gbSizer2, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_pgTRAN->SetSizer( bSizer81 );
	m_pgTRAN->Layout();
	bSizer81->Fit( m_pgTRAN );
	m_simPages->AddPage( m_pgTRAN, _("a page"), false );
	m_pgFFT = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer151;
	bSizer151 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_signalsLabel = new wxStaticText( m_pgFFT, wxID_ANY, _("Input signals:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_signalsLabel->Wrap( -1 );
	bSizer14->Add( m_signalsLabel, 0, wxBOTTOM|wxLEFT, 5 );

	m_inputSignalsFilter = new wxSearchCtrl( m_pgFFT, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifndef __WXMAC__
	m_inputSignalsFilter->ShowSearchButton( true );
	#endif
	m_inputSignalsFilter->ShowCancelButton( true );
	bSizer14->Add( m_inputSignalsFilter, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_inputSignalsListChoices;
	m_inputSignalsList = new wxCheckListBox( m_pgFFT, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_inputSignalsListChoices, 0 );
	bSizer14->Add( m_inputSignalsList, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_linearize = new wxCheckBox( m_pgFFT, wxID_ANY, _("Linearize inputs before performing FFT"), wxDefaultPosition, wxDefaultSize, 0 );
	m_linearize->SetValue(true);
	bSizer14->Add( m_linearize, 0, wxALL, 5 );


	bSizer151->Add( bSizer14, 1, wxRIGHT|wxLEFT, 5 );


	m_pgFFT->SetSizer( bSizer151 );
	m_pgFFT->Layout();
	bSizer151->Fit( m_pgFFT );
	m_simPages->AddPage( m_pgFFT, _("a page"), false );
	m_pgNOISE = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText14 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Measured node:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	fgSizer7->Add( m_staticText14, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_noiseMeasChoices;
	m_noiseMeas = new wxChoice( m_pgNOISE, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_noiseMeasChoices, 0 );
	m_noiseMeas->SetSelection( 0 );
	fgSizer7->Add( m_noiseMeas, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizer7->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText15 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Reference node:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer7->Add( m_staticText15, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_noiseRefChoices;
	m_noiseRef = new wxChoice( m_pgNOISE, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_noiseRefChoices, 0 );
	m_noiseRef->SetSelection( 0 );
	fgSizer7->Add( m_noiseRef, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_staticText23 = new wxStaticText( m_pgNOISE, wxID_ANY, _("(optional; default GND)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgSizer7->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText16 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Noise source:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	fgSizer7->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_noiseSrcChoices;
	m_noiseSrc = new wxChoice( m_pgNOISE, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_noiseSrcChoices, 0 );
	m_noiseSrc->SetSelection( 0 );
	fgSizer7->Add( m_noiseSrc, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	fgSizer7->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer15->Add( fgSizer7, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_noiseScaleChoices[] = { _("Decade"), _("Octave"), _("Linear") };
	int m_noiseScaleNChoices = sizeof( m_noiseScaleChoices ) / sizeof( wxString );
	m_noiseScale = new wxRadioBox( m_pgNOISE, wxID_ANY, _("Frequency scale"), wxDefaultPosition, wxDefaultSize, m_noiseScaleNChoices, m_noiseScaleChoices, 1, wxRA_SPECIFY_COLS );
	m_noiseScale->SetSelection( 0 );
	m_noiseScale->Hide();

	bSizer10->Add( m_noiseScale, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText11 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Number of points per decade:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	fgSizer11->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_noisePointsNumber = new wxTextCtrl( m_pgNOISE, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_noisePointsNumber->SetMinSize( wxSize( 80,-1 ) );

	fgSizer11->Add( m_noisePointsNumber, 1, wxALIGN_CENTER_VERTICAL|wxTOP, 5 );


	fgSizer11->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText21 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Start frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer11->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_noiseFreqStart = new wxTextCtrl( m_pgNOISE, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_noiseFreqStart->SetMinSize( wxSize( 80,-1 ) );

	fgSizer11->Add( m_noiseFreqStart, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_noiseFreqStartUnits = new wxStaticText( m_pgNOISE, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_noiseFreqStartUnits->Wrap( -1 );
	fgSizer11->Add( m_noiseFreqStartUnits, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText31 = new wxStaticText( m_pgNOISE, wxID_ANY, _("Stop frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizer11->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_noiseFreqStop = new wxTextCtrl( m_pgNOISE, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_noiseFreqStop->SetMinSize( wxSize( 80,-1 ) );

	fgSizer11->Add( m_noiseFreqStop, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

	m_noiseFreqStopUnits = new wxStaticText( m_pgNOISE, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_noiseFreqStopUnits->Wrap( -1 );
	fgSizer11->Add( m_noiseFreqStopUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer10->Add( fgSizer11, 0, wxALIGN_BOTTOM|wxTOP|wxLEFT, 5 );


	bSizer15->Add( bSizer10, 0, wxEXPAND|wxTOP, 5 );

	m_saveAllNoise = new wxCheckBox( m_pgNOISE, wxID_ANY, _("Save contributions from all noise generators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_saveAllNoise->SetValue(true);
	bSizer15->Add( m_saveAllNoise, 0, wxALL, 10 );


	m_pgNOISE->SetSizer( bSizer15 );
	m_pgNOISE->Layout();
	bSizer15->Fit( m_pgNOISE );
	m_simPages->AddPage( m_pgNOISE, _("a page"), false );
	m_pgSP = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );

	wxString m_spScaleChoices[] = { _("Decade"), _("Octave"), _("Linear") };
	int m_spScaleNChoices = sizeof( m_spScaleChoices ) / sizeof( wxString );
	m_spScale = new wxRadioBox( m_pgSP, wxID_ANY, _("Frequency scale"), wxDefaultPosition, wxDefaultSize, m_spScaleNChoices, m_spScaleChoices, 1, wxRA_SPECIFY_COLS );
	m_spScale->SetSelection( 0 );
	m_spScale->Hide();

	bSizer31->Add( m_spScale, 0, wxEXPAND|wxALL, 5 );

	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 3, 4, 5 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText12 = new wxStaticText( m_pgSP, wxID_ANY, _("Number of points per decade:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	fgSizer12->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spPointsNumber = new wxTextCtrl( m_pgSP, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spPointsNumber->SetMinSize( wxSize( 100,-1 ) );

	fgSizer12->Add( m_spPointsNumber, 1, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer12->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText22 = new wxStaticText( m_pgSP, wxID_ANY, _("Start frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer12->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spFreqStart = new wxTextCtrl( m_pgSP, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spFreqStart->SetMinSize( wxSize( 100,-1 ) );

	fgSizer12->Add( m_spFreqStart, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText191 = new wxStaticText( m_pgSP, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText191->Wrap( -1 );
	fgSizer12->Add( m_staticText191, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_staticText32 = new wxStaticText( m_pgSP, wxID_ANY, _("Stop frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	fgSizer12->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spFreqStop = new wxTextCtrl( m_pgSP, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spFreqStop->SetMinSize( wxSize( 100,-1 ) );

	fgSizer12->Add( m_spFreqStop, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticText1101 = new wxStaticText( m_pgSP, wxID_ANY, _("Hz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1101->Wrap( -1 );
	fgSizer12->Add( m_staticText1101, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );


	bSizer31->Add( fgSizer12, 0, wxEXPAND|wxALL, 5 );

	m_spDoNoise = new wxCheckBox( m_pgSP, wxID_ANY, _("Compute noise current correlation matrix"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_spDoNoise, 0, wxLEFT|wxRIGHT|wxTOP, 5 );


	m_pgSP->SetSizer( bSizer31 );
	m_pgSP->Layout();
	bSizer31->Fit( m_pgSP );
	m_simPages->AddPage( m_pgSP, _("a page"), false );
	m_pgCustom = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_staticText18 = new wxStaticText( m_pgCustom, wxID_ANY, _("SPICE directives:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	bSizer2->Add( m_staticText18, 0, wxRIGHT|wxLEFT, 5 );

	m_customTxt = new wxTextCtrl( m_pgCustom, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_customTxt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_customTxt, 1, wxALL|wxEXPAND, 5 );

	m_loadDirectives = new wxButton( m_pgCustom, wxID_ANY, _("Load Directives from Schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_loadDirectives, 0, wxALL|wxEXPAND, 5 );


	m_pgCustom->SetSizer( bSizer2 );
	m_pgCustom->Layout();
	bSizer2->Fit( m_pgCustom );
	m_simPages->AddPage( m_pgCustom, _("a page"), false );
	m_pgPZ = new wxPanel( m_simPages, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer821;
	bSizer821 = new wxBoxSizer( wxVERTICAL );


	bSizer821->Add( 0, 5, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer11;
	gbSizer11 = new wxGridBagSizer( 6, 0 );
	gbSizer11->SetFlexibleDirection( wxBOTH );
	gbSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pzFunctionTypeLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Transfer function:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzFunctionTypeLabel->Wrap( -1 );
	gbSizer11->Add( m_pzFunctionTypeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_pzFunctionTypeChoices[] = { _("(output voltage) / (input voltage)"), _("(output voltage) / (input current)") };
	int m_pzFunctionTypeNChoices = sizeof( m_pzFunctionTypeChoices ) / sizeof( wxString );
	m_pzFunctionType = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzFunctionTypeNChoices, m_pzFunctionTypeChoices, 0 );
	m_pzFunctionType->SetSelection( 0 );
	gbSizer11->Add( m_pzFunctionType, wxGBPosition( 0, 1 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_pzInputLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Input:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzInputLabel->Wrap( -1 );
	gbSizer11->Add( m_pzInputLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_pzInputChoices;
	m_pzInput = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzInputChoices, 0 );
	m_pzInput->SetSelection( 0 );
	gbSizer11->Add( m_pzInput, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_pzInputRefLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzInputRefLabel->Wrap( -1 );
	gbSizer11->Add( m_pzInputRefLabel, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_pzInputRefChoices;
	m_pzInputRef = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzInputRefChoices, 0 );
	m_pzInputRef->SetSelection( 0 );
	gbSizer11->Add( m_pzInputRef, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 10 );

	m_pzOutputLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Output:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzOutputLabel->Wrap( -1 );
	gbSizer11->Add( m_pzOutputLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxArrayString m_pzOutputChoices;
	m_pzOutput = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzOutputChoices, 0 );
	m_pzOutput->SetSelection( 0 );
	gbSizer11->Add( m_pzOutput, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 10 );

	m_pzOutputRefLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Ref:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzOutputRefLabel->Wrap( -1 );
	gbSizer11->Add( m_pzOutputRefLabel, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_pzOutputRefChoices;
	m_pzOutputRef = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzOutputRefChoices, 0 );
	m_pzOutputRef->SetSelection( 0 );
	gbSizer11->Add( m_pzOutputRef, wxGBPosition( 2, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 10 );


	bSizer821->Add( gbSizer11, 0, wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );

	m_pzAnalysesLabel = new wxStaticText( m_pgPZ, wxID_ANY, _("Find:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pzAnalysesLabel->Wrap( -1 );
	bSizer17->Add( m_pzAnalysesLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_pzAnalysesChoices[] = { _("Poles and Zeros"), _("Poles"), _("Zeros"), wxEmptyString };
	int m_pzAnalysesNChoices = sizeof( m_pzAnalysesChoices ) / sizeof( wxString );
	m_pzAnalyses = new wxChoice( m_pgPZ, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pzAnalysesNChoices, m_pzAnalysesChoices, 0 );
	m_pzAnalyses->SetSelection( 0 );
	bSizer17->Add( m_pzAnalyses, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer821->Add( bSizer17, 0, wxEXPAND|wxLEFT|wxTOP, 5 );


	m_pgPZ->SetSizer( bSizer821 );
	m_pgPZ->Layout();
	bSizer821->Fit( m_pgPZ );
	m_simPages->AddPage( m_pgPZ, _("a page"), false );

	m_analysisParamsSizer->Add( m_simPages, 1, wxEXPAND | wxALL, 5 );


	bCommandSizer->Add( m_analysisParamsSizer, 1, wxEXPAND|wxALL, 5 );

	m_netlistOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelCommand, wxID_ANY, _("Netlist Options") ), wxVERTICAL );

	wxBoxSizer* bSizer88;
	bSizer88 = new wxBoxSizer( wxVERTICAL );

	m_fixIncludePaths = new wxCheckBox( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Add full path for .include library directives"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fixIncludePaths->SetValue(true);
	bSizer88->Add( m_fixIncludePaths, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_saveAllVoltages = new wxCheckBox( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Save all voltages"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_saveAllVoltages, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_saveAllCurrents = new wxCheckBox( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Save all currents"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_saveAllCurrents, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_saveAllDissipations = new wxCheckBox( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Save all power dissipations"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_saveAllDissipations, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_saveAllEvents = new wxCheckBox( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Save all digital event data"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer88->Add( m_saveAllEvents, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_compatibilityModeSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* compatibilityLabel;
	compatibilityLabel = new wxStaticText( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, _("Compatibility mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	compatibilityLabel->Wrap( -1 );
	m_compatibilityModeSizer->Add( compatibilityLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 8 );

	wxString m_compatibilityModeChoices[] = { _("User configuration"), _("Spice"), _("PSpice"), _("LTSpice"), _("PSpice and LTSpice"), _("HSpice") };
	int m_compatibilityModeNChoices = sizeof( m_compatibilityModeChoices ) / sizeof( wxString );
	m_compatibilityMode = new wxChoice( m_netlistOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_compatibilityModeNChoices, m_compatibilityModeChoices, 0 );
	m_compatibilityMode->SetSelection( 0 );
	m_compatibilityModeSizer->Add( m_compatibilityMode, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer88->Add( m_compatibilityModeSizer, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_netlistOptionsSizer->Add( bSizer88, 0, wxEXPAND|wxTOP|wxLEFT, 10 );


	bCommandSizer->Add( m_netlistOptionsSizer, 0, wxEXPAND|wxALL, 5 );


	m_panelCommand->SetSizer( bCommandSizer );
	m_panelCommand->Layout();
	bCommandSizer->Fit( m_panelCommand );
	m_notebook1->AddPage( m_panelCommand, _("SPICE Command"), true );
	m_panelPlotSetup = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bPlotSetupSizer;
	bPlotSetupSizer = new wxBoxSizer( wxVERTICAL );

	m_axisScalesSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelPlotSetup, wxID_ANY, _("Axis Scales") ), wxVERTICAL );

	m_bSizerY1 = new wxBoxSizer( wxVERTICAL );

	m_lockY1 = new wxCheckBox( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Fixed %s scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bSizerY1->Add( m_lockY1, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	m_bSizerY1->Add( 0, 2, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerY1;
	fgSizerY1 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerY1->SetFlexibleDirection( wxBOTH );
	fgSizerY1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_y1MinLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Min:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y1MinLabel->Wrap( -1 );
	fgSizerY1->Add( m_y1MinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_y1Min = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY1->Add( m_y1Min, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y1MaxLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Max:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y1MaxLabel->Wrap( -1 );
	fgSizerY1->Add( m_y1MaxLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 18 );

	m_y1Max = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY1->Add( m_y1Max, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y1Units = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y1Units->Wrap( -1 );
	m_y1Units->SetMinSize( wxSize( 40,-1 ) );

	fgSizerY1->Add( m_y1Units, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	m_bSizerY1->Add( fgSizerY1, 0, wxBOTTOM, 8 );


	m_axisScalesSizer->Add( m_bSizerY1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bSizerY2 = new wxBoxSizer( wxVERTICAL );

	m_lockY2 = new wxCheckBox( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Fixed %s scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bSizerY2->Add( m_lockY2, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	m_bSizerY2->Add( 0, 2, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerY2;
	fgSizerY2 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerY2->SetFlexibleDirection( wxBOTH );
	fgSizerY2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_y2MinLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Min:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y2MinLabel->Wrap( -1 );
	fgSizerY2->Add( m_y2MinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_y2Min = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY2->Add( m_y2Min, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y2MaxLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Max:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y2MaxLabel->Wrap( -1 );
	fgSizerY2->Add( m_y2MaxLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 18 );

	m_y2Max = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY2->Add( m_y2Max, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y2Units = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y2Units->Wrap( -1 );
	fgSizerY2->Add( m_y2Units, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	m_bSizerY2->Add( fgSizerY2, 0, wxBOTTOM, 8 );


	m_axisScalesSizer->Add( m_bSizerY2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_bSizerY3 = new wxBoxSizer( wxVERTICAL );

	m_lockY3 = new wxCheckBox( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Fixed %s scale"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bSizerY3->Add( m_lockY3, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	m_bSizerY3->Add( 0, 2, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerY3;
	fgSizerY3 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizerY3->SetFlexibleDirection( wxBOTH );
	fgSizerY3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_y3MinLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Min:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y3MinLabel->Wrap( -1 );
	fgSizerY3->Add( m_y3MinLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_y3Min = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY3->Add( m_y3Min, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y3MaxLabel = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("Max:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y3MaxLabel->Wrap( -1 );
	fgSizerY3->Add( m_y3MaxLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 18 );

	m_y3Max = new wxTextCtrl( m_axisScalesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerY3->Add( m_y3Max, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_y3Units = new wxStaticText( m_axisScalesSizer->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_y3Units->Wrap( -1 );
	m_y3Units->SetMinSize( wxSize( 40,-1 ) );

	fgSizerY3->Add( m_y3Units, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


	m_bSizerY3->Add( fgSizerY3, 0, wxBOTTOM, 8 );


	m_axisScalesSizer->Add( m_bSizerY3, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bPlotSetupSizer->Add( m_axisScalesSizer, 0, wxEXPAND|wxALL, 5 );

	m_appearanceSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelPlotSetup, wxID_ANY, _("Appearance") ), wxVERTICAL );

	wxBoxSizer* bSizerCheckboxes;
	bSizerCheckboxes = new wxBoxSizer( wxVERTICAL );

	m_grid = new wxCheckBox( m_appearanceSizer->GetStaticBox(), wxID_ANY, _("Show grid"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid->SetValue(true);
	bSizerCheckboxes->Add( m_grid, 0, wxALL|wxEXPAND, 5 );

	m_legend = new wxCheckBox( m_appearanceSizer->GetStaticBox(), wxID_ANY, _("Show legend"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerCheckboxes->Add( m_legend, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_dottedSecondary = new wxCheckBox( m_appearanceSizer->GetStaticBox(), wxID_ANY, _("Dotted current/phase"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dottedSecondary->SetValue(true);
	bSizerCheckboxes->Add( m_dottedSecondary, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxBoxSizer* bSizerLineWidth;
	bSizerLineWidth = new wxBoxSizer( wxHORIZONTAL );

	m_lineWidthLabel = new wxStaticText( m_appearanceSizer->GetStaticBox(), wxID_ANY, _("Line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	bSizerLineWidth->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_lineWidth = new wxSpinCtrl( m_appearanceSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 4, 2 );
	bSizerLineWidth->Add( m_lineWidth, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerCheckboxes->Add( bSizerLineWidth, 0, wxALL|wxEXPAND, 8 );


	m_appearanceSizer->Add( bSizerCheckboxes, 0, wxEXPAND|wxLEFT, 5 );


	bPlotSetupSizer->Add( m_appearanceSizer, 0, wxEXPAND|wxALL, 5 );

	m_marginsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelPlotSetup, wxID_ANY, _("Margins") ), wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxHORIZONTAL );

	m_marginLeftLabel = new wxStaticText( m_marginsSizer->GetStaticBox(), wxID_ANY, _("Left:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginLeftLabel->Wrap( -1 );
	bSizerLeft->Add( m_marginLeftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_marginLeft = new wxTextCtrl( m_marginsSizer->GetStaticBox(), wxID_ANY, _("70"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginLeft->SetMinSize( wxSize( 60,-1 ) );

	bSizerLeft->Add( m_marginLeft, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( bSizerLeft, 0, wxEXPAND|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerTopBottom;
	fgSizerTopBottom = new wxFlexGridSizer( 0, 2, 4, 0 );
	fgSizerTopBottom->SetFlexibleDirection( wxBOTH );
	fgSizerTopBottom->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_marginTopLabel = new wxStaticText( m_marginsSizer->GetStaticBox(), wxID_ANY, _("Top:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginTopLabel->Wrap( -1 );
	fgSizerTopBottom->Add( m_marginTopLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_marginTop = new wxTextCtrl( m_marginsSizer->GetStaticBox(), wxID_ANY, _("30"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginTop->SetMinSize( wxSize( 60,-1 ) );

	fgSizerTopBottom->Add( m_marginTop, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_marginBottomLabel = new wxStaticText( m_marginsSizer->GetStaticBox(), wxID_ANY, _("Bottom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginBottomLabel->Wrap( -1 );
	fgSizerTopBottom->Add( m_marginBottomLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_marginBottom = new wxTextCtrl( m_marginsSizer->GetStaticBox(), wxID_ANY, _("45"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginBottom->SetMinSize( wxSize( 60,-1 ) );

	fgSizerTopBottom->Add( m_marginBottom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	bSizerMargins->Add( fgSizerTopBottom, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxHORIZONTAL );

	m_marginRightLabel = new wxStaticText( m_marginsSizer->GetStaticBox(), wxID_ANY, _("Right:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginRightLabel->Wrap( -1 );
	bSizerRight->Add( m_marginRightLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15 );

	m_marginRight = new wxTextCtrl( m_marginsSizer->GetStaticBox(), wxID_ANY, _("70"), wxDefaultPosition, wxDefaultSize, 0 );
	m_marginRight->SetMinSize( wxSize( 60,-1 ) );

	bSizerRight->Add( m_marginRight, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerMargins->Add( bSizerRight, 0, wxEXPAND, 5 );


	m_marginsSizer->Add( bSizerMargins, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bPlotSetupSizer->Add( m_marginsSizer, 0, wxEXPAND|wxALL, 5 );


	bPlotSetupSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	m_panelPlotSetup->SetSizer( bPlotSetupSizer );
	m_panelPlotSetup->Layout();
	bPlotSetupSizer->Fit( m_panelPlotSetup );
	m_notebook1->AddPage( m_panelPlotSetup, _("Plot Setup"), false );
	m_panelSimOptions = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerSimOptions;
	bSizerSimOptions = new wxBoxSizer( wxVERTICAL );

	m_simOptionsHint = new wxStaticText( m_panelSimOptions, wxID_ANY, _("These values are passed to ngspice as .options.  Leave a field empty to use the ngspice default.  Any .options directive on the schematic overrides these."), wxDefaultPosition, wxDefaultSize, 0 );
	m_simOptionsHint->Wrap( 500 );
	bSizerSimOptions->Add( m_simOptionsHint, 0, wxALL, 10 );

	wxBoxSizer* bSizerSimOptionsColumns;
	bSizerSimOptionsColumns = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSimOptionsLeft;
	fgSimOptionsLeft = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSimOptionsLeft->AddGrowableCol( 1 );
	fgSimOptionsLeft->SetFlexibleDirection( wxHORIZONTAL );
	fgSimOptionsLeft->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_labelMethod;
	m_labelMethod = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Integration method (method):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelMethod->Wrap( -1 );
	m_labelMethod->SetToolTip( _("Numerical integration method") );

	fgSimOptionsLeft->Add( m_labelMethod, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_optMethodChoices[] = { _("(default)"), _("trap"), _("gear") };
	int m_optMethodNChoices = sizeof( m_optMethodChoices ) / sizeof( wxString );
	m_optMethod = new wxChoice( m_panelSimOptions, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_optMethodNChoices, m_optMethodChoices, 0 );
	m_optMethod->SetSelection( 0 );
	m_optMethod->SetToolTip( _("Numerical integration method") );

	fgSimOptionsLeft->Add( m_optMethod, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSimOptionsLeft->Add( 0, 0, 0, wxEXPAND, 5 );

	wxStaticText* m_labelReltol;
	m_labelReltol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Relative tolerance (reltol):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelReltol->Wrap( -1 );
	m_labelReltol->SetToolTip( _("Relative error tolerance (default 1e-3)") );

	fgSimOptionsLeft->Add( m_labelReltol, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optReltol = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optReltol->SetToolTip( _("Relative error tolerance (default 1e-3)") );

	fgSimOptionsLeft->Add( m_optReltol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSimOptionsLeft->Add( 0, 0, 0, wxEXPAND, 5 );

	wxStaticText* m_labelAbstol;
	m_labelAbstol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Absolute current tolerance (abstol):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelAbstol->Wrap( -1 );
	m_labelAbstol->SetToolTip( _("Absolute current error tolerance (default 1e-12 A)") );

	fgSimOptionsLeft->Add( m_labelAbstol, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optAbstol = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optAbstol->SetToolTip( _("Absolute current error tolerance (default 1e-12 A)") );

	fgSimOptionsLeft->Add( m_optAbstol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsAbstol;
	m_unitsAbstol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsAbstol->Wrap( -1 );
	fgSimOptionsLeft->Add( m_unitsAbstol, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelVntol;
	m_labelVntol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Absolute voltage tolerance (vntol):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelVntol->Wrap( -1 );
	m_labelVntol->SetToolTip( _("Absolute voltage error tolerance (default 1e-6 V)") );

	fgSimOptionsLeft->Add( m_labelVntol, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optVntol = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optVntol->SetToolTip( _("Absolute voltage error tolerance (default 1e-6 V)") );

	fgSimOptionsLeft->Add( m_optVntol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsVntol;
	m_unitsVntol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsVntol->Wrap( -1 );
	fgSimOptionsLeft->Add( m_unitsVntol, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelChgtol;
	m_labelChgtol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Charge tolerance (chgtol):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelChgtol->Wrap( -1 );
	m_labelChgtol->SetToolTip( _("Absolute charge error tolerance (default 1e-14 C)") );

	fgSimOptionsLeft->Add( m_labelChgtol, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optChgtol = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optChgtol->SetToolTip( _("Absolute charge error tolerance (default 1e-14 C)") );

	fgSimOptionsLeft->Add( m_optChgtol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsChgtol;
	m_unitsChgtol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsChgtol->Wrap( -1 );
	fgSimOptionsLeft->Add( m_unitsChgtol, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelTrtol;
	m_labelTrtol = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Truncation error factor (trtol):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTrtol->Wrap( -1 );
	m_labelTrtol->SetToolTip( _("Transient truncation error factor (default 7)") );

	fgSimOptionsLeft->Add( m_labelTrtol, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optTrtol = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optTrtol->SetToolTip( _("Transient truncation error factor (default 7)") );

	fgSimOptionsLeft->Add( m_optTrtol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSimOptionsLeft->Add( 0, 0, 0, wxEXPAND, 5 );


	bSizerSimOptionsColumns->Add( fgSimOptionsLeft, 1, wxEXPAND|wxRIGHT, 10 );

	wxFlexGridSizer* fgSimOptionsRight;
	fgSimOptionsRight = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSimOptionsRight->AddGrowableCol( 1 );
	fgSimOptionsRight->SetFlexibleDirection( wxHORIZONTAL );
	fgSimOptionsRight->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxStaticText* m_labelGmin;
	m_labelGmin = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Minimum conductance (gmin):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelGmin->Wrap( -1 );
	m_labelGmin->SetToolTip( _("Minimum conductance across junctions (default 1e-12 S)") );

	fgSimOptionsRight->Add( m_labelGmin, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optGmin = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optGmin->SetToolTip( _("Minimum conductance across junctions (default 1e-12 S)") );

	fgSimOptionsRight->Add( m_optGmin, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsGmin;
	m_unitsGmin = new wxStaticText( m_panelSimOptions, wxID_ANY, _("S"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsGmin->Wrap( -1 );
	fgSimOptionsRight->Add( m_unitsGmin, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelGshunt;
	m_labelGshunt = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Node-to-ground conductance (gshunt):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelGshunt->Wrap( -1 );
	m_labelGshunt->SetToolTip( _("Conductance added from each node to ground (default 0)") );

	fgSimOptionsRight->Add( m_labelGshunt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optGshunt = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optGshunt->SetToolTip( _("Conductance added from each node to ground (default 0)") );

	fgSimOptionsRight->Add( m_optGshunt, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsGshunt;
	m_unitsGshunt = new wxStaticText( m_panelSimOptions, wxID_ANY, _("S"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsGshunt->Wrap( -1 );
	fgSimOptionsRight->Add( m_unitsGshunt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelRshunt;
	m_labelRshunt = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Node-to-ground resistance (rshunt):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelRshunt->Wrap( -1 );
	m_labelRshunt->SetToolTip( _("Resistance added from each node to ground (default off)") );

	fgSimOptionsRight->Add( m_labelRshunt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optRshunt = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optRshunt->SetToolTip( _("Resistance added from each node to ground (default off)") );

	fgSimOptionsRight->Add( m_optRshunt, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsRshunt;
	m_unitsRshunt = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Ω"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsRshunt->Wrap( -1 );
	fgSimOptionsRight->Add( m_unitsRshunt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelItl1;
	m_labelItl1 = new wxStaticText( m_panelSimOptions, wxID_ANY, _("DC iteration limit (itl1):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelItl1->Wrap( -1 );
	m_labelItl1->SetToolTip( _("Operating point iteration limit (default 100)") );

	fgSimOptionsRight->Add( m_labelItl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optItl1 = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optItl1->SetToolTip( _("Operating point iteration limit (default 100)") );

	fgSimOptionsRight->Add( m_optItl1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSimOptionsRight->Add( 0, 0, 0, wxEXPAND, 5 );

	wxStaticText* m_labelItl4;
	m_labelItl4 = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Transient iteration limit (itl4):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelItl4->Wrap( -1 );
	m_labelItl4->SetToolTip( _("Iteration limit per transient time step (default 10)") );

	fgSimOptionsRight->Add( m_labelItl4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optItl4 = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optItl4->SetToolTip( _("Iteration limit per transient time step (default 10)") );

	fgSimOptionsRight->Add( m_optItl4, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSimOptionsRight->Add( 0, 0, 0, wxEXPAND, 5 );

	wxStaticText* m_labelTemp;
	m_labelTemp = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Temperature (temp):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTemp->Wrap( -1 );
	m_labelTemp->SetToolTip( _("Operating temperature (default 27 °C)") );

	fgSimOptionsRight->Add( m_labelTemp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optTemp = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optTemp->SetToolTip( _("Operating temperature (default 27 °C)") );

	fgSimOptionsRight->Add( m_optTemp, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsTemp;
	m_unitsTemp = new wxStaticText( m_panelSimOptions, wxID_ANY, _("°C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsTemp->Wrap( -1 );
	fgSimOptionsRight->Add( m_unitsTemp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxStaticText* m_labelTnom;
	m_labelTnom = new wxStaticText( m_panelSimOptions, wxID_ANY, _("Nominal temperature (tnom):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelTnom->Wrap( -1 );
	m_labelTnom->SetToolTip( _("Temperature at which model parameters were measured (default 27 °C)") );

	fgSimOptionsRight->Add( m_labelTnom, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_optTnom = new wxTextCtrl( m_panelSimOptions, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_optTnom->SetToolTip( _("Temperature at which model parameters were measured (default 27 °C)") );

	fgSimOptionsRight->Add( m_optTnom, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticText* m_unitsTnom;
	m_unitsTnom = new wxStaticText( m_panelSimOptions, wxID_ANY, _("°C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsTnom->Wrap( -1 );
	fgSimOptionsRight->Add( m_unitsTnom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerSimOptionsColumns->Add( fgSimOptionsRight, 1, wxEXPAND|wxLEFT, 10 );


	bSizerSimOptions->Add( bSizerSimOptionsColumns, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_panelSimOptions->SetSizer( bSizerSimOptions );
	m_panelSimOptions->Layout();
	bSizerSimOptions->Fit( m_panelSimOptions );
	m_notebook1->AddPage( m_panelSimOptions, _("Simulator Options"), false );

	bSizer1->Add( m_notebook1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer1->Add( m_sdbSizer, 0, wxALL|wxALIGN_RIGHT, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SIM_COMMAND_BASE::onInitDlg ) );
	m_commandType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::OnCommandType ), NULL, this );
	m_dcEnable2->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCEnableSecondSource ), NULL, this );
	m_dcSourceType1->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource1Selected ), NULL, this );
	m_dcSourceType2->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource2Selected ), NULL, this );
	m_swapDCSources->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onSwapDCSources ), NULL, this );
	m_inputSignalsFilter->Connect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SIM_COMMAND_BASE::OnFilterMouseMoved ), NULL, this );
	m_inputSignalsFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::OnFilterText ), NULL, this );
	m_loadDirectives->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onLoadDirectives ), NULL, this );
	m_pzFunctionType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource1Selected ), NULL, this );
	m_y1MinLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Min->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1MaxLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Max->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Units->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y2MinLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Min->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2MaxLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Max->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Units->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y3MinLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Min->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3MaxLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Max->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Units->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
}

DIALOG_SIM_COMMAND_BASE::~DIALOG_SIM_COMMAND_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SIM_COMMAND_BASE::onInitDlg ) );
	m_commandType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::OnCommandType ), NULL, this );
	m_dcEnable2->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCEnableSecondSource ), NULL, this );
	m_dcSourceType1->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource1Selected ), NULL, this );
	m_dcSourceType2->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource2Selected ), NULL, this );
	m_swapDCSources->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onSwapDCSources ), NULL, this );
	m_inputSignalsFilter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( DIALOG_SIM_COMMAND_BASE::OnFilterMouseMoved ), NULL, this );
	m_inputSignalsFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::OnFilterText ), NULL, this );
	m_loadDirectives->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onLoadDirectives ), NULL, this );
	m_pzFunctionType->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_SIM_COMMAND_BASE::onDCSource1Selected ), NULL, this );
	m_y1MinLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Min->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1MaxLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Max->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y1Units->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY1 ), NULL, this );
	m_y2MinLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Min->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2MaxLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Max->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y2Units->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY2 ), NULL, this );
	m_y3MinLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Min->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3MaxLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Max->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );
	m_y3Units->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SIM_COMMAND_BASE::OnUpdateUILockY3 ), NULL, this );

}
