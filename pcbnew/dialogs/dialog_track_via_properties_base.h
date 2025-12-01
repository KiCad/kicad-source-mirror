///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <widgets/net_selector.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/bmpcbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TRACK_VIA_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TRACK_VIA_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
		wxStaticText* m_stLenPercentUnits;
		wxStaticText* m_stLenPercentHint;
		wxStaticText* m_stLenPercentSuffix;
		wxStaticText* m_stWidthPercentUnits;
		wxStaticText* m_stWidthPercentHint;
		wxStaticText* m_stWidthPercentSuffix;

	protected:
		wxBoxSizer* m_MainSizer;
		wxStaticBoxSizer* m_sbCommonSizer;
		wxStaticText* m_netSelectorLabel;
		NET_SELECTOR* m_netSelector;
		wxCheckBox* m_viaNotFree;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_lockedCbox;
		wxStaticBoxSizer* m_sbTrackSizer;
		wxStaticText* m_TrackStartXLabel;
		wxTextCtrl* m_TrackStartXCtrl;
		wxStaticText* m_TrackStartYLabel;
		wxTextCtrl* m_TrackStartYCtrl;
		wxStaticText* m_TrackStartYUnit;
		wxStaticText* m_TrackEndXLabel;
		wxTextCtrl* m_TrackEndXCtrl;
		wxStaticText* m_TrackEndYLabel;
		wxTextCtrl* m_TrackEndYCtrl;
		wxStaticText* m_TrackEndYUnit;
		wxStaticText* m_predefinedTrackWidthsLabel;
		wxChoice* m_predefinedTrackWidthsCtrl;
		wxStaticText* m_predefinedTrackWidthsUnits;
		wxStaticText* m_TrackWidthLabel;
		wxTextCtrl* m_TrackWidthCtrl;
		wxStaticText* m_TrackWidthUnit;
		wxStaticText* m_TrackLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_TrackLayerCtrl;
		wxStaticText* m_techLayersLabel;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_trackHasSolderMask;
		wxStaticText* m_trackMaskMarginLabel;
		wxTextCtrl* m_trackMaskMarginCtrl;
		wxStaticText* m_trackMaskMarginUnit;
		wxStaticBoxSizer* m_sbViaSizer;
		wxStaticText* m_ViaXLabel;
		wxTextCtrl* m_ViaXCtrl;
		wxStaticText* m_ViaYLabel;
		wxTextCtrl* m_ViaYCtrl;
		wxStaticText* m_ViaYUnit;
		wxStaticText* m_predefinedViaSizesLabel;
		wxChoice* m_predefinedViaSizesCtrl;
		wxStaticText* m_predefinedViaSizesUnits;
		wxBoxSizer* m_sbPadstackSettings;
		wxStaticText* m_stPadstackMode;
		wxChoice* m_cbPadstackMode;
		wxStaticText* m_stEditLayer;
		wxChoice* m_cbEditLayer;
		wxStaticText* m_ViaDiameterLabel;
		wxTextCtrl* m_ViaDiameterCtrl;
		wxStaticText* m_ViaDiameterUnit;
		wxStaticText* m_ViaDrillLabel;
		wxTextCtrl* m_ViaDrillCtrl;
		wxStaticText* m_ViaDrillUnit;
		wxStaticText* m_ViaTypeLabel;
		wxChoice* m_ViaTypeChoice;
		wxStaticText* m_ViaStartLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_ViaStartLayer;
		wxStaticText* m_ViaEndLayerLabel1;
		PCB_LAYER_BOX_SELECTOR* m_ViaEndLayer;
		wxStaticText* m_annularRingsLabel;
		wxChoice* m_annularRingsCtrl;
		wxStaticText* m_protectionPresetsLabel;
		wxChoice* m_protectionFeatures;
		wxStaticText* m_backDrillLabel;
		wxStaticLine* m_staticline2;
		wxChoice* m_backDrillChoice;
		wxStaticText* m_backDrillFrontLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_backDrillFrontLayer;
		wxStaticText* m_backDrillBackLayer;
		PCB_LAYER_BOX_SELECTOR* m_ViaStartLayer11;
		wxStaticText* m_postMachineSectionLabel;
		wxStaticLine* m_staticline21;
		wxStaticText* m_topPostMachineLabel;
		wxChoice* m_topPostMachine;
		wxStaticText* m_topPostMachineSize1Label;
		wxTextCtrl* m_topPostMachineSize1;
		wxStaticText* m_topPostMachineSize1Units;
		wxStaticText* m_topPostMachineSize2Label;
		wxTextCtrl* m_topPostMachineSize2;
		wxStaticText* m_topPostMachineSize2Units;
		wxStaticText* m_bottomPostMachineLabel;
		wxChoice* m_bottomPostMachine;
		wxStaticText* m_bottomPostMachineSize1Label;
		wxTextCtrl* m_bottomPostMachineSize1;
		wxStaticText* m_bottomPostMachineSize1Units;
		wxStaticText* m_bottomPostMachineSize2Label;
		wxTextCtrl* m_bottomPostMachineSize2;
		wxStaticText* m_bottomPostMachineSize2Units;
		wxStaticLine* m_staticline4;
		wxBoxSizer* m_legacyTeardropsWarning;
		wxStaticBitmap* m_legacyTeardropsIcon;
		wxStaticText* m_staticText85;
		wxStaticText* m_staticText851;
		wxCheckBox* m_cbTeardrops;
		wxCheckBox* m_cbTeardropsUseNextTrack;
		wxStaticText* m_stHDRatio;
		wxTextCtrl* m_tcHDRatio;
		wxStaticText* m_stHDRatioUnits;
		wxStaticText* m_minTrackWidthHint;
		wxStaticBitmap* m_bitmapTeardrop;
		wxStaticText* m_stLenPercentLabel;
		wxTextCtrl* m_tcLenPercent;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stMaxLenUnits;
		wxStaticText* m_stWidthPercentLabel;
		wxTextCtrl* m_tcWidthPercent;
		wxStaticText* m_stMaxWidthLabel;
		wxTextCtrl* m_tcMaxWidth;
		wxStaticText* m_stMaxWidthUnits;
		wxCheckBox* m_curvedEdges;
		wxStdDialogButtonSizer* m_StdButtons;
		wxButton* m_StdButtonsOK;
		wxButton* m_StdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onViaNotFreeClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthEdit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTrackEdit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPadstackModeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditLayerChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaEdit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBackdrillChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTopPostMachineChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBottomPostMachineChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTeardropsUpdateUi( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Track & Via Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~DIALOG_TRACK_VIA_PROPERTIES_BASE();

};

