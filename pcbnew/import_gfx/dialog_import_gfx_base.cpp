///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 11 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_import_gfx_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_GFX_BASE::DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerFile;
	bSizerFile = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText37 = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	bSizerFile->Add( m_staticText37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_textCtrlFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlFileName->SetMinSize( wxSize( 300,-1 ) );
	
	bSizerFile->Add( m_textCtrlFileName, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	m_buttonBrowse = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFile->Add( m_buttonBrowse, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );
	
	
	bSizerMain->Add( bSizerFile, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer5->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_PCBGridUnitsChoices[] = { _("mm"), _("inch") };
	int m_PCBGridUnitsNChoices = sizeof( m_PCBGridUnitsChoices ) / sizeof( wxString );
	m_PCBGridUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PCBGridUnitsNChoices, m_PCBGridUnitsChoices, 0 );
	m_PCBGridUnits->SetSelection( 0 );
	m_PCBGridUnits->SetToolTip( _("Select PCB grid units") );
	
	bSizer5->Add( m_PCBGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer10->Add( bSizer5, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbOffsetOptionChoices[] = { _("Center of page"), _("Upper left corner of page"), _("Center left side of page"), _("Lower left corner of page"), _("User defined position") };
	int m_rbOffsetOptionNChoices = sizeof( m_rbOffsetOptionChoices ) / sizeof( wxString );
	m_rbOffsetOption = new wxRadioBox( this, wxID_ORIGIN_SELECT, _("Place origin (0,0) point:"), wxDefaultPosition, wxDefaultSize, m_rbOffsetOptionNChoices, m_rbOffsetOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOffsetOption->SetSelection( 0 );
	bSizer3->Add( m_rbOffsetOption, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("X Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer6->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_PCBXCoord = new wxTextCtrl( this, wxID_ANY, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_PCBXCoord->HasFlag( wxTE_MULTILINE ) )
	{
	m_PCBXCoord->SetMaxLength( 10 );
	}
	#else
	m_PCBXCoord->SetMaxLength( 10 );
	#endif
	m_PCBXCoord->SetToolTip( _("DXF origin on PCB Grid, X Coordinate") );
	
	bSizer6->Add( m_PCBXCoord, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer4->Add( bSizer6, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Y Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer7->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_PCBYCoord = new wxTextCtrl( this, wxID_ANY, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_PCBYCoord->HasFlag( wxTE_MULTILINE ) )
	{
	m_PCBYCoord->SetMaxLength( 10 );
	}
	#else
	m_PCBYCoord->SetMaxLength( 10 );
	#endif
	m_PCBYCoord->SetToolTip( _("DXF origin on PCB Grid, Y Coordinate") );
	
	bSizer7->Add( m_PCBYCoord, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer4->Add( bSizer7, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextScale = new wxStaticText( this, wxID_ANY, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScale->Wrap( -1 );
	bSizer11->Add( m_staticTextScale, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_tcScale = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_tcScale, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer4->Add( bSizer11, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer3->Add( bSizer4, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer10->Add( bSizer3, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizer10, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextBrdlayer = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdlayer->Wrap( -1 );
	bSizer8->Add( m_staticTextBrdlayer, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer8->Add( m_SelLayerBox, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizer8, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline8 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline8, 0, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnBrowseFiles ), NULL, this );
	m_rbOffsetOption->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_tcScale->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::onChangeHeight ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnOKClick ), NULL, this );
}

DIALOG_IMPORT_GFX_BASE::~DIALOG_IMPORT_GFX_BASE()
{
	// Disconnect Events
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnBrowseFiles ), NULL, this );
	m_rbOffsetOption->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_tcScale->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::onChangeHeight ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::OnOKClick ), NULL, this );
	
}
