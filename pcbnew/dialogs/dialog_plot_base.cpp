///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 18 2010)
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
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_plotFormatOptChoices[] = { _("HPGL"), _("Gerber"), _("Postscript"), _("Postscript A4"), _("DXF") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxRadioBox( this, wxID_ANY, _("Format"), wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 1, 0 );
	m_plotFormatOpt->SetSelection( 0 );
	bSizer12->Add( m_plotFormatOpt, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers") ), wxHORIZONTAL );
	
	m_CopperLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Copper Layers") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_CopperLayersBoxSizer, 1, wxALL, 5 );
	
	m_TechnicalLayersBoxSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Technical Layers") ), wxVERTICAL );
	
	sbLayersSizer->Add( m_TechnicalLayersBoxSizer, 1, wxALL, 5 );
	
	bUpperSizer->Add( sbLayersSizer, 1, wxALL, 3 );
	
	wxBoxSizer* bPlotOptionsSizer;
	bPlotOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );
	
	m_plotSheetRef = new wxCheckBox( this, wxID_ANY, _("Plot sheet reference on all layers"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_plotSheetRef, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_plotPads_on_Silkscreen = new wxCheckBox( this, ID_ALLOW_PRINT_PAD_ON_SILKSCREEN, _("Plot pads on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotPads_on_Silkscreen->SetToolTip( _("Enable/disable print/plot pads on silkscreen layers\nWhen disable, pads are never potted on silkscreen layers\nWhen enable, pads are potted only if they appear on silkscreen layers") );
	
	sbOptionsSizer->Add( m_plotPads_on_Silkscreen, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_plotModuleValueOpt = new wxCheckBox( this, wxID_ANY, _("Plot module value on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_plotModuleValueOpt, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_plotModuleRefOpt = new wxCheckBox( this, ID_PRINT_REF, _("Plot module reference on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_plotModuleRefOpt, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_plotTextOther = new wxCheckBox( this, wxID_ANY, _("Plot other module texts on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotTextOther->SetToolTip( _("Enable/disable print/plot module field texts on silkscreen layers") );
	
	sbOptionsSizer->Add( m_plotTextOther, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_plotInvisibleText = new wxCheckBox( this, wxID_ANY, _("Plot invisible texts on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotInvisibleText->SetToolTip( _("Force print/plot module invisible texts on silkscreen layers") );
	
	sbOptionsSizer->Add( m_plotInvisibleText, 0, wxALL, 2 );
	
	m_plotNoViaOnMaskOpt = new wxCheckBox( this, wxID_ANY, _("Do not tent vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotNoViaOnMaskOpt->SetToolTip( _("Print/plot vias on mask layers. They are in this case not protected") );
	
	sbOptionsSizer->Add( m_plotNoViaOnMaskOpt, 0, wxALL, 2 );
	
	m_plotMirrorOpt = new wxCheckBox( this, ID_MIROR_OPT, _("Mirrored plot"), wxDefaultPosition, wxDefaultSize, 0 );
	sbOptionsSizer->Add( m_plotMirrorOpt, 0, wxALL, 2 );
	
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Drill marks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	bSizer15->Add( m_staticText11, 0, wxALL, 5 );
	
	wxString m_drillShapeOptChoices[] = { _("None"), _("Small"), _("Actual size") };
	int m_drillShapeOptNChoices = sizeof( m_drillShapeOptChoices ) / sizeof( wxString );
	m_drillShapeOpt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_drillShapeOptNChoices, m_drillShapeOptChoices, 0 );
	m_drillShapeOpt->SetSelection( 0 );
	bSizer15->Add( m_drillShapeOpt, 0, wxEXPAND, 5 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Scaling:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer15->Add( m_staticText12, 0, wxALL, 5 );
	
	wxString m_scaleOptChoices[] = { _("Auto"), _("1:1"), _("3:2"), _("2:1"), _("3:1") };
	int m_scaleOptNChoices = sizeof( m_scaleOptChoices ) / sizeof( wxString );
	m_scaleOpt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_scaleOptNChoices, m_scaleOptChoices, 0 );
	m_scaleOpt->SetSelection( 0 );
	bSizer15->Add( m_scaleOpt, 0, wxEXPAND, 5 );
	
	bSizer14->Add( bSizer15, 1, wxALL|wxEXPAND, 3 );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Plot mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	bSizer16->Add( m_staticText13, 0, wxALL, 5 );
	
	wxString m_plotModeOptChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_plotModeOptNChoices = sizeof( m_plotModeOptChoices ) / sizeof( wxString );
	m_plotModeOpt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotModeOptNChoices, m_plotModeOptChoices, 0 );
	m_plotModeOpt->SetSelection( 0 );
	bSizer16->Add( m_plotModeOpt, 0, wxEXPAND, 5 );
	
	m_textDefaultPenSize = new wxStaticText( this, wxID_ANY, _("Default\nlinewidth"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textDefaultPenSize->Wrap( -1 );
	m_textDefaultPenSize->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	bSizer16->Add( m_textDefaultPenSize, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_linesWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_linesWidth->SetToolTip( _("Line width for, e.g., sheet references.") );
	
	bSizer16->Add( m_linesWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer14->Add( bSizer16, 1, wxALL|wxEXPAND, 3 );
	
	sbOptionsSizer->Add( bSizer14, 1, wxALL|wxEXPAND, 3 );
	
	bPlotOptionsSizer->Add( sbOptionsSizer, 0, wxALL|wxEXPAND, 3 );
	
	bUpperSizer->Add( bPlotOptionsSizer, 1, 0, 5 );
	
	wxBoxSizer* bPlotIndOptionsSizer;
	bPlotIndOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_Gerber_OptionsBox;
	m_Gerber_OptionsBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Gerber Options") ), wxVERTICAL );
	
	m_useGerberExtensions = new wxCheckBox( this, wxID_ANY, _("Use proper file extensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberExtensions->SetToolTip( _("Use proper Gerber extensions - .GBL, .GTL, etc...") );
	
	m_Gerber_OptionsBox->Add( m_useGerberExtensions, 0, wxLEFT|wxRIGHT|wxTOP, 2 );
	
	m_excludeEdgeLayerOpt = new wxCheckBox( this, wxID_ANY, _("Exclude PCB edge layer\nfrom other layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludeEdgeLayerOpt->SetToolTip( _("Exclude contents of the pcb edge layer from all other layers") );
	
	m_Gerber_OptionsBox->Add( m_excludeEdgeLayerOpt, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_subtractMaskFromSilk = new wxCheckBox( this, wxID_ANY, _("Subtract mask from\nsilkscren"), wxDefaultPosition, wxDefaultSize, 0 );
	m_subtractMaskFromSilk->SetValue(true); 
	m_subtractMaskFromSilk->SetToolTip( _("Remove silkscreen from areas without soldermask") );
	
	m_Gerber_OptionsBox->Add( m_subtractMaskFromSilk, 0, wxTOP|wxRIGHT|wxLEFT, 2 );
	
	m_useAuxOriginCheckBox = new wxCheckBox( this, wxID_ANY, _("Use auxiliary origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Gerber_OptionsBox->Add( m_useAuxOriginCheckBox, 0, wxALL, 2 );
	
	bPlotIndOptionsSizer->Add( m_Gerber_OptionsBox, 0, wxALL|wxEXPAND, 3 );
	
	wxStaticBoxSizer* m_HPGL_OptionsBox;
	m_HPGL_OptionsBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("HPGL Options") ), wxVERTICAL );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	m_textPenSize = new wxStaticText( this, wxID_ANY, _("Pen size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPenSize->Wrap( -1 );
	bSizer20->Add( m_textPenSize, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	m_HPGLPenSizeOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer20->Add( m_HPGLPenSizeOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_textPenOvr = new wxStaticText( this, wxID_ANY, _("Pen overlay"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPenOvr->Wrap( -1 );
	bSizer20->Add( m_textPenOvr, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_HPGLPenOverlayOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGLPenOverlayOpt->SetToolTip( _("Set plot overlay for filling") );
	
	bSizer20->Add( m_HPGLPenOverlayOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizer22->Add( bSizer20, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	m_textPenSpeed = new wxStaticText( this, wxID_ANY, _("Pen speed (cm/s):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textPenSpeed->Wrap( -1 );
	bSizer21->Add( m_textPenSpeed, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_HPGLPenSpeedOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGLPenSpeedOpt->SetToolTip( _("Set pen speed in cm/s") );
	
	bSizer21->Add( m_HPGLPenSpeedOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizer22->Add( bSizer21, 1, wxEXPAND, 5 );
	
	m_HPGL_OptionsBox->Add( bSizer22, 1, wxEXPAND, 5 );
	
	bPlotIndOptionsSizer->Add( m_HPGL_OptionsBox, 0, wxALL|wxEXPAND, 3 );
	
	wxStaticBoxSizer* m_PS_OptionsBox;
	m_PS_OptionsBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Postscript Options") ), wxVERTICAL );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("X scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bSizer18->Add( m_staticText7, 0, wxALL, 5 );
	
	m_fineAdjustXscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXscaleOpt->SetToolTip( _("Set X scale adjust for exact scale plotting") );
	
	bSizer18->Add( m_fineAdjustXscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer17->Add( bSizer18, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Y scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bSizer19->Add( m_staticText8, 0, wxALL, 5 );
	
	m_fineAdjustYscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer19->Add( m_fineAdjustYscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer17->Add( bSizer19, 1, wxEXPAND, 5 );
	
	m_PS_OptionsBox->Add( bSizer17, 1, wxEXPAND, 5 );
	
	m_plotPSNegativeOpt = new wxCheckBox( this, wxID_ANY, _("Negative plot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PS_OptionsBox->Add( m_plotPSNegativeOpt, 0, wxALL, 2 );
	
	bPlotIndOptionsSizer->Add( m_PS_OptionsBox, 1, wxALL|wxEXPAND, 3 );
	
	bUpperSizer->Add( bPlotIndOptionsSizer, 0, 0, 5 );
	
	bSizer12->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerDirChoice;
	bSizerDirChoice = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bSizerDirChoice->Add( m_staticTextDir, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDirNmae;
	bSizerDirNmae = new wxBoxSizer( wxHORIZONTAL );
	
	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be relative or absolute.") );
	
	bSizerDirNmae->Add( m_outputDirectoryName, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_browseButton = new wxButton( this, wxID_ANY, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerDirNmae->Add( m_browseButton, 0, wxRIGHT|wxLEFT, 5 );
	
	bSizerDirChoice->Add( bSizerDirNmae, 0, wxRIGHT|wxEXPAND, 5 );
	
	bSizer12->Add( bSizerDirChoice, 0, wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer12->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_messagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_messagesBox->SetMinSize( wxSize( -1,100 ) );
	
	bSizer12->Add( m_messagesBox, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainSizer->Add( bSizer12, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_plotButton = new wxButton( this, wxID_ANY, _("Plot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotButton->SetDefault(); 
	bButtonsSizer->Add( m_plotButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonSaveOpt = new wxButton( this, ID_SAVE_OPT_PLOT, _("Apply Settings"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonSaveOpt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonDrill = new wxButton( this, ID_CREATE_DRILL_FILE, _("Generate Drill File"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonDrill, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bButtonsSizer, 0, wxALIGN_BOTTOM|wxALL, 3 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_scaleOpt->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnSetScaleOpt ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_plotButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::applyPlotSettings ), NULL, this );
	m_buttonDrill->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
}

DIALOG_PLOT_BASE::~DIALOG_PLOT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_scaleOpt->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnSetScaleOpt ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_plotButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::applyPlotSettings ), NULL, this );
	m_buttonDrill->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
	
}
