///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_drill_chart_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_DRILL_CHART_OPTIONS_BASE::PANEL_DRILL_CHART_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : GENERATED_TABLE_OPTIONS_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMain;
	bMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bOptions;
	bOptions = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbInclude;
	sbInclude = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Include") ), wxVERTICAL );

	wxFlexGridSizer* fgFilter;
	fgFilter = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgFilter->SetFlexibleDirection( wxBOTH );
	fgFilter->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_filterPlated = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Plated holes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterPlated->SetValue(true);
	fgFilter->Add( m_filterPlated, 0, wxRIGHT, 5 );

	m_filterNonPlated = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Non-plated holes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterNonPlated->SetValue(true);
	fgFilter->Add( m_filterNonPlated, 0, wxRIGHT, 5 );

	m_filterVias = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterVias->SetValue(true);
	fgFilter->Add( m_filterVias, 0, wxRIGHT, 5 );

	m_filterSlots = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Slots"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterSlots->SetValue(true);
	fgFilter->Add( m_filterSlots, 0, wxRIGHT, 5 );

	m_filterBackdrills = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Backdrills"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterBackdrills->SetValue(true);
	fgFilter->Add( m_filterBackdrills, 0, wxRIGHT, 5 );

	m_filterCastellated = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Castellated"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterCastellated->SetValue(true);
	fgFilter->Add( m_filterCastellated, 0, wxRIGHT, 5 );


	sbInclude->Add( fgFilter, 0, wxEXPAND|wxALL, 5 );

	m_showTotals = new wxCheckBox( sbInclude->GetStaticBox(), wxID_ANY, _("Show totals row"), wxDefaultPosition, wxDefaultSize, 0 );
	m_showTotals->SetValue(true);
	sbInclude->Add( m_showTotals, 0, wxALL|wxEXPAND, 5 );


	bOptions->Add( sbInclude, 1, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbGroupBy;
	sbGroupBy = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Group holes by") ), wxVERTICAL );

	wxString m_groupByListChoices[] = { _("Hole size"), _("Slot shape"), _("Plating"), _("Layer span"), _("Drill operation"), _("Hole function"), _("IPC-4761 protection"), _("Post-machining") };
	int m_groupByListNChoices = sizeof( m_groupByListChoices ) / sizeof( wxString );
	m_groupByList = new wxCheckListBox( sbGroupBy->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_groupByListNChoices, m_groupByListChoices, 0 );
	m_groupByList->SetToolTip( _("Grouping is shared by every drill chart and drill map on the board") );
	m_groupByList->SetMinSize( wxSize( -1,140 ) );

	sbGroupBy->Add( m_groupByList, 1, wxEXPAND|wxALL, 5 );


	bOptions->Add( sbGroupBy, 1, wxEXPAND|wxALL, 5 );


	bMain->Add( bOptions, 0, wxEXPAND, 0 );

	wxBoxSizer* bTemplate;
	bTemplate = new wxBoxSizer( wxHORIZONTAL );

	m_importTemplateButton = new wxButton( this, wxID_ANY, _("Import Template..."), wxDefaultPosition, wxDefaultSize, 0 );
	bTemplate->Add( m_importTemplateButton, 0, wxRIGHT, 5 );

	m_exportTemplateButton = new wxButton( this, wxID_ANY, _("Export Template..."), wxDefaultPosition, wxDefaultSize, 0 );
	bTemplate->Add( m_exportTemplateButton, 0, wxRIGHT, 5 );


	bMain->Add( bTemplate, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMain );
	this->Layout();
	bMain->Fit( this );

	// Connect Events
	m_importTemplateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRILL_CHART_OPTIONS_BASE::onImportTemplate ), NULL, this );
	m_exportTemplateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRILL_CHART_OPTIONS_BASE::onExportTemplate ), NULL, this );
}

PANEL_DRILL_CHART_OPTIONS_BASE::~PANEL_DRILL_CHART_OPTIONS_BASE()
{
	// Disconnect Events
	m_importTemplateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRILL_CHART_OPTIONS_BASE::onImportTemplate ), NULL, this );
	m_exportTemplateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_DRILL_CHART_OPTIONS_BASE::onExportTemplate ), NULL, this );

}
