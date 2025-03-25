///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"

#include "panel_fp_editor_field_defaults_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	defaultFieldPropertiesLabel = new wxStaticText( this, wxID_ANY, _("Default Field Properties for New Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	defaultFieldPropertiesLabel->Wrap( -1 );
	bSizerMargins->Add( defaultFieldPropertiesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );


	bSizerMargins->Add( 0, 4, 0, wxEXPAND, 5 );

	wxBoxSizer* defaultFieldPropertiesSizer;
	defaultFieldPropertiesSizer = new wxBoxSizer( wxVERTICAL );

	m_fieldPropsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );

	// Grid
	m_fieldPropsGrid->CreateGrid( 2, 3 );
	m_fieldPropsGrid->EnableEditing( true );
	m_fieldPropsGrid->EnableGridLines( true );
	m_fieldPropsGrid->EnableDragGridSize( false );
	m_fieldPropsGrid->SetMargins( 0, 0 );

	// Columns
	m_fieldPropsGrid->SetColSize( 0, 240 );
	m_fieldPropsGrid->SetColSize( 1, 60 );
	m_fieldPropsGrid->SetColSize( 2, 150 );
	m_fieldPropsGrid->EnableDragColMove( false );
	m_fieldPropsGrid->EnableDragColSize( true );
	m_fieldPropsGrid->SetColLabelValue( 0, _("Value") );
	m_fieldPropsGrid->SetColLabelValue( 1, _("Show") );
	m_fieldPropsGrid->SetColLabelValue( 2, _("Layer") );
	m_fieldPropsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_fieldPropsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_fieldPropsGrid->EnableDragRowSize( false );
	m_fieldPropsGrid->SetRowLabelValue( 0, _("Reference designator") );
	m_fieldPropsGrid->SetRowLabelValue( 1, _("Value") );
	m_fieldPropsGrid->SetRowLabelSize( 160 );
	m_fieldPropsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_fieldPropsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	defaultFieldPropertiesSizer->Add( m_fieldPropsGrid, 0, wxEXPAND, 5 );


	bSizerMargins->Add( defaultFieldPropertiesSizer, 0, wxEXPAND, 5 );


	bSizerMargins->Add( 5, 25, 0, wxEXPAND, 5 );

	defaultTextItemsLabel = new wxStaticText( this, wxID_ANY, _("Default Text Items for New Footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	defaultTextItemsLabel->Wrap( -1 );
	bSizerMargins->Add( defaultTextItemsLabel, 0, wxTOP|wxLEFT|wxEXPAND, 8 );


	bSizerMargins->Add( 0, 4, 0, wxEXPAND, 5 );

	wxBoxSizer* defaultTextItemsSizer;
	defaultTextItemsSizer = new wxBoxSizer( wxVERTICAL );

	m_textItemsGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );

	// Grid
	m_textItemsGrid->CreateGrid( 0, 2 );
	m_textItemsGrid->EnableEditing( true );
	m_textItemsGrid->EnableGridLines( true );
	m_textItemsGrid->EnableDragGridSize( false );
	m_textItemsGrid->SetMargins( 0, 0 );

	// Columns
	m_textItemsGrid->SetColSize( 0, 460 );
	m_textItemsGrid->SetColSize( 1, 150 );
	m_textItemsGrid->EnableDragColMove( false );
	m_textItemsGrid->EnableDragColSize( true );
	m_textItemsGrid->SetColLabelValue( 0, _("Text Items") );
	m_textItemsGrid->SetColLabelValue( 1, _("Layer") );
	m_textItemsGrid->SetColLabelSize( wxGRID_AUTOSIZE );
	m_textItemsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_textItemsGrid->EnableDragRowSize( false );
	m_textItemsGrid->SetRowLabelSize( 0 );
	m_textItemsGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_textItemsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_textItemsGrid->SetMinSize( wxSize( -1,140 ) );

	defaultTextItemsSizer->Add( m_textItemsGrid, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetMinSize( wxSize( 30,29 ) );

	bButtonSize->Add( m_bpAdd, 0, wxBOTTOM|wxLEFT|wxTOP, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_bpDelete = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetMinSize( wxSize( 30,29 ) );

	bButtonSize->Add( m_bpDelete, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	defaultTextItemsSizer->Add( bButtonSize, 0, wxEXPAND, 5 );


	bSizerMargins->Add( defaultTextItemsSizer, 1, wxEXPAND, 20 );


	bSizerMargins->Add( 5, 25, 0, wxEXPAND, 5 );


	bSizerMain->Add( bSizerMargins, 1, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_fieldPropsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnGridSize ), NULL, this );
	m_textItemsGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnAddTextItem ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnDeleteTextItem ), NULL, this );
}

PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::~PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE()
{
	// Disconnect Events
	m_fieldPropsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnGridSize ), NULL, this );
	m_textItemsGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnGridSize ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnAddTextItem ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE::OnDeleteTextItem ), NULL, this );

}
