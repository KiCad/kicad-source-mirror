///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_plot_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_BASE::DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,350 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxHORIZONTAL );
	
	m_CopperLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Copper Layers:") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_CopperLayersBoxSizer, 1, wxALL, 5 );
	
	m_TechnicalLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Technical Layers:") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_TechnicalLayersBoxSizer, 1, wxALL, 5 );
	
	bLeftSizer->Add( sbLayersSizer, 1, wxEXPAND, 5 );
	
	m_useGerberExtensions = new wxCheckBox( this, wxID_ANY, _("Use proper Gerber extensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberExtensions->SetToolTip( _("Use Proper Gerber Extensions - .GBL, .GTL, etc...") );
	
	bLeftSizer->Add( m_useGerberExtensions, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_excludeEdgeLayerOpt = new wxCheckBox( this, wxID_ANY, _("Exclude pcb edge layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludeEdgeLayerOpt->SetToolTip( _("Exclude contents of the pcb edge layer from all other layers") );
	
	bLeftSizer->Add( m_excludeEdgeLayerOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_subtractMaskFromSilk = new wxCheckBox( this, wxID_ANY, _("Subtract mask from silk"), wxDefaultPosition, wxDefaultSize, 0 );
	m_subtractMaskFromSilk->SetToolTip( _("Remove silkscreen from areas without soldermask") );
	
	bLeftSizer->Add( m_subtractMaskFromSilk, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotSheetRef = new wxCheckBox( this, wxID_ANY, _("Plot sheet reference"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_plotSheetRef, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotPads_on_Silkscreen = new wxCheckBox( this, ID_ALLOW_PRINT_PAD_ON_SILKSCREEN, _("Plot pads on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotPads_on_Silkscreen->SetToolTip( _("Enable/disable print/plot pads on silkscreen layers\nWhen disable, pads are never potted on silkscreen layers\nWhen enable, pads are potted only if they appear on silkscreen layers") );
	
	bLeftSizer->Add( m_plotPads_on_Silkscreen, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotModuleValueOpt = new wxCheckBox( this, wxID_ANY, _("Plot module value"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_plotModuleValueOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotModuleRefOpt = new wxCheckBox( this, ID_PRINT_REF, _("Plot module reference"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_plotModuleRefOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotTextOther = new wxCheckBox( this, wxID_ANY, _("Plot other module texts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotTextOther->SetToolTip( _("Enable/disable print/plot module field texts on silkscreen layers") );
	
	bLeftSizer->Add( m_plotTextOther, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_plotInvisibleText = new wxCheckBox( this, wxID_ANY, _("Force plot invisible texts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotInvisibleText->SetToolTip( _("Force print/plot module invisible texts on silkscreen layers") );
	
	bLeftSizer->Add( m_plotInvisibleText, 0, wxALL, 5 );
	
	bUpperSizer->Add( bLeftSizer, 2, wxEXPAND, 5 );
	
	wxBoxSizer* bPlotOptionsSizer;
	bPlotOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_drillShapeOptChoices[] = { _("No drill mark"), _("Small mark"), _("Real drill") };
	int m_drillShapeOptNChoices = sizeof( m_drillShapeOptChoices ) / sizeof( wxString );
	m_drillShapeOpt = new wxRadioBox( this, wxID_ANY, _("Pads Drill Opt"), wxDefaultPosition, wxDefaultSize, m_drillShapeOptNChoices, m_drillShapeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_drillShapeOpt->SetSelection( 1 );
	bPlotOptionsSizer->Add( m_drillShapeOpt, 0, wxEXPAND|wxALL, 5 );
	
	wxString m_scaleOptChoices[] = { _("Auto scale"), _("Scale 1"), _("Scale 1.5"), _("Scale 2"), _("Scale 3") };
	int m_scaleOptNChoices = sizeof( m_scaleOptChoices ) / sizeof( wxString );
	m_scaleOpt = new wxRadioBox( this, wxID_ANY, _("Scale Opt"), wxDefaultPosition, wxDefaultSize, m_scaleOptNChoices, m_scaleOptChoices, 1, wxRA_SPECIFY_COLS );
	m_scaleOpt->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_scaleOpt, 0, wxEXPAND|wxALL, 5 );
	
	wxString m_plotModeOptChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_plotModeOptNChoices = sizeof( m_plotModeOptChoices ) / sizeof( wxString );
	m_plotModeOpt = new wxRadioBox( this, wxID_ANY, _("Plot Mode"), wxDefaultPosition, wxDefaultSize, m_plotModeOptNChoices, m_plotModeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_plotModeOpt->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_plotModeOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_choicePlotOffsetChoices[] = { _("Absolute"), _("Auxiliary axis") };
	int m_choicePlotOffsetNChoices = sizeof( m_choicePlotOffsetChoices ) / sizeof( wxString );
	m_choicePlotOffset = new wxRadioBox( this, wxID_ANY, _("Plot Origin"), wxDefaultPosition, wxDefaultSize, m_choicePlotOffsetNChoices, m_choicePlotOffsetChoices, 1, wxRA_SPECIFY_COLS );
	m_choicePlotOffset->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_choicePlotOffset, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bPlotOptionsSizer, 1, 0, 5 );
	
	wxBoxSizer* bSizerFmtPlot;
	bSizerFmtPlot = new wxBoxSizer( wxVERTICAL );
	
	wxString m_plotFormatOptChoices[] = { _("HPGL"), _("Gerber"), _("Postscript"), _("Postscript A4"), _("DXF Export") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxRadioBox( this, wxID_ANY, _("Plot Format"), wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 1, wxRA_SPECIFY_COLS );
	m_plotFormatOpt->SetSelection( 1 );
	bSizerFmtPlot->Add( m_plotFormatOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_HPGL_OptionsBox;
	m_HPGL_OptionsBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("HPGL Options:") ), wxVERTICAL );
	
	m_textPenSize = new wxStaticText( this, wxID_ANY, _("Pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPenSize->Wrap( -1 );
	m_HPGL_OptionsBox->Add( m_textPenSize, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_HPGLPenSizeOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGL_OptionsBox->Add( m_HPGLPenSizeOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Pen Speed (cm/s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_HPGL_OptionsBox->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_HPGLPenSpeedOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGLPenSpeedOpt->SetToolTip( _("Set pen speed in cm/s") );
	
	m_HPGL_OptionsBox->Add( m_HPGLPenSpeedOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_textPenOvr = new wxStaticText( this, wxID_ANY, _("Pen ovr"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPenOvr->Wrap( -1 );
	m_HPGL_OptionsBox->Add( m_textPenOvr, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_HPGLPenOverlayOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGLPenOverlayOpt->SetToolTip( _("Set plot overlay for filling") );
	
	m_HPGL_OptionsBox->Add( m_HPGLPenOverlayOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizerFmtPlot->Add( m_HPGL_OptionsBox, 0, wxEXPAND, 5 );
	
	bUpperSizer->Add( bSizerFmtPlot, 1, 0, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerPlotOpt;
	sbSizerPlotOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Plot Options:") ), wxVERTICAL );
	
	m_plotPSNegativeOpt = new wxCheckBox( this, wxID_ANY, _("Plot negative"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPlotOpt->Add( m_plotPSNegativeOpt, 0, wxALL, 5 );
	
	m_plotMirrorOpt = new wxCheckBox( this, ID_MIROR_OPT, _("Plot mirror"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerPlotOpt->Add( m_plotMirrorOpt, 0, wxALL, 5 );
	
	m_plotNoViaOnMaskOpt = new wxCheckBox( this, wxID_ANY, _("Vias on mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotNoViaOnMaskOpt->SetToolTip( _("Print/plot vias on mask layers. They are in this case not protected") );
	
	sbSizerPlotOpt->Add( m_plotNoViaOnMaskOpt, 0, wxALL, 5 );
	
	bButtonsSizer->Add( sbSizerPlotOpt, 0, wxEXPAND|wxALL, 5 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Default pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	bButtonsSizer->Add( m_staticText6, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_linesWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_linesWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bButtonsSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("X scale adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bButtonsSizer->Add( m_staticText7, 0, wxRIGHT|wxLEFT, 5 );
	
	m_fineAdjustXscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXscaleOpt->SetToolTip( _("Set X scale adjust for exact scale plotting") );
	
	bButtonsSizer->Add( m_fineAdjustXscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Y scale adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bButtonsSizer->Add( m_staticText8, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_fineAdjustYscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_fineAdjustYscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bButtonsSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_plotButton = new wxButton( this, wxID_ANY, _("Plot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotButton->SetDefault(); 
	bButtonsSizer->Add( m_plotButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonSaveOpt = new wxButton( this, ID_SAVE_OPT_PLOT, _("Save Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonSaveOpt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonDrill = new wxButton( this, ID_CREATE_DRILL_FILE, _("Generate drill file"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonDrill, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Quit"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bButtonsSizer, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDirChoice;
	bSizerDirChoice = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output Directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bSizerDirChoice->Add( m_staticTextDir, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDirNmae;
	bSizerDirNmae = new wxBoxSizer( wxHORIZONTAL );
	
	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDirNmae->Add( m_outputDirectoryName, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_browseButton = new wxButton( this, wxID_ANY, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDirNmae->Add( m_browseButton, 0, wxRIGHT|wxLEFT, 5 );
	
	bSizerDirChoice->Add( bSizerDirNmae, 1, wxEXPAND|wxRIGHT, 5 );
	
	bMainSizer->Add( bSizerDirChoice, 0, wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bMainSizer->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_messagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_messagesBox->SetMinSize( wxSize( -1,120 ) );
	
	bMainSizer->Add( m_messagesBox, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_plotButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::savePlotOptions ), NULL, this );
	m_buttonDrill->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
}

DIALOG_PLOT_BASE::~DIALOG_PLOT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_plotButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::savePlotOptions ), NULL, this );
	m_buttonDrill->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	
}
