///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_plot_schematic_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_SCHEMATIC_BASE::DIALOG_PLOT_SCHEMATIC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerDir;
	bSizerDir = new wxBoxSizer( wxHORIZONTAL );

	m_outputPathLabel = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputPathLabel->Wrap( -1 );
	bSizerDir->Add( m_outputPathLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_outputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputPath->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the schematic main file location.") );

	bSizerDir->Add( m_outputPath, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 4 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerDir->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bMainSizer->Add( bSizerDir, 0, wxALL|wxEXPAND, 7 );

	m_optionsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_formatVariantSize = new wxBoxSizer( wxVERTICAL );

	wxString m_plotFormatOptChoices[] = { _("Postscript"), _("PDF"), _("SVG"), _("DXF") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxRadioBox( this, wxID_ANY, _("Output Format"), wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 1, wxRA_SPECIFY_COLS );
	m_plotFormatOpt->SetSelection( 1 );
	m_formatVariantSize->Add( m_plotFormatOpt, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_variantChoiceCtrlChoices;
	m_variantChoiceCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_variantChoiceCtrlChoices, 0 );
	m_variantChoiceCtrl->SetSelection( 0 );
	m_formatVariantSize->Add( m_variantChoiceCtrl, 0, wxLEFT|wxRIGHT, 5 );


	m_optionsSizer->Add( m_formatVariantSize, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbOptions;
	sbOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 5, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	m_staticText4 = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Page size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	gbSizer1->Add( m_staticText4, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_paperSizeOptionChoices[] = { _("Schematic size"), _("A4"), _("A") };
	int m_paperSizeOptionNChoices = sizeof( m_paperSizeOptionChoices ) / sizeof( wxString );
	m_paperSizeOption = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_paperSizeOptionNChoices, m_paperSizeOptionChoices, 0 );
	m_paperSizeOption->SetSelection( 0 );
	gbSizer1->Add( m_paperSizeOption, wxGBPosition( 0, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_plotDrawingSheet = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Plot drawing sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotDrawingSheet->SetValue(true);
	m_plotDrawingSheet->SetToolTip( _("Plot the drawing sheet border and title block") );

	gbSizer1->Add( m_plotDrawingSheet, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* bOutputModeLabel;
	bOutputModeLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	bOutputModeLabel->Wrap( -1 );
	gbSizer1->Add( bOutputModeLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and White") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 0 );
	m_ModeColorOption->SetSelection( 0 );
	gbSizer1->Add( m_ModeColorOption, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_colorThemeLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Color theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_colorThemeLabel->Wrap( -1 );
	gbSizer1->Add( m_colorThemeLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_colorThemeChoices;
	m_colorTheme = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_colorThemeChoices, 0 );
	m_colorTheme->SetSelection( 0 );
	m_colorTheme->SetToolTip( _("Select the color theme to use for plotting") );

	gbSizer1->Add( m_colorTheme, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_plotBackgroundColor = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Plot background color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotBackgroundColor->SetToolTip( _("Plot the background color if the output format supports it") );

	gbSizer1->Add( m_plotBackgroundColor, wxGBPosition( 5, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lineWidthLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Minimum line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( sbOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthCtrl->SetToolTip( _("Selection of the default pen thickness used to draw items, when their thickness is set to 0.") );

	gbSizer1->Add( m_lineWidthCtrl, wxGBPosition( 7, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_lineWidthUnits = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnits, wxGBPosition( 7, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbOptions->Add( gbSizer1, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	m_optionsSizer->Add( sbOptions, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bOptionsRight;
	bOptionsRight = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* m_sizerPDFOptions;
	m_sizerPDFOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("PDF Options") ), wxVERTICAL );

	m_plotPDFPropertyPopups = new wxCheckBox( m_sizerPDFOptions->GetStaticBox(), wxID_ANY, _("Generate property popups"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotPDFPropertyPopups->SetValue(true);
	m_sizerPDFOptions->Add( m_plotPDFPropertyPopups, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_plotPDFHierarchicalLinks = new wxCheckBox( m_sizerPDFOptions->GetStaticBox(), wxID_ANY, _("Generate clickable links for hierarchical elements"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotPDFHierarchicalLinks->SetValue(true);
	m_sizerPDFOptions->Add( m_plotPDFHierarchicalLinks, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_plotPDFMetadata = new wxCheckBox( m_sizerPDFOptions->GetStaticBox(), wxID_ANY, _("Generate metadata from AUTHOR && SUBJECT variables"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotPDFMetadata->SetValue(true);
	m_plotPDFMetadata->SetToolTip( _("Generate PDF document properties from AUTHOR and SUBJECT text variables") );

	m_sizerPDFOptions->Add( m_plotPDFMetadata, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bOptionsRight->Add( m_sizerPDFOptions, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_SizerDxfOption = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("DXF Options") ), wxVERTICAL );

	wxBoxSizer* bSizerDxf;
	bSizerDxf = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextDXF = new wxStaticText( m_SizerDxfOption->GetStaticBox(), wxID_ANY, _("Export units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDXF->Wrap( -1 );
	bSizerDxf->Add( m_staticTextDXF, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 8 );

	wxString m_DXF_plotUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_DXF_plotUnitsNChoices = sizeof( m_DXF_plotUnitsChoices ) / sizeof( wxString );
	m_DXF_plotUnits = new wxChoice( m_SizerDxfOption->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DXF_plotUnitsNChoices, m_DXF_plotUnitsChoices, 0 );
	m_DXF_plotUnits->SetSelection( 0 );
	bSizerDxf->Add( m_DXF_plotUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_SizerDxfOption->Add( bSizerDxf, 1, wxEXPAND, 5 );


	bOptionsRight->Add( m_SizerDxfOption, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_otherOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Other Options") ), wxVERTICAL );

	m_openFileAfterPlot = new wxCheckBox( m_otherOptions->GetStaticBox(), wxID_ANY, _("Open file after plot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_openFileAfterPlot->SetToolTip( _("Open output file with associated application after successful plot") );

	m_otherOptions->Add( m_openFileAfterPlot, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bOptionsRight->Add( m_otherOptions, 1, wxEXPAND|wxTOP, 5 );


	m_optionsSizer->Add( bOptionsRight, 1, wxBOTTOM|wxEXPAND|wxLEFT, 5 );


	bMainSizer->Add( m_optionsSizer, 0, wxEXPAND|wxRIGHT, 5 );

	wxBoxSizer* bSizerMsgPanel;
	bSizerMsgPanel = new wxBoxSizer( wxVERTICAL );

	m_MessagesBox = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessagesBox->SetMinSize( wxSize( 300,150 ) );

	bSizerMsgPanel->Add( m_MessagesBox, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizerMsgPanel, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnCloseWindow ) );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onPlotFormatSelection ), NULL, this );
	m_ModeColorOption->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onColorMode ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotCurrent ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotAll ), NULL, this );
}

DIALOG_PLOT_SCHEMATIC_BASE::~DIALOG_PLOT_SCHEMATIC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnCloseWindow ) );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onPlotFormatSelection ), NULL, this );
	m_ModeColorOption->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::onColorMode ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotCurrent ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotAll ), NULL, this );

}
