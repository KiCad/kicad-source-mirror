///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_r_calculator_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_R_CALCULATOR_BASE::PANEL_R_CALCULATOR_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerESeries;
	bSizerESeries = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMiddleSizerESeries;
	bMiddleSizerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerESeriesInput;
	sbSizerESeriesInput = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Inputs") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerAttPrms1;
	fgSizerAttPrms1 = new wxFlexGridSizer( 4, 3, 3, 0 );
	fgSizerAttPrms1->AddGrowableCol( 1 );
	fgSizerAttPrms1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizerAttPrms1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESrequired = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Required resistance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_ResRequired = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResRequired, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_reqResUnits = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_reqResUnits->Wrap( -1 );
	fgSizerAttPrms1->Add( m_reqResUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired1 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired1->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude1 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_exclude1Units = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("kOhm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_exclude1Units->Wrap( -1 );
	fgSizerAttPrms1->Add( m_exclude1Units, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESrequired11 = new wxStaticText( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("Exclude value 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESrequired11->Wrap( -1 );
	fgSizerAttPrms1->Add( m_ESrequired11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ResExclude2 = new wxTextCtrl( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerAttPrms1->Add( m_ResExclude2, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

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

	m_e24 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E24"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e24, 0, wxALL, 5 );


	sbSizerESeriesInput->Add( bSizer40, 1, wxEXPAND, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesInput, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerESeriesSolutions;
	sbSizerESeriesSolutions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Solutions") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerESeriesResults;
	fgSizerESeriesResults = new wxFlexGridSizer( 6, 5, 3, 0 );
	fgSizerESeriesResults->AddGrowableCol( 1 );
	fgSizerESeriesResults->SetFlexibleDirection( wxBOTH );
	fgSizerESeriesResults->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ESeriesSimpleSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Simple solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimpleSolution->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesSimpleSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol2R->SetMinSize( wxSize( 200,-1 ) );

	fgSizerESeriesResults->Add( m_ESeries_Sol2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesSimpleErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Approximation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimpleErr->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesSimpleErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_ESeriesError2R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesResults->Add( m_ESeriesError2R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_ESeriesSimplePercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesSimplePercent->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesSimplePercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESeries3RSolution1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("3R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries3RSolution1->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeries3RSolution1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol3R->SetMinSize( wxSize( 200,-1 ) );

	fgSizerESeriesResults->Add( m_ESeries_Sol3R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_ESeriesAltErr = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Approximation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesAltErr, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_ESeriesError3R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesResults->Add( m_ESeriesError3R, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 3 );

	m_ESeriesAltPercent = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesAltPercent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_ESeries4RSolution = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("4R solution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries4RSolution->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeries4RSolution, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_ESeries_Sol4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeries_Sol4R->SetMinSize( wxSize( 200,-1 ) );

	fgSizerESeriesResults->Add( m_ESeries_Sol4R, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxEXPAND, 5 );

	m_ESeriesAltErr1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Approximation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltErr1->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesAltErr1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	m_ESeriesError4R = new wxTextCtrl( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerESeriesResults->Add( m_ESeriesError4R, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

	m_ESeriesAltPercent1 = new wxStaticText( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ESeriesAltPercent1->Wrap( -1 );
	fgSizerESeriesResults->Add( m_ESeriesAltPercent1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	sbSizerESeriesSolutions->Add( fgSizerESeriesResults, 0, wxBOTTOM|wxEXPAND, 5 );

	m_staticline7 = new wxStaticLine( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerESeriesSolutions->Add( m_staticline7, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_buttonEScalculate = new wxButton( sbSizerESeriesSolutions->GetStaticBox(), wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerESeriesSolutions->Add( m_buttonEScalculate, 0, wxALL, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesSolutions, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer47;
	bSizer47 = new wxBoxSizer( wxVERTICAL );


	bMiddleSizerESeries->Add( bSizer47, 1, wxALIGN_BOTTOM, 5 );


	bSizerESeries->Add( bMiddleSizerESeries, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

	wxBoxSizer* bLowerESeries;
	bLowerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbLowerSizerEseriesHelp;
	sbLowerSizerEseriesHelp = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelESeriesHelp = new HTML_WINDOW( sbLowerSizerEseriesHelp->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_panelESeriesHelp->SetMinSize( wxSize( -1,100 ) );

	sbLowerSizerEseriesHelp->Add( m_panelESeriesHelp, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLowerESeries->Add( sbLowerSizerEseriesHelp, 1, wxEXPAND|wxALL, 5 );


	bSizerESeries->Add( bLowerESeries, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	this->SetSizer( bSizerESeries );
	this->Layout();
	bSizerESeries->Fit( this );

	// Connect Events
	m_e1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnCalculateESeries ), NULL, this );
}

PANEL_R_CALCULATOR_BASE::~PANEL_R_CALCULATOR_BASE()
{
	// Disconnect Events
	m_e1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_R_CALCULATOR_BASE::OnCalculateESeries ), NULL, this );

}
