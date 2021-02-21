///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_grid_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRID_SETTINGS_BASE::DIALOG_GRID_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_book = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	wxPanel* gridOriginPage;
	gridOriginPage = new wxPanel( m_book, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbLeftSizer;
	sbLeftSizer = new wxStaticBoxSizer( new wxStaticBox( gridOriginPage, wxID_ANY, _("Grid Origin") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerGridOrigin;
	fgSizerGridOrigin = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizerGridOrigin->AddGrowableCol( 1 );
	fgSizerGridOrigin->SetFlexibleDirection( wxBOTH );
	fgSizerGridOrigin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGridPosX = new wxStaticText( sbLeftSizer->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosX->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosX, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_GridOriginXCtrl = new wxTextCtrl( sbLeftSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginXCtrl, 0, wxEXPAND|wxLEFT, 5 );

	m_TextPosXUnits = new wxStaticText( sbLeftSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosXUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_staticTextGridPosY = new wxStaticText( sbLeftSizer->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosY->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosY, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_GridOriginYCtrl = new wxTextCtrl( sbLeftSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginYCtrl, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_TextPosYUnits = new wxStaticText( sbLeftSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosYUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );


	sbLeftSizer->Add( fgSizerGridOrigin, 0, wxEXPAND|wxALL, 5 );


	gridOriginPage->SetSizer( sbLeftSizer );
	gridOriginPage->Layout();
	sbLeftSizer->Fit( gridOriginPage );
	m_book->AddPage( gridOriginPage, _("a page"), false );
	wxPanel* currentGridPage;
	currentGridPage = new wxPanel( m_book, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( currentGridPage, wxID_ANY, _("Current Grid") ), wxVERTICAL );

	wxArrayString m_currentGridCtrlChoices;
	m_currentGridCtrl = new wxChoice( sbSizer5->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_currentGridCtrlChoices, 0 );
	m_currentGridCtrl->SetSelection( 0 );
	sbSizer5->Add( m_currentGridCtrl, 0, wxALL|wxEXPAND, 5 );


	currentGridPage->SetSizer( sbSizer5 );
	currentGridPage->Layout();
	sbSizer5->Fit( currentGridPage );
	m_book->AddPage( currentGridPage, _("a page"), false );

	bUpperSizer->Add( m_book, 1, wxEXPAND | wxALL, 5 );

	wxStaticBoxSizer* sbUserGridSizer;
	sbUserGridSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Defined Grid") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextSizeX = new wxStaticText( sbUserGridSizer->GetStaticBox(), wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	fgSizer31->Add( m_staticTextSizeX, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_OptGridSizeX = new wxTextCtrl( sbUserGridSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_OptGridSizeX, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );

	m_TextSizeXUnits = new wxStaticText( sbUserGridSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeXUnits->Wrap( -1 );
	fgSizer31->Add( m_TextSizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_staticTextSizeY = new wxStaticText( sbUserGridSizer->GetStaticBox(), wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	fgSizer31->Add( m_staticTextSizeY, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_OptGridSizeY = new wxTextCtrl( sbUserGridSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer31->Add( m_OptGridSizeY, 0, wxEXPAND|wxLEFT, 5 );

	m_TextSizeYUnits = new wxStaticText( sbUserGridSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeYUnits->Wrap( -1 );
	fgSizer31->Add( m_TextSizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	sbUserGridSizer->Add( fgSizer31, 0, wxEXPAND|wxALL, 5 );


	bUpperSizer->Add( sbUserGridSizer, 1, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( bUpperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbFastSwitchSizer;
	sbFastSwitchSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fast Switching") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 3, 0, 5 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextGrid1 = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("Grid 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid1->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_grid1CtrlChoices;
	m_grid1Ctrl = new wxChoice( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid1CtrlChoices, 0 );
	m_grid1Ctrl->SetSelection( 0 );
	m_grid1Ctrl->SetMinSize( wxSize( 240,-1 ) );

	fgSizer3->Add( m_grid1Ctrl, 1, wxALL|wxEXPAND, 5 );

	m_grid1HotKey = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid1HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid1HotKey, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticTextGrid2 = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("Grid 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid2->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );

	wxArrayString m_grid2CtrlChoices;
	m_grid2Ctrl = new wxChoice( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_grid2CtrlChoices, 0 );
	m_grid2Ctrl->SetSelection( 0 );
	m_grid2Ctrl->SetMinSize( wxSize( 240,-1 ) );

	fgSizer3->Add( m_grid2Ctrl, 1, wxALL|wxEXPAND, 5 );

	m_grid2HotKey = new wxStaticText( sbFastSwitchSizer->GetStaticBox(), wxID_ANY, _("(hotkey)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_grid2HotKey->Wrap( -1 );
	fgSizer3->Add( m_grid2HotKey, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbFastSwitchSizer->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( sbFastSwitchSizer, 0, wxEXPAND|wxALL, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bButtonSizer;
	bButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonResetOrigin = new wxButton( this, wxID_ANY, _("Reset Grid Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonSizer->Add( m_buttonResetOrigin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_buttonResetSizes = new wxButton( this, wxID_ANY, _("Reset Grid Sizes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonResetSizes->Hide();
	m_buttonResetSizes->SetToolTip( _("Resets the list of grid sizes to default values") );

	bButtonSizer->Add( m_buttonResetSizes, 0, wxALL, 5 );


	bButtonSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bButtonSizer->Add( m_sdbSizer, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bButtonSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRID_SETTINGS_BASE::OnInitDlg ) );
	m_buttonResetOrigin->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnResetGridOriginClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnOkClick ), NULL, this );
}

DIALOG_GRID_SETTINGS_BASE::~DIALOG_GRID_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRID_SETTINGS_BASE::OnInitDlg ) );
	m_buttonResetOrigin->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnResetGridOriginClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRID_SETTINGS_BASE::OnOkClick ), NULL, this );

}
