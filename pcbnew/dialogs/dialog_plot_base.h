///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
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
#include <wx/gbsizer.h>
#include <wx/statbmp.h>
#include <wx/hyperlink.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLOT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLOT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			ID_PRINT_REF = 1000,
			ID_ALLOW_PRINT_PAD_ON_SILKSCREEN,
			ID_MIROR_OPT,
			ID_LAYER_FAB,
			ID_SELECT_COPPER_LAYERS,
			ID_DESELECT_COPPER_LAYERS,
			ID_SELECT_ALL_LAYERS,
			ID_DESELECT_ALL_LAYERS
		};

		wxBoxSizer* m_MainSizer;
		wxStaticText* m_staticTextPlotFmt;
		wxChoice* m_plotFormatOpt;
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxBitmapButton* m_browseButton;
		wxStaticBoxSizer* m_LayersSizer;
		wxCheckListBox* m_layerCheckListBox;
		wxBoxSizer* m_PlotOptionsSizer;
		wxCheckBox* m_plotSheetRef;
		wxCheckBox* m_plotModuleValueOpt;
		wxCheckBox* m_plotModuleRefOpt;
		wxCheckBox* m_plotInvisibleText;
		wxCheckBox* m_includeEdgeLayerOpt;
		wxCheckBox* m_sketchPadsOnFabLayers;
		wxCheckBox* m_plotNoViaOnMaskOpt;
		wxCheckBox* m_useAuxOriginCheckBox;
		wxStaticText* drillMarksLabel;
		wxChoice* m_drillShapeOpt;
		wxStaticText* scalingLabel;
		wxChoice* m_scaleOpt;
		wxStaticText* plotModeLabel;
		wxChoice* m_plotModeOpt;
		wxCheckBox* m_plotMirrorOpt;
		wxCheckBox* m_plotPSNegativeOpt;
		wxCheckBox* m_zoneFillCheck;
		wxBoxSizer* m_SizerSolderMaskAlert;
		wxStaticBitmap* m_bitmapAlert;
		wxStaticText* m_staticTextAlert;
		wxStaticText* m_staticTextAlert1;
		wxHyperlinkCtrl* m_boardSetup;
		wxStaticBoxSizer* m_GerberOptionsSizer;
		wxCheckBox* m_useGerberExtensions;
		wxCheckBox* m_generateGerberJobFile;
		wxCheckBox* m_subtractMaskFromSilk;
		wxStaticText* coordFormatLabel;
		wxChoice* m_coordFormatCtrl;
		wxCheckBox* m_useGerberX2Format;
		wxCheckBox* m_useGerberNetAttributes;
		wxCheckBox* m_disableApertMacros;
		wxStaticBoxSizer* m_HPGLOptionsSizer;
		wxStaticText* m_hpglPenLabel;
		wxTextCtrl* m_hpglPenCtrl;
		wxStaticText* m_hpglPenUnits;
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
		wxCheckBox* m_DXF_plotTextStrokeFontOpt;
		wxStaticText* DXF_exportUnitsLabel;
		wxChoice* m_DXF_plotUnits;
		wxStaticBoxSizer* m_svgOptionsSizer;
		wxStaticText* svgUnitLabel;
		wxChoice* m_svgUnits;
		wxStaticText* svgPrecisionLabel;
		wxSpinCtrl* m_svgPrecsision;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_buttonDRC;
		wxStaticText* m_DRCExclusionsWarning;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;
		wxMenu* m_popMenu;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnRightClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void SetPlotFormat( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetScaleOpt( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBoardSetup( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnGerberX2Checked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeDXFPlotMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRunDRC( wxCommandEvent& event ) { event.Skip(); }
		virtual void CreateDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void Plot( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPopUpLayers( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PLOT_BASE();

		void DIALOG_PLOT_BASEOnContextMenu( wxMouseEvent &event )
		{
			this->PopupMenu( m_popMenu, event.GetPosition() );
		}

};

