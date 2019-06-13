///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_plot_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_BASE::DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextPlotFmt = new wxStaticText( this, wxID_ANY, _("Plot format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPlotFmt->Wrap( -1 );
	bupperSizer->Add( m_staticTextPlotFmt, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxString m_plotFormatOptChoices[] = { _("Gerber"), _("Postscript"), _("SVG"), _("DXF"), _("HPGL"), _("PDF") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 0 );
	m_plotFormatOpt->SetSelection( 0 );
	bupperSizer->Add( m_plotFormatOpt, 0, wxALL, 6 );


	bupperSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bupperSizer->Add( m_staticTextDir, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );

	bupperSizer->Add( m_outputDirectoryName, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_browseButton->SetMinSize( wxSize( 29,29 ) );

	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_MainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bmiddleSizer;
	bmiddleSizer = new wxBoxSizer( wxHORIZONTAL );

	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Included Layers") ), wxHORIZONTAL );

	wxArrayString m_layerCheckListBoxChoices;
	m_layerCheckListBox = new wxCheckListBox( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_layerCheckListBoxChoices, 0 );
	m_layerCheckListBox->SetMinSize( wxSize( 150,-1 ) );

	m_LayersSizer->Add( m_layerCheckListBox, 1, wxEXPAND, 5 );


	bmiddleSizer->Add( m_LayersSizer, 1, wxALL|wxEXPAND, 3 );

	m_PlotOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General Options") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxHORIZONTAL );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_plotSheetRef = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Plot border and title block"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotSheetRef->SetMinSize( wxSize( 280,-1 ) );

	gbSizer1->Add( m_plotSheetRef, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotModuleValueOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Plot footprint values"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotModuleValueOpt->SetValue(true);
	gbSizer1->Add( m_plotModuleValueOpt, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotModuleRefOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), ID_PRINT_REF, _("Plot footprint references"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotModuleRefOpt->SetValue(true);
	gbSizer1->Add( m_plotModuleRefOpt, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotInvisibleText = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Force plotting of invisible values / refs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotInvisibleText->SetToolTip( _("Force plot invisible values and/or references") );

	gbSizer1->Add( m_plotInvisibleText, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_excludeEdgeLayerOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Exclude PCB edge layer from other layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludeEdgeLayerOpt->SetToolTip( _("Do not plot the contents of the PCB edge layer on any other layers.") );

	gbSizer1->Add( m_excludeEdgeLayerOpt, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_excludePadsFromSilkscreen = new wxCheckBox( sbOptionsSizer->GetStaticBox(), ID_ALLOW_PRINT_PAD_ON_SILKSCREEN, _("Exclude pads from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_excludePadsFromSilkscreen->SetToolTip( _("Do not plot pads on silkscreen layers, even when they are assigned to them.\nUncheck this if you wish to create assembly drawings from silkscreen layers.") );

	gbSizer1->Add( m_excludePadsFromSilkscreen, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotNoViaOnMaskOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Do not tent vias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotNoViaOnMaskOpt->SetToolTip( _("Remove soldermask on vias") );

	gbSizer1->Add( m_plotNoViaOnMaskOpt, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_useAuxOriginCheckBox = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Use auxiliary axis as origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useAuxOriginCheckBox->SetToolTip( _("Use auxiliary axis as coordinates origin in plot files") );

	gbSizer1->Add( m_useAuxOriginCheckBox, wxGBPosition( 7, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	drillMarksLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Drill marks:"), wxDefaultPosition, wxDefaultSize, 0 );
	drillMarksLabel->Wrap( -1 );
	gbSizer1->Add( drillMarksLabel, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_drillShapeOptChoices[] = { _("None"), _("Small"), _("Actual size") };
	int m_drillShapeOptNChoices = sizeof( m_drillShapeOptChoices ) / sizeof( wxString );
	m_drillShapeOpt = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_drillShapeOptNChoices, m_drillShapeOptChoices, 0 );
	m_drillShapeOpt->SetSelection( 0 );
	gbSizer1->Add( m_drillShapeOpt, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT, 5 );

	scalingLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Scaling:"), wxDefaultPosition, wxDefaultSize, 0 );
	scalingLabel->Wrap( -1 );
	gbSizer1->Add( scalingLabel, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_scaleOptChoices[] = { _("Auto"), _("1:1"), _("3:2"), _("2:1"), _("3:1") };
	int m_scaleOptNChoices = sizeof( m_scaleOptChoices ) / sizeof( wxString );
	m_scaleOpt = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_scaleOptNChoices, m_scaleOptChoices, 0 );
	m_scaleOpt->SetSelection( 1 );
	gbSizer1->Add( m_scaleOpt, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT, 5 );

	plotModeLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Plot mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	plotModeLabel->Wrap( -1 );
	gbSizer1->Add( plotModeLabel, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_plotModeOptChoices[] = { _("Filled"), _("Sketch") };
	int m_plotModeOptNChoices = sizeof( m_plotModeOptChoices ) / sizeof( wxString );
	m_plotModeOpt = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotModeOptNChoices, m_plotModeOptChoices, 0 );
	m_plotModeOpt->SetSelection( 0 );
	gbSizer1->Add( m_plotModeOpt, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT, 5 );

	m_lineWidthLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	m_lineWidthLabel->SetToolTip( _("Pen size used to draw items that have no pen size specified.\nUsed mainly to draw items in sketch mode.") );

	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_lineWidthCtrl = new wxTextCtrl( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthCtrl->SetToolTip( _("Line width for, e.g., sheet references.") );
	m_lineWidthCtrl->SetMinSize( wxSize( 120,-1 ) );

	gbSizer1->Add( m_lineWidthCtrl, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxLEFT, 5 );

	m_lineWidthUnits = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnits, wxGBPosition( 3, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_plotMirrorOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), ID_MIROR_OPT, _("Mirrored plot"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_plotMirrorOpt, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_plotPSNegativeOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Negative plot"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_plotPSNegativeOpt, wxGBPosition( 5, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_zoneFillCheck = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Check zone fills before plotting"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_zoneFillCheck, wxGBPosition( 7, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );


	gbSizer1->AddGrowableCol( 0 );
	gbSizer1->AddGrowableCol( 2 );

	sbOptionsSizer->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( sbOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_GerberOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Gerber Options") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 3, 0 );
	gbSizer2->SetFlexibleDirection( wxHORIZONTAL );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_useGerberExtensions = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Use Protel filename extensions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberExtensions->SetToolTip( _("Use Protel Gerber extensions (.GBL, .GTL, etc...)\nNo longer recommended. The official extension is .gbr") );
	m_useGerberExtensions->SetMinSize( wxSize( 280,-1 ) );

	gbSizer2->Add( m_useGerberExtensions, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_generateGerberJobFile = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Generate Gerber job file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_generateGerberJobFile->SetToolTip( _("Generate a Gerber job file that contains info about the board,\nand the list of generated Gerber plot files") );

	gbSizer2->Add( m_generateGerberJobFile, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_subtractMaskFromSilk = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_subtractMaskFromSilk->SetToolTip( _("Remove silkscreen from areas without soldermask") );

	gbSizer2->Add( m_subtractMaskFromSilk, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	coordFormatLabel = new wxStaticText( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Coordinate format:"), wxDefaultPosition, wxDefaultSize, 0 );
	coordFormatLabel->Wrap( -1 );
	gbSizer2->Add( coordFormatLabel, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_coordFormatCtrlChoices[] = { _("4.5, unit mm"), _("4.6, unit mm") };
	int m_coordFormatCtrlNChoices = sizeof( m_coordFormatCtrlChoices ) / sizeof( wxString );
	m_coordFormatCtrl = new wxChoice( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_coordFormatCtrlNChoices, m_coordFormatCtrlChoices, 0 );
	m_coordFormatCtrl->SetSelection( 0 );
	gbSizer2->Add( m_coordFormatCtrl, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_useGerberX2Format = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Use extended X2 format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberX2Format->SetToolTip( _("Use X2 Gerber file format.\nInclude  mainly X2 attributes in Gerber headers.\nIf not checked, use X1 format.\nIn X1 format, these attributes are included as comments in files.") );

	gbSizer2->Add( m_useGerberX2Format, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_useGerberNetAttributes = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Include netlist attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberNetAttributes->SetToolTip( _("Include netlist metadata and aperture attributes in Gerber files.\nIn X1 format, they are comments.\nUsed to check connectivity in CAM tools and Gerber viewers.") );

	gbSizer2->Add( m_useGerberNetAttributes, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 30 );


	gbSizer2->AddGrowableCol( 2 );

	m_GerberOptionsSizer->Add( gbSizer2, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( m_GerberOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_HPGLOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("HPGL Options") ), wxHORIZONTAL );

	m_hpglPenLabel = new wxStaticText( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, _("Default pen size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hpglPenLabel->Wrap( -1 );
	m_HPGLOptionsSizer->Add( m_hpglPenLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_hpglPenCtrl = new wxTextCtrl( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_HPGLOptionsSizer->Add( m_hpglPenCtrl, 5, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_hpglPenUnits = new wxStaticText( m_HPGLOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_hpglPenUnits->Wrap( -1 );
	m_HPGLOptionsSizer->Add( m_hpglPenUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	m_HPGLOptionsSizer->Add( 0, 0, 11, wxEXPAND, 5 );


	m_PlotOptionsSizer->Add( m_HPGLOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_PSOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Postscript Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 6, 3, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->AddGrowableCol( 4 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fgSizer2->SetMinSize( wxSize( 60,-1 ) );
	m_fineAdjustXLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("X scale factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXLabel->Wrap( -1 );
	fgSizer2->Add( m_fineAdjustXLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_fineAdjustXCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXCtrl->SetToolTip( _("Set global X scale adjust for exact scale postscript output.") );

	fgSizer2->Add( m_fineAdjustXCtrl, 0, wxEXPAND|wxRIGHT, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_fineAdjustYLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Y scale factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustYLabel->Wrap( -1 );
	fgSizer2->Add( m_fineAdjustYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_fineAdjustYCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustYCtrl->SetToolTip( _("Set global Y scale adjust for exact scale postscript output.") );

	fgSizer2->Add( m_fineAdjustYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 30 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_widthAdjustLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Track width correction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustLabel->Wrap( -1 );
	fgSizer2->Add( m_widthAdjustLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_widthAdjustCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustCtrl->SetToolTip( _("Set global width correction for exact width postscript output.\nThese width correction is intended to compensate tracks width and also pads and vias size errors.\nThe reasonable width correction value must be in a range of [-(MinTrackWidth-1), +(MinClearanceValue-1)] in decimils.") );

	fgSizer2->Add( m_widthAdjustCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT, 5 );

	m_widthAdjustUnits = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustUnits->Wrap( -1 );
	fgSizer2->Add( m_widthAdjustUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	m_PSOptionsSizer->Add( fgSizer2, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_forcePSA4OutputOpt = new wxCheckBox( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Force A4 output"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PSOptionsSizer->Add( m_forcePSA4OutputOpt, 0, wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( m_PSOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_SizerDXF_options = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("DXF Options") ), wxVERTICAL );

	m_DXF_plotModeOpt = new wxCheckBox( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Plot graphic items using their contours"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXF_plotModeOpt->SetValue(true);
	m_DXF_plotModeOpt->SetToolTip( _("Uncheck to plot graphic items using their center lines") );

	m_SizerDXF_options->Add( m_DXF_plotModeOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_DXF_plotTextStrokeFontOpt = new wxCheckBox( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Use Pcbnew font to plot texts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXF_plotTextStrokeFontOpt->SetToolTip( _("Check to use Pcbnew stroke font\nUncheck to plot oneline ASCII texts as editable text (using DXF font)") );

	m_SizerDXF_options->Add( m_DXF_plotTextStrokeFontOpt, 0, wxALL, 5 );

	wxBoxSizer* dxfSizer1;
	dxfSizer1 = new wxBoxSizer( wxHORIZONTAL );

	DXF_exportUnitsLabel = new wxStaticText( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Export units:"), wxDefaultPosition, wxDefaultSize, 0 );
	DXF_exportUnitsLabel->Wrap( -1 );
	dxfSizer1->Add( DXF_exportUnitsLabel, 0, wxALIGN_CENTER|wxALIGN_LEFT|wxALL, 5 );

	wxString m_DXF_plotUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_DXF_plotUnitsNChoices = sizeof( m_DXF_plotUnitsChoices ) / sizeof( wxString );
	m_DXF_plotUnits = new wxChoice( m_SizerDXF_options->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DXF_plotUnitsNChoices, m_DXF_plotUnitsChoices, 0 );
	m_DXF_plotUnits->SetSelection( 0 );
	m_DXF_plotUnits->SetToolTip( _("The units to use for the exported DXF file") );

	dxfSizer1->Add( m_DXF_plotUnits, 0, wxALIGN_CENTER|wxALL, 5 );


	m_SizerDXF_options->Add( dxfSizer1, 1, wxEXPAND, 5 );


	m_PlotOptionsSizer->Add( m_SizerDXF_options, 0, wxEXPAND|wxALL, 5 );


	bmiddleSizer->Add( m_PlotOptionsSizer, 0, 0, 5 );


	m_MainSizer->Add( bmiddleSizer, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* sbSizerMsg;
	sbSizerMsg = new wxBoxSizer( wxVERTICAL );

	m_messagesPanel = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_messagesPanel->SetMinSize( wxSize( -300,150 ) );

	sbSizerMsg->Add( m_messagesPanel, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	m_MainSizer->Add( sbSizerMsg, 1, wxEXPAND, 5 );

	m_sizerButtons = new wxBoxSizer( wxHORIZONTAL );

	m_buttonDRC = new wxButton( this, wxID_ANY, _("Run DRC..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizerButtons->Add( m_buttonDRC, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	m_sizerButtons->Add( m_sdbSizer1, 1, wxEXPAND, 5 );


	m_MainSizer->Add( m_sizerButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	m_popMenu = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_popMenu, ID_LAYER_FAB, wxString( _("Select Fab Layers") ) , wxEmptyString, wxITEM_NORMAL );
	m_popMenu->Append( m_menuItem1 );

	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_popMenu, ID_SELECT_COPPER_LAYERS, wxString( _("Select all Copper Layers") ) , wxEmptyString, wxITEM_NORMAL );
	m_popMenu->Append( m_menuItem2 );

	wxMenuItem* m_menuItem3;
	m_menuItem3 = new wxMenuItem( m_popMenu, ID_DESELECT_COPPER_LAYERS, wxString( _("Deselect all Copper Layers") ) , wxEmptyString, wxITEM_NORMAL );
	m_popMenu->Append( m_menuItem3 );

	wxMenuItem* m_menuItem4;
	m_menuItem4 = new wxMenuItem( m_popMenu, ID_SELECT_ALL_LAYERS, wxString( _("Select all Layers") ) , wxEmptyString, wxITEM_NORMAL );
	m_popMenu->Append( m_menuItem4 );

	wxMenuItem* m_menuItem5;
	m_menuItem5 = new wxMenuItem( m_popMenu, ID_DESELECT_ALL_LAYERS, wxString( _("Deselect all Layers") ) , wxEmptyString, wxITEM_NORMAL );
	m_popMenu->Append( m_menuItem5 );

	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT_BASE::DIALOG_PLOT_BASEOnContextMenu ), NULL, this );


	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT_BASE::OnRightClick ) );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_layerCheckListBox->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT_BASE::OnRightClick ), NULL, this );
	m_scaleOpt->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnSetScaleOpt ), NULL, this );
	m_useGerberX2Format->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnGerberX2Checked ), NULL, this );
	m_DXF_plotModeOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnChangeDXFPlotMode ), NULL, this );
	m_buttonDRC->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onRunDRC ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
	m_popMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnPopUpLayers ), this, m_menuItem1->GetId());
	m_popMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnPopUpLayers ), this, m_menuItem2->GetId());
	m_popMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnPopUpLayers ), this, m_menuItem3->GetId());
	m_popMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnPopUpLayers ), this, m_menuItem4->GetId());
	m_popMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnPopUpLayers ), this, m_menuItem5->GetId());
}

DIALOG_PLOT_BASE::~DIALOG_PLOT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	this->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT_BASE::OnRightClick ) );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnOutputDirectoryBrowseClicked ), NULL, this );
	m_layerCheckListBox->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DIALOG_PLOT_BASE::OnRightClick ), NULL, this );
	m_scaleOpt->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnSetScaleOpt ), NULL, this );
	m_useGerberX2Format->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnGerberX2Checked ), NULL, this );
	m_DXF_plotModeOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnChangeDXFPlotMode ), NULL, this );
	m_buttonDRC->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onRunDRC ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );

	delete m_popMenu;
}
