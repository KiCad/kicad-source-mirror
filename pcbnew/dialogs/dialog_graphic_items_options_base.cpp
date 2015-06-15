///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_graphic_items_options_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizerLeft;
	sbSizerLeft = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Graphics:") ), wxVERTICAL );
	
	m_GraphicSegmWidthTitle = new wxStaticText( this, wxID_ANY, _("Graphic segm Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_GraphicSegmWidthTitle->Wrap( -1 );
	sbSizerLeft->Add( m_GraphicSegmWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptPcbSegmWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptPcbSegmWidth->SetMaxLength( 0 ); 
	sbSizerLeft->Add( m_OptPcbSegmWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_BoardEdgesWidthTitle = new wxStaticText( this, wxID_ANY, _("Board Edges Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BoardEdgesWidthTitle->Wrap( -1 );
	sbSizerLeft->Add( m_BoardEdgesWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptPcbEdgesWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptPcbEdgesWidth->SetMaxLength( 0 ); 
	sbSizerLeft->Add( m_OptPcbEdgesWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_CopperTextWidthTitle = new wxStaticText( this, wxID_ANY, _("Copper Text Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CopperTextWidthTitle->Wrap( -1 );
	sbSizerLeft->Add( m_CopperTextWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptPcbTextWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptPcbTextWidth->SetMaxLength( 0 ); 
	sbSizerLeft->Add( m_OptPcbTextWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TextSizeVTitle = new wxStaticText( this, wxID_ANY, _("Text Size V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeVTitle->Wrap( -1 );
	sbSizerLeft->Add( m_TextSizeVTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptPcbTextVSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptPcbTextVSize->SetMaxLength( 0 ); 
	sbSizerLeft->Add( m_OptPcbTextVSize, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TextSizeHTitle = new wxStaticText( this, wxID_ANY, _("Text Size H"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextSizeHTitle->Wrap( -1 );
	sbSizerLeft->Add( m_TextSizeHTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptPcbTextHSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptPcbTextHSize->SetMaxLength( 0 ); 
	sbSizerLeft->Add( m_OptPcbTextHSize, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerUpper->Add( sbSizerLeft, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerMiddle;
	sbSizerMiddle = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Footprints:") ), wxVERTICAL );
	
	m_EdgeModWidthTitle = new wxStaticText( this, wxID_ANY, _("Edges Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeModWidthTitle->Wrap( -1 );
	sbSizerMiddle->Add( m_EdgeModWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptModuleEdgesWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleEdgesWidth->SetMaxLength( 0 ); 
	sbSizerMiddle->Add( m_OptModuleEdgesWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TextModWidthTitle = new wxStaticText( this, wxID_ANY, _("Text Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModWidthTitle->Wrap( -1 );
	sbSizerMiddle->Add( m_TextModWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptModuleTextWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextWidth->SetMaxLength( 0 ); 
	sbSizerMiddle->Add( m_OptModuleTextWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_TextModSizeVTitle = new wxStaticText( this, wxID_ANY, _("Text Size V"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeVTitle->Wrap( -1 );
	sbSizerMiddle->Add( m_TextModSizeVTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptModuleTextVSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextVSize->SetMaxLength( 0 ); 
	sbSizerMiddle->Add( m_OptModuleTextVSize, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_TextModSizeHTitle = new wxStaticText( this, wxID_ANY, _("Text Size H"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TextModSizeHTitle->Wrap( -1 );
	sbSizerMiddle->Add( m_TextModSizeHTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptModuleTextHSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptModuleTextHSize->SetMaxLength( 0 ); 
	sbSizerMiddle->Add( m_OptModuleTextHSize, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizerUpper->Add( sbSizerMiddle, 1, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizerRight;
	sbSizerRight = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General:") ), wxVERTICAL );
	
	m_DefaultPenSizeTitle = new wxStaticText( this, wxID_ANY, _("Default pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DefaultPenSizeTitle->Wrap( -1 );
	m_DefaultPenSizeTitle->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	sbSizerRight->Add( m_DefaultPenSizeTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultPenSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DefaultPenSizeCtrl->SetMaxLength( 0 ); 
	sbSizerRight->Add( m_DefaultPenSizeCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerUpper->Add( sbSizerRight, 1, wxEXPAND|wxALL, 5 );
	
	
	bSizerMain->Add( bSizerUpper, 1, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	
	// Connect Events
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::OnOkClick ), NULL, this );
}

DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::~DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE()
{
	// Disconnect Events
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::OnCancelClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE::OnOkClick ), NULL, this );
	
}
