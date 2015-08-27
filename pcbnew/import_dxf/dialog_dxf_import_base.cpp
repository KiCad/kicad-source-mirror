///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "class_pcb_layer_box_selector.h"

#include "dialog_dxf_import_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DXF_IMPORT_BASE::DIALOG_DXF_IMPORT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_rbOffsetOptionChoices[] = { _("Center of page"), _("Upper left corner of page"), _("Center left side of page"), _("Lower left corner of page"), _("User defined position") };
	int m_rbOffsetOptionNChoices = sizeof( m_rbOffsetOptionChoices ) / sizeof( wxString );
	m_rbOffsetOption = new wxRadioBox( this, wxID_ORIGIN_SELECT, _("Place DXF origin (0,0) point:"), wxDefaultPosition, wxDefaultSize, m_rbOffsetOptionNChoices, m_rbOffsetOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_rbOffsetOption->SetSelection( 0 );
	bSizer3->Add( m_rbOffsetOption, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("X Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer6->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_DXFPCBXCoord = new wxTextCtrl( this, wxID_ANY, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXFPCBXCoord->SetMaxLength( 10 ); 
	m_DXFPCBXCoord->SetToolTip( _("DXF origin on PCB Grid, X Coordinate") );
	
	bSizer6->Add( m_DXFPCBXCoord, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer4->Add( bSizer6, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("Y Position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer7->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_DXFPCBYCoord = new wxTextCtrl( this, wxID_ANY, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXFPCBYCoord->SetMaxLength( 10 ); 
	m_DXFPCBYCoord->SetToolTip( _("DXF origin on PCB Grid, Y Coordinate") );
	
	bSizer7->Add( m_DXFPCBYCoord, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer4->Add( bSizer7, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer5->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_DXFPCBGridUnitsChoices[] = { _("mm"), _("inch") };
	int m_DXFPCBGridUnitsNChoices = sizeof( m_DXFPCBGridUnitsChoices ) / sizeof( wxString );
	m_DXFPCBGridUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DXFPCBGridUnitsNChoices, m_DXFPCBGridUnitsChoices, 0 );
	m_DXFPCBGridUnits->SetSelection( 0 );
	m_DXFPCBGridUnits->SetToolTip( _("Select PCB grid units") );
	
	bSizer5->Add( m_DXFPCBGridUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer4->Add( bSizer5, 1, wxEXPAND, 5 );
	
	
	bSizer3->Add( bSizer4, 1, 0, 5 );
	
	
	bSizerMain->Add( bSizer3, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextBrdlayer = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdlayer->Wrap( -1 );
	bSizer8->Add( m_staticTextBrdlayer, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer8->Add( m_SelLayerBox, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	
	bSizerMain->Add( bSizer8, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline8 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline8, 0, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonBrowse->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnBrowseDxfFiles ), NULL, this );
	m_rbOffsetOption->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DXF_IMPORT_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnOKClick ), NULL, this );
}

DIALOG_DXF_IMPORT_BASE::~DIALOG_DXF_IMPORT_BASE()
{
	// Disconnect Events
	m_buttonBrowse->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnBrowseDxfFiles ), NULL, this );
	m_rbOffsetOption->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_DXF_IMPORT_BASE::OriginOptionOnUpdateUI ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DXF_IMPORT_BASE::OnOKClick ), NULL, this );
	
}
