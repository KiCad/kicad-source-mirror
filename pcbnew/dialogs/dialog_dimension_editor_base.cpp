///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "class_pcb_layer_box_selector.h"

#include "dialog_dimension_editor_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DIMENSION_EDITOR_BASE::DIALOG_DIMENSION_EDITOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDim = new wxStaticText( this, wxID_ANY, _("Text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDim->Wrap( -1 );
	bSizerMain->Add( m_staticTextDim, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Name = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Name->SetMaxLength( 0 ); 
	m_Name->SetMinSize( wxSize( 400,-1 ) );
	
	bSizerMain->Add( m_Name, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextSizeX = new wxStaticText( this, wxID_ANY, _("Text Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeX->Wrap( -1 );
	bSizerLeft->Add( m_staticTextSizeX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtSizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TxtSizeXCtrl->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_TxtSizeXCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextSizeY = new wxStaticText( this, wxID_ANY, _("Text Height"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSizeY->Wrap( -1 );
	bSizerLeft->Add( m_staticTextSizeY, 0, wxRIGHT|wxLEFT, 5 );
	
	m_TxtSizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TxtSizeYCtrl->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_TxtSizeYCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextWidth = new wxStaticText( this, wxID_ANY, _("Text Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextWidth->Wrap( -1 );
	bSizerLeft->Add( m_staticTextWidth, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_TxtWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TxtWidthCtrl->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_TxtWidthCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextPosX = new wxStaticText( this, wxID_ANY, _("Text Position X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosX->Wrap( -1 );
	bSizerLeft->Add( m_staticTextPosX, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlPosX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlPosX->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_textCtrlPosX, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticTextPosY = new wxStaticText( this, wxID_ANY, _("Text Position Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPosY->Wrap( -1 );
	bSizerLeft->Add( m_staticTextPosY, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlPosY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlPosY->SetMaxLength( 0 ); 
	bSizerLeft->Add( m_textCtrlPosY, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerUpper->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	wxString m_rbMirrorChoices[] = { _("Normal"), _("Mirror") };
	int m_rbMirrorNChoices = sizeof( m_rbMirrorChoices ) / sizeof( wxString );
	m_rbMirror = new wxRadioBox( this, wxID_ANY, _("Display"), wxDefaultPosition, wxDefaultSize, m_rbMirrorNChoices, m_rbMirrorChoices, 1, wxRA_SPECIFY_COLS );
	m_rbMirror->SetSelection( 0 );
	bSizerRight->Add( m_rbMirror, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextLayer = new wxStaticText( this, wxID_ANY, _("Layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayer->Wrap( -1 );
	bSizerRight->Add( m_staticTextLayer, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizerRight->Add( m_SelLayerBox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerUpper->Add( bSizerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_sdbSizerBts = new wxStdDialogButtonSizer();
	m_sdbSizerBtsOK = new wxButton( this, wxID_OK );
	m_sdbSizerBts->AddButton( m_sdbSizerBtsOK );
	m_sdbSizerBtsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerBts->AddButton( m_sdbSizerBtsCancel );
	m_sdbSizerBts->Realize();
	
	bSizerMain->Add( m_sdbSizerBts, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_sdbSizerBtsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerBtsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_EDITOR_BASE::OnOKClick ), NULL, this );
}

DIALOG_DIMENSION_EDITOR_BASE::~DIALOG_DIMENSION_EDITOR_BASE()
{
	// Disconnect Events
	m_sdbSizerBtsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_EDITOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerBtsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DIMENSION_EDITOR_BASE::OnOKClick ), NULL, this );
	
}
