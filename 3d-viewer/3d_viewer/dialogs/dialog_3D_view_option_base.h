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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/slider.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_3D_VIEW_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_3D_VIEW_OPTIONS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_panelDspOpt;
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
		wxStaticText* m_staticTextRotAngle;
		wxSpinCtrlDouble* m_spinCtrlRotationAngle;
		wxStaticText* m_staticTextRotAngleUnits;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkBoxEnableAnimation;
		wxStaticText* m_staticAnimationSpeed;
		wxSlider* m_sliderAnimationSpeed;
		wxPanel* m_panelOpenGL;
		wxCheckBox* m_checkBoxBoundingBoxes;
		wxCheckBox* m_checkBoxCuThickness;
		wxCheckBox* m_checkBoxHighlightOnRollOver;
		wxStaticText* m_staticText221;
		wxChoice* m_choiceAntiAliasing;
		wxStaticText* m_staticText231;
		wxColourPickerCtrl* m_colourPickerSelection;
		wxCheckBox* m_checkBoxDisableAAMove;
		wxCheckBox* m_checkBoxDisableMoveThickness;
		wxCheckBox* m_checkBoxDisableMoveVias;
		wxCheckBox* m_checkBoxDisableMoveHoles;
		wxPanel* m_panelRaytracing;
		wxNotebook* m_notebook2;
		wxPanel* m_panelRaytracingCfg;
		wxCheckBox* m_checkBoxRaytracing_proceduralTextures;
		wxCheckBox* m_checkBoxRaytracing_addFloor;
		wxCheckBox* m_checkBoxRaytracing_antiAliasing;
		wxCheckBox* m_checkBoxRaytracing_postProcessing;
		wxStaticText* m_staticText19;
		wxStaticText* m_staticText201;
		wxStaticText* m_staticText211;
		wxCheckBox* m_checkBoxRaytracing_renderShadows;
		wxSpinCtrl* m_spinCtrl_NrSamples_Shadows;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Shadows;
		wxCheckBox* m_checkBoxRaytracing_showReflections;
		wxSpinCtrl* m_spinCtrl_NrSamples_Reflections;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Reflections;
		wxSpinCtrl* m_spinCtrlRecursiveLevel_Reflections;
		wxCheckBox* m_checkBoxRaytracing_showRefractions;
		wxSpinCtrl* m_spinCtrl_NrSamples_Refractions;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Refractions;
		wxSpinCtrl* m_spinCtrlRecursiveLevel_Refractions;
		wxPanel* m_panelLightsConfig;
		wxStaticText* m_staticText17;
		wxColourPickerCtrl* m_colourPickerCameraLight;
		wxStaticText* m_staticText5;
		wxColourPickerCtrl* m_colourPickerTopLight;
		wxStaticText* m_staticText6;
		wxColourPickerCtrl* m_colourPickerBottomLight;
		wxStaticText* m_staticText20;
		wxStaticText* m_staticText18;
		wxStaticText* m_staticText27;
		wxStaticText* m_staticText28;
		wxStaticText* m_staticText21;
		wxColourPickerCtrl* m_colourPickerLight1;
		wxSpinCtrl* m_spinCtrlLightElevation1;
		wxSpinCtrl* m_spinCtrlLightAzimuth1;
		wxStaticText* m_staticText22;
		wxColourPickerCtrl* m_colourPickerLight5;
		wxSpinCtrl* m_spinCtrlLightElevation5;
		wxSpinCtrl* m_spinCtrlLightAzimuth5;
		wxStaticText* m_staticText23;
		wxColourPickerCtrl* m_colourPickerLight2;
		wxSpinCtrl* m_spinCtrlLightElevation2;
		wxSpinCtrl* m_spinCtrlLightAzimuth2;
		wxStaticText* m_staticText24;
		wxColourPickerCtrl* m_colourPickerLight6;
		wxSpinCtrl* m_spinCtrlLightElevation6;
		wxSpinCtrl* m_spinCtrlLightAzimuth6;
		wxStaticText* m_staticText25;
		wxColourPickerCtrl* m_colourPickerLight3;
		wxSpinCtrl* m_spinCtrlLightElevation3;
		wxSpinCtrl* m_spinCtrlLightAzimuth3;
		wxStaticText* m_staticText26;
		wxColourPickerCtrl* m_colourPickerLight7;
		wxSpinCtrl* m_spinCtrlLightElevation7;
		wxSpinCtrl* m_spinCtrlLightAzimuth7;
		wxStaticText* m_staticText171;
		wxColourPickerCtrl* m_colourPickerLight4;
		wxSpinCtrl* m_spinCtrlLightElevation4;
		wxSpinCtrl* m_spinCtrlLightAzimuth4;
		wxStaticText* m_staticText181;
		wxColourPickerCtrl* m_colourPickerLight8;
		wxSpinCtrl* m_spinCtrlLightElevation8;
		wxSpinCtrl* m_spinCtrlLightAzimuth8;
		wxButton* m_buttonLightsResetToDefaults;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckRealisticMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckEnableAnimation( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLightsResetToDefaults( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 810,484 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_3D_VIEW_OPTIONS_BASE();

};

