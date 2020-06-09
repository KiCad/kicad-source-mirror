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


	bButtonSize->Add( 0, 0, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_bpDelete = new wxBitmapButton( sbFields->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_bpDelete->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_bpDelete, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );

	m_updateFieldValues = new wxButton( sbFields->GetStaticBox(), wxID_ANY, _("Update Fields from Library..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_updateFieldValues->SetToolTip( _("Sets fields to the original library values") );

	bButtonSize->Add( m_updateFieldValues, 0, wxEXPAND, 5 );


	sbFields->Add( bButtonSize, 0, wxALL|wxEXPAND, 5 );


	mainSizer->Add( sbFields, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* lowerSizer;
	lowerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizerLibraryReference;
	sbSizerLibraryReference = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbol") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 5, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText3 = new wxStaticText( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, _("Library Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	fgSizer1->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bLibraryReferenceSizer;
	bLibraryReferenceSizer = new wxBoxSizer( wxHORIZONTAL );

	m_libraryNameTextCtrl = new wxTextCtrl( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_libraryNameTextCtrl->SetToolTip( _("Name of the symbol in the library to which this symbol is linked") );

	bLibraryReferenceSizer->Add( m_libraryNameTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_buttonBrowseLibrary = new wxBitmapButton( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonBrowseLibrary->SetToolTip( _("Browse library") );

	bLibraryReferenceSizer->Add( m_buttonBrowseLibrary, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizer1->Add( bLibraryReferenceSizer, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_unitLabel = new wxStaticText( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, _("Unit:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabel->Wrap( -1 );
	fgSizer1->Add( m_unitLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_unitChoiceChoices;
	m_unitChoice = new wxChoice( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitChoiceChoices, 0 );
	m_unitChoice->SetSelection( 0 );
	m_unitChoice->SetMinSize( wxSize( 100,-1 ) );

	fgSizer1->Add( m_unitChoice, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );


	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cbAlternateSymbol = new wxCheckBox( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, _("Alternate symbol (DeMorgan)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbAlternateSymbol->SetToolTip( _("Use the alternate shape of this symbol.\nFor gates, this is the \"De Morgan\" conversion") );

	fgSizer1->Add( m_cbAlternateSymbol, 0, wxRIGHT|wxTOP, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbExcludeFromBom = new wxCheckBox( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, _("Exclude from bill of materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBom->SetToolTip( _("This is useful for adding symbols for board footprints such as fiducials\nand logos that you do not want to appear in the bill of materials export") );

	fgSizer1->Add( m_cbExcludeFromBom, 0, 0, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbExcludeFromBoard = new wxCheckBox( sbSizerLibraryReference->GetStaticBox(), wxID_ANY, _("Exclude from board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbExcludeFromBoard->SetToolTip( _("This is useful for adding symbols that only get exported to the bill of materials but\nnot required to layout the board such as mechanical fasteners and enclosures") );

	fgSizer1->Add( m_cbExcludeFromBoard, 0, wxBOTTOM, 5 );


	sbSizerLibraryReference->Add( fgSizer1, 0, wxEXPAND, 5 );


	lowerSizer->Add( sbSizerLibraryReference, 5, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxString m_rbOrientationChoices[] = { _("0"), _("+90"), _("+180"), _("-90") };
	int m_rbOrientationNChoices = sizeof( m_rbOrientationChoices ) / sizeof( wxString );
	m_rbOrientation = new wxRadioBox( this, wxID_ANY, _("Orientation"), wxDefaultPosition, wxDefaultSize, m_rbOrientationNChoices, m_rbOrientationChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOrientation->SetSelection( 0 );
	m_rbOrientation->SetToolTip( _("Select if the symbol is to be rotated when drawn") );

	lowerSizer->Add( m_rbOrientation, 2, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxString m_rbMirrorChoices[] = { _("Default"), _("Mirror around X axis"), _("Mirror around Y axis") };
	int m_rbMirrorNChoices = sizeof( m_rbMirrorChoices ) / sizeof( wxString );
	m_rbMirror = new wxRadioBox( this, wxID_ANY, _("Aspect"), wxDefaultPosition, wxDefaultSize, m_rbMirrorNChoices, m_rbMirrorChoices, 1, wxRA_SPECIFY_COLS );
	m_rbMirror->SetSelection( 1 );
	m_rbMirror->SetToolTip( _("Pick the graphical transformation to be used when displaying the symbol") );

	lowerSizer->Add( m_rbMirror, 2, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	mainSizer->Add( lowerSizer, 0, wxEXPAND, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerUUID;
	bSizerUUID = new wxBoxSizer( wxHORIZONTAL );

	m_timeStampLabel = new wxStaticText( this, wxID_ANY, _("Unique ID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeStampLabel->Wrap( -1 );
	bSizerUUID->Add( m_timeStampLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_textCtrlTimeStamp = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_NONE );
	m_textCtrlTimeStamp->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	m_textCtrlTimeStamp->SetToolTip( _("Unique ID that identifies the symbol") );

	bSizerUUID->Add( m_textCtrlTimeStamp, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bSizerBottom->Add( bSizerUUID, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerBottom->Add( 20, 0, 0, wxEXPAND, 5 );

	m_spiceFieldsButton = new wxButton( this, wxID_ANY, _("Edit Spice Model..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_spiceFieldsButton, 0, wxEXPAND|wxALL, 5 );


	bSizerBottom->Add( 10, 0, 0, wxEXPAND, 5 );

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
	m_updateFieldValues->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::UpdateFieldsFromLibrary ), NULL, this );
	m_buttonBrowseLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnBrowseLibrary ), NULL, this );
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
	m_updateFieldValues->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::UpdateFieldsFromLibrary ), NULL, this );
	m_buttonBrowseLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnBrowseLibrary ), NULL, this );
	m_spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnEditSpiceModel ), NULL, this );
	m_stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE::OnCancelButtonClick ), NULL, this );

}
