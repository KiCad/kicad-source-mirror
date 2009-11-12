///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_global_edit_tracks_and_vias_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbCurrSettingsSizer;
	sbCurrSettingsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Current Settings") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizeNetInfo;
	fgSizeNetInfo = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizeNetInfo->SetFlexibleDirection( wxBOTH );
	fgSizeNetInfo->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_CurrentNetText = new wxStaticText( this, wxID_ANY, _("Current Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentNetText->Wrap( -1 );
	fgSizeNetInfo->Add( m_CurrentNetText, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	m_CurrentNetName = new wxStaticText( this, wxID_ANY, _("NetName"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentNetName->Wrap( -1 );
	m_CurrentNetName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizeNetInfo->Add( m_CurrentNetName, 0, wxALL, 5 );
	
	m_CurrentNetclassText = new wxStaticText( this, wxID_ANY, _("Current NetClass:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentNetclassText->Wrap( -1 );
	fgSizeNetInfo->Add( m_CurrentNetclassText, 0, wxALL|wxALIGN_RIGHT, 5 );
	
	m_CurrentNetclassName = new wxStaticText( this, wxID_ANY, _("NetClassName"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CurrentNetclassName->Wrap( -1 );
	m_CurrentNetclassName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizeNetInfo->Add( m_CurrentNetclassName, 0, wxALL, 5 );
	
	sbCurrSettingsSizer->Add( fgSizeNetInfo, 1, wxEXPAND, 5 );
	
	m_gridDisplayCurrentSettings = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridDisplayCurrentSettings->CreateGrid( 2, 5 );
	m_gridDisplayCurrentSettings->EnableEditing( true );
	m_gridDisplayCurrentSettings->EnableGridLines( true );
	m_gridDisplayCurrentSettings->EnableDragGridSize( false );
	m_gridDisplayCurrentSettings->SetMargins( 0, 0 );
	
	// Columns
	m_gridDisplayCurrentSettings->AutoSizeColumns();
	m_gridDisplayCurrentSettings->EnableDragColMove( false );
	m_gridDisplayCurrentSettings->EnableDragColSize( true );
	m_gridDisplayCurrentSettings->SetColLabelSize( 30 );
	m_gridDisplayCurrentSettings->SetColLabelValue( 0, _("Track size") );
	m_gridDisplayCurrentSettings->SetColLabelValue( 1, _("Via diameter") );
	m_gridDisplayCurrentSettings->SetColLabelValue( 2, _("Via drill") );
	m_gridDisplayCurrentSettings->SetColLabelValue( 3, _("uVia size") );
	m_gridDisplayCurrentSettings->SetColLabelValue( 4, _("uVia Drill") );
	m_gridDisplayCurrentSettings->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridDisplayCurrentSettings->AutoSizeRows();
	m_gridDisplayCurrentSettings->EnableDragRowSize( true );
	m_gridDisplayCurrentSettings->SetRowLabelSize( 100 );
	m_gridDisplayCurrentSettings->SetRowLabelValue( 0, _("Netclass value") );
	m_gridDisplayCurrentSettings->SetRowLabelValue( 1, _("Current value") );
	m_gridDisplayCurrentSettings->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridDisplayCurrentSettings->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	sbCurrSettingsSizer->Add( m_gridDisplayCurrentSettings, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( sbCurrSettingsSizer, 1, wxALL|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerCommands;
	sbSizerCommands = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 5, 2, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_Net2CurrValueText = new wxStaticText( this, wxID_ANY, _("Set tracks and vias of the current Net to the current value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Net2CurrValueText->Wrap( -1 );
	fgSizer2->Add( m_Net2CurrValueText, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_Net2CurrValueButton = new wxButton( this, ID_CURRENT_VALUES_TO_CURRENT_NET, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_Net2CurrValueButton, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Set tracks and vias of the current Net to the Netclass value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer2->Add( m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_button3 = new wxButton( this, ID_NETCLASS_VALUES_TO_CURRENT_NET, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_button3, 0, wxALL, 5 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Set ALL tracks and vias to their Netclass value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	fgSizer2->Add( m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_button4 = new wxButton( this, ID_ALL_TRACKS_VIAS, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_button4, 0, wxALL, 5 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("Set ALL vias (no track) to their Netclass value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer2->Add( m_staticText7, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_button5 = new wxButton( this, ID_ALL_VIAS, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_button5, 0, wxALL, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Set ALL tracks (no via) to their Netclass value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer2->Add( m_staticText8, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_button6 = new wxButton( this, ID_ALL_TRACKS, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_button6, 0, wxALL, 5 );
	
	sbSizerCommands->Add( fgSizer2, 1, wxEXPAND, 5 );
	
	bLowerSizer->Add( sbSizerCommands, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonCancel, 0, wxALL|wxEXPAND, 5 );
	
	bLowerSizer->Add( bbuttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bMainSizer->Add( bLowerSizer, 1, wxEXPAND|wxALL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_Net2CurrValueButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE()
{
	// Disconnect Events
	m_Net2CurrValueButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button3->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button4->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnOkClick ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE::OnCancelClick ), NULL, this );
}
