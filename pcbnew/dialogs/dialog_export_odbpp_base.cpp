///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialogs/panel_fab_export_content.h"
#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_export_odbpp_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_ODBPP_BASE::DIALOG_EXPORT_ODBPP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	m_lblBrdFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblBrdFile->Wrap( -1 );
	bSizerTop->Add( m_lblBrdFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_outputFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputFileName->SetToolTip( _("Output archive or directory") );
	m_outputFileName->SetMinSize( wxSize( 350,-1 ) );

	bSizerTop->Add( m_outputFileName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerTop->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 10 );


	bMainSizer->Add( bSizerTop, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_fileFormatLabel = new wxStaticText( this, wxID_ANY, _("File Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fileFormatLabel->Wrap( -1 );
	bSizerLeftCol->Add( m_fileFormatLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerLeftCol->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblUnits = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblUnits->Wrap( -1 );
	gbSizer1->Add( m_lblUnits, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceUnitsChoices[] = { _("Millimeters"), _("Inches") };
	int m_choiceUnitsNChoices = sizeof( m_choiceUnitsChoices ) / sizeof( wxString );
	m_choiceUnits = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitsNChoices, m_choiceUnitsChoices, 0 );
	m_choiceUnits->SetSelection( 0 );
	gbSizer1->Add( m_choiceUnits, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_lblPrecision = new wxStaticText( this, wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPrecision->Wrap( -1 );
	m_lblPrecision->SetToolTip( _("The number of values following the decimal separator") );

	gbSizer1->Add( m_lblPrecision, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 15 );

	m_precision = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 16, 6 );
	m_precision->SetToolTip( _("The number of values following the decimal separator") );

	gbSizer1->Add( m_precision, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_lblVersion = new wxStaticText( this, wxID_ANY, _("Version:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVersion->Wrap( -1 );
	gbSizer1->Add( m_lblVersion, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_versionChoiceChoices[] = { _("8.1") };
	int m_versionChoiceNChoices = sizeof( m_versionChoiceChoices ) / sizeof( wxString );
	m_versionChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_versionChoiceNChoices, m_versionChoiceChoices, 0 );
	m_versionChoice->SetSelection( 0 );
	gbSizer1->Add( m_versionChoice, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );

	m_refillZones = new wxCheckBox( this, wxID_ANY, _("Refill zones before export"), wxDefaultPosition, wxDefaultSize, 0 );
	m_refillZones->SetValue(true);
	m_refillZones->SetToolTip( _("Refill zones before generating ODB++") );

	gbSizer1->Add( m_refillZones, wxGBPosition( 6, 0 ), wxGBSpan( 1, 2 ), wxTOP, 5 );

	m_lblCompress = new wxStaticText( this, wxID_ANY, _("Compression format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblCompress->Wrap( -1 );
	m_lblCompress->SetToolTip( _("Select the format to compress the output ODB++ files") );

	gbSizer1->Add( m_lblCompress, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceCompressChoices[] = { _("None"), _("ZIP"), _("TGZ") };
	int m_choiceCompressNChoices = sizeof( m_choiceCompressChoices ) / sizeof( wxString );
	m_choiceCompress = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), m_choiceCompressNChoices, m_choiceCompressChoices, 0 );
	m_choiceCompress->SetSelection( 1 );
	gbSizer1->Add( m_choiceCompress, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_lblProductName = new wxStaticText( this, wxID_ANY, _("Product name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblProductName->Wrap( -1 );
	gbSizer1->Add( m_lblProductName, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_productName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_productName->SetToolTip( _("Product model name in ODB++") );

	gbSizer1->Add( m_productName, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_lblOrigin = new wxStaticText( this, wxID_ANY, _("Origin:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblOrigin->Wrap( -1 );
	gbSizer1->Add( m_lblOrigin, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxString m_choiceOriginChoices[] = { _("Absolute coordinates"), _("Drill/place file origin"), _("Grid origin") };
	int m_choiceOriginNChoices = sizeof( m_choiceOriginChoices ) / sizeof( wxString );
	m_choiceOrigin = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceOriginNChoices, m_choiceOriginChoices, 0 );
	m_choiceOrigin->SetSelection( 0 );
	gbSizer1->Add( m_choiceOrigin, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 10 );


	gbSizer1->AddGrowableCol( 1 );

	bSizerLeftCol->Add( gbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );


	bSizerMiddle->Add( bSizerLeftCol, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizerMiddle->Add( 10, 0, 0, wxEXPAND, 5 );

	m_contentPanel = new PANEL_FAB_EXPORT_CONTENT( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerMiddle->Add( m_contentPanel, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bMainSizer->Add( bSizerMiddle, 0, wxEXPAND|wxBOTTOM, 5 );

	m_layersLabel = new wxStaticText( this, wxID_ANY, _("Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_layersLabel->Wrap( -1 );
	bMainSizer->Add( m_layersLabel, 0, wxTOP|wxLEFT|wxRIGHT, 10 );

	m_layersLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_layersLine, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_layers = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_layers->CreateGrid( 0, 6 );
	m_layers->EnableEditing( true );
	m_layers->EnableGridLines( true );
	m_layers->EnableDragGridSize( false );
	m_layers->SetMargins( 0, 0 );

	// Columns
	m_layers->SetColSize( 0, 36 );
	m_layers->SetColSize( 1, 150 );
	m_layers->SetColSize( 2, 200 );
	m_layers->SetColSize( 3, 145 );
	m_layers->SetColSize( 4, 85 );
	m_layers->SetColSize( 5, 175 );
	m_layers->EnableDragColMove( false );
	m_layers->EnableDragColSize( true );
	m_layers->SetColLabelValue( 0, wxEmptyString );
	m_layers->SetColLabelValue( 1, _("KiCad Layer") );
	m_layers->SetColLabelValue( 2, _("ODB++ Name") );
	m_layers->SetColLabelValue( 3, _("Type") );
	m_layers->SetColLabelValue( 4, _("Context") );
	m_layers->SetColLabelValue( 5, _("Span / Reference") );
	m_layers->SetColLabelSize( 22 );
	m_layers->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_layers->EnableDragRowSize( false );
	m_layers->SetRowLabelSize( 0 );
	m_layers->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_layers->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_layers->SetMinSize( wxSize( 900,300 ) );

	bMainSizer->Add( m_layers, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( -300,150 ) );

	bMainSizer->Add( m_messagesPanel, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onBrowseClicked ), NULL, this );
	m_choiceCompress->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onFormatChoice ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onOKClick ), NULL, this );
}

DIALOG_EXPORT_ODBPP_BASE::~DIALOG_EXPORT_ODBPP_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onBrowseClicked ), NULL, this );
	m_choiceCompress->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onFormatChoice ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_ODBPP_BASE::onOKClick ), NULL, this );

}
