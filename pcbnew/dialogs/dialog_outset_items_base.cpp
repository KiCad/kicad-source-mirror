///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_outset_items_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_OUTSET_ITEMS_BASE::DIALOG_OUTSET_ITEMS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_outsetLabel = new wxStaticText( this, wxID_ANY, _("Outset:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_outsetLabel->Wrap( -1 );
	gbSizer1->Add( m_outsetLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_outsetEntry = new wxComboBox( this, wxID_ANY, _("0.1"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_outsetEntry, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );

	m_outsetUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outsetUnit->Wrap( -1 );
	gbSizer1->Add( m_outsetUnit, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_roundToGrid = new wxCheckBox( this, wxID_ANY, _("Round outwards to grid multiples (when possible)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_roundToGrid->SetValue(true);
	m_roundToGrid->SetToolTip( _("This is only possible for rectangular outsets.") );

	gbSizer1->Add( m_roundToGrid, wxGBPosition( 2, 0 ), wxGBSpan( 1, 4 ), wxALL|wxEXPAND, 5 );

	m_roundCorners = new wxCheckBox( this, wxID_ANY, _("Round corners (when possible)"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_roundCorners, wxGBPosition( 1, 0 ), wxGBSpan( 1, 4 ), wxALL|wxEXPAND, 5 );

	m_gridRoundingLabel = new wxStaticText( this, wxID_ANY, _("Grid size:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_gridRoundingLabel->Wrap( -1 );
	gbSizer1->Add( m_gridRoundingLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_gridRoundingEntry = new wxComboBox( this, wxID_ANY, _("0.01"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_gridRoundingEntry, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );

	m_gridRoundingUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridRoundingUnit->Wrap( -1 );
	gbSizer1->Add( m_gridRoundingUnit, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer1->Add( m_staticline2, wxGBPosition( 4, 0 ), wxGBSpan( 1, 4 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_copyLayers = new wxCheckBox( this, wxID_ANY, _("Copy item layers"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_copyLayers, wxGBPosition( 5, 0 ), wxGBSpan( 1, 4 ), wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_layerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_layerLabel->Wrap( -1 );
	gbSizer1->Add( m_layerLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_LayerSelectionCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 3 ), wxALL|wxEXPAND, 5 );

	m_copyWidths = new wxCheckBox( this, wxID_ANY, _("Copy item widths (if possible)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_copyWidths->SetToolTip( _("This is not possible for items like pads, which will still use the value below.") );

	gbSizer1->Add( m_copyWidths, wxGBPosition( 7, 0 ), wxGBSpan( 1, 4 ), wxALL, 5 );

	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("Line width:"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_lineWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 8, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_lineWidthEntry = new wxComboBox( this, wxID_ANY, _("0.1"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_lineWidthEntry, wxGBPosition( 8, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_lineWidthUnit = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnit->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnit, wxGBPosition( 8, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	m_layerDefaultBtn = new wxButton( this, wxID_ANY, _("Layer Default"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_layerDefaultBtn, wxGBPosition( 8, 3 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline21 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer1->Add( m_staticline21, wxGBPosition( 9, 0 ), wxGBSpan( 1, 4 ), wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_deleteSourceItems = new wxCheckBox( this, wxID_ANY, _("Delete source items after outset"), wxDefaultPosition, wxDefaultSize, 0 );
	m_deleteSourceItems->SetToolTip( _("This is not possible for items like pads, which will still use the value below.") );

	gbSizer1->Add( m_deleteSourceItems, wxGBPosition( 10, 0 ), wxGBSpan( 1, 4 ), wxALL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bMainSizer->Add( gbSizer1, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );


	bSizerBottom->Add( 40, 0, 1, wxEXPAND, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bSizerBottom->Add( m_stdButtons, 0, wxBOTTOM|wxTOP, 5 );


	bMainSizer->Add( bSizerBottom, 0, wxEXPAND|wxTOP, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnClose ) );
	m_roundToGrid->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnRoundToGridChecked ), NULL, this );
	m_copyLayers->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnCopyLayersChecked ), NULL, this );
	m_layerDefaultBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnLayerDefaultClick ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnOkClick ), NULL, this );
}

DIALOG_OUTSET_ITEMS_BASE::~DIALOG_OUTSET_ITEMS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnClose ) );
	m_roundToGrid->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnRoundToGridChecked ), NULL, this );
	m_copyLayers->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnCopyLayersChecked ), NULL, this );
	m_layerDefaultBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnLayerDefaultClick ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_OUTSET_ITEMS_BASE::OnOkClick ), NULL, this );

}
