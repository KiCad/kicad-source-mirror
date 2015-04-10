///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_set_grid_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SET_GRID_BASE::DIALOG_SET_GRID_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLeftSizer;
	sbLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("User Defined Grid") ), wxVERTICAL );
	
	wxString m_UnitGridChoices[] = { _("Inches"), _("Millimeters") };
	int m_UnitGridNChoices = sizeof( m_UnitGridChoices ) / sizeof( wxString );
	m_UnitGrid = new wxRadioBox( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_UnitGridNChoices, m_UnitGridChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitGrid->SetSelection( 0 );
	sbLeftSizer->Add( m_UnitGrid, 0, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextSizeX = new wxStaticText( this, wxID_ANY, _("Size X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	fgSizer31->Add( m_staticTextSizeX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptGridSizeX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptGridSizeX->SetMaxLength( 0 ); 
	fgSizer31->Add( m_OptGridSizeX, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_staticTextSizeY = new wxStaticText( this, wxID_ANY, _("Size Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	fgSizer31->Add( m_staticTextSizeY, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptGridSizeY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptGridSizeY->SetMaxLength( 0 ); 
	fgSizer31->Add( m_OptGridSizeY, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	
	sbLeftSizer->Add( fgSizer31, 0, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( sbLeftSizer, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbRightSizer;
	sbRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Origin") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizerGridOrigin;
	fgSizerGridOrigin = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizerGridOrigin->AddGrowableCol( 1 );
	fgSizerGridOrigin->SetFlexibleDirection( wxBOTH );
	fgSizerGridOrigin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextGridPosX = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosX->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosX, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxTOP, 5 );
	
	m_GridOriginXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_GridOriginXCtrl->SetMaxLength( 0 ); 
	fgSizerGridOrigin->Add( m_GridOriginXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_TextPosXUnits = new wxStaticText( this, wxID_ANY, _("Inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosXUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT|wxTOP, 5 );
	
	m_staticTextGridPosY = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGridPosY->Wrap( -1 );
	fgSizerGridOrigin->Add( m_staticTextGridPosY, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_GridOriginYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_GridOriginYCtrl->SetMaxLength( 0 ); 
	fgSizerGridOrigin->Add( m_GridOriginYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_TextPosYUnits = new wxStaticText( this, wxID_ANY, _("Inches"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextPosYUnits->Wrap( -1 );
	fgSizerGridOrigin->Add( m_TextPosYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );
	
	
	sbRightSizer->Add( fgSizerGridOrigin, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonReset = new wxButton( this, wxID_ANY, _("Reset Grid Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	sbRightSizer->Add( m_buttonReset, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer4->Add( sbRightSizer, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Fast Switching") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextGrid1 = new wxStaticText( this, wxID_ANY, _("Grid 1:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid1->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxTOP, 5 );
	
	m_comboBoxGrid1 = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizer3->Add( m_comboBoxGrid1, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_staticTextGrid2 = new wxStaticText( this, wxID_ANY, _("Grid 2:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrid2->Wrap( -1 );
	fgSizer3->Add( m_staticTextGrid2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_comboBoxGrid2 = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizer3->Add( m_comboBoxGrid2, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	sbSizer4->Add( fgSizer3, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer4->Add( sbSizer4, 1, wxALL|wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bSizer4, 1, wxEXPAND, 5 );
	
	
	bSizerMain->Add( bUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND|wxTOP, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
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
