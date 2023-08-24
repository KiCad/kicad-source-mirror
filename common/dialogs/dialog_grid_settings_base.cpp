///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_grid_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRID_SETTINGS_BASE::DIALOG_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerColumns;
	bSizerColumns = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbCurrentGrid;
	sbCurrentGrid = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Current Grid") ), wxVERTICAL );

	m_currentGridCtrl = new wxListBox( sbCurrentGrid->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	sbCurrentGrid->Add( m_currentGridCtrl, 1, wxEXPAND|wxBOTTOM, 3 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_addGridButton = new STD_BITMAP_BUTTON( sbCurrentGrid->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_addGridButton, 0, 0, 5 );


	bSizer4->Add( 20, 0, 0, wxEXPAND, 5 );

	m_removeGridButton = new STD_BITMAP_BUTTON( sbCurrentGrid->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_removeGridButton, 0, 0, 5 );


	sbCurrentGrid->Add( bSizer4, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerColumns->Add( sbCurrentGrid, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizerRightCol;
	bSizerRightCol = new wxBoxSizer( wxVERTICAL );

	sbGridOriginSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Grid Origin") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerGridOrigin;
	fgSizerGridOrigin = new wxFlexGridSizer( 2, 3, 5, 0 );
	fgSizerGridOrigin->AddGrowableCol( 1 );
	fgSizerGridOrigin->SetFlexibleDirection( wxBOTH );
	fgSizerGridOrigin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGridPosX = new wxStaticText( sbGridOriginSizer->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosX->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_GridOriginXCtrl = new wxTextCtrl( sbGridOriginSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginXCtrl, 0, wxEXPAND|wxLEFT, 5 );

	m_TextPosXUnits = new wxStaticText( sbGridOriginSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosXUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticTextGridPosY = new wxStaticText( sbGridOriginSizer->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosY->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosY, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_GridOriginYCtrl = new wxTextCtrl( sbGridOriginSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginYCtrl, 0, wxEXPAND|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_TextPosYUnits = new wxStaticText( sbGridOriginSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosYUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );


	sbGridOriginSizer->Add( fgSizerGridOrigin, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerRightCol->Add( sbGridOriginSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbFastSwitchSizer;
	sbFastSwitchSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fast Switching") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 3, 6, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGrid1 = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("Grid 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid1->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_grid1CtrlChoices;
	m_grid1Ctrl = new wxChoice( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid1CtrlChoices, 0 );
	m_grid1Ctrl->SetSelection( 0 );
	fgSizer3->Add( m_grid1Ctrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_grid1HotKey = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid1HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid1HotKey, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextGrid2 = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("Grid 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid2->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	wxArrayString m_grid2CtrlChoices;
	m_grid2Ctrl = new wxChoice( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid2CtrlChoices, 0 );
	m_grid2Ctrl->SetSelection( 0 );
	fgSizer3->Add( m_grid2Ctrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_grid2HotKey = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid2HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid2HotKey, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbFastSwitchSizer->Add( fgSizer3, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerRightCol->Add( sbFastSwitchSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	sbGridOverridesSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Grid Overrides") ), wxVERTICAL );

	wxFlexGridSizer* fgGridOverrides;
	fgGridOverrides = new wxFlexGridSizer( 5, 4, 4, 0 );
	fgGridOverrides->AddGrowableCol( 2 );
	fgGridOverrides->SetFlexibleDirection( wxBOTH );
	fgGridOverrides->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkGridOverrideConnectables = new wxCheckBox( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideConnectables, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 10 );

	m_staticTextConnectables = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("Connectable items:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextConnectables->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextConnectables, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 5 );

	m_GridOverrideConnectablesSize = new wxTextCtrl( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_GridOverrideConnectablesSize, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextConnectablesUnits = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextConnectablesUnits->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextConnectablesUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_checkGridOverrideWires = new wxCheckBox( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideWires, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_staticTextWires = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("Wires:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextWires->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextWires, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_GridOverrideWiresSize = new wxTextCtrl( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_GridOverrideWiresSize, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextWiresUnits = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWiresUnits->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextWiresUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_checkGridOverrideVias = new wxCheckBox( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideVias, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextVias = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVias->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextVias, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_GridOverrideViasSize = new wxTextCtrl( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_GridOverrideViasSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticTextViasUnits = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextViasUnits->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextViasUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_checkGridOverrideText = new wxCheckBox( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideText, 0, wxALIGN_CENTER, 5 );

	m_staticTextText = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextText->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_GridOverrideTextSize = new wxTextCtrl( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_GridOverrideTextSize, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticTextTextUnits = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextTextUnits->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextTextUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_checkGridOverrideGraphics = new wxCheckBox( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideGraphics, 0, wxALIGN_CENTER_VERTICAL, 10 );

	m_staticTextGraphics = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("Graphics:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextGraphics->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextGraphics, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_GridOverrideGraphicsSize = new wxTextCtrl( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_GridOverrideGraphicsSize, 0, wxEXPAND|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextGraphicsUnits = new wxStaticText( sbGridOverridesSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextGraphicsUnits->Wrap( -1 );
	fgGridOverrides->Add( m_staticTextGraphicsUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );


	sbGridOverridesSizer->Add( fgGridOverrides, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerRightCol->Add( sbGridOverridesSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerColumns->Add( bSizerRightCol, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerColumns, 1, wxEXPAND, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonResetSizes = new wxButton( this, wxID_ANY, _("Reset Grids"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonResetSizes->SetToolTip( _("Resets the list of grid sizes to default values") );

	bButtonSizer->Add( m_buttonResetSizes, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bButtonSizer->Add( 40, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bButtonSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRID_SETTINGS_BASE::OnInitDlg ) );
	m_addGridButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnAddGrid ), NULL, this );
	m_removeGridButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnRemoveGrid ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_GRID_SETTINGS_BASE::~DIALOG_GRID_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRID_SETTINGS_BASE::OnInitDlg ) );
	m_addGridButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnAddGrid ), NULL, this );
	m_removeGridButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnRemoveGrid ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnOkClick ), NULL, this );

}
