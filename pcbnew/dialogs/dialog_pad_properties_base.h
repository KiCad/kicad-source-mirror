///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class TEXT_CTRL_EVAL;
class WX_GRID;

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
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
#include <wx/combobox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_DIALOG_EDIT_PAD = 1000,
			wxID_PADNUMCTRL
		};

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
		wxCheckBox* m_locked;
		wxStaticLine* m_staticline5;
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
		wxStaticLine* m_staticline6;
		wxStaticText* m_holeShapeLabel;
		wxChoice* m_holeShapeCtrl;
		wxStaticText* m_holeXLabel;
		wxTextCtrl* m_holeXCtrl;
		wxStaticText* m_holeXUnits;
		wxStaticText* m_holeYLabel;
		wxTextCtrl* m_holeYCtrl;
		wxStaticText* m_holeYUnits;
		wxStaticLine* m_staticline7;
		wxCheckBox* m_offsetShapeOpt;
		wxStaticText* m_offsetShapeOptLabel;
		wxFlexGridSizer* m_offsetCtrls;
		wxStaticText* m_offsetXLabel;
		wxTextCtrl* m_offsetXCtrl;
		wxStaticText* m_offsetXUnits;
		wxStaticText* m_offsetYLabel;
		wxTextCtrl* m_offsetYCtrl;
		wxStaticText* m_offsetYUnits;
		wxCheckBox* m_padToDieOpt;
		wxStaticText* m_padToDieLabel;
		wxTextCtrl* m_padToDieCtrl;
		wxStaticText* m_padToDieUnits;
		wxBoxSizer* m_middleBoxSizer;
		wxBoxSizer* m_FlippedWarningSizer;
		wxStaticBitmap* m_FlippedWarningIcon;
		wxStaticText* m_staticText86;
		wxStaticText* m_copperLayersLabel;
		wxChoice* m_rbCopperLayersSel;
		wxStaticText* m_techLayersLabel;
		wxCheckBox* m_PadLayerAdhCmp;
		wxCheckBox* m_PadLayerAdhCu;
		wxCheckBox* m_PadLayerPateCmp;
		wxCheckBox* m_PadLayerPateCu;
		wxCheckBox* m_PadLayerSilkCmp;
		wxCheckBox* m_PadLayerSilkCu;
		wxCheckBox* m_PadLayerMaskCmp;
		wxCheckBox* m_PadLayerMaskCu;
		wxCheckBox* m_PadLayerDraft;
		wxCheckBox* m_PadLayerECO1;
		wxCheckBox* m_PadLayerECO2;
		wxStaticText* m_staticTextFabProperty;
		wxChoice* m_choiceFabProperty;
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
		TEXT_CTRL_EVAL* m_pasteMarginRatioCtrl;
		wxStaticText* m_pasteMarginRatioUnits;
		wxSimplebook* m_nonCopperWarningBook;
		wxStaticText* m_nonCopperNote;
		wxStaticText* m_staticTextInfoPaste;
		wxStaticBitmap* m_nonCopperWarningIcon;
		wxStaticText* m_nonCopperWarningText;
		wxStaticBoxSizer* m_sbSizerZonesSettings;
		wxStaticText* m_staticText40;
		wxChoice* m_ZoneConnectionChoice;
		wxStaticText* m_spokeWidthLabel;
		wxTextCtrl* m_spokeWidthCtrl;
		wxStaticText* m_spokeWidthUnits;
		wxStaticText* m_thermalGapLabel;
		wxTextCtrl* m_thermalGapCtrl;
		wxStaticText* m_thermalGapUnits;
		wxStaticText* m_staticTextcps;
		wxChoice* m_ZoneCustomPadShape;
		wxPanel* m_panelCustomShapePrimitives;
		wxBoxSizer* m_bSizerPanelPrimitives;
		wxStaticText* m_staticTextPrimitivesList;
		wxStaticText* m_staticTextPrimitiveListWarning;
		wxListView* m_listCtrlPrimitives;
		wxButton* m_buttonAddShape;
		wxButton* m_buttonEditShape;
		wxButton* m_buttonDup;
		wxButton* m_buttonGeometry;
		wxButton* m_buttonDel;
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

		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadShapeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerSizePercentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerRadiusChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOffsetCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadToDieCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetCopperLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUINonCopperWarning( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPrimitiveDClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnPrimitiveSelection( wxListEvent& event ) { event.Skip(); }
		virtual void onAddPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDuplicatePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGeometryTransform( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeletePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onChangePadMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Pad Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PAD_PROPERTIES_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPosStart;
		wxStaticText* m_startXLabel;
		TEXT_CTRL_EVAL* m_startXCtrl;
		wxStaticText* m_startXUnits;
		wxStaticText* m_startYLabel;
		TEXT_CTRL_EVAL* m_startYCtrl;
		wxStaticText* m_startYUnits;
		wxStaticText* m_staticTextPosCtrl1;
		wxStaticText* m_ctrl1XLabel;
		TEXT_CTRL_EVAL* m_ctrl1XCtrl;
		wxStaticText* m_ctrl1XUnits;
		wxStaticText* m_ctrl1YLabel;
		TEXT_CTRL_EVAL* m_ctrl1YCtrl;
		wxStaticText* m_ctrl1YUnits;
		wxStaticText* m_staticTextPosCtrl2;
		wxStaticText* m_ctrl2XLabel;
		TEXT_CTRL_EVAL* m_ctrl2XCtrl;
		wxStaticText* m_ctrl2XUnits;
		wxStaticText* m_ctrl2YLabel;
		TEXT_CTRL_EVAL* m_ctrl2YCtrl;
		wxStaticText* m_ctrl2YUnits;
		wxStaticText* m_staticTextPosEnd;
		wxStaticText* m_endXLabel;
		TEXT_CTRL_EVAL* m_endXCtrl;
		wxStaticText* m_endXUnits;
		wxStaticText* m_endYLabel;
		TEXT_CTRL_EVAL* m_endYCtrl;
		wxStaticText* m_endYUnits;
		wxStaticText* m_radiusLabel;
		TEXT_CTRL_EVAL* m_radiusCtrl;
		wxStaticText* m_radiusUnits;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxCheckBox* m_filledCtrl;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextMove;
		wxStaticText* m_xLabel;
		TEXT_CTRL_EVAL* m_xCtrl;
		wxStaticText* m_xUnits;
		wxStaticText* m_yLabel;
		TEXT_CTRL_EVAL* m_yCtrl;
		wxStaticText* m_yUnits;
		wxStaticText* m_rotationLabel;
		TEXT_CTRL_EVAL* m_rotationCtrl;
		wxStaticText* m_rotationUnits;
		wxStaticText* m_scaleLabel;
		TEXT_CTRL_EVAL* m_scaleCtrl;
		wxStaticText* m_staticTextDupCnt;
		wxSpinCtrl* m_spinCtrlDuplicateCount;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad Custom Shape Geometry Transform"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_GRID* m_gridCornersList;
		wxBitmapButton* m_addButton;
		wxBitmapButton* m_deleteButton;
		wxStaticText* m_thicknessLabel;
		TEXT_CTRL_EVAL* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxCheckBox* m_filledCtrl;
		wxPanel* m_panelPoly;
		wxStaticBitmap* m_warningIcon;
		wxStaticText* m_warningText;
		wxStaticLine* m_staticline3;
		wxStaticText* m_statusLine1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onGridSelect( wxGridRangeSelectEvent& event ) { event.Skip(); }
		virtual void onCellSelect( wxGridEvent& event ) { event.Skip(); }
		virtual void OnButtonAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPaintPolyPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void onPolyPanelResize( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Basic Shape Polygon"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE();

};

