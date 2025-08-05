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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RENDER_JOB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RENDER_JOB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputFile;
		wxStaticText* m_formatLabel;
		wxChoice* m_choiceFormat;
		wxStaticText* m_dimensionsLabel;
		wxSpinCtrl* m_spinCtrlWidth;
		wxStaticText* m_staticText17;
		wxStaticText* m_staticText19;
		wxSpinCtrl* m_spinCtrlHeight;
		wxStaticText* m_staticText182;
		wxStaticText* m_presetLabel;
		wxChoice* m_presetCtrl;
		wxStaticText* m_backgroundStyleLabel;
		wxChoice* m_choiceBgStyle;
		wxCheckBox* m_cbUseBoardStackupColors;
		wxCheckBox* m_cbRaytracing_proceduralTextures;
		wxCheckBox* m_cbRaytracing_addFloor;
		wxCheckBox* m_cbRaytracing_antiAliasing;
		wxCheckBox* m_cbRaytracing_postProcessing;
		wxStaticText* m_sideLabel;
		wxChoice* m_choiceSide;
		wxStaticText* m_zoomLabel;
		wxSpinCtrlDouble* m_spinCtrlZoom;
		wxRadioBox* m_radioProjection;
		wxStaticText* m_labelX;
		wxStaticText* m_labelY;
		wxStaticText* m_labelZ;
		wxStaticText* m_labelxx;
		wxSpinCtrlDouble* m_spinCtrlPivotX;
		wxSpinCtrlDouble* m_spinCtrlPivotY;
		wxSpinCtrlDouble* m_spinCtrlPivotZ;
		wxStaticText* m_labelMM1;
		wxStaticText* m_labelxx2;
		wxSpinCtrlDouble* m_spinCtrlPanX;
		wxSpinCtrlDouble* m_spinCtrlPanY;
		wxSpinCtrlDouble* m_spinCtrlPanZ;
		wxStaticText* m_labelMM2;
		wxStaticText* m_labelxx21;
		wxSpinCtrlDouble* m_spinCtrlRotX;
		wxSpinCtrlDouble* m_spinCtrlRotY;
		wxSpinCtrlDouble* m_spinCtrlRotZ;
		wxStaticText* m_labelDeg1;
		wxStaticText* m_TopLabel;
		wxSpinCtrlDouble* m_spinCtrlLightsTop;
		wxStaticText* m_bottomLabel;
		wxSpinCtrlDouble* m_spinCtrlLightsBottom;
		wxStaticText* m_sidesLabel;
		wxSpinCtrlDouble* m_spinCtrlLightsSides;
		wxStaticText* m_cameraLabel;
		wxSpinCtrlDouble* m_spinCtrlLightsCamera;
		wxStaticText* m_labelSideLightsElevation;
		wxSpinCtrl* m_spinCtrlLightsSideElevation;
		wxStaticText* m_labelDegrees;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFormatChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_RENDER_JOB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Render PCB Job Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_RENDER_JOB_BASE();

};

