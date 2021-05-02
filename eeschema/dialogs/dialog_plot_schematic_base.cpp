///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_plot_schematic_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_SCHEMATIC_BASE::DIALOG_PLOT_SCHEMATIC_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerDir;
	bSizerDir = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextOutputDirectory = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOutputDirectory->Wrap( -1 );
	bSizerDir->Add( m_staticTextOutputDirectory, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the schematic main file location.") );

	bSizerDir->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 4 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerDir->Add( m_browseButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bMainSizer->Add( bSizerDir, 0, wxALL|wxEXPAND, 7 );

	m_optionsSizer = new wxBoxSizer( wxHORIZONTAL );

	wxString m_plotFormatOptChoices[] = { _("Postscript"), _("PDF"), _("SVG"), _("DXF"), _("HPGL") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxRadioBox( this, wxID_ANY, _("Output Format"), wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 1, wxRA_SPECIFY_COLS );
	m_plotFormatOpt->SetSelection( 2 );
	m_optionsSizer->Add( m_plotFormatOpt, 0, wxALIGN_TOP|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxStaticBoxSizer* sbOptions;
	sbOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText4 = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Page size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	gbSizer1->Add( m_staticText4, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	wxString m_paperSizeOptionChoices[] = { _("Schematic size"), _("A4"), _("A") };
	int m_paperSizeOptionNChoices = sizeof( m_paperSizeOptionChoices ) / sizeof( wxString );
	m_paperSizeOption = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_paperSizeOptionNChoices, m_paperSizeOptionChoices, 0 );
	m_paperSizeOption->SetSelection( 0 );
	gbSizer1->Add( m_paperSizeOption, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_plotDrawingSheet = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Plot drawing sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotDrawingSheet->SetValue(true);
	m_plotDrawingSheet->SetToolTip( _("Plot the drawing sheet border and title block") );

	gbSizer1->Add( m_plotDrawingSheet, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALL, 5 );

	wxStaticText* bOutputModeLabel;
	bOutputModeLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	bOutputModeLabel->Wrap( -1 );
	gbSizer1->Add( bOutputModeLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and White") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 0 );
	m_ModeColorOption->SetSelection( 0 );
	gbSizer1->Add( m_ModeColorOption, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_plotBackgroundColor = new wxCheckBox( sbOptions->GetStaticBox(), wxID_ANY, _("Plot background color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotBackgroundColor->SetToolTip( _("Plot the background color if the output format supports it") );

	gbSizer1->Add( m_plotBackgroundColor, wxGBPosition( 3, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Color theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	gbSizer1->Add( m_staticText9, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_colorThemeChoices;
	m_colorTheme = new wxChoice( sbOptions->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_colorThemeChoices, 0 );
	m_colorTheme->SetSelection( 0 );
	m_colorTheme->SetToolTip( _("Select the color theme to use for plotting") );

	gbSizer1->Add( m_colorTheme, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_lineWidthLabel = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( sbOptions->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthCtrl->SetToolTip( _("Selection of the default pen thickness used to draw items, when their thickness is set to 0.") );

	gbSizer1->Add( m_lineWidthCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_lineWidthUnits = new wxStaticText( sbOptions->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );


	gbSizer1->AddGrowableCol( 1 );

	sbOptions->Add( gbSizer1, 1, wxEXPAND, 5 );


	m_optionsSizer->Add( sbOptions, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_HPGLOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("HPGL Options") ), wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_plotOriginTitle = new wxStaticText( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, _("Position and units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotOriginTitle->Wrap( -1 );
	gbSizer2->Add( m_plotOriginTitle, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	wxString m_plotOriginOptChoices[] = { _("Bottom left, plotter units"), _("Centered, plotter units"), _("Page fit, user units"), _("Content fit, user units") };
	int m_plotOriginOptNChoices = sizeof( m_plotOriginOptChoices ) / sizeof( wxString );
	m_plotOriginOpt = new wxChoice( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotOriginOptNChoices, m_plotOriginOptChoices, 0 );
	m_plotOriginOpt->SetSelection( 0 );
	gbSizer2->Add( m_plotOriginOpt, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_penWidthLabel = new wxStaticText( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, _("Pen width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penWidthLabel->Wrap( -1 );
	gbSizer2->Add( m_penWidthLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_penWidthCtrl = new wxTextCtrl( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_penWidthCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALL, 5 );

	m_penWidthUnits = new wxStaticText( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penWidthUnits->Wrap( -1 );
	gbSizer2->Add( m_penWidthUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->AddGrowableCol( 1 );

	m_HPGLOptionsSizer->Add( gbSizer2, 1, wxEXPAND, 5 );


	m_optionsSizer->Add( m_HPGLOptionsSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( m_optionsSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMsgPanel;
	bSizerMsgPanel = new wxBoxSizer( wxVERTICAL );

	m_MessagesBox = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessagesBox->SetMinSize( wxSize( 300,150 ) );

	bSizerMsgPanel->Add( m_MessagesBox, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bSizerMsgPanel, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

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
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnUpdateUI ) );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotFormatSelection ), NULL, this );
	m_paperSizeOption->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPageSizeSelected ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotCurrent ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotAll ), NULL, this );
}

DIALOG_PLOT_SCHEMATIC_BASE::~DIALOG_PLOT_SCHEMATIC_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnCloseWindow ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnUpdateUI ) );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotFormatSelection ), NULL, this );
	m_paperSizeOption->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPageSizeSelected ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotCurrent ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_BASE::OnPlotAll ), NULL, this );

}
