///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class COLOR_SWATCH;
class STD_BITMAP_BUTTON;
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checklst.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/gbsizer.h>
#include <wx/statbmp.h>
#include <wx/hyperlink.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_ALLOW_PRINT_PAD_ON_SILKSCREEN 6000
#define ID_MIROR_OPT 6001

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLOT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLOT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainSizer;
		wxStaticText* m_staticTextPlotFmt;
		wxChoice* m_plotFormatOpt;
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		STD_BITMAP_BUTTON* m_browseButton;
		STD_BITMAP_BUTTON* m_openDirButton;
		wxBoxSizer* bmiddleSizer;
		wxStaticBoxSizer* m_LayersSizer;
		wxCheckListBox* m_layerCheckListBox;
		wxBoxSizer* m_PlotOptionsSizer;
		wxCheckBox* m_plotSheetRef;
		wxCheckBox* m_subtractMaskFromSilk;
		wxCheckBox* m_plotDNP;
		wxRadioButton* m_hideDNP;
		wxRadioButton* m_crossoutDNP;
		wxCheckBox* m_sketchPadsOnFabLayers;
		wxCheckBox* m_plotPadNumbers;
		wxCheckBox* m_zoneFillCheck;
		wxStaticText* drillMarksLabel;
		wxChoice* m_drillShapeOpt;
		wxStaticText* scalingLabel;
		wxChoice* m_scaleOpt;
		wxCheckBox* m_useAuxOriginCheckBox;
		wxCheckBox* m_plotMirrorOpt;
		wxCheckBox* m_plotPSNegativeOpt;
		wxBoxSizer* m_SizerSolderMaskAlert;
		wxStaticBitmap* m_bitmapAlert;
		wxStaticText* m_staticTextAlert;
		wxStaticText* m_staticTextAlert1;
		wxHyperlinkCtrl* m_boardSetup;
		wxStaticBoxSizer* m_GerberOptionsSizer;
		wxCheckBox* m_useGerberExtensions;
		wxCheckBox* m_generateGerberJobFile;
		wxStaticText* coordFormatLabel;
		wxChoice* m_coordFormatCtrl;
		wxCheckBox* m_useGerberX2Format;
		wxCheckBox* m_useGerberNetAttributes;
		wxCheckBox* m_disableApertMacros;
		wxStaticBoxSizer* m_PSOptionsSizer;
		wxStaticText* m_fineAdjustXLabel;
		wxTextCtrl* m_fineAdjustXCtrl;
		wxStaticText* m_fineAdjustYLabel;
		wxTextCtrl* m_fineAdjustYCtrl;
		wxStaticText* m_widthAdjustLabel;
		wxTextCtrl* m_widthAdjustCtrl;
		wxStaticText* m_widthAdjustUnits;
		wxCheckBox* m_forcePSA4OutputOpt;
		wxStaticBoxSizer* m_SizerDXF_options;
		wxCheckBox* m_DXF_plotModeOpt;
		wxStaticText* DXF_exportUnitsLabel;
		wxChoice* m_DXF_plotUnits;
		wxCheckBox* m_DXF_plotTextStrokeFontOpt;
		wxCheckBox* m_DXF_exportAsMultiLayeredFile;
		wxStaticBoxSizer* m_svgOptionsSizer;
		wxStaticText* svgPrecisionLabel;
		wxSpinCtrl* m_svgPrecsision;
		wxStaticText* m_staticText18;
		wxChoice* m_SVGColorChoice;
		wxCheckBox* m_SVG_fitPageToBoard;
		wxStaticBoxSizer* m_PDFOptionsSizer;
		wxStaticText* m_staticText19;
		wxChoice* m_PDFColorChoice;
		wxCheckBox* m_frontFPPropertyPopups;
		wxCheckBox* m_backFPPropertyPopups;
		wxCheckBox* m_pdfMetadata;
		wxCheckBox* m_pdfSingle;
		wxStaticText* m_pdfBackgroundColorText;
		COLOR_SWATCH* m_pdfBackgroundColorSwatch;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_buttonDRC;
		wxStaticText* m_DRCExclusionsWarning;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void SetPlotFormat( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOpenOutputDirectory( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDNPCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSketchPads( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBoardSetup( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnGerberX2Checked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeDXFPlotMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPDFColorChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRunDRC( wxCommandEvent& event ) { event.Skip(); }
		virtual void CreateDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void Plot( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PLOT_BASE();

};

