///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "dialog_sheet_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SHEET_PROPERTIES_BASE::DIALOG_SHEET_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	mainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	m_longForm = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_grid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 4, 13 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 72 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 72 );
	m_grid->SetColSize( 4, 72 );
	m_grid->SetColSize( 5, 48 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->SetColSize( 11, 60 );
	m_grid->SetColSize( 12, 48 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelValue( 2, _("Show") );
	m_grid->SetColLabelValue( 3, _("H Align") );
	m_grid->SetColLabelValue( 4, _("V Align") );
	m_grid->SetColLabelValue( 5, _("Italic") );
	m_grid->SetColLabelValue( 6, _("Bold") );
	m_grid->SetColLabelValue( 7, _("Text Size") );
	m_grid->SetColLabelValue( 8, _("Orientation") );
	m_grid->SetColLabelValue( 9, _("X Position") );
	m_grid->SetColLabelValue( 10, _("Y Position") );
	m_grid->SetColLabelValue( 11, _("Font") );
	m_grid->SetColLabelValue( 12, _("Color") );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( -1,120 ) );

	sbFields->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );

	bButtonSize->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonSize->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonSize->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new STD_BITMAP_BUTTON( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSize, 0, wxALL|wxEXPAND, 5 );


	m_longForm->Add( sbFields, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbAttributes;
	sbAttributes = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Attributes") ), wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_pageNumberStaticText = new wxStaticText( sbAttributes->GetStaticBox(), wxID_ANY, _("Page number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pageNumberStaticText->Wrap( -1 );
	bSizer6->Add( m_pageNumberStaticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_pageNumberTextCtrl = new wxTextCtrl( sbAttributes->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_pageNumberTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer6->Add( 0, 0, 3, wxEXPAND, 5 );


	sbAttributes->Add( bSizer6, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 10 );

	m_cbExcludeFromSim = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributes->Add( m_cbExcludeFromSim, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	sbAttributes->Add( 0, 10, 0, wxEXPAND, 5 );

	m_cbExcludeFromBom = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBom->SetToolTip( _("This is useful for adding symbols for board footprints such as fiducials\nand logos that you do not want to appear in the bill of materials export") );

	sbAttributes->Add( m_cbExcludeFromBom, 0, wxALL, 5 );

	m_cbExcludeFromBoard = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBoard->SetToolTip( _("This is useful for adding symbols that only get exported to the bill of materials but\nnot required to layout the board such as mechanical fasteners and enclosures") );

	sbAttributes->Add( m_cbExcludeFromBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbDNP = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Do not populate"), wxDefaultPosition, wxDefaultSize, 0 );
	sbAttributes->Add( m_cbDNP, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bSizer5->Add( sbAttributes, 0, wxEXPAND|wxRIGHT, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Style") ), wxVERTICAL );

	wxBoxSizer* bStyleColumns;
	bStyleColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bBorderSizer;
	bBorderSizer = new wxBoxSizer( wxVERTICAL );

	m_borderLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Border"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderLabel->Wrap( -1 );
	bBorderSizer->Add( m_borderLabel, 0, wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_borderWidthLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthLabel->Wrap( -1 );
	bSizer10->Add( m_borderWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_borderWidthCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_borderWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_borderWidthUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderWidthUnits->Wrap( -1 );
	bSizer10->Add( m_borderWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	bSizer10->Add( 20, 0, 0, wxEXPAND, 5 );

	m_borderColorLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_borderColorLabel->Wrap( -1 );
	bSizer10->Add( m_borderColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_borderSwatch = new COLOR_SWATCH( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_borderSwatch->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_borderSwatch->SetMinSize( wxSize( 48,24 ) );

	bSizer10->Add( m_borderSwatch, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bBorderSizer->Add( bSizer10, 1, wxEXPAND, 5 );


	bStyleColumns->Add( bBorderSizer, 2, wxEXPAND, 5 );

	wxBoxSizer* bFillSizer;
	bFillSizer = new wxBoxSizer( wxVERTICAL );

	m_fillLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Fill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fillLabel->Wrap( -1 );
	bFillSizer->Add( m_fillLabel, 0, wxBOTTOM, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_backgroundColorLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Color:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundColorLabel->Wrap( -1 );
	bSizer11->Add( m_backgroundColorLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_backgroundSwatch = new COLOR_SWATCH( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_backgroundSwatch->SetMinSize( wxSize( 48,24 ) );

	bSizer11->Add( m_backgroundSwatch, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bFillSizer->Add( bSizer11, 1, wxEXPAND, 5 );


	bStyleColumns->Add( bFillSizer, 1, wxEXPAND, 5 );


	sbSizer2->Add( bStyleColumns, 0, wxEXPAND, 5 );


	bSizer5->Add( sbSizer2, 1, wxEXPAND|wxLEFT, 10 );


	m_longForm->Add( bSizer5, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( m_longForm, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_sizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_hierarchicalPathLabel = new wxStaticText( this, wxID_ANY, _("Hierarchical path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hierarchicalPathLabel->Wrap( -1 );
	m_sizerBottom->Add( m_hierarchicalPathLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 10 );

	m_hierarchicalPath = new wxStaticText( this, wxID_ANY, _("path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hierarchicalPath->Wrap( -1 );
	m_sizerBottom->Add( m_hierarchicalPath, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_stdDialogButtonSizer = new wxStdDialogButtonSizer();
	m_stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerOK );
	m_stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerCancel );
	m_stdDialogButtonSizer->Realize();

	m_sizerBottom->Add( m_stdDialogButtonSizer, 0, wxEXPAND|wxALL, 5 );


	mainSizer->Add( m_sizerBottom, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnUpdateUI ) );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_cbExcludeFromSim->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBom->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBoard->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );
}

DIALOG_SHEET_PROPERTIES_BASE::~DIALOG_SHEET_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnUpdateUI ) );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnDeleteField ), NULL, this );
	m_cbExcludeFromSim->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBom->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );
	m_cbExcludeFromBoard->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_SHEET_PROPERTIES_BASE::OnCheckBox ), NULL, this );

}
