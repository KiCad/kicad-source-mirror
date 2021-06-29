///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/slider.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_3D_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_3D_DISPLAY_OPTIONS_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_checkBox3DshapesTH;
		wxCheckBox* m_checkBox3DshapesSMD;
		wxCheckBox* m_checkBox3DshapesVirtual;
		wxCheckBox* m_checkBoxSilkscreen;
		wxCheckBox* m_checkBoxSolderMask;
		wxCheckBox* m_checkBoxSolderpaste;
		wxCheckBox* m_checkBoxAdhesive;
		wxCheckBox* m_checkBoxComments;
		wxCheckBox* m_checkBoxECO;
		wxCheckBox* m_checkBoxRealisticMode;
		wxCheckBox* m_checkBoxBoardBody;
		wxCheckBox* m_checkBoxAreas;
		wxCheckBox* m_checkBoxSubtractMaskFromSilk;
		wxCheckBox* m_checkBoxClipSilkOnViaAnnulus;
		wxCheckBox* m_checkBoxRenderPlatedPadsAsPlated;
		wxStaticText* m_materialPropertiesLabel;
		wxChoice* m_materialProperties;
		wxStaticText* m_staticTextRotAngle;
		wxSpinCtrlDouble* m_spinCtrlRotationAngle;
		wxStaticText* m_staticTextRotAngleUnits;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkBoxEnableAnimation;
		wxStaticText* m_staticAnimationSpeed;
		wxSlider* m_sliderAnimationSpeed;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckRealisticMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckEnableAnimation( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_3D_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_3D_DISPLAY_OPTIONS_BASE();

};

