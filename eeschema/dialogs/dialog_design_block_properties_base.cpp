///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_design_block_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::DIALOG_DESIGN_BLOCK_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerGeneral;
	sbSizerGeneral = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("General") ), wxVERTICAL );

	wxFlexGridSizer* fgProperties;
	fgProperties = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgProperties->AddGrowableCol( 1 );
	fgProperties->AddGrowableRow( 2 );
	fgProperties->SetFlexibleDirection( wxBOTH );
	fgProperties->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextName = new wxStaticText( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxT("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextName->Wrap( -1 );
	fgProperties->Add( m_staticTextName, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_textName = new wxTextCtrl( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgProperties->Add( m_textName, 0, wxALL|wxEXPAND, 5 );

	m_staticTextKeywords = new wxStaticText( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxT("Keywords:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextKeywords->Wrap( -1 );
	fgProperties->Add( m_staticTextKeywords, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_textKeywords = new wxTextCtrl( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgProperties->Add( m_textKeywords, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticTextDescription = new wxStaticText( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxT("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDescription->Wrap( -1 );
	bSizer3->Add( m_staticTextDescription, 1, wxALIGN_RIGHT|wxALL, 5 );


	bSizer3->Add( 0, 0, 3, wxEXPAND, 5 );


	fgProperties->Add( bSizer3, 1, wxEXPAND, 5 );

	m_textDescription = new wxTextCtrl( sbSizerGeneral->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	fgProperties->Add( m_textDescription, 0, wxALL|wxEXPAND, 5 );


	sbSizerGeneral->Add( fgProperties, 1, wxEXPAND, 5 );


	bMainSizer->Add( sbSizerGeneral, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Default Fields") ), wxVERTICAL );

	m_fieldsGrid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_fieldsGrid->CreateGrid( 1, 2 );
	m_fieldsGrid->EnableEditing( true );
	m_fieldsGrid->EnableGridLines( true );
	m_fieldsGrid->EnableDragGridSize( false );
	m_fieldsGrid->SetMargins( 0, 0 );

	// Columns
	m_fieldsGrid->SetColSize( 0, 150 );
	m_fieldsGrid->SetColSize( 1, 300 );
	m_fieldsGrid->EnableDragColMove( false );
	m_fieldsGrid->EnableDragColSize( true );
	m_fieldsGrid->SetColLabelValue( 0, wxT("Name") );
	m_fieldsGrid->SetColLabelValue( 1, wxT("Value") );
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

	sbFields->Add( m_fieldsGrid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSizer->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bButtonSizer->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( sbFields, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_fieldsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_stdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );
}

DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::~DIALOG_DESIGN_BLOCK_PROPERTIES_BASE()
{
	// Disconnect Events
	m_fieldsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnMoveFieldDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_stdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DESIGN_BLOCK_PROPERTIES_BASE::OnCancelButtonClick ), NULL, this );

}
