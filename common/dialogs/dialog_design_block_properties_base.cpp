///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "dialog_design_block_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::DIALOG_DESIGN_BLOCK_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Default Fields") ), wxVERTICAL );

	m_fieldsGrid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_fieldsGrid->CreateGrid( 1, 2 );
	m_fieldsGrid->EnableEditing( true );
	m_fieldsGrid->EnableGridLines( true );
	m_fieldsGrid->EnableDragGridSize( false );
	m_fieldsGrid->SetMargins( 0, 0 );

	// Columns
	m_fieldsGrid->SetColSize( 0, 84 );
	m_fieldsGrid->SetColSize( 1, 120 );
	m_fieldsGrid->EnableDragColMove( false );
	m_fieldsGrid->EnableDragColSize( true );
	m_fieldsGrid->SetColLabelValue( 0, _("Name") );
	m_fieldsGrid->SetColLabelValue( 1, _("Value") );
	m_fieldsGrid->SetColLabelSize( 22 );
	m_fieldsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_fieldsGrid->EnableDragRowSize( true );
	m_fieldsGrid->SetRowLabelSize( 0 );
	m_fieldsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_fieldsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_fieldsGrid->SetMinSize( wxSize( -1,180 ) );

	sbFields->Add( m_fieldsGrid, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSizer->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSizer, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bMargins->Add( sbFields, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bMargins, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgProperties;
	fgProperties = new wxFlexGridSizer( 0, 2, 5, 0 );
	fgProperties->AddGrowableCol( 1 );
	fgProperties->AddGrowableRow( 2 );
	fgProperties->SetFlexibleDirection( wxBOTH );
	fgProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( this, wxID_ANY, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	fgProperties->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgProperties->Add( m_textName, 0, wxEXPAND, 5 );

	m_staticTextKeywords = new wxStaticText( this, wxID_ANY, _("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKeywords->Wrap( -1 );
	fgProperties->Add( m_staticTextKeywords, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textKeywords = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgProperties->Add( m_textKeywords, 0, wxEXPAND, 5 );

	m_staticTextDescription = new wxStaticText( this, wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDescription->Wrap( -1 );
	fgProperties->Add( m_staticTextDescription, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textDescription = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	fgProperties->Add( m_textDescription, 0, wxEXPAND, 5 );


	bMainSizer->Add( fgProperties, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
}

DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::~DIALOG_DESIGN_BLOCK_PROPERTIES_BASE()
{
	// Disconnect Events
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );

}
