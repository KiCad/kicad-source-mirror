///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_panel.h"

#include "panel_setup_time_domain_parameters_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bpanelTomeDomainSizer;
	bpanelTomeDomainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitter->SetMinimumPaneSize( 160 );

	m_timeDomainParametersPane = new WX_PANEL( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText3 = new wxStaticText( m_timeDomainParametersPane, wxID_ANY, _("Delay Profiles"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bUpperSizer->Add( m_staticText3, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bUpperSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	m_tracePropagationGrid = new WX_GRID( m_timeDomainParametersPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );

	// Grid
	m_tracePropagationGrid->CreateGrid( 0, 2 );
	m_tracePropagationGrid->EnableEditing( true );
	m_tracePropagationGrid->EnableGridLines( true );
	m_tracePropagationGrid->EnableDragGridSize( false );
	m_tracePropagationGrid->SetMargins( 0, 0 );

	// Columns
	m_tracePropagationGrid->EnableDragColMove( false );
	m_tracePropagationGrid->EnableDragColSize( true );
	m_tracePropagationGrid->SetColLabelValue( 0, _("Profile Name") );
	m_tracePropagationGrid->SetColLabelValue( 1, _("Vias") );
	m_tracePropagationGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_tracePropagationGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_tracePropagationGrid->EnableDragRowSize( true );
	m_tracePropagationGrid->SetRowLabelSize( 0 );
	m_tracePropagationGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_tracePropagationGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bUpperSizer->Add( m_tracePropagationGrid, 1, wxEXPAND|wxFIXED_MINSIZE|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* buttonBoxSizer;
	buttonBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addDelayProfileButton = new STD_BITMAP_BUTTON( m_timeDomainParametersPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_addDelayProfileButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeDelayProfileButton = new STD_BITMAP_BUTTON( m_timeDomainParametersPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_removeDelayProfileButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer->Add( 20, 0, 0, wxEXPAND, 5 );


	bUpperSizer->Add( buttonBoxSizer, 0, wxEXPAND|wxTOP, 3 );


	m_timeDomainParametersPane->SetSizer( bUpperSizer );
	m_timeDomainParametersPane->Layout();
	bUpperSizer->Fit( m_timeDomainParametersPane );
	m_viaDelayOverridesPane = new WX_PANEL( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bUpperSizer1;
	bUpperSizer1 = new wxBoxSizer( wxVERTICAL );

	m_staticText31 = new wxStaticText( m_viaDelayOverridesPane, wxID_ANY, _("Via Delay Overrides"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	bUpperSizer1->Add( m_staticText31, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bUpperSizer1->Add( 0, 3, 0, wxEXPAND, 5 );

	m_viaPropagationGrid = new WX_GRID( m_viaDelayOverridesPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );

	// Grid
	m_viaPropagationGrid->CreateGrid( 0, 6 );
	m_viaPropagationGrid->EnableEditing( true );
	m_viaPropagationGrid->EnableGridLines( true );
	m_viaPropagationGrid->EnableDragGridSize( false );
	m_viaPropagationGrid->SetMargins( 0, 0 );

	// Columns
	m_viaPropagationGrid->EnableDragColMove( false );
	m_viaPropagationGrid->EnableDragColSize( true );
	m_viaPropagationGrid->SetColLabelValue( 0, _("Profile Name") );
	m_viaPropagationGrid->SetColLabelValue( 1, _("Signal Layer From") );
	m_viaPropagationGrid->SetColLabelValue( 2, _("Signal Layer To") );
	m_viaPropagationGrid->SetColLabelValue( 3, _("Via Layer From") );
	m_viaPropagationGrid->SetColLabelValue( 4, _("Via Layer To") );
	m_viaPropagationGrid->SetColLabelValue( 5, _("Delay") );
	m_viaPropagationGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_viaPropagationGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_viaPropagationGrid->EnableDragRowSize( true );
	m_viaPropagationGrid->SetRowLabelSize( 0 );
	m_viaPropagationGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_viaPropagationGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bUpperSizer1->Add( m_viaPropagationGrid, 1, wxEXPAND|wxFIXED_MINSIZE|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* buttonBoxSizer1;
	buttonBoxSizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_addViaOverrideButton = new STD_BITMAP_BUTTON( m_viaDelayOverridesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer1->Add( m_addViaOverrideButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer1->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeViaOverrideButton = new STD_BITMAP_BUTTON( m_viaDelayOverridesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer1->Add( m_removeViaOverrideButton, 0, wxBOTTOM|wxLEFT, 5 );


	buttonBoxSizer1->Add( 20, 0, 0, wxEXPAND, 5 );


	bUpperSizer1->Add( buttonBoxSizer1, 0, wxEXPAND|wxTOP, 3 );


	m_viaDelayOverridesPane->SetSizer( bUpperSizer1 );
	m_viaDelayOverridesPane->Layout();
	bUpperSizer1->Fit( m_viaDelayOverridesPane );
	m_splitter->SplitHorizontally( m_timeDomainParametersPane, m_viaDelayOverridesPane, -1 );
	bMargins->Add( m_splitter, 1, wxEXPAND, 10 );


	bpanelTomeDomainSizer->Add( bMargins, 1, wxEXPAND|wxTOP, 2 );


	this->SetSizer( bpanelTomeDomainSizer );
	this->Layout();
	bpanelTomeDomainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnUpdateUI ) );
	m_tracePropagationGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnSizeTraceParametersGrid ), NULL, this );
	m_addDelayProfileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnAddDelayProfileClick ), NULL, this );
	m_removeDelayProfileButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnRemoveDelayProfileClick ), NULL, this );
	m_viaPropagationGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnSizeTraceParametersGrid ), NULL, this );
	m_addViaOverrideButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnAddViaOverrideClick ), NULL, this );
	m_removeViaOverrideButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnRemoveViaOverrideClick ), NULL, this );
}

PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::~PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnUpdateUI ) );
	m_tracePropagationGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnSizeTraceParametersGrid ), NULL, this );
	m_addDelayProfileButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnAddDelayProfileClick ), NULL, this );
	m_removeDelayProfileButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnRemoveDelayProfileClick ), NULL, this );
	m_viaPropagationGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnSizeTraceParametersGrid ), NULL, this );
	m_addViaOverrideButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnAddViaOverrideClick ), NULL, this );
	m_removeViaOverrideButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE::OnRemoveViaOverrideClick ), NULL, this );

}
