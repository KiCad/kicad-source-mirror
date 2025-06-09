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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_3D_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_3D_DISPLAY_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_renderOptionsLabel;
		wxStaticLine* m_staticline4;
		wxCheckBox* m_checkBoxClipSilkOnViaAnnulus;
		wxCheckBox* m_checkBoxSubtractMaskFromSilk;
		wxCheckBox* m_checkBoxAreas;
		wxCheckBox* m_checkBoxRenderPlatedPadsAsPlated;
		wxStaticText* m_materialPropertiesLabel;
		wxChoice* m_materialProperties;
		wxStaticText* m_cameraOptionsLabel;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticTextRotAngle;
		wxSpinCtrlDouble* m_spinCtrlRotationAngle;
		wxStaticText* m_staticTextRotAngleUnits;
		wxCheckBox* m_checkBoxEnableAnimation;
		wxStaticText* m_staticAnimationSpeed;
		wxSlider* m_sliderAnimationSpeed;

		// Virtual event handlers, override them in your derived class
		virtual void OnCheckEnableAnimation( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_3D_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_3D_DISPLAY_OPTIONS_BASE();

};

