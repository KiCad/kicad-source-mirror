///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sync_sheet_pins_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYNC_SHEET_PINS_BASE::PANEL_SYNC_SHEET_PINS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_panel11 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );

	m_labelSymName = new wxStaticText( m_panel11, wxID_ANY, _("Symbol name"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL|wxBORDER_THEME );
	m_labelSymName->Wrap( -1 );
	bSizer61->Add( m_labelSymName, 0, wxALL|wxEXPAND, 0 );

	m_viewSheetPins = new wxDataViewCtrl( m_panel11, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	bSizer61->Add( m_viewSheetPins, 1, wxALL|wxEXPAND, 0 );

	wxBoxSizer* bSizer51;
	bSizer51 = new wxBoxSizer( wxVERTICAL );

	m_btnAddLabels = new wxButton( m_panel11, wxID_ANY, _("Add Hierarchical Labels"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer51->Add( m_btnAddLabels, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_btnRmPins = new wxButton( m_panel11, wxID_ANY, _("Delete Sheet Pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer51->Add( m_btnRmPins, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizer61->Add( bSizer51, 0, wxEXPAND, 5 );


	bSizer41->Add( bSizer61, 1, wxEXPAND, 5 );


	m_panel11->SetSizer( bSizer41 );
	m_panel11->Layout();
	bSizer41->Fit( m_panel11 );
	bSizer3->Add( m_panel11, 1, wxEXPAND | wxALL, 5 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_labelSheetName = new wxStaticText( m_panel1, wxID_ANY, _("Sheet name"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL|wxBORDER_THEME );
	m_labelSheetName->Wrap( -1 );
	bSizer6->Add( m_labelSheetName, 0, wxALL|wxEXPAND, 0 );

	m_viewSheetLabels = new wxDataViewCtrl( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	bSizer6->Add( m_viewSheetLabels, 1, wxALL|wxEXPAND, 0 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_btnAddSheetPins = new wxButton( m_panel1, wxID_ANY, _("Add Sheet Pins"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btnAddSheetPins, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_btnRmLabels = new wxButton( m_panel1, wxID_ANY, _("Delete Hierarchical Labels"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_btnRmLabels, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizer6->Add( bSizer5, 0, wxEXPAND, 5 );


	bSizer4->Add( bSizer6, 1, wxEXPAND, 5 );


	m_panel1->SetSizer( bSizer4 );
	m_panel1->Layout();
	bSizer4->Fit( m_panel1 );
	bSizer3->Add( m_panel1, 1, wxEXPAND | wxALL, 5 );

	m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );


	bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );

	m_panel8 = new wxPanel( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	m_btnUseLabelAsTemplate = new wxBitmapButton( m_panel8, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnUseLabelAsTemplate->SetToolTip( _("Associate selected sheet pin and hierarchical label using the label name") );

	bSizer14->Add( m_btnUseLabelAsTemplate, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_btnUsePinAsTemplate = new wxBitmapButton( m_panel8, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnUsePinAsTemplate->SetToolTip( _("Associate selected sheet pin and hierarchical label using the pin name") );

	bSizer14->Add( m_btnUsePinAsTemplate, 0, wxALL|wxEXPAND, 5 );

	m_btnUndo = new wxBitmapButton( m_panel8, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnUndo->SetToolTip( _("Break sheet pin and hierarchical label association(s)") );

	bSizer14->Add( m_btnUndo, 0, wxALL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	m_panel8->SetSizer( bSizer14 );
	m_panel8->Layout();
	bSizer14->Fit( m_panel8 );
	bSizer13->Add( m_panel8, 0, wxEXPAND | wxALL, 0 );


	bSizer13->Add( 0, 0, 2, wxEXPAND, 5 );


	m_panel3->SetSizer( bSizer13 );
	m_panel3->Layout();
	bSizer13->Fit( m_panel3 );
	bSizer3->Add( m_panel3, 0, wxEXPAND | wxALL, 0 );

	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_viewAssociated = new wxDataViewCtrl( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_MULTIPLE|wxDV_ROW_LINES|wxDV_VERT_RULES );
	bSizer16->Add( m_viewAssociated, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	bSizer15->Add( bSizer16, 1, wxEXPAND, 5 );


	m_panel4->SetSizer( bSizer15 );
	m_panel4->Layout();
	bSizer15->Fit( m_panel4 );
	bSizer3->Add( m_panel4, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizer3 );
	this->Layout();

	// Connect Events
	m_viewSheetPins->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewSheetPinCellClicked ), NULL, this );
	m_btnAddLabels->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnAddLabelsClicked ), NULL, this );
	m_btnRmPins->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnRmPinsClicked ), NULL, this );
	m_viewSheetLabels->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewSheetLabelCellClicked ), NULL, this );
	m_btnAddSheetPins->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnAddSheetPinsClicked ), NULL, this );
	m_btnRmLabels->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnRmLabelsClicked ), NULL, this );
	m_btnUseLabelAsTemplate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUseLabelAsTemplateClicked ), NULL, this );
	m_btnUsePinAsTemplate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUsePinAsTemplateClicked ), NULL, this );
	m_btnUndo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUndoClicked ), NULL, this );
	m_viewAssociated->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewMatchedCellClicked ), NULL, this );
}

PANEL_SYNC_SHEET_PINS_BASE::~PANEL_SYNC_SHEET_PINS_BASE()
{
	// Disconnect Events
	m_viewSheetPins->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewSheetPinCellClicked ), NULL, this );
	m_btnAddLabels->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnAddLabelsClicked ), NULL, this );
	m_btnRmPins->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnRmPinsClicked ), NULL, this );
	m_viewSheetLabels->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewSheetLabelCellClicked ), NULL, this );
	m_btnAddSheetPins->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnAddSheetPinsClicked ), NULL, this );
	m_btnRmLabels->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnRmLabelsClicked ), NULL, this );
	m_btnUseLabelAsTemplate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUseLabelAsTemplateClicked ), NULL, this );
	m_btnUsePinAsTemplate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUsePinAsTemplateClicked ), NULL, this );
	m_btnUndo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnBtnUndoClicked ), NULL, this );
	m_viewAssociated->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( PANEL_SYNC_SHEET_PINS_BASE::OnViewMatchedCellClicked ), NULL, this );

}
