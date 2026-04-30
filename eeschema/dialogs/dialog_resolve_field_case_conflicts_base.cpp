///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_resolve_field_case_conflicts_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 600,400 ), wxDefaultSize );

	wxBoxSizer* m_mainSizer;
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_headerLabel = new wxStaticText( this, wxID_ANY, _("Some symbols have user fields whose names differ only in case. Choose how to resolve each before opening the table."), wxDefaultPosition, wxDefaultSize, 0 );
	m_headerLabel->Wrap( 580 );
	m_mainSizer->Add( m_headerLabel, 0, wxALL|wxEXPAND, 10 );

	m_conflictsGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_conflictsGrid->CreateGrid( 0, 5 );
	m_conflictsGrid->EnableEditing( true );
	m_conflictsGrid->EnableGridLines( true );
	m_conflictsGrid->EnableDragGridSize( false );
	m_conflictsGrid->SetMargins( 0, 0 );

	// Columns
	m_conflictsGrid->SetColSize( 0, 60 );
	m_conflictsGrid->SetColSize( 1, 80 );
	m_conflictsGrid->SetColSize( 2, 140 );
	m_conflictsGrid->SetColSize( 3, 160 );
	m_conflictsGrid->SetColSize( 4, 140 );
	m_conflictsGrid->EnableDragColMove( false );
	m_conflictsGrid->EnableDragColSize( true );
	m_conflictsGrid->SetColLabelValue( 0, _("Reference") );
	m_conflictsGrid->SetColLabelValue( 1, _("Sheet") );
	m_conflictsGrid->SetColLabelValue( 2, _("Field") );
	m_conflictsGrid->SetColLabelValue( 3, _("Value") );
	m_conflictsGrid->SetColLabelValue( 4, _("Action") );
	m_conflictsGrid->SetColLabelSize( 22 );
	m_conflictsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_conflictsGrid->EnableDragRowSize( false );
	m_conflictsGrid->SetRowLabelSize( 0 );
	m_conflictsGrid->SetRowLabelAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_conflictsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_conflictsGrid->SetMinSize( wxSize( 600,200 ) );

	m_mainSizer->Add( m_conflictsGrid, 1, wxALL|wxEXPAND, 10 );

	wxBoxSizer* m_bulkApplySizer;
	m_bulkApplySizer = new wxBoxSizer( wxHORIZONTAL );

	m_bulkApplyCheckbox = new wxCheckBox( this, wxID_ANY, _("Apply same choice to all conflicts of the same field name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bulkApplyCheckbox->SetValue(true);
	m_bulkApplySizer->Add( m_bulkApplyCheckbox, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_bulkApplySizer->Add( 0, 0, 1, wxEXPAND, 0 );

	m_separatorLabel = new wxStaticText( this, wxID_ANY, _("Join separator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_separatorLabel->Wrap( -1 );
	m_bulkApplySizer->Add( m_separatorLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_separatorCtrl = new wxTextCtrl( this, wxID_ANY, _("; "), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	#ifdef __WXGTK__
	if ( !m_separatorCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_separatorCtrl->SetMaxLength( 8 );
	}
	#else
	m_separatorCtrl->SetMaxLength( 8 );
	#endif
	m_separatorCtrl->SetToolTip( _("String inserted between values when the \"Join\" action is selected.") );

	m_bulkApplySizer->Add( m_separatorCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_mainSizer->Add( m_bulkApplySizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_stdButtonsSizer = new wxStdDialogButtonSizer();
	m_stdButtonsSizerOK = new wxButton( this, wxID_OK );
	m_stdButtonsSizer->AddButton( m_stdButtonsSizerOK );
	m_stdButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtonsSizer->AddButton( m_stdButtonsSizerCancel );
	m_stdButtonsSizer->Realize();

	m_mainSizer->Add( m_stdButtonsSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onInitDialog ) );
	m_conflictsGrid->Connect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onActionCellChanged ), NULL, this );
	m_bulkApplyCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onBulkApplyToggled ), NULL, this );
	m_stdButtonsSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onApplyAndContinue ), NULL, this );
}

DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::~DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onInitDialog ) );
	m_conflictsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED, wxGridEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onActionCellChanged ), NULL, this );
	m_bulkApplyCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onBulkApplyToggled ), NULL, this );
	m_stdButtonsSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE::onApplyAndContinue ), NULL, this );

}
