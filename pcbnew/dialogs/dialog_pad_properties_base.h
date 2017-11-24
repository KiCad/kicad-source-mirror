///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 17 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PAD_PROPERTIES_BASE_H__
#define __DIALOG_PAD_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class TEXT_CTRL_EVAL;
class wxListView;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wxBasePcbFrame.h>
#include <pcb_draw_panel_gal.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>

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
			wxID_PADNUMCTRL,
			wxID_PADNETNAMECTRL
		};
		
		wxNotebook* m_notebook;
		wxPanel* m_panelGeneral;
		wxStaticText* m_PadNumText;
		wxTextCtrl* m_PadNumCtrl;
		wxStaticText* m_PadNameText;
		wxTextCtrl* m_PadNetNameCtrl;
		wxStaticText* m_staticText44;
		wxChoice* m_PadType;
		wxStaticText* m_staticText45;
		wxChoice* m_PadShape;
		wxStaticText* m_staticText4;
		TEXT_CTRL_EVAL* m_PadPosition_X_Ctrl;
		wxStaticText* m_PadPosX_Unit;
		wxStaticText* m_staticText41;
		TEXT_CTRL_EVAL* m_PadPosition_Y_Ctrl;
		wxStaticText* m_PadPosY_Unit;
		wxStaticText* m_staticText12;
		TEXT_CTRL_EVAL* m_ShapeSize_X_Ctrl;
		wxStaticText* m_PadShapeSizeX_Unit;
		wxStaticText* m_staticText15;
		TEXT_CTRL_EVAL* m_ShapeSize_Y_Ctrl;
		wxStaticText* m_PadShapeSizeY_Unit;
		wxStaticText* m_PadOrientText;
		wxChoice* m_PadOrient;
		wxStaticText* m_staticText491;
		TEXT_CTRL_EVAL* m_PadOrientCtrl;
		wxStaticText* m_customOrientUnits;
		wxStaticText* m_staticText17;
		TEXT_CTRL_EVAL* m_ShapeOffset_X_Ctrl;
		wxStaticText* m_PadShapeOffsetX_Unit;
		wxStaticText* m_staticText19;
		TEXT_CTRL_EVAL* m_ShapeOffset_Y_Ctrl;
		wxStaticText* m_PadShapeOffsetY_Unit;
		wxStaticText* m_staticText38;
		TEXT_CTRL_EVAL* m_LengthPadToDieCtrl;
		wxStaticText* m_PadLengthDie_Unit;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticLine* m_staticline6;
		wxStaticText* m_staticText21;
		TEXT_CTRL_EVAL* m_ShapeDelta_Ctrl;
		wxStaticText* m_PadShapeDelta_Unit;
		wxStaticText* m_staticText23;
		wxChoice* m_trapDeltaDirChoice;
		wxStaticLine* m_staticline7;
		wxStaticLine* m_staticline8;
		wxStaticLine* m_staticline9;
		wxStaticText* m_staticTextCornerSizeRatio;
		TEXT_CTRL_EVAL* m_tcCornerSizeRatio;
		wxStaticText* m_staticTextCornerSizeRatioUnit;
		wxStaticText* m_staticTextCornerRadius;
		wxStaticText* m_staticTextCornerRadiusValue;
		wxStaticText* m_staticTextCornerSizeUnit;
		wxStaticText* m_staticText47;
		wxChoice* m_DrillShapeCtrl;
		wxStaticText* m_staticText51;
		wxStaticText* m_textPadDrillX;
		TEXT_CTRL_EVAL* m_PadDrill_X_Ctrl;
		wxStaticText* m_PadDrill_X_Unit;
		wxStaticText* m_textPadDrillY;
		TEXT_CTRL_EVAL* m_PadDrill_Y_Ctrl;
		wxStaticText* m_PadDrill_Y_Unit;
		wxStaticText* m_staticText511;
		wxChoice* m_rbCopperLayersSel;
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
		wxStaticText* m_staticTitleModuleRot;
		wxStaticText* m_staticModuleRotValue;
		wxStaticText* m_staticTitleModuleSide;
		wxStaticText* m_staticModuleSideValue;
		wxPanel* m_localSettingsPanel;
		wxStaticText* m_staticTextNetClearance;
		TEXT_CTRL_EVAL* m_NetClearanceValueCtrl;
		wxStaticText* m_NetClearanceUnits;
		wxStaticText* m_MaskClearanceTitle;
		TEXT_CTRL_EVAL* m_SolderMaskMarginCtrl;
		wxStaticText* m_SolderMaskMarginUnits;
		wxStaticText* m_staticTextSolderPaste;
		TEXT_CTRL_EVAL* m_SolderPasteMarginCtrl;
		wxStaticText* m_SolderPasteMarginUnits;
		wxStaticText* m_staticTextRatio;
		TEXT_CTRL_EVAL* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxStaticBoxSizer* m_sbSizerZonesSettings;
		wxStaticText* m_staticText40;
		wxChoice* m_ZoneConnectionChoice;
		wxStaticText* m_staticText49;
		TEXT_CTRL_EVAL* m_ThermalWidthCtrl;
		wxStaticText* m_ThermalWidthUnits;
		wxStaticText* m_staticText52;
		TEXT_CTRL_EVAL* m_ThermalGapCtrl;
		wxStaticText* m_ThermalGapUnits;
		wxStaticText* m_staticTextcps;
		wxChoice* m_ZoneCustomPadShape;
		wxStaticText* m_staticTextWarning;
		wxPanel* m_panelCustomShapePrimitives;
		wxBoxSizer* m_bSizerPanelPrimitives;
		wxStaticText* m_staticTextPrimitivesList;
		wxStaticText* m_staticTextPrimitiveListWarning;
		wxListView* m_listCtrlPrimitives;
		wxButton* m_buttonDel;
		wxButton* m_buttonEditShape;
		wxButton* m_buttonAddShape;
		wxButton* m_buttonDup;
		wxButton* m_buttonGeometry;
		wxButton* m_buttonImport;
		wxPanel* m_panelShowPad;
		PCB_DRAW_PANEL_GAL* m_panelShowPadGal;
		KIGFX::GAL_DISPLAY_OPTIONS m_galOptions;
		wxStaticText* m_staticTextWarningPadFlipped;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadShapeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerSizePercentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPrimitiveDClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnPrimitiveSelection( wxListEvent& event ) { event.Skip(); }
		virtual void onDeletePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onAddPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDuplicatePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGeometryTransform( wxCommandEvent& event ) { event.Skip(); }
		virtual void onImportPrimitives( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintShowPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Pad Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 785,659 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSUNKEN_BORDER ); 
		~DIALOG_PAD_PROPERTIES_BASE();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextInfo;
		wxStaticText* m_staticTextPosStart;
		wxStaticText* m_staticTextStartX;
		TEXT_CTRL_EVAL* m_textCtrPosX;
		wxStaticText* m_staticTextStartY;
		TEXT_CTRL_EVAL* m_textCtrPosY;
		wxStaticText* m_staticTextPosUnit;
		wxStaticText* m_staticTextPosEnd;
		wxStaticText* m_staticTextEndX;
		TEXT_CTRL_EVAL* m_textCtrEndX;
		wxStaticText* m_staticTextEndY;
		TEXT_CTRL_EVAL* m_textCtrEndY;
		wxStaticText* m_staticTextEndUnit;
		wxStaticText* m_staticTextAngle;
		TEXT_CTRL_EVAL* m_textCtrAngle;
		wxStaticText* m_staticTextAngleUnit;
		wxStaticText* m_staticTextThickness;
		wxTextCtrl* m_textCtrlThickness;
		wxStaticText* m_staticTextThicknessUnit;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 561,243 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
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
		wxStaticText* m_staticTextMoveX;
		TEXT_CTRL_EVAL* m_textCtrMoveX;
		wxStaticText* m_staticTextMoveY;
		TEXT_CTRL_EVAL* m_textCtrMoveY;
		wxStaticText* m_staticTextMoveUnit;
		wxStaticText* m_staticTextAngle;
		TEXT_CTRL_EVAL* m_textCtrAngle;
		wxStaticText* m_staticTextAngleUnit;
		wxStaticText* m_staticTextSF;
		TEXT_CTRL_EVAL* m_textCtrlScalingFactor;
		wxStaticText* m_staticTextDupCnt;
		wxSpinCtrl* m_spinCtrlDuplicateCount;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad Custom Shape Geometry Transform"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 472,208 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextCornerListWarning;
		wxStaticText* m_staticTextValidate;
		wxGrid* m_gridCornersList;
		wxButton* m_buttonAdd;
		wxButton* m_buttonDelete;
		wxPanel* m_panelPoly;
		wxStaticText* m_staticTextThickness;
		TEXT_CTRL_EVAL* m_textCtrlThickness;
		wxStaticText* m_staticTextThicknessUnit;
		wxStaticText* m_staticTextInfo;
		wxStaticLine* m_staticline3;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onGridSelect( wxGridRangeSelectEvent& event ) { event.Skip(); }
		virtual void onCellSelect( wxGridEvent& event ) { event.Skip(); }
		virtual void onButtonAdd( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPaintPolyPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void onPolyPanelResize( wxSizeEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Basic Shape Polygon"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 503,396 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE();
	
};

#endif //__DIALOG_PAD_PROPERTIES_BASE_H__
