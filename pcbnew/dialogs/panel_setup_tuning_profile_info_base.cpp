///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_setup_tuning_profile_info_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TUNING_PROFILE_INFO_BASE::PANEL_SETUP_TUNING_PROFILE_INFO_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 3, 1, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableRow( 2 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 1, 9, 0, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->AddGrowableCol( 5 );
	fgSizer2->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_nameLabel = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_nameLabel->Wrap( -1 );
	fgSizer2->Add( m_nameLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_name = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_name, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_typeLabel = new wxStaticText( this, wxID_ANY, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_typeLabel->Wrap( -1 );
	fgSizer2->Add( m_typeLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxString m_typeChoices[] = { _("Single"), _("Differential") };
	int m_typeNChoices = sizeof( m_typeChoices ) / sizeof( wxString );
	m_type = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_typeNChoices, m_typeChoices, 0 );
	m_type->SetSelection( 0 );
	fgSizer2->Add( m_type, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_targetImpedanceLabel = new wxStaticText( this, wxID_ANY, _("Target impedance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetImpedanceLabel->Wrap( -1 );
	fgSizer2->Add( m_targetImpedanceLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_targetImpedance = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_targetImpedance->HasFlag( wxTE_MULTILINE ) )
	{
	m_targetImpedance->SetMaxLength( 15 );
	}
	#else
	m_targetImpedance->SetMaxLength( 15 );
	#endif
	fgSizer2->Add( m_targetImpedance, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_ohmsLabel = new wxStaticText( this, wxID_ANY, _("ohms"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ohmsLabel->Wrap( -1 );
	fgSizer2->Add( m_ohmsLabel, 0, wxALL|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( fgSizer2, 1, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_enableDelayTuning = new wxCheckBox( this, wxID_ANY, _("Enable time domain tuning"), wxDefaultPosition, wxDefaultSize, 0 );
	m_enableDelayTuning->SetValue(true);
	gbSizer1->Add( m_enableDelayTuning, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer1->Add( m_staticline1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxEXPAND | wxALL, 5 );


	gbSizer1->AddGrowableCol( 0 );

	fgSizer1->Add( gbSizer1, 1, wxEXPAND, 5 );

	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitter1->SetSashGravity( 0.5 );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::m_splitter1OnIdle ), NULL, this );

	m_panelTrackPropagation = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerTrackPropagation;
	bSizerTrackPropagation = new wxBoxSizer( wxVERTICAL );

	m_trackPropagationLabel = new wxStaticText( m_panelTrackPropagation, wxID_ANY, _("Track Propagation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackPropagationLabel->Wrap( -1 );
	bSizerTrackPropagation->Add( m_trackPropagationLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_trackPropagationGrid = new WX_GRID( m_panelTrackPropagation, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_trackPropagationGrid->CreateGrid( 0, 6 );
	m_trackPropagationGrid->EnableEditing( true );
	m_trackPropagationGrid->EnableGridLines( true );
	m_trackPropagationGrid->EnableDragGridSize( false );
	m_trackPropagationGrid->SetMargins( 0, 0 );

	// Columns
	m_trackPropagationGrid->AutoSizeColumns();
	m_trackPropagationGrid->EnableDragColMove( false );
	m_trackPropagationGrid->EnableDragColSize( true );
	m_trackPropagationGrid->SetColLabelValue( 0, _("Signal Layer") );
	m_trackPropagationGrid->SetColLabelValue( 1, _("Top Reference") );
	m_trackPropagationGrid->SetColLabelValue( 2, _("Bottom Reference") );
	m_trackPropagationGrid->SetColLabelValue( 3, _("Track Width") );
	m_trackPropagationGrid->SetColLabelValue( 4, _("Diff Pair Gap") );
	m_trackPropagationGrid->SetColLabelValue( 5, _("Unit Delay") );
	m_trackPropagationGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_trackPropagationGrid->EnableDragRowSize( true );
	m_trackPropagationGrid->SetRowLabelSize( 0 );
	m_trackPropagationGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_trackPropagationGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerTrackPropagation->Add( m_trackPropagationGrid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_addTrackPropogationLayer = new STD_BITMAP_BUTTON( m_panelTrackPropagation, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer9->Add( m_addTrackPropogationLayer, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bSizer9->Add( 20, 0, 0, 0, 5 );

	m_deleteTrackPropogationLayer = new STD_BITMAP_BUTTON( m_panelTrackPropagation, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer9->Add( m_deleteTrackPropogationLayer, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bSizerTrackPropagation->Add( bSizer9, 0, wxEXPAND, 5 );


	m_panelTrackPropagation->SetSizer( bSizerTrackPropagation );
	m_panelTrackPropagation->Layout();
	bSizerTrackPropagation->Fit( m_panelTrackPropagation );
	m_panelViaPropagation = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerViaPropagation;
	bSizerViaPropagation = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_viaPropagationLabel = new wxStaticText( m_panelViaPropagation, wxID_ANY, _("Via Propagation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaPropagationLabel->Wrap( -1 );
	bSizer8->Add( m_viaPropagationLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizer8->Add( 0, 0, 1, wxEXPAND, 5 );

	m_viaPropagationSpeedLabel = new wxStaticText( m_panelViaPropagation, wxID_ANY, _("Global unit delay:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaPropagationSpeedLabel->Wrap( -1 );
	bSizer8->Add( m_viaPropagationSpeedLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_viaPropagationSpeed = new wxTextCtrl( m_panelViaPropagation, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( m_viaPropagationSpeed, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_viaPropSpeedUnits = new wxStaticText( m_panelViaPropagation, wxID_ANY, _("ps/cm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaPropSpeedUnits->Wrap( -1 );
	bSizer8->Add( m_viaPropSpeedUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bSizer8->Add( 50, 0, 1, wxEXPAND, 5 );


	bSizerViaPropagation->Add( bSizer8, 0, wxEXPAND, 5 );

	m_viaDelayOverridesLabel = new wxStaticText( m_panelViaPropagation, wxID_ANY, _("Via delay overrides:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viaDelayOverridesLabel->Wrap( -1 );
	bSizerViaPropagation->Add( m_viaDelayOverridesLabel, 0, wxLEFT|wxRIGHT, 5 );

	m_viaOverrides = new WX_GRID( m_panelViaPropagation, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_viaOverrides->CreateGrid( 0, 5 );
	m_viaOverrides->EnableEditing( true );
	m_viaOverrides->EnableGridLines( true );
	m_viaOverrides->EnableDragGridSize( false );
	m_viaOverrides->SetMargins( 0, 0 );

	// Columns
	m_viaOverrides->AutoSizeColumns();
	m_viaOverrides->EnableDragColMove( false );
	m_viaOverrides->EnableDragColSize( true );
	m_viaOverrides->SetColLabelValue( 0, _("Signal Layer From") );
	m_viaOverrides->SetColLabelValue( 1, _("Signal Layer To") );
	m_viaOverrides->SetColLabelValue( 2, _("Via Layer From") );
	m_viaOverrides->SetColLabelValue( 3, _("Via Layer To") );
	m_viaOverrides->SetColLabelValue( 4, _("Delay") );
	m_viaOverrides->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viaOverrides->EnableDragRowSize( true );
	m_viaOverrides->SetRowLabelSize( 0 );
	m_viaOverrides->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viaOverrides->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizerViaPropagation->Add( m_viaOverrides, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );

	m_addViaPropagationOverride = new STD_BITMAP_BUTTON( m_panelViaPropagation, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_addViaPropagationOverride, 0, wxBOTTOM|wxLEFT, 5 );


	bSizer91->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeViaPropagationOverride = new STD_BITMAP_BUTTON( m_panelViaPropagation, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer91->Add( m_removeViaPropagationOverride, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerViaPropagation->Add( bSizer91, 0, wxEXPAND, 5 );


	m_panelViaPropagation->SetSizer( bSizerViaPropagation );
	m_panelViaPropagation->Layout();
	bSizerViaPropagation->Fit( m_panelViaPropagation );
	m_splitter1->SplitHorizontally( m_panelTrackPropagation, m_panelViaPropagation, 200 );
	fgSizer1->Add( m_splitter1, 1, wxEXPAND, 5 );


	this->SetSizer( fgSizer1 );
	this->Layout();

	// Connect Events
	m_name->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnProfileNameChanged ), NULL, this );
	m_type->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnChangeProfileType ), NULL, this );
	m_addTrackPropogationLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnAddTrackRow ), NULL, this );
	m_deleteTrackPropogationLayer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnRemoveTrackRow ), NULL, this );
	m_addViaPropagationOverride->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnAddViaOverride ), NULL, this );
	m_removeViaPropagationOverride->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::OnRemoveViaOverride ), NULL, this );
}

PANEL_SETUP_TUNING_PROFILE_INFO_BASE::~PANEL_SETUP_TUNING_PROFILE_INFO_BASE()
{
}
