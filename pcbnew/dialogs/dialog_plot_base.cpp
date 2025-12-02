///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/color_swatch.h"
#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

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
	bupperSizer->Add( m_staticTextPlotFmt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_plotFormatOptChoices[] = { _("Gerber"), _("Postscript"), _("SVG"), _("DXF"), _("PDF") };
	int m_plotFormatOptNChoices = sizeof( m_plotFormatOptChoices ) / sizeof( wxString );
	m_plotFormatOpt = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_plotFormatOptNChoices, m_plotFormatOptChoices, 0 );
	m_plotFormatOpt->SetSelection( 0 );
	bupperSizer->Add( m_plotFormatOpt, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 28 );

	m_staticTextDir = new wxStaticText( this, wxID_ANY, _("Output directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDir->Wrap( -1 );
	bupperSizer->Add( m_staticTextDir, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_outputDirectoryName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_outputDirectoryName->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );

	bupperSizer->Add( m_outputDirectoryName, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_openDirButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	m_openDirButton->SetToolTip( _("Open output directory") );

	bupperSizer->Add( m_openDirButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bupperSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT, 5 );


	m_MainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	bmiddleSizer = new wxBoxSizer( wxHORIZONTAL );

	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Include Layers") ), wxHORIZONTAL );

	wxArrayString m_layerCheckListBoxChoices;
	m_layerCheckListBox = new wxCheckListBox( m_LayersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_layerCheckListBoxChoices, 0 );
	m_layerCheckListBox->SetMinSize( wxSize( 150,-1 ) );

	m_LayersSizer->Add( m_layerCheckListBox, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 3 );


	bmiddleSizer->Add( m_LayersSizer, 1, wxALL|wxEXPAND, 5 );

	m_PlotOptionsSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbOptionsSizer;
	sbOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("General Options") ), wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 0 );
	gbSizer1->SetFlexibleDirection( wxHORIZONTAL );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_plotSheetRef = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Plot drawing sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotSheetRef->SetMinSize( wxSize( 280,-1 ) );

	gbSizer1->Add( m_plotSheetRef, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_subtractMaskFromSilk = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Subtract soldermask from silkscreen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_subtractMaskFromSilk->SetToolTip( _("Remove silkscreen from areas without soldermask") );

	gbSizer1->Add( m_subtractMaskFromSilk, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotDNP = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Indicate DNP on fabrication layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_plotDNP->SetToolTip( _("Hide or cross-out DNP footprints on fabrication layers") );

	gbSizer1->Add( m_plotDNP, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_hideDNP = new wxRadioButton( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Hide"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_hideDNP->SetToolTip( _("Hide the footprint text and graphics") );

	gbSizer1->Add( m_hideDNP, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 25 );

	m_crossoutDNP = new wxRadioButton( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Cross-out"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_crossoutDNP, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 25 );

	m_sketchPadsOnFabLayers = new wxCheckBox( sbOptionsSizer->GetStaticBox(), ID_ALLOW_PRINT_PAD_ON_SILKSCREEN, _("Sketch pads on fabrication layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sketchPadsOnFabLayers->SetToolTip( _("Include pad outlines on F.Fab and B.Fab layers when plotting") );

	gbSizer1->Add( m_sketchPadsOnFabLayers, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_plotPadNumbers = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Include pad numbers"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_plotPadNumbers, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxLEFT, 25 );

	m_zoneFillCheck = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Check zone fills before plotting"), wxDefaultPosition, wxDefaultSize, 0 );
	m_zoneFillCheck->SetValue(true);
	gbSizer1->Add( m_zoneFillCheck, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	drillMarksLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Drill marks:"), wxDefaultPosition, wxDefaultSize, 0 );
	drillMarksLabel->Wrap( -1 );
	gbSizer1->Add( drillMarksLabel, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_drillShapeOptChoices[] = { _("None"), _("Small"), _("Actual size") };
	int m_drillShapeOptNChoices = sizeof( m_drillShapeOptChoices ) / sizeof( wxString );
	m_drillShapeOpt = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_drillShapeOptNChoices, m_drillShapeOptChoices, 0 );
	m_drillShapeOpt->SetSelection( 2 );
	gbSizer1->Add( m_drillShapeOpt, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT, 5 );

	scalingLabel = new wxStaticText( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Scaling:"), wxDefaultPosition, wxDefaultSize, 0 );
	scalingLabel->Wrap( -1 );
	gbSizer1->Add( scalingLabel, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_scaleOptChoices[] = { _("Auto"), _("1:1"), _("3:2"), _("2:1"), _("3:1") };
	int m_scaleOptNChoices = sizeof( m_scaleOptChoices ) / sizeof( wxString );
	m_scaleOpt = new wxChoice( sbOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_scaleOptNChoices, m_scaleOptChoices, 0 );
	m_scaleOpt->SetSelection( 1 );
	gbSizer1->Add( m_scaleOpt, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxLEFT, 5 );

	m_useAuxOriginCheckBox = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Use drill/place file origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useAuxOriginCheckBox->SetToolTip( _("Use the drill/place file origin as the coordinate origin for plotted files") );

	gbSizer1->Add( m_useAuxOriginCheckBox, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_plotMirrorOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), ID_MIROR_OPT, _("Mirrored plot"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_plotMirrorOpt, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_plotPSNegativeOpt = new wxCheckBox( sbOptionsSizer->GetStaticBox(), wxID_ANY, _("Negative plot"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_plotPSNegativeOpt, wxGBPosition( 4, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );


	sbOptionsSizer->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( sbOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_SizerSolderMaskAlert = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapAlert = new wxStaticBitmap( this, wxID_ANY, wxArtProvider::GetBitmap( wxASCII_STR(wxART_WARNING), wxASCII_STR(wxART_CMN_DIALOG) ), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizerSolderMaskAlert->Add( m_bitmapAlert, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	wxBoxSizer* bSizerWarningText;
	bSizerWarningText = new wxBoxSizer( wxVERTICAL );

	m_staticTextAlert = new wxStaticText( this, wxID_ANY, _("Global solder mask minimum width and/or margin are not set to 0. "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAlert->Wrap( -1 );
	bSizerWarningText->Add( m_staticTextAlert, 0, wxTOP|wxLEFT, 5 );

	m_staticTextAlert1 = new wxStaticText( this, wxID_ANY, _("Most board manufacturers expect 0 and use their own constraints for solder mask minimum width."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAlert1->Wrap( 580 );
	bSizerWarningText->Add( m_staticTextAlert1, 0, wxTOP|wxLEFT, 5 );

	wxBoxSizer* bSizerSecondLine;
	bSizerSecondLine = new wxBoxSizer( wxHORIZONTAL );


	bSizerSecondLine->Add( 0, 0, 1, wxEXPAND, 5 );

	m_boardSetup = new wxHyperlinkCtrl( this, wxID_ANY, _("Board setup"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_boardSetup->SetToolTip( _("File > Board Setup...") );

	bSizerSecondLine->Add( m_boardSetup, 0, wxLEFT|wxRIGHT|wxTOP, 2 );


	bSizerWarningText->Add( bSizerSecondLine, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_SizerSolderMaskAlert->Add( bSizerWarningText, 1, wxEXPAND, 5 );


	m_PlotOptionsSizer->Add( m_SizerSolderMaskAlert, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

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
	m_generateGerberJobFile->SetValue(true);
	m_generateGerberJobFile->SetToolTip( _("Generate a Gerber job file that contains info about the board,\nand the list of generated Gerber plot files") );

	gbSizer2->Add( m_generateGerberJobFile, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	coordFormatLabel = new wxStaticText( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Coordinate format:"), wxDefaultPosition, wxDefaultSize, 0 );
	coordFormatLabel->Wrap( -1 );
	gbSizer2->Add( coordFormatLabel, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	wxString m_coordFormatCtrlChoices[] = { _("4.5, unit mm"), _("4.6, unit mm") };
	int m_coordFormatCtrlNChoices = sizeof( m_coordFormatCtrlChoices ) / sizeof( wxString );
	m_coordFormatCtrl = new wxChoice( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_coordFormatCtrlNChoices, m_coordFormatCtrlChoices, 0 );
	m_coordFormatCtrl->SetSelection( 1 );
	gbSizer2->Add( m_coordFormatCtrl, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_useGerberX2Format = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Use extended X2 format (recommended)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberX2Format->SetToolTip( _("Use X2 Gerber file format.\nInclude  mainly X2 attributes in Gerber headers.\nIf not checked, use X1 format.\nIn X1 format, these attributes are included as comments in files.") );

	gbSizer2->Add( m_useGerberX2Format, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_useGerberNetAttributes = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Include netlist attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_useGerberNetAttributes->SetToolTip( _("Include netlist metadata and aperture attributes in Gerber files.\nThey are comments in the X1 format.\nUsed to check connectivity in CAM tools and Gerber viewers.") );

	gbSizer2->Add( m_useGerberNetAttributes, wxGBPosition( 2, 1 ), wxGBSpan( 1, 2 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 30 );

	m_disableApertMacros = new wxCheckBox( m_GerberOptionsSizer->GetStaticBox(), wxID_ANY, _("Disable aperture macros (not recommended)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_disableApertMacros->SetToolTip( _("Disable aperture macros in Gerber files\nUse *only* for broken Gerber viewers.") );

	gbSizer2->Add( m_disableApertMacros, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 30 );


	gbSizer2->AddGrowableCol( 2 );

	m_GerberOptionsSizer->Add( gbSizer2, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( m_GerberOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_PSOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Postscript Options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 6, 5, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->AddGrowableCol( 4 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fgSizer2->SetMinSize( wxSize( 60,-1 ) );
	m_fineAdjustXLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("X scale factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXLabel->Wrap( -1 );
	fgSizer2->Add( m_fineAdjustXLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 1 );

	m_fineAdjustXCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustXCtrl->SetToolTip( _("Set global X scale adjust for exact scale PostScript output.") );

	fgSizer2->Add( m_fineAdjustXCtrl, 0, wxEXPAND|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_fineAdjustYLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Y scale factor:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustYLabel->Wrap( -1 );
	fgSizer2->Add( m_fineAdjustYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 30 );

	m_fineAdjustYCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fineAdjustYCtrl->SetToolTip( _("Set global Y scale adjust for exact scale PostScript output.") );

	fgSizer2->Add( m_fineAdjustYCtrl, 0, wxEXPAND|wxRIGHT|wxLEFT, 30 );


	fgSizer2->Add( 0, 0, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_widthAdjustLabel = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Track width correction:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustLabel->Wrap( -1 );
	fgSizer2->Add( m_widthAdjustLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_widthAdjustCtrl = new wxTextCtrl( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustCtrl->SetToolTip( _("Set global width correction for exact width PostScript output.\nThese width correction is intended to compensate tracks width and also pads and vias size errors.\nThe reasonable width correction value must be in a range of [-(MinTrackWidth-1), +(MinClearanceValue-1)].") );

	fgSizer2->Add( m_widthAdjustCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_widthAdjustUnits = new wxStaticText( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthAdjustUnits->Wrap( -1 );
	fgSizer2->Add( m_widthAdjustUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


	m_PSOptionsSizer->Add( fgSizer2, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_forcePSA4OutputOpt = new wxCheckBox( m_PSOptionsSizer->GetStaticBox(), wxID_ANY, _("Force A4 output"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PSOptionsSizer->Add( m_forcePSA4OutputOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_PlotOptionsSizer->Add( m_PSOptionsSizer, 0, wxALL|wxEXPAND, 5 );

	m_SizerDXF_options = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("DXF Options") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer5;
	gbSizer5 = new wxGridBagSizer( 5, 5 );
	gbSizer5->SetFlexibleDirection( wxBOTH );
	gbSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_DXF_plotModeOpt = new wxCheckBox( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Plot graphic items using their contours"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXF_plotModeOpt->SetValue(true);
	m_DXF_plotModeOpt->SetToolTip( _("Uncheck to plot graphic items using their center lines") );
	m_DXF_plotModeOpt->SetMinSize( wxSize( 300,-1 ) );

	gbSizer5->Add( m_DXF_plotModeOpt, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	DXF_exportUnitsLabel = new wxStaticText( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Export units:"), wxDefaultPosition, wxDefaultSize, 0 );
	DXF_exportUnitsLabel->Wrap( -1 );
	gbSizer5->Add( DXF_exportUnitsLabel, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_DXF_plotUnitsChoices[] = { _("Inches"), _("Millimeters") };
	int m_DXF_plotUnitsNChoices = sizeof( m_DXF_plotUnitsChoices ) / sizeof( wxString );
	m_DXF_plotUnits = new wxChoice( m_SizerDXF_options->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DXF_plotUnitsNChoices, m_DXF_plotUnitsChoices, 0 );
	m_DXF_plotUnits->SetSelection( 1 );
	m_DXF_plotUnits->SetToolTip( _("The units to use for the exported DXF file") );

	gbSizer5->Add( m_DXF_plotUnits, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_DXF_plotTextStrokeFontOpt = new wxCheckBox( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Use KiCad font to plot text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXF_plotTextStrokeFontOpt->SetToolTip( _("Check to use KiCad stroke font\nUncheck to plot single-line ASCII texts as editable text (using DXF font)") );

	gbSizer5->Add( m_DXF_plotTextStrokeFontOpt, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_DXF_exportAsMultiLayeredFile = new wxCheckBox( m_SizerDXF_options->GetStaticBox(), wxID_ANY, _("Single document"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DXF_exportAsMultiLayeredFile->SetToolTip( _("Export selected layers into a single DXF") );

	gbSizer5->Add( m_DXF_exportAsMultiLayeredFile, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_SizerDXF_options->Add( gbSizer5, 1, wxEXPAND|wxBOTTOM, 5 );


	m_PlotOptionsSizer->Add( m_SizerDXF_options, 0, wxEXPAND|wxALL, 5 );

	m_svgOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("SVG Options") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 5, 0 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	svgPrecisionLabel = new wxStaticText( m_svgOptionsSizer->GetStaticBox(), wxID_ANY, _("Precision:"), wxDefaultPosition, wxDefaultSize, 0 );
	svgPrecisionLabel->Wrap( -1 );
	gbSizer3->Add( svgPrecisionLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_svgPrecsision = new wxSpinCtrl( m_svgOptionsSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 6, 4 );
	m_svgPrecsision->SetToolTip( _("This number defines how many digits are exported that are below 1 mm.\nUser unit is 10^-<N> mm\nChoose 4 if you are not sure.") );

	gbSizer3->Add( m_svgPrecsision, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticText18 = new wxStaticText( m_svgOptionsSizer->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	gbSizer3->Add( m_staticText18, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_SVGColorChoiceChoices[] = { _("Color"), _("Black and white") };
	int m_SVGColorChoiceNChoices = sizeof( m_SVGColorChoiceChoices ) / sizeof( wxString );
	m_SVGColorChoice = new wxChoice( m_svgOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SVGColorChoiceNChoices, m_SVGColorChoiceChoices, 0 );
	m_SVGColorChoice->SetSelection( 0 );
	gbSizer3->Add( m_SVGColorChoice, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SVG_fitPageToBoard = new wxCheckBox( m_svgOptionsSizer->GetStaticBox(), wxID_ANY, _("Fit page to board"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_SVG_fitPageToBoard, wxGBPosition( 2, 0 ), wxGBSpan( 2, 1 ), wxALL, 5 );


	m_svgOptionsSizer->Add( gbSizer3, 1, wxEXPAND|wxBOTTOM, 5 );


	m_PlotOptionsSizer->Add( m_svgOptionsSizer, 1, wxEXPAND|wxALL, 5 );

	m_PDFOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("PDF Options") ), wxHORIZONTAL );

	wxGridBagSizer* gbSizer4;
	gbSizer4 = new wxGridBagSizer( 5, 0 );
	gbSizer4->SetFlexibleDirection( wxBOTH );
	gbSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText19 = new wxStaticText( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Output mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText19->Wrap( -1 );
	gbSizer4->Add( m_staticText19, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_PDFColorChoiceChoices[] = { _("Color"), _("Black and white") };
	int m_PDFColorChoiceNChoices = sizeof( m_PDFColorChoiceChoices ) / sizeof( wxString );
	m_PDFColorChoice = new wxChoice( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PDFColorChoiceNChoices, m_PDFColorChoiceChoices, 0 );
	m_PDFColorChoice->SetSelection( 0 );
	gbSizer4->Add( m_PDFColorChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_frontFPPropertyPopups = new wxCheckBox( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Generate property popups for front footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer4->Add( m_frontFPPropertyPopups, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxRIGHT|wxLEFT, 5 );

	m_backFPPropertyPopups = new wxCheckBox( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Generate property popups for back footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer4->Add( m_backFPPropertyPopups, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxRIGHT|wxLEFT, 5 );

	m_pdfMetadata = new wxCheckBox( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Generate metadata from AUTHOR && SUBJECT variables"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pdfMetadata->SetToolTip( _("Generate PDF document properties from AUTHOR and SUBJECT text variables") );

	gbSizer4->Add( m_pdfMetadata, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT, 5 );

	m_pdfSingle = new wxCheckBox( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Single document"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer4->Add( m_pdfSingle, wxGBPosition( 4, 0 ), wxGBSpan( 1, 2 ), wxLEFT|wxRIGHT, 5 );

	m_pdfBackgroundColorText = new wxStaticText( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, _("Background Color"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pdfBackgroundColorText->Wrap( -1 );
	gbSizer4->Add( m_pdfBackgroundColorText, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALL, 5 );

	m_pdfBackgroundColorSwatch = new COLOR_SWATCH( m_PDFOptionsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer4->Add( m_pdfBackgroundColorSwatch, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );


	m_PDFOptionsSizer->Add( gbSizer4, 1, wxEXPAND|wxBOTTOM, 5 );


	m_PlotOptionsSizer->Add( m_PDFOptionsSizer, 1, wxALL|wxEXPAND, 5 );


	bmiddleSizer->Add( m_PlotOptionsSizer, 0, 0, 6 );


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

	m_DRCExclusionsWarning = new wxStaticText( this, wxID_ANY, _("(%d known DRC violations; %d exclusions)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DRCExclusionsWarning->Wrap( -1 );
	m_sizerButtons->Add( m_DRCExclusionsWarning, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

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

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_openDirButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onOpenOutputDirectory ), NULL, this );
	m_plotDNP->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onDNPCheckbox ), NULL, this );
	m_sketchPadsOnFabLayers->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onSketchPads ), NULL, this );
	m_boardSetup->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_PLOT_BASE::onBoardSetup ), NULL, this );
	m_useGerberX2Format->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnGerberX2Checked ), NULL, this );
	m_DXF_plotModeOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnChangeDXFPlotMode ), NULL, this );
	m_PDFColorChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::onPDFColorChoice ), NULL, this );
	m_buttonDRC->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onRunDRC ), NULL, this );
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );
}

DIALOG_PLOT_BASE::~DIALOG_PLOT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_PLOT_BASE::OnInitDialog ) );
	m_plotFormatOpt->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::SetPlotFormat ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onOutputDirectoryBrowseClicked ), NULL, this );
	m_openDirButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onOpenOutputDirectory ), NULL, this );
	m_plotDNP->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onDNPCheckbox ), NULL, this );
	m_sketchPadsOnFabLayers->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onSketchPads ), NULL, this );
	m_boardSetup->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_PLOT_BASE::onBoardSetup ), NULL, this );
	m_useGerberX2Format->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnGerberX2Checked ), NULL, this );
	m_DXF_plotModeOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::OnChangeDXFPlotMode ), NULL, this );
	m_PDFColorChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_PLOT_BASE::onPDFColorChoice ), NULL, this );
	m_buttonDRC->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::onRunDRC ), NULL, this );
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::CreateDrillFile ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_BASE::Plot ), NULL, this );

}
