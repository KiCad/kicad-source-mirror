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
class TEXT_CTRL_EVAL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <widgets/net_selector.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/simplebook.h>
#include <wx/combobox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/bmpcbox.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_DIALOG_EDIT_PAD 10000
#define wxID_PADNUMCTRL 10001

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainSizer;
		wxNotebook* m_notebook;
		wxPanel* m_panelGeneral;
		wxBoxSizer* m_LeftBoxSizer;
		wxStaticText* m_padTypeLabel;
		wxChoice* m_padType;
		wxStaticText* m_padNumLabel;
		wxTextCtrl* m_padNumCtrl;
		wxStaticText* m_padNetLabel;
		NET_SELECTOR* m_padNetSelector;
		wxStaticText* m_posXLabel;
		wxTextCtrl* m_posXCtrl;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYLabel;
		wxTextCtrl* m_posYCtrl;
		wxStaticText* m_posYUnits;
		wxStaticLine* m_staticline5;
		wxBoxSizer* m_padstackControls;
		wxStaticText* m_staticText891;
		wxChoice* m_cbPadstackMode;
		wxStaticText* m_staticText90;
		wxChoice* m_cbEditLayer;
		wxStaticText* m_shapeLabel;
		wxChoice* m_PadShapeSelector;
		wxSimplebook* m_shapePropsBook;
		wxPanel* m_emptyProps;
		wxPanel* m_trapProps;
		wxFlexGridSizer* fgSizerTrapProps;
		wxStaticText* m_trapDeltaLabel;
		wxTextCtrl* m_trapDeltaCtrl;
		wxStaticText* m_trapDeltaUnits;
		wxStaticText* m_trapAxisLabel;
		wxChoice* m_trapAxisCtrl;
		wxPanel* m_roudingProps;
		wxFlexGridSizer* fgSizerRoundingProps;
		wxStaticText* m_cornerRatioLabel;
		TEXT_CTRL_EVAL* m_cornerRatioCtrl;
		wxStaticText* m_cornerRatioUnits;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxPanel* m_chamferProps;
		wxStaticText* m_chamferRatioLabel;
		TEXT_CTRL_EVAL* m_chamferRatioCtrl;
		wxStaticText* m_chamferRatioUnits;
		wxStaticText* m_staticTextChamferCorner;
		wxCheckBox* m_cbTopLeft;
		wxCheckBox* m_cbTopRight;
		wxCheckBox* m_cbBottomLeft;
		wxCheckBox* m_cbBottomRight;
		wxPanel* m_mixedProps;
		wxStaticText* m_mixedChamferRatioLabel;
		TEXT_CTRL_EVAL* m_mixedChamferRatioCtrl;
		wxStaticText* m_mixedChamferRatioUnits;
		wxStaticText* m_staticTextChamferCorner1;
		wxCheckBox* m_cbTopLeft1;
		wxCheckBox* m_cbTopRight1;
		wxCheckBox* m_cbBottomLeft1;
		wxCheckBox* m_cbBottomRight1;
		wxStaticText* m_mixedCornerRatioLabel;
		TEXT_CTRL_EVAL* m_mixedCornerRatioCtrl;
		wxStaticText* m_mixedCornerRatioUnits;
		wxStaticText* m_sizeXLabel;
		wxTextCtrl* m_sizeXCtrl;
		wxStaticText* m_sizeXUnits;
		wxStaticText* m_sizeYLabel;
		wxTextCtrl* m_sizeYCtrl;
		wxStaticText* m_sizeYUnits;
		wxStaticText* m_PadOrientText;
		wxComboBox* m_cb_padrotation;
		wxStaticText* m_orientationUnits;
		wxCheckBox* m_offsetShapeOpt;
		wxStaticText* m_offsetShapeOptLabel;
		wxFlexGridSizer* m_offsetCtrls;
		wxStaticText* m_offsetXLabel;
		wxTextCtrl* m_offsetXCtrl;
		wxStaticText* m_offsetXUnits;
		wxStaticText* m_offsetYLabel;
		wxTextCtrl* m_offsetYCtrl;
		wxStaticText* m_offsetYUnits;
		wxStaticLine* m_staticline7;
		wxGridBagSizer* m_gbSizerHole;
		wxStaticText* m_holeShapeLabel;
		wxChoice* m_holeShapeCtrl;
		wxStaticText* m_holeXLabel;
		wxTextCtrl* m_holeXCtrl;
		wxStaticText* m_holeXUnits;
		wxStaticText* m_holeYLabel;
		wxTextCtrl* m_holeYCtrl;
		wxStaticText* m_holeYUnits;
		wxStaticLine* m_staticline71;
		wxCheckBox* m_padToDieOpt;
		wxStaticText* m_padToDieLabel;
		wxTextCtrl* m_padToDieCtrl;
		wxStaticText* m_padToDieUnits;
		wxCheckBox* m_padToDieDelayOpt;
		wxStaticText* m_padToDieDelayLabel;
		wxTextCtrl* m_padToDieDelayCtrl;
		wxStaticText* m_padToDieDelayUnits;
		wxBoxSizer* m_middleBoxSizer;
		wxBoxSizer* m_FlippedWarningSizer;
		wxStaticBitmap* m_FlippedWarningIcon;
		wxStaticText* m_staticText86;
		wxStaticText* m_copperLayersLabel;
		wxChoice* m_rbCopperLayersSel;
		wxStaticText* m_techLayersLabel;
		wxStaticLine* m_staticline52;
		wxCheckBox* m_layerFrontAdhesive;
		wxCheckBox* m_layerBackAdhesive;
		wxCheckBox* m_layerFrontPaste;
		wxCheckBox* m_layerBackPaste;
		wxCheckBox* m_layerFrontSilk;
		wxCheckBox* m_layerBackSilk;
		wxCheckBox* m_layerFrontMask;
		wxCheckBox* m_layerBackMask;
		wxCheckBox* m_layerUserDwgs;
		wxCheckBox* m_layerECO1;
		wxCheckBox* m_layerECO2;
		wxStaticText* m_staticTextFabProperty;
		wxChoice* m_choiceFabProperty;
		wxPanel* m_connectionsPanel;
		wxBoxSizer* m_legacyTeardropsWarning;
		wxStaticBitmap* m_legacyTeardropsIcon;
		wxStaticText* m_staticText85;
		wxStaticText* m_staticText851;
		wxCheckBox* m_cbTeardrops;
		wxCheckBox* m_cbPreferZoneConnection;
		wxCheckBox* m_cbTeardropsUseNextTrack;
		wxStaticText* m_stHDRatio;
		wxSpinCtrlDouble* m_spTeardropHDPercent;
		wxStaticText* m_minTrackWidthUnits;
		wxStaticText* m_minTrackWidthHint;
		wxStaticText* m_staticText87;
		wxStaticText* m_teardropShapeLabel;
		wxStaticLine* m_staticline51;
		wxStaticBitmap* m_bitmapTeardrop;
		wxStaticText* m_stHsetting;
		wxSpinCtrlDouble* m_spTeardropLenPercent;
		wxStaticText* m_stLenPercentUnits;
		wxStaticText* m_stLenPercentHint;
		wxStaticText* m_staticText88;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stMaxLenUnits;
		wxStaticText* m_stVsetting;
		wxSpinCtrlDouble* m_spTeardropSizePercent;
		wxStaticText* m_stWidthPercentUnits;
		wxStaticText* m_stWidthPercentHint;
		wxStaticText* m_staticText89;
		wxStaticText* m_stTdMaxSize;
		wxTextCtrl* m_tcMaxHeight;
		wxStaticText* m_stMaxHeightUnits;
		wxCheckBox* m_curvedEdges;
		wxStaticBoxSizer* m_sbSizerZonesSettings;
		wxStaticText* m_padConnectionLabel;
		wxChoice* m_ZoneConnectionChoice;
		wxStaticText* m_zoneKnockoutLabel;
		wxChoice* m_ZoneCustomPadShape;
		wxStaticText* m_thermalGapLabel;
		wxTextCtrl* m_thermalGapCtrl;
		wxStaticText* m_thermalGapUnits;
		wxStaticText* m_spokeWidthLabel;
		wxTextCtrl* m_spokeWidthCtrl;
		wxStaticText* m_spokeWidthUnits;
		wxStaticText* m_spokeAngleLabel;
		wxTextCtrl* m_spokeAngleCtrl;
		wxStaticText* m_spokeAngleUnits;
		wxPanel* m_localSettingsPanel;
		wxStaticText* m_staticTextInfoPosValue;
		wxStaticText* m_staticTextInfoNegVal;
		wxStaticText* m_clearanceLabel;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticText* m_maskMarginLabel;
		wxTextCtrl* m_maskMarginCtrl;
		wxStaticText* m_maskMarginUnits;
		wxStaticText* m_pasteMarginLabel;
		wxTextCtrl* m_pasteMarginCtrl;
		wxStaticText* m_pasteMarginUnits;
		wxStaticText* m_pasteMarginRatioLabel;
		wxTextCtrl* m_pasteMarginRatioCtrl;
		wxStaticText* m_pasteMarginRatioUnits;
		wxSimplebook* m_nonCopperWarningBook;
		wxStaticText* m_nonCopperNote;
		wxStaticText* m_staticTextInfoPaste;
		wxStaticBitmap* m_nonCopperWarningIcon;
		wxStaticText* m_nonCopperWarningText;
		wxPanel* m_backDrillPanel;
		wxStaticText* m_topPostMachiningLabel;
		wxChoice* m_topPostMachining;
		wxStaticText* m_topPostMachineSize1Label;
		wxTextCtrl* m_topPostmachineSize1;
		wxStaticText* m_topPostMachineSize1Units;
		wxStaticText* m_topPostMachineSize2Label;
		wxTextCtrl* m_topPostMachineSize2;
		wxStaticText* m_topPostMachineSize2Units;
		wxStaticText* m_bottomPostMachiningLabel;
		wxChoice* m_bottomPostMachining;
		wxStaticText* m_bottomPostMachineSize1Label;
		wxTextCtrl* m_bottomPostMachineSize1;
		wxStaticText* m_bottomPostMachineSize1Units;
		wxStaticText* m_bottomPostMachineSize2Label;
		wxTextCtrl* m_bottomPostMachineSize2;
		wxStaticText* m_bottomPostMachineSize2Units;
		wxChoice* m_backDrillChoice;
		wxStaticText* m_backDrillTopLayerLabel;
		wxBitmapComboBox* m_backDrillTopLayer;
		wxStaticText* m_backDrillTopSizeLabel;
		wxTextCtrl* m_backDrillTopSize;
		wxStaticText* m_backDrillTopSizeUnits;
		wxStaticText* m_backDrillBottomLayerLabel;
		wxBitmapComboBox* m_backDrillBottomLayer;
		wxStaticText* m_backDrillBottomSizeLabel;
		wxTextCtrl* m_backDrillBottomSize;
		wxStaticText* m_backDrillBottomSizeUnits;
		wxSimplebook* m_stackupImagesBook;
		wxPanel* page0;
		wxStaticBitmap* m_stackupImage0;
		wxPanel* page1;
		wxStaticBitmap* m_stackupImage1;
		wxPanel* page2;
		wxStaticBitmap* m_stackupImage2;
		wxPanel* page3;
		wxPanel* page4;
		wxStaticBitmap* m_stackupImage4;
		wxPanel* page5;
		wxStaticBitmap* m_stackupImage5;
		wxPanel* page6;
		wxStaticBitmap* m_stackupImage6;
		wxPanel* page7;
		wxStaticBitmap* m_stackupImage7;
		wxPanel* m_boardViewPanel;
		wxBoxSizer* m_padPreviewSizer;
		wxStaticText* m_parentInfo;
		wxCheckBox* m_cbShowPadOutline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadstackModeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditLayerChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadShapeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerSizePercentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerRadiusChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOffsetCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadToDieCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadToDieDelayCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetCopperLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModify( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTeardropsUpdateUi( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onModify( wxSpinDoubleEvent& event ) { event.Skip(); }
		virtual void OnUpdateUINonCopperWarning( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onTopPostMachining( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBottomPostMachining( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBackDrillChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onChangePadMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Pad Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PAD_PROPERTIES_BASE();

};

