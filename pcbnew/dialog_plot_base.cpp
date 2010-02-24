///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
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
	
	m_Use_Gerber_Extensions = new wxCheckBox( this, ID_USE_GERBER_EXTENSIONS, _("Use Proper Gerber Extensions"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Use_Gerber_Extensions->SetToolTip( _("Use Proper Gerber Extensions - .GBL, .GTL, etc...") );
	
	bLeftSizer->Add( m_Use_Gerber_Extensions, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Exclude_Edges_Pcb = new wxCheckBox( this, wxID_ANY, _("Exclude pcb edge layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Exclude_Edges_Pcb->SetToolTip( _("Exclude contents of the pcb edge layer from all other layers") );
	
	bLeftSizer->Add( m_Exclude_Edges_Pcb, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Sheet_Ref = new wxCheckBox( this, wxID_ANY, _("Print sheet reference"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bLeftSizer->Add( m_Plot_Sheet_Ref, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Pads_on_Silkscreen = new wxCheckBox( this, ID_ALLOW_PRINT_PAD_ON_SILKSCREEN, _("Print pads on silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Plot_Pads_on_Silkscreen->SetToolTip( _("Enable/disable print/plot pads on silkscreen layers\nWhen disable, pads are never potted on silkscreen layers\nWhen enable, pads are potted only if they appear on silkscreen layers") );
	
	bLeftSizer->Add( m_Plot_Pads_on_Silkscreen, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Text_Value = new wxCheckBox( this, ID_PRINT_VALUE, _("Print module value"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bLeftSizer->Add( m_Plot_Text_Value, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Text_Ref = new wxCheckBox( this, ID_PRINT_REF, _("Print module reference"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bLeftSizer->Add( m_Plot_Text_Ref, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Text_Div = new wxCheckBox( this, ID_PRINT_MODULE_TEXTS, _("Print other module texts"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Plot_Text_Div->SetToolTip( _("Enable/disable print/plot module field texts on silkscreen layers") );
	
	bLeftSizer->Add( m_Plot_Text_Div, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Plot_Invisible_Text = new wxCheckBox( this, ID_FORCE_PRINT_INVISIBLE_TEXT, _("Force print invisible texts"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_Plot_Invisible_Text->SetToolTip( _("Force print/plot module invisible texts on silkscreen layers") );
	
	bLeftSizer->Add( m_Plot_Invisible_Text, 0, wxALL, 5 );
	
	bUpperSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bPlotOptionsSizer;
	bPlotOptionsSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_Drill_Shape_OptChoices[] = { _("No drill mark"), _("Small mark"), _("Real drill") };
	int m_Drill_Shape_OptNChoices = sizeof( m_Drill_Shape_OptChoices ) / sizeof( wxString );
	m_Drill_Shape_Opt = new wxRadioBox( this, ID_DRILL_SHAPE_OPT, _("Pads Drill Opt"), wxDefaultPosition, wxDefaultSize, m_Drill_Shape_OptNChoices, m_Drill_Shape_OptChoices, 1, wxRA_SPECIFY_COLS );
	m_Drill_Shape_Opt->SetSelection( 1 );
	bPlotOptionsSizer->Add( m_Drill_Shape_Opt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Scale_OptChoices[] = { _("Auto scale"), _("Scale 1"), _("Scale 1.5"), _("Scale 2"), _("Scale 3") };
	int m_Scale_OptNChoices = sizeof( m_Scale_OptChoices ) / sizeof( wxString );
	m_Scale_Opt = new wxRadioBox( this, wxID_ANY, _("Scale Opt"), wxDefaultPosition, wxDefaultSize, m_Scale_OptNChoices, m_Scale_OptChoices, 1, wxRA_SPECIFY_COLS );
	m_Scale_Opt->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_Scale_Opt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_PlotModeOptChoices[] = { _("Line"), _("Filled"), _("Sketch") };
	int m_PlotModeOptNChoices = sizeof( m_PlotModeOptChoices ) / sizeof( wxString );
	m_PlotModeOpt = new wxRadioBox( this, wxID_ANY, _("Plot Mode"), wxDefaultPosition, wxDefaultSize, m_PlotModeOptNChoices, m_PlotModeOptChoices, 1, wxRA_SPECIFY_COLS );
	m_PlotModeOpt->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_PlotModeOpt, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_Plot_OffsetChoices[] = { _("Absolute"), _("Auxiliary axis") };
	int m_Choice_Plot_OffsetNChoices = sizeof( m_Choice_Plot_OffsetChoices ) / sizeof( wxString );
	m_Choice_Plot_Offset = new wxRadioBox( this, wxID_ANY, _("Plot Origin"), wxDefaultPosition, wxDefaultSize, m_Choice_Plot_OffsetNChoices, m_Choice_Plot_OffsetChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Plot_Offset->SetSelection( 0 );
	bPlotOptionsSizer->Add( m_Choice_Plot_Offset, 0, wxALL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bPlotOptionsSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerFmtPlot;
	bSizerFmtPlot = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PlotFormatOptChoices[] = { _("HPGL"), _("Gerber"), _("Postscript"), _("Postscript A4"), _("DXF Export") };
	int m_PlotFormatOptNChoices = sizeof( m_PlotFormatOptChoices ) / sizeof( wxString );
	m_PlotFormatOpt = new wxRadioBox( this, wxID_ANY, _("Plot Format"), wxDefaultPosition, wxDefaultSize, m_PlotFormatOptNChoices, m_PlotFormatOptChoices, 1, wxRA_SPECIFY_COLS );
	m_PlotFormatOpt->SetSelection( 1 );
	bSizerFmtPlot->Add( m_PlotFormatOpt, 0, wxALL|wxEXPAND, 5 );
	
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
	
	wxStaticBoxSizer* sbSizerPSOpt;
	sbSizerPSOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("PS Options:") ), wxVERTICAL );
	
	m_Plot_PS_Negative = new wxCheckBox( this, wxID_ANY, _("Plot negative"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbSizerPSOpt->Add( m_Plot_PS_Negative, 0, wxALL, 5 );
	
	bSizerFmtPlot->Add( sbSizerPSOpt, 0, wxEXPAND, 5 );
	
	bUpperSizer->Add( bSizerFmtPlot, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_PlotMirorOpt = new wxCheckBox( this, ID_MIROR_OPT, _("Plot mirror"), wxDefaultPosition, wxDefaultSize, 0 );
	
	bButtonsSizer->Add( m_PlotMirorOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PlotNoViaOnMaskOpt = new wxCheckBox( this, ID_MASKVIA_OPT, _("Vias on mask"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_PlotNoViaOnMaskOpt->SetToolTip( _("Print/plot vias on mask layers. They are in this case not protected") );
	
	bButtonsSizer->Add( m_PlotNoViaOnMaskOpt, 0, wxALL, 5 );
	
	m_staticText6 = new wxStaticText( this, wxID_ANY, _("Default pen size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );
	
	bButtonsSizer->Add( m_staticText6, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LinesWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_LinesWidth, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bButtonsSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_staticText7 = new wxStaticText( this, wxID_ANY, _("X scale adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bButtonsSizer->Add( m_staticText7, 0, wxRIGHT|wxLEFT, 5 );
	
	m_FineAdjustXscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FineAdjustXscaleOpt->SetToolTip( _("Set X scale adjust for exact scale plotting") );
	
	bButtonsSizer->Add( m_FineAdjustXscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Y scale adjust"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bButtonsSizer->Add( m_staticText8, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_FineAdjustYscaleOpt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_FineAdjustYscaleOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bButtonsSizer->Add( 0, 20, 1, wxEXPAND, 5 );
	
	m_PlotButton = new wxButton( this, ID_EXEC_PLOT, _("Plot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PlotButton->SetDefault(); 
	bButtonsSizer->Add( m_PlotButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonSaveOpt = new wxButton( this, ID_SAVE_OPT_PLOT, _("Save Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonSaveOpt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonDrill = new wxButton( this, ID_CREATE_DRILL_FILE, _("Generate drill file"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonDrill, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonQuit = new wxButton( this, wxID_CANCEL, _("Quit"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_buttonQuit, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bUpperSizer->Add( bButtonsSizer, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bUpperSizer, 0, wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Messages:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bMainSizer->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MessagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	m_MessagesBox->SetMinSize( wxSize( -1,120 ) );
	
	bMainSizer->Add( m_MessagesBox, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_PlotFormatOpt->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_PlotButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::SaveOptPlot ), NULL, this );
	m_buttonDrill->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
}

DIALOG_PLOT_BASE::~DIALOG_PLOT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_PLOT_BASE::OnCloseWindow ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_PlotFormatOpt->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_PlotButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_buttonSaveOpt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::SaveOptPlot ), NULL, this );
	m_buttonDrill->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_buttonQuit->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnQuit ), NULL, this );
}
