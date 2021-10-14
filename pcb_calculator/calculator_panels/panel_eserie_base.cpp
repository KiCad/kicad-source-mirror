///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eserie_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_E_SERIE_BASE::PANEL_E_SERIE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerESerie;
	bSizerESerie = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMiddleSizerESeries;
	bMiddleSizerESeries = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerESeriesInput;
	sbSizerESeriesInput = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Inputs") ), wxVERTICAL );

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

	m_e24 = new wxRadioButton( sbSizerESeriesInput->GetStaticBox(), wxID_ANY, _("E24"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer40->Add( m_e24, 0, wxALL, 5 );


	sbSizerESeriesInput->Add( bSizer40, 1, wxEXPAND, 5 );


	bMiddleSizerESeries->Add( sbSizerESeriesInput, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizerESeriesSolutions;
	sbSizerESeriesSolutions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Solutions") ), wxVERTICAL );

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
	sbLowerSizerEseriesHelp = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Help") ), wxVERTICAL );

	m_panelESeriesHelp = new HTML_WINDOW( sbLowerSizerEseriesHelp->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	sbLowerSizerEseriesHelp->Add( m_panelESeriesHelp, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLowerESerie->Add( sbLowerSizerEseriesHelp, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerESerie->Add( bLowerESerie, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerESerie );
	this->Layout();

	// Connect Events
	m_e1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnCalculateESeries ), NULL, this );
}

PANEL_E_SERIE_BASE::~PANEL_E_SERIE_BASE()
{
	// Disconnect Events
	m_e1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e6->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e12->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_e24->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnESeriesSelection ), NULL, this );
	m_buttonEScalculate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_E_SERIE_BASE::OnCalculateESeries ), NULL, this );

}
