///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 30 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_export_svg_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_SVG_BASE::DIALOG_EXPORT_SVG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bSizer4->Add( m_staticTextDir, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Enter a filename if you do not want to use default file names\nCan be used only when printing the current sheet") );
	m_outputDirectoryName->SetMinSize( wxSize( 450,-1 ) );

	bSizer4->Add( m_outputDirectoryName, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer4->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bMainSizer->Add( bSizer4, 0, wxEXPAND|wxALL, 10 );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers") ), wxHORIZONTAL );

	wxBoxSizer* bSizerCopper;
	bSizerCopper = new wxBoxSizer( wxVERTICAL );

	m_staticTextCopperLayers = new wxStaticText( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopperLayers->Wrap( -1 );
	bSizerCopper->Add( m_staticTextCopperLayers, 0, wxRIGHT|wxLEFT, 5 );

	wxArrayString m_CopperLayersListChoices;
	m_CopperLayersList = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_CopperLayersListChoices, 0 );
	bSizerCopper->Add( m_CopperLayersList, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbLayersSizer->Add( bSizerCopper, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizerTech;
	bSizerTech = new wxBoxSizer( wxVERTICAL );

	m_staticTextTechLayers = new wxStaticText( sbLayersSizer->GetStaticBox(), wxID_ANY, _("Technical layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTechLayers->Wrap( -1 );
	bSizerTech->Add( m_staticTextTechLayers, 0, wxRIGHT|wxLEFT, 5 );

	wxArrayString m_TechnicalLayersListChoices;
	m_TechnicalLayersList = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TechnicalLayersListChoices, 0 );
	bSizerTech->Add( m_TechnicalLayersList, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbLayersSizer->Add( bSizerTech, 1, wxEXPAND, 5 );


	bUpperSizer->Add( sbLayersSizer, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	wxString m_ModeColorOptionChoices[] = { _("Color"), _("Black and white") };
	int m_ModeColorOptionNChoices = sizeof( m_ModeColorOptionChoices ) / sizeof( wxString );
	m_ModeColorOption = new wxRadioBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Print Mode"), wxDefaultPosition, wxDefaultSize, m_ModeColorOptionNChoices, m_ModeColorOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_ModeColorOption->SetSelection( 1 );
	m_ModeColorOption->SetToolTip( _("Export as black elements on a white background") );

	sbOptionsSizer->Add( m_ModeColorOption, 0, wxEXPAND|wxALL, 5 );

	wxString m_rbSvgPageSizeOptChoices[] = { _("Page with frame and title block"), _("Current page size"), _("Board area only") };
	int m_rbSvgPageSizeOptNChoices = sizeof( m_rbSvgPageSizeOptChoices ) / sizeof( wxString );
	m_rbSvgPageSizeOpt = new wxRadioBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("SVG Page Size"), wxDefaultPosition, wxDefaultSize, m_rbSvgPageSizeOptNChoices, m_rbSvgPageSizeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbSvgPageSizeOpt->SetSelection( 0 );
	sbOptionsSizer->Add( m_rbSvgPageSizeOpt, 0, wxEXPAND|wxALL, 5 );

	m_PrintBoardEdgesCtrl = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Print board edges"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PrintBoardEdgesCtrl->SetValue(true);
	m_PrintBoardEdgesCtrl->SetToolTip( _("Print (or not) the edges layer on others layers") );

	sbOptionsSizer->Add( m_PrintBoardEdgesCtrl, 0, wxALL, 5 );

	m_printMirrorOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Print mirrored"), wxDefaultPosition, wxDefaultSize, 0 );
	m_printMirrorOpt->SetToolTip( _("Print the layer(s) horizontally mirrored") );

	sbOptionsSizer->Add( m_printMirrorOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxString m_rbFileOptChoices[] = { _("One file per layer"), _("All layers in a single file") };
	int m_rbFileOptNChoices = sizeof( m_rbFileOptChoices ) / sizeof( wxString );
	m_rbFileOpt = new wxRadioBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Pagination"), wxDefaultPosition, wxDefaultSize, m_rbFileOptNChoices, m_rbFileOptChoices, 1, wxRA_SPECIFY_COLS );
	m_rbFileOpt->SetSelection( 0 );
	sbOptionsSizer->Add( m_rbFileOpt, 0, wxALL|wxEXPAND, 5 );


	bUpperSizer->Add( sbOptionsSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bUpperSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( 300,150 ) );

	bSizer5->Add( m_messagesPanel, 1, wxEXPAND | wxALL, 5 );


	bMainSizer->Add( bSizer5, 1, wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_SVG_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_SVG_BASE::OnButtonPlot ), NULL, this );
}

DIALOG_EXPORT_SVG_BASE::~DIALOG_EXPORT_SVG_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_SVG_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXPORT_SVG_BASE::OnButtonPlot ), NULL, this );

}
