///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_selection_filter_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SELECTION_FILTER_BASE::PANEL_SELECTION_FILTER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : WX_PANEL( parent, id, pos, size, style, name )
{
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbAllItems = new wxCheckBox( this, wxID_ANY, _("All items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAllItems->SetValue(true);
	gbSizer1->Add( m_cbAllItems, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxTOP, 5 );

	m_cbLockedItems = new wxCheckBox( this, wxID_ANY, _("Locked items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbLockedItems->SetValue(true);
	m_cbLockedItems->SetToolTip( _("Allow selection of locked items") );

	gbSizer1->Add( m_cbLockedItems, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbFootprints = new wxCheckBox( this, wxID_ANY, _("Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbFootprints->SetValue(true);
	gbSizer1->Add( m_cbFootprints, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbText = new wxCheckBox( this, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbText->SetValue(true);
	gbSizer1->Add( m_cbText, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbTracks = new wxCheckBox( this, wxID_ANY, _("Tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbTracks->SetValue(true);
	gbSizer1->Add( m_cbTracks, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbVias = new wxCheckBox( this, wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbVias->SetValue(true);
	gbSizer1->Add( m_cbVias, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbPads = new wxCheckBox( this, wxID_ANY, _("Pads"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPads->SetValue(true);
	gbSizer1->Add( m_cbPads, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbGraphics = new wxCheckBox( this, wxID_ANY, _("Graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbGraphics->SetValue(true);
	gbSizer1->Add( m_cbGraphics, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbZones = new wxCheckBox( this, wxID_ANY, _("Zones"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbZones->SetValue(true);
	gbSizer1->Add( m_cbZones, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbKeepouts = new wxCheckBox( this, wxID_ANY, _("Rule Areas"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbKeepouts->SetValue(true);
	gbSizer1->Add( m_cbKeepouts, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbDimensions = new wxCheckBox( this, wxID_ANY, _("Dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDimensions->SetValue(true);
	gbSizer1->Add( m_cbDimensions, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbPoints = new wxCheckBox( this, wxID_ANY, _("Points"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPoints->SetValue(true);
	gbSizer1->Add( m_cbPoints, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_cbOtherItems = new wxCheckBox( this, wxID_ANY, _("Other items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOtherItems->SetValue(true);
	gbSizer1->Add( m_cbOtherItems, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	// Connect Events
	m_cbAllItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLockedItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbFootprints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbText->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbTracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbVias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPads->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbGraphics->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbZones->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbKeepouts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbDimensions->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPoints->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbOtherItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
}

PANEL_SELECTION_FILTER_BASE::~PANEL_SELECTION_FILTER_BASE()
{
	// Disconnect Events
	m_cbAllItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLockedItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbFootprints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbText->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbTracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbVias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPads->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbGraphics->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbZones->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbKeepouts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbDimensions->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPoints->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbOtherItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );

}
