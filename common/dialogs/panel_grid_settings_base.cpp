///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_grid_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_GRID_SETTINGS_BASE::PANEL_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerColumns;
	bSizerColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_gridsLabel = new wxStaticText( this, wxID_ANY, _("Grids"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridsLabel->Wrap( -1 );
	bSizerLeftCol->Add( m_gridsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_currentGridCtrl = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_currentGridCtrl->SetMinSize( wxSize( 240,-1 ) );

	bSizerLeftCol->Add( m_currentGridCtrl, 1, wxEXPAND|wxBOTTOM|wxLEFT, 3 );

	wxBoxSizer* bSizerGridButtons;
	bSizerGridButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addGridButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerGridButtons->Add( m_addGridButton, 0, wxRIGHT|wxLEFT, 5 );

	m_editGridButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerGridButtons->Add( m_editGridButton, 0, wxRIGHT, 5 );

	m_moveUpButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerGridButtons->Add( m_moveUpButton, 0, wxRIGHT, 5 );

	m_moveDownButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerGridButtons->Add( m_moveDownButton, 0, 0, 5 );


	bSizerGridButtons->Add( 25, 0, 0, wxEXPAND, 5 );

	m_removeGridButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerGridButtons->Add( m_removeGridButton, 0, 0, 5 );


	bSizerLeftCol->Add( bSizerGridButtons, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	bSizerColumns->Add( bSizerLeftCol, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bSizerColumns->Add( 16, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol;
	bSizerRightCol = new wxBoxSizer( wxVERTICAL );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Fast Grid Switching"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bSizerRightCol->Add( m_staticText21, 0, wxRIGHT|wxLEFT, 12 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRightCol->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 3, 6, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGrid1 = new wxStaticText( this, wxID_ANY, _("Grid 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid1->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 8 );

	wxArrayString m_grid1CtrlChoices;
	m_grid1Ctrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid1CtrlChoices, 0 );
	m_grid1Ctrl->SetSelection( 0 );
	fgSizer3->Add( m_grid1Ctrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_grid1HotKey = new wxStaticText( this, wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid1HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid1HotKey, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextGrid2 = new wxStaticText( this, wxID_ANY, _("Grid 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid2->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT, 8 );

	wxArrayString m_grid2CtrlChoices;
	m_grid2Ctrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid2CtrlChoices, 0 );
	m_grid2Ctrl->SetSelection( 0 );
	fgSizer3->Add( m_grid2Ctrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_grid2HotKey = new wxStaticText( this, wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid2HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid2HotKey, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRightCol->Add( fgSizer3, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 10 );

	m_overridesLabel = new wxStaticText( this, wxID_ANY, _("Grid Overrides"), wxDefaultPosition, wxDefaultSize, 0 );
	m_overridesLabel->Wrap( -1 );
	bSizerRightCol->Add( m_overridesLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRightCol->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgGridOverrides;
	fgGridOverrides = new wxFlexGridSizer( 0, 2, 6, 4 );
	fgGridOverrides->AddGrowableCol( 1 );
	fgGridOverrides->SetFlexibleDirection( wxBOTH );
	fgGridOverrides->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_checkGridOverrideConnected = new wxCheckBox( this, wxID_ANY, _("Connected items:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideConnected, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 8 );

	wxArrayString m_gridOverrideConnectedChoiceChoices;
	m_gridOverrideConnectedChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridOverrideConnectedChoiceChoices, 0 );
	m_gridOverrideConnectedChoice->SetSelection( 0 );
	fgGridOverrides->Add( m_gridOverrideConnectedChoice, 0, 0, 5 );

	m_checkGridOverrideWires = new wxCheckBox( this, wxID_ANY, _("Wires:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideWires, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 8 );

	wxArrayString m_gridOverrideWiresChoiceChoices;
	m_gridOverrideWiresChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridOverrideWiresChoiceChoices, 0 );
	m_gridOverrideWiresChoice->SetSelection( 0 );
	fgGridOverrides->Add( m_gridOverrideWiresChoice, 0, 0, 5 );

	m_checkGridOverrideVias = new wxCheckBox( this, wxID_ANY, _("Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideVias, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 8 );

	wxArrayString m_gridOverrideViasChoiceChoices;
	m_gridOverrideViasChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridOverrideViasChoiceChoices, 0 );
	m_gridOverrideViasChoice->SetSelection( 0 );
	fgGridOverrides->Add( m_gridOverrideViasChoice, 0, 0, 5 );

	m_checkGridOverrideText = new wxCheckBox( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 8 );

	wxArrayString m_gridOverrideTextChoiceChoices;
	m_gridOverrideTextChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridOverrideTextChoiceChoices, 0 );
	m_gridOverrideTextChoice->SetSelection( 0 );
	fgGridOverrides->Add( m_gridOverrideTextChoice, 0, 0, 5 );

	m_checkGridOverrideGraphics = new wxCheckBox( this, wxID_ANY, _("Graphics:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgGridOverrides->Add( m_checkGridOverrideGraphics, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 8 );

	wxArrayString m_gridOverrideGraphicsChoiceChoices;
	m_gridOverrideGraphicsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridOverrideGraphicsChoiceChoices, 0 );
	m_gridOverrideGraphicsChoice->SetSelection( 0 );
	fgGridOverrides->Add( m_gridOverrideGraphicsChoice, 0, 0, 5 );


	bSizerRightCol->Add( fgGridOverrides, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 10 );


	bSizerColumns->Add( bSizerRightCol, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerColumns, 1, wxEXPAND|wxTOP, 8 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_currentGridCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnEditGrid ), NULL, this );
	m_addGridButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnAddGrid ), NULL, this );
	m_editGridButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnEditGrid ), NULL, this );
	m_editGridButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateEditGrid ), NULL, this );
	m_moveUpButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnMoveGridUp ), NULL, this );
	m_moveUpButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateMoveUp ), NULL, this );
	m_moveDownButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnMoveGridDown ), NULL, this );
	m_moveDownButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateMoveDown ), NULL, this );
	m_removeGridButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnRemoveGrid ), NULL, this );
	m_removeGridButton->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateRemove ), NULL, this );
}

PANEL_GRID_SETTINGS_BASE::~PANEL_GRID_SETTINGS_BASE()
{
	// Disconnect Events
	m_currentGridCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnEditGrid ), NULL, this );
	m_addGridButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnAddGrid ), NULL, this );
	m_editGridButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnEditGrid ), NULL, this );
	m_editGridButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateEditGrid ), NULL, this );
	m_moveUpButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnMoveGridUp ), NULL, this );
	m_moveUpButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateMoveUp ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnMoveGridDown ), NULL, this );
	m_moveDownButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateMoveDown ), NULL, this );
	m_removeGridButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_GRID_SETTINGS_BASE::OnRemoveGrid ), NULL, this );
	m_removeGridButton->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_GRID_SETTINGS_BASE::OnUpdateRemove ), NULL, this );

}
