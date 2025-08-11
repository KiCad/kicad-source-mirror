///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialogs/panel_printer_list.h"

#include "dialog_print_generic_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PRINT_GENERIC_BASE::DIALOG_PRINT_GENERIC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerPanelPrinters;
	bSizerPanelPrinters = new wxBoxSizer( wxVERTICAL );

	m_panelPrinters = new PANEL_PRINTER_LIST( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerPanelPrinters->Add( m_panelPrinters, 1, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bSizerPanelPrinters, 0, wxEXPAND, 5 );

	m_bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bRightCol;
	bRightCol = new wxBoxSizer( wxVERTICAL );

	m_sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_gbOptionsSizer = new wxGridBagSizer( 3, 0 );
	m_gbOptionsSizer->SetFlexibleDirection( wxBOTH );
	m_gbOptionsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	m_gbOptionsSizer->SetEmptyCellSize( wxSize( -1,10 ) );

	m_outputModeLabel = new wxStaticText( m_sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputModeLabel->Wrap( -1 );
	m_gbOptionsSizer->Add( m_outputModeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxString m_outputModeChoices[] = { _("Color"), _("Black and white") };
	int m_outputModeNChoices = sizeof( m_outputModeChoices ) / sizeof( wxString );
	m_outputMode = new wxChoice( m_sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_outputModeNChoices, m_outputModeChoices, 0 );
	m_outputMode->SetSelection( 0 );
	m_gbOptionsSizer->Add( m_outputMode, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_titleBlock = new wxCheckBox( m_sbOptionsSizer->GetStaticBox(), wxID_FRAME_SEL, _("Print drawing sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	m_titleBlock->SetValue(true);
	m_titleBlock->SetToolTip( _("Print Frame references.") );

	m_gbOptionsSizer->Add( m_titleBlock, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_gbOptionsSizer->AddGrowableCol( 1 );

	m_sbOptionsSizer->Add( m_gbOptionsSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );


	bRightCol->Add( m_sbOptionsSizer, 1, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* bScaleSizer;
	bScaleSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Scale") ), wxVERTICAL );

	m_scale1 = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("1:1"), wxDefaultPosition, wxDefaultSize, 0 );
	bScaleSizer->Add( m_scale1, 0, wxRIGHT|wxLEFT, 5 );


	bScaleSizer->Add( 0, 5, 0, 0, 5 );

	m_scaleFit = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("Fit to page"), wxDefaultPosition, wxDefaultSize, 0 );
	bScaleSizer->Add( m_scaleFit, 0, wxRIGHT|wxLEFT, 5 );


	bScaleSizer->Add( 0, 3, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerScaleCustom;
	bSizerScaleCustom = new wxBoxSizer( wxHORIZONTAL );

	m_scaleCustom = new wxRadioButton( bScaleSizer->GetStaticBox(), wxID_ANY, _("Custom:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerScaleCustom->Add( m_scaleCustom, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_scaleCustomText = new wxTextCtrl( bScaleSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerScaleCustom->Add( m_scaleCustomText, 1, wxEXPAND|wxRIGHT, 5 );


	bScaleSizer->Add( bSizerScaleCustom, 1, wxEXPAND|wxBOTTOM, 5 );


	bRightCol->Add( bScaleSizer, 0, wxALL|wxEXPAND, 5 );


	m_bUpperSizer->Add( bRightCol, 0, wxEXPAND, 5 );


	bMainSizer->Add( m_bUpperSizer, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_infoText = new wxStaticText( this, wxID_ANY, _("Info text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_infoText->Wrap( -1 );
	m_infoText->Hide();

	bSizer6->Add( m_infoText, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizer6, 0, wxEXPAND|wxLEFT, 10 );

	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_buttonOption = new wxButton( this, wxID_PRINT_OPTIONS, _("Page Setup..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOption->SetMinSize( wxSize( 120,-1 ) );

	bButtonsSizer->Add( m_buttonOption, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bButtonsSizer->Add( m_sdbSizer1, 1, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bButtonsSizer, 0, wxEXPAND|wxLEFT, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_GENERIC_BASE::onClose ) );
	m_scaleCustomText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onSetCustomScale ), NULL, this );
	m_buttonOption->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPageSetup ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPrintPreview ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPrintButtonClick ), NULL, this );
}

DIALOG_PRINT_GENERIC_BASE::~DIALOG_PRINT_GENERIC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PRINT_GENERIC_BASE::onClose ) );
	m_scaleCustomText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onSetCustomScale ), NULL, this );
	m_buttonOption->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPageSetup ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPrintPreview ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onCancelButtonClick ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PRINT_GENERIC_BASE::onPrintButtonClick ), NULL, this );

}
