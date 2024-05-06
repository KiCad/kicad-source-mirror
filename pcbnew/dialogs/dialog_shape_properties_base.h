///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf0)
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
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>
#include <widgets/net_selector.h>
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
		wxBoxSizer* m_upperSizer;
		wxGridBagSizer* m_sizerStartEnd;
		wxStaticText* m_startPointLabel;
		wxStaticText* m_startXLabel;
		wxTextCtrl* m_startXCtrl;
		wxStaticText* m_startXUnits;
		wxStaticText* m_startYLabel;
		wxTextCtrl* m_startYCtrl;
		wxStaticText* m_startYUnits;
		wxStaticText* m_endPointLabel;
		wxStaticText* m_endXLabel;
		wxTextCtrl* m_endXCtrl;
		wxStaticText* m_endXUnits;
		wxStaticText* m_endYLabel;
		wxTextCtrl* m_endYCtrl;
		wxStaticText* m_endYUnits;
		wxGridBagSizer* m_sizerBezier;
		wxStaticText* m_bezierCtrlPt1Label;
		wxStaticText* m_BezierPointC1XLabel;
		wxTextCtrl* m_BezierC1X_Ctrl;
		wxStaticText* m_BezierPointC1XUnit;
		wxStaticText* m_BezierPointC1YLabel;
		wxTextCtrl* m_BezierC1Y_Ctrl;
		wxStaticText* m_BezierPointC1YUnit;
		wxStaticText* m_bezierCtrlPt2Label;
		wxStaticText* m_BezierPointC2XLabel;
		wxTextCtrl* m_BezierC2X_Ctrl;
		wxStaticText* m_BezierPointC2XUnit;
		wxStaticText* m_BezierPointC2YLabel;
		wxTextCtrl* m_BezierC2Y_Ctrl;
		wxStaticText* m_BezierPointC2YUnit;
		wxStaticText* m_segmentLengthLabel;
		wxTextCtrl* m_segmentLengthCtrl;
		wxStaticText* m_segmentLengthUnits;
		wxStaticText* m_segmentAngleLabel;
		wxTextCtrl* m_segmentAngleCtrl;
		wxStaticText* m_segmentAngleUnits;
		wxStaticText* m_rectangleHeightLabel;
		wxTextCtrl* m_rectangleHeightCtrl;
		wxStaticText* m_rectangleHeightUnits;
		wxStaticText* m_rectangleWidthLabel;
		wxTextCtrl* m_rectangleWidthCtrl;
		wxStaticText* m_rectangleWidthUnits;
		wxStaticText* m_angleLabel;
		wxTextCtrl* m_angleCtrl;
		wxStaticText* m_angleUnits;
		wxCheckBox* m_locked;
		wxCheckBox* m_filledCtrl;
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_thicknessUnits;
		wxStaticText* m_lineStyleLabel;
		wxBitmapComboBox* m_lineStyleCombo;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticText* m_netLabel;
		NET_SELECTOR* m_netSelector;
		wxStdDialogButtonSizer* m_StandardButtonsSizer;
		wxButton* m_StandardButtonsSizerOK;
		wxButton* m_StandardButtonsSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onFilledCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLayerSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SHAPE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("%s Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SHAPE_PROPERTIES_BASE();

};

