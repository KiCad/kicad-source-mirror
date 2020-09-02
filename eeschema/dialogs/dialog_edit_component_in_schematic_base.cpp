///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_edit_component_in_schematic_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFields;
	sbFields = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fields") ), wxVERTICAL );

	m_grid = new WX_GRID( sbFields->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 4, 11 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 120 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 72 );
	m_grid->SetColSize( 4, 72 );
	m_grid->SetColSize( 5, 48 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
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
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( -1,180 ) );

	sbFields->Add( m_grid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_bpAdd = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpAdd->SetToolTip( _("Add field") );

	bButtonSize->Add( m_bpAdd, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveUp = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveUp->SetToolTip( _("Move up") );

	bButtonSize->Add( m_bpMoveUp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_bpMoveDown = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpMoveDown->SetToolTip( _("Move down") );

	bButtonSize->Add( m_bpMoveDown, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 10 );

	m_bpDelete = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );


	sbFields->Add( bButtonSize, 0, wxALL|wxEXPAND, 5 );


	mainSizer->Add( sbFields, 1, wxALL|wxEXPAND, 10 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbGeneralProps;
	sbGeneralProps = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	m_unitLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabel->Wrap( -1 );
	gbSizer1->Add( m_unitLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT, 5 );

	wxArrayString m_unitChoiceChoices;
	m_unitChoice = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitChoiceChoices, 0 );
	m_unitChoice->SetSelection( 0 );
	m_unitChoice->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_unitChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_cbAlternateSymbol = new wxCheckBox( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Alternate symbol (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAlternateSymbol->SetToolTip( _("Use the alternate shape of this symbol.\nFor gates, this is the \"De Morgan\" conversion") );

	gbSizer1->Add( m_cbAlternateSymbol, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_orientationLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationLabel->Wrap( -1 );
	gbSizer1->Add( m_orientationLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxString m_orientationCtrlChoices[] = { _("0"), _("+90"), _("-90"), _("180") };
	int m_orientationCtrlNChoices = sizeof( m_orientationCtrlChoices ) / sizeof( wxString );
	m_orientationCtrl = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_orientationCtrlNChoices, m_orientationCtrlChoices, 0 );
	m_orientationCtrl->SetSelection( 0 );
	gbSizer1->Add( m_orientationCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_mirrorLabel = new wxStaticText( sbGeneralProps->GetStaticBox(), wxID_ANY, _("Mirror:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mirrorLabel->Wrap( -1 );
	gbSizer1->Add( m_mirrorLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT, 5 );

	wxString m_mirrorCtrlChoices[] = { _("Not mirrored"), _("Around X axis"), _("Around Y axis") };
	int m_mirrorCtrlNChoices = sizeof( m_mirrorCtrlChoices ) / sizeof( wxString );
	m_mirrorCtrl = new wxChoice( sbGeneralProps->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_mirrorCtrlNChoices, m_mirrorCtrlChoices, 0 );
	m_mirrorCtrl->SetSelection( 0 );
	gbSizer1->Add( m_mirrorCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbGeneralProps->Add( gbSizer1, 1, wxEXPAND, 5 );


	bLowerSizer->Add( sbGeneralProps, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bMiddleCol;
	bMiddleCol = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerPinTextOpts;
	sbSizerPinTextOpts = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pin Text") ), wxVERTICAL );

	m_ShowPinNumButt = new wxCheckBox( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Show pin numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNumButt->SetValue(true);
	m_ShowPinNumButt->SetToolTip( _("Show or hide pin numbers") );

	sbSizerPinTextOpts->Add( m_ShowPinNumButt, 0, wxRIGHT|wxLEFT, 4 );

	m_ShowPinNameButt = new wxCheckBox( sbSizerPinTextOpts->GetStaticBox(), wxID_ANY, _("Show pin names"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShowPinNameButt->SetValue(true);
	m_ShowPinNameButt->SetToolTip( _("Show or hide pin names") );

	sbSizerPinTextOpts->Add( m_ShowPinNameButt, 0, wxALL, 4 );


	bMiddleCol->Add( sbSizerPinTextOpts, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbAttributes;
	sbAttributes = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Attributes") ), wxVERTICAL );

	m_cbExcludeFromBom = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBom->SetToolTip( _("This is useful for adding symbols for board footprints such as fiducials\nand logos that you do not want to appear in the bill of materials export") );

	sbAttributes->Add( m_cbExcludeFromBom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );

	m_cbExcludeFromBoard = new wxCheckBox( sbAttributes->GetStaticBox(), wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBoard->SetToolTip( _("This is useful for adding symbols that only get exported to the bill of materials but\nnot required to layout the board such as mechanical fasteners and enclosures") );

	sbAttributes->Add( m_cbExcludeFromBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 4 );


	bMiddleCol->Add( sbAttributes, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bLowerSizer->Add( bMiddleCol, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* buttonsSizer;
	buttonsSizer = new wxBoxSizer( wxVERTICAL );

	m_updateSymbolBtn = new wxButton( this, wxID_ANY, _("Update Symbol from Library..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_updateSymbolBtn, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_changeSymbolBtn = new wxButton( this, wxID_ANY, _("Change Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_changeSymbolBtn, 0, wxEXPAND|wxALL, 5 );

	m_editSchematicSymbolBtn = new wxButton( this, wxID_ANY, _("Edit Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_editSchematicSymbolBtn, 0, wxEXPAND|wxALL, 5 );

	m_pinTableButton = new wxButton( this, wxID_ANY, _("Alternate Pin Assignments..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinTableButton->SetMinSize( wxSize( 112,-1 ) );

	buttonsSizer->Add( m_pinTableButton, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );


	buttonsSizer->Add( 0, 0, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_editLibrarySymbolBtn = new wxButton( this, wxID_ANY, _("Edit Library Symbol..."), wxDefaultPosition, wxDefaultSize, 0 );
	buttonsSizer->Add( m_editLibrarySymbolBtn, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bLowerSizer->Add( buttonsSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	mainSizer->Add( bLowerSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLibLink;
	bLibLink = new wxBoxSizer( wxHORIZONTAL );

	m_libraryIDLabel = new wxStaticText( this, wxID_ANY, _("Library link:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryIDLabel->Wrap( -1 );
	bLibLink->Add( m_libraryIDLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_tcLibraryID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_NONE );
	m_tcLibraryID->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_tcLibraryID->SetToolTip( _("The library ID and footprint ID currently assigned.  Use “Change Footprint…” to assign a different footprint.") );

	bLibLink->Add( m_tcLibraryID, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizerBottom->Add( bLibLink, 1, wxEXPAND|wxLEFT, 10 );


	bSizerBottom->Add( 20, 0, 0, wxEXPAND, 5 );

	m_spiceFieldsButton = new wxButton( this, wxID_ANY, _("Spice Model..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_spiceFieldsButton->SetMinSize( wxSize( 112,-1 ) );

	bSizerBottom->Add( m_spiceFieldsButton, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

	m_stdDialogButtonSizer = new wxStdDialogButtonSizer();
	m_stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerOK );
	m_stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	m_stdDialogButtonSizer->AddButton( m_stdDialogButtonSizerCancel );
	m_stdDialogButtonSizer->Realize();

	bSizerBottom->Add( m_stdDialogButtonSizer, 0, wxEXPAND|wxALL, 5 );


	mainSizer->Add( bSizerBottom, 0, wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnDeleteField ), NULL, this );
	m_updateSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnUpdateSymbol ), NULL, this );
	m_changeSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnExchangeSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditSymbol ), NULL, this );
	m_pinTableButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditPinTable ), NULL, this );
	m_editLibrarySymbolBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditLibrarySymbol ), NULL, this );
	m_spiceFieldsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditSpiceModel ), NULL, this );
	m_stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnCancelButtonClick ), NULL, this );
}

DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnAddField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnMoveDown ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnDeleteField ), NULL, this );
	m_updateSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnUpdateSymbol ), NULL, this );
	m_changeSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnExchangeSymbol ), NULL, this );
	m_editSchematicSymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditSymbol ), NULL, this );
	m_pinTableButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditPinTable ), NULL, this );
	m_editLibrarySymbolBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditLibrarySymbol ), NULL, this );
	m_spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditSpiceModel ), NULL, this );
	m_stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnCancelButtonClick ), NULL, this );

}
