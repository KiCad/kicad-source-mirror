///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sch_selection_filter_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SCH_SELECTION_FILTER_BASE::PANEL_SCH_SELECTION_FILTER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : WX_PANEL( parent, id, pos, size, style, name )
{
	m_gridSizer = new wxGridBagSizer( 0, 0 );
	m_gridSizer->SetFlexibleDirection( wxBOTH );
	m_gridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbAllItems = new wxCheckBox( this, wxID_ANY, _("All items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAllItems->SetValue(true);
	m_gridSizer->Add( m_cbAllItems, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxTOP, 5 );

	m_cbRuleAreas = new wxCheckBox( this, wxID_ANY, _("Rule Areas"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbRuleAreas->SetValue(true);
	m_gridSizer->Add( m_cbRuleAreas, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbLockedItems = new wxCheckBox( this, wxID_ANY, _("Locked items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbLockedItems->SetValue(true);
	m_cbLockedItems->Hide();
	m_cbLockedItems->SetToolTip( _("Allow selection of locked items") );

	m_gridSizer->Add( m_cbLockedItems, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT|wxTOP, 5 );

	m_cbSymbols = new wxCheckBox( this, wxID_ANY, _("Symbols"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbSymbols->SetValue(true);
	m_gridSizer->Add( m_cbSymbols, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbPins = new wxCheckBox( this, wxID_ANY, _("Pins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbPins->SetValue(true);
	m_gridSizer->Add( m_cbPins, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbWires = new wxCheckBox( this, wxID_ANY, _("Wires"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbWires->SetValue(true);
	m_gridSizer->Add( m_cbWires, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbLabels = new wxCheckBox( this, wxID_ANY, _("Labels"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbLabels->SetValue(true);
	m_gridSizer->Add( m_cbLabels, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbGraphics = new wxCheckBox( this, wxID_ANY, _("Graphics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbGraphics->SetValue(true);
	m_cbGraphics->SetToolTip( _("Graphical shapes") );

	m_gridSizer->Add( m_cbGraphics, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbImages = new wxCheckBox( this, wxID_ANY, _("Images"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbImages->SetValue(true);
	m_gridSizer->Add( m_cbImages, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbText = new wxCheckBox( this, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbText->SetValue(true);
	m_gridSizer->Add( m_cbText, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT, 5 );

	m_cbOtherItems = new wxCheckBox( this, wxID_ANY, _("Other items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbOtherItems->SetValue(true);
	m_gridSizer->Add( m_cbOtherItems, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( m_gridSizer );
	this->Layout();
	m_gridSizer->Fit( this );

	// Connect Events
	m_cbAllItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLockedItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbSymbols->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbWires->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLabels->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbGraphics->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbImages->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbText->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbOtherItems->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
}

PANEL_SCH_SELECTION_FILTER_BASE::~PANEL_SCH_SELECTION_FILTER_BASE()
{
	// Disconnect Events
	m_cbAllItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLockedItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbSymbols->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbWires->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbLabels->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbGraphics->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbImages->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbText->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );
	m_cbOtherItems->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SCH_SELECTION_FILTER_BASE::OnFilterChanged ), NULL, this );

}
