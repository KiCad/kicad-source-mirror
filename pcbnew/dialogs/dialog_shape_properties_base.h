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
#include <wx/gdicmn.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
#include <widgets/net_selector.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SHAPE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SHAPE_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebookShapeDefs;
		wxPanel* m_rectangleByCorners;
		wxGridBagSizer* m_gbsRectangleByCorners;
		wxPanel* m_rectangleByCornerSize;
		wxGridBagSizer* m_gbsRectangleByCornerSize;
		wxPanel* m_rectangleByCenterSize;
		wxGridBagSizer* m_gbsRectangleByCenterSize;
		wxPanel* m_lineByEnds;
		wxGridBagSizer* m_gbsLineByEnds;
		wxPanel* m_lineByLengthAngle;
		wxGridBagSizer* m_gbsLineByLengthAngle;
		wxPanel* m_lineByStartMid;
		wxGridBagSizer* m_gbsLineByStartMid;
		wxPanel* m_arcByCSA;
		wxGridBagSizer* m_gbsArcByCSA;
		wxPanel* m_arcBySME;
		wxGridBagSizer* m_gbsArcBySME;
		wxPanel* m_circle;
		wxGridBagSizer* m_gbsCircleCenterRadius;
		wxPanel* m_circleCenterPoint;
		wxGridBagSizer* m_gbsCircleCenterPoint;
		wxPanel* m_bezier;
		wxGridBagSizer* m_gbsBezier;
		wxCheckBox* m_locked;
		wxCheckBox* m_cbRoundRect;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxBoxSizer* m_upperSizer;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxStaticText* m_lineStyleLabel;
		wxBitmapComboBox* m_lineStyleCombo;
		wxStaticText* m_fillLabel;
		wxChoice* m_fillCtrl;
		wxStaticText* m_netLabel;
		NET_SELECTOR* m_netSelector;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticText* m_techLayersLabel;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_hasSolderMask;
		wxStaticText* m_solderMaskMarginLabel;
		wxTextCtrl* m_solderMaskMarginCtrl;
		wxStaticText* m_solderMaskMarginUnit;
		wxStdDialogButtonSizer* m_StandardButtonsSizer;
		wxButton* m_StandardButtonsSizerOK;
		wxButton* m_StandardButtonsSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onRoundedRectChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCornerRadius( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLayerSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTechLayersChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("%s Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SHAPE_PROPERTIES_BASE();

};

