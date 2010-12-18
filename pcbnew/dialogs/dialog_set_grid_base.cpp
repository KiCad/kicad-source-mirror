///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_set_grid_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SET_GRID_BASE::DIALOG_SET_GRID_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLeftSizer;
	sbLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Grid Size") ), wxVERTICAL );
	
	wxString m_UnitGridChoices[] = { _("Inches"), _("mm") };
	int m_UnitGridNChoices = sizeof( m_UnitGridChoices ) / sizeof( wxString );
	m_UnitGrid = new wxRadioBox( this, wxID_ANY, _("Grid Size Units"), wxDefaultPosition, wxDefaultSize, m_UnitGridNChoices, m_UnitGridChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitGrid->SetSelection( 0 );
	sbLeftSizer->Add( m_UnitGrid, 0, wxALL|wxEXPAND, 5 );
	
	
	sbLeftSizer->Add( 10, 10, 0, 0, 5 );
	
	m_staticTextSizeX = new wxStaticText( this, wxID_ANY, _("User Grid Size X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	sbLeftSizer->Add( m_staticTextSizeX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptGridSizeX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbLeftSizer->Add( m_OptGridSizeX, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextSizeY = new wxStaticText( this, wxID_ANY, _("User Grid Size Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	sbLeftSizer->Add( m_staticTextSizeY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptGridSizeY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbLeftSizer->Add( m_OptGridSizeY, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bUpperSizer->Add( sbLeftSizer, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbRightSizer;
	sbRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Grid Origin") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerGridOrigin;
	fgSizerGridOrigin = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizerGridOrigin->AddGrowableCol( 1 );
	fgSizerGridOrigin->SetFlexibleDirection( wxBOTH );
	fgSizerGridOrigin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextGridPosX = new wxStaticText( this, wxID_ANY, _("Grid origin X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosX->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosX, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_GridOriginXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginXCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextPosXUnits = new wxStaticText( this, wxID_ANY, _("Inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosXUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosXUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextGridPosY = new wxStaticText( this, wxID_ANY, _("Grid origin Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosY->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosY, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_GridOriginYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerGridOrigin->Add( m_GridOriginYCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TextPosYUnits = new wxStaticText( this, wxID_ANY, _("Inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosYUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosYUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbRightSizer->Add( fgSizerGridOrigin, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonReset = new wxButton( this, wxID_ANY, _("Reset Grid Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRightSizer->Add( m_buttonReset, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( sbRightSizer, 2, wxEXPAND|wxALL, 5 );
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizerMain->Add( m_sdbSizer1, 0, wxALIGN_RIGHT|wxTOP|wxBOTTOM, 5 );
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	// Connect Events
	m_buttonReset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnResetGridOrgClick ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnOkClick ), NULL, this );
}

DIALOG_SET_GRID_BASE::~DIALOG_SET_GRID_BASE()
{
	// Disconnect Events
	m_buttonReset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnResetGridOrgClick ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SET_GRID_BASE::OnOkClick ), NULL, this );
}
