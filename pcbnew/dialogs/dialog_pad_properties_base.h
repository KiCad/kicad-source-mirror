///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
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
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <pcb_draw_panel_gal.h>
#include <wx/button.h>
#include <wx/dialog.h>

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
		wxTextCtrl* m_PadPosition_X_Ctrl;
		wxStaticText* m_PadPosX_Unit;
		wxStaticText* m_staticText41;
		wxTextCtrl* m_PadPosition_Y_Ctrl;
		wxStaticText* m_PadPosY_Unit;
		wxStaticText* m_staticText12;
		wxTextCtrl* m_ShapeSize_X_Ctrl;
		wxStaticText* m_PadShapeSizeX_Unit;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_ShapeSize_Y_Ctrl;
		wxStaticText* m_PadShapeSizeY_Unit;
		wxStaticText* m_staticText48;
		wxChoice* m_PadOrient;
		wxStaticText* m_staticText491;
		wxStaticText* m_PadOrientText;
		wxTextCtrl* m_PadOrientCtrl;
		wxStaticText* m_customOrientUnits;
		wxStaticText* m_staticText17;
		wxTextCtrl* m_ShapeOffset_X_Ctrl;
		wxStaticText* m_PadShapeOffsetX_Unit;
		wxStaticText* m_staticText19;
		wxTextCtrl* m_ShapeOffset_Y_Ctrl;
		wxStaticText* m_PadShapeOffsetY_Unit;
		wxStaticText* m_staticText38;
		wxTextCtrl* m_LengthPadToDieCtrl;
		wxStaticText* m_PadLengthDie_Unit;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_ShapeDelta_Ctrl;
		wxStaticText* m_PadShapeDelta_Unit;
		wxStaticText* m_staticText23;
		wxChoice* m_trapDeltaDirChoice;
		wxStaticText* m_staticText521;
		wxStaticText* m_staticTitleModuleRot;
		wxStaticText* m_staticModuleRotValue;
		wxStaticText* m_staticTitleModuleSide;
		wxStaticText* m_staticModuleSideValue;
		wxStaticText* m_staticText47;
		wxChoice* m_DrillShapeCtrl;
		wxStaticText* m_staticText51;
		wxStaticText* m_textPadDrillX;
		wxTextCtrl* m_PadDrill_X_Ctrl;
		wxStaticText* m_PadDrill_X_Unit;
		wxStaticText* m_textPadDrillY;
		wxTextCtrl* m_PadDrill_Y_Ctrl;
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
		wxPanel* m_localSettingsPanel;
		wxStaticText* m_staticTextNetClearance;
		wxTextCtrl* m_NetClearanceValueCtrl;
		wxStaticText* m_NetClearanceUnits;
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_SolderMaskMarginCtrl;
		wxStaticText* m_SolderMaskMarginUnits;
		wxStaticText* m_staticTextSolderPaste;
		wxTextCtrl* m_SolderPasteMarginCtrl;
		wxStaticText* m_SolderPasteMarginUnits;
		wxStaticText* m_staticTextRatio;
		wxTextCtrl* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxStaticText* m_staticText40;
		wxChoice* m_ZoneConnectionChoice;
		wxStaticText* m_staticText53;
		wxStaticText* m_staticText49;
		wxTextCtrl* m_ThermalWidthCtrl;
		wxStaticText* m_ThermalWidthUnits;
		wxStaticText* m_staticText52;
		wxTextCtrl* m_ThermalGapCtrl;
		wxStaticText* m_ThermalGapUnits;
		wxStaticText* m_staticTextWarning;
		wxPanel* m_panelShowPad;
		PCB_DRAW_PANEL_GAL* m_panelShowPadGal;
		wxStaticText* m_staticTextWarningPadFlipped;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadShapeSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetLayers( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDrillShapeSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPaintShowPanel( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void PadPropertiesAccept( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Pad Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSUNKEN_BORDER ); 
		~DIALOG_PAD_PROPERTIES_BASE();
	
};

#endif //__DIALOG_PAD_PROPERTIES_BASE_H__
