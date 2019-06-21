///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PAD_PROPERTIES_BASE_H__
#define __DIALOG_PAD_PROPERTIES_BASE_H__

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
#include <wx/textctrl.h>
#include <widgets/net_selector.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <wx/statline.h>
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
		
		wxNotebook* m_notebook;
		wxPanel* m_panelGeneral;
		wxStaticText* m_PadNumText;
		wxTextCtrl* m_PadNumCtrl;
		wxStaticText* m_PadNameText;
		NET_SELECTOR* m_PadNetSelector; 
		wxStaticText* m_staticText44;
		wxChoice* m_PadType;
		wxStaticText* m_staticText45;
		wxChoice* m_PadShape;
		wxStaticText* m_posXLabel;
		wxTextCtrl* m_posXCtrl;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYLabel;
		wxTextCtrl* m_posYCtrl;
		wxStaticText* m_posYUnits;
		wxStaticText* m_sizeXLabel;
		wxTextCtrl* m_sizeXCtrl;
		wxStaticText* m_sizeXUnits;
		wxStaticText* m_sizeYLabel;
		wxTextCtrl* m_sizeYCtrl;
		wxStaticText* m_sizeYUnits;
		wxStaticText* m_PadOrientText;
		wxComboBox* m_orientation;
		wxStaticText* m_staticText491;
		wxStaticText* m_offsetXLabel;
		wxTextCtrl* m_offsetXCtrl;
		wxStaticText* m_offsetXUnits;
		wxStaticText* m_offsetYLabel;
		wxTextCtrl* m_offsetYCtrl;
		wxStaticText* m_offsetYUnits;
		wxStaticText* m_padToDieLabel;
		wxTextCtrl* m_padToDieCtrl;
		wxStaticText* m_padToDieUnits;
		wxStaticText* m_trapDeltaLabel;
		wxTextCtrl* m_trapDeltaCtrl;
		wxStaticText* m_trapDeltaUnits;
		wxStaticText* m_trapAxisLabel;
		wxChoice* m_trapAxisCtrl;
		wxStaticText* m_staticTextCornerSizeRatio;
		TEXT_CTRL_EVAL* m_tcCornerSizeRatio;
		wxStaticText* m_staticTextCornerSizeRatioUnit;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_tcCornerRadius;
		wxStaticText* m_cornerRadiusUnits;
		wxStaticText* m_staticTextChamferRatio;
		TEXT_CTRL_EVAL* m_tcChamferRatio;
		wxStaticText* m_staticTextChamferRatioUnit;
		wxStaticText* m_staticTextChamferCorner;
		wxCheckBox* m_cbTopLeft;
		wxCheckBox* m_cbTopRight;
		wxCheckBox* m_cbBottomLeft;
		wxCheckBox* m_cbBottomRight;
		wxStaticText* m_holeShapeLabel;
		wxChoice* m_holeShapeCtrl;
		wxStaticText* m_staticText51;
		wxStaticText* m_holeXLabel;
		wxTextCtrl* m_holeXCtrl;
		wxStaticText* m_holeXUnits;
		wxStaticText* m_holeYLabel;
		wxTextCtrl* m_holeYCtrl;
		wxStaticText* m_holeYUnits;
		wxBoxSizer* m_FlippedWarningSizer;
		wxStaticBitmap* m_FlippedWarningIcon;
		wxStaticText* m_staticText86;
		wxStaticText* m_staticText511;
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
		wxPanel* m_localSettingsPanel;
		wxStaticText* m_staticTextInfoPosValue;
		wxStaticText* m_staticTextInfoNegVal;
		wxStaticText* m_clearanceLabel;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticText* m_maskClearanceLabel;
		wxTextCtrl* m_maskClearanceCtrl;
		wxStaticText* m_maskClearanceUnits;
		wxStaticText* m_pasteClearanceLabel;
		wxTextCtrl* m_pasteClearanceCtrl;
		wxStaticText* m_pasteClearanceUnits;
		wxStaticText* m_staticTextRatio;
		TEXT_CTRL_EVAL* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxSimplebook* m_nonCopperWarningBook;
		wxStaticText* m_nonCopperNote;
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
		wxButton* m_buttonDel;
		wxButton* m_buttonEditShape;
		wxButton* m_buttonAddShape;
		wxButton* m_buttonDup;
		wxButton* m_buttonGeometry;
		wxStaticText* m_parentInfoLine1;
		wxStaticText* m_parentInfoLine2;
		wxPanel* m_panelShowPad;
		PCB_DRAW_PANEL_GAL* m_panelShowPadGal;
		KIGFX::GAL_DISPLAY_OPTIONS m_galOptions;
		wxCheckBox* m_cbShowPadOutline;
		wxStaticLine* m_staticline13;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadShapeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerSizePercentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerRadiusChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUINonCopperWarning( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPrimitiveDClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnPrimitiveSelection( wxListEvent& event ) { event.Skip(); }
		virtual void onDeletePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onAddPrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDuplicatePrimitive( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGeometryTransform( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintShowPanel( wxPaintEvent& event ) { event.Skip(); }
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
		wxStaticText* m_staticTextInfo;
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
		wxPanel* m_panelPoly;
		wxStaticBitmap* m_warningIcon;
		wxStaticText* m_warningText;
		wxStaticLine* m_staticline3;
		wxStaticText* m_statusLine1;
		wxStaticText* m_statusLine2;
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

#endif //__DIALOG_PAD_PROPERTIES_BASE_H__
