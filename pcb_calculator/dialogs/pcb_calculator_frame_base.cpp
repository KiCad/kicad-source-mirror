///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "pcb_calculator_frame_base.h"

///////////////////////////////////////////////////////////////////////////

PCB_CALCULATOR_FRAME_BASE::PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar = new wxMenuBar( 0 );
	this->SetMenuBar( m_menubar );

	wxBoxSizer* bmainFrameSizer;
	bmainFrameSizer = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelRegulators = new PANEL_REGULATOR( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelRegulators, _("Regulators"), true );
	m_panelAttenuators = new PANEL_ATTENUATORS( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelAttenuators, _("RF Attenuators"), false );
	m_panelESeries = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerESerie;
	bSizerESerie = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMiddleSizerESeries;
	bMiddleSizerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerESeriesInput;
	sbSizerESeriesInput = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Inputs") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttPrms1;
	fgSizerAttPrms1 = new wxFlexGridSizer( 4, 3, 3, 0 );
	fgSizerAttPrms1->AddGrowableRow( 1 );
	fgSizerAttPrms1->SetFlexibleDirection( wxBOTH );
	fgSizerAttPrms1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESrequired = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Required resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ResRequired = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResRequired, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_reqResUnits = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_reqResUnits->Wrap( -1 );
	fgSizerAttPrms1->Add( m_reqResUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired1 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired1->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude1 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_exclude1Units = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exclude1Units->Wrap( -1 );
	fgSizerAttPrms1->Add( m_exclude1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired11 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired11->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude2 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_exclude2Units = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exclude2Units->Wrap( -1 );
	fgSizerAttPrms1->Add( m_exclude2Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerESeriesInput->Add( fgSizerAttPrms1, 0, wxEXPAND|wxBOTTOM, 5 );

	m_staticline6 = new wxStaticLine( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerESeriesInput->Add( m_staticline6, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxHORIZONTAL );

	m_e1 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E1"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer40->Add( m_e1, 1, wxALL, 5 );

	m_e3 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E3"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e3, 1, wxALL, 5 );

	m_e6 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_e6->SetValue( true );
	bSizer40->Add( m_e6, 1, wxALL, 5 );

	m_e12 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E12"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e12, 1, wxALL, 5 );


	sbSizerESeriesInput->Add( bSizer40, 1, wxEXPAND, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesInput, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerESeriesSolutions;
	sbSizerESeriesSolutions = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Solutions") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerESerieResults;
	fgSizerESerieResults = new wxFlexGridSizer( 6, 5, 3, 0 );
	fgSizerESerieResults->AddGrowableCol( 1 );
	fgSizerESerieResults->SetFlexibleDirection( wxBOTH );
	fgSizerESerieResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESerieSimpleSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Simple solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESerieSimpleSolution->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESerieSimpleSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol2R->SetMinSize( wxSize( 150,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesSimpleErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimpleErr->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesSimpleErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ESeriesSimplePercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimplePercent->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesSimplePercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESerie3RSolution1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("3R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESerie3RSolution1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESerie3RSolution1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol3R->SetMinSize( wxSize( 220,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol3R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesAltErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError3R, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ESeriesAltPercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltPercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESeries4RSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("4R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries4RSolution->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeries4RSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol4R->SetMinSize( wxSize( 290,-1 ) );

	fgSizerESerieResults->Add( m_ESeries_Sol4R, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_ESeriesAltErr1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Error:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltErr1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesError4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESerieResults->Add( m_ESeriesError4R, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeriesAltPercent1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent1->Wrap( -1 );
	fgSizerESerieResults->Add( m_ESeriesAltPercent1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerESeriesSolutions->Add( fgSizerESerieResults, 0, wxBOTTOM|wxEXPAND, 5 );

	m_staticline7 = new wxStaticLine( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerESeriesSolutions->Add( m_staticline7, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_buttonEScalculate = new wxButton( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerESeriesSolutions->Add( m_buttonEScalculate, 0, wxALL, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesSolutions, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer47;
	bSizer47 = new wxBoxSizer( wxVERTICAL );


	bMiddleSizerESeries->Add( bSizer47, 1, wxALIGN_BOTTOM, 5 );


	bSizerESerie->Add( bMiddleSizerESeries, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bLowerESerie;
	bLowerESerie = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbLowerSizerEseriesHelp;
	sbLowerSizerEseriesHelp = new wxStaticBoxSizer( new wxStaticBox( m_panelESeries, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelESeriesHelp = new wxHtmlWindow( sbLowerSizerEseriesHelp->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbLowerSizerEseriesHelp->Add( m_panelESeriesHelp, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLowerESerie->Add( sbLowerSizerEseriesHelp, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerESerie->Add( bLowerESerie, 1, wxEXPAND, 5 );


	m_panelESeries->SetSizer( bSizerESerie );
	m_panelESeries->Layout();
	bSizerESerie->Fit( m_panelESeries );
	m_Notebook->AddPage( m_panelESeries, _("E-Series"), false );
	m_panelColorCode = new PANEL_COLOR_CODE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelColorCode, _("Color Code"), false );
	m_panelTransline = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizeTransline;
	bSizeTransline = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxString m_TranslineSelectionChoices[] = { _("Microstrip Line"), _("Coplanar wave guide"), _("Coplanar wave guide w/ ground plane"), _("Rectangular Waveguide"), _("Coaxial Line"), _("Coupled Microstrip Line"), _("Stripline"), _("Twisted Pair") };
	int m_TranslineSelectionNChoices = sizeof( m_TranslineSelectionChoices ) / sizeof( wxString );
	m_TranslineSelection = new wxRadioBox( m_panelTransline, wxID_ANY, _("Transmission Line Type"), wxDefaultPosition, wxDefaultSize, m_TranslineSelectionNChoices, m_TranslineSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_TranslineSelection->SetSelection( 2 );
	bLeftSizer->Add( m_TranslineSelection, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_translineBitmap = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_translineBitmap, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );


	bSizeTransline->Add( bLeftSizer, 0, wxEXPAND, 5 );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSubstrateBoxSizer;
	sbSubstrateBoxSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Substrate Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerSubstPrms;
	fgSizerSubstPrms = new wxFlexGridSizer( 9, 3, 3, 0 );
	fgSizerSubstPrms->AddGrowableCol( 1 );
	fgSizerSubstPrms->SetFlexibleDirection( wxBOTH );
	fgSizerSubstPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_EpsilonR_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Er:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EpsilonR_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_EpsilonR_label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxBoxSizer* bSizer441;
	bSizer441 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_EpsilonR = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer441->Add( m_Value_EpsilonR, 1, wxEXPAND|wxLEFT, 5 );

	m_button_EpsilonR = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer441->Add( m_button_EpsilonR, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer441, 1, wxEXPAND, 5 );


	fgSizerSubstPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_TanD_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Tan delta:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TanD_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_TanD_label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxBoxSizer* bSizer442;
	bSizer442 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_TanD = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer442->Add( m_Value_TanD, 1, wxEXPAND|wxLEFT, 5 );

	m_button_TanD = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer442->Add( m_button_TanD, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer442, 1, wxEXPAND, 5 );


	fgSizerSubstPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_Rho_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rho:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Rho_label->Wrap( -1 );
	m_Rho_label->SetToolTip( _("Specific resistance in ohms * meters") );

	fgSizerSubstPrms->Add( m_Rho_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer443;
	bSizer443 = new wxBoxSizer( wxHORIZONTAL );

	m_Value_Rho = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer443->Add( m_Value_Rho, 1, wxEXPAND|wxLEFT, 5 );

	m_button_Rho = new wxButton( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer443->Add( m_button_Rho, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerSubstPrms->Add( bSizer443, 1, wxEXPAND, 5 );


	fgSizerSubstPrms->Add( 0, 0, 1, wxEXPAND, 5 );

	m_substrate_prm4_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm4_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm4_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm4_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm4_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm4_choiceUnitChoices;
	m_SubsPrm4_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm4_choiceUnitChoices, 0 );
	m_SubsPrm4_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm4_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm5_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("H_t:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm5_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm5_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm5_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm5_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm5_choiceUnitChoices;
	m_SubsPrm5_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm5_choiceUnitChoices, 0 );
	m_SubsPrm5_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm5_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm6_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("T:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm6_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm6_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm6_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm6_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm6_choiceUnitChoices;
	m_SubsPrm6_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm6_choiceUnitChoices, 0 );
	m_SubsPrm6_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm6_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm7_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Rough:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm7_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm7_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm7_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm7_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm7_choiceUnitChoices;
	m_SubsPrm7_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm7_choiceUnitChoices, 0 );
	m_SubsPrm7_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm7_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm8_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Insulator mu:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm8_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm8_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Substrate_prm8_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm8_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm8_choiceUnitChoices;
	m_SubsPrm8_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm8_choiceUnitChoices, 0 );
	m_SubsPrm8_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm8_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_substrate_prm9_label = new wxStaticText( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, _("Conductor mu:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_substrate_prm9_label->Wrap( -1 );
	fgSizerSubstPrms->Add( m_substrate_prm9_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Substrate_prm9_Value = new wxTextCtrl( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSubstPrms->Add( m_Substrate_prm9_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_SubsPrm9_choiceUnitChoices;
	m_SubsPrm9_choiceUnit = new UNIT_SELECTOR_LEN( sbSubstrateBoxSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SubsPrm9_choiceUnitChoices, 0 );
	m_SubsPrm9_choiceUnit->SetSelection( 0 );
	fgSizerSubstPrms->Add( m_SubsPrm9_choiceUnit, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSubstrateBoxSizer->Add( fgSizerSubstPrms, 1, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizer->Add( sbSubstrateBoxSizer, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbCmpPrmsSizer;
	sbCmpPrmsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Component Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizeCmpPrms;
	fgSizeCmpPrms = new wxFlexGridSizer( 1, 3, 0, 0 );
	fgSizeCmpPrms->AddGrowableCol( 1 );
	fgSizeCmpPrms->SetFlexibleDirection( wxBOTH );
	fgSizeCmpPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Frequency_label = new wxStaticText( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Frequency_label->Wrap( -1 );
	fgSizeCmpPrms->Add( m_Frequency_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Value_Frequency_Ctrl = new wxTextCtrl( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizeCmpPrms->Add( m_Value_Frequency_Ctrl, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxArrayString m_choiceUnit_FrequencyChoices;
	m_choiceUnit_Frequency = new UNIT_SELECTOR_FREQUENCY( sbCmpPrmsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_FrequencyChoices, 0 );
	m_choiceUnit_Frequency->SetSelection( 0 );
	fgSizeCmpPrms->Add( m_choiceUnit_Frequency, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbCmpPrmsSizer->Add( fgSizeCmpPrms, 0, wxEXPAND|wxBOTTOM, 5 );


	bMiddleSizer->Add( sbCmpPrmsSizer, 0, wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerHelpBitmaps;
	bSizerHelpBitmaps = new wxBoxSizer( wxVERTICAL );

	m_bmCMicrostripZoddZeven = new wxStaticBitmap( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerHelpBitmaps->Add( m_bmCMicrostripZoddZeven, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 10 );

	m_fgSizerZcomment = new wxFlexGridSizer( 0, 2, 0, 15 );
	m_fgSizerZcomment->AddGrowableCol( 0 );
	m_fgSizerZcomment->AddGrowableCol( 1 );
	m_fgSizerZcomment->SetFlexibleDirection( wxBOTH );
	m_fgSizerZcomment->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextZdiff = new wxStaticText( m_panelTransline, wxID_ANY, _("Zdiff =\n2*Z0( (1 - 0.48exp( -0.96*S/H ) )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextZdiff->Wrap( -1 );
	m_staticTextZdiff->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgSizerZcomment->Add( m_staticTextZdiff, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticTextZcommon = new wxStaticText( m_panelTransline, wxID_ANY, _("Zcommon = Zeven / 2"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticTextZcommon->Wrap( -1 );
	m_staticTextZcommon->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	m_fgSizerZcomment->Add( m_staticTextZcommon, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


	bSizerHelpBitmaps->Add( m_fgSizerZcomment, 0, wxEXPAND, 5 );


	bMiddleSizer->Add( bSizerHelpBitmaps, 1, wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizeTransline->Add( bMiddleSizer, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* btranslineRightSizer;
	btranslineRightSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Physical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerPhysPrms;
	fgSizerPhysPrms = new wxFlexGridSizer( 4, 4, 3, 0 );
	fgSizerPhysPrms->AddGrowableCol( 1 );
	fgSizerPhysPrms->SetFlexibleDirection( wxBOTH );
	fgSizerPhysPrms->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_phys_prm1_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm1_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm1_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm1_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param1Choices;
	m_choiceUnit_Param1 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param1Choices, 0 );
	m_choiceUnit_Param1->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_radioBtnPrm1 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	fgSizerPhysPrms->Add( m_radioBtnPrm1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_phys_prm2_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm2_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm2_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm2_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm2_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param2Choices;
	m_choiceUnit_Param2 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param2Choices, 0 );
	m_choiceUnit_Param2->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param2, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_radioBtnPrm2 = new wxRadioButton( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_radioBtnPrm2, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_phys_prm3_label = new wxStaticText( btranslineRightSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_phys_prm3_label->Wrap( -1 );
	fgSizerPhysPrms->Add( m_phys_prm3_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Phys_prm3_Value = new wxTextCtrl( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPhysPrms->Add( m_Phys_prm3_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_Param3Choices;
	m_choiceUnit_Param3 = new UNIT_SELECTOR_LEN( btranslineRightSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_Param3Choices, 0 );
	m_choiceUnit_Param3->SetSelection( 0 );
	fgSizerPhysPrms->Add( m_choiceUnit_Param3, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerPhysPrms->Add( 0, 0, 0, 0, 5 );


	btranslineRightSizer->Add( fgSizerPhysPrms, 0, wxEXPAND|wxBOTTOM, 5 );


	bRightSizer->Add( btranslineRightSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* btranslineButtonsSizer;
	btranslineButtonsSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_bpButtonAnalyze = new wxBitmapButton( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonAnalyze, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_AnalyseButton = new wxButton( m_panelTransline, wxID_ANY, _("Analyze"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_AnalyseButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_SynthetizeButton = new wxButton( m_panelTransline, wxID_ANY, _("Synthesize"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_SynthetizeButton, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_bpButtonSynthetize = new wxBitmapButton( m_panelTransline, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerButtons->Add( m_bpButtonSynthetize, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	btranslineButtonsSizer->Add( bSizerButtons, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


	bRightSizer->Add( btranslineButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	wxStaticBoxSizer* sbElectricalResultsSizer;
	sbElectricalResultsSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Electrical Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerResults;
	fgSizerResults = new wxFlexGridSizer( 3, 3, 3, 0 );
	fgSizerResults->AddGrowableCol( 1 );
	fgSizerResults->SetFlexibleDirection( wxBOTH );
	fgSizerResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_elec_prm1_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm1_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm1_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Elec_prm1_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm1_Value, 0, wxRIGHT|wxLEFT|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm1Choices;
	m_choiceUnit_ElecPrm1 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm1Choices, 0 );
	m_choiceUnit_ElecPrm1->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_elec_prm2_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm2_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm2_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_Elec_prm2_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm2_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm2Choices;
	m_choiceUnit_ElecPrm2 = new UNIT_SELECTOR_RESISTOR( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm2Choices, 0 );
	m_choiceUnit_ElecPrm2->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm2, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_elec_prm3_label = new wxStaticText( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_elec_prm3_label->Wrap( -1 );
	fgSizerResults->Add( m_elec_prm3_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Elec_prm3_Value = new wxTextCtrl( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerResults->Add( m_Elec_prm3_Value, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceUnit_ElecPrm3Choices;
	m_choiceUnit_ElecPrm3 = new UNIT_SELECTOR_ANGLE( sbElectricalResultsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnit_ElecPrm3Choices, 0 );
	m_choiceUnit_ElecPrm3->SetSelection( 0 );
	fgSizerResults->Add( m_choiceUnit_ElecPrm3, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbElectricalResultsSizer->Add( fgSizerResults, 0, wxEXPAND|wxBOTTOM, 5 );


	bRightSizer->Add( sbElectricalResultsSizer, 0, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbMessagesSizer;
	sbMessagesSizer = new wxStaticBoxSizer( new wxStaticBox( m_panelTransline, wxID_ANY, _("Results") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerTranslResults;
	fgSizerTranslResults = new wxFlexGridSizer( 8, 2, 0, 0 );
	fgSizerTranslResults->AddGrowableCol( 1 );
	fgSizerTranslResults->SetFlexibleDirection( wxBOTH );
	fgSizerTranslResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_left_message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message1, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message1 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message1->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message1, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message2 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message2->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_left_message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message3, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message3 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message3->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message3, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message4, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message4 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message4->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message4, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxALIGN_RIGHT, 5 );

	m_Message5 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message5->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message5, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message6, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message6 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message6->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message6, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message7, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	m_Message7 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message7->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message7, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_left_message8 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_left_message8->Wrap( -1 );
	fgSizerTranslResults->Add( m_left_message8, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_Message8 = new wxStaticText( sbMessagesSizer->GetStaticBox(), wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Message8->Wrap( -1 );
	fgSizerTranslResults->Add( m_Message8, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbMessagesSizer->Add( fgSizerTranslResults, 1, wxEXPAND, 5 );


	bRightSizer->Add( sbMessagesSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_buttonTransLineReset = new wxButton( m_panelTransline, wxID_ANY, _("Reset to Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonTransLineReset, 0, wxALIGN_RIGHT|wxALL, 5 );


	bSizeTransline->Add( bRightSizer, 1, wxEXPAND, 5 );


	m_panelTransline->SetSizer( bSizeTransline );
	m_panelTransline->Layout();
	bSizeTransline->Fit( m_panelTransline );
	m_Notebook->AddPage( m_panelTransline, _("TransLine"), false );
	m_panelViaSize = new PANEL_VIA_SIZE( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelViaSize, _("Via Size"), false );
	m_panelTrackWidth = new PANEL_TRACK_WIDTH( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelTrackWidth, _("Track Width"), false );
	m_panelElectricalSpacing = new PANEL_ELECTRICAL_SPACING( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelElectricalSpacing, _("Electrical Spacing"), false );
	m_panelBoardClass = new PANEL_BOARD_CLASS( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Notebook->AddPage( m_panelBoardClass, _("Board Classes"), false );

	bmainFrameSizer->Add( m_Notebook, 1, wxEXPAND, 5 );


	this->SetSizer( bmainFrameSizer );
	this->Layout();
	bmainFrameSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );
	m_e1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateESeries ), NULL, this );
	m_TranslineSelection->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_bpButtonAnalyze->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_AnalyseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTransLineResetButtonClick ), NULL, this );
}

PCB_CALCULATOR_FRAME_BASE::~PCB_CALCULATOR_FRAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME_BASE::OnClosePcbCalc ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME_BASE::OnUpdateUI ) );
	m_e1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnCalculateESeries ), NULL, this );
	m_TranslineSelection->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSelection ), NULL, this );
	m_button_EpsilonR->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineEpsilonR_Button ), NULL, this );
	m_button_TanD->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineTanD_Button ), NULL, this );
	m_button_Rho->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineRho_Button ), NULL, this );
	m_bpButtonAnalyze->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_AnalyseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineAnalyse ), NULL, this );
	m_SynthetizeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_bpButtonSynthetize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTranslineSynthetize ), NULL, this );
	m_buttonTransLineReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PCB_CALCULATOR_FRAME_BASE::OnTransLineResetButtonClick ), NULL, this );

}
